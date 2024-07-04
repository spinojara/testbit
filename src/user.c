#define _POSIX_C_SOURCE 200112L
#include "user.h"

#include <stdlib.h>
#include <unistd.h>
#include <pwd.h>
#include <errno.h>
#include <stdio.h>

void su(const char *user) {
	struct passwd *pwd;
	if ((pwd = getpwnam(user)) == NULL) {
		printf("error: bad user \"testbit\"\n");
		exit(70);
	}

	if (setgid(pwd->pw_gid) || setuid(pwd->pw_uid) || setegid(pwd->pw_gid) || seteuid(pwd->pw_uid)) {
		printf("error: failed to switch user\n");
		exit(71);
	}
}
