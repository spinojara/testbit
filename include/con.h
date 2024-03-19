#ifndef CON_H
#define CON_H

#include <openssl/ssl.h>

enum {
	TYPELISTENER,
	TYPECLIENT,
	TYPENODE,
};

enum {
	STATUSWAIT,
	STATUSRUN,
	STATUSCANCEL,
};

struct test {
	char status;
	int64_t id;

	char type;

	uint32_t maintime;
	uint32_t increment;
	double alpha;
	double beta;
	double elo0;
	double elo1;
	double eloe;

	char *branch;
	char *commit;
};

struct connection {
	char type;
	char privileged;
	
	struct test test;

	SSL *ssl;
};

int sendexact(SSL *ssl, const char *buf, size_t len);
int recvexact(SSL *ssl, char *buf, size_t len);
int sendf(SSL *ssl, const char *fmt, ...);
int recvf(SSL *ssl, const char *fmt, ...);
int sendfile(SSL *ssl, int fd);
int recvfile(SSL *ssl, int fd);

int read_secret(char *secret, int size);
int create_secret(char *secret, int size);

#endif
