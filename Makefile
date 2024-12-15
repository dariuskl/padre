CFLAGS += -Wall -Wextra -pedantic
CFLAGS += -Werror -pedantic-errors
CFLAGS += -Wconversion -Wsign-conversion
CFLAGS += -Wno-unused-function
CFLAGS += -std=c23 -nostdlib -fno-stack-protector -fwhole-program
CFLAGS += -fno-tree-loop-distribute-patterns  # don't emit calls to memset etc.
CFLAGS += -fno-math-errno

.PHONY: all clean test stats

all: build build/padre build/padre-debug build/test_padre build/test_sha256

clean:
	rm -r build

build:
	mkdir build

build/padre: CFLAGS += -O2 -fanalyzer
build/padre: src/linux_amd64.c \
	     src/linux.c       \
	     src/nonstd.h      \
	     src/nonstd.c      \
	     src/main.c        \
	     src/cli.c         \
	     src/tui.c         \
	     src/padre.c       \
	     src/padre.h       \
	     src/scrypt.c      \
	     src/sha256.c      \
	     Makefile
	$(CC) $(CPPFLAGS) $(CFLAGS) src/main.c -o $@ $(LDFLAGS)
	strip $@

build/padre-debug: CFLAGS += -O2 -Og -g
build/padre-debug: src/linux_amd64.c \
		   src/linux.c       \
		   src/nonstd.h      \
		   src/nonstd.c      \
		   src/main.c        \
		   src/cli.c         \
		   src/tui.c         \
		   src/padre.c       \
		   src/padre.h       \
		   src/scrypt.c      \
		   src/sha256.c      \
		   Makefile
	$(CC) $(CPPFLAGS) $(CFLAGS) src/main.c -o $@ $(LDFLAGS)

build/test_padre: src/linux_amd64.c \
		  src/linux.c       \
		  src/nonstd.h      \
		  src/nonstd.c      \
		  src/nonstd_test.h \
		  src/test_padre.c  \
		  src/sha256.c      \
		  Makefile
	$(CC) $(CPPFLAGS) $(CFLAGS) src/test_padre.c -o $@ $(LDFLAGS)

build/test_sha256: src/linux_amd64.c \
		   src/linux.c       \
		   src/nonstd.h      \
		   src/nonstd.c      \
		   src/test_sha256.c \
		   src/sha256.c      \
		   Makefile
	$(CC) $(CPPFLAGS) $(CFLAGS) src/test_sha256.c -o $@ $(LDFLAGS)

test: build/padre build/test_sha256 build/test_padre
	sh test.sh
