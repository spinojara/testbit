#include "cgroup.h"

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <poll.h>

int echo(const char *file, const char *str) {
	FILE *f = fopen(file, "w");
	if (!f) {
		fprintf(stderr, "cannot open %s\n", file);
		return 1;
	}

	if (fprintf(f, "%s\n", str) != (int)strlen(str) + 1) {
		fprintf(stderr, "cannot write to %s\n", file);
		fclose(f);
		return 2;
	}

	return fclose(f);
}

int cat(const char *file, char *buf, size_t size) {
	int fd = open(file, O_RDONLY);
	if (fd < 0)
		return 1;

	ssize_t ret;
	do {
		ret = read(fd, buf, size - 1);
	} while (ret == -1 && errno == EINTR);

	if (ret == -1) {
		close(fd);
		return 1;
	}
	buf[ret] = '\0';
	close(fd);
	return 0;
}

void print_cpu(const struct cpu *cpu) {
	printf("cpu: %d", cpu->cpu);
	if (cpu->performance)
		printf(" (p)");
	if (cpu->cpu0)
		printf(" (cpu0)");
	printf(" [");
	for (int i = 0; i < cpu->n_thread_siblings; i++)
		printf("%d,", cpu->thread_siblings[i]);
	printf("]\n");
}

void free_cpus(struct cpus *cpus) {
	for (int i = 0; i < cpus->n; i++) {
		struct cpu *cpu = &cpus->cpus[i];
		free(cpu->thread_siblings);
	}
	free(cpus->cpus);
	cpus->cpus = NULL;
	cpus->n = 0;
}

void append_cpu(struct cpus *cpus, int cpu) {
	cpus->n++;
	cpus->cpus = realloc(cpus->cpus, cpus->n * sizeof(*cpus->cpus));
	if (!cpus->cpus)
		exit(124);
	memset(&cpus->cpus[cpus->n - 1], 0, sizeof(*cpus->cpus));
	cpus->cpus[cpus->n - 1].cpu = cpu;
}

int is_performance(int cpu) {
	char buf[4096];
	if (cat("/sys/devices/cpu_core/cpus", buf, sizeof(buf)))
		return 1;

	struct cpus cpus = { 0 };
	if (parse_cpus(&cpus, buf, -1)) {
		free_cpus(&cpus);
		exit(131);
	}

	int performance = 0;
	for (int i = 0; i < cpus.n; i++)
		if (cpu == cpus.cpus[i].cpu)
			performance = 1;

	free_cpus(&cpus);
	return performance;
}

int parse_cpus(struct cpus *cpus, const char *str, int prev) {
	if (*str < '0' || *str > '9')
		return 1;

	char *endptr = NULL;
	errno = 0;
	int cpu = strtol(str, &endptr, 10);
	if (cpu < 0 || errno)
		return 1;

	if (prev != -1) {
		if (prev > cpu)
			return 1;
		for (int i = prev + 1; i < cpu; i++)
			append_cpu(cpus, i);
	}
	append_cpu(cpus, cpu);

	if (*endptr == '\0' || *endptr == '\n') {
		return 0;
	}
	else if (*endptr == '-') {
		if (prev != -1)
			return 1;
		return parse_cpus(cpus, endptr + 1, cpu);
	}
	else if (*endptr == ',') {
		return parse_cpus(cpus, endptr + 1, -1);
	}
	return 1;
}

void cpuset_cpus_effective(struct cpus *cpus) {
	char buf[4096];
	if (cat("/sys/fs/cgroup/cpuset.cpus.effective", buf, sizeof(buf)))
		exit(130);
	parse_cpus(cpus, buf, -1);

	for (int i = 0; i < cpus->n; i++) {
		struct cpu *cpu = &cpus->cpus[i];
		cpu->performance = is_performance(cpu->cpu);
		struct cpus thread_siblings = { 0 };
		char file[4096];
		sprintf(file, "/sys/devices/system/cpu/cpu%d/topology/thread_siblings_list", cpu->cpu);
		cat(file, buf, sizeof(buf));
		parse_cpus(&thread_siblings, buf, -1);

		for (int j = 0; j < thread_siblings.n; j++) {
			struct cpu *thread_sibling = &thread_siblings.cpus[j];
			if (thread_sibling->cpu == 0)
				cpu->cpu0 = 1;

			if (thread_sibling->cpu != cpu->cpu) {
				cpu->n_thread_siblings++;
				cpu->thread_siblings = realloc(cpu->thread_siblings, cpu->n_thread_siblings * sizeof(*cpu->thread_siblings));
				cpu->thread_siblings[cpu->n_thread_siblings - 1] = thread_sibling->cpu;
			}
		}
		free_cpus(&thread_siblings);
	}
}

