#ifndef TC_H
#define TC_H

#include <stdlib.h>

int tcadjust(const char *tc, char *adjusted, size_t n);

int tccheck(const char *tc);

int tcinfo(const char *tc, long *moves, double *maintime, double *increment);

#endif
