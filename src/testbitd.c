#define _POSIX_C_SOURCE 200112L
#include <unistd.h>
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>
#include <poll.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <errno.h>
#include <shadow.h>

#include <openssl/ssl.h>

#include "con.h"
#include "ssl.h"
#include "req.h"
#include "sql.h"

static int running = 1;

void init_fds(struct fds *fds) {
	fds->fd_count = 2;
	fds->fd_size = 4;
	fds->pfds = malloc(fds->fd_size * sizeof(*fds->pfds));
	fds->cons = malloc(fds->fd_size * sizeof(*fds->cons));

	fds->pfds[0].fd = fds->listener;
	fds->pfds[0].events = POLLIN;
	fds->cons[0].type = TYPELISTENER;
	fds->cons[0].privileged = 1;

	fds->pfds[1].fd = STDIN_FILENO;
	fds->pfds[1].events = POLLIN;
	fds->cons[1].type = TYPESTDIN;
	fds->cons[1].privileged = 1;
}

void *get_in_addr(struct sockaddr *addr) {
	if (addr->sa_family == AF_INET)
		return &((struct sockaddr_in *)addr)->sin_addr;
	return &((struct sockaddr_in6 *)addr)->sin6_addr;
}

void add_to_fds(struct fds *fds, int newfd, SSL *ssl, char type, char name[INET6_ADDRSTRLEN]) {
	printf("connecting to %s\n", name);
	if (fds->fd_count == fds->fd_size) {
		fds->fd_size *= 2;
		fds->pfds = realloc(fds->pfds, fds->fd_size * sizeof(*fds->pfds));
		fds->cons = realloc(fds->cons, fds->fd_size * sizeof(*fds->cons));
		if (!fds->pfds || !fds->cons)
			exit(8);
	}
	memset(&(fds->pfds)[fds->fd_count], 0, sizeof(*fds->pfds));
	memset(&(fds->cons)[fds->fd_count], 0, sizeof(*fds->cons));

	fds->pfds[fds->fd_count].fd = newfd;
	fds->pfds[fds->fd_count].events = POLLIN;

	fds->cons[fds->fd_count].ssl = ssl;
	fds->cons[fds->fd_count].privileged = 0;
	fds->cons[fds->fd_count].type = type;
	fds->cons[fds->fd_count].status = STATUSWAIT;
	memcpy(fds->cons[fds->fd_count].name, name, INET6_ADDRSTRLEN);

	fds->fd_count++;
}

void del_from_fds(struct fds *fds, int i) {
	printf("closing %s\n", fds->cons[i].name);
	ssl_close(fds->cons[i].ssl, 1);
	fds->pfds[i] = fds->pfds[fds->fd_count - 1];
	fds->cons[i] = fds->cons[fds->fd_count - 1];
	fds->fd_count--;
}

void handle_new_connection(SSL_CTX *ctx, struct fds *fds) {
	struct sockaddr_storage remoteaddr;
	socklen_t addrlen = sizeof(remoteaddr);

	int newfd = accept(fds->listener, (struct sockaddr *)&remoteaddr, &addrlen);
	if (newfd == -1)
		return;

	char name[INET6_ADDRSTRLEN] = { 0 };
	if (!inet_ntop(remoteaddr.ss_family, get_in_addr((struct sockaddr *)&remoteaddr), name, INET6_ADDRSTRLEN))
		return;

	struct timeval tv = { .tv_sec = 10 };
	setsockopt(newfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

	SSL *ssl;
	BIO *bio;
	ssl = SSL_new(ctx);
	if (!ssl)
		exit(5);
	bio = BIO_new(BIO_s_socket());
	if (!bio)
		exit(6);

	BIO_set_fd(bio, newfd, BIO_CLOSE);
	SSL_set_bio(ssl, bio, bio);

	if (SSL_accept(ssl) <= 0) {
		SSL_free(ssl);
		return;
	}

	char type;
	if (recvexact(ssl, &type, 1)) {
		ssl_close(ssl, 1);
		return;
	}

	switch (type) {
	case TYPECLIENT:
	case TYPENODE:
		add_to_fds(fds, newfd, ssl, type, name);
		break;
	default:
		ssl_close(ssl, 1);
		return;
	}
}

void handle_stdin(void) {
	char buf[BUFSIZ];
	if (!fgets(buf, sizeof(buf), stdin) || !strcmp(buf, "quit\n"))
		running = 0;
}

static void stop(int signum) {
	(void)signum;
	running = 0;
}

int main(void) {
	signal(SIGPIPE, SIG_IGN);
	signal(SIGINT, &stop);
	signal(SIGTERM, &stop);

	SSL_CTX *ctx = ssl_ctx_server();
	if (!ctx)
		return 1;

	struct spwd *spwd = getspnam("testbit");
	if (!spwd || !spwd->sp_pwdp) {
		fprintf(stderr, "error: failed to retrieve testbit user\n");
		return 2;
	}
	const char *password = spwd->sp_pwdp;
	if (!password[0]) {
		fprintf(stderr, "error: testbit user has empty password\n");
		return 3;
	}

	sqlite3 *db;
	struct fds fds;
	fds.listener = get_listener_socket();
	if (fds.listener < 0) {
		fprintf(stderr, "error: failed to get listener socket\n");
		return 4;
	}
	
	init_fds(&fds);
	if (init_db(&db))
		return 7;

	while (running) {
		if (start_tests(db, &fds))
			return 8;

		if (poll(fds.pfds, fds.fd_count, -1) <= 0)
			continue;

		for (int i = 0; i < fds.fd_count; i++) {
			if (!(fds.pfds[i].revents & POLLIN))
				continue;

			struct connection *con = &fds.cons[i];

			time_t t = time(NULL);
			printf("Request from %d at %s", fds.pfds[i].fd, ctime(&t));

			int r = 0;
			switch (con->type) {
			case TYPELISTENER:
				handle_new_connection(ctx, &fds);
				break;
			case TYPESTDIN:
				handle_stdin();
				break;
			case TYPECLIENT:
				r = handle_client_request(con, db, password, &fds);
				break;
			case TYPENODE:
				r = handle_node_request(con, db, password);
				printf("returned %d\n", r);
				if (r && con->status == STATUSRUN)
					requeue_test(db, con->id);
				break;
			default:
				fprintf(stderr, "error: bad status\n");
				return 6;
			}

			if (r) {
				del_from_fds(&fds, i);
				break;
			}
		}
	}

	for (int i = 0; i < fds.fd_count; i++) {
		struct connection *con = &fds.cons[i];
		struct pollfd *pfds = &fds.pfds[i];
		switch (con->type) {
		case TYPELISTENER:
			close(pfds->fd);
			break;
		case TYPESTDIN:
			break;
		case TYPENODE:
		case TYPECLIENT:
			ssl_close(con->ssl, 1);
			break;
		}
	}
	free(fds.pfds);
	free(fds.cons);
	SSL_CTX_free(ctx);
	sqlite3_close(db);

	return 0;
}
