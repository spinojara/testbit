#include "util.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <unistd.h>
#include <errno.h>

int startswith(const char *str, const char *s) {
	return !strncmp(str, s, strlen(s));
}

int endswith(const char *str, const char *s) {
	if (strlen(str) < strlen(s))
		return 0;
	return !strcmp(str + strlen(str) - strlen(s), s);
}

static int base64_val(char c) {
	if (c >= 'A' && c <= 'Z')
		return c - 'A';
	if (c >= 'a' && c <= 'z')
		return c - 'a' + 26;
	if (c >= '0' && c <= '9')
		return c - '0' + 52;
	if (c == '+')
		return 62;
	if (c == '/')
		return 63;
	return -1;
}

char *base64_decode(const char *src) {
	char *out = calloc(strlen(src) + 1, 1);
	size_t len = 0;
	unsigned bits = 0, buf = 0;

	for (size_t i = 0; src[i]; i++) {
		if (src[i] == '=')
			break;
		int v = base64_val(src[i]);
		if (v == -1) {
			if (src[i] == '\n' || src[i] == '\r' || src[i] == ' ')
				continue;
			free(out);
			return NULL;
		}
		buf = (buf << 6) | v;
		bits += 6;
		if (bits >= 8) {
			bits -= 8;
			out[len++] = (buf >> bits) & 0xFF;
		}
	}

	return out;
}

char *append_string(char *s, const char *a) {
	size_t lens = strlen(s);
	size_t lena = strlen(a);
	s = realloc(s, lens + lena + 1);
	s[lens + lena] = 0;
	memcpy(s + lens, a, lena);

	return s;
}

void map_store(struct map *map, const char *key, const char *value) {
	printf("Storing '%s' = '%s'\n", key, value);
	for (size_t i = 0; i < map->length; i++) {
		struct entry *entry = &map->entries[i];
		if (!strcmp(entry->key, key)) {
			free(entry->value);
			entry->value = strdup(value);
			return;
		}
	}
	map->entries = realloc(map->entries, ++map->length * sizeof(*map->entries));
	map->entries[map->length - 1].key = strdup(key);
	map->entries[map->length - 1].value = strdup(value);
}

void map_free(struct map *map) {
	for (size_t i = 0; i < map->length; i++) {
		struct entry *entry = &map->entries[i];
		free(entry->key);
		free(entry->value);
	}
	free(map->entries);
	map->entries = 0;
}

const char *map_get(const struct map *map, const char *key, const char *d) {
	for (size_t i = 0; i < map->length; i++) {
		const struct entry *entry = &map->entries[i];
		if (!strcmp(entry->key, key))
			return entry->value;
	}
	return d;
}

void cJSON_AddStringOrNullToObject(cJSON *json, const char *name, const char *string) {
	if (!string)
		cJSON_AddNullToObject(json, name);
	else
		cJSON_AddStringToObject(json, name, string);
}

int strisalnum(const char *s) {
	if (!s)
		return 0;
	for (size_t i = 0; s[i]; i++)
		if (!isalnum((unsigned char)s[i]))
			return 0;
	return 1;
}

char *read_fd(int fd) {
	size_t size = 0;
	char *buf = calloc(size + 1, 1);
	char chunk[4096];
	ssize_t n;
	while ((n = read(fd, chunk, sizeof(chunk))) != 0) {
		if (n < 0) {
			if (errno == EINTR)
				continue;
			else
				break;
		}

		buf = realloc(buf, size + n + 1);
		if (!buf)
			exit(150);
		memcpy(buf + size, chunk, n);
		buf[size + n] = 0;
		size += n;
		if (size >= 16 * 1024 * 1024)
			break;
	}

	return buf;
}
