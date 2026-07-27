#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <poll.h>
#include <unistd.h>
#include <signal.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <time.h>
#include <sys/time.h>
#include <pthread.h>
#include <stdatomic.h>
#include <sqlite3.h>
#include <getopt.h>
#include <errno.h>

#include "socket.h"
#include "sql.h"
#include "http.h"
#include "request.h"
#include "clop.h"
#include "post.h"

struct worker {
	pthread_t thread;
	pthread_mutex_t mutex;
	pthread_cond_t cond;
	int sleep;
	int fd;
};
struct worker *workers = NULL;
int pool_size = 1;

int available_workers = 0;
pthread_mutex_t available_workers_mutex;
pthread_cond_t available_workers_cond;

atomic_int stop;

int stop_read;
int stop_write;

static void sigint_handler(int signum) {
	(void)signum;
	int saved_errno = errno;
	atomic_store_explicit(&stop, 1, memory_order_relaxed);
	char c = 1;
	write(stop_write, &c, 1);
	errno = saved_errno;
}

void increase_available_workers(void) {
	pthread_mutex_lock(&available_workers_mutex);
	available_workers++;
	pthread_cond_signal(&available_workers_cond);
	pthread_mutex_unlock(&available_workers_mutex);
}

/* Wakes every worker so it notices stop. The signal handler cannot do this
 * itself, as pthread_mutex_lock and pthread_cond_signal are not
 * async-signal-safe. Signalling while holding the worker's own mutex is what
 * keeps a worker sitting between testing stop and calling pthread_cond_wait
 * from missing the wakeup and sleeping forever.
 */
void wake_workers(void) {
	for (int i = 0; i < pool_size; i++) {
		pthread_mutex_lock(&workers[i].mutex);
		pthread_cond_signal(&workers[i].cond);
		pthread_mutex_unlock(&workers[i].mutex);
	}
}

void *worker(void *arg) {
	struct worker *w = arg;
	while (1) {
		pthread_mutex_lock(&w->mutex);
		w->sleep = 1;
		pthread_mutex_unlock(&w->mutex);

		increase_available_workers();

		pthread_mutex_lock(&w->mutex);
		while (!atomic_load_explicit(&stop, memory_order_relaxed) && w->sleep) {
			printf("Going to sleep\n");
			pthread_cond_wait(&w->cond, &w->mutex);
		}

		if (atomic_load_explicit(&stop, memory_order_relaxed)) {
			pthread_mutex_unlock(&w->mutex);
			break;
		}
		int fd = w->fd;
		pthread_mutex_unlock(&w->mutex);

		struct http http = { 0 };
		if (get_headers(fd, &http)) {
			free_headers(&http);
			continue;
		}
		handle_request(fd, &http);
		free_headers(&http);
	}
	return NULL;
}

void *get_in_addr(const void *addr) {
	if (((const struct sockaddr *)addr)->sa_family == AF_INET)
		return &((struct sockaddr_in *)addr)->sin_addr;
	return &((struct sockaddr_in6 *)addr)->sin6_addr;
}

void handle_connection(int listener) {
	printf("new connection\n");
	struct sockaddr_storage remoteaddr;
	socklen_t addrlen = sizeof(remoteaddr);
	int fd = accept(listener, (struct sockaddr *)&remoteaddr, &addrlen);
	if (fd == -1)
		return;

	char name[INET6_ADDRSTRLEN] = { 0 };
	if (!inet_ntop(remoteaddr.ss_family, get_in_addr(&remoteaddr), name, INET6_ADDRSTRLEN)) {
		close(fd);
		return;
	}

	struct timeval tv = { .tv_sec = 30 };
	setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

	pthread_mutex_lock(&available_workers_mutex);
	while (!available_workers) {
		printf("waiting for available workers\n");
		pthread_cond_wait(&available_workers_cond, &available_workers_mutex);
	}
	available_workers--;
	pthread_mutex_unlock(&available_workers_mutex);

	for (int i = 0; i < pool_size; i++) {
		struct worker *w = &workers[i];
		printf("checking worker %d\n", i);
		pthread_mutex_lock(&w->mutex);
		if (!w->sleep) {
			pthread_mutex_unlock(&w->mutex);
			continue;
		}
		w->sleep = 0;
		w->fd = fd;
		pthread_cond_signal(&w->cond);
		pthread_mutex_unlock(&w->mutex);
		return;
	}
	fprintf(stderr, "error: had available workers, but none available?\n");
	exit(1);
}

