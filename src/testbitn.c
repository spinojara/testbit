#define _POSIX_C_SOURCE 200809L
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>
#include <sys/stat.h>
#include <getopt.h>
#include <fcntl.h>

#include <openssl/ssl.h>

#include "node.h"
#include "con.h"
#include "ssl.h"

int daemon_mode = 0;

int main(int argc, char **argv) {
	char *syzygy = NULL;
	char *hostname = NULL;
	int sockfd, cpus = 0;
	SSL *ssl;
	SSL_CTX *ctx;

	static struct option opts[] = {
		{ "syzygy",   required_argument, NULL, 'z' },
		{ "daemon",   no_argument,       NULL, 'd' },
		{ "stdin",    required_argument, NULL, 's' },
		{ "hostname", required_argument, NULL, 'h' },
		{ 0,          0,                 0,    0,  },
	};

	char *endptr;
	int c, option_index = 0;
	int error = 0;

	while ((c = getopt_long(argc, argv, "z:ds:h:", opts, &option_index)) != -1) {
		switch (c) {
		case 'z':
			syzygy = optarg;
			struct stat sb;
			if (stat(syzygy, &sb) || !S_ISDIR(sb.st_mode)) {
				fprintf(stderr, "error: failed to open directory '%s'\n", syzygy);
				error = 1;
			}
			break;
		case 'd':
			daemon_mode = 1;
			break;
		case 's':;
			int fd = open(optarg, O_RDONLY);
			if (fd < 0) {
				fprintf(stderr, "error: failed to open file '%s'\n", optarg);
				error = 1;
			}
			if (dup2(fd, STDIN_FILENO) == -1) {
				fprintf(stderr, "error: dup2\n");
				error = 1;
			}
			break;
		case 'h':
			hostname = optarg;
			break;
		default:
			error = 1;
			break;
		}
	}
	if (error)
		return -1;

	if (daemon_mode) {
		FILE *f = fopen("/etc/testbit.conf", "r");
		char buf[4096];
		if (!f || !fgets(buf, sizeof(buf), f)) {
			fprintf(stderr, "error: failed to open file '/etc/testbit.conf'\n");
			return -4;
		}

		errno = 0;
		if ((cpus = strtol(buf, &endptr, 10)) <= 0 || (*endptr != '\0' && *endptr != '\n') || errno) {
			fprintf(stderr, "error: cpus: %s\n", argv[optind]);
			return -3;
		}

		printf("cpus: %d\n", cpus);
	}
	else {
		if (optind >= argc) {
			fprintf(stderr, "usage: %s cpus\n", argv[0]);
			return -2;
		}
		errno = 0;
		if ((cpus = strtol(argv[optind], &endptr, 10)) <= 0 || *endptr != '\0' || errno) {
			fprintf(stderr, "error: cpus: %s\n", argv[optind]);
			return -3;
		}
	}

	if (!(ctx = ssl_ctx_client()))
		return 1;

	if ((sockfd = get_socket(hostname)) < 0) {
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
