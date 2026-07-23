#include "build.h"

#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <errno.h>

#include "util.h"
#include "auth.h"

void kill_parent(void) {
	fprintf(stderr, "killing parent\n");
	pid_t pid = getppid();
	kill(pid, SIGKILL);
	while (waitpid(pid, NULL, 0) < 0 && errno == EINTR);
}

void send_error(CURL *curl, const char *message) {
	printf("sending error: '%s'\n", message);
}

/* TODO: Add protection for long running commands and cancel them after a timeout. */
int execvp_wrapper(CURL *curl, char *const argv[]) {
	int fd[2];
	if (pipe(fd) < 0)
		exit(105);

	printf("executing:");
	for (size_t i = 0; argv[i]; i++)
		printf(" %s", argv[i]);
	printf("\n");

	pid_t pid = fork();
	if (pid < 0)
		exit(106);

	if (pid == 0) {
		if (su("testbit")) {
			kill_parent();
			exit(109);
		}
		close(fd[0]);
		dup2(fd[1], STDOUT_FILENO);
		dup2(fd[1], STDERR_FILENO);
		close(fd[1]);
		execvp(argv[0], argv);
		kill_parent();
		exit(103);
	}

	close(fd[1]);
	char *out = read_fd(fd[0]);
	close(fd[0]);

	int wstatus;
	while (waitpid(pid, &wstatus, 0) < 0)
		if (errno != EINTR)
			exit(104);

	if (!WIFEXITED(wstatus) || WEXITSTATUS(wstatus)) {
		send_error(curl, out);
		free(out);
		return 1;
	}

	free(out);
	return 0;
}

int execlp_wrapper(CURL *curl, ...) {
	va_list ap;
	va_start(ap, curl);

	size_t argc = 0;
	char **argv = NULL;

	char *arg;
	do {
		arg = va_arg(ap, char *);

		argc++;
		argv = realloc(argv, argc * sizeof(*argv));
		argv[argc - 1] = arg;
	} while (arg);
	va_end(ap);


	int ret = execvp_wrapper(curl, argv);
	printf("returning %d\n", ret);
	free(argv);
	return ret;
}

int build_test(struct test *test, const char *patch, const char *simd, const char *commit, CURL *curl) {
	printf("building test\n");
	char template[] = "/tmp/testbit-XXXXXX";
	if (!mkdtemp(template))
		exit(109);

	char oldfile[64];
	char newfile[64];
	char bitbit[64];
	sprintf(oldfile, "%s/bitbit-old", template);
	sprintf(newfile, "%s/bitbit-new", template);
	sprintf(bitbit, "%s/bitbit", template);

	test->dir = strdup(template);
	if (execlp_wrapper(curl, "git", "clone", "https://github.com/spinojara/bitbit.git", test->dir, (char *)NULL))
		return 1;

	if (execlp_wrapper(curl, "git", "-C", test->dir, "checkout", commit, (char *)NULL))
		return 1;

	if (execlp_wrapper(curl, "make", "-C", test->dir, "clean", (char *)NULL))
		return 1;

	char *realsimd = malloc((simd ? strlen(simd) : 0) + 6);
	if (!realsimd)
		exit(112);

	sprintf(realsimd, "SIMD=%s", simd ? simd : "");

	if (execlp_wrapper(curl, "make", "-C", test->dir, "bitbit-pgo", realsimd, (char *)NULL)) {
		free(realsimd);
		return 1;
	}

	if (rename(bitbit, oldfile))
		exit(116);

	char patchfile[] = "/tmp/testbit-patch-XXXXXX";
	int fd;
	if ((fd = mkstemp(patchfile)) == -1)
		exit(110);

	size_t size = strlen(patch);
	size_t written = 0;
	ssize_t n = 0;
	while (written < size) {
		n = write(fd, patch + written, size - written);
		if (n <= 0)
			exit(111);
		written += n;

	}
	close(fd);

	if (execlp_wrapper(curl, "make", "-C", test->dir, "clean", (char *)NULL)) {
		free(realsimd);
		unlink(patchfile);
		return 1;
	}

	if (execlp_wrapper(curl, "git", "-C", test->dir, "apply", "--allow-empty", patchfile, (char *)NULL)) {
		free(realsimd);
		unlink(patchfile);
		return 1;
	}

	unlink(patchfile);

	if (execlp_wrapper(curl, "make", "-C", test->dir, "bitbit-pgo", realsimd, (char *)NULL)) {
		free(realsimd);
		return 1;
	}
	free(realsimd);

	if (rename(bitbit, newfile))
		exit(116);

	printf("built test\n");

	return 0;
}

