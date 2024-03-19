#include "reqc.h"

#include <math.h>
#include <fcntl.h>
#include <unistd.h>

#include "req.h"
#include "sprt.h"

const double eps = 1e-6;

static inline int inrange(double x, double a, double b) {
	return a < x && x < b;
}

int handle_new_test(struct connection *con, sqlite3 *db) {
	con->test.branch = con->test.commit = NULL;
	int r = 0;
	if ((r = recvf(con->ssl, "cLLDDDDDss", &con->test.type,
				          &con->test.maintime,
					  &con->test.increment,
					  &con->test.alpha,
					  &con->test.beta,
					  &con->test.elo0,
					  &con->test.elo1,
					  &con->test.eloe,
					  &con->test.branch,
					  &con->test.commit)))
		goto error;
	double zero = eps;
	double one = 1.0 - eps;
	if ((r = !inrange(con->test.alpha, zero, one) ||
			!inrange(con->test.beta, zero, one) ||
			con->test.eloe < zero))
		goto error;

	sqlite3_stmt *stmt;
	sqlite3_prepare_v2(db,
			"INSERT INTO test (type, status, maintime, increment, alpha, beta, "
			"elo0, elo1, eloe, queuetime, elo, pm, branch, commithash) VALUES "
			"(?, ?, ?, ?, ?, ?, ?, ?, ?, unixepoch(), ?, ?, ?, ?) RETURNING id;",
			-1, &stmt, NULL);
	sqlite3_bind_int(stmt, 1, con->test.type);
	sqlite3_bind_int(stmt, 2, TESTQUEUE);
	sqlite3_bind_int(stmt, 3, con->test.maintime);
	sqlite3_bind_int(stmt, 4, con->test.increment);
	sqlite3_bind_double(stmt, 5, con->test.alpha);
	sqlite3_bind_double(stmt, 6, con->test.beta);
	sqlite3_bind_double(stmt, 7, con->test.elo0);
	sqlite3_bind_double(stmt, 8, con->test.elo1);
	sqlite3_bind_double(stmt, 9, con->test.eloe);

	sqlite3_bind_double(stmt, 11, 0.0 / 0.0);
	sqlite3_bind_double(stmt, 12, 0.0 / 0.0);
	sqlite3_bind_text(stmt, 13, con->test.branch, -1, NULL);
	sqlite3_bind_text(stmt, 14, con->test.commit, -1, NULL);
	sqlite3_step(stmt);
	con->test.id = sqlite3_column_int(stmt, 0);
	sqlite3_step(stmt);
	sqlite3_finalize(stmt);

	char path[4096];
	sprintf(path, "/var/lib/bitbit/patch/%ld.patch", con->test.id);
	int fd;
	if ((fd = open(path, O_WRONLY | O_CREAT, 0644) < 0)) {
		fprintf(stderr, "Failed to open file %s\n", path);
		exit(9);
	}
	if (recvfile(con->ssl, fd)) {
		/* Delete the file. */
		sqlite3_prepare_v2(db, "DELETE FROM test WHERE id = ?;", -1, &stmt, NULL);
		sqlite3_bind_int(stmt, 1, con->test.id);
		sqlite3_step(stmt);
		sqlite3_finalize(stmt);
		close(fd);
		unlink(path);
		goto error;
	}
	close(fd);
error:
	free(con->test.branch);
	free(con->test.commit);
	return r;
}

int handle_mod_test(struct connection *con, sqlite3 *db) {
	(void)con;
	(void)db;
	return 0;
}

int handle_client_request(struct connection *con, sqlite3 *db, const char password[128]) {
	char request;
	if (recvf(con->ssl, "c", &request))
		return 1;

	switch (request) {
	case REQUESTPRIVILEGE:
		return handle_password(con, password);
	case REQUESTNEWTEST:
		return handle_new_test(con, db);
	case REQUESTMODTEST:
		return handle_mod_test(con, db);
	}
	return 0;
}
