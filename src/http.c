#include "http.h"

#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>
#include <poll.h>

#include "util.h"

static int wait_or_stop(int fd, short events, time_t maxtime) {
	struct pollfd pfd[2] = {
		{ .fd = stop_read, .events = POLLIN },
		{ .fd = fd, .events = events },
	};

	while (1) {
		time_t now = time(NULL);
		if (now >= maxtime)
			return 1;

		int ret = poll(pfd, 2, (int)(1000 * (maxtime - now)));
		if (ret < 0) {
			if (errno == EINTR)
				continue;
			return 1;
		}
		/* Timed out. */
		if (ret == 0)
			return 1;
		/* Stopping wins over finishing the request. */
		if (pfd[0].revents & POLLIN)
			return 1;
		if (pfd[1].revents)
			return 0;
	}
}

void free_headers(struct http *http) {
	free(http->path);
	cJSON_Delete(http->content);
	free(http->auth);
	map_free(&http->query);
}

static size_t recvlen(struct fdreader *fdr, time_t maxtime) {
	if (fdr->buf[0])
		return strlen(fdr->buf);
	if (wait_or_stop(fdr->fd, POLLIN, maxtime))
		return 0;
	ssize_t ret = recv(fdr->fd, fdr->buf, BUFLEN - 1, 0);
	if (ret <= 0)
		return 0;
	fdr->buf[ret] = 0;
	if (strlen(fdr->buf) != (size_t)ret)
		return 0;
	return ret;
}

int recvfixed(char *buf, size_t size, struct fdreader *fdr, time_t maxtime) {
	if (size == 0)
		return 1;
	size_t real_size = size - 1;
	size_t received = 0;
	while (received < real_size) {
		size_t size_to_copy = recvlen(fdr, maxtime);
		if (!size_to_copy)
			return 1;
		if (received + size_to_copy > real_size)
			size_to_copy = real_size - received;
		fdtake(fdr, buf + received, size_to_copy);
		received += size_to_copy;
	}
	buf[received] = 0;
	return 0;
}

int recvline(char *buf, size_t size, struct fdreader *fdr, time_t maxtime) {
	if (size == 0)
		return 1;
	size_t real_size = size - 1;
	size_t received = 0;
	while (received < real_size) {
		size_t size_to_copy = recvlen(fdr, maxtime);
		if (!size_to_copy)
			return 1;

		char *newline = strchr(fdr->buf, '\n');
		if (newline && size_to_copy + fdr->buf > newline + 1)
			size_to_copy = newline - fdr->buf + 1;

		if (received + size_to_copy > real_size)
			size_to_copy = real_size - received;

		fdtake(fdr, buf + received, size_to_copy);
		received += size_to_copy;

		if (newline)
			break;
	}
	buf[received] = 0;
	return 0;
}

int sendexact(int fd, const char *buf, size_t size, time_t maxtime) {
	size_t sent = 0;
	while (sent < size) {
		if (wait_or_stop(fd, POLLOUT, maxtime))
			return 1;
		ssize_t ret = send(fd, &buf[sent], size - sent, 0);
		if (ret <= 0)
			return 1;
		sent += ret;
	}
	return 0;
}

int sendstr(int fd, const char *str, time_t maxtime) {
	return sendexact(fd, str, strlen(str), maxtime);
}

int recvexact(int fd, char *buf, size_t size, time_t maxtime) {
	size_t received = 0;
	while (received < size) {
		if (wait_or_stop(fd, POLLIN, maxtime))
			return 1;
		ssize_t ret = recv(fd, buf + received, size - received, 0);
		if (ret <= 0)
			return 1;
		received += ret;
	}
	return 0;
}

int send_json_response(int fd, const char *status, cJSON *json) {
	char *response = cJSON_PrintUnformatted(json);
	int ret = send_response(fd, status, response);
	cJSON_free(response);
	return ret;
}

int bad_request(int fd, const char *message) {
	const char *before = "{\"message\": \"";
	const char *after = "\"}";
	char *response = malloc(strlen(before) + strlen(message) + strlen(after) + 1);
	sprintf(response, "%s%s%s", before, message, after);
	int ret = send_response(fd, "400 Bad Request", response);
	free(response);
	return ret;
}

