#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE
#endif
#include "binary.h"

#include <endian.h>
#include <sys/stat.h>
#include <unistd.h>
#include <math.h>

#include "con.h"

int sendi8(SSL *ssl, signed char s) {
	union {
		signed char a;
		char c[1];
	} t = { .a = s };
	return sendexact(ssl, t.c, 1);
}

int sendu8(SSL *ssl, unsigned char s) {
	union {
		unsigned char a;
		char c[1];
	} t = { .a = s };
	return sendexact(ssl, t.c, 1);
}

int sendi16(SSL *ssl, int16_t s) {
	union {
		int16_t a;
		uint16_t b;
		char c[2];
	} t = { .a = s };
	t.b = htole16(t.b);
	return sendexact(ssl, t.c, 2);
}

int sendu16(SSL *ssl, uint16_t s) {
	union {
		uint16_t a;
		uint16_t b;
		char c[2];
	} t = { .a = s };
	t.b = htole16(t.b);
	return sendexact(ssl, t.c, 2);
}

int sendi32(SSL *ssl, int32_t s) {
	union {
		int32_t a;
		uint32_t b;
		char c[4];
	} t = { .a = s };
	t.b = htole32(t.b);
	return sendexact(ssl, t.c, 4);
}

int sendu32(SSL *ssl, uint32_t s) {
	union {
		uint32_t a;
		uint32_t b;
		char c[4];
	} t = { .a = s };
	t.b = htole32(t.b);
	return sendexact(ssl, t.c, 4);
}

int sendi64(SSL *ssl, int64_t s) {
	union {
		int64_t a;
		uint64_t b;
		char c[8];
	} t = { .a = s };
	t.b = htole64(t.b);
	return sendexact(ssl, t.c, 8);
}

int sendu64(SSL *ssl, uint64_t s) {
	union {
		uint64_t a;
		uint64_t b;
		char c[8];
	} t = { .a = s };
	t.b = htole64(t.b);
	return sendexact(ssl, t.c, 8);
}


int sendd32(SSL *ssl, float s) {
	union {
		float a;
		uint32_t b;
		char c[4];
	} t = { .a = s };
	t.b = htole32(t.b);
	return sendexact(ssl, t.c, 4);
}

int sendd64(SSL *ssl, double s) {
	union {
		double a;
		uint64_t b;
		char c[8];
	} t = { .a = s };
	t.b = htole64(t.b);
	return sendexact(ssl, t.c, 8);
}

int recvi8(SSL *ssl, signed char *s) {
	union {
		signed char a;
		char c[1];
	} t;
	if (recvexact(ssl, t.c, 1))
		return 1;
	*s = t.a;
	return 0;
}

int recvu8(SSL *ssl, unsigned char *s) {
	union {
		unsigned char a;
		char c[1];
	} t;
	if (recvexact(ssl, t.c, 1))
		return 1;
	*s = t.a;
	return 0;
}

int recvi16(SSL *ssl, int16_t *s) {
	union {
		int16_t a;
		uint16_t b;
		char c[2];
	} t;
	if (recvexact(ssl, t.c, 2))
		return 1;
	t.b = le16toh(t.b);
	*s = t.a;
	return 0;
}

int recvu16(SSL *ssl, uint16_t *s) {
	union {
		uint16_t a;
		uint16_t b;
		char c[2];
	} t;
	if (recvexact(ssl, t.c, 2))
		return 1;
	t.b = le16toh(t.b);
	*s = t.a;
	return 0;
}

int recvi32(SSL *ssl, int32_t *s) {
	union {
		int32_t a;
		uint32_t b;
		char c[4];
	} t;
	if (recvexact(ssl, t.c, 4))
		return 1;
	t.b = le32toh(t.b);
	*s = t.a;
	return 0;
}

int recvu32(SSL *ssl, uint32_t *s) {
	union {
		uint32_t a;
		uint32_t b;
		char c[4];
	} t;
	if (recvexact(ssl, t.c, 4))
		return 1;
	t.b = le32toh(t.b);
	*s = t.a;
	return 0;
}

int recvi64(SSL *ssl, int64_t *s) {
	union {
		int64_t a;
		uint64_t b;
		char c[8];
	} t;
	if (recvexact(ssl, t.c, 8))
		return 1;
	t.b = le64toh(t.b);
	*s = t.a;
	return 0;
}

int recvu64(SSL *ssl, uint64_t *s) {
	union {
		uint64_t a;
		uint64_t b;
		char c[8];
	} t;
	if (recvexact(ssl, t.c, 8))
		return 1;
	t.b = le64toh(t.b);
	*s = t.a;
	return 0;
}

int recvd32(SSL *ssl, float *s) {
	union {
		float a;
		uint32_t b;
		char c[4];
	} t;
	if (recvexact(ssl, t.c, 4))
		return 1;
	t.b = le32toh(t.b);
	*s = t.a;
	return 0;
}

int recvd64(SSL *ssl, double *s) {
	union {
		double a;
		uint64_t b;
		char c[8];
	} t;
	if (recvexact(ssl, t.c, 8))
		return 1;
	t.b = le64toh(t.b);
	*s = t.a;
	return 0;
}

int sendstr(SSL *ssl, const char *s) {
	if (sendu64(ssl, strlen(s)))
		return 1;
	return sendexact(ssl, s, strlen(s));
}

int recvstr(SSL *ssl, char *s, size_t size) {
	uint64_t len;
	if (recvu64(ssl, &len))
		return 1;
	
	size_t readlen;
	if (len < size)
		readlen = len;
	else
		readlen = size - 1;

	int error = 0;
	if (recvexact(ssl, s, readlen))
		error = 1;

	s[readlen] = '\0';

	char c;
	for (size_t i = 0; i < len - readlen && !error; i++)
		if (recvexact(ssl, &c, 1))
			error = 1;
	
	return error;
}

char *recvstrdynamic(SSL *ssl, size_t size) {
	if (size == 0)
		return NULL;
	uint64_t len;
	if (recvu64(ssl, &len))
		return NULL;

	size_t readlen;
	if (len < size)
		readlen = len;
	else
		readlen = size - 1;

	char *s = malloc(readlen + 1);
	if (!s)
		return NULL;

	int error = 0;
	if (recvexact(ssl, s, readlen))
		error = 1;

	s[readlen] = '\0';
	char c;
	for (size_t i = 0; i < len - readlen && !error; i++)
		if (recvexact(ssl, &c, 1))
			error = 1;

	return error ? free(s), NULL : s;
}

int sendfile(SSL *ssl, int fd) {
	struct stat statbuf;
	if (fstat(fd, &statbuf))
		return 2;
	if (sendu64(ssl, statbuf.st_size))
		return 1;
	char buf[BUFSIZ];
	ssize_t n;
	while ((n = read(fd, buf, sizeof(buf))) > 0) {
		if (sendexact(ssl, buf, n))
			return 1;
	}
	return n < 0;
}

int sendnullfile(SSL *ssl) {
	return sendu64(ssl, 0);
}

int recvfile(SSL *ssl, int fd, ssize_t size) {
	uint64_t len;
	uint64_t rec = 0;
	/* Limit file size to 128 MiB. */
	if (recvu64(ssl, &len) || len > 134217728)
		return 1;

	size_t readlen;
	if (size < 0)
		readlen = len;
	else if (len <= (uint64_t)size)
		readlen = len;
	else
		readlen = size;

	char c;
	while (rec < len && !recvexact(ssl, &c, 1)) {
		rec++;
		if (rec <= readlen)
			if (write(fd, &c, 1) != 1)
				exit(19);
	}
	return rec < len;
}

