CC      ?= gcc
CFLAGS  ?= -O2 -Wall -Wextra -std=c11
LDFLAGS ?=

X11_CFLAGS := $(shell pkg-config --cflags x11 2>/dev/null)
X11_LIBS   := $(shell pkg-config --libs x11 2>/dev/null || echo -lX11)
LUA_CFLAGS  := $(shell pkg-config --cflags lua5.4 2>/dev/null || pkg-config --cflags lua5.3 2>/dev/null || pkg-config --cflags lua 2>/dev/null)
LUA_LIBS    := $(shell pkg-config --libs lua5.4 2>/dev/null || pkg-config --libs lua5.3 2>/dev/null || pkg-config --libs lua 2>/dev/null || echo -llua)

SRC := src/wm.c src/config.c src/plugins.c src/main.c
BIN := w3m

.PHONY: all clean test

all: $(BIN)

$(BIN): $(SRC) src/wm.h src/config.h src/plugins.h
	$(CC) $(CFLAGS) -DWM_WITH_LUA $(X11_CFLAGS) $(LUA_CFLAGS) \
	      -o $@ $(SRC) $(X11_LIBS) $(LUA_LIBS) -lm

# core-only build (logic verification, no X11/Lua)
w3m-core: src/wm.c src/config.c
	$(CC) $(CFLAGS) -o $@ src/wm.c src/config.c

test: w3m-core
	./tests/run_tests.sh

clean:
	rm -f $(BIN) w3m-core tests/test_wm
