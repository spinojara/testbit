CC         = cc
CSTANDARD  = -std=c11
CWARNINGS  = -Wall -Wextra -Wshadow -pedantic -Wno-unused-result -Wvla
COPTIMIZE  = -O2
CDEBUG     =

CFLAGS     = $(CSTANDARD) $(CWARNINGS) $(COPTIMIZE) $(CDEBUG) -Iinclude
LDFLAGS    = $(CFLAGS)
LDLIBS     = -lm

SRC_TESTBIT  = testbit.c con.c ssl.c binary.c tui.c color.c draw.c \
	       state.c menu.c oldtest.c newtest.c prompt.c infobox.c util.c \
	       active.c done.c single.c line.c toggle.c tc.c
SRC_TESTBITN = testbitn.c con.c ssl.c binary.c setup.c sprt.c node.c util.c elo.c cgroup.c user.c source.c tc.c
SRC_TESTBITD = testbitd.c con.c ssl.c binary.c req.c reqc.c reqn.c sql.c tc.c
SRC_TCFACTOR = tcfactor.c source.c util.c user.c
SRC_ALL      = $(SRC_TESTBIT) $(SRC_TESTBITN) $(SRC_TESTBITD) $(SRC_TCFACTOR)

DEP = $(sort $(patsubst %.c,dep/%.d,$(SRC_ALL)))

OBJ_TESTBIT  = $(patsubst %.c,obj/%.o,$(SRC_TESTBIT))
OBJ_TESTBITN = $(patsubst %.c,obj/%.o,$(SRC_TESTBITN))
OBJ_TESTBITD = $(patsubst %.c,obj/%.o,$(SRC_TESTBITD))
OBJ_TCFACTOR = $(patsubst %.c,obj/%.o,$(SRC_TCFACTOR))

BIN          = testbit testbitn testbitd tcfactor

PREFIX = /usr/local
BINDIR = $(PREFIX)/bin

all: testbit

everything: $(BIN)

testbit: $(OBJ_TESTBIT)
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@
testbitd: $(OBJ_TESTBITD)
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@
testbitn: $(OBJ_TESTBITN)
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@
tcfactor: $(OBJ_TCFACTOR)
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@

obj/%.o: src/%.c dep/%.d
	@mkdir -p obj
	$(CC) $(CFLAGS) -c $< -o $@

testbit:  CFLAGS += $(shell pkg-config --cflags openssl ncurses) -DTERMINAL_FLICKER
testbit:  LDLIBS += $(shell pkg-config --libs openssl ncurses)
testbitn: CFLAGS += $(shell pkg-config --cflags openssl)
testbitn: LDLIBS += $(shell pkg-config --libs openssl)
testbitd: CFLAGS += $(shell pkg-config --cflags openssl sqlite3 libcrypt)
testbitd: LDLIBS += $(shell pkg-config --libs openssl sqlite3 libcrypt)

dep/%.d: src/%.c Makefile
	@mkdir -p dep
	@$(CC) -MM -MT "$@ $(<:src/%.c=obj/%.o)" $(CFLAGS) $< -o $@

install: all
	mkdir -p $(DESTDIR)$(BINDIR)
	install -m 0755 testbit $(DESTDIR)$(BINDIR)

install-everything: install everything
	mkdir -p $(DESTDIR)/var/lib/bitbit/{private,patch,nnue}
	chmod 700 $(DESTDIR)/var/lib/bitbit/private
	install -m 0755 testbit{n,d} $(DESTDIR)$(BINDIR)

install-daemon:
	mkdir -p $(DESTDIR)/etc/init.d
	install -m 0755 testbitd-openrc.sh $(DESTDIR)/etc/init.d/testbitd
	install -m 0755 testbitn-openrc.sh $(DESTDIR)/etc/init.d/testbitn

uninstall:
	rm -f $(DESTDIR)$(BINDIR)/testbit{,n,d}
	rm -f $(DESTDIR)/etc/init.d/testbit{d,n}
	rm -rf $(DESTDIR)/var/lib/bitbit

clean:
	rm -rf obj dep
	rm -f $(BIN)

doc: doc/maximumlikelihood.pdf doc/elo.pdf

doc/%.pdf: doc/src/%.tex doc/src/%.bib
	latexmk -pdf -cd $< -output-directory=../../doc

-include $(DEP)

.PRECIOUS: dep/%.d
.PHONY: all clean install install-everything install-daemon uninstall doc
