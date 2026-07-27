#define _GNU_SOURCE
#include "build.h"

#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>
#include <sys/pidfd.h>
#include <poll.h>

#include "util.h"
#include "auth.h"

static char *mkdtemp_testbit(char *template) {
	uid_t uid;
	gid_t gid;
	if (user_info("testbit", &uid, &gid))
		return NULL;

	if (!mkdtemp(template))
		printf("Cannot make %s?\n", template);

	if (chown(template, uid, gid))
		return NULL;

	return template;
}

static int mkstemp_testbit(char *template) {
	uid_t uid;
	gid_t gid;
	if (user_info("testbit", &uid, &gid))
		return -1;

	int fd;
	if ((fd = mkostemp(template, O_CLOEXEC)) == -1)
		return -1;

	if (fchown(fd, uid, gid)) {
		unlink(template);
		return -1;
	}

	return fd;
}

int interruptable_fgets(char *buf, size_t size, struct fdreader *fdr, int stop_fd) {
	if (size == 0)
		return 1;
	size_t real_size = size - 1;
	size_t received = 0;
	while (received < real_size) {
		ssize_t size_to_copy = fdlen(fdr, stop_fd);
		if (size_to_copy <= 0) {
			if (received > 0)
				break;
			return 1;
		}

		char *newline = strchr(fdr->buf, '\n');

		if (newline && size_to_copy + fdr->buf > newline + 1)
			size_to_copy = newline - fdr->buf + 1;

		if (received + size_to_copy > real_size)
			size_to_copy = real_size - received;

		fdtake(fdr, buf + received, size_to_copy);
		received += size_to_copy;

		if (newline)
			break;
	}
	buf[received] = 0;
	return 0;
}

int interruptable_waitpid(pid_t pid, int *wstatus, int stop_fd) {
	int ret = 0;
	*wstatus = 0;
	int pidfd = pidfd_open(pid, 0);
	struct pollfd pfd[2] = {
		{ .fd = stop_fd, .events = POLLIN },
		{ .fd = pidfd, .events = POLLIN },
	};
	while (1) {
		if (poll(pfd, 2, -1) < 0) {
			if (errno == EINTR)
				continue;
			exit(171);
		}

		if (pfd[0].revents & POLLIN) {
			kill(-pid, SIGKILL);
			ret = 1;
			break;
		}

		if (pfd[1].revents & POLLIN)
			break;
	}

	while (waitpid(pid, wstatus, 0) < 0)
		if (errno != EINTR)
			exit(104);

	close(pidfd);
	return ret;
}

void kill_parent(void) {
	pid_t pid = getppid();
	kill(pid, SIGKILL);
}

void send_error(CURL *curl, const char *url, int id, int task_id, const char *message) {
	printf("sending error: '%s'\n", message);
	char *errorurl = calloc(strlen(url) + 1000, 1);
	if (!errorurl)
		exit(160);
	sprintf(errorurl, "%s/test/error/%d", url, id);

	cJSON *json = cJSON_CreateObject();
	cJSON_AddNumberToObject(json, "taskid", task_id);
	cJSON_AddStringToObject(json, "errorlog", message);
	char *body = cJSON_PrintUnformatted(json);

	struct curl_slist *headers = NULL;
	headers = curl_slist_append(headers, "Content-Type: application/json");

	curl_easy_setopt(curl, CURLOPT_URL, errorurl);
	curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, NULL);
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body);
	curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long)strlen(body));

	curl_easy_perform(curl);

	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, NULL);
	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, NULL);

	curl_slist_free_all(headers);
	cJSON_free(body);
	cJSON_Delete(json);
	free(errorurl);
}

