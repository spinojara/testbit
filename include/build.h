#ifndef BUILD_H
#define BUILD_H

#include <curl/curl.h>

#include "test.h"
#include "cgroup.h"

int git_clone(const char *dir, CURL *curl);

int build_test(struct test *test, const char *patch, const char *simd, const char *commit, CURL *curl);

int fastchess(CURL *curl, const struct cpu *cpu, const char *dir, const char *adjudicate, char *syzygy, char *tc);

#endif