int cpu_strategy(struct cpus *cpus, int n) {
	struct cpus all = { 0 };

	cpuset_cpus_effective(&all);

	for (int i = 0; i < all.n; i++) {
		struct cpu *a = &all.cpus[i];
		int skip = 0;
		for (int j = 0; j < cpus->n; j++) {
			struct cpu *cpu = &cpus->cpus[j];
			for (int k = 0; k < cpu->n_thread_siblings; k++)
				if (a->cpu == cpu->thread_siblings[k])
					skip = 1;
		}
		if (skip || a->cpu0 || cpus->n >= n) {
			free(a->thread_siblings);
			continue;
		}

		cpus->n++;
		cpus->cpus = realloc(cpus->cpus, cpus->n * sizeof(*cpus->cpus));
		memcpy(&cpus->cpus[cpus->n - 1], a, sizeof(*a));
	}

	free(all.cpus);

	if (cpus->n < n)
		return 1;

	for (int i = 0; i < cpus->n; i++)
		print_cpu(&cpus->cpus[i]);

	return 0;
}

int claim_cpu(struct cpu *cpu) {
	if (cpu->claimed)
		return 0;
	cpu->claimed = 1;
	char file[4096];
	sprintf(file, "/sys/fs/cgroup/testbit-%d", cpu->cpu);
	if (mkdir(file, 0555)) {
		fprintf(stderr, "error: failed to mkdir\n");
		return 1;
	}

	sprintf(file, "/sys/fs/cgroup/testbit-%d/cpuset.cpus", cpu->cpu);
	char cpustr[4096];
	sprintf(cpustr, "%d", cpu->cpu);
	if (echo(file, cpustr)) {
		fprintf(stderr, "error: failed to isolate cpu\n");
		return 1;
	}

	sprintf(file, "/sys/fs/cgroup/testbit-%d/cpuset.cpus.partition", cpu->cpu);
	if (echo(file, "isolated")) {
		fprintf(stderr, "error: failed to make cgroup isolated\n");
		return 1;
	}

	for (int i = 0; i < cpu->n_thread_siblings; i++) {
		sprintf(file, "/sys/devices/system/cpu/cpu%d/online", cpu->thread_siblings[i]);
		if (echo(file, "0"))
			return 1;
	}

	return 0;
}

int read_populated(int fd) {
	char buf[256];
	if (lseek(fd, 0, SEEK_SET) < 0)
		return -1;
	ssize_t n;
	do {
		n = read(fd, buf, sizeof(buf) - 1);
	} while (n == -1 && errno == EINTR);

	if (n < 0)
		return -1;
	buf[n] = '\0';

	return strstr(buf, "populated 1") != NULL;
}

int release_cpu(struct cpu *cpu) {
	if (!cpu->claimed)
		return 0;

	printf("releasing %d\n", cpu->cpu);

	char file[4096];
	for (int i = 0; i < cpu->n_thread_siblings; i++) {
		sprintf(file, "/sys/devices/system/cpu/cpu%d/online", cpu->thread_siblings[i]);
		if (echo(file, "1"))
			fprintf(stderr, "error: failed to turn cpu%d back online\n", cpu->thread_siblings[i]);
	}

	sprintf(file, "/sys/fs/cgroup/testbit-%d/cpuset.cpus.partition", cpu->cpu);
	if (echo(file, "member"))
		fprintf(stderr, "error: failed to make cgroup testbit-%d non-isolated\n", cpu->cpu);

	sprintf(file, "/sys/fs/cgroup/testbit-%d/cpuset.cpus", cpu->cpu);
	if (echo(file, ""))
		fprintf(stderr, "error: failed to release cpu%d from cgroup testbit-%d\n", cpu->cpu, cpu->cpu);

	sprintf(file, "/sys/fs/cgroup/testbit-%d/cgroup.kill", cpu->cpu);
	if (echo(file, "1"))
		fprintf(stderr, "error: failed to kill cgroup testbit-%d\n", cpu->cpu);
	else {
		/* Wait for cgroup to die. */
		sprintf(file, "/sys/fs/cgroup/testbit-%d/cgroup.events", cpu->cpu);
		int fd = open(file, O_RDONLY | O_CLOEXEC);
		if (fd >= 0) {
			struct pollfd pfd = {
				.fd = fd,
				.events = POLLPRI,
			};
			while (1) {
				int r = poll(&pfd, 1, -1);
				if (r < 0) {
					if (errno == EINTR)
						continue;
					else
						break;
				}
				if (pfd.revents & (POLLPRI | POLLERR)) {
					int populated = read_populated(fd);
					if (populated == 0)
						break;
				}
			}
			close(fd);
		}
	}

	sprintf(file, "/sys/fs/cgroup/testbit-%d", cpu->cpu);
	if (rmdir(file))
		fprintf(stderr, "error: failed to remove cgroup testbit-%d\n", cpu->cpu);

	cpu->claimed = 0;
	return 0;
}
