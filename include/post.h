#ifndef POST_H
#define POST_H

#include "http.h"

void test_new(int fd, const struct http *http, int id);

extern char backupdir[4096];

void backup_database(int fd, const struct http *http, int id);

#endif
