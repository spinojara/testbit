#include "test.h"

#include <string.h>
#include <pthread.h>
#include <stdlib.h>
#include <curl/curl.h>
#include <cjson/cJSON.h>
#include <ftw.h>

#include "util.h"
#include "git.h"
#include "build.h"

pthread_mutex_t lock;

size_t n_tests = 0;
struct test **tests = NULL;

int rm(const char *path, const struct stat *sb, int typeflag, struct FTW *ftwbuf) {
	(void)sb;
	(void)typeflag;
	(void)ftwbuf;
	return remove(path);
}

void delete_test(struct test *test) {
	pthread_mutex_destroy(&test->lock);
	pthread_cond_destroy(&test->cond);

	if (test->dir)
		nftw(test->dir, rm, 64, FTW_DEPTH | FTW_PHYS);

	free(test->dir);
	free(test);
}

void clear_tests(void) {
	pthread_mutex_lock(&lock);
	for (size_t i = 0; i < n_tests; i++)
		delete_test(tests[i]);

	free(tests);
	tests = NULL;
	n_tests = 0;

	pthread_mutex_unlock(&lock);
}

void clear_old_tests(void) {
	pthread_mutex_lock(&lock);
	int cleared = 1;
	while (cleared) {
		cleared = 0;

		for (size_t i = 0; i < n_tests; i++) {
			struct test *test = tests[i];
			pthread_mutex_lock(&test->lock);
			if (!test->active && (test->error || test->unused > 16)) {
				cleared = 1;
				n_tests--;
				memmove(&tests[i], &tests[i + 1], (n_tests - i) * sizeof(*tests));
				pthread_mutex_unlock(&test->lock);
				delete_test(test);
				break;
			}
			pthread_mutex_unlock(&test->lock);
		}
	}
	pthread_mutex_unlock(&lock);
}

int load_test(int id, const char *url, CURL *curl, const char **dir) {
	clear_old_tests();
	*dir = NULL;
	pthread_mutex_lock(&lock);
	for (size_t i = 0; i < n_tests; i++) {
		struct test *test = tests[i];
		if (test->id == id && !test->error) {
			pthread_mutex_lock(&test->lock);
			test->active++;
			test->unused = 0;

			pthread_mutex_unlock(&lock);

			while (!test->ready)
				pthread_cond_wait(&test->cond, &test->lock);
			int error = test->error;
			if (error)
				test->active--;
			pthread_mutex_unlock(&test->lock);

			if (!error)
				*dir = test->dir;
			return error;
		}
		else {
			pthread_mutex_lock(&test->lock);
			test->unused++;
			pthread_mutex_unlock(&test->lock);
		}
	}

	n_tests++;
	tests = realloc(tests, n_tests * sizeof(*tests));
	if (!tests)
		exit(101);
	struct test *test = calloc(1, sizeof(*test));
	if (!test)
		exit(102);
	test->id = id;
	test->ready = 0;
	test->active = 1;
	tests[n_tests - 1] = test;
	pthread_mutex_init(&test->lock, NULL);
	pthread_cond_init(&test->cond, NULL);

	pthread_mutex_unlock(&lock);

	char *fullurl = calloc(strlen(url) + 1000, 1);
	if (!fullurl)
		exit(100);
	sprintf(fullurl, "%s/build/%d", url, id);
	struct memory chunk = { 0 };
	curl_easy_setopt(curl, CURLOPT_HTTPGET, 1);
	curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, NULL);
	curl_easy_setopt(curl, CURLOPT_URL, fullurl);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &chunk);
	printf("%s\n", fullurl);

	CURLcode res;
	cJSON *json;
	long code = 0;
	if ((res = curl_easy_perform(curl)) != CURLE_OK || (curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code), code != 200) || !(json = cJSON_Parse(chunk.data))) {
		free(chunk.data);
		free(fullurl);
		pthread_mutex_lock(&test->lock);
		test->error = 1;
		test->ready = 1;
		test->active--;
		pthread_cond_broadcast(&test->cond);
		pthread_mutex_unlock(&test->lock);
		return 1;
	}
	free(fullurl);

	free(chunk.data);
	cJSON *patch = cJSON_GetObjectItemCaseSensitive(json, "patch");
	cJSON *simd = cJSON_GetObjectItemCaseSensitive(json, "simd");
	cJSON *commit = cJSON_GetObjectItemCaseSensitive(json, "commit");
	cJSON *adjudicate = cJSON_GetObjectItemCaseSensitive(json, "adjudicate");
	if (!simd || !(cJSON_IsString(simd) || cJSON_IsNull(simd)) || !commit || !cJSON_IsString(commit) || check_ref_format(commit->valuestring) || !patch || !cJSON_IsString(patch) || !adjudicate || !cJSON_IsString(adjudicate)
 || build_test(test, patch->valuestring, cJSON_IsNull(simd) ? NULL : simd->valuestring, commit->valuestring, curl)) {
		cJSON_Delete(json);
		pthread_mutex_lock(&test->lock);
		test->error = 1;
		test->ready = 1;
		test->active--;
		pthread_cond_broadcast(&test->cond);
		pthread_mutex_unlock(&test->lock);
		return 1;
	}
	cJSON_Delete(json);

	pthread_mutex_lock(&test->lock);
	test->ready = 1;
	pthread_cond_broadcast(&test->cond);
	pthread_mutex_unlock(&test->lock);

	*dir = test->dir;
	return 0;
}

void return_test(int id) {
	pthread_mutex_lock(&lock);
	for (size_t i = 0; i < n_tests; i++) {
		struct test *test = tests[i];
		if (test->id == id && !test->error) {
			pthread_mutex_lock(&test->lock);
			test->active--;
			pthread_mutex_unlock(&test->lock);
		}
	}
	pthread_mutex_unlock(&lock);
}

void test_init(void) {
	pthread_mutex_init(&lock, NULL);
}

void test_term(void) {
	clear_tests();
	pthread_mutex_destroy(&lock);
}
