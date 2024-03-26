#ifndef UTIL_H
#define UTIL_H

#include <time.h>

char *iso8601tm(char *str, const struct tm *tm);
char *iso8601local(char *str, time_t t);

int rmdir_r(const char *path);

#endif
