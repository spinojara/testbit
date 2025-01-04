#include "newtest.h"

#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>

#include "draw.h"
#include "sprt.h"
#include "req.h"
#include "infobox.h"
#include "util.h"
#include "tui.h"
#include "source.h"
#include "tc.h"

enum {
	PROMPTTIME,
	PROMPTALPHA,
	PROMPTBETA,
	PROMPTELO0,
	PROMPTELO1,
	PROMPTELOE,
	PROMPTBRANCH,
	PROMPTCOMMIT,
	PROMPTPATH,

	PROMPTNNUE,
	PROMPTSIMD,

	PROMPTDRAW,
	PROMPTRESIGN,

	PROMPTQUEUE,

	PROMPTCOUNT,
};

enum {
	TOGGLEDRAW,
	TOGGLERESIGN,
};

int allow_float(chtype ch) {
	return ('0' <= ch && ch <= '9') || ch == '-' || ch == '.';
}

int allow_tc(chtype ch) {
	return allow_float(ch) || ch == '+' || ch == '/';
}

int allow_integer(chtype ch) {
	return ('0' <= ch && ch <= '9') || ch == '-';
}

/* ASCII */
int allow_text(chtype ch) {
	return (' ' <= ch && ch <= '~');
}

void draw_prompts(struct newteststate *ns);
void queue_test(struct newteststate *ns);

void handle_newtest(struct newteststate *ns, chtype ch) {
	if (LINES < NEWTESTLINESMIN) {
		draw_newtest(ns);
		return;
	}
	int redraw = 0;
	switch (ch) {
	case KEY_UP:
	case 'k':
		if (ns->selected > 0) {
			redraw = 1;
			ns->selected--;
		}
		break;
	case KEY_DOWN:
	case 'j':
		if (ns->selected < PROMPTCOUNT - 1) {
			redraw = 1;
			ns->selected++;
		}
		break;
	case '\n':
		if (ns->selected == PROMPTQUEUE) {
			redraw = 1;
			queue_test(ns);
			draw_newtest(ns);
			break;
		}
		/* fallthrough */
	case 'i':
		if (ns->selected <= PROMPTSIMD) {
			redraw = 1;
			handle_prompt(&ns->prompt[ns->selected]);
		}
		else if (ns->selected == PROMPTDRAW) {
			redraw = 1;
			toggle_toggle(&ns->toggle[TOGGLEDRAW]);
		}
		else if (ns->selected == PROMPTRESIGN) {
			redraw = 1;
			toggle_toggle(&ns->toggle[TOGGLERESIGN]);
		}
		break;
	default:
		break;
	}

	if (redraw)
		draw_prompts(ns);
}

void draw_prompt_border(WINDOW *win, const char *title, int y, int x, int promptlen, int highlight, int error) {
	struct color *upper, *lower;
	if (highlight) {
		if (error) {
			upper = &cs.errorhl;
			lower = &cs.error;
		}
		else {
			upper = &cs.border;
			lower = &cs.bordershadow;
		}
	}
	else {
		if (error) {
			upper = &cs.error;
			lower = &cs.errorhl;
		}
		else {
			upper = &cs.bordershadow;
			lower = &cs.border;
		}
	}

	draw_border(win, NULL, upper, lower, 0, y, x, 3, promptlen);

	wattrset(win, cs.bordershadow.attr);
	mvwprintw(win, y, x + 2, " %s ", title);

#ifdef TERMINAL_FLICKER
	wrefresh(win);
#endif
}

static void draw_button(WINDOW *win, const char *title, int y, int x, int n, int highlight) {
	struct color *upper, *lower;
	if (highlight) {
		upper = &cs.border;
		lower = &cs.bordershadow;
	
	}
	else {
		upper = &cs.bordershadow;
		lower = &cs.border;
	}
	draw_border(win, NULL, upper, lower, 0, y, x, 3, n);

	wattrset(win, cs.text.attr);
	mvwaddstr(win, y + 1, x + 2, title);

#ifdef TERMINAL_FLICKER
	wrefresh(win);
#endif
}

void resize_prompts(struct newteststate *ns) {
	int x = getmaxx(ns->win);

	resize_prompt(&ns->prompt[PROMPTTIME], x - 8);
	resize_prompt(&ns->prompt[PROMPTALPHA], x - 8);
	resize_prompt(&ns->prompt[PROMPTBETA], x - 8);
	resize_prompt(&ns->prompt[PROMPTELO0], x - 8);
	resize_prompt(&ns->prompt[PROMPTELO1], x - 8);
	resize_prompt(&ns->prompt[PROMPTELOE], x - 8);
	resize_prompt(&ns->prompt[PROMPTBRANCH], x - 8);
	resize_prompt(&ns->prompt[PROMPTCOMMIT], x - 8);
	resize_prompt(&ns->prompt[PROMPTPATH], x - 8);
	resize_prompt(&ns->prompt[PROMPTNNUE], x - 8);
	resize_prompt(&ns->prompt[PROMPTSIMD], x - 8);
}

