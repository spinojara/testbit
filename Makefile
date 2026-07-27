MKDIR_P     = mkdir -p
RM          = rm
INSTALL     = install

PREFIX      = /usr/local
BINDIR      = $(PREFIX)/bin

CC          = gcc
CXX         = g++

CSTANDARD   = -std=c99
CXXSTANDARD = -std=c++11

CWARNINGS   = -Wall -Wextra -Wshadow -pedantic -Wno-unused-result -Wvla -Wno-missing-field-initializers
CXXWARNINGS = -Wall -Wextra -pedantic -Wno-unused-parameter
COPTIMIZE   = -O2
CDEBUG      = -g3 -fsanitize=address,undefined
INCLUDE     = -Iinclude -Iclop

CFLAGS      = $(CSTANDARD) $(CWARNINGS) $(COPTIMIZE) $(CDEBUG) $(INCLUDE) $(shell pkg-config --cflags libcjson libcurl sqlite3 ncurses libcrypt) -pthread
CXXFLAGS    = $(CXXSTANDARD) $(CXXWARNINGS) $(COPTIMIZE) $(CDEBUG) $(INCLUDE) -Drestrict=__restrict__

LDFLAGS     = $(CFLAGS)
LDLIBS      = $(shell pkg-config --libs libcjson libcurl ncurses sqlite3 libcrypt) -lm -lpthread

SRC_CLOP     = CPFQuadratic.cpp CParametricFunction.cpp CDiffFunction.cpp CMatrixOperations.cpp CParameterCollection.cpp CRegression.cpp CDFVariance.cpp CObserver.cpp CDFConfidence.cpp clop.cpp CMESampleMean.cpp CSPWeight.cpp CPFQuadratic.cpp CResults.cpp CLinearParameter.cpp
SRC_TESTBITD = testbitd.c socket.c http.c request.c sql.c util.c auth.c git.c get.c elo.c put.c post.c tc.c
SRC_TESTBITN = testbitn.c test.c util.c build.c git.c auth.c tc.c cgroup.c
OBJ_TESTBITD = $(patsubst %.c,obj/%.o,$(SRC_TESTBITD)) $(patsubst %.cpp,obj/%.o,$(SRC_CLOP))
OBJ_TESTBITN = $(patsubst %.c,obj/%.o,$(SRC_TESTBITN))

DEP          = $(patsubst %.c,dep/%.d,$(SRC_TESTBITD)) $(patsubst %.cpp,dep/%.d,$(SRC_CLOP)) $(patsubst %.c,dep/%.d,$(SRC_TESTBITN))

all: testbitd testbitn

testbitd: $(OBJ_TESTBITD)
	$(CXX) $(LDFLAGS) $^ -o $@ $(LDLIBS)

testbitn: CFLAGS += $(shell pkg-config --cflags inih libcurl)
testbitn: LDLIBS += $(shell pkg-config --libs inih libcurl)
testbitn: $(OBJ_TESTBITN)
	$(CC) $(LDFLAGS) $^ -o $@ $(LDLIBS)

cloplot: src/cloplot.c src/color.c
	$(CC) $(LDFLAGS) $^ -o $@ $(LDLIBS)

obj/%.o: src/%.c dep/%.d
	@$(MKDIR_P) obj
	$(CC) $(CFLAGS) -c $< -o $@

obj/%.o: clop/%.cpp dep/%.d
	@$(MKDIR_P) obj
	$(CXX) $(CXXFLAGS) -c $< -o $@

dep/%.d: src/%.c Makefile
	@$(MKDIR_P) dep
	@$(CC) -MM -MP -MT "$@ $(<:src/%.c=obj/%.o)" $(CFLAGS) $< -o $@

dep/%.d: clop/%.cpp Makefile
	@$(MKDIR_P) dep
	@$(CXX) -MM -MP -MT "$@ $(<:clop/%.cpp=obj/%.o)" $(CXXFLAGS) $< -o $@

install: all
	$(MKDIR_P) $(DESTDIR)$(BINDIR)
	$(INSTALL) -m 755 testbitd $(DESTDIR)$(BINDIR)/testbitd
	$(INSTALL) -m 755 testbitn $(DESTDIR)$(BINDIR)/testbitn

uninstall:
	$(RM) -f $(DESTDIR)$(BINDIR)/testbitd $(DESTDIR)$(BINDIR)/testbitn

clean:
	$(RM) -rf obj dep

-include $(DEP)

.PRECIOUS: dep/%.d
.SUFFIXES: .c .h .d
.PHONY: all install uninstall clean
