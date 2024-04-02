#ifndef SETUP_H
#define SETUP_H

#include <stdint.h>

#include <openssl/ssl.h>

void kill_parent(void);

void setup(SSL *ssl, int type, uint32_t games, int nthreads, double maintime,
		double increment, double alpha, double beta, double elo0,
		double elo1, double eloe, int adjudicate,
		const char *branch, const char *commit);

#endif
