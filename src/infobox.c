#include "infobox.h"

#include <string.h>

#include <ncurses.h>

#include "color.h"
#include "draw.h"

void infobox(const char *info) {
	int len = strlen(info);
	int size = len + 8;
	int y = (LINES - 6) / 2;
	int x = (COLS - size) / 2;
	WINDOW *win = newwin(6, size, y, x);
	keypad(win, TRUE);

	draw_border(win, &cs.border, &cs.border, &cs.bordershadow, 1, 0, 0, 6, size);
	wattrset(win, cs.text.attr);
	mvwaddstr(win, 2, 3, info);
	wrefresh(win);
	wgetch(win);
	delwin(win);
}
