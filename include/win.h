#ifndef WIN_H
#define WIN_H

#include <openssl/ssl.h>
#include <ncurses.h>

#include "color.h"
#include "newtest.h"
#include "prompt.h"

enum {
	WINTOPMENU,
	WINOVERVIEW,
	WINNEWTEST,

	WINCOUNT,
};

struct windows {
	WINDOW *mainwin;
	int mainoffset[2];
	int mainsize[2];

	WINDOW *win[WINCOUNT];
	int offset[WINCOUNT][2];
	int size[WINCOUNT][2];
};

struct newteststate {
	int newtest;

	char type;

	struct prompt prompt[NEWTESTCOUNT];
};

struct menustate {
	SSL *ssl;

	int running;

	int topmenu;

	struct newteststate nts;
};

void init_windows(struct windows *wins);
void term_windows(struct windows *wins);
void init_menustate(struct menustate *ms, struct windows *wins);
void term_menustate(struct menustate *ms);

void handle_resize(struct windows *wins, struct menustate *ms);

void wmenu(struct windows *wins, const struct menustate *ms);

void wtopmenu(WINDOW *win, const struct menustate *ms);

#endif
