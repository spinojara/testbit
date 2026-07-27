#ifndef AUTH_H
#define AUTH_H

#include <unistd.h>

int authorize(const char *user_password);

int su(const char *user);

int user_info(const char *user, uid_t *uid, gid_t *gid);

#endif
