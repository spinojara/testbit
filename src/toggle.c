#include "toggle.h"

#include "color.h"

void new_toggle(struct toggle *t, WINDOW *win, int y, int x, int n, int state) {
	t->initial = t->state = state != 0;
	t->y = y;
	t->x = x;
	t->n = n;
	t->win = win;
}

void draw_toggle(struct toggle *t) {
	wattrset(t->win, t->state ? cs.bg.attr : cs.text.attr);
	mvwhline(t->win, t->y, t->x, ' ', t->n);
	wattrset(t->win, cs.texthl.attr);
	mvwaddstr(t->win, t->y, t->x + t->state * (t->n - 3), "   ");
	mvwaddch(t->win, t->y, t->x + 1 + t->state * (t->n - 3), ACS_DIAMOND);
}

void toggle_toggle(struct toggle *t) {
	t->state = 1 - t->state;
}

void reset_toggle(struct toggle *t) {
	t->state = t->initial;
}
