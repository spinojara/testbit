#include "auth.h"

#include <crypt.h>
#include <shadow.h>
#include <string.h>
#include <stdio.h>
#include <pwd.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>
#include <errno.h>

char passphrase[PASSPHRASE_MAX];

static void zero(void *p, size_t size) {
	volatile unsigned char *v = p;
	while (size--)
		*v++ = 0;
}

int read_passphrase(void) {
	struct termios saved;
	int tty = isatty(STDIN_FILENO);
	if (tty) {
		struct termios noecho;
		if (tcgetattr(STDIN_FILENO, &saved))
			return 1;
		noecho = saved;
		noecho.c_lflag &= ~ECHO;
		/* TCSAFLUSH discards anything typed before echo was switched off. */
		if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &noecho))
			return 1;
		fprintf(stderr, "Passphrase: ");
	}

	size_t n = 0;
	int error = 0;
	while (1) {
		char c;
		ssize_t r = read(STDIN_FILENO, &c, 1);
		if (r < 0) {
			if (errno == EINTR)
				continue;
			error = 1;
			break;
		}
		if (r == 0 || c == '\n')
			break;
		/* Truncating silently would only turn into a confusing 401. */
		if (n + 1 >= sizeof(passphrase)) {
			error = 1;
			break;
		}
		passphrase[n++] = c;
	}

	if (tty) {
		tcsetattr(STDIN_FILENO, TCSAFLUSH, &saved);
		fprintf(stderr, "\n");
	}

	if (n > 0 && passphrase[n - 1] == '\r')
		n--;

	passphrase[n] = '\0';

	if (error || n == 0) {
		zero(passphrase, sizeof(passphrase));
		return 1;
	}

	return 0;
}

int authorize(const char *user_password) {
	struct spwd spwd = { 0 };
	struct spwd *spwdp = NULL;
	char buf[4096];
	if (getspnam_r("testbit", &spwd, buf, sizeof(buf), &spwdp) || !spwdp) {
		printf("getspname?\n");
		return 0;
	}
	if (!user_password)
		return 0;
	const char *colon = strchr(user_password, ':');
	if (!colon)
		return 0;
	const char *password = colon + 1;
	struct crypt_data data = { 0 };
	const char *hash = crypt_r(password, spwd.sp_pwdp, &data);
	if (!hash)
		return 0;

	return !strcmp(hash, spwd.sp_pwdp);
}

int su(const char *user) {
	struct passwd pwd = { 0 };
	char buf[16384] = { 0 };
	struct passwd *result;
	if (getpwnam_r(user, &pwd, buf, sizeof(buf), &result) || !result) {
		fprintf(stderr, "error: no user '%s'\n", user);
		return 1;
	}

	if (setgid(pwd.pw_gid) || setuid(pwd.pw_uid) || setegid(pwd.pw_gid) || seteuid(pwd.pw_uid)) {
		fprintf(stderr, "error: failed to switch user\n");
		return 1;
	}

	return 0;
}

int user_info(const char *user, uid_t *uid, gid_t *gid) {
	struct passwd pwd = { 0 };
	char buf[16384] = { 0 };
	struct passwd *result;
	if (getpwnam_r(user, &pwd, buf, sizeof(buf), &result) || !result) {
		fprintf(stderr, "error: no user '%s'\n", user);
		return 1;
	}
	*uid = pwd.pw_uid;
	*gid = pwd.pw_gid;
	return 0;
}
