#ifndef PUT_H
#define PUT_H

#include "http.h"

void test_data(int fd, const struct http *http, int id);

void test_cancel(int fd, const struct http *http, int id);

void test_resume(int fd, const struct http *http, int id);

void test_requeue(int fd, const struct http *http, int id);

void test_error(int fd, const struct http *http, int id);

void task_new(int fd, const struct http *http, int id);

#endif