#define ARG(str) (argv[argc++] = (str))
int fastchess(CURL *curl, const struct cpu *cpu, const char *dir, const char *adjudicate, char *syzygy, char *tc) {
	int argc = 0;
	char *argv[128];
#warning fix args
	char pgnfile[] = "/tmp/testbit-pgn-XXXXXX";
	int fd;
	if ((fd = mkstemp(pgnfile)) == -1)
		exit(150);
	close(fd);

	int new_first = rand() % 2;

	char openingfile[256];
	sprintf(openingfile, "file=%s/etc/book/testbit-50cp5d6m100k.epd", dir);
	char processname[128];
	char old[256];
	char new[256];
	sprintf(old, "cmd=%s/bitbit-old", dir);
	sprintf(new, "cmd=%s/bitbit-new", dir);

	/* clang-format: off */
	ARG(processname);
	ARG("-testEnv");
	ARG("-concurrency"); ARG("1");
	ARG("-each"); ARG(tc);
	ARG("proto=uci"); ARG("timemargin=10000");
	ARG("option.Debug=true");
	ARG("-rounds"); ARG("1");
	ARG("-games"); ARG("2");
	ARG("-pgnout"); ARG(pgnfile); ARG("nodes=true"); ARG("min=true");
	ARG("-openings"); ARG("format=epd"); ARG(openingfile); ARG("order=random");
	ARG("-repeat");

	if (new_first) {
		ARG("-engine"); ARG(new); ARG("name=bitbit-new");
		ARG("-engine"); ARG(old); ARG("name=bitbit-old");
	}
	else {
		ARG("-engine"); ARG(old); ARG("name=bitbit-old");
		ARG("-engine"); ARG(new); ARG("name=bitbit-new");
	}

	if (!strcmp(adjudicate, "draw") || !strcmp(adjudicate, "both")) {
		ARG("-draw"); ARG("movenumber=40"); ARG("movecount=8"); ARG("score=10");
	}
	if (!strcmp(adjudicate, "resign") || !strcmp(adjudicate, "both")) {
		ARG("-resign"); ARG("twosided=true"); ARG("movecount=3"); ARG("score=800");
	}

	if (syzygy) {
		ARG("-tb"); ARG(syzygy);
	}
	ARG(NULL);
	/* clang-format: on */

	int fd[2];
	if (pipe(fd) < 0)
		exit(105);

	pid_t pid = fork();
	if (pid < 0)
		exit(106);

	if (pid == 0) {
		char file[4096];
		sprintf(file, "/sys/fs/cgroup/testbit-%d/cgroup.procs", cpu->cpu);
		FILE *f = fopen(file, "w");
		if (!f) {
			fprintf(stderr, "no cgroup?\n");
			kill_parent();
			exit(135);
		}
		fprintf(f, "%d\n", getpid());
		fclose(f);
		if (su("testbit")) {
			kill_parent();
			exit(109);
		}
		close(fd[0]);
		dup2(fd[1], STDOUT_FILENO);
		dup2(fd[1], STDERR_FILENO);
		close(fd[1]);
		execvp("fastchess", argv);
		kill_parent();
		exit(103);
	}

	close(fd[1]);
	FILE *f = fdopen(fd[0], "r");
	if (!f)
		exit(132);

	int stats[3] = { 0 };

	char buf[4096];
	size_t size = 0;
	char *out = calloc(size + 1, 1);
	int error = 0;
	while (fgets(buf, sizeof(buf), f)) {
		size_t n = strlen(buf);
		out = realloc(out, size + n + 1);
		if (!out)
			exit(151);
		memcpy(out + size, buf, n);
		out[size + n] = 0;
		size += n;
		if (size >= 16 * 1024 * 1024) {
			error = 1;
			break;
		}

		if (strstr(buf, "Finished game ") == buf) {
			int white;
			if (strstr(buf, " (bitbit-new vs bitbit-old): "))
				white = 1;
			else if (strstr(buf, " (bitbit-old vs bitbit-new): "))
				white = 0;
			else
				exit(151);

			int score;
			if (strstr(buf, ": 1-0 "))
				score = 2;
			else if (strstr(buf, ": 1/2-1/2 "))
				score = 1;
			else if (strstr(buf, ": 0-1 "))
				score = 0;
			else
				exit(152);

			stats[white ? score : 2 - score]++;
		}
	}
	fclose(f);

	int wstatus;
	while (waitpid(pid, &wstatus, 0) < 0)
		if (errno != EINTR)
			exit(104);

	if (!WIFEXITED(wstatus)) {
		unlink(pgnfile);
		return 1;
	}

	int w = stats[2];
	int d = stats[1];
	int l = stats[0];

	if (WEXITSTATUS(wstatus) || error || w + d + l != 2) {
		send_error(curl, out);
		free(out);
		unlink(pgnfile);
		return 1;
	}
	printf("finished games!\n");
	free(out);

	unlink(pgnfile);
	return 0;
}
