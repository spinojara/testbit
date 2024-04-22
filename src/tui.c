#define _POSIX_C_SOURCE 1
#include "tui.h"

#include <signal.h>
#include <unistd.h>

#include <ncurses.h>

#include "color.h"
#include "draw.h"
#include "state.h"
#include "menu.h"
#include "ssl.h"

static struct state *diestate;
static SSL *diessl;

void sigint(int num) {
	(void)num;
	if (diestate)
		term_state(diestate);
	if (diessl)
		ssl_close(diessl, 1);
	endwin();
	signal(SIGINT, SIG_DFL);
	kill(getpid(), SIGINT);
}

void die(int ret, const char *str) {
	if (diestate)
		term_state(diestate);
	if (diessl)
		ssl_close(diessl, 1);
	endwin();
	if (str)
		fprintf(stderr, "%s", str);
	exit(ret);
}

void lostcon(void) {
	die(16, "error: connection lost\n");
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
	diestate = &st;
	diessl = ssl;
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
