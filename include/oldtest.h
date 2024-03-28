#ifndef OLDTEST_H
#define OLDTEST_H

#include "state.h"

enum {
	OLDTESTACTIVE,
	OLDTESTDONE,
	OLDTESTCANCELLED,
	OLDTESTFAILED,

	OLDTESTSINGLE,
};

void handle_oldtest(struct oldteststate *os, chtype ch);

void draw_oldtest(struct oldteststate *os, int lazy, int load);

void resize_oldtest(struct oldteststate *os);

void draw_dynamic(struct oldteststate *os, void (*attr)(const struct oldteststate *os, int i, int j), ...);

int powi(int a, int n);

char *fstr(char *s, double f, int n);

void free_oldtest(struct oldteststate *os);

#endif
