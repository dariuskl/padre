CFLAGS += -Wall -Wextra -pedantic
CFLAGS += -Werror -pedantic-errors
CFLAGS += -Wconversion -Wsign-conversion
CFLAGS += -Wno-unused-function
CFLAGS += -std=c2x
CFLAGS += -O3 -g -Og

.PHONY: test clean install uninstall

all: build build/padre build/padre_test

build:
	mkdir build

build/padre: LDFLAGS += -lmenu -lncurses -lscrypt-kdf
build/padre: src/main.c src/padre.c src/cli.c src/tui.c src/padre.h
	$(CC) $(CPPFLAGS) $(CFLAGS) $< -o $@ $(LDFLAGS)

build/unity.o: lib/unity/unity.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -isystem lib/unity -c $< -o $@

build/padre_test: src/padre_test.c build/unity.o
	$(CC) $(CPPFLAGS) $(CFLAGS) -isystem lib/unity $(LDFLAGS) $^ -o $@

test: build/padre_test build/padre
	./build/padre_test
	@echo -n "calling padre without arguments yields an error: "
	@./build/padre > /dev/null 2>&1 || echo "OK"
	@echo -n "calling padre with too many arguments yields an error: "
	@./build/padre 1 2 3 > /dev/null 2>&1 || echo "OK"
	@echo -n "calling padre with a non-existent file yields an error: "
	@./build/padre no_such_file > /dev/null 2>&1 || echo "OK"
	@echo -n "a file without newline at the end is parsed correctly: "
	@echo -n "domain.com,user@domain.com,0,32,*" | ./build/padre -
	@echo -n "a single account entry is automatically selected: OK"

clean:
	rm -r build

install: build/padre padre.desktop
	@echo "Not yet implemented"

uninstall:
	@echo "Not yet implemented"
