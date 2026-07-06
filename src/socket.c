#define _POSIX_C_SOURCE 200112L
#include "socket.h"

#include <netdb.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <unistd.h>

int get_listener_socket(const char *port) {
	int listener, yes = 1;

	struct addrinfo hints = { 0 }, *ai, *p;
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	if (getaddrinfo(NULL, port, &hints, &ai))
		return -1;

	for (p = ai; p; p = p->ai_next) {
		if ((listener = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) < 0)
			continue;
		setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

		if (bind(listener, p->ai_addr, p->ai_addrlen) < 0) {
			close(listener);
			continue;
		}

		break;
	}

	freeaddrinfo(ai);

	if (!p)
		return -1;

	if (listen(listener, 10) == -1)
		return -1;

	return listener;
}