int send_response(int fd, const char *status, const char *response) {
	time_t maxtime = time(NULL) + 30;
	char header[512];
	sprintf(header,
		"HTTP/1.1 %s\r\n"
		"Content-Type: application/json\r\n"
		"Content-Length: %zu\r\n"
		"Connection: close\r\n"
		"\r\n"
	, status, strlen(response));
	int ret = sendstr(fd, header, maxtime) || sendstr(fd, response, maxtime);
	close(fd);
	return ret;
}

int parse_query(struct http *http, const char *query) {
	int done = 0;
	const char *equal = strchr(query, '=');
	if (!equal)
		return 1;
	const char *end = strchr(equal, '&');
	if (!end) {
		end = &query[strlen(query)];
		done = 1;
	}

	char *key = calloc(equal - query + 1, 1);
	char *value = calloc(end - (equal + 1) + 1, 1);

	memcpy(key, query, equal - query);
	memcpy(value, equal + 1, end - (equal + 1));

	map_store(&http->query, key, value);

	free(key);
	free(value);

	if (!done)
		return parse_query(http, end + 1);
	return 0;
}

int get_headers(int fd, struct http *http) {
	time_t maxtime = time(NULL) + 30;
	struct fdreader fdr = { .fd = fd };
	char buf[4096] = { 0 };

	if (!recvline(buf, sizeof(buf), &fdr, maxtime)) {
		if (startswith(buf, "GET "))
			http->method = HTTP_GET;
		else if (startswith(buf, "POST "))
			http->method = HTTP_POST;
		else if (startswith(buf, "PUT "))
			http->method = HTTP_PUT;
		else if (startswith(buf, "OPTIONS "))
			http->method = HTTP_OPTION;
		else {
			bad_request(fd, "bad method");
			return 1;
		}

		char *space[2];
		if (!(space[0] = strchr(buf, ' '))) {
			bad_request(fd, "bad path");
			return 1;
		}
		space[0]++;
		if (!(space[1] = strchr(space[0], ' ')) || space[1] == space[0]) {
			bad_request(fd, "bad path");
			return 1;
		}

		size_t pathlen = space[1] - space[0];
		http->path = calloc(pathlen + 1, 1);

		memcpy(http->path, space[0], pathlen);

		char *query;
		if ((query = strchr(http->path, '?'))) {
			if (parse_query(http, query + 1)) {
				bad_request(fd, "bad query");
				return 1;
			}
			query[0] = 0;
		}
	}
	else {
		close(fd);
		return 1;
	}
	long long content_length = 0;
	while (!recvline(buf, sizeof(buf), &fdr, maxtime)) {
		if (!strcmp(buf, "\r\n"))
			break;
		else if (startswith_nocase(buf, "Content-Length: ")) {
			if (content_length) {
				bad_request(fd, "bad content length");
				return 1;
			}
			char *endptr = NULL;
			errno = 0;
			content_length = strtoll(buf + strlen("Content-Length: "), &endptr, 10);
			if (errno || *endptr != '\r' || content_length > 512 * 1024 * 1024 || content_length < 0) {
				/* There is a specific http code for content length. */
				bad_request(fd, "bad content length");
				return 1;
			}
		}
		else if (startswith_nocase(buf, "Authorization: Basic ")) {
			if (http->auth) {
				bad_request(fd, "bad authorization");
				return 1;
			}
			if (!(http->auth = base64_decode(buf + strlen("Authorization: Basic ")))) {
				bad_request(fd, "bad authorization");
				return 1;
			}
		}
	}
	if (content_length > 0) {
		char *content = malloc(content_length + 1);
		if (!content || recvfixed(content, content_length + 1, &fdr, maxtime)) {
			free(content);
			bad_request(fd, "bad content");
			return 1;
		}
		http->content = cJSON_Parse(content);
		free(content);
		if (!http->content) {
			bad_request(fd, "bad json");
			return 1;
		}
	}
	return 0;
}
