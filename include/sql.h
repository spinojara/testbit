#ifndef SQL_H
#define SQL_H

#include <poll.h>
#include <stdint.h>

#include <sqlite3.h>

#include "con.h"
#include "sprt.h"

struct fds {
	struct pollfd *pfds;
	struct connection *cons;

	int fd_count;
	int fd_size;

	int listener;
};

int init_db(sqlite3 **db);

int start_tests(sqlite3 *db, struct fds *fds);

void requeue_test(sqlite3 *db, int64_t id);

int start_test(sqlite3 *db, int64_t id);

#endif
