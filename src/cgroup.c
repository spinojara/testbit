#define _POSIX_C_SOURCE 1
#define _GNU_SOURCE
#include "cgroup.h"

#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <sched.h>

struct cpu {
	int cpu;

	int size;
	int count;
	int *cpus;
};

static int claimedcpus_count;
static struct cpu *claimedcpus;

int echo(const char *path, const char *buf) {
	FILE *f = fopen(path, "w");
	if (!f)
		return 1;
	int len = strlen(buf);
	if (fprintf(f, "%s\n", buf) != len + 1)
		return 2;
	if (fclose(f))
		return 3;

	return 0;
}

int cat(const char *path, char *buf, int n) {
	FILE *f = fopen(path, "r");
	if (!f || !fgets(buf, n, f) || fclose(f))
		return 1;
	return 0;
}

void print_cpu(struct cpu *cpu) {
	printf("cpu: %d\n", cpu->cpu);
	printf("cpus:");
	for (int i = 0; i < cpu->count; i++)
		printf(" %d", cpu->cpus[i]);
	printf("\n");
}

int append_cpu(struct cpu *cpu, int newcpu) {
	if (cpu->count >= cpu->size) {
		cpu->cpus = realloc(cpu->cpus, ++cpu->size * sizeof(*cpu->cpus));
		if (!cpu->cpus)
			return 1;
	}
	cpu->cpus[cpu->count++] = newcpu;
	return 0;
}

int parse_cpus(const char *buf, struct cpu *cpus, int exclude) {
	char *endptr;
	int cpu1, cpu2;

	if (*buf == '\n' || *buf == '\0')
		return 0;

	cpu1 = strtol(buf, &endptr, 10);
	if (cpu1 != exclude && append_cpu(cpus, cpu1))
		return 1;
	switch (*endptr) {
	case ',':
		return parse_cpus(endptr + 1, cpus, exclude);
	case '-':
		cpu2 = strtol(endptr + 1, &endptr, 10);
		for (int cpu = cpu1 + 1; cpu <= cpu2; cpu++)
			if (cpu != exclude && append_cpu(cpus, cpu))
				return 1;
		return parse_cpus(endptr + 1, cpus, exclude);
	case '\n':
		return 0;
	default:
		return 1;
	}
}

int cpu_is_online(int cpu) {
	char path[1024], c[2];
	/* Overflow if cpu is too large. */
	sprintf(path, "/sys/devices/system/cpu/cpu%d/online", cpu);
	/* If file does not exist, the cpu is permanently online. */
	if (cat(path, c, 2))
		return 1;
	return *c == '1';
}

int already_claimed(int cpu) {
	for (int i = 0; i < claimedcpus_count; i++) {
		struct cpu *cpus = &claimedcpus[i];
		if (cpu == cpus->cpu)
			return 1;
		for (int j = 0; j < cpus->count; j++)
			if (cpu == cpus->cpus[j])
				return 1;
	}
	return 0;
}

int claim_cpu(int cpu) {
	char path[1024], buf[1024];
	struct cpu *cpus = &claimedcpus[claimedcpus_count++];
	cpus->cpu = cpu;
	/* Overflow if cpu is too large. */
	sprintf(path, "/sys/devices/system/cpu/cpu%d/topology/thread_siblings_list", cpu);
	return cat(path, buf, 1024) || parse_cpus(buf, cpus, cpu);
}

void exit_release_cpus(void) {
	release_cpus();
}

void sig_release_cpus(int n) {
	(void)n;
	release_cpus();
	signal(SIGINT, SIG_DFL);
	kill(getpid(), SIGINT);
}

int claim_cpus(int n) {
	static int already_registered = 0;
	if (n <= 0)
		return 10;
	/* We skip cpu0 by first claiming it as an extra cpu. */
	n++;
	char buf[1024], path[1024];
	if (mkdir("/sys/fs/cgroup/testbit", 0555))
		return 1;

	if (!already_registered) {
		already_registered = 1;
		atexit(&exit_release_cpus);
		signal(SIGINT, &sig_release_cpus);
	}

	char pid[1024];
	sprintf(pid, "%d\n", getpid());
	if (echo("/sys/fs/cgroup/testbit/cgroup.procs", pid))
		return 11;

	if (echo("/sys/fs/cgroup/testbit/cgroup.subtree_control", "+cpuset"))
		return 10;

	claimedcpus = calloc(n, sizeof(*claimedcpus));
	claimedcpus_count = 0;
	if (!claimedcpus)
		return 2;

	struct cpu available = { 0 };
	if (cat("/sys/fs/cgroup/testbit/cpuset.cpus.effective", buf, 1024))
		return 3;
	parse_cpus(buf, &available, -1);
	print_cpu(&available);

	for (int i = 0; i < available.count && claimedcpus_count < n; i++) {
		int cpu = available.cpus[i];
		if (cpu_is_online(cpu) && !already_claimed(cpu))
			if (claim_cpu(cpu))
				return 4;
	}

	if (claimedcpus_count != n)
		return 5;

	FILE *f = fopen("/sys/fs/cgroup/testbit/cpuset.cpus", "w");
	if (!f)
		return 6;
	cpu_set_t mask;
	CPU_ZERO(&mask);
	/* Start from 1 to skip cpu0. */
	for (int i = 1; i < claimedcpus_count; i++) {
		struct cpu *cpus = &claimedcpus[i];
		if (i)
			fprintf(f, ",");
		fprintf(f, "%d", cpus->cpu);
		CPU_SET(cpus->cpu, &mask);
		for (int j = 0; j < cpus->count; j++) {
			sprintf(path, "/sys/devices/system/cpu/cpu%d/online", cpus->cpus[j]);
			if (echo(path, "0"))
				return 7;
		}
	}
	fprintf(f, "\n");
	if (fclose(f))
		return 8;

	if (echo("/sys/fs/cgroup/testbit/cpuset.cpus.partition", "isolated"))
		return 9;

	struct sched_param param = { 99 };
	if (sched_setscheduler(0, SCHED_FIFO, &param))
		return 13;

	struct timespec tp = { 0 };
	sched_rr_get_interval(0, &tp);
	printf("round robin: %ld %ld\n", tp.tv_sec, tp.tv_nsec);

	printf("claimed %d cpus\n", claimedcpus_count - 1);
	for (int i = 1; i < claimedcpus_count; i++)
		print_cpu(&claimedcpus[i]);

	free(available.cpus);
	return 0;
}

int release_cpus() {
	int error = 0;
	char path[1024];
	for (int i = 1; i < claimedcpus_count; i++) {
		struct cpu *cpus = &claimedcpus[i];
		for (int j = 0; j < cpus->count; j++) {
			sprintf(path, "/sys/devices/system/cpu/cpu%d/online", cpus->cpus[j]);
			if (echo(path, "1")) {
				fprintf(stderr, "fatal error: failed to turn cpu%d back online\n", cpus->cpus[j]);
				error = 1;
			}
		}
	}
	claimedcpus_count = 0;

	char pid[1024];
	sprintf(pid, "%d\n", getpid());
	if (echo("/sys/fs/cgroup/cgroup.procs", pid))
		return 2;

	if (echo("/sys/fs/cgroup/testbit/cgroup.kill", "1"))
		return 3;

	if (rmdir("/sys/fs/cgroup/testbit"))
		return 4;

	free(claimedcpus);
	claimedcpus = NULL;
	return error;
}
