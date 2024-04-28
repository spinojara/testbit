#include "line.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <ncurses.h>

size_t physlen(const char *s) {
	size_t len;
	for (len = 0; *s; s++)
		len += *s == '\t' ? TABSIZE - (len % TABSIZE) : 1;
	return len;
}

struct line *next_line(const char **file, struct line *prev) {
	char *s;
	if (!(s = strchr(*file, '\n')))
		return NULL;

	struct line *line = malloc(sizeof(*line));
	if (!line)
		return NULL;

	size_t len = s - *file;

	line->data = malloc(len + 1);
	if (!line->data) {
		free(line);
		return NULL;
	}
	memcpy(line->data, *file, len);
	line->data[len] = '\0';

	*file += len + 1;
	line->prev = prev;
	line->next = NULL;
	line->len = physlen(line->data);
	return line;
}

struct line *new_file(const char *file) {
	struct line *top = next_line(&file, NULL);
	struct line *cur;
	for (cur = top; cur; cur = cur->next)
		cur->next = next_line(&file, cur);

	return top;
}

void free_file(struct line *top) {
	while (top->next) {
		top = top->next;
		free(top->prev->data);
		free(top->prev);
	}
	free(top->data);
	free(top);
}
