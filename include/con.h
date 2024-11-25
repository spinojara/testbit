#ifndef CON_H
#define CON_H

#include <arpa/inet.h>

#include <openssl/ssl.h>

enum {
	TYPELISTENER,
	TYPESTDIN,
	TYPECLIENT,
	TYPENODE,
};

enum {
	STATUSWAIT,
	STATUSRUN,
	STATUSCANCEL,
};

struct connection {
	char type;
	char status;
	char privileged;
	
	int64_t id;

	char name[INET6_ADDRSTRLEN];

	SSL *ssl;
};

int sendexact(SSL *ssl, const char *buf, size_t len);
int recvexact(SSL *ssl, char *buf, size_t len);
int sendf(SSL *ssl, const char *fmt, ...);
int recvf(SSL *ssl, const char *fmt, ...);

int read_secret(char *secret, int size, int hide);

#endif
