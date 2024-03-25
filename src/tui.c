#include "tui.h"

#include <ncurses.h>

#include "color.h"
#include "draw.h"
#include "state.h"
#include "menu.h"

void tuiloop(SSL *ssl) {
	int ch;

	initscr();
	keypad(stdscr, TRUE);
	curs_set(0);
	cbreak();
	noecho();
	init_colors();

	struct state st = { 0 };
	init_state(&st, ssl, MENUNEWTEST);
	draw_resize(&st);

	while (st.selected != MENUQUIT) {
		ch = wgetch(stdscr);
		handle_menu(&st, ch);
	}

	term_state(&st);

	endwin();
}
