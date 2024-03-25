#ifndef PROMPT_H
#define PROMPT_H

#include <ncurses.h>

struct prompt {
	int n;

	int size;
	int len;
	char *str;

	char *suggestion;

	WINDOW *win;

	int y, x;

	int cur, disp;

	int error;

	int (*allow)(chtype);
};

void handle_prompt(struct prompt *p);

void new_prompt(struct prompt *p, WINDOW *win, int y, int x, int n, const char *init, int (*allow)(chtype));

void delete_prompt(struct prompt *p);

void reset_prompt(struct prompt *p);

void resize_prompt(struct prompt *p, int n);

void draw_prompt(struct prompt *p);

char *prompt_str(struct prompt *p);

int prompt_passphrase(char *passphrase, int size);

#endif
