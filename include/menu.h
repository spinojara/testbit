#ifndef MENU_H
#define MENU_H

#include <ncurses.h>

#include "state.h"

enum {
	MENUQUIT = -1,
	MENUNEWTEST,
	MENUACTIVE,
	MENUDONE,
	MENUCANCELLED,
	MENUFAILED,

	MENUMAX,
};

void handle_menu(struct state *st, chtype ch);

void draw_menu(struct state *st);

#endif
