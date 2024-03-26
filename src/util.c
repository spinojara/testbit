#define _POSIX_C_SOURCE 1
#define _XOPEN_SOURCE 500
#include "util.h"

#include <string.h>
#include <stdio.h>
#include <ftw.h>

char *appendstr(char *dest, const char *src) {
	size_t i, n;
	for (n = 0; dest[n]; n++);
	for (i = 0; i <= strlen(src); i++)
		dest[n + i] = src[i];
	return dest;
}

char *iso8601tm(char *str, const struct tm *tm) {
	char buf[16] = { 0 };
	strftime(buf, 16, "%z", tm);
	if (buf[3] != ':') {
		buf[5] = buf[4];
		buf[4] = buf[3];
		buf[3] = ':';
	}
	if (!strcmp(buf, "+00:00")) {
		buf[0] = 'Z';
		buf[1] = '\0';
	}
	strftime(str, 28, "%FT%T", tm);
	appendstr(str, buf);
	return str;
}

char *iso8601local(char *str, time_t t) {
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
