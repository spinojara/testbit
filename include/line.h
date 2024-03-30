#ifndef LINE_H
#define LINE_H

#include <stdlib.h>

struct line {
	char *data;
	size_t len;

	struct line *prev;
	struct line *next;
};

struct line *new_file(const char *file);

void free_file(struct line *top);

#endif
