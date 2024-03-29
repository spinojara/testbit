#define _POSIX_C_SOURCE 200112L
#include "ssl.h"

#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>

#define PORT "2718"
#define HOSTNAME "test.bitbitchess.org"

int get_socket(void) {
	int sockfd;
	struct addrinfo hints = { 0 }, *ai, *p;

	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	if (getaddrinfo(HOSTNAME, PORT, &hints, &ai))
		return -1;

	for (p = ai; p; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) < 0)
			continue;

		if (connect(sockfd, p->ai_addr, p->ai_addrlen)) {
			close(sockfd);
			continue;
		}
		break;
	}

	freeaddrinfo(ai);

	if (!p)
		return -1;

	return sockfd;
}

int get_listener_socket(void) {
	int listener, yes = 1;

	struct addrinfo hints = { 0 }, *ai, *p;
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	if (getaddrinfo(NULL, PORT, &hints, &ai))
		return -1;

	for (p = ai; p; p = p->ai_next) {
		if ((listener = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) < 0)
			continue;
		setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

		if (bind(listener, p->ai_addr, p->ai_addrlen) < 0) {
			close(listener);
			continue;
		}

		break;
	}

	freeaddrinfo(ai);

	if (!p)
		return -1;

	if (listen(listener, 10) == -1)
		return -1;

	return listener;
}

void ssl_close(SSL *ssl, int fast) {
	SSL_shutdown(ssl);
	if (!fast)
		SSL_shutdown(ssl);
	SSL_free(ssl);
}

SSL_CTX *ssl_ctx_server(void) {
	SSL_CTX *ctx = SSL_CTX_new(TLS_server_method());
	if (!ctx)
		return NULL;

	if (!SSL_CTX_set_min_proto_version(ctx, TLS1_2_VERSION))
		return NULL;

	if (!SSL_CTX_use_certificate_file(ctx, "/var/lib/bitbit/certs/bitbit.crt", SSL_FILETYPE_PEM)) {
		fprintf(stderr, "error: failed to set the certificate\n");
		return NULL;
	}
	if (!SSL_CTX_use_PrivateKey_file(ctx, "/var/lib/bitbit/private/bitbit.key.pem", SSL_FILETYPE_PEM)) {
		fprintf(stderr, "error: failed to set the private key\n");
		return NULL;
	}

	return ctx;
}

SSL_CTX *ssl_ctx_client(void) {
	SSL_CTX *ctx = SSL_CTX_new(TLS_client_method());
	if (!ctx)
		return NULL;

	if (!SSL_CTX_set_min_proto_version(ctx, TLS1_2_VERSION))
		return NULL;

	if (!SSL_CTX_set_default_verify_paths(ctx))
		return NULL;

	SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER, NULL);

	return ctx;
}

SSL *ssl_ssl_client(SSL_CTX *ctx, int fd) {
	SSL *ssl = SSL_new(ctx);
	if (!ssl)
		return NULL;

	BIO *bio = BIO_new(BIO_s_socket());
	if (!bio)
		return NULL;

	BIO_set_fd(bio, fd, BIO_CLOSE);
	SSL_set_bio(ssl, bio, bio);

	if (!SSL_set_tlsext_host_name(ssl, HOSTNAME))
		return NULL;

	if (!SSL_set1_host(ssl, HOSTNAME))
		return NULL;

	if (SSL_connect(ssl) <= 0)
		return NULL;

	return ssl;
}
