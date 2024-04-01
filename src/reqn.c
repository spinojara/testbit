#include "reqn.h"

#include "req.h"
#include "sql.h"

int handle_update(struct connection *con, sqlite3 *db) {
	if (sendf(con->ssl, "c", con->status == STATUSCANCEL ? 1 : 0))
		return 1;
	if (con->status == STATUSCANCEL)
		return 0;
	if (con->status != STATUSRUN)
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

	sqlite3_stmt *stmt;
	sqlite3_prepare_v2(db,
			"UPDATE test SET t0 = ?, t1 = ?, t2 = ?, "
			"p0 = ?, p1 = ?, p2 = ?, p3 = ?, p4 = ?, "
			"llr = ?, elo = ?, pm = ? WHERE id = ?;",
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
	printf("test %ld done\n", con->id);
	char status = con->status;
	con->status = STATUSWAIT;
	if (status == STATUSCANCEL)
		return 0;

	if (recvf(con->ssl, "c", &status))
		return 1;

	sqlite3_stmt *stmt;
	sqlite3_prepare_v2(db,
			"UPDATE test SET status = ?, donetime = unixepoch() WHERE id = ?;",
			-1, &stmt, NULL);

	sqlite3_bind_int(stmt, 1, status);
	sqlite3_bind_int64(stmt, 2, con->id);

	sqlite3_step(stmt);
	sqlite3_finalize(stmt);

	return 0;
}

int handle_node_request(struct connection *con, sqlite3 *db, const char password[128]) {
	char request;
	if (recvf(con->ssl, "c", &request))
		return 1;
	if ((!con->privileged || con->status == STATUSWAIT) && request != REQUESTPRIVILEGE)
		return 1;

	switch (request) {
	case REQUESTPRIVILEGE:
		return handle_password(con, password);
	case REQUESTNODEUPDATE:
		return handle_update(con, db);
	case REQUESTNODEDONE:
		return handle_done(con, db);
	case REQUESTNODESTART:
		return start_test(db, con->id);
	default:
		return 1;
	}
}
