#include "tc.h"

#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>

int parsetc(const char *tc, struct tc *dst) {
	if (!tc || !*tc)
		return 1;
	memset(dst, 0, sizeof(*dst));

	char *copy = strdup(tc);
	char *moves = NULL;
	char *maintime = NULL;
	char *increment = NULL;

	char *p;
	if ((p = strchr(copy, '/'))) {
		*p = 0;
		moves = copy;
		p += 1;
	}
	else {
		p = copy;
	}

	char *q;
	if ((q = strchr(p, '+'))) {
		*q = 0;
		maintime = p;
		increment = q + 1;
	}
	else {
		maintime = p;
	}

	int ret = 0;
	char *endptr = NULL;

	errno = 0;
	if ((dst->maintime = strtod(maintime, &endptr)) <= 0.0 || errno || *endptr)
		ret = 1;

	errno = 0;
	if (moves && ((dst->moves = strtoll(moves, &endptr, 10)) <= 0 || errno || *endptr))
		ret = 1;

	errno = 0;
	if (increment && ((dst->increment = strtod(increment, &endptr)) <= 0.0 || errno || *endptr))
		ret = 1;

	free(copy);

	return ret;
}

void adjusttc(struct tc *tc, double factor) {
	tc->maintime *= factor;
	tc->increment *= factor;
}

void tctostr(char *str, struct tc *tc) {
	size_t n = 0;
	n += sprintf(str, "tc=");
	if (tc->moves)
		n += sprintf(str + n, "%lld/", tc->moves);

	n += sprintf(str + n, "%lf", tc->maintime);
	if (tc->increment > 0)
		sprintf(str + n, "+%lf", tc->increment);
}
