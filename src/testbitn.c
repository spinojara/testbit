#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>

#include <openssl/ssl.h>

#include "node.h"
#include "con.h"
#include "ssl.h"

int main(int argc, char **argv) {
	signal(SIGPIPE, SIG_IGN);

	char *endptr;
	int sockfd, nthreads;
	SSL *ssl;
	SSL_CTX *ctx;
	
	errno = 0;
	if (argc != 2 || (nthreads = strtol(argv[1], &endptr, 10)) <= 0 || *endptr != '\0' || errno) {
		fprintf(stderr, "usage: testbitn nthreads\n");
		return 5;
	}

	if (!(ctx = ssl_ctx_client()))
		return 1;

	if ((sockfd = get_socket()) < 0) {
		printf("error: failed to connect\n");
		return 2;
	}

	if (!(ssl = ssl_ssl_client(ctx, sockfd)))
		return 3;

	sendf(ssl, "c", TYPENODE);

	nodeloop(ssl, nthreads);

	ssl_close(ssl, 0);
	SSL_CTX_free(ctx);

	return 0;
}
