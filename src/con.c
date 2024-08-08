#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif
#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE
#endif
#include "con.h"

#include <stdarg.h>
#include <endian.h>
#include <limits.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <termios.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>

#include "binary.h"

int sendexact(SSL *ssl, const char *buf, size_t len) {
	if (len == 0)
		return 0;

	size_t sent = 0;
	size_t s = 0;

	while (sent < len) {
		s = 0;
		SSL_write_ex(ssl, buf + sent, len - sent, &s);
		if (s <= 0)
			break;
		sent += s;
	}

	return s <= 0;
}

int recvexact(SSL *ssl, char *buf, size_t len) {
	if (len == 0)
		return 0;

	memset(buf, 0, len);
	size_t rec = 0;
	size_t s = 0;

	while (rec < len) {
		s = 0;
		SSL_read_ex(ssl, buf + rec, len - rec, &s);
		if (s <= 0)
			break;
		rec += s;
	}

	return s <= 0;
}

int read_secret(char *secret, int size) {
	struct termios old, new;
	tcgetattr(STDIN_FILENO, &old);
	new = old;
	new.c_lflag &= ~ECHO;
	new.c_lflag |= ECHONL;

	if (tcsetattr(STDIN_FILENO, TCSADRAIN, &new))
		return 1;

	fgets(secret, size, stdin);

	tcsetattr(STDIN_FILENO, TCSADRAIN, &old);

	char *p = strchr(secret, '\n');
	if (p) {
		*p = '\0';
	}
	else {
		int c;
		/* Clear rest of password from stdin. */
		while ((c = getchar()) != '\n' && c != EOF);
	}

	return 0;
}

int create_secret(char *secret, int size) {
	int r;
	char *repeat = malloc(size);
	if (!repeat)
		return 1;
	printf("Choose a passphrase: ");
	if ((r = read_secret(secret, size)))
		goto clean;
	printf("Repeat the passphrase: ");
	if ((r = read_secret(repeat, size)))
		goto clean;
	if ((r = strcmp(secret, repeat))) {
		fprintf(stderr, "error: passphrases do not match\n");
		goto clean;
	}

clean:
	free(repeat);
	return r;
}

int sendf(SSL *ssl, const char *fmt, ...) {
	va_list ap;
	int r = 0;

	va_start(ap, fmt);

	for (; *fmt; fmt++) {
		switch (*fmt) {
		case 'c':
			r = sendi8(ssl, va_arg(ap, signed int));
			break;
		case 'C':
			r = sendu8(ssl, va_arg(ap, unsigned int));
			break;
		case 'h':
			r = sendi16(ssl, va_arg(ap, signed int));
			break;
		case 'H':
			r = sendu16(ssl, va_arg(ap, unsigned int));
			break;
		case 'l':
			r = sendi32(ssl, va_arg(ap, signed int));
			break;
		case 'L':
			r = sendu32(ssl, va_arg(ap, unsigned int));
			break;
		case 'q':
			r = sendi64(ssl, va_arg(ap, int64_t));
			break;
		case 'Q':
			r = sendu64(ssl, va_arg(ap, uint64_t));
			break;
		case 'd':
			r = sendd32(ssl, va_arg(ap, double));
			break;
		case 'D':
			r = sendd64(ssl, va_arg(ap, double));
			break;
		case 's':
			r = sendstr(ssl, va_arg(ap, const char *));
			break;
		case 'f':
			r = sendfile(ssl, va_arg(ap, int));
			break;
		default:
			break;
		}
		if (r)
			goto error;
	}

error:
	va_end(ap);
	return r;
}

int recvf(SSL *ssl, const char *fmt, ...) {
	va_list ap;
	int r = 0;

	va_start(ap, fmt);

	char *s;
	int fd;
	for (; *fmt; fmt++) {
		switch (*fmt) {
		case 'c':
			r = recvi8(ssl, va_arg(ap, signed char *));
			break;
		case 'C':
			r = recvu8(ssl, va_arg(ap, unsigned char *));
			break;
		case 'h':
			r = recvi16(ssl, va_arg(ap, int16_t *));
			break;
		case 'H':
			r = recvu16(ssl, va_arg(ap, uint16_t *));
			break;
		case 'l':
			r = recvi32(ssl, va_arg(ap, int32_t *));
			break;
		case 'L':
			r = recvu32(ssl, va_arg(ap, uint32_t *));
			break;
		case 'q':
			r = recvi64(ssl, va_arg(ap, int64_t *));
			break;
		case 'Q':
			r = recvu64(ssl, va_arg(ap, uint64_t *));
			break;
		case 'd':
			r = recvd32(ssl, va_arg(ap, float *));
			break;
		case 'D':
			r = recvd64(ssl, va_arg(ap, double *));
			break;
		case 's':
			s = va_arg(ap, char *);
			r = recvstr(ssl, s, va_arg(ap, size_t));
			break;
		case 'f':
			fd = va_arg(ap, int);
			r = recvfile(ssl, fd, va_arg(ap, ssize_t));
		default:
			break;
		}
		if (r)
			goto error;
	}

error:
	va_end(ap);
	return r;
}
