#include "sql.h"

#include <stdio.h>

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
			"maintime   REAL, "
			"increment  REAL, "
			"alpha      REAL, "
			"beta       REAL, "
			"elo0       REAL, "
			"elo1       REAL, "
			"eloe       REAL, "
			"queuetime  INTEGER, "
			"starttime  INTEGER, "
			"donetime   INTEGER, "
			"elo        REAL, "
			"pm         REAL, "
			"result     INTEGER, "
			"llh        REAL, "
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

#if 0
int queue_tests(sqlite3 *db, struct fds *fds) {
	int r;
	sqlite3_stmt *stmt;
	for (int i = 0; i < fds->fd_count; i++) {
		struct connection *con = &fds->cons[i];
		if (con->status != STATUSNODEWAIT)
			continue;

		r = sqlite3_prepare_v2(db,
				"SELECT id, maintime, increment, alpha, beta, "
				"elo0, elo1, eloerror, testtype, patch, branch, commithash "
				"FROM test WHERE status = ? ORDER BY queuetime ASC LIMIT 1;",
				-1, &stmt, NULL);
		if (r != SQLITE_OK) {
			sqlite3_errout(db, __FILE__, __LINE__);
			return 1;
		}
		sqlite3_bind_int(stmt, 1, TESTQUEUE);

		switch (sqlite3_step(stmt)) {
		case SQLITE_ROW:
			break;
		case SQLITE_DONE:
			i = fds->fd_count;
			break;
		default:
			sqlite3_errout(db, __FILE__, __LINE__);
			return 1;
		}

		con->status = STATUSNODERUN;

		con->id = sqlite3_column_int(stmt, 0);
		con->maintime = sqlite3_column_int(stmt, 1);
		con->increment = sqlite3_column_int(stmt, 2);
		con->alpha = sqlite3_column_double(stmt, 3);
		con->beta = sqlite3_column_double(stmt, 4);
		con->elo0 = sqlite3_column_double(stmt, 5);
		con->elo1 = sqlite3_column_double(stmt, 6);
		con->eloerror = sqlite3_column_double(stmt, 7);
		con->testtype = sqlite3_column_int(stmt, 8);

		con->patch = (const char *)sqlite3_column_text(stmt, 9);
		con->branch = (const char *)sqlite3_column_text(stmt, 10);
		con->commit = (const char *)sqlite3_column_text(stmt, 11);
	}

	return 0;
}
#endif
