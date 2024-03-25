#include <locale.h>
#include <stdio.h>
#include <signal.h>

#include "ssl.h"
#include "con.h"
#include "tui.h"

int main(int argc, char **argv) {
	setlocale(LC_ALL, "");
	signal(SIGPIPE, SIG_IGN);
	(void)argc;
	(void)argv;

#if 1
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

	sendf(ssl, "c", TYPECLIENT);

#else
	void *ssl = NULL
#endif

	tuiloop(ssl);

#if 1
	ssl_close(ssl);
	SSL_CTX_free(ctx);
#endif

	return 0;
}
