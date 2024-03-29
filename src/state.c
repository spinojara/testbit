#include "state.h"

#include "newtest.h"
#include "oldtest.h"
#include "single.h"

void init_state(struct state *st, SSL *ssl, int selected) {
	st->selected = selected;
	st->ns.ssl = ssl;
	st->as.ssl = ssl;
	st->ds.ssl = ssl;
	st->fs.ssl = ssl;
	st->cs.ssl = ssl;

	st->as.type = OLDTESTACTIVE;
	st->as.fd = -1;
	st->ds.type = OLDTESTDONE;
	st->ds.fd = -1;
	st->fs.type = OLDTESTFAILED;
	st->fs.fd = -1;
	st->cs.type = OLDTESTCANCELLED;
	st->cs.fd = -1;

	st->win = newwin(0, 0, 0, 0);
	keypad(st->win, TRUE);

	st->ns.win = newwin(0, 0, 0, 0);
	keypad(st->ns.win, TRUE);

	st->as.win = newwin(0, 0, 0, 0);
	keypad(st->as.win, TRUE);
	st->as.test = NULL;

	st->ds.win = newwin(0, 0, 0, 0);
	keypad(st->ds.win, TRUE);
	st->ds.test = NULL;

	st->fs.win = newwin(0, 0, 0, 0);
	keypad(st->fs.win, TRUE);
	st->fs.test = NULL;

	st->cs.win = newwin(0, 0, 0, 0);
	keypad(st->cs.win, TRUE);
	st->cs.test = NULL;

	init_newtest(&st->ns);
}

void term_state(struct state *st) {
	delwin(st->win);
	delwin(st->ns.win);
	delwin(st->as.win);
	delwin(st->ds.win);
	delwin(st->cs.win);
	delwin(st->fs.win);

	free(st->as.test);
	free(st->ds.test);
	free(st->fs.test);
	free(st->cs.test);

	term_newtest(&st->ns);

	free_single(&st->as);
	free_single(&st->ds);
	free_single(&st->fs);
	free_single(&st->cs);
}
