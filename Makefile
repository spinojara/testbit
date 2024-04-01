CC         = cc
CSTANDARD  = -std=c11
CWARNINGS  = -Wall -Wextra -Wshadow -pedantic -Wno-unused-result -Wvla
COPTIMIZE  = -O2
CDEBUG     =

CFLAGS     = $(CSTANDARD) $(CWARNINGS) $(COPTIMIZE) $(CDEBUG) -Iinclude -DTERMINAL_FLICKER
LDFLAGS    = $(CFLAGS)
LDLIBS     = -lm -lssl -lcrypto

SRC_TESTBIT  = testbit.c con.c ssl.c binary.c tui.c color.c draw.c \
	       state.c menu.c oldtest.c newtest.c prompt.c infobox.c util.c \
	       active.c done.c single.c line.c
SRC_TESTBITN = testbitn.c con.c ssl.c binary.c setup.c sprt.c node.c util.c elo.c
SRC_TESTBITD = testbitd.c con.c ssl.c binary.c req.c reqc.c reqn.c sql.c
SRC_ALL      = $(SRC_TESTBIT) $(SRC_TESTBITN) $(SRC_TESTBITD)

DEP = $(sort $(patsubst %.c,dep/%.d,$(SRC_ALL)))

OBJ_testbit  = $(patsubst %.c,obj/%.o,$(SRC_TESTBIT))
OBJ_testbitn = $(patsubst %.c,obj/%.o,$(SRC_TESTBITN))
OBJ_testbitd = $(patsubst %.c,obj/%.o,$(SRC_TESTBITD))

BIN          = testbit testbitn testbitd

PREFIX = /usr/local
BINDIR = $(PREFIX)/bin

all: $(BIN)

.SECONDEXPANSION:
$(BIN): $$(OBJ_$$@)
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@

obj/%.o: src/%.c dep/%.d
	@mkdir -p obj
	$(CC) $(CFLAGS) -c $< -o $@

testbit:  LDLIBS += -lncurses -ltinfo
testbitd: LDLIBS += -lsqlite3

dep/%.d: src/%.c Makefile
	@mkdir -p dep
	@$(CC) -MM -MT "$@ $(<:src/%.c=obj/%.o)" $(CFLAGS) $< -o $@

install: all
	mkdir -p $(DESTDIR)$(BINDIR)
	mkdir -p $(DESTDIR)/var/lib/bitbit/{certs,private,patch}
	chmod 700 $(DESTDIR)/var/lib/bitbit/private
	cp -f testbit $(DESTDIR)$(BINDIR)/testbit
	chmod 755 $(DESTDIR)$(BINDIR)/testbit
	cp -f testbitn $(DESTDIR)$(BINDIR)/testbitn
	chmod 755 $(DESTDIR)$(BINDIR)/testbitn
	cp -f testbitd $(DESTDIR)$(BINDIR)/testbitd
	chmod 755 $(DESTDIR)$(BINDIR)/testbitd

uninstall:
	rm -f $(DESTDIR)$(BINDIR)/{testbit,testbitn,testbitd}
	rm -rf $(DESTDIR)/var/lib/bitbit

clean:
	rm -rf obj dep
	rm -f $(BIN)

doc: doc/maximumlikelihood.pdf doc/elo.pdf

doc/%.pdf: doc/src/%.tex doc/src/%.bib
	latexmk -pdf -cd $< -output-directory=../../doc

-include $(DEP)

.PRECIOUS: dep/%.d
.PHONY: all clean install uninstall doc
