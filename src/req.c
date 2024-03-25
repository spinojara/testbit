#include "req.h"

int handle_password(struct connection *con, const char password[128]) {
	char *buf = NULL;
	if (recvf(con->ssl, "s", &buf)) {
		free(buf);
		return 1;
	}
	con->privileged = !strcmp(password, buf);
	sendf(con->ssl, "c", con->privileged ? RESPONSEOK : RESPONSEPERMISSIONDENIED);
	free(buf);
	return 0;
}
