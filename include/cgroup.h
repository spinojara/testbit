#ifndef CGROUP_H
#define CGROUP_H

struct cpus {
	int n;
	struct cpu *cpus;
};

struct cpu {
	int cpu;
	int claimed;
	int performance;
	int n_thread_siblings;
	int *thread_siblings;
	int cpu0;
};

int parse_cpus(struct cpus *cpus, const char *str, int prev);

void free_cpus(struct cpus *cpus);

void cpuset_cpus_effective(struct cpus *cpus);

int cpu_strategy(struct cpus *cpus, int n);

int claim_cpu(struct cpu *cpu);

int release_cpu(struct cpu *cpu);

#endif
