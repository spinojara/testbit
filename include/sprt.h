#ifndef SPRT_H
#define SPRT_H

#include <stdint.h>

#include <openssl/ssl.h>

enum {
	TESTQUEUE,
	TESTDONE,
	TESTRUN,
	TESTCANCEL,
	TESTERRBRANCH,
	TESTERRCOMMIT,
	TESTERRPATCH,
	TESTERRMAKE,
	TESTERRRUN,
};

enum {
	TESTTYPESPRT,
	TESTTYPEELO,
};

enum {
	HINCONCLUSIVE,
	H0,
	H1,
	HERROR,
	HCANCEL,
};

void setup_sprt(SSL *ssl, int type, uint32_t games, int nthreads, double maintime,
		double increment, double alpha, double beta, double elo0,
		double elo1, double eloe, const char *branch, const char *commit);

#endif
