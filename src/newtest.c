#include "newtest.h"

#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include "draw.h"
#include "sprt.h"
#include "req.h"
#include "infobox.h"
#include "util.h"

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

	PROMPTQUEUE,

	PROMPTCOUNT,
};

int allow_float(chtype ch) {
	return ('0' <= ch && ch <= '9') || ch == '-' || ch == '.';
}

int allow_time(chtype ch) {
	return allow_float(ch) || ch == '+';
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
		}
		/* fallthrough */
	case 'i':
		if (ns->selected <= PROMPTPATH) {
			redraw = 1;
			handle_prompt(&ns->prompt[ns->selected]);
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

#ifdef WINDOWS_TERMINAL_BUG
	wrefresh(win);
#endif
}

void draw_button(WINDOW *win, const char *title, int y, int x, int n, int highlight) {
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

#ifdef WINDOWS_TERMINAL_BUG
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
	draw_prompt_border(ns->win, "Path", 25, 2, x - 4, ns->selected == PROMPTPATH, ns->prompt[PROMPTPATH].error);

	draw_button(ns->win, "Submit Test", 30, (x - 15)/ 2, 15, ns->selected == PROMPTQUEUE);

	for (int i = PROMPTTIME; i <= PROMPTPATH; i++)
		draw_prompt(&ns->prompt[i]);

	wrefresh(ns->win);
}

void draw_newtest(struct newteststate *ns) {
	draw_border(ns->win, NULL, &cs.bordershadow, &cs.border, 1, 0, 0, getmaxy(ns->win), getmaxx(ns->win));

	draw_prompts(ns);
}

void init_newtest(struct newteststate *ns) {
	/* x will be the size of the full window at start up because
	 * we have not yet reduced the size of the smallest windows.
	 */
	int x = getmaxx(ns->win);

	new_prompt(&ns->prompt[PROMPTTIME], ns->win, 2, 4, x - 18, "10+0.1", &allow_time);
	new_prompt(&ns->prompt[PROMPTALPHA], ns->win, 5, 4, x - 18, "0.025", &allow_float);
	new_prompt(&ns->prompt[PROMPTBETA], ns->win, 8, 4, x - 18, "0.025", &allow_float);
	new_prompt(&ns->prompt[PROMPTELO0], ns->win, 11, 4, x - 18, "0.0", &allow_float);
	new_prompt(&ns->prompt[PROMPTELO1], ns->win, 14, 4, x - 18, "4.0", &allow_float);
	new_prompt(&ns->prompt[PROMPTELOE], ns->win, 17, 4, x - 18, "0.0", &allow_float);
	new_prompt(&ns->prompt[PROMPTBRANCH], ns->win, 20, 4, x - 18, "master", &allow_text);
	new_prompt(&ns->prompt[PROMPTCOMMIT], ns->win, 23, 4, x - 18, "HEAD", &allow_text);
	new_prompt(&ns->prompt[PROMPTPATH], ns->win, 26, 4, x - 18, "", &allow_text);
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
}

void queue_test(struct newteststate *ns) {
	struct prompt *p;
	char *str, *c, *err, *err2 = NULL;

	const double zero = eps;
	const double one = 1.0 - zero;

	char type;
	double maintime;
	double increment;
	double alpha;
	double beta;
	double elo0;
	double elo1;
	double eloe;
	const char *branch, *commit;
	int fd = -1;

	int error = 0;

	p = &ns->prompt[PROMPTTIME];
	str = prompt_str(p);
	c = strchr(str, '+');
	errno = 0;
	if (c) {
		increment = strtod(c + 1, &err2);
		*c = '\0';
	}
	else
		increment = 0.0;

	int olderrno = errno;
	errno = 0;
	maintime = strtod(str, &err);
	errno = olderrno || errno;

	if (*err != '\0' || (err2 && *err2 != '\0') || maintime <= 0.1 || increment < 0.0 || errno) {
		error = 1;
		p->error = 1;
	}

	if (c)
		*c = '+';

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

	char passphrase[48] = { 0 };

	if (error || prompt_passphrase(passphrase, 48)) {
		if (fd >= 0)
			close(fd);
		draw_prompts(ns);
		return;
	}

	draw_newtest(ns);

	char response = RESPONSEPERMISSIONDENIED;
	sendf(ns->ssl, "cs", REQUESTPRIVILEGE, passphrase);
	recvf(ns->ssl, "c", &response);

	if (response != RESPONSEOK) {
		if (fd >= 0)
			close(fd);

		infobox("Permission Denied.");
		return;
	}

	sendf(ns->ssl, "c", REQUESTNEWTEST);
	sendf(ns->ssl, "cDDDDDDDss",
			type, maintime, increment, alpha,
			beta, elo0, elo1, eloe, branch, commit);
	sendf(ns->ssl, "f", fd);
	close(fd);

	if (recvf(ns->ssl, "c", &response) || response != RESPONSEOK) {
		infobox("An unexpected error occured.");
	}
	else {
		infobox("The test has been put in queue.");
		for (int i = 0; i < 9; i++)
			reset_prompt(&ns->prompt[i]);
	}
}
