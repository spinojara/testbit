#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv) {
	if (argc != 2) {
		fprintf(stderr, "usage: %s tc\n", argv[0]);
		return 1;
	}
	
	FILE *f = fopen("/etc/bitbit/tcfactor", "r");
	if (!f) {
		fprintf(stderr, "error: failed to open file /etc/bitbit/tcfactor\n");
		return 2;
	}

	char buf[BUFSIZ] = { 0 };
	fgets(buf, sizeof(buf), f);

	char *endptr;
	errno = 0;
	double tcfactor = strtod(buf, &endptr);
	if (errno || *endptr != '\n' || tcfactor < 1e-6) {
		fprintf(stderr, "error: poorly formatted file /etc/bitbit/tcfactor\n");
		return 3;
	}

	char *ptr;

	char *tc = argv[1];

	char *moves = NULL;
	char *maintime = NULL;
	char *increment = NULL;

	if ((ptr = strchr(tc, '/'))) {
		moves = tc;
		tc = ptr + 1;
		ptr[0] = '\0';
	}
	maintime = tc;
	if ((ptr = strchr(maintime, '+'))) {
		increment = ptr + 1;
		ptr[0] = '\0';
	}

	long movesn;
	double maintimen;
	double incrementn;

	if (moves) {
		errno = 0;
		movesn = strtol(moves, &endptr, 10);
		if (errno || *endptr != '\0' || movesn <= 0) {
			fprintf(stderr, "error: bad tc\n");
			return 4;
		}
	}

	errno = 0;
	maintimen = strtod(maintime, &endptr);
	/* Should have at least 1 ms maintime.
	 * It can still be less than 1 ms after adjusting.
	 */
	if (errno || *endptr != '\0' || maintimen < 1e-3) {
		fprintf(stderr, "error: bad tc\n");
		return 5;
	}

	if (increment) {
		errno = 0;
		incrementn = strtod(increment, &endptr);
		if (errno || *endptr != '\0' || incrementn < 0) {
			fprintf(stderr, "error: bad tc\n");
			return 6;
		}
	}

	if (moves)
		printf("%ld/", movesn);
	printf("%lf", tcfactor * maintimen);
	if (increment)
		printf("+%lf", tcfactor * incrementn);
	printf("\n");

	return 0;
}
