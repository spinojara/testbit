#ifndef HTTP_H
#define HTTP_H

#include <cjson/cJSON.h>

#include "util.h"

enum {
	HTTP_GET,
	HTTP_POST,
	HTTP_PUT,
	HTTP_OPTION,
};

struct http {
	int method;
	char *path;
	char *auth;
	struct map query;
	cJSON *content;
};

int send_json_response(int fd, const char *status, cJSON *json);

int send_response(int fd, const char *status, const char *response);

int get_headers(int fd, struct http *http);

void free_headers(struct http *http);

int bad_request(int fd, const char *message);

#endif
