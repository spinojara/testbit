CC         = gcc
CSTANDARD  = -std=c99
CWARNINGS  = -Wall -Wextra -Wshadow -pedantic -Wno-unused-result -Wvla
COPTIMIZE  = -O2

CFLAGS     = $(CSTANDARD) $(CWARNINGS) $(COPTIMIZE) $(pkg-config libcjson -pthread)
LDFLAGS    = $(CFLAGS) $(shell pkg-config --cflags libcjson libcurl ncurses)
LDLIBS     = $(shell pkg-config --libs libcjson libcurl ncurses) -lm -lpthread


cloplot: src/cloplot.c src/color.c
	$(CC) $(LDFLAGS) $^ -o $@ $(LDLIBS)


