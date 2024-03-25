#include "prompt.h"

#include <string.h>
#include <stdlib.h>

#include "color.h"
#include "tui.h"
#include "draw.h"

void new_prompt(struct prompt *p, WINDOW *win, int y, int x, int n, const char *suggestion, int (*allow)(chtype)) {
	p->n = n;
	p->allow = allow;
	p->y = y;
	p->x = x;
	p->win = win;

	p->suggestion = malloc(strlen(suggestion) + 1);
	memcpy(p->suggestion, suggestion, strlen(suggestion) + 1);

	p->str = NULL;
	reset_prompt(p);
}

void draw_prompt(struct prompt *p) {
	char *str = malloc(p->n + 1);
	memset(str, ' ', p->n);
	str[p->n] = '\0';
	if (!p->len) {
		wattrset(p->win, cs.textdim.attr);
		int len = strlen(p->suggestion);
		int n = len > p->n ? p->n : len;
		memcpy(str, p->suggestion, n);
		mvwaddstr(p->win, p->y, p->x, str);
	}
	else {
		int left = p->len - p->disp;
		int n = left > p->n ? p->n : left;
		memcpy(str, p->str + p->disp, n);
		wattrset(p->win, cs.text.attr);
		mvwaddstr(p->win, p->y, p->x, str);
	}
	free(str);
	wmove(p->win, p->y, p->x + p->cur - p->disp);
}

void delete_prompt(struct prompt *p) {
	free(p->str);
	free(p->suggestion);
}

void reset_prompt(struct prompt *p) {
	free(p->str);

	p->size = 32;
	p->len = 0;
	p->str = malloc(p->size);

	p->cur = p->disp = 0;
}

void resize_prompt(struct prompt *p, int n) {
	if (p->n == n)
		return;
	p->n = n;
	p->cur = p->len;
	p->disp = p->len > p->n ? p->len - p->n : 0;
}

void curpp(struct prompt *p) {
	p->cur++;
	if (p->disp + p->n <= p->cur)
		p->disp++;
}

void curmm(struct prompt *p) {
	p->cur--;
	if (p->disp > p->cur)
		p->disp--;
}

void append_prompt(struct prompt *p, char c) {
	if (p->len >= p->size - 1) {
		p->size *= 2;
		p->str = realloc(p->str, p->size);
	}
	for (int i = p->len; i >= p->cur; i--)
		p->str[i + 1] = p->str[i];
	p->str[p->cur] = c;

	p->len++;
	curpp(p);
}

void backspace_prompt(struct prompt *p) {
	if (p->cur == 0)
		return;

	for (int i = p->cur; i <= p->len; i++)
		p->str[i - 1] = p->str[i];
	curmm(p);
	p->len--;
	/* Move cursor on backspace even if curmm did not. */
	if (0 < p->disp && p->disp < p->cur && p->disp + p->n >= p->len)
		p->disp--;
}

void handle_prompt(struct prompt *p) {
	curs_set(1);
	p->error = 0;
	draw_prompt(p);
	wrefresh(p->win);
	chtype ch = 0;
	while (ch != KEY_ESC) {
		ch = wgetch(p->win);
		switch (ch) {
		case KEY_LEFT:
			if (p->cur > 0)
				curmm(p);
			break;
		case KEY_RIGHT:
			if (p->cur < p->len)
				curpp(p);
			break;
			break;
		case KEY_UP:
		case KEY_DOWN:
		case KEY_RESIZE:
			ungetch(ch);
			/* fallthrough */
		case '\n':
			ch = KEY_ESC;
			break;
		case KEY_ESC:
			break;
		case KEY_BACKSPACE:
		case KEY_DL:
		case 127:
			backspace_prompt(p);
			break;
		default:
			if (p->allow(ch))
				append_prompt(p, (char)ch);
			break;
		}
		draw_prompt(p);
		wrefresh(p->win);
	}
	curs_set(0);
}

char *prompt_str(struct prompt *p) {
	p->str[p->len] = '\0';
	return p->len ? p->str : p->suggestion;
}

int prompt_passphrase(char *passphrase, int size) {
	WINDOW *win = newwin(6, size + 6, (LINES - 8) / 2, (COLS - (size + 6)) / 2);
	keypad(win, TRUE);
	draw_border(win, &cs.bordershadow, &cs.border, &cs.bordershadow, 1, 0, 0, getmaxy(win), getmaxx(win));
	wattrset(win, cs.text.attr);
	mvwaddstr(win, 0, 2, "Enter Passphrase");
	curs_set(1);
	wmove(win, 2, 2);
	wrefresh(win);

	int len = strlen(passphrase);

	char *visible = malloc(size);
	for (int i = 0; i < size; i++)
		visible[i] = i <= len ? '*' : ' ';
	visible[size - 1] = '\0';

	chtype ch = 0;
	int ret = 0;
	while (ch != KEY_ESC) {
		int update = 0;
		ch = wgetch(win);

		switch (ch) {
		case KEY_ESC:
			ret = 1;
			break;
		case KEY_RESIZE:
			ret = 1;
			ungetch(KEY_RESIZE);
			/* fallthrough */
		case '\n':
			ch = KEY_ESC;
			break;
		case KEY_BACKSPACE:
		case KEY_DL:
		case 127:
			if (len > 0) {
				len--;
				visible[len] = ' ';
				update = 1;
			}
		}

		if (' ' <= ch && ch <= '~' && len < size - 1) {
			visible[len] = '*';
			passphrase[len++] = (char)ch;
			update = 1;
		}

		if (update) {
			mvwaddstr(win, 2, 2, visible);
			wmove(win, 2, 2 + len);
			wrefresh(win);
		}
	}

	passphrase[len] = '\0';
	free(visible);

	curs_set(0);
	delwin(win);
	return ret;
}
