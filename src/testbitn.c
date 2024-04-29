#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>
#include <sys/stat.h>

#include <openssl/ssl.h>

#include "node.h"
#include "con.h"
#include "ssl.h"

int main(int argc, char **argv) {
	signal(SIGPIPE, SIG_IGN);

	char *syzygy = NULL;
	char *endptr;
	int sockfd, cpus;
	SSL *ssl;
	SSL_CTX *ctx;
	
	errno = 0;
	if (argc < 2 || argc > 3 || (cpus = strtol(argv[1], &endptr, 10)) <= 0 || *endptr != '\0' || errno) {
		fprintf(stderr, "usage: testbitn cpus [syzygy]\n");
		return 5;
	}


	if (argc == 3) {
		syzygy = argv[2];
		struct stat sb;
		if (stat(syzygy, &sb) || !S_ISDIR(sb.st_mode)) {
			fprintf(stderr, "error: failed to open directory '%s'\n", syzygy);
			return 6;
		}
	}
	else
		fprintf(stderr, "warning: no syzygy tablebases specifed\n");

	if (!(ctx = ssl_ctx_client()))
		return 1;

	if ((sockfd = get_socket()) < 0) {
		printf("error: failed to connect\n");
		return 2;
	}

	if (!(ssl = ssl_ssl_client(ctx, sockfd)))
		return 3;

	sendf(ssl, "c", TYPENODE);

	nodeloop(ssl, cpus, syzygy);

	ssl_close(ssl, 0);
	SSL_CTX_free(ctx);

	return 0;
}
