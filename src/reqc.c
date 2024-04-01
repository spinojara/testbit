#include "reqc.h"

#include <math.h>
#include <fcntl.h>
#include <unistd.h>

#include "req.h"
#include "sprt.h"
#include "oldtest.h"
#include "util.h"

static inline int inrange(double x, double a, double b) {
	return a <= x && x <= b;
}

int handle_new_test(struct connection *con, sqlite3 *db) {
	char type;
	double maintime, increment;
	double alpha, beta;
	double elo0, elo1, eloe;
	char branch[128], commit[128];
	int r = 0;
	if ((r = recvf(con->ssl, "cDDDDDDDss",
					&type, &maintime, &increment, &alpha,
					&beta, &elo0, &elo1, &eloe,
					branch, sizeof(branch),
					commit, sizeof(commit))))
		goto error;

	double zero = eps;
	double one = 1.0 - eps;
	if ((r = !inrange(alpha, zero, one) ||
			!inrange(beta, zero, one) ||
			maintime <= 0.1 || increment < 0.0 ||
			(type == TESTTYPEELO && eloe < zero)))
		goto error;

	sqlite3_stmt *stmt;
	sqlite3_prepare_v2(db,
			"INSERT INTO test (type, status, maintime, increment, alpha, beta, "
			"elo0, elo1, eloe, queuetime, elo, pm, branch, commithash) VALUES "
			"(?, ?, ?, ?, ?, ?, ?, ?, ?, unixepoch(), ?, ?, ?, ?) RETURNING id;",
			-1, &stmt, NULL);
	sqlite3_bind_int(stmt, 1, type);
	sqlite3_bind_int(stmt, 2, TESTQUEUE);
	sqlite3_bind_double(stmt, 3, maintime);
	sqlite3_bind_double(stmt, 4, increment);
	sqlite3_bind_double(stmt, 5, alpha);
	sqlite3_bind_double(stmt, 6, beta);
	sqlite3_bind_double(stmt, 7, elo0);
	sqlite3_bind_double(stmt, 8, elo1);
	sqlite3_bind_double(stmt, 9, eloe);

	sqlite3_bind_double(stmt, 10, 0.0 / 0.0);
	sqlite3_bind_double(stmt, 11, 0.0 / 0.0);
	sqlite3_bind_text(stmt, 12, branch, -1, NULL);
	sqlite3_bind_text(stmt, 13, commit, -1, NULL);
	sqlite3_step(stmt);
	con->id = sqlite3_column_int(stmt, 0);
	sqlite3_step(stmt);
	sqlite3_finalize(stmt);

	char path[4096];
	sprintf(path, "/var/lib/bitbit/patch/%ld", con->id);
	int fd;
	if ((fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0444)) < 0) {
		fprintf(stderr, "Failed to open file %s\n", path);
		exit(9);
	}
	if ((r = recvf(con->ssl, "f", fd, -1))) {
		/* Delete the file. */
		sqlite3_prepare_v2(db, "DELETE FROM test WHERE id = ?;", -1, &stmt, NULL);
		sqlite3_bind_int(stmt, 1, con->id);
		sqlite3_step(stmt);
		sqlite3_finalize(stmt);
		close(fd);
		unlink(path);
		goto error;
	}
	close(fd);
error:
	sendf(con->ssl, "c", r ? RESPONSEFAIL : RESPONSEOK);
	return r;
}

