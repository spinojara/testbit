#include "win.h"

#include <string.h>

void wfill(WINDOW *win, struct color *bg) {
	int y, x;
	getmaxyx(win, y, x);
	wattrset(win, bg->attr);
	for (y = 0; y < LINES; y++)
		mvwhline(win, y, 0, ' ', x);
}

void wdrawborder(WINDOW *win, struct color *bg, int shadow, int popout) {
	int y, x;
	getmaxyx(win, y, x);
	wfill(win, &cs.bordershadow);

	if (shadow) {
		y -= 1;
		x -= 2;
	}

	if (popout)
		wattrset(win, cs.border.attr);
	else
		wattrset(win, cs.bordershadow.attr);
	mvwhline(win, 0, 0, 0, x);
	mvwvline(win, 0, 0, 0, y);
	mvwaddch(win, 0, 0, ACS_ULCORNER);
	mvwaddch(win, y - 1, 0, ACS_LLCORNER);

	if (popout)
		wattrset(win, cs.bordershadow.attr);
	else
		wattrset(win, cs.border.attr);
	mvwhline(win, y - 1, 1, ACS_HLINE, x - 1);
	mvwvline(win, 0, x - 1, 0, y);
	mvwaddch(win, 0, x - 1, ACS_URCORNER);
	mvwaddch(win, y - 1, x - 1, ACS_LRCORNER);

	if (shadow) {
		wattrset(win, cs.shadow.attr);
		mvwhline(win, y, 2, ' ', x);
		mvwvline(win, 1, x + 1, ' ', y - 1);
		mvwvline(win, 1, x, ' ', y - 1);

		wattrset(win, bg->attr);
		mvwhline(win, y, 0, ' ', 2);
		mvwhline(win, y, 0, ' ', 2);
		mvwhline(win, 0, x, ' ', 2);
	}
}

void wmainwin(WINDOW *win) {
	wdrawborder(win, &cs.bg, 1, 1);
	wrefresh(win);
}

void wtopmenu(WINDOW *win, const struct menustate *ms) {
	wfill(win, &cs.bordershadow);
	char *options[WINCOUNT] = { "", "< Overview >", "< New Test >" };

	for (int i = 1, x = 4; i < WINCOUNT; i++) {
		wattrset(win, i == ms->topmenu ? cs.texthl.attr : cs.text.attr);
		mvwaddstr(win, 1, x, options[i]);
		x += strlen(options[i]) + 5;
	}

	touchwin(win);
	wrefresh(win);
}

void woverview(WINDOW *win) {
	wdrawborder(win, &cs.border, 0, 0);

	wattrset(win, cs.text.attr);
	mvwprintw(win, 3, 4, "Overview");

	touchwin(win);
	wrefresh(win);
}

void wnewtest(WINDOW *win, struct menustate *ms) {
	wdrawborder(win, &cs.border, 0, 0);

	for (int i = NEWTESTTIME; i <= NEWTESTELOE; i++)
		highlight_prompt(&ms->nts.prompt[i], i == ms->nts.newtest);

	touchwin(win);
	wrefresh(win);
}

void wmenus(struct windows *wins, struct menustate *ms) {
	wrefresh(stdscr);

	wmainwin(wins->mainwin);

	wnewtest(wins->win[WINNEWTEST], ms);
	woverview(wins->win[WINOVERVIEW]);

	switch (ms->topmenu) {
	case WINNEWTEST:
		wnewtest(wins->win[WINNEWTEST], ms);
		break;
	case WINOVERVIEW:
		woverview(wins->win[WINOVERVIEW]);
		break;
	}

	wtopmenu(wins->win[WINTOPMENU], ms);
}

void handle_resize(struct windows *wins, struct menustate *ms) {
	wresize(stdscr, LINES, COLS);
	wfill(stdscr, &cs.bg);

	wresize(wins->mainwin, LINES + wins->mainsize[0], COLS + wins->mainsize[1]);
	mvwin(wins->mainwin, wins->mainoffset[0], wins->mainoffset[1]);

	wins->size[WINTOPMENU][0] = 3 - LINES;

	for (int i = 0; i < WINCOUNT; i++) {
		wresize(wins->win[i], LINES + wins->size[i][0], COLS + wins->size[i][1]);
		mvwin(wins->win[i], wins->offset[i][0], wins->offset[i][1]);
	}

	wmenus(wins, ms);
}

void init_windows(struct windows *wins) {
	wins->mainwin = newwin(1, 1, 0, 0);
	keypad(wins->mainwin, TRUE);

	wins->mainsize[0] = -2;
	wins->mainsize[1] = -4;

	wins->mainoffset[0] = 1;
	wins->mainoffset[1] = 2;

	for (int i = 0; i < WINCOUNT; i++) {
		wins->win[i] = newwin(1, 1, 0, 0);
		keypad(wins->win[i], TRUE);
	}

	wins->size[WINTOPMENU][1] = -10;

	wins->size[WINOVERVIEW][0] = -8;
	wins->size[WINOVERVIEW][1] = -10;

	wins->size[WINNEWTEST][0] = -8;
	wins->size[WINNEWTEST][1] = -10;

	wins->offset[WINTOPMENU][0] = 2;
	wins->offset[WINTOPMENU][1] = 4;

	wins->offset[WINOVERVIEW][0] = 5;
	wins->offset[WINOVERVIEW][1] = 4;

	wins->offset[WINNEWTEST][0] = 5;
	wins->offset[WINNEWTEST][1] = 4;
}

void init_menustate(struct menustate *ms, struct windows *wins) {
	new_prompt(&ms->nts.prompt[NEWTESTTIME],  wins->win[WINNEWTEST], 4, 4, "Time Control", "10+0.1", NULL);
	new_prompt(&ms->nts.prompt[NEWTESTELO0],  wins->win[WINNEWTEST], 5, 4, "Elo0        ", "0.0", NULL);
	new_prompt(&ms->nts.prompt[NEWTESTELO1],  wins->win[WINNEWTEST], 6, 4, "Elo1        ", "4.0", NULL);
	new_prompt(&ms->nts.prompt[NEWTESTALPHA], wins->win[WINNEWTEST], 7, 4, "Alpha       ", "0.025", NULL);
	new_prompt(&ms->nts.prompt[NEWTESTBETA],  wins->win[WINNEWTEST], 8, 4, "Beta        ", "0.025", NULL);
	new_prompt(&ms->nts.prompt[NEWTESTELOE],  wins->win[WINNEWTEST], 9, 4, "Elo Error   ", "4.0", NULL);
}

void term_menustate(struct menustate *ms) {
	delete_prompt(&ms->nts.prompt[NEWTESTTIME]);
	delete_prompt(&ms->nts.prompt[NEWTESTELO0]);
	delete_prompt(&ms->nts.prompt[NEWTESTELO1]);
	delete_prompt(&ms->nts.prompt[NEWTESTALPHA]);
	delete_prompt(&ms->nts.prompt[NEWTESTBETA]);
	delete_prompt(&ms->nts.prompt[NEWTESTELOE]);
}

void term_windows(struct windows *wins) {
	for (int i = 0; i < WINCOUNT; i++)
		wins->win[i] = newwin(1, 1, 0, 0);
}
