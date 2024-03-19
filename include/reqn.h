#ifndef REQN_H
#define REQN_H

#include <sqlite3.h>

#include "con.h"

int handle_node_request(struct connection *con, sqlite3 *db, const char password[128]);

#endif
