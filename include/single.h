#ifndef SINGLE_H
#define SINGLE_H

#include <ncurses.h>

#include "state.h"

void free_single(struct oldteststate *os);

void handle_single(struct oldteststate *os, chtype ch);

void draw_single(struct oldteststate *os, int lazy, int load);

#endif
