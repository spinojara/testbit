#ifndef BUILD_H
#define BUILD_H

#include <curl/curl.h>
#include <cJSON.h>

#include "test.h"
#include "cgroup.h"

int git_clone(const char *dir, CURL *curl);

int build_test(struct test *test, int task_id, const char *patch, const char *simd, const char *commit, int tune, CURL *curl, const char *url, int stop_fd);

int fastchess(CURL *curl, const char *url, int id, int task_id, const struct cpu *cpu, const char *dir, const char *adjudicate, char *syzygy, char *tc, cJSON *argsplus, cJSON *argsminus, int stop_fd);

#endif
