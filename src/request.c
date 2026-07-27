#include "request.h"

#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <strings.h>

#include "util.h"
#include "http.h"
#include "auth.h"
#include "get.h"
#include "put.h"
#include "post.h"

struct handler {
	int method;
	int auth;
	const char *path;
	void (*handler)(int, const struct http *, int id);
};

const struct handler handlers[] = {
	{HTTP_POST, 1, "/test", &test_new},
	{HTTP_POST, 1, "/test/backup", &backup_database},

	{HTTP_PUT, 1, "/test/{id}", &test_data},
	{HTTP_PUT, 1, "/test/cancel/{id}", &test_cancel},
	{HTTP_PUT, 1, "/test/resume/{id}", &test_resume},
	{HTTP_PUT, 1, "/test/requeue/{id}", &test_requeue},
	{HTTP_PUT, 1, "/test/error/{id}", &test_error},
	{HTTP_PUT, 1, "/test/task", &task_new},

	{HTTP_GET, 0, "/test/{id}", &test_fetch_single},
	{HTTP_GET, 0, "/test", &test_fetch_all},
	{HTTP_GET, 0, "/spsa/{id}", &spsa_fetch_single},
	{HTTP_GET, 0, "/spsa", &spsa_fetch_all},
	{HTTP_GET, 0, "/clop/{id}", &clop_fetch_single},
	{HTTP_GET, 0, "/clop", &clop_fetch_all},
	{HTTP_GET, 0, "/build/{id}", &build_info},
};

int matches(int method, const char *path, int method1, const char *path1, int *id) {
	if (method != method1)
		return 0;

	if (startswith(path, "/testbit/"))
		path = path + strlen("/testbit");

	if (endswith(path1, "{id}")) {
		/* path does not start with path1 - {id}. */
		if (strncmp(path, path1, strlen(path1) - strlen("{id}")))
			return 0;

		errno = 0;
		char *endptr = NULL;
		if (strlen(path) <= strlen(path1) - strlen("{id}"))
			return 0;
		*id = strtol(path + strlen(path1) - strlen("{id}"), &endptr, 10);
		if (errno || *endptr != 0)
			return 0;

		return 1;
	}
	else {
		return !strcmp(path, path1);
	}
}

void handle_request(int fd, const struct http *http) {
	printf("method: %d\n", http->method);
	printf("path: '%s'\n", http->path);
	printf("map\n");
	for (size_t i = 0; i < http->query.length; i++) {
		printf("'%s': '%s'\n", http->query.entries[i].key, http->query.entries[i].value);
	}
	for (size_t i = 0; i < sizeof(handlers) / sizeof(*handlers); i++) {
		const struct handler *handler = &handlers[i];

		int id = 0;
		if (matches(http->method, http->path, handler->method, handler->path, &id)) {
			if (handler->auth && !authorize(http->auth)) {
				send_response(fd, "401 Unauthorized", "{\"message\": \"wrong password\"}");
				return;
			}
			handler->handler(fd, http, id);
			return;
		}
	}
	send_response(fd, "404 Not Found", "{\"message\": \"not found\"}");
}
