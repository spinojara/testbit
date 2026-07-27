#include "auth.h"

#include <crypt.h>
#include <shadow.h>
#include <string.h>
#include <stdio.h>
#include <pwd.h>
#include <stdlib.h>
#include <unistd.h>

int authorize(const char *user_password) {
	struct spwd spwd = { 0 };
	struct spwd *spwdp = NULL;
	char buf[4096];
	if (getspnam_r("testbit", &spwd, buf, sizeof(buf), &spwdp) || !spwdp) {
		printf("getspname?\n");
		return 0;
	}
	printf("%s\n", spwd.sp_pwdp);
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
