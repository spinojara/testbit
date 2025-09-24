#include "reqn.h"

#include <fcntl.h>
#include <unistd.h>

#include "req.h"
#include "sql.h"

int handle_update(struct connection *con, sqlite3 *db) {
	if (sendf(con->ssl, "c", con->status == STATUSCANCEL ? 1 : 0))
		return 1;

	int32_t tri[3];
	int32_t penta[5];
	double llr;
	double elo, pm;

	if (recvf(con->ssl, "llllllllDDD",
				&tri[0], &tri[1], &tri[2],
				&penta[0], &penta[1], &penta[2], &penta[3], &penta[4],
				&llr, &elo, &pm))
		return 1;

	if (con->status == STATUSCANCEL)
		return 0;

	sqlite3_stmt *stmt;
	sqlite3_prepare_v2(db,
			"UPDATE test SET t0 = ?, t1 = ?, t2 = ?, "
			"p0 = ?, p1 = ?, p2 = ?, p3 = ?, p4 = ?, "
			"llr = ?, elo = ?, pm = ?, donetime = unixepoch() "
			"WHERE id = ?;",
			-1, &stmt, NULL);

	sqlite3_bind_double(stmt, 1, tri[0]);
	sqlite3_bind_double(stmt, 2, tri[1]);
	sqlite3_bind_double(stmt, 3, tri[2]);
	sqlite3_bind_double(stmt, 4, penta[0]);
	sqlite3_bind_double(stmt, 5, penta[1]);
	sqlite3_bind_double(stmt, 6, penta[2]);
	sqlite3_bind_double(stmt, 7, penta[3]);
	sqlite3_bind_double(stmt, 8, penta[4]);
	sqlite3_bind_double(stmt, 9, llr);
	sqlite3_bind_double(stmt, 10, elo);
	sqlite3_bind_double(stmt, 11, pm);
	sqlite3_bind_int64(stmt, 12, con->id);

	sqlite3_step(stmt);
	sqlite3_finalize(stmt);

	return 0;
}

int handle_done(struct connection *con, sqlite3 *db) {
	char status = con->status;
	char result;
	char path[BUFSIZ];
	sprintf(path, "/var/lib/bitbit/pgn/%ld", con->id);
	int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0444);
	if (fd < 0) {
		fprintf(stderr, "Failed to open file %s\n", path);
		exit(23);
	}
	if (recvf(con->ssl, "c", &result) || result < TESTERRBRANCH || result > TESTELO) {
		close(fd);
		return 1;
	}
	if (TESTERRRUN <= result && result <= TESTELO) {
		if (recvf(con->ssl, "f", fd, -1)) {
			close(fd);
			return 1;
		}
		/* In the case of a runtime error, get the log file and current tt. */
		if (result == TESTERRRUN) {
			char logfile[BUFSIZ], ttfile[BUFSIZ];
			sprintf(logfile, "/var/lib/bitbit/log/%ld", con->id);
			sprintf(ttfile, "/var/lib/bitbit/tt/%ld", con->id);
			int logfd = open(logfile, O_WRONLY | O_CREAT | O_TRUNC, 0444);
			if (logfd < 0) {
				fprintf(stderr, "Failed to open file %s\n", logfile);
				exit(24);
			}
			int ttfd = open(ttfile, O_WRONLY | O_CREAT | O_TRUNC, 0444);
			if (ttfd < 0) {
				fprintf(stderr, "Failed to open file %s\n", ttfile);
				exit(25);
			}
			if (recvf(con->ssl, "ff", logfd, -1, ttfd, -1)) {
				close(logfd);
				close(ttfd);
				return 1;
			}
			close(logfd);
			close(ttfd);
		}
	}
	close(fd);

	con->status = STATUSWAIT;
	if (status == STATUSCANCEL)
		return 0;

	sqlite3_stmt *stmt;
	sqlite3_prepare_v2(db,
			"UPDATE test SET status = ?, donetime = unixepoch() WHERE id = ?;",
			-1, &stmt, NULL);

	sqlite3_bind_int(stmt, 1, result);
	sqlite3_bind_int64(stmt, 2, con->id);

	sqlite3_step(stmt);
	sqlite3_finalize(stmt);

	return 0;
}

int handle_start(struct connection *con, sqlite3 *db) {
	if (con->status == STATUSCANCEL)
		return 0;

	sqlite3_stmt *stmt;
	sqlite3_prepare_v2(db,
			"UPDATE test SET starttime = unixepoch() WHERE id = ?;",
			-1, &stmt, NULL);

	sqlite3_bind_int64(stmt, 1, con->id);

	sqlite3_step(stmt);
	sqlite3_finalize(stmt);
	return 0;
}

int handle_node_request(struct connection *con, sqlite3 *db, const char *password) {
	char request;
	if (recvf(con->ssl, "c", &request))
		return 1;
	if ((!con->privileged || con->status == STATUSWAIT) && request != REQUESTPRIVILEGE)
		return 1;

	printf("New node request of type %d\n", request);

	switch (request) {
	case REQUESTPRIVILEGE:
		return handle_password(con, password);
	case REQUESTNODEUPDATE:
		return handle_update(con, db);
	case REQUESTNODEDONE:
		return handle_done(con, db);
	case REQUESTNODESTART:
		return handle_start(con, db);
	default:
		return 1;
	}
}
