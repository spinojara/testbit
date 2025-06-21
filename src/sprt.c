#define _POSIX_C_SOURCE 1
#include "sprt.h"

#include <math.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>

#include "util.h"
#include "req.h"
#include "elo.h"
#include "setup.h"
#include "user.h"
#include "tc.h"

enum {
	RESULTNONE = -1,
	RESULTLOSS,
	RESULTDRAW,
	RESULTWIN,
};

struct game {
	int done;
	int result;
};

void parse_finished_game(char *line, struct game *game, int max) {
	char *endptr;
	int n = strtol(line + 14, &endptr, 10) - 1;
	if (*endptr != ' ' || n < 0 || n >= max)
		exit(42);

	int white;
	if (strstr(line, "(bitbit vs bitbitold)"))
		white = 1;
	else if (strstr(line, "(bitbitold vs bitbit)"))
		white = 0;
	else
		exit(39);

	int result = RESULTNONE;

	if (strstr(line, "): 1-0")) {
		if (result != RESULTNONE)
			exit(40);
		result = white ? RESULTWIN : RESULTLOSS;
	}
	if (strstr(line, "): 0-1")) {
		if (result != RESULTNONE)
			exit(40);
		result = white ? RESULTLOSS : RESULTWIN;
	}
	if (strstr(line, "): 1/2-1/2")) {
		if (result != RESULTNONE)
			exit(40);
		result = RESULTDRAW;
	}

	if (result == RESULTNONE)
		exit(41);

	game[n].done = 1;
	game[n].result = result;
}

#define APPENDARG(str) (argv[argc++] = (str))
int run_games(int games, int concurrency, char *syzygy, const char *tc, int adjudicate, int epoch, int32_t tri[3], int32_t penta[5]) {
	if (games % 2)
		exit(39);
	int wstatus;

	char gamesstr[1024];
	char concurrencystr[1024];

	char fulltc[131];
	snprintf(fulltc, 131, "tc=%s", tc);
	fulltc[130] = '\0';

	sprintf(gamesstr, "%d", games / 2);
	sprintf(concurrencystr, "%d", concurrency);

	int pipefd[2];
	if (pipe(pipefd))
		exit(28);

	pid_t pid = fork();
	if (pid == -1)
		exit(29);

	if (pid == 0) {
		close(pipefd[0]);
		close(STDOUT_FILENO);

		dup2(pipefd[1], STDOUT_FILENO);

		char cpus[BUFSIZ] = { 0 };
		FILE *f = fopen("/sys/fs/cgroup/testbit/cpuset.cpus", "r");
		if (!f || !fgets(cpus, sizeof(cpus), f))
			exit(29);
		fclose(f);
		cpus[strcspn(cpus, "\n")] = '\0';

		char *argv[64];
		int argc = 0;
		APPENDARG("fastchess");
		APPENDARG("-concurrency"); APPENDARG(concurrencystr);
		APPENDARG("-each"); APPENDARG(fulltc);
		APPENDARG("proto=uci"); APPENDARG("timemargin=10000");
		APPENDARG("-rounds"); APPENDARG(gamesstr);
		APPENDARG("-games"); APPENDARG("2");
		APPENDARG("-openings"); APPENDARG("format=epd");
		APPENDARG("file=etc/book/testbit-50cp5d6m100k.epd"); APPENDARG("order=random");
		APPENDARG("-repeat");
		APPENDARG("-use-affinity"); APPENDARG(cpus);
		if (epoch % 2) {
			APPENDARG("-engine"); APPENDARG("cmd=./bitbit"); APPENDARG("name=bitbit");
			APPENDARG("-engine"); APPENDARG("cmd=./bitbitold"); APPENDARG("name=bitbitold");
		}
		else {
			APPENDARG("-engine"); APPENDARG("cmd=./bitbitold"); APPENDARG("name=bitbitold");
			APPENDARG("-engine"); APPENDARG("cmd=./bitbit"); APPENDARG("name=bitbit");
		}
		if (adjudicate & ADJUDICATE_DRAW) {
			APPENDARG("-draw"); APPENDARG("movenumber=60");
			APPENDARG("movecount=8"); APPENDARG("score=20");
		}
		if (adjudicate & ADJUDICATE_RESIGN) {
			APPENDARG("-resign"); APPENDARG("movecount=3");
			APPENDARG("score=800"); APPENDARG("twosided=true");
		}
		if (syzygy) {
			APPENDARG("-tb"); APPENDARG(syzygy);
		}
		APPENDARG(NULL);
		su("testbit");
		execvp("fastchess", argv);

		fprintf(stderr, "error: exec fastchess");
		kill_parent();
		exit(30);
	}

	if (waitpid(pid, &wstatus, 0) == -1) {
		fprintf(stderr, "error: waitpid\n");
		exit(31);
	}

	if (WEXITSTATUS(wstatus)) {
		close(pipefd[0]);
		close(pipefd[1]);
		exit(59);
	}

	struct game *game = calloc(games, sizeof(*game));

	close(pipefd[1]);
	FILE *f = fdopen(pipefd[0], "r");
	if (!f) {
		fprintf(stderr, "error: fdopen fastchess\n");
		exit(31);
	}

	char line[BUFSIZ];
	while (fgets(line, sizeof(line), f)) {
		printf("%s", line);
		if (!strstr(line, "Finished game"))
			continue;
		parse_finished_game(line, game, games);
	}

	int error = 0;

	for (int pair = 0; pair < games / 2; pair++) {
		int first = 2 * pair;
		int second = 2 * pair + 1;

		int done = game[first].done && game[second].done;
		first = game[first].result;
		second = game[second].result;

		if (!done || first == RESULTNONE || second == RESULTNONE) {
			error = 1;
			break;
		}

		penta[first + second]++;
		tri[first]++;
		tri[second]++;
	}

	fclose(f);
	free(game);

	return error;
}

