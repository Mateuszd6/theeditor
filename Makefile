.PHONY: all debug release clean clang_complete

CC=clang++
CFLAGS=-std=c++17 -fno-exceptions -fno-rtti

DISABLED_WARNINGS=-Wno-padded \
                  -Wno-sign-conversion

SANITIZERS=-fsanitize=address,undefined

ifeq ($(CC),clang++)
	WARN_FLAGS=-Weverything
	DISABLED_WARNINGS += -Wno-c++98-compat-pedantic \
			     -Wno-gnu-zero-variadic-macro-arguments \
			     -Wno-gnu-anonymous-struct \
			     -Wno-nested-anon-types \
			     -Wno-vla-extension \
			     -Wno-vla
else
	WARN_FLAGS=-Wall -Wextra -Wshadow
endif

DEBUG_FLAGS=-g -O0 -DDEBUG -fno-omit-frame-pointer
RELEASE_FLAGS=-O3 -DNDEBUG

INCLUDE_FLAGS=$(shell pkg-config --cflags x11 xft freetype2) -I.
LINKER_FLAGS=$(shell pkg-config --libs x11 xft freetype2)

all: debug

debug:
	$(CC) $(CFLAGS) $(WARN_FLAGS) $(DISABLED_WARNINGS) $(DEBUG_FLAGS)   \
	$(INCLUDE_FLAGS) $(LINKER_FLAGS) $(SANITIZERS) main.cpp -o program
release:
	$(CC) $(CFLAGS) $(WARN_FLAGS) $(DISABLED_WARNINGS) $(RELEASE_FLAGS) \
	$(INCLUDE_FLAGS) $(LINKER_FLAGS) main.cpp -o program

clang_complete:
	echo $(INCLUDE_FLAGS) $(LINKER_FLAGS) > .clang_complete
