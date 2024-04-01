#include "done.h"

#include <math.h>

#include "oldtest.h"
#include "sprt.h"
#include "util.h"
#include "color.h"

static void attr(const struct oldteststate *os, int i, int j) {
	if (i == 0 || j != 1)
		return;
	struct test *test = &os->test[i - 1];
	switch (test->status) {
	case TESTCANCEL:
		wattrset(os->win, cs.yellow.attr);
		break;
	case TESTERRBRANCH:
		wattrset(os->win, cs.error.attr);
		break;
	case TESTERRCOMMIT:
		wattrset(os->win, cs.error.attr);
		break;
	case TESTERRPATCH:
		wattrset(os->win, cs.error.attr);
		break;
	case TESTERRMAKE:
		wattrset(os->win, cs.error.attr);
		break;
	case TESTERRRUN:
		wattrset(os->win, cs.error.attr);
		break;
	case TESTINCONCLUSIVE:
		wattrset(os->win, cs.yellow.attr);
		break;
	case TESTH0:
		wattrset(os->win, cs.error.attr);
		break;
	case TESTH1:
		wattrset(os->win, cs.green.attr);
		break;
	case TESTELO:
		wattrset(os->win, cs.yellow.attr);
		break;
	}
}

void draw_done(struct oldteststate *os) {
	int tests = os->tests + 1;

	char (*select)[128] = calloc(tests, sizeof(*select));
	char (*status)[128] = calloc(tests, sizeof(*status));
	char (*tc)[128] = calloc(tests, sizeof(*tc));
	char (*elo)[128] = calloc(tests, sizeof(*elo));
	char (*llr)[128] = calloc(tests, sizeof(*llr));
	char (*ab)[128] = calloc(tests, sizeof(*ab));
	char (*elo0)[128] = calloc(tests, sizeof(*elo0));
	char (*elo1)[128] = calloc(tests, sizeof(*elo1));
	char (*eloe)[128] = calloc(tests, sizeof(*eloe));
	char (*tri)[128] = calloc(tests, sizeof(*tri));
	char (*penta)[128] = calloc(tests, sizeof(*penta));
	char (*qtime)[128] = calloc(tests, sizeof(*qtime));
	char (*stime)[128] = calloc(tests, sizeof(*stime));
	char (*dtime)[128] = calloc(tests, sizeof(*dtime));
	char (*branch)[128] = calloc(tests, sizeof(*branch));
	char (*commit)[128] = calloc(tests, sizeof(*commit));

	sprintf(select[0], " ");
	sprintf(status[0], "Status");
	sprintf(tc[0], "TC");
	sprintf(elo[0], "Elo");
	sprintf(llr[0], "LLR");
	sprintf(ab[0], "(A, B)");
	sprintf(elo0[0], "Elo0");
	sprintf(elo1[0], "Elo1");
	sprintf(eloe[0], "EloE");
	sprintf(tri[0], "Trinomial");
	sprintf(penta[0], "Pentanomial");
	sprintf(qtime[0], "Queue Timestamp");
	sprintf(stime[0], "Start Timestamp");
	sprintf(dtime[0], "Done Timestamp");
	sprintf(branch[0], "Branch");
	sprintf(commit[0], "Commit");

	char tmp1[128], tmp2[128];
	for (int i = 1; i < tests; i++) {
		struct test *test = &os->test[i - 1];
		test->alpha = test->beta = 0.025;
		int games = test->t0 + test->t1 + test->t2 > 0;
		double A = log(test->beta / (1.0 - test->alpha));
		double B = log((1.0 - test->beta) / test->alpha);

		sprintf(tc[i], "%s+%s", fstr(tmp1, test->maintime, 2), fstr(tmp2, test->increment, 2));
		strcpy(branch[i], test->branch);
		strcpy(commit[i], test->commit);
		iso8601local(qtime[i], test->qtime);
		iso8601local(stime[i], test->stime);
		iso8601local(dtime[i], test->dtime);
		if (games) {
			sprintf(elo[i], "%.2lf+-%.2lf", fabs(test->elo) <= 0.005 ? 0.0 : test->elo, test->pm);
			sprintf(tri[i], "%u-%u-%u", test->t0, test->t1, test->t2);
			sprintf(penta[i], "%u-%u-%u-%u-%u", test->p0, test->p1, test->p2, test->p3, test->p4);
		}
		else {
			sprintf(elo[i], "N/A");
			sprintf(tri[i], "N/A");
			sprintf(penta[i], "N/A");
		}
		if (test->type == TESTTYPESPRT) {
			if (games)
				sprintf(llr[i], "%.2lf", test->llr);
			else
				sprintf(llr[i], "N/A");
			sprintf(ab[i], "(%.2lf, %.2lf)", A, B);
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

		switch (test->status) {
		case TESTCANCEL:
			sprintf(status[i], "Cancelled");
			break;
		case TESTERRBRANCH:
			sprintf(status[i], "Branch not found");
			break;
		case TESTERRCOMMIT:
			sprintf(status[i], "Commit not found");
			break;
		case TESTERRPATCH:
			sprintf(status[i], "Failed to apply patch");
			break;
		case TESTERRMAKE:
			sprintf(status[i], "Failed to compile");
			break;
		case TESTERRRUN:
			sprintf(status[i], "Runtime error");
			break;
		case TESTINCONCLUSIVE:
			sprintf(status[i], "Inconclusive");
			break;
		case TESTH0:
			sprintf(status[i], "H0 accepted");
			break;
		case TESTH1:
			sprintf(status[i], "H1 accepted");
			break;
		case TESTELO:
			sprintf(status[i], "Done");
			break;
		}
	}
	
	draw_dynamic(os, &attr, 1, select, status, tc, elo, tri, penta, llr, ab, elo0, elo1, eloe, dtime, stime, qtime, branch, commit, NULL);

	free(select);
	free(status);
	free(tc);
	free(elo);
	free(llr);
	free(ab);
	free(elo0);
	free(elo1);
	free(eloe);
	free(tri);
	free(penta);
	free(qtime);
	free(stime);
	free(dtime);
	free(branch);
	free(commit);
}
