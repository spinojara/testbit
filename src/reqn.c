#include "reqn.h"
#include "req.h"

int handle_update(struct connection *con, sqlite3 *db) {
	return 0;
}

int handle_node_request(struct connection *con, sqlite3 *db, const char password[128]) {
	char request;
	if (recvf(con->ssl, "c", &request))
		return 1;
	if ((!con->privileged || con->test.status == STATUSWAIT) && request != REQUESTPRIVILEGE)
		return 1;

	switch (request) {
	case REQUESTPRIVILEGE:
		return handle_password(con, password);
	case REQUESTNODEUPDATE:
		return handle_update(con, db);
	default:
		return 1;
	}
}
