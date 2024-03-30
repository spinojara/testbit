#include "draw.h"

#include <string.h>

#include "menu.h"
#include "oldtest.h"
#include "newtest.h"

void mvwaddnstrtab(WINDOW *win, int y, int x, const char *str, int n) {
	int x_start = x;
	for (int i = 0; i < n && str[i]; i++) {
		if (str[i] == '\t') {
			int until_tab = TABSIZE - ((x - x_start) % TABSIZE);
			wmove(win, y, x);
			for (int j = 0; j < until_tab; j++)
				waddch(win, ' ');
			x += until_tab;
		}
		else {
			mvwaddch(win, y, x, str[i]);
			x++;
		}
	}
}

void draw_fill(WINDOW *win, struct color *bg, int ymin, int xmin, int ysize, int xsize) {
	wattrset(win, bg->attr);
	for (int y = ymin; y < ymin + ysize; y++)
		mvwhline(win, y, xmin, ' ', xsize);
}

void draw_border(WINDOW *win, struct color *bg, struct color *upper, struct color *lower, int fill, int ymin, int xmin, int ysize, int xsize) {
	if (fill)
		draw_fill(win, &cs.bordershadow, ymin, xmin, ysize, xsize);

	if (bg) {
		ysize -= 1;
		xsize -= 2;
	}

	int ymax = ysize + ymin - 1;
	int xmax = xsize + xmin - 1;

	wattrset(win, upper->attr);
	mvwhline(win, ymin, xmin, 0, xsize);
	mvwvline(win, ymin, xmin, 0, ysize);
	mvwaddch(win, ymin, xmin, ACS_ULCORNER);
	mvwaddch(win, ymax, xmin, ACS_LLCORNER);

	wattrset(win, lower->attr);
	mvwhline(win, ymax, xmin + 1, 0, xsize - 1);
	mvwvline(win, ymin, xmax, 0, ysize);
	mvwaddch(win, ymin, xmax, ACS_URCORNER);
	mvwaddch(win, ymax, xmax, ACS_LRCORNER);

	if (bg) {
		wattrset(win, cs.shadow.attr);
		mvwhline(win, ymax + 1, xmin + 1, ' ', xsize + 1);
		mvwvline(win, ymin + 1, xmax + 1, ' ', ysize - 1);
		mvwvline(win, ymin + 1, xmax + 2, ' ', ysize - 1);

		wattrset(win, bg->attr);
		mvwhline(win, ymax + 1, xmin, ' ', 2);
		mvwhline(win, ymin, xmax + 1, ' ', 2);
	}
}

void draw_main(WINDOW *win) {
	draw_fill(win, &cs.bg, 0, 0, LINES, COLS);
	draw_border(win, &cs.bg, &cs.border, &cs.bordershadow, 1, 1, 2, LINES - 2, COLS - 4);
	wattrset(win, cs.border.attr);
	wrefresh(win);
}

void draw_all(struct state *st) {
	draw_main(stdscr);
	draw_menu(st);
	draw_newtest(&st->ns);
	draw_oldtest(&st->as, 0, st->selected == MENUACTIVE);
	draw_oldtest(&st->ds, 0, st->selected == MENUDONE);
	draw_oldtest(&st->cs, 0, st->selected == MENUCANCELLED);
	draw_oldtest(&st->fs, 0, st->selected == MENUFAILED);

	switch (st->selected) {
	case MENUNEWTEST:
		touchwin(st->ns.win);
		wrefresh(st->ns.win);
		break;
	case MENUACTIVE:
		touchwin(st->as.win);
		wrefresh(st->as.win);
		break;
	case MENUDONE:
		touchwin(st->ds.win);
		wrefresh(st->ds.win);
		break;
	case MENUCANCELLED:
		touchwin(st->cs.win);
		wrefresh(st->cs.win);
		break;
	case MENUFAILED:
		touchwin(st->fs.win);
		wrefresh(st->fs.win);
		break;
	}
}

void resize_oldtests(struct state *st) {
	resize_oldtest(&st->as);
	resize_oldtest(&st->ds);
	resize_oldtest(&st->cs);
	resize_oldtest(&st->fs);
}

void draw_resize(struct state *st) {
	if (LINES < LINESMIN || COLS < COLSMIN) {
		endwin();
		fprintf(stderr, "Terminal needs to be of size at least %dx%d.\n", COLSMIN, LINESMIN);
		/* Close ssl here. */
		exit(11);
	}

	wresize(stdscr, LINES, COLS);

	wresize(st->win, 3, COLS - 10);
	mvwin(st->win, 2, 4);

	wresize(st->ns.win, LINES - 8, COLS - 10);
	mvwin(st->ns.win, 5, 4);

	wresize(st->as.win, LINES - 8, COLS - 10);
	mvwin(st->as.win, 5, 4);

	wresize(st->ds.win, LINES - 8, COLS - 10);
	mvwin(st->ds.win, 5, 4);

	wresize(st->cs.win, LINES - 8, COLS - 10);
	mvwin(st->cs.win, 5, 4);

	wresize(st->fs.win, LINES - 8, COLS - 10);
	mvwin(st->fs.win, 5, 4);

	resize_oldtests(st);

	resize_prompts(&st->ns);

	draw_all(st);
}
