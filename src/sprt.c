#define _POSIX_C_SOURCE 1
#define _GNU_SOURCE
#include "sprt.h"

#include <math.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <sched.h>
#include <fcntl.h>

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

struct process {
	int running;
	pid_t pid;
	FILE *r;
};

void write_pgn(int ncpu, int *cpus) {
	FILE *g = fopen("full.pgn", "w");
	if (!g)
		exit(104);

	for (int i = 0; i < ncpu; i++) {
		int cpu = cpus[i];
		char buf[BUFSIZ];
		sprintf(buf, "%d.pgn", cpu);
		FILE *f = fopen(buf, "r");
		if (!f)
			continue;

		while (fgets(buf, sizeof(buf), f))
			fputs(buf, g);

		fclose(f);
	}

	fclose(g);
}

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

int parse_fastchess(FILE *r, int32_t tri[3], int32_t penta[5]) {
	char line[16384];
	struct game game[2] = { 0 };
	while (fgets(line, sizeof(line), r)) {
		fprintf(stderr, "fastchess: %s", line);
		if (strncmp(line, "Finished game", 13))
			continue;
		parse_finished_game(line, game, 2);
	}

	int done = game[0].done && game[1].done;

	int first = game[0].result;
	int second = game[1].result;

	if (!done || first == RESULTNONE || second == RESULTNONE)
		return 1;

	penta[first + second]++;
	tri[first]++;
	tri[second]++;

	return 0;
}

#define APPENDARG(str) (argv[argc++] = (str))
int run_games(int cpu, struct process *proc, char *syzygy, const char *tc, int adjudicate, int epoch) {
	char fulltc[131];
	snprintf(fulltc, 131, "tc=%s", tc);
	fulltc[130] = '\0';

	char pgnfile[128];
	sprintf(pgnfile, "file=%d.pgn", cpu);

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

		FILE *f = fopen("/sys/fs/cgroup/testbit/cgroup.procs", "w");
		if (!f)
			exit(102);
		fprintf(f, "%d\n", getpid());
		if (fclose(f))
			exit(103);

		struct sched_param param = { 99 };
		sched_setscheduler(0, SCHED_FIFO, &param);

		cpu_set_t mask;
		CPU_ZERO(&mask);
		CPU_SET(cpu, &mask);

		sched_setaffinity(0, sizeof(cpu_set_t), &mask);

		char *argv[64];
		int argc = 0;
		APPENDARG("fastchess");
		APPENDARG("-testEnv");
		APPENDARG("-concurrency"); APPENDARG("1");
		APPENDARG("-each"); APPENDARG(fulltc);
		APPENDARG("proto=uci"); APPENDARG("timemargin=10000");
		APPENDARG("-rounds"); APPENDARG("1");
		APPENDARG("-games"); APPENDARG("2");
		APPENDARG("-pgnout"); APPENDARG(pgnfile);
		APPENDARG("nodes=true"); APPENDARG("seldepth=true");
		APPENDARG("nps=true"); APPENDARG("hashfull=true");
		APPENDARG("tbhits=true"); APPENDARG("timeleft=true");
		APPENDARG("-openings"); APPENDARG("format=epd");
		APPENDARG("file=etc/book/testbit-50cp5d6m100k.epd"); APPENDARG("order=random");
		APPENDARG("-repeat");
		if (epoch % 2) {
			APPENDARG("-engine"); APPENDARG("cmd=./bitbit"); APPENDARG("name=bitbit-new");
			APPENDARG("-engine"); APPENDARG("cmd=./bitbitold"); APPENDARG("name=bitbit-old");
		}
		else {
			APPENDARG("-engine"); APPENDARG("cmd=./bitbitold"); APPENDARG("name=bitbit-old");
			APPENDARG("-engine"); APPENDARG("cmd=./bitbit"); APPENDARG("name=bitbit-new");
		}
		if (adjudicate & ADJUDICATE_DRAW) {
			APPENDARG("-draw"); APPENDARG("movenumber=40");
			APPENDARG("movecount=8"); APPENDARG("score=10");
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

	close(pipefd[1]);
	proc->r = fdopen(pipefd[0], "r");
	if (!proc->r) {
		fprintf(stderr, "error: fdopen fastchess\n");
		exit(31);
	}

	proc->pid = pid;
	proc->running = 1;

	return 0;
}

void sprt(SSL *ssl, int type, int ncpus, int *cpus, char *syzygy, const char *tc, double alpha, double beta, double elo0, double elo1, double eloe, int adjudicate) {
	sendf(ssl, "c", REQUESTNODESTART);

	double A = log(beta / (1.0 - alpha));
	double B = log((1.0 - beta) / alpha);

	int32_t tri[3] = { 0 };
	int32_t penta[5] = { 0 };

	long moves = 0;
	double maintime = 0;
	double increment = 0;
	tcinfo(tc, &moves, &maintime, &increment);

	int epoch = 0;

	char status = TESTINCONCLUSIVE;

	struct process *procs = calloc(ncpus, sizeof(*procs));

	char cancel = 0;
	while (status == TESTINCONCLUSIVE && !cancel) {
		/* Start games. */
		for (int i = 0; i < ncpus; i++) {
			if (procs[i].running)
				continue;

			run_games(cpus[i], &procs[i], syzygy, tc, adjudicate, epoch++);
		}

		int any_done = 0;
		/* Done games. */
		for (int i = 0; i < ncpus; i++) {
			int wstatus;
			switch (waitpid(procs[i].pid, &wstatus, WNOHANG)) {
			case -1:
				exit(31);
			case 0:
				continue;
			default:
				break;
			}

			any_done = 1;

			procs[i].running = 0;

			if (WEXITSTATUS(wstatus) || parse_fastchess(procs[i].r, tri, penta)) {
				status = TESTERRRUN;
				fclose(procs[i].r);
				break;
			}

			fclose(procs[i].r);

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

			if (sendf(ssl, "cllllllllDDD",
						REQUESTNODEUPDATE,
						tri[0], tri[1], tri[2],
						penta[0], penta[1], penta[2], penta[3], penta[4],
						llr, elo, pm) ||
					recvf(ssl, "c", &cancel))
				exit(100);
			if (cancel || status != TESTINCONCLUSIVE)
				break;
		}

		/* Sleep for 100 ms. */
		if (!any_done)
			usleep(100000);
	}

	for (int i = 0; i < ncpus; i++) {
		if (!procs[i].running)
			continue;
		fclose(procs[i].r);
		if (kill(procs[i].pid, SIGKILL))
			exit(101);
	}

	write_pgn(ncpus, cpus);

	int fd = open("full.pgn", O_RDONLY, 0);
	if (fd < 0)
		exit(105);

	sendf(ssl, "ccf", REQUESTNODEDONE, status, fd);

	free(procs);
}