int handle_log_test(struct connection *con, sqlite3 *db) {
	int64_t id;
	if (recvf(con->ssl, "q", &id))
		return 1;

	sqlite3_stmt *stmt;
	sqlite3_prepare_v2(db,
			"SELECT type, status, maintime, increment, alpha, beta, llr, "
			"elo0, elo1, eloe, elo, pm, branch, commithash, queuetime, "
			"starttime, donetime, t0, t1, t2, p0, p1, p2, p3, p4 "
			"FROM test WHERE id = ?;",
			-1, &stmt, NULL);
	sqlite3_bind_int64(stmt, 1, id);
	if (sqlite3_step(stmt) == SQLITE_ROW) {
		sendf(con->ssl, "c", 0);
		char type = sqlite3_column_int(stmt, 0);
		char status = sqlite3_column_int(stmt, 1);
		double maintime = sqlite3_column_double(stmt, 2);
		double increment = sqlite3_column_double(stmt, 3);
		double alpha = sqlite3_column_double(stmt, 4);
		double beta = sqlite3_column_double(stmt, 5);
		double llr = sqlite3_column_double(stmt, 6);
		double elo0 = sqlite3_column_double(stmt, 7);
		double elo1 = sqlite3_column_double(stmt, 8);
		double eloe = sqlite3_column_double(stmt, 9);
		double elo = sqlite3_column_double(stmt, 10);
		double pm = sqlite3_column_double(stmt, 11);
		const char *branch = (const char *)sqlite3_column_text(stmt, 12);
		const char *commit = (const char *)sqlite3_column_text(stmt, 13);
		int64_t qtime = sqlite3_column_int64(stmt, 14);
		int64_t stime = sqlite3_column_int64(stmt, 15);
		int64_t dtime = sqlite3_column_int64(stmt, 16);
		uint32_t t0 = sqlite3_column_int(stmt, 17);
		uint32_t t1 = sqlite3_column_int(stmt, 18);
		uint32_t t2 = sqlite3_column_int(stmt, 19);
		uint32_t p0 = sqlite3_column_int(stmt, 20);
		uint32_t p1 = sqlite3_column_int(stmt, 21);
		uint32_t p2 = sqlite3_column_int(stmt, 22);
		uint32_t p3 = sqlite3_column_int(stmt, 23);
		uint32_t p4 = sqlite3_column_int(stmt, 24);
		sendf(con->ssl, "ccDDDDDDDDDDssqqqLLLLLLLL",
				type, status, maintime, increment, alpha, beta, llr,
				elo0, elo1, eloe, elo, pm, branch, commit, qtime, stime, dtime,
				t0, t1, t2, p0, p1, p2, p3, p4);

	}
	else {
		sendf(con->ssl, "c", 1);
	}
	
	sqlite3_finalize(stmt);

	return 0;
}

int handle_log_tests(struct connection *con, sqlite3 *db) {
	char request;
	if (recvf(con->ssl, "c", &request))
		return 1;

	int filter[5];
	sqlite3_stmt *stmt;
	switch (request) {
	case OLDTESTACTIVE:
		filter[0] = filter[1] = filter[2] = filter[3] = TESTQUEUE;
		filter[4] = TESTRUN;
		break;
	case OLDTESTDONE:
		filter[0] = filter[1] = TESTINCONCLUSIVE;
		filter[2] = TESTH0;
		filter[3] = TESTH1;
		filter[4] = TESTELO;
		break;
	case OLDTESTCANCELLED:
		filter[0] = filter[1] = filter[2] = filter[3] = filter[4] = TESTCANCEL;
		break;
	case OLDTESTFAILED:
		filter[0] = TESTERRBRANCH;
		filter[1] = TESTERRCOMMIT;
		filter[2] = TESTERRPATCH;
		filter[3] = TESTERRMAKE;
		filter[4] = TESTERRRUN;
		break;
	case OLDTESTSINGLE:
		return handle_log_test(con, db);
	default:
		return 1;
	}

	int64_t min, max;

	if (recvf(con->ssl, "qq", &min, &max))
		return 1;

	sqlite3_prepare_v2(db,
			"SELECT id, type, status, maintime, increment, alpha, beta, llr, "
			"elo0, elo1, eloe, elo, pm, branch, commithash, queuetime, "
			"starttime, donetime, t0, t1, t2, p0, p1, p2, p3, p4 "
			"FROM test WHERE status = ? or status = ? "
			"or status = ? or status = ? or status = ? "
			"ORDER BY queuetime ASC;",
			-1, &stmt, NULL);
	sqlite3_bind_int(stmt, 1, filter[0]);
	sqlite3_bind_int(stmt, 2, filter[1]);
	sqlite3_bind_int(stmt, 3, filter[2]);
	sqlite3_bind_int(stmt, 4, filter[3]);
	sqlite3_bind_int(stmt, 5, filter[4]);

	int error = 0;

	for (int i = 0; i < max; i++) {
		if (sqlite3_step(stmt) != SQLITE_ROW)
			break;

		if (i < min)
			continue;

		int64_t id = sqlite3_column_int64(stmt, 0);
		char type = sqlite3_column_int(stmt, 1);
		char status = sqlite3_column_int(stmt, 2);
		double maintime = sqlite3_column_double(stmt, 3);
		double increment = sqlite3_column_double(stmt, 4);
		double alpha = sqlite3_column_double(stmt, 5);
		double beta = sqlite3_column_double(stmt, 6);
		double llr = sqlite3_column_double(stmt, 7);
		double elo0 = sqlite3_column_double(stmt, 8);
		double elo1 = sqlite3_column_double(stmt, 9);
		double eloe = sqlite3_column_double(stmt, 10);
		double elo = sqlite3_column_double(stmt, 11);
		double pm = sqlite3_column_double(stmt, 12);
		const char *branch = (const char *)sqlite3_column_text(stmt, 13);
		const char *commit = (const char *)sqlite3_column_text(stmt, 14);
		int64_t qtime = sqlite3_column_int64(stmt, 15);
		int64_t stime = sqlite3_column_int64(stmt, 16);
		int64_t dtime = sqlite3_column_int64(stmt, 17);
		uint32_t t0 = sqlite3_column_int(stmt, 18);
		uint32_t t1 = sqlite3_column_int(stmt, 19);
		uint32_t t2 = sqlite3_column_int(stmt, 20);
		uint32_t p0 = sqlite3_column_int(stmt, 21);
		uint32_t p1 = sqlite3_column_int(stmt, 22);
		uint32_t p2 = sqlite3_column_int(stmt, 23);
		uint32_t p3 = sqlite3_column_int(stmt, 24);
		uint32_t p4 = sqlite3_column_int(stmt, 25);

		/* Send information that a row was found. */
		if (sendf(con->ssl, "c", 0)) {
			error = 1;
			break;
		}
		/* Send the data. */
		if (sendf(con->ssl, "qccDDDDDDDDDDssqqqLLLLLLLL",
				id, type, status, maintime, increment, alpha, beta, llr,
				elo0, elo1, eloe, elo, pm, branch, commit, qtime, stime, dtime,
				t0, t1, t2, p0, p1, p2, p3, p4)) {
			error = 1;
			break;
		}
	}

	sqlite3_finalize(stmt);

	return error || sendf(con->ssl, "c", 1);
}

