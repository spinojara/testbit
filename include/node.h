#ifndef NODE_H
#define NODE_H

#include <openssl/ssl.h>

void nodeloop(SSL *ssl, int cpus, char *syzygy);

#endif
