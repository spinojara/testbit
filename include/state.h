#ifndef STATE_H
#define STATE_H

#include <ncurses.h>
#include <openssl/ssl.h>
#include <arpa/inet.h>

#include "prompt.h"
#include "line.h"
#include "toggle.h"

struct test {
	int64_t id;

	char type;
	char status;

	char tc[128];
	double alpha;
	double beta;
	double elo0;
	double elo1;
	double eloe;

	char adjudicate;

	double llr;
	double elo, pm;

	int64_t qtime, stime, dtime;

	uint32_t t0, t1, t2;
	uint32_t p0, p1, p2, p3, p4;

	char branch[128];
	char commit[128];

	char host[INET6_ADDRSTRLEN];
};

struct oldteststate {
	WINDOW *win;

	int type;

	time_t last_loaded;
	int page_loaded;

	int tests;
	int selected;
	int64_t selected_id;

	struct test *test;

	int single;
	struct test singletest;

	struct line *patch;
	struct line *top;
	int fills;
	
	int page_size;
	int page;

	SSL *ssl;
};

struct newteststate {
	WINDOW *win;

	SSL *ssl;

	int selected;
	struct prompt prompt[11];
	struct toggle toggle[2];
};

struct state {
	WINDOW *win;

	int selected;

	struct newteststate ns;
	struct oldteststate as;
	struct oldteststate ds;
	struct oldteststate cs;
	struct oldteststate fs;
};

void init_state(struct state *st, SSL *ssl, int selected);
void term_state(struct state *st);

#endif
