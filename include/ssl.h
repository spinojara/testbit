#ifndef SSL_H
#define SSL_H

#include <openssl/ssl.h>

int get_socket(void);

int get_listener_socket(void);

void ssl_close(SSL *ssl, int fast);

SSL_CTX *ssl_ctx_server(void);

SSL_CTX *ssl_ctx_client(void);

SSL *ssl_ssl_client(SSL_CTX *ctx, int fd);

#endif
