#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/stat.h>

#include "source.h"
#include "user.h"
#include "util.h"

int bench(void) {
	int pipefd[2];
	if (pipe(pipefd))
		exit(28);

	pid_t pid = fork();
	if (pid == -1)
		exit(29);

	if (pid == 0) {
		su("testbit");
		close(pipefd[0]);
		close(STDOUT_FILENO);

		dup2(pipefd[1], STDOUT_FILENO);
		
		execlp("./bitbit", "./bitbit", "bench,", "quit", (char *)NULL);

		fprintf(stderr, "error: exec bitbit");
		kill_parent();
		exit(30);
	}

	close(pipefd[1]);
	FILE *f = fdopen(pipefd[0], "r");

	char line[BUFSIZ];
	int total_time = -1;
	char *endptr;
	while (fgets(line, sizeof(line), f)) {
		if (strncmp(line, "time: ", 6))
			continue;

		errno = 0;
		total_time = strtol(&line[6], &endptr, 10);
		break;
	}
	fclose(f);

	int wstatus;
	if (waitpid(pid, &wstatus, 0) == -1) {
		fprintf(stderr, "error: waitpid\n");
		exit(31);
	}

	if (WEXITSTATUS(wstatus)) {
		fprintf(stderr, "error: bitbit\n");
		exit(32);
	}

	if (total_time < 0 || *endptr != ' ' || errno) {
		fprintf(stderr, "error: bench\n");
		exit(33);
	}

	return total_time;
}

int main(void) {
	mkdir("/etc/bitbit", 0777);
	FILE *f = fopen("/etc/bitbit/tcfactor", "w");
	if (!f) {
		fprintf(stderr, "error: failed to open file /etc/bitbit/tcfactor\n");
		return 1;
	}
	char dtemp[] = "tcfactor-bitbit-XXXXXX";
	int r;
	/* commit: Clear tt before bench */
	if ((r = git_clone(dtemp, "master", "c7ee42b")))
		return r;

	if ((r = make()))
		return r;

	int average = 0;
	int N = 10;
	for (int i = 0; i < N; i++) {
		int total_time = bench();
		printf("time: %d ms\n", total_time);
		average += bench();
	}

	average /= N;

	printf("average: %d ms\n", average);

	double tcfactor = (double)average / 6619 * 2681 / 2054;

	printf("tcfactor: %lf\n", tcfactor);
	fprintf(f, "%lf\n", tcfactor);
	fclose(f);
	if (chdir("/tmp")) {
		fprintf(stderr, "error: chdir /tmp\n");
		return 18;
	}
	if ((r = rmdir_r(dtemp))) {
		fprintf(stderr, "error: rmdir_r %s\n", dtemp);
		return r;
	}
}