void draw_prompts(struct newteststate *ns) {
	int x = getmaxx(ns->win);

	draw_prompt_border(ns->win, "Time Control", 1, 2, x - 4, ns->selected == PROMPTTIME, ns->prompt[PROMPTTIME].error);
	draw_prompt_border(ns->win, "Alpha", 4, 2, x - 4, ns->selected == PROMPTALPHA, ns->prompt[PROMPTALPHA].error);
	draw_prompt_border(ns->win, "Beta", 7, 2, x - 4, ns->selected == PROMPTBETA, ns->prompt[PROMPTBETA].error);
	draw_prompt_border(ns->win, "Elo0", 10, 2, x - 4, ns->selected == PROMPTELO0, ns->prompt[PROMPTELO0].error);
	draw_prompt_border(ns->win, "Elo1", 13, 2, x - 4, ns->selected == PROMPTELO1, ns->prompt[PROMPTELO1].error);
	draw_prompt_border(ns->win, "Elo Error", 16, 2, x - 4, ns->selected == PROMPTELOE, ns->prompt[PROMPTELOE].error);
	draw_prompt_border(ns->win, "Branch", 19, 2, x - 4, ns->selected == PROMPTBRANCH, ns->prompt[PROMPTBRANCH].error);
	draw_prompt_border(ns->win, "Commit", 22, 2, x - 4, ns->selected == PROMPTCOMMIT, ns->prompt[PROMPTCOMMIT].error);
	draw_prompt_border(ns->win, "Patch Path", 25, 2, x - 4, ns->selected == PROMPTPATH, ns->prompt[PROMPTPATH].error);
	draw_prompt_border(ns->win, "NNUE Path", 28, 2, x - 4, ns->selected == PROMPTNNUE, ns->prompt[PROMPTNNUE].error);
	draw_prompt_border(ns->win, "SIMD", 31, 2, x - 4, ns->selected == PROMPTSIMD, ns->prompt[PROMPTSIMD].error);

	draw_prompt_border(ns->win, "Adj Draw", 34, 2, 16, ns->selected == PROMPTDRAW, 0);
	draw_prompt_border(ns->win, "Adj Resign", 37, 2, 16, ns->selected == PROMPTRESIGN, 0);

	draw_button(ns->win, "Submit Test", 40, 2, 15, ns->selected == PROMPTQUEUE);

	for (int i = PROMPTTIME; i <= PROMPTSIMD; i++)
		draw_prompt(&ns->prompt[i]);

	for (int i = TOGGLEDRAW; i <= TOGGLERESIGN; i++)
		draw_toggle(&ns->toggle[i]);

	wrefresh(ns->win);
}

void draw_newtest(struct newteststate *ns) {
	draw_border(ns->win, NULL, &cs.bordershadow, &cs.border, 1, 0, 0, getmaxy(ns->win), getmaxx(ns->win));

	if (LINES < NEWTESTLINESMIN) {
		wattrset(ns->win, cs.text.attr);
		mvwaddstr(ns->win, getmaxy(ns->win) / 2, getmaxx(ns->win) / 2 - 21, "Terminal is too small to display this page.");
		wrefresh(ns->win);
		return;
	}

	draw_prompts(ns);
}

void init_newtest(struct newteststate *ns) {
	/* x will be the size of the full window at start up because
	 * we have not yet reduced the size of the smallest windows.
	 */
	int x = getmaxx(ns->win);

	new_prompt(&ns->prompt[PROMPTTIME], ns->win, 2, 4, x - 18, "40/10+0.1", &allow_tc);
	new_prompt(&ns->prompt[PROMPTALPHA], ns->win, 5, 4, x - 18, "0.025", &allow_float);
	new_prompt(&ns->prompt[PROMPTBETA], ns->win, 8, 4, x - 18, "0.025", &allow_float);
	new_prompt(&ns->prompt[PROMPTELO0], ns->win, 11, 4, x - 18, "0.0", &allow_float);
	new_prompt(&ns->prompt[PROMPTELO1], ns->win, 14, 4, x - 18, "4.0", &allow_float);
	new_prompt(&ns->prompt[PROMPTELOE], ns->win, 17, 4, x - 18, "0.0", &allow_float);
	new_prompt(&ns->prompt[PROMPTBRANCH], ns->win, 20, 4, x - 18, "master", &allow_text);
	new_prompt(&ns->prompt[PROMPTCOMMIT], ns->win, 23, 4, x - 18, "HEAD", &allow_text);
	new_prompt(&ns->prompt[PROMPTPATH], ns->win, 26, 4, x - 18, "", &allow_text);
	new_prompt(&ns->prompt[PROMPTNNUE], ns->win, 29, 4, x - 18, "", &allow_text);
	new_prompt(&ns->prompt[PROMPTSIMD], ns->win, 32, 4, x - 18, "avx2", &allow_text);

	new_toggle(&ns->toggle[TOGGLEDRAW], ns->win, 35, 4, 12, 1);
	new_toggle(&ns->toggle[TOGGLERESIGN], ns->win, 38, 4, 12, 1);
}

