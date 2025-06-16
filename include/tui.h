#ifndef TUI_H
#define TUI_H

#include <openssl/ssl.h>
#include <ncurses.h>

#define KEY_ESC 27

#define REFRESH_SECONDS 5

void tuiloop(SSL *ssl);

void die(int ret, const char *str);

void lostcon(void);

#endif