int handle_mod_test(struct connection *con, sqlite3 *db, struct fds *fds) {
	int64_t id;
	char status;
	if (recvf(con->ssl, "qc", &id, &status))
		return 1;

	sqlite3_stmt *stmt;
	switch (status) {
	case TESTQUEUE:
		sqlite3_prepare_v2(db,
				"UPDATE test SET status = ?, queuetime = unixepoch(), "
				"starttime = 0 WHERE id = ? AND status != ?;",
				-1, &stmt, NULL);
		sqlite3_bind_int(stmt, 1, TESTQUEUE);
		sqlite3_bind_int64(stmt, 2, id);
		sqlite3_bind_int(stmt, 3, TESTRUN);
		break;
	case TESTCANCEL:
		sqlite3_prepare_v2(db,
				"UPDATE test SET status = ?, donetime = unixepoch() "
				"WHERE id = ? AND (status = ? OR status = ?);",
				-1, &stmt, NULL);
		sqlite3_bind_int(stmt, 1, TESTCANCEL);
		sqlite3_bind_int64(stmt, 2, id);
		sqlite3_bind_int(stmt, 3, TESTQUEUE);
		sqlite3_bind_int(stmt, 4, TESTRUN);
		for (int i = 0; i < fds->fd_count; i++)
			if (fds->cons[i].type == TYPENODE &&
					fds->cons[i].status == STATUSRUN &&
					fds->cons[i].id == id)
				fds->cons[i].status = STATUSCANCEL;
		break;
	default:
		return 1;
	}
	sqlite3_step(stmt);
	sqlite3_finalize(stmt);

	return 0;
}

int handle_patch(struct connection *con) {
	int64_t id;
	if (recvf(con->ssl, "q", &id))
		return 1;

	char path[4096];
	sprintf(path, "/var/lib/bitbit/patch/%ld", id);
	int fd;
	if ((fd = open(path, O_RDONLY, 0)) < 0) {
		sendf(con->ssl, "c", 1);
		return 0;
	}
	int error = 0;
	if (sendf(con->ssl, "cf", 0, fd))
		error = 1;

	close(fd);

	return error;
}

int handle_client_request(struct connection *con, sqlite3 *db, const char password[128], struct fds *fds) {
	char request;
	if (recvf(con->ssl, "c", &request))
		return 1;

	switch (request) {
	case REQUESTPRIVILEGE:
		return handle_password(con, password);
	case REQUESTNEWTEST:
		return con->privileged ? handle_new_test(con, db) : 1;
	case REQUESTMODTEST:
		return con->privileged ? handle_mod_test(con, db, fds) : 1;
	case REQUESTLOGTEST:
		return handle_log_tests(con, db);
	case REQUESTPATCH:
		return handle_patch(con);
	default:
		return 1;
	}
}
