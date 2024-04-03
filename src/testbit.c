#include <locale.h>
#include <stdio.h>
#include <signal.h>

#include <ncurses.h>

#include "ssl.h"
#include "con.h"
#include "tui.h"

int main(void) {
	setlocale(LC_ALL, "");
	signal(SIGPIPE, SIG_IGN);

	int sockfd;
	SSL *ssl;
	SSL_CTX *ctx;
	if (!(ctx = ssl_ctx_client()))
		return 1;

	if ((sockfd = get_socket()) < 0) {
		printf("error: failed to connect\n");
		return 2;
	}

	if (!(ssl = ssl_ssl_client(ctx, sockfd)))
		return 3;

	if (sendf(ssl, "c", TYPECLIENT))
		return 4;

	tuiloop(ssl);

	ssl_close(ssl, 0);
	SSL_CTX_free(ctx);

	return 0;
}
