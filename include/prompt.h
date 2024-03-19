#ifndef PROMPT_H
#define PROMPT_H

#include <ncurses.h>

struct prompt {
	int size;
	int len;
	char *str;

	char *init;
	char *prompt;

	WINDOW *win;

	int y, x;

	int (*allow)(chtype);
};

void new_prompt(struct prompt *p, WINDOW *win, int y, int x, const char *prompt, const char *init, int (*allow)(chtype));

void delete_prompt(struct prompt *p);

void highlight_prompt(struct prompt *p, int highlight);

#endif
