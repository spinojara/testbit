#ifndef BINARY_H
#define BINARY_H

#include <stdint.h>

#include <openssl/ssl.h>

int sendi8(SSL *ssl, signed char s);
int sendu8(SSL *ssl, unsigned char s);
int sendi16(SSL *ssl, int16_t s);
int sendu16(SSL *ssl, uint16_t s);
int sendi32(SSL *ssl, int32_t s);
int sendu32(SSL *ssl, uint32_t s);
int sendi64(SSL *ssl, int64_t s);
int sendu64(SSL *ssl, uint64_t s);
int sendd32(SSL *ssl, float s);
int sendd64(SSL *ssl, double s);
int sendstr(SSL *ssl, const char *s);

int recvi8(SSL *ssl, signed char *s);
int recvu8(SSL *ssl, unsigned char *s);
int recvi16(SSL *ssl, int16_t *s);
int recvu16(SSL *ssl, uint16_t *s);
int recvi32(SSL *ssl, int32_t *s);
int recvu32(SSL *ssl, uint32_t *s);
int recvi64(SSL *ssl, int64_t *s);
int recvu64(SSL *ssl, uint64_t *s);
int recvd32(SSL *ssl, float *s);
int recvd64(SSL *ssl, double *s);
int recvstr(SSL *ssl, char **s);

#endif
