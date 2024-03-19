#ifndef SQL_H
#define SQL_H

#include <poll.h>

#include <sqlite3.h>

#include "con.h"

struct fds {
	struct pollfd *pfds;
	struct connection *cons;

	int fd_count;
	int fd_size;

	int listener;
};

enum {
	TESTQUEUE,
	TESTRUN,
	TESTCANCEL,
	TESTDONE,
	TESTERRBRANCH,
	TESTERRCOMMIT,
	TESTERRPATCH,
	TESTERRMAKE,
	TESTERRRUN,
};

int init_db(sqlite3 **db);

int queue_tests(sqlite3 *db, struct fds *fds);

#endif
