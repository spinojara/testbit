#include "prompt.h"

#include <string.h>
#include <stdlib.h>

#include "color.h"

void draw_prompt(struct prompt *p, int highlight) {
	wattrset(p->win, highlight ? cs.texthl.attr : cs.text.attr);

	int x = p->x;
	mvwaddstr(p->win, p->y, x, p->prompt);
	x += strlen(p->prompt);
	wattrset(p->win, cs.text.attr);
	mvwaddstr(p->win, p->y, x, " : ");
	x += 3;

	if (!p->len) {
		wattrset(p->win, cs.textdim.attr);
		mvwaddstr(p->win, p->y, x, p->init);
		x += strlen(p->init);
	}
}

void new_prompt(struct prompt *p, WINDOW *win, int y, int x, const char *prompt, const char *init, int (*allow)(chtype)) {
	p->allow = allow;
	p->y = y;
	p->x = x;
	p->win = win;

	p->init = malloc(strlen(init) + 1);
	memcpy(p->init, init, strlen(init) + 1);

	p->prompt = malloc(strlen(prompt) + 1);
	memcpy(p->prompt, prompt, strlen(prompt) + 1);

	p->size = 32;
	p->len = 0;
	p->str = malloc(p->size);
}

void delete_prompt(struct prompt *p) {
	free(p->str);
	free(p->prompt);
	free(p->init);
}

void highlight_prompt(struct prompt *p, int highlight) {
	draw_prompt(p, highlight);
}

void enter_prompt(struct prompt *p) {

	chtype ch;
	while ((ch = wgetch(p->win)) != KEY_ENTER) {
		switch (ch) {
		default:
			break;
		}
	}
}