void term_newtest(struct newteststate *ns) {
	delete_prompt(&ns->prompt[PROMPTTIME]);
	delete_prompt(&ns->prompt[PROMPTALPHA]);
	delete_prompt(&ns->prompt[PROMPTBETA]);
	delete_prompt(&ns->prompt[PROMPTELO0]);
	delete_prompt(&ns->prompt[PROMPTELO1]);
	delete_prompt(&ns->prompt[PROMPTELOE]);
	delete_prompt(&ns->prompt[PROMPTBRANCH]);
	delete_prompt(&ns->prompt[PROMPTCOMMIT]);
	delete_prompt(&ns->prompt[PROMPTPATH]);
	delete_prompt(&ns->prompt[PROMPTNNUE]);
	delete_prompt(&ns->prompt[PROMPTSIMD]);
}

void queue_test(struct newteststate *ns) {
	struct prompt *p;
	char *str, *err;

	const double zero = eps;
	const double one = 1.0 - zero;

	char type;
	const char *tc;
	double alpha;
	double beta;
	double elo0;
	double elo1;
	double eloe;
	char adjudicate = 0;
	const char *branch, *commit, *simd;
	int fd = -1, nnuefd = -1;

	int error = 0;

	p = &ns->prompt[PROMPTTIME];
	tc = prompt_str(p);
	if (tccheck(tc)) {
		error = 1;
		p->error = 1;
	}

	p = &ns->prompt[PROMPTALPHA];
	str = prompt_str(p);
	errno = 0;
	alpha = strtod(str, &err);
	if (*err != '\0' || alpha < zero || alpha > one || errno) {
		error = 1;
		p->error = 1;
	}

	p = &ns->prompt[PROMPTBETA];
	str = prompt_str(p);
	errno = 0;
	beta = strtod(str, &err);
	if (*err != '\0' || beta < zero || beta > one || errno) {
		error = 1;
		p->error = 1;
	}

	p = &ns->prompt[PROMPTELO0];
	str = prompt_str(p);
	errno = 0;
	elo0 = strtod(str, &err);
	if (*err != '\0' || errno) {
		error = 1;
		p->error = 1;
	}

	p = &ns->prompt[PROMPTELO1];
	str = prompt_str(p);
	errno = 0;
	elo1 = strtod(str, &err);
	if (*err != '\0' || errno) {
		error = 1;
		p->error = 1;
	}

	p = &ns->prompt[PROMPTELOE];
	str = prompt_str(p);
	errno = 0;
	eloe = strtod(str, &err);
	if (*err != '\0' || errno) {
		error = 1;
		p->error = 1;
	}

	type = eloe < zero ? TESTTYPESPRT : TESTTYPEELO;

	p = &ns->prompt[PROMPTBRANCH];
	branch = prompt_str(p);

	p = &ns->prompt[PROMPTCOMMIT];
	commit = prompt_str(p);

	p = &ns->prompt[PROMPTPATH];
	str = prompt_str(p);
	fd = open(str, O_RDONLY, 0);
	if (fd < 0) {
		error = 1;
		p->error = 1;
	}

	p = &ns->prompt[PROMPTNNUE];
	str = prompt_str(p);
	if (strlen(str)) {
		nnuefd = open(str, O_RDONLY, 0);
		if (nnuefd < 0) {
			error = 1;
			p->error = 1;
		}
	}

	p = &ns->prompt[PROMPTSIMD];
	simd = prompt_str(p);

	if (ns->toggle[TOGGLEDRAW].state)
		adjudicate |= ADJUDICATE_DRAW;
	if (ns->toggle[TOGGLERESIGN].state)
		adjudicate |= ADJUDICATE_RESIGN;

	char passphrase[48] = { 0 };

	if (error || prompt_passphrase(passphrase, 48)) {
		if (fd >= 0)
			close(fd);
		if (nnuefd >= 0)
			close(nnuefd);
		draw_prompts(ns);
		return;
	}

	draw_newtest(ns);

	char response = RESPONSEPERMISSIONDENIED;
	if (sendf(ns->ssl, "cs", REQUESTPRIVILEGE, passphrase) || recvf(ns->ssl, "c", &response))
		lostcon();

	if (response != RESPONSEOK) {
		if (fd >= 0)
			close(fd);
		if (nnuefd >= 0)
			close(nnuefd);

		infobox("Permission denied.");
		return;
	}

	if (sendf(ns->ssl, "c", REQUESTNEWTEST) || 
		sendf(ns->ssl, "csDDDDDcsss",
			type, tc, alpha,
			beta, elo0, elo1, eloe, adjudicate, branch, commit, simd) ||
		sendf(ns->ssl, "ff", fd, nnuefd))
		lostcon();
	close(fd);
	if (nnuefd >= 0)
		close(nnuefd);

	if (recvf(ns->ssl, "c", &response)) {
		lostcon();
	}
	else if (response != RESPONSEOK) {
		infobox("An unexpected error occured.");
	}
	else {
		infobox("The test has been put in queue.");
		for (int i = PROMPTTIME; i <= PROMPTSIMD; i++)
			reset_prompt(&ns->prompt[i]);
		for (int i = TOGGLEDRAW; i <= TOGGLERESIGN; i++)
			reset_toggle(&ns->toggle[i]);
		ns->selected = 0;
	}
}
