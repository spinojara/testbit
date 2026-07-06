#ifndef UTIL_H
#define UTIL_H

#include <stddef.h>
#include <cjson/cJSON.h>

#define SIZE(arr) (sizeof(arr) / sizeof(*(arr)))

int startswith(const char *s1, const char *s2);

int endswith(const char *s1, const char *s2);

char *base64_decode(const char *s);

char *append_string(char *s, const char *a);

struct map {
	size_t length;
	struct entry {
		char *key;
		char *value;
	} *entries;
};

struct memory {
	char *data;
	size_t size;
};

void map_store(struct map *map, const char *key, const char *value);

void map_free(struct map *map);

const char *map_get(const struct map *map, const char *key, const char *d);

void cJSON_AddStringOrNullToObject(cJSON *json, const char *name, const char *string);

int strisalnum(const char *s);

char *read_fd(int fd);

#endif