int main(int argc, char **argv) {
	const char *port = "2718";
	char *db_path = NULL;
	static struct option opts[] = {
		{"db", required_argument, 0, 'd' },
		{"backup", required_argument, 0, 'b' },
		{"port", required_argument, 0, 'p' },
		{"pool-size", required_argument, 0, 'P' },
		{ 0, 0, 0, 0 },
	};

	char *endptr;
	int c, option_index = 0;
	int error = 0;
	while ((c = getopt_long(argc, argv, "", opts, &option_index)) != -1) {
		switch (c) {
		case 'd':
			db_path = optarg;
			break;
		case 'b':
			if (strlen(optarg) >= 4096) {
				error = 1;
				fprintf(stderr, "error: bad --backup\n");
			}
			else {
				strcpy(backupdir, optarg);
			}
			break;
		case 'p':
			port = optarg;
			errno = 0;
			int portnum = strtol(port, &endptr, 10);
			if (errno || *endptr || portnum <= 0 || portnum > 65535) {
				error = 1;
				fprintf(stderr, "error: bad --port\n");
			}
			break;
		case 'P':
			errno = 0;
			pool_size = strtol(optarg, &endptr, 10);
			if (errno || *endptr || pool_size <= 0) {
				error = 1;
				fprintf(stderr, "error: bad --pool-size\n");
			}
			break;
		default:
			error = 1;
			break;
		}
	}

	if (error)
		exit(1);

	if (!db_path) {
		fprintf(stderr, "error: need --db\n");
		exit(1);
	}

	srand(time(NULL));
	clop_init();

	signal(SIGPIPE, SIG_IGN);

	int stop_fd[2];
	if (pipe(stop_fd)) {
		fprintf(stderr, "error: failed to create stop pipe\n");
		exit(1);
	}
	stop_read = stop_fd[0];
	stop_write = stop_fd[1];
	signal(SIGINT, sigint_handler);

	int listener = get_listener_socket(port);
	if (listener < 0) {
		fprintf(stderr, "error: failed to bind\n");
		exit(1);
	}

	if (db_open(db_path)) {
		fprintf(stderr, "error: failed to open '%s'\n", db_path);
		exit(1);
	}

	pthread_mutex_init(&available_workers_mutex, NULL);
	pthread_cond_init(&available_workers_cond, NULL);

	workers = calloc(pool_size, sizeof(*workers));
	for (int i = 0; i < pool_size; i++) {
		struct worker *w = &workers[i];
		w->sleep = 1;
		pthread_mutex_init(&w->mutex, NULL);
		pthread_cond_init(&w->cond, NULL);
		pthread_create(&w->thread, NULL, &worker, w);
	}

	struct pollfd fds[2] = {
		{ .fd = listener, .events = POLLIN },
		{ .fd = stop_read, .events = POLLIN },
	};

	while (!atomic_load_explicit(&stop, memory_order_relaxed)) {
		if (poll(fds, 2, -1) <= 0)
			continue;

		if (fds[1].revents & POLLIN)
			break;

		if (fds[0].revents & POLLIN)
			handle_connection(listener);
	}

	printf("exiting...\n");
	wake_workers();
	for (int i = 0; i < pool_size; i++) {
		struct worker *w = &workers[i];
		pthread_join(w->thread, NULL);
		pthread_mutex_destroy(&w->mutex);
		pthread_cond_destroy(&w->cond);
	}
	pthread_mutex_destroy(&available_workers_mutex);
	pthread_cond_destroy(&available_workers_cond);
	free(workers);
	db_close();
	clop_term();

	close(stop_read);
	close(stop_write);
}
