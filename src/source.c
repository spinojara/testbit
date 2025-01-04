#define _POSIX_C_SOURCE 200809L
#include "source.h"

#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <pwd.h>

#include "user.h"
#include "util.h"

int git_clone(char *dtemp, const char *branch, const char *commit) {
	int wstatus;
	pid_t pid;

	if (chdir("/tmp")) {
		fprintf(stderr, "error: chdir /tmp\n");
		exit(7);
	}

	struct passwd *pw = getpwnam("testbit");
	if (!mkdtemp(dtemp) || !pw || chown(dtemp, pw->pw_uid, pw->pw_gid)) {
		fprintf(stderr, "error: mkdtemp %s\n", dtemp);
		exit(8);
	}

	pid = fork();
	if (pid == -1)
		exit(9);

	if (pid == 0) {
		su("testbit");
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
		su("testbit");
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
		su("testbit");
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

int make(const char *simd, const char *nnue) {
	int wstatus;
	pid_t pid;

	pid = fork();
	if (pid == -1)
		exit(49);

	if (pid == 0) {
		su("testbit");
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

	char simdstr[4096], nnuestr[4096];
	snprintf(simdstr, sizeof(simdstr), "SIMD=%s", simd);
	snprintf(nnuestr, sizeof(nnuestr), "NNUE=%s", nnue);

	if (pid == 0) {
		su("testbit");
		execlp("make", "make", simdstr, nnuestr, "bitbit", (char *)NULL);
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
