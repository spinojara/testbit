#ifndef REQUEST_H
#define REQUEST_H

#include <sqlite3.h>

#include "http.h"

void handle_request(int fd, const struct http *http);

#endif
