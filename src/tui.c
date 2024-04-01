#include "tui.h"

#include <signal.h>

#include <ncurses.h>

#include "color.h"
#include "draw.h"
#include "state.h"
#include "menu.h"

static struct state *sigintst;

void sigint(int num) {
	term_state(sigintst);
	endwin();
	exit(num);
}

void tuiloop(SSL *ssl) {
	int ch;

	initscr();
	keypad(stdscr, TRUE);
	set_escdelay(25);
	curs_set(0);
	cbreak();
	noecho();
	init_colors();

	struct state st = { 0 };
	init_state(&st, ssl, MENUNEWTEST);
	sigintst = &st;
	signal(SIGINT, &sigint);
	draw_resize(&st);

	while (st.selected != MENUQUIT) {
		timeout(1000 * REFRESH_SECONDS);
		ch = getch();
		if (ch == ERR)
			ch = 'r';
		timeout(-1);
		handle_menu(&st, ch);
	}

	term_state(&st);

	endwin();
}
