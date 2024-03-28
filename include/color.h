#ifndef COLOR_H
#define COLOR_H

#include <ncurses.h>

struct color {
	chtype attr;

	int fg;
	int bg;
};

struct rgb {
	short r;
	short g;
	short b;
};

struct colors {
	struct color bg;
	struct color border;
	struct color bordershadow;
	struct color shadow;
	struct color error;
	struct color errorhl;
	struct color yellow;
	struct color green;

	struct color text;
	struct color texthl;
	struct color textdim;
	struct color textblue;
};

extern struct colors cs;

void init_colors(void);

#endif
