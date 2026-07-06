#ifndef GET_H
#define GET_H

#include "http.h"

void test_fetch_single(int fd, const struct http *http, int id);

void test_fetch_all(int fd, const struct http *http, int id);

void clop_fetch_single(int fd, const struct http *http, int id);

void clop_fetch_all(int fd, const struct http *http, int id);

void spsa_fetch_single(int fd, const struct http *http, int id);

void spsa_fetch_all(int fd, const struct http *http, int id);

void build_info(int fd, const struct http *http, int id);

#endif
