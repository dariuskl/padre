CFLAGS += -Wall -Wextra -pedantic
CFLAGS += -Werror -pedantic-errors
CFLAGS += -Wconversion -Wsign-conversion
CFLAGS += -Wno-unused-function
CFLAGS += -std=c23
CFLAGS += -O3
CFLAGS += -Ilib -isystem lib/nuklear -isystem lib/unity
LDFLAGS += -lscrypt-kdf

.PHONY: test clean install uninstall

all: build build/padre build/padre_gui test

build:
	mkdir build

build/padre: src/cli.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) $< -o $@

build/nuklear.o: lib/nuklear_impl.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

build/padre_gui: src/gui.c build/nuklear.o
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -lSDL2 -lm $^ -o $@

build/unity.o: lib/unity/unity.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -Ilib/unity -c $< -o $@

build/padre_test: src/padre_test.c build/unity.o
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) $^ -o $@

test: build/padre_test
	./build/padre_test

clean:
	rm -r build

install: padre padre_gui padre.desktop
	@echo "Not yet implemented"

uninstall:
	@echo "Not yet implemented"
