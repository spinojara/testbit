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

void parse_finished_game(char *line, struct game *game) {
	int n = strtol(line + 14, NULL, 10) - 1;

	int white = n % 2;

	int result = RESULTNONE;

	int error = 0;
	if (strstr(line, "1-0")) {
		if (result != RESULTNONE)
			error = 1;
		result = white ? RESULTWIN : RESULTLOSS;
	}
	if (strstr(line, "0-1")) {
		if (result != RESULTNONE)
			error = 1;
		result = white ? RESULTLOSS : RESULTWIN;
	}
	if (strstr(line, "1/2-1/2")) {
		if (result != RESULTNONE)
			error = 1;
		result = RESULTDRAW;
	}

	game[n].done = 1;
	game[n].result = error ? RESULTNONE : result;
}

#define APPENDARG(str) (argv[argc++] = (str))
int run_games(int games, int nthreads, double maintime, double increment, int adjudicate, int32_t tri[3], int32_t penta[5]) {
	if (games % 2)
		exit(39);
	int wstatus;

	char gamesstr[1024];
	char concurrencystr[1024];
	char tc[1024];
	sprintf(gamesstr, "%d", games);
	sprintf(concurrencystr, "%d", nthreads);
	sprintf(tc, "tc=%lg+%lg", maintime, increment);

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

		char *argv[32];
		int argc = 0;
		APPENDARG("cutechess-cli");
		APPENDARG("-variant"); APPENDARG("standard");
		APPENDARG("-tournament"); APPENDARG("gauntlet");
		APPENDARG("-concurrency"); APPENDARG(concurrencystr);
		APPENDARG("-each"); APPENDARG(tc); APPENDARG("proto=uci");
		APPENDARG("-games"); APPENDARG(gamesstr);
		APPENDARG("-openings"); APPENDARG("format=epd");
		APPENDARG("file=etc/book/testbit-50cp5d6m100k.epd"); APPENDARG("order=random");
		APPENDARG("-repeat");
		APPENDARG("-engine"); APPENDARG("cmd=./bitbitold"); APPENDARG("name=bitbitold");
		APPENDARG("-engine"); APPENDARG("cmd=./bitbit");
		if (adjudicate & ADJUDICATE_DRAW) {
			APPENDARG("-draw"); APPENDARG("movenumber=60");
			APPENDARG("movecount=8"); APPENDARG("score=20");
		}
		if (adjudicate & ADJUDICATE_RESIGN) {
			APPENDARG("-resign"); APPENDARG("movecount=3");
			APPENDARG("score=800"); APPENDARG("twosided=true");
		}
		APPENDARG(NULL);
		execvp("cutechess-cli", argv);

		fprintf(stderr, "error: exec cutechess-cli");
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
		return 1;
	}

	struct game *game = calloc(games, sizeof(*game));

	close(pipefd[1]);
	FILE *f = fdopen(pipefd[0], "r");
	if (!f) {
		fprintf(stderr, "error: fdopen cutechess-cli\n");
		exit(31);
	}

	char line[BUFSIZ];
	while (fgets(line, sizeof(line), f)) {
		printf("%s", line);
		if (!strstr(line, "Finished game"))
			continue;
		parse_finished_game(line, game);
	}

	int error = 0;

	for (int pair = 0; pair < games / 2; pair++) {
		int first = 2 * pair;
		int second = 2 * pair + 1;

		first = game[first].result;
		second = game[second].result;

		if (first == RESULTNONE || second == RESULTNONE) {
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

void sprt(SSL *ssl, int type, int games, int nthreads, double maintime, double increment, double alpha, double beta, double elo0, double elo1, double eloe, int adjudicate) {
	sendf(ssl, "c", REQUESTNODESTART);
	games = 2 * (games / 2);

	double A = log(beta / (1.0 - alpha));
	double B = log((1.0 - beta) / alpha);

	int32_t tri[3] = { 0 };
	int32_t penta[5] = { 0 };

	double gametime = 2.0 * (maintime + 75 * increment);
	double seconds = 180.0;
	int batch_size = max(2, seconds * nthreads / gametime);
	batch_size = 2 * (batch_size / 2);

	char status = TESTINCONCLUSIVE;

	while (games > 0 && status == TESTINCONCLUSIVE) {
		int batch = min(batch_size, games);
		if (run_games(batch, nthreads, maintime, increment, adjudicate, tri, penta)) {
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

		games -= batch;
	}
	
	sendf(ssl, "cc", REQUESTNODEDONE, status);
}
