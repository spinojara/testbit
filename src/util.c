#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 1
#endif
#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 500
#endif
#include "util.h"

#include <string.h>
#include <stdio.h>
#include <ftw.h>
#include <signal.h>
#include <unistd.h>

char *iso8601tm(char *str, const struct tm *tm) {
	strftime(str, 28, "%F %T", tm);
	return str;
}

char *iso8601local(char *str, time_t t) {
	if (t == 0) {
		sprintf(str, "N/A");
		return str;
	}
	struct tm local;
	localtime_r(&t, &local);

	return iso8601tm(str, &local);
}

int rm(const char *path, const struct stat *sb, int typeflag, struct FTW *ftwbuf) {
	(void)sb;
	(void)typeflag;
	(void)ftwbuf;
	int r = remove(path);
	if (r)
		perror(path);
	return r;
}

int rmdir_r(const char *path) {
	return nftw(path, rm, 64, FTW_DEPTH | FTW_PHYS);
}

void kill_parent(void) {
	pid_t pid = getppid();
	kill(pid, SIGKILL);
}
