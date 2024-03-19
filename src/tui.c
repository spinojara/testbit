#include "tui.h"

#include <ncurses.h>

#include "color.h"
#include "win.h"
#include "newtest.h"

void tuiloop(SSL *ssl) {
	(void)ssl;

	int ch;

	initscr();
	keypad(stdscr, TRUE);
	curs_set(0);
	cbreak();
	noecho();
	init_colors();
	refresh();

	struct windows wins;
	struct menustate ms = { .running = 1, .topmenu = WINOVERVIEW, .nts.newtest = NEWTESTTIME, .ssl = ssl };
	init_windows(&wins);
	init_menustate(&ms, &wins);
	handle_resize(&wins, &ms);

	while (ms.running) {
		ch = getch();
		switch (ch) {
		case KEY_RESIZE:
			handle_resize(&wins, &ms);
			break;
		case 'q':
			ms.running = 0;
			break;
		case 'h':
		case KEY_LEFT:
			if (ms.topmenu > WINOVERVIEW) {
				ms.topmenu--;
				touchwin(wins.win[ms.topmenu]);
				wrefresh(wins.win[ms.topmenu]);
				wtopmenu(wins.win[WINTOPMENU], &ms);
			}
			break;
		case 'l':
		case KEY_RIGHT:
			if (ms.topmenu < WINNEWTEST) {
				ms.topmenu++;
				touchwin(wins.win[ms.topmenu]);
				wrefresh(wins.win[ms.topmenu]);
				wtopmenu(wins.win[WINTOPMENU], &ms);
			}
			break;
		default:
			break;
		}
		
		switch (ms.topmenu) {
		case WINOVERVIEW:
			break;
		case WINNEWTEST:
			newtest_key(ch, &wins, &ms);
			break;
		}
	}

	term_menustate(&ms);
	term_windows(&wins);

	endwin();
}

