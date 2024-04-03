#include "req.h"

int handle_password(struct connection *con, const char password[128]) {
	char buf[128];
	if (recvf(con->ssl, "s", buf, sizeof(buf)))
		return 1;
	con->privileged = !strcmp(password, buf);
	if (sendf(con->ssl, "c", con->privileged ? RESPONSEOK : RESPONSEPERMISSIONDENIED))
		return 1;
	return 0;
}
