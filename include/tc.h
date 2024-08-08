#ifndef TC_H
#define TC_H

#include <stdlib.h>

int tcadjust(const char *tc, char *adjusted, size_t n);

int tccheck(const char *tc);

int tcinfo(char *tc, long *moves, double *maintime, double *increment);

#endif