/* TODO: Add protection for long running commands and cancel them after a timeout. */
int execvp_wrapper(int stop_fd, CURL *curl, const char *url, int id, int task_id, char *const argv[]) {
	int fd[2];
	if (pipe2(fd, O_CLOEXEC) < 0)
		exit(105);

	printf("executing:");
	for (size_t i = 0; argv[i]; i++)
		printf(" %s", argv[i]);
	printf("\n");

	pid_t pid = fork();
	if (pid < 0)
		exit(106);

	if (pid == 0) {
		setpgid(0, 0);
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
	setpgid(pid, pid);

	close(fd[1]);
	char *out = read_fd(fd[0], stop_fd);
	close(fd[0]);

	/* out is only NULL if we are stopping. */
	int exiting = out == NULL;

	int wstatus;
	if (interruptable_waitpid(pid, &wstatus, stop_fd) || exiting) {
		free(out);
		return 1;
	}


	if (!WIFEXITED(wstatus) || WEXITSTATUS(wstatus)) {
		send_error(curl, url, id, task_id, out);
		free(out);
		return 1;
	}

	free(out);
	return 0;
}

int execlp_wrapper(int stop_fd, CURL *curl, const char *url, int id, int task_id, ...) {
	va_list ap;
	va_start(ap, task_id);

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


	int ret = execvp_wrapper(stop_fd, curl, url, id, task_id, argv);
	printf("returning %d\n", ret);
	free(argv);
	return ret;
}

int build_test(struct test *test, int task_id, const char *patch, const char *simd, const char *commit, int tune, CURL *curl, const char *url, int stop_fd) {
	int id = test->id;
	printf("building test\n");
	char template[] = "/tmp/testbit-XXXXXX";
	if (!mkdtemp_testbit(template))
		exit(109);

	char oldfile[64];
	char newfile[64];
	char bitbit[64];
	sprintf(oldfile, "%s/bitbit-old", template);
	sprintf(newfile, "%s/bitbit-new", template);
	sprintf(bitbit, "%s/bitbit", template);

	test->dir = strdup(template);
	if (execlp_wrapper(stop_fd, curl, url, id, task_id, "git", "clone", "https://github.com/spinojara/bitbit.git", test->dir, (char *)NULL))
		return 1;

	if (execlp_wrapper(stop_fd, curl, url, id, task_id, "git", "-C", test->dir, "checkout", commit, (char *)NULL))
		return 1;

	if (execlp_wrapper(stop_fd, curl, url, id, task_id, "make", "-C", test->dir, "clean", (char *)NULL))
		return 1;

	char *realsimd = malloc((simd ? strlen(simd) : 0) + 6);
	if (!realsimd)
		exit(112);

	char realtune[16];
	sprintf(realtune, "TUNE=%s", tune ? "yes" : "");
	sprintf(realsimd, "SIMD=%s", simd ? simd : "");

	if (execlp_wrapper(stop_fd, curl, url, id, task_id, "make", "-C", test->dir, "bitbit-pgo", realsimd, realtune, (char *)NULL)) {
		free(realsimd);
		return 1;
	}

	if (rename(bitbit, oldfile))
		exit(116);

	char patchfile[] = "/tmp/testbit-patch-XXXXXX";
	int fd;
	if ((fd = mkstemp_testbit(patchfile)) == -1)
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

	if (execlp_wrapper(stop_fd, curl, url, id, task_id, "make", "-C", test->dir, "clean", (char *)NULL)) {
		free(realsimd);
		unlink(patchfile);
		return 1;
	}

	if (execlp_wrapper(stop_fd, curl, url, id, task_id, "git", "-C", test->dir, "apply", "--allow-empty", patchfile, (char *)NULL)) {
		free(realsimd);
		unlink(patchfile);
		return 1;
	}

	unlink(patchfile);

	if (execlp_wrapper(stop_fd, curl, url, id, task_id, "make", "-C", test->dir, "bitbit-pgo", realsimd, realtune, (char *)NULL)) {
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
int fastchess(CURL *curl, const char *url, int id, int task_id, const struct cpu *cpu, const char *dir, const char *adjudicate, char *syzygy, char *tc, cJSON *argsplus, cJSON *argsminus, int stop_fd) {
	int argc = 0;
	char *argv[8192];
	if (argsplus && cJSON_GetArraySize(argsplus) > 2048) {
		send_error(curl, url, id, task_id, "error: too many args\n");
		return 1;
	}
	if (argsminus && cJSON_GetArraySize(argsminus) > 2048) {
		send_error(curl, url, id, task_id, "error: too many args\n");
		return 1;
	}
	char pgnfile[] = "/tmp/testbit-pgn-XXXXXX";
	int pgnfilefd;
	if ((pgnfilefd = mkstemp_testbit(pgnfile)) == -1)
		exit(150);
	close(pgnfilefd);

	char pgnfilearg[256];
	sprintf(pgnfilearg, "file=%s", pgnfile);

	int new_first = rand() % 2;

	char openingfile[256];
	sprintf(openingfile, "file=%s/etc/book/testbit-50cp5d6m100k.epd", dir);
	char processname[128];
	sprintf(processname, "fastchess-%d", cpu->cpu);
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
	ARG("-pgnout"); ARG(pgnfilearg); ARG("nodes=true"); ARG("min=true");
	ARG("-openings"); ARG("format=epd"); ARG(openingfile); ARG("order=random");
	ARG("-repeat");

	cJSON *json;
	for (int i = 0; i < 2; i++) {
		if (i != new_first) {
			ARG("-engine"); ARG(new); ARG("name=bitbit-new");
			if (argsplus) {
				json = NULL;
				cJSON_ArrayForEach(json, argsplus) {
					if (!cJSON_IsString(json)) {
						unlink(pgnfile);
						return 1;
					}
					ARG(json->valuestring);
				}
			}
		}
		else {
			ARG("-engine"); ARG(old); ARG("name=bitbit-old");
			if (argsminus) {
				json = NULL;
				cJSON_ArrayForEach(json, argsminus) {
					if (!cJSON_IsString(json)) {
						unlink(pgnfile);
						return 1;
					}
					ARG(json->valuestring);
				}
			}
		}
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
	if (pipe2(fd, O_CLOEXEC) < 0)
		exit(105);

	pid_t pid = fork();
	if (pid < 0)
		exit(106);

	if (pid == 0) {
		setpgid(0, 0);
		char file[4096];
		sprintf(file, "/sys/fs/cgroup/testbit-%d/cgroup.procs", cpu->cpu);
		/* e = O_CLOEXEC */
		FILE *f = fopen(file, "we");
		if (!f) {
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
	setpgid(pid, pid);

	close(fd[1]);

	int stats[3] = { 0 };

	char buf[4096];
	size_t size = 0;
	char *out = calloc(size + 1, 1);
	int error = 0;

	struct fdreader fdr = { .fd = fd[0] };
	while (!interruptable_fgets(buf, sizeof(buf), &fdr, stop_fd)) {
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

		printf("line: %s\n", buf);

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
	close(fd[0]);
	printf("waitpid\n");

	int wstatus;
	if (interruptable_waitpid(pid, &wstatus, stop_fd)) {
		free(out);
		unlink(pgnfile);
		return 1;
	}

	if (!WIFEXITED(wstatus)) {
		free(out);
		unlink(pgnfile);
		return 1;
	}

	int w = stats[2];
	int d = stats[1];
	int l = stats[0];

	if (WEXITSTATUS(wstatus) || error || w + d + l != 2) {
		printf("sending error because: %d, %d, %d\n", WEXITSTATUS(wstatus), error, w + d + l != 2);
		send_error(curl, url, id, task_id, out);
		free(out);
		unlink(pgnfile);
		return 1;
	}
	printf("finished games!\n");
	free(out);

	/* e = O_CLOEXEC */
	FILE *f = fopen(pgnfile, "re");
	if (!f)
		exit(175);

	struct stat st;
	if (fstat(fileno(f), &st))
		exit(185);

	/* A pgnfile of two games should never be this large. */
	if (st.st_size > 128 * 1024 * 1024)
		exit(195);

	char *pgn = calloc(st.st_size + 1, 1);
	if (!pgn)
		exit(196);

	fread(pgn, 1, st.st_size, f);
	fclose(f);
	unlink(pgnfile);


	char *responseurl = calloc(strlen(url) + 1000, 1);
	if (!responseurl)
		exit(160);
	sprintf(responseurl, "%s/test/%d", url, id);

	json = cJSON_CreateObject();
	cJSON_AddNumberToObject(json, "taskid", task_id);
	cJSON_AddNumberToObject(json, "wins", w);
	cJSON_AddNumberToObject(json, "draws", d);
	cJSON_AddNumberToObject(json, "losses", l);
	cJSON_AddStringToObject(json, "pgn", pgn);
	char *body = cJSON_PrintUnformatted(json);

	struct curl_slist *headers = NULL;
	headers = curl_slist_append(headers, "Content-Type: application/json");

	curl_easy_setopt(curl, CURLOPT_URL, responseurl);
	curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, NULL);
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body);
	curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long)strlen(body));

	curl_easy_perform(curl);

	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, NULL);
	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, NULL);

	curl_slist_free_all(headers);
	cJSON_free(body);
	cJSON_Delete(json);
	free(responseurl);
	free(pgn);
	return 0;
}
