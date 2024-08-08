#ifndef UTIL_H
#define UTIL_H

#include <time.h>
#include <math.h>

#define eps 1.0e-6

static inline int max(int a, int b) { return a < b ? b : a; }
static inline int min(int a, int b) { return a < b ? a : b; }
static inline int clamp(int a, int b, int c) { return max(b, min(a, c)); }
static inline double fclamp(double a, double b, double c) { return fmax(b, fmin(a, c)); }
 
char *iso8601tm(char *str, const struct tm *tm);
char *iso8601local(char *str, time_t t);

int rmdir_r(const char *path);

void kill_parent(void);

#endif
