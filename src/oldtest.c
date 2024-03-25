#include "oldtest.h"

#include <stdarg.h>
#include <ctype.h>
#include <math.h>

#include "draw.h"
#include "sprt.h"
#include "con.h"
#include "req.h"
#include "util.h"

const int refresh_interval = 30;

int load_oldtest(struct oldteststate *os);

void handle_oldtest(struct oldteststate *os, chtype ch) {
	int update = 0;
	switch (ch) {
	case KEY_UP:
	case 'k':
		if (os->selected > 0) {
			os->selected--;
			update = 1;
		}
		break;
	case KEY_DOWN:
	case 'j':
		if (os->selected < os->tests - 1) {
			os->selected++;
			update = 1;
		}
		break;
	}

	if (update)
		draw_oldtest(os, 1, 1);
}

/* NULL terminated list of args. */
void draw_dynamic(struct oldteststate *os, ...) {
	
	va_list ap;

	int tests = os->tests + 1;

	int x = 1;

	va_start(ap, os);
	while (1) {
		const char (*strs)[128] = va_arg(ap, const char (*)[128]);
		if (!strs)
			break;

		int maxlen = 0;
		for (int i = 0; i < tests; i++) {
			int len = strlen(strs[i]);
			if (len > maxlen)
				maxlen = len;
		}

		if (x + maxlen + 3 >= getmaxx(os->win))
			continue;

		for (int i = 0; i < tests; i++) {
			int y = 1 + 2 * i;
			int align_right = isdigit(strs[i][0]) || strs[i][0] == '.';
			int selected = i == os->selected + 1 && x == 1;
			int len = strlen(strs[i]);
			wattrset(os->win, selected ? cs.texthl.attr : cs.text.attr);
			if (x == 1)
				mvwprintw(os->win, y, 1, " %c ", selected ? '*' : ' ');
			else
				mvwprintw(os->win, y, x + 1 + align_right * (maxlen - len), "%s", strs[i]);
			wattrset(os->win, cs.textdim.attr);
			if (x == 1) {
				wattrset(os->win, cs.text.attr);
				mvwaddch(os->win, y + 1, 0, ACS_LTEE);
				wattrset(os->win, cs.textdim.attr);
			}
			else {
				if (i == tests - 1)
					mvwaddch(os->win, y + 1, x - 1, ACS_BTEE);
				if (i == 0) {
					wattrset(os->win, cs.text.attr);
					mvwaddch(os->win, 0, x - 1, ACS_TTEE);
					wattrset(os->win, cs.textdim.attr);
				}
				else {
					mvwaddch(os->win, y - 1, x - 1, ACS_PLUS);
				}
			}
			/* An extra refresh to fix stuttering. */
			wrefresh(os->win);
			if (i == 0) {
				wattrset(os->win, cs.text.attr);
				mvwaddch(os->win, 0, x + maxlen + 2, ACS_TTEE);
				wattrset(os->win, cs.textdim.attr);
			}
			if (0 < i && i <= tests - 1)
				mvwaddch(os->win, y - 1, x + maxlen + 2, ACS_RTEE);
			if (i == tests - 1)
				mvwaddch(os->win, y + 1, x + maxlen + 2, ACS_LRCORNER);
			mvwaddch(os->win, y, x + maxlen + 2, ACS_VLINE);
			mvwhline(os->win, y + 1, x, 0, maxlen + 2);

			wrefresh(os->win);
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

void draw_active(struct oldteststate *os) {
	int tests = os->tests + 1;

	char (*select)[128] = calloc(tests, sizeof(*select));
	char (*id)[128] = calloc(tests, sizeof(*id));
	char (*tc)[128] = calloc(tests, sizeof(*tc));
	char (*elo)[128] = calloc(tests, sizeof(*elo));
	char (*llh)[128] = calloc(tests, sizeof(*llh));
	char (*ab)[128] = calloc(tests, sizeof(*ab));
	char (*elo0)[128] = calloc(tests, sizeof(*elo0));
	char (*elo1)[128] = calloc(tests, sizeof(*elo1));
	char (*eloe)[128] = calloc(tests, sizeof(*eloe));
	char (*tri)[128] = calloc(tests, sizeof(*tri));
	char (*penta)[128] = calloc(tests, sizeof(*penta));
	char (*qtime)[128] = calloc(tests, sizeof(*qtime));
	char (*stime)[128] = calloc(tests, sizeof(*stime));
	char (*branch)[128] = calloc(tests, sizeof(*branch));
	char (*commit)[128] = calloc(tests, sizeof(*commit));

	sprintf(select[0], " ");
	sprintf(id[0], "Id");
	sprintf(tc[0], "TC");
	sprintf(elo[0], "Elo");
	sprintf(llh[0], "LLH");
	sprintf(ab[0], "(A,B)");
	sprintf(elo0[0], "Elo0");
	sprintf(elo1[0], "Elo1");
	sprintf(eloe[0], "EloE");
	sprintf(tri[0], "Trinomial");
	sprintf(penta[0], "Pentanomial");
	sprintf(qtime[0], "Queue Timestamp");
	sprintf(stime[0], "Start Timestamp");
	sprintf(branch[0], "Branch");
	sprintf(commit[0], "Commit");

	char tmp1[128], tmp2[128];
	for (int i = 1; i < tests; i++) {
		struct test *test = &os->test[i - 1];
		test->alpha = test->beta = 0.025;
		double A = log(test->beta / (1 - test->alpha));
		double B = log((1 - test->beta) / test->alpha);

		sprintf(id[i], "%ld", test->id);
		sprintf(tc[i], "%s+%s", fstr(tmp1, test->maintime, 2), fstr(tmp2, test->increment, 2));
		/* Both branch[i] and commit[i] will be null terminated since
		 * they were initialized to 0 by calloc.
		 */
		snprintf(branch[i], 127, "%s", test->branch);
		snprintf(commit[i], 127, "%s", test->commit);
		sprintf(qtime[i], "%s", iso8601local(tmp1, test->qtime));
		if (test->status != TESTQUEUE) {
			sprintf(elo[i], "%.2lf+%.2lf", test->elo, test->pm);
			sprintf(tri[i], "%u-%u-%u", test->t0, test->t1, test->t2);
			sprintf(penta[i], "%u-%u-%u-%u-%u", test->p0, test->p1, test->p2, test->p3, test->p4);
			sprintf(stime[i], "%s", iso8601local(tmp1, test->stime));
		}
		else {
			sprintf(elo[i], "N/A");
			sprintf(tri[i], "N/A");
			sprintf(penta[i], "N/A");
			sprintf(stime[i], "N/A");
		}
		if (test->type == TESTTYPESPRT) {
			if (test->status != TESTQUEUE) {
				sprintf(llh[i], "%.2lf", test->llh);
				sprintf(ab[i], "(%.2lf, %.2lf)", A, B);
			}
			else {
				sprintf(llh[i], "N/A");
				sprintf(ab[i], "N/A");
			}
			sprintf(elo0[i], "%.1lf", test->elo0);
			sprintf(elo1[i], "%.1lf", test->elo1);
			sprintf(eloe[i], "N/A");
		}
		else {
			sprintf(llh[i], "N/A");
			sprintf(ab[i], "N/A");
			sprintf(elo0[i], "N/A");
			sprintf(elo1[i], "N/A");
			sprintf(eloe[i], "%.1lf", test->eloe);
		}
	}
	
	draw_dynamic(os, select, id, tc, elo, tri, penta, llh, ab, elo0, elo1, eloe, qtime, stime, branch, commit, NULL);

	free(id);
	free(tc);
	free(elo);
	free(llh);
	free(ab);
	free(elo0);
	free(elo1);
	free(eloe);
	free(tri);
	free(penta);
	free(qtime);
	free(stime);
}

void draw_oldtest(struct oldteststate *os, int lazy, int load) {
	if (load && time(NULL) - os->last_loaded > refresh_interval) {
		load_oldtest(os);
		lazy = 0;
	}

	if (!lazy)
		draw_border(os->win, NULL, &cs.bordershadow, &cs.border, 1, 0, 0, getmaxy(os->win), getmaxx(os->win));

	switch (os->type) {
	case OLDTESTACTIVE:
		draw_active(os);
		break;
	}

	touchwin(os->win);
	wrefresh(os->win);
}

void resize_oldtest(struct oldteststate *os) {
	if (os->test) {
		for (int i = 0; i < os->page_size; i++) {
			free(os->test[i].branch);
			free(os->test[i].commit);
		}
	}

	os->last_loaded = 0;
	os->tests = 0;
	os->page = 0;
	os->page_size = (LINES - 12) / 2;
	os->selected = 0;

	free(os->test);
	/* This initializes all os->test->{branch,commit} to
	 * NULL. So we can always free them without problems.
	 */
	os->test = calloc(os->page_size, sizeof(*os->test));
}

int load_oldtest(struct oldteststate *os) {
	int64_t min = os->page_size * os->page;
	int64_t max = os->page_size + min;
	if (sendf(os->ssl, "ccqq", REQUESTLOGTEST, os->type, min, max))
		return 1;

	char done;
	for (int i = 0; i < max - min; i++) {
		if (recvf(os->ssl, "c", &done))
			return 1;
		if (done)
			break;
		if (i == 0)
			os->tests = 0;
		os->tests++;

		struct test *test = &os->test[i];

		if (recvf(os->ssl, "qccDDDDDDDDDDssqqqLLLLLLLL",
					&test->id, &test->type, &test->status,
					&test->maintime, &test->increment, &test->alpha,
					&test->beta, &test->llh, &test->elo0,
					&test->elo1, &test->eloe, &test->elo,
					&test->pm, &test->branch, &test->commit,
					&test->qtime, &test->stime, &test->dtime,
					&test->t0, &test->t1, &test->t2, &test->p0,
					&test->p1, &test->p2, &test->p3, &test->p4))
			return 1;
	}

	os->last_loaded = time(NULL);

	return 0;
}
