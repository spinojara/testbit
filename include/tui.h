#ifndef TUI_H
#define TUI_H

#include <openssl/ssl.h>
#include <ncurses.h>

#define KEY_ESC 27

void tuiloop(SSL *ssl);

#endif
