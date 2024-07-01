#define _DEFAULT_SOURCE
#include "setup.h"

#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>

#include "con.h"
#include "util.h"
#include "sprt.h"
#include "req.h"

void kill_parent(void) {
	pid_t pid = getppid();
	kill(pid, SIGKILL);
}

int git_clone(char *dtemp, const char *branch, const char *commit) {
	int wstatus;
	pid_t pid;

	if (chdir("/tmp")) {
		fprintf(stderr, "error: chdir /tmp\n");
		exit(7);
	}

	if (!mkdtemp(dtemp)) {
		fprintf(stderr, "error: mkdtemp %s\n", dtemp);
		exit(8);
	}

	pid = fork();
	if (pid == -1)
		exit(9);

	if (pid == 0) {
		execlp("git", "git", "clone",
				"https://github.com/spinojara/bitbit.git",
				"--branch", branch,
				"--single-branch",
				dtemp, (char *)NULL);
		fprintf(stderr, "error: exec git clone\n");
		kill_parent();
		exit(10);
	}

	if (waitpid(pid, &wstatus, 0) == -1) {
		fprintf(stderr, "error: waitpid\n");
		exit(11);
	}

	if (chdir(dtemp)) {
		fprintf(stderr, "error: chdir %s\n", dtemp);
		exit(12);
	}

	if (WEXITSTATUS(wstatus)) {
		fprintf(stderr, "error: git clone\n");
		return 1;
	}

	pid = fork();
	if (pid == -1)
		exit(13);

	if (pid == 0) {
		execlp("git", "git", "reset", "--hard", commit, (char *)NULL);
		fprintf(stderr, "error: exec git reset\n");
		kill_parent();
		exit(14);
	}

	if (waitpid(pid, &wstatus, 0) == -1) {
		fprintf(stderr, "error: waitpid\n");
		exit(15);
	}

	if (WEXITSTATUS(wstatus)) {
		fprintf(stderr, "error: git reset\n");
		return 2;
	}

	return 0;
}

int git_patch(void) {
	int wstatus;
	pid_t pid = fork();
	if (pid == -1)
		exit(21);

	if (pid == 0) {
		execlp("git", "git", "apply", "patch", (char *)NULL);
		fprintf(stderr, "error: exec git apply\n");
		kill_parent();
		exit(22);
	}

	if (waitpid(pid, &wstatus, 0) == -1) {
		fprintf(stderr, "error: waitpid\n");
		exit(23);
	}

	if (WEXITSTATUS(wstatus)) {
		fprintf(stderr, "error: git apply\n");
		return 1;
	}

	return 0;
}

int make(void) {
	int wstatus;
	pid_t pid;

	pid = fork();
	if (pid == -1)
		exit(49);

	if (pid == 0) {
		execlp("make", "make", "clean", (char *)NULL);
		fprintf(stderr, "error: exec make\n");
		kill_parent();
		exit(50);
	}

	if (waitpid(pid, &wstatus, 0) == -1) {
		fprintf(stderr, "error: waitpid\n");
		exit(51);
	}

	if (WEXITSTATUS(wstatus)) {
		fprintf(stderr, "error: make clean");
		return 1;
	}

	pid = fork();
	if (pid == -1)
		exit(24);

	if (pid == 0) {
		execlp("make", "make", "SIMD=avx2", "bitbit", (char *)NULL);
		fprintf(stderr, "error: exec make\n");
		kill_parent();
		exit(25);
	}

	if (waitpid(pid, &wstatus, 0) == -1) {
		fprintf(stderr, "error: waitpid\n");
		exit(26);
	}

	if (WEXITSTATUS(wstatus)) {
		fprintf(stderr, "error: make\n");
		return 1;
	}

	return 0;
}

void setup(SSL *ssl, int type, int cpus, char *syzygy, double maintime,
		double increment, double alpha, double beta, double elo0,
		double elo1, double eloe, int adjudicate,
		const char *branch, const char *commit) {
	
	char dtemp[] = "testbitn-bitbit-XXXXXX";
	int r, fd, error = 0;
	if ((r = git_clone(dtemp, branch, commit))) {
		sendf(ssl, "cc", REQUESTNODEDONE, r == 1 ? TESTERRBRANCH : TESTERRCOMMIT);
		error = 1;
	}
	/* We are now inside dtemp even if an error occured. */
	if ((fd = open("patch", O_WRONLY | O_CREAT | O_TRUNC, 0644)) < 0) {
		fprintf(stderr, "open patch\n");
		exit(16);
	}

	if (recvf(ssl, "f", fd, -1))
		exit(20);

	if (close(fd)) {
		fprintf(stderr, "error: close patch\n");
		exit(17);
	}

	if (error)
		goto cleanup;

	if (make()) {
		/* This should probably never fail. But if a bad commit
		 * is pushed to the github it can fail.
		 */
		sendf(ssl, "cc", REQUESTNODEDONE, TESTERRMAKE);
		goto cleanup;
	}

	if (rename("bitbit", "bitbitold")) {
		fprintf(stderr, "error: rename\n");
		exit(27);
	}

	if (git_patch()) {
		sendf(ssl, "cc", REQUESTNODEDONE, TESTERRPATCH);
		goto cleanup;
	}

	if (make()) {
		sendf(ssl, "cc", REQUESTNODEDONE, TESTERRMAKE);
		goto cleanup;
	}

	sprt(ssl, type, cpus, syzygy, maintime, increment, alpha, beta, elo0, elo1, eloe, adjudicate);

cleanup:
	if (chdir("/tmp")) {
		fprintf(stderr, "error: chdir /tmp\n");
		exit(18);
	}

	if (rmdir_r(dtemp)) {
		fprintf(stderr, "error: rmdir %s\n", dtemp);
		exit(19);
	}
}
