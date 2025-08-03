#ifndef CGROUP_H
#define CGROUP_H

int claim_cpus(int n, int *ret);

int release_cpus();

#endif
