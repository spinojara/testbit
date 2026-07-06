#ifndef TEST_H
#define TEST_H

#include <curl/curl.h>
#include <pthread.h>

struct test {
	int id;

	int ready;
	int error;

	pthread_mutex_t lock;
	pthread_cond_t cond;

	int active;
	int unused;

	char *dir;
};

int load_test(int id, const char *url, CURL *curl, const char **dir);

void return_test(int id);

void test_init(void);

void test_term(void);

#endif
