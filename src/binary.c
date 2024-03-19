#define _DEFAULT_SOURCE
#include "binary.h"

#include <endian.h>
#include <sys/stat.h>
#include <unistd.h>
#include <math.h>

#include "con.h"

#if CHAR_BIT != 8
#error
#endif

#ifndef __STDC_IEC_559__
#error
#endif

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

int recvstr(SSL *ssl, char **s) {
	uint64_t len;
	/* Limit string size to 16 KiB. */
	if (recvu64(ssl, &len) || len >= 16384)
		return 1;
	*s = malloc(len + 1);
	if (!*s)
		return 2;
	(*s)[len] = '\0';
	return recvexact(ssl, *s, len);
}

int sendfile(SSL *ssl, int fd) {
	struct stat statbuf;
	if (fstat(fd, &statbuf))
		return 2;
	if (sendu64(ssl, statbuf.st_size))
		return 1;
	char buf[BUFSIZ];
	ssize_t n;
	while ((n = read(fd, buf, sizeof(buf))) > 0)
		if (sendexact(ssl, buf, n))
			return 1;
	return n < 0;
}

int recvfile(SSL *ssl, int fd) {
	uint64_t len;
	uint64_t rec = 0;
	/* Limit file size to 16 MiB. */
	if (recvu64(ssl, &len) || len > 16777216)
		return 1;
	char c;
	while (rec < len && recvexact(ssl, &c, 1) > 0) {
		rec++;
		if (write(fd, &c, 1))
			return 1;
	}
	return rec < len;
}

