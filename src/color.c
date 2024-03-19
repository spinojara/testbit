#include "color.h"

#include <ncurses.h>

/* Inspired from the Linux utility menuconfig's default theme. */

struct colors cs;

#define COLOR(c, f, b, a)   \
	do {                \
		c.fg = f;   \
		c.bg = b;   \
		c.attr = a; \
	} while(0)

void set_theme(void) {
	COLOR(cs.bg,           COLOR_BLUE,  COLOR_BLUE,  A_NORMAL);
	COLOR(cs.border,       COLOR_WHITE, COLOR_WHITE, A_BOLD);
	COLOR(cs.bordershadow, COLOR_BLACK, COLOR_WHITE, A_NORMAL);
	COLOR(cs.shadow,       COLOR_BLACK, COLOR_BLACK, A_NORMAL);

	COLOR(cs.text,         COLOR_BLACK, COLOR_WHITE, A_NORMAL);
	COLOR(cs.texthl,       COLOR_WHITE, COLOR_BLUE,  A_NORMAL | A_BOLD);
	COLOR(cs.textdim,      COLOR_WHITE, COLOR_WHITE, A_DIM);
}

void make_color(struct color *c) {
	static int pairs = 0;
	pairs++;
	init_pair(pairs, c->fg, c->bg);
	c->attr |= COLOR_PAIR(pairs);
}

void make_colors(void) {
	make_color(&cs.bg);
	make_color(&cs.border);
	make_color(&cs.bordershadow);
	make_color(&cs.shadow);

	make_color(&cs.text);
	make_color(&cs.texthl);
	make_color(&cs.textdim);
}

void init_colors(void) {
	if (!has_colors())
		return;

	start_color();

	set_theme();
	make_colors();
}
