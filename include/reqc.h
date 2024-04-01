#ifndef REQC_H
#define REQC_H

#include <sqlite3.h>

#include "con.h"
#include "sql.h"

int handle_client_request(struct connection *con, sqlite3 *db, const char password[128], struct fds *fds);

#endif
