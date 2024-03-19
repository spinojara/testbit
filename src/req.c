#include "req.h"

int handle_password(struct connection *con, const char password[128]) {
	char *buf = NULL;
	if (recvf(con->ssl, "s", &buf) || strcmp(password, buf)) {
		sendf(con->ssl, "c", RESPONSEPERMISSIONDENIED);
		free(buf);
		return 1;
	}
	free(buf);
	return 0;
}
