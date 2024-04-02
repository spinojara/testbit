#include "active.h"

#include <math.h>

#include "oldtest.h"
#include "sprt.h"
#include "util.h"
#include "color.h"

static void attr(const struct oldteststate *os, int i, int j) {
	if (j == 1 && i > 0) {
		wattrset(os->win, os->test[i - 1].status == TESTQUEUE ? cs.yellow.attr : cs.green.attr);
	}
}

char *etastr(char *buf, struct test *test, double A, double B) {
	if (test->t0 + test->t1 + test->t2 == 0)
		return NULL;

	time_t duration = test->dtime - test->stime;
	if (test->type == TESTTYPESPRT) {
		if (fabs(test->llr) < eps)
			return NULL;

		double goal = test->llr > 0.0 ? B : A;
		time_t expected = test->stime + goal / test->llr * duration;
		iso8601local(buf, expected);
	}
	else if (test->type == TESTTYPEELO) {
		time_t expected = test->stime + test->pm * test->pm * duration / (test->eloe * test->eloe);
		iso8601local(buf, expected);
	}
	return buf;
}

void draw_active(struct oldteststate *os) {
	int tests = os->tests + 1;

	char (*select)[128] = calloc(tests, sizeof(*select));
	char (*status)[128] = calloc(tests, sizeof(*status));
	char (*tc)[128] = calloc(tests, sizeof(*tc));
	char (*adj)[128] = calloc(tests, sizeof(*adj));
	char (*elo)[128] = calloc(tests, sizeof(*elo));
	char (*llr)[128] = calloc(tests, sizeof(*llr));
	char (*ab)[128] = calloc(tests, sizeof(*ab));
	char (*elo0)[128] = calloc(tests, sizeof(*elo0));
	char (*elo1)[128] = calloc(tests, sizeof(*elo1));
	char (*eloe)[128] = calloc(tests, sizeof(*eloe));
	char (*tri)[128] = calloc(tests, sizeof(*tri));
	char (*penta)[128] = calloc(tests, sizeof(*penta));
	char (*eta)[128] = calloc(tests, sizeof(*eta));
	char (*qtime)[128] = calloc(tests, sizeof(*qtime));
	char (*stime)[128] = calloc(tests, sizeof(*stime));
	char (*branch)[128] = calloc(tests, sizeof(*branch));
	char (*commit)[128] = calloc(tests, sizeof(*commit));

	sprintf(select[0], " ");
	sprintf(status[0], "Status");
	sprintf(tc[0], "TC");
	sprintf(adj[0], "Adj");
	sprintf(elo[0], "Elo");
	sprintf(llr[0], "LLR");
	sprintf(ab[0], "(A, B)");
	sprintf(elo0[0], "Elo0");
	sprintf(elo1[0], "Elo1");
	sprintf(eloe[0], "EloE");
	sprintf(tri[0], "Trinomial");
	sprintf(penta[0], "Pentanomial");
	sprintf(eta[0], "ETA");
	sprintf(qtime[0], "Queue Timestamp");
	sprintf(stime[0], "Start Timestamp");
	sprintf(branch[0], "Branch");
	sprintf(commit[0], "Commit");

	char tmp1[128], tmp2[128];
	for (int i = 1; i < tests; i++) {
		struct test *test = &os->test[i - 1];
		int started = test->status == TESTRUN && (test->t0 + test->t1 + test->t2);
		double A = log(test->beta / (1.0 - test->alpha));
		double B = log((1.0 - test->beta) / test->alpha);

		switch (test->adjudicate) {
		case 0:
			sprintf(adj[i], "None");
			break;
		case ADJUDICATE_DRAW:
			sprintf(adj[i], "Draw");
			break;
		case ADJUDICATE_RESIGN:
			sprintf(adj[i], "Resign");
			break;
		case ADJUDICATE_DRAW | ADJUDICATE_RESIGN:
			sprintf(adj[i], "Both");
			break;
		}
		sprintf(tc[i], "%s+%s", fstr(tmp1, test->maintime, 2), fstr(tmp2, test->increment, 2));
		strcpy(branch[i], test->branch);
		strcpy(commit[i], test->commit);
		iso8601local(qtime[i], test->qtime);
		if (started) {
			sprintf(elo[i], "%.2lf+-%.2lf", fabs(test->elo) <= 0.005 ? 0.0 : test->elo, test->pm);
		}
		else {
			sprintf(elo[i], "N/A");
		}
		if (test->status == TESTQUEUE) {
			sprintf(status[i], "Pending");
			sprintf(tri[i], "N/A");
			sprintf(penta[i], "N/A");
			sprintf(eta[i], "N/A");
			sprintf(stime[i], "N/A");
		}
		else {
			sprintf(status[i], "Running");
			sprintf(tri[i], "%u-%u-%u", test->t0, test->t1, test->t2);
			sprintf(penta[i], "%u-%u-%u-%u-%u", test->p0, test->p1, test->p2, test->p3, test->p4);
			if (!etastr(eta[i], test, A, B))
				sprintf(eta[i], "N/A");
			iso8601local(stime[i], test->stime);
		}
		if (test->type == TESTTYPESPRT) {
			sprintf(ab[i], "(%.2lf, %.2lf)", A, B);
			if (started)
				sprintf(llr[i], "%.2lf", test->llr);
			else
				sprintf(llr[i], "N/A");
			sprintf(elo0[i], "%.1lf", test->elo0);
			sprintf(elo1[i], "%.1lf", test->elo1);
			sprintf(eloe[i], "N/A");
		}
		else {
			sprintf(llr[i], "N/A");
			sprintf(ab[i], "N/A");
			sprintf(elo0[i], "N/A");
			sprintf(elo1[i], "N/A");
			sprintf(eloe[i], "%.1lf", test->eloe);
		}
	}
	
	draw_dynamic(os, &attr, 1, select, status, tc, adj, elo, tri, penta, llr, ab, elo0, elo1, eloe, eta, stime, qtime, branch, commit, NULL);

	free(select);
	free(status);
	free(tc);
	free(adj);
	free(elo);
	free(llr);
	free(ab);
	free(elo0);
	free(elo1);
	free(eloe);
	free(tri);
	free(penta);
	free(eta);
	free(qtime);
	free(stime);
	free(branch);
	free(commit);
}
