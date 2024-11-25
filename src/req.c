#define _DEFAULT_SOURCE
#include "req.h"

#include <unistd.h>

int handle_password(struct connection *con, const char *password) {
	char buf[4096];
	if (recvf(con->ssl, "s", buf, sizeof(buf)))
		return 1;
	char *hashed = NULL;
	con->privileged = (hashed = crypt(buf, password)) && !strcmp(hashed, password);
	if (sendf(con->ssl, "c", con->privileged ? RESPONSEOK : RESPONSEPERMISSIONDENIED))
		return 1;
	return 0;
}
