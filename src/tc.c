#include "tc.h"

#include <stdio.h>
#include <errno.h>
#include <string.h>

int tcinfo(const char *tc, long *moves, double *maintime, double *increment) {
	char copy[128];
	snprintf(copy, 128, "%s", tc);
	copy[127] = '\0';
	char *tcptr = copy;

	char *ptr, *endptr;

	char *mo = NULL;
	char *ma = NULL;
	char *in = NULL;

	if ((ptr = strchr(tcptr, '/'))) {
		mo = tcptr;
		tcptr = ptr + 1;
		ptr[0] = '\0';
	}
	ma = tcptr;
	if ((ptr = strchr(ma, '+'))) {
		in = ptr + 1;
		ptr[0] = '\0';
	}

	if (mo) {
		errno = 0;
		*moves = strtol(mo, &endptr, 10);
		if (errno || *endptr != '\0' || *moves <= 0)
			return 4;
	}
	else
		*moves = 0;

	errno = 0;
	*maintime = strtod(ma, &endptr);
	/* Should have at least 1 ms maintime.
	 * It can still be less than 1 ms after adjusting.
	 */
	if (errno || *endptr != '\0' || *maintime < 1e-3)
		return 5;

	if (in) {
		errno = 0;
		*increment = strtod(in, &endptr);
		if (errno || *endptr != '\0' || *increment < 0)
			return 6;
	}
	else
		*increment = 0.0;

	return 0;
}

int tccheck(const char *tc) {
	long moves;
	double maintime, increment;
	return tcinfo(tc, &moves, &maintime, &increment);
}

int tcadjust(const char *tc, char *adjusted, size_t n) {
	FILE *f = fopen("/etc/bitbit/tcfactor", "r");
	if (!f) {
		fprintf(stderr, "error: failed to open file /etc/bitbit/tcfactor\n");
		return 2;
	}

	char buf[BUFSIZ] = { 0 };
	fgets(buf, sizeof(buf), f);
	fclose(f);

	char *endptr;
	errno = 0;
	double tcfactor = strtod(buf, &endptr);
	if (errno || *endptr != '\n' || tcfactor < 1e-6) {
		fprintf(stderr, "error: poorly formatted file /etc/bitbit/tcfactor\n");
		return 3;
	}

	long moves;
	double maintime;
	double increment;
	if (tcinfo(tc, &moves, &maintime, &increment))
		return 1;

	int total = 0;
	if (moves > 0)
		total += snprintf(adjusted + total, n - total, "%ld/", moves);
	total += snprintf(adjusted + total, n - total, "%lf", tcfactor * maintime);
	if (increment > 0)
		total += snprintf(adjusted + total, n - total, "+%lf", tcfactor * increment);
	adjusted[n - 1] = '\0';

	return 0;
}
