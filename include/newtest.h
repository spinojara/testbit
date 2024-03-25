#ifndef NEWTEST_H
#define NEWTEST_H

#include <ncurses.h>

#include "state.h"

void handle_newtest(struct newteststate *ns, chtype ch);

void draw_newtest(struct newteststate *ns);

void init_newtest(struct newteststate *ns);

void resize_prompts(struct newteststate *ns);

void term_newtest(struct newteststate *ns);

#endif