void sprt(SSL *ssl, int type, int cpus, char *syzygy, const char *tc, double alpha, double beta, double elo0, double elo1, double eloe, int adjudicate) {
	sendf(ssl, "c", REQUESTNODESTART);

	double A = log(beta / (1.0 - alpha));
	double B = log((1.0 - beta) / alpha);

	int32_t tri[3] = { 0 };
	int32_t penta[5] = { 0 };

	long moves = 0;
	double maintime = 0;
	double increment = 0;
	tcinfo(tc, &moves, &maintime, &increment);

	int expected_moves = 75;
	double tcs = moves == 0 || moves >= expected_moves ? 1.0 : (double)expected_moves / moves;

	double gametime = 2.0 * tcs * (maintime + (moves == 0 || moves >= expected_moves ? expected_moves : moves) * increment);
	double seconds = 180.0;
	int batch_size = max(2, seconds * cpus / gametime);
	int epoch = 0;
	batch_size = 2 * (batch_size / 2);

	char status = TESTINCONCLUSIVE;

	while (status == TESTINCONCLUSIVE) {
		if (run_games(batch_size, cpus, syzygy, tc, adjudicate, epoch++, tri, penta)) {
			status = TESTERRRUN;
			break;
		}

		double llr;
		double elo, pm;
 		llr = type == TESTTYPESPRT ? loglikelihoodratio(penta, elo0, elo1) : 0.0 / 0.0;
		elo = elo_calc(penta, &pm);

		if (type == TESTTYPESPRT) {
			if (llr >= B)
				status = TESTH1;
			else if (llr <= A)
				status = TESTH0;
		}
		else if (type == TESTTYPEELO) {
			if (fabs(pm) <= eloe)
				status = TESTELO;
		}

		char cancel;
		if (sendf(ssl, "cllllllllDDD",
					REQUESTNODEUPDATE,
					tri[0], tri[1], tri[2],
					penta[0], penta[1], penta[2], penta[3], penta[4],
					llr, elo, pm) ||
				recvf(ssl, "c", &cancel) || cancel || status != TESTINCONCLUSIVE)
			break;
	}
	
	sendf(ssl, "cc", REQUESTNODEDONE, status);
}
