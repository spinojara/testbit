#ifndef TOGGLE_H
#define TOGGLE_H

#include <ncurses.h>

struct toggle {
	int initial;
	int state;
	int y, x;
	int n;

	WINDOW *win;
};

void new_toggle(struct toggle *t, WINDOW *win, int y, int x, int n, int state);

void draw_toggle(struct toggle *t);

void toggle_toggle(struct toggle *t);

void reset_toggle(struct toggle *t);

#endif
