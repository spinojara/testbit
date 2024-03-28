#ifndef SPRT_H
#define SPRT_H

#include <stdint.h>

#include <openssl/ssl.h>

enum {
	TESTQUEUE,
	TESTRUN,
	TESTCANCEL,
	TESTERRBRANCH,
	TESTERRCOMMIT,
	TESTERRPATCH,
	TESTERRMAKE,
	TESTERRRUN,

	TESTINCONCLUSIVE,
	TESTH0,
	TESTH1,
	TESTELO,
};

enum {
	TESTTYPESPRT,
	TESTTYPEELO,
};

void sprt(SSL *ssl, int type, int games, int nthreads, double maintime, double increment, double alpha, double beta, double elo0, double elo1, double eloe);

#endif
