#include "sql.h"

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

void sqlite3_errout(sqlite3 *db, const char *file, int line) {
	fprintf(stderr, "%s:%d: error: %s\n", file, line, sqlite3_errmsg(db));
}

int init_db(sqlite3 **db) {
	if (sqlite3_open("/var/lib/bitbit/bitbit.db", db) != SQLITE_OK)
		return 1;

	int r = sqlite3_exec(*db,
			"CREATE TABLE IF NOT EXISTS test ("
			"id         INTEGER PRIMARY KEY, "
			"type       INTEGER, "
			"status     INTEGER, "
			"tc         TEXT, "
			"alpha      REAL, "
			"beta       REAL, "
			"elo0       REAL, "
			"elo1       REAL, "
			"eloe       REAL, "
			"adjudicate INTEGER, "
			"queuetime  INTEGER, "
			"starttime  INTEGER, "
			"donetime   INTEGER, "
			"elo        REAL, "
			"pm         REAL, "
			"llr        REAL, "
			"t0         INTEGER, "
			"t1         INTEGER, "
			"t2         INTEGER, "
			"p0         INTEGER, "
			"p1         INTEGER, "
			"p2         INTEGER, "
			"p3         INTEGER, "
			"p4         INTEGER, "
			"branch     TEXT, "
			"commithash TEXT"
			");",
			NULL, NULL, NULL);

	if (r != SQLITE_OK) {
		sqlite3_errout(*db, __FILE__, __LINE__);
		return 1;
	}

	/* Requeue all old tests. */
	sqlite3_stmt *stmt;
	r = sqlite3_prepare_v2(*db,
			"UPDATE test SET status = ? WHERE status = ?;",
			-1, &stmt, NULL);
	if (r != SQLITE_OK) {
		sqlite3_errout(*db, __FILE__, __LINE__);
		return 1;
	}
	sqlite3_bind_int(stmt, 1, TESTQUEUE);
	sqlite3_bind_int(stmt, 2, TESTRUN);
	if (sqlite3_step(stmt) != SQLITE_DONE) {
		sqlite3_errout(*db, __FILE__, __LINE__);
		return 1;
	}
	sqlite3_finalize(stmt);

	return 0;
}

int start_tests(sqlite3 *db, struct fds *fds) {
	int r;
	sqlite3_stmt *stmt;
	for (int i = 0; i < fds->fd_count; i++) {
		int error = 0;
		struct connection *con = &fds->cons[i];
		if (con->type != TYPENODE || con->status != STATUSWAIT || !con->privileged)
			continue;

		r = sqlite3_prepare_v2(db,
				"SELECT id, type, tc, alpha, beta, "
				"elo0, elo1, eloe, branch, commithash, adjudicate "
				"FROM test WHERE status = ? ORDER BY queuetime ASC LIMIT 1;",
				-1, &stmt, NULL);
		if (r != SQLITE_OK) {
			sqlite3_errout(db, __FILE__, __LINE__);
			return 1;
		}
		sqlite3_bind_int(stmt, 1, TESTQUEUE);

		r = sqlite3_step(stmt);
		if (r == SQLITE_DONE)
			break;

		con->status = STATUSRUN;

		con->id = sqlite3_column_int64(stmt, 0);
		char type = sqlite3_column_int(stmt, 1);
		const char *tc = (const char *)sqlite3_column_text(stmt, 2);
		double alpha = sqlite3_column_double(stmt, 3);
		double beta = sqlite3_column_double(stmt, 4);
		double elo0 = sqlite3_column_double(stmt, 5);
		double elo1 = sqlite3_column_double(stmt, 6);
		double eloe = sqlite3_column_double(stmt, 7);

		const char *branch = (const char *)sqlite3_column_text(stmt, 8);
		const char *commit = (const char *)sqlite3_column_text(stmt, 9);

		char adjudicate = sqlite3_column_int(stmt, 11);

		char path[4096];
		sprintf(path, "/var/lib/bitbit/patch/%ld", con->id);
		int fd;

		if ((fd = open(path, O_RDONLY, 0)) < 0) {
			fprintf(stderr, "Failed to open %s\n", path);
			exit(10);
		}

		if (sendf(con->ssl, "csDDDDDcssf",
				type, tc,
				alpha, beta, elo0, elo1, eloe,
				adjudicate, branch,
				commit, fd))
			error = 1;

		sqlite3_finalize(stmt);
		close(fd);

		if (error)
			continue;

		sqlite3_prepare_v2(db,
				"UPDATE test SET starttime = unixepoch(), "
				"status = ?, t0 = ?, t1 = ?, t2 = ?, "
				"p0 = ?, p1 = ?, p2 = ?, p3 = ?, p4 = ? "
				"WHERE id = ?;",
				-1, &stmt, NULL);

		sqlite3_bind_int(stmt, 1, TESTRUN);
		sqlite3_bind_int(stmt, 2, 0);
		sqlite3_bind_int(stmt, 3, 0);
		sqlite3_bind_int(stmt, 4, 0);
		sqlite3_bind_int(stmt, 5, 0);
		sqlite3_bind_int(stmt, 6, 0);
		sqlite3_bind_int(stmt, 7, 0);
		sqlite3_bind_int(stmt, 8, 0);
		sqlite3_bind_int(stmt, 9, 0);
		sqlite3_bind_int64(stmt, 10, con->id);

		sqlite3_step(stmt);
		sqlite3_finalize(stmt);
	}

	return 0;
}

void requeue_test(sqlite3 *db, int64_t id) {
	sqlite3_stmt *stmt;
	sqlite3_prepare_v2(db,
			"UPDATE test SET status = ? WHERE id = ?;",
			-1, &stmt, NULL);

	sqlite3_bind_int(stmt, 1, TESTQUEUE);
	sqlite3_bind_int64(stmt, 2, id);

	sqlite3_step(stmt);
	sqlite3_finalize(stmt);
}
