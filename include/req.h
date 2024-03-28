#ifndef REQ_H
#define REQ_H

#include "con.h"
#include "reqc.h"
#include "reqn.h"

enum {
	REQUESTPRIVILEGE,
	REQUESTNODEUPDATE,
	REQUESTNODEDONE,
	REQUESTNODESTART,
	REQUESTNEWTEST,
	REQUESTMODTEST,
	REQUESTLOGTEST,
	REQUESTPATCH,
};

enum {
	RESPONSEOK,
	RESPONSEFAIL,
	RESPONSEPERMISSIONDENIED,
};

int handle_password(struct connection *con, const char password[128]);

#endif
