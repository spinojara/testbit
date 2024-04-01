#ifndef TUI_H
#define TUI_H

#include <openssl/ssl.h>
#include <ncurses.h>

#define KEY_ESC 27

#define REFRESH_SECONDS 30

void tuiloop(SSL *ssl);

#endif
