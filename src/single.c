#define _XOPEN_SOURCE 500
#include "single.h"

#include <unistd.h>
#include <stdlib.h>

#include "tui.h"
#include "color.h"
#include "draw.h"
#include "req.h"
#include "con.h"
#include "oldtest.h"

extern const int refresh_interval;

void free_single(struct oldteststate *os) {
	free(os->singletest.branch);
	free(os->singletest.commit);
	os->singletest.branch = NULL;
	os->singletest.commit = NULL;
	if (os->fd < 0)
		return;
	close(os->fd);
	unlink(os->path);
	os->fd = -1;
}

void handle_single(struct oldteststate *os, chtype ch) {
	int update = 0;
	switch (ch) {
	case KEY_ESC:
		os->single = 0;
		os->last_loaded = 0;
		free_single(os);
		break;
	case 'r':
		os->last_loaded = 0;
		update = 1;
	default:
		break;
	}

	if (update)
		draw_single(os, 1, 1);
}

void draw_table(struct oldteststate *os);
void draw_patch(struct oldteststate *os);
int load_single(struct oldteststate *os);
int load_patch(struct oldteststate *os);

void draw_single(struct oldteststate *os, int lazy, int load) {
	if (os->fd < 0)
		load_patch(os);

	if (load && (time(NULL) - os->last_loaded > refresh_interval)) {
		load_single(os);
		lazy = 0;
	}

	if (!lazy)
		draw_border(os->win, NULL, &cs.bordershadow, &cs.border, 1, 0, 0, getmaxy(os->win), getmaxx(os->win));

	draw_table(os);

	draw_patch(os);
}

int load_single(struct oldteststate *os) {
	sendf(os->ssl, "ccq", REQUESTLOGTEST, OLDTESTSINGLE, os->selected_id);
	char response;
	recvf(os->ssl, "c", &response);
	if (response)
		return 1;
	free(os->singletest.branch);
	free(os->singletest.commit);
	os->singletest.branch = NULL;
	os->singletest.commit = NULL;
	return recvf(os->ssl, "ccDDDDDDDDDDssqqqLLLLLLLL",
			&os->singletest.type, &os->singletest.status,
			&os->singletest.maintime, &os->singletest.increment,
			&os->singletest.alpha, &os->singletest.beta,
			&os->singletest.llr, &os->singletest.elo0,
			&os->singletest.elo1, &os->singletest.eloe,
			&os->singletest.elo, &os->singletest.pm,
			&os->singletest.branch, &os->singletest.commit,
			&os->singletest.qtime, &os->singletest.stime,
			&os->singletest.dtime, &os->singletest.t0,
			&os->singletest.t1, &os->singletest.t2,
			&os->singletest.p0, &os->singletest.p1,
			&os->singletest.p2, &os->singletest.p3,
			&os->singletest.p4);
}

int load_patch(struct oldteststate *os) {
	sprintf(os->path, "/tmp/patch-testbit-XXXXXX");
	os->fd = mkstemp(os->path);

	sendf(os->ssl, "cq", REQUESTPATCH, os->selected_id);
	char response;
	recvf(os->ssl, "c", &response);
	if (response)
		return 1;
	return recvf(os->ssl, "f", os->fd);
}

void draw_table(struct oldteststate *os) {
}

void draw_patch(struct oldteststate *os) {
}
