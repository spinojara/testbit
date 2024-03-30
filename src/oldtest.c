#include "oldtest.h"

#include <stdarg.h>
#include <ctype.h>
#include <math.h>

#include "draw.h"
#include "sprt.h"
#include "con.h"
#include "req.h"
#include "util.h"
#include "active.h"
#include "done.h"
#include "single.h"

const int refresh_interval = 30;

int load_oldtest(struct oldteststate *os);

void handle_oldtest(struct oldteststate *os, chtype ch) {
	if (os->single) {
		handle_single(os, ch);
		return;
	}

	int update = 0;
	switch (ch) {
	case KEY_UP:
	case 'k':
		if (os->selected > 0) {
			os->selected--;
			update = 1;
		}
		else if (os->page > 0) {
			os->page--;
			os->selected = os->page_size - 1;
			update = 1;
		}
		break;
	case KEY_DOWN:
	case 'j':
		if (os->selected < os->tests - 1) {
			os->selected++;
			update = 1;
		}
		else if (os->selected == os->page_size - 1) {
			os->page++;
			os->selected = 0;
			update = 1;
		}
		break;
	case '\n':
		if (os->tests > 0) {
			os->single = 1;
			handle_single(os, 'r');
			return;
		}
		/* fallthrough */
	case 'r':
		os->last_loaded = 0;
		break;
	default:
		break;
	}

	os->selected_id = os->test[os->selected].id;
	if (update || time(NULL) - os->last_loaded > refresh_interval)
		draw_oldtest(os, 1, 1);
}

/* NULL terminated list of args. */
void draw_dynamic(struct oldteststate *os, void (*attr)(const struct oldteststate *os, int i, int j), int select, ...) {
	va_list ap;

	int tests = os->tests + 1;

	int x = 3;

	va_start(ap, select);
	for (int j = 0; ; j++) {
		const char (*strs)[128] = va_arg(ap, const char (*)[128]);
		if (!strs)
			break;

		int maxlen = 0;
		for (int i = 0; i < tests; i++) {
			int len = strlen(strs[i]);
			if (len > maxlen)
				maxlen = len;
		}

		if (x + maxlen + 4 >= getmaxx(os->win))
			continue;

		for (int i = 0; i < tests; i++) {
			int y = 2 + 2 * i;
			int align_right = isdigit(strs[i][0]) || strs[i][0] == '.';
			int selected = i == os->selected + 1 && j == 0;
			int len = strlen(strs[i]);
			wattrset(os->win, selected ? cs.texthl.attr : cs.text.attr);
			if (j == 0 && select)
				mvwprintw(os->win, y, x, " %c ", selected ? '*' : ' ');
			else {
				attr(os, i, j);
				mvwprintw(os->win, y, x + 1 + align_right * (maxlen - len), "%s", strs[i]);
			}
			wattrset(os->win, cs.textdim.attr);
			mvwhline(os->win, y - 1, x, 0, maxlen + 2);
			mvwhline(os->win, y + 1, x, 0, maxlen + 2);
			mvwaddch(os->win, y, x - 1, ACS_VLINE);
			mvwaddch(os->win, y, x + maxlen + 2, ACS_VLINE);
			if (j == 0) {
				if (i == 0)
					mvwaddch(os->win, y - 1, x - 1, ACS_ULCORNER);
				else
					mvwaddch(os->win, y - 1, x - 1, ACS_LTEE);
				if (i == tests - 1)
					mvwaddch(os->win, y + 1, x - 1, ACS_LLCORNER);
			}
			else if (i == 0) {
				mvwaddch(os->win, y - 1, x - 1, ACS_TTEE);
				mvwaddch(os->win, y - 1, x + maxlen + 2, ACS_URCORNER);
			}
			else {
				mvwaddch(os->win, y - 1, x + maxlen + 2, ACS_RTEE);
				mvwaddch(os->win, y - 1, x - 1, ACS_PLUS);
			}
			if (j > 0) {
				mvwaddch(os->win, y + 1, x - 1, ACS_BTEE);
			}
			if (i == tests - 1) {
				mvwaddch(os->win, y + 1, x + maxlen + 2, ACS_LRCORNER);
			}
		}

		x += maxlen + 3;
	}
	va_end(ap);
}

int powi(int a, int n) {
	int r = 1;
	for (int i = 0; i < n; i++)
		r *= a;
	return r;
}

char *fstr(char *s, double f, int n) {
	int pos = 0;
	if (f < 0.0) {
		s[pos++] = '-';
		f = -f;
	}
	int m = powi(10, n);
	double r = f * m;
	int d = (int)round(r);

	pos += sprintf(s + pos, "%d", d / m);

	d %= m;
	for (int i = 1; i < n; i++) {
		if (!(d % 10))
			d /= 10;
	}
	if (d)
		sprintf(s + pos, ".%d", d);

	return s;
}

void draw_oldtest(struct oldteststate *os, int lazy, int load) {
	if (os->single) {
		draw_single(os, lazy, load);
		return;
	}

	if (load && (time(NULL) - os->last_loaded > refresh_interval || os->page_loaded != os->page)) {
		load_oldtest(os);
		lazy = 0;
	}

	if (!lazy)
		draw_border(os->win, NULL, &cs.bordershadow, &cs.border, 1, 0, 0, getmaxy(os->win), getmaxx(os->win));

	switch (os->type) {
	case OLDTESTACTIVE:
		draw_active(os);
		break;
	case OLDTESTDONE:
	case OLDTESTCANCELLED:
	case OLDTESTFAILED:
		draw_done(os);
		break;
	}

	touchwin(os->win);
	wrefresh(os->win);
}

void resize_oldtest(struct oldteststate *os) {
	os->last_loaded = 0;
	os->page_loaded = 0;
	os->tests = 0;
	os->page = 0;
	os->page_size = (LINES - 13) / 2;
	os->selected = 0;
	os->top = os->patch;
	os->fills = 0;

	free(os->test);
	/* This initializes all os->test->{branch,commit} to
	 * NULL. So we can always free them without problems.
	 */
	os->test = calloc(os->page_size, sizeof(*os->test));
}

int load_oldtest(struct oldteststate *os) {
	os->last_loaded = time(NULL);
	os->page_loaded = os->page;
	int64_t min = os->page_size * os->page;
	int64_t max = os->page_size + min;
	if (sendf(os->ssl, "ccqq", REQUESTLOGTEST, os->type, min, max))
		return 1;

	char done;
	for (int i = 0; ; i++) {
		if (recvf(os->ssl, "c", &done))
			return 1;
		if (i == 0)
			os->tests = 0;
		if (done)
			break;
		if (i >= os->page_size)
			continue;
		os->tests++;

		struct test *test = &os->test[i];

		if (recvf(os->ssl, "qccDDDDDDDDDDssqqqLLLLLLLL",
					&test->id, &test->type, &test->status,
					&test->maintime, &test->increment, &test->alpha,
					&test->beta, &test->llr, &test->elo0,
					&test->elo1, &test->eloe, &test->elo,
					&test->pm, test->branch, sizeof(test->branch),
					test->commit, sizeof(test->commit),
					&test->qtime, &test->stime, &test->dtime,
					&test->t0, &test->t1, &test->t2, &test->p0,
					&test->p1, &test->p2, &test->p3, &test->p4))
			return 1;

		if (os->selected_id == test->id)
			os->selected = i;
	}

	if (!os->tests && os->page > 0) {
		os->page--;
		return load_oldtest(os);
	}
	os->selected = clamp(os->selected, 0, os->tests - 1);
	os->selected_id = os->test[os->selected].id;

	return 0;
}
