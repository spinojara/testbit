#ifndef NEWTEST_H
#define NEWTEST_H

#include <ncurses.h>

struct menustate;
struct windows;

enum {
	NEWTESTTIME,
	NEWTESTELO0,
	NEWTESTELO1,
	NEWTESTALPHA,
	NEWTESTBETA,
	NEWTESTELOE,

	NEWTESTCOUNT,

	NEWTESTSEND,
};

#include "win.h"

void newtest_key(chtype ch, struct windows *wins, struct menustate *ms);

#endif
