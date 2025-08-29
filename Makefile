CFLAGS += -std=c99 -Wall -Wextra -Werror -Wold-style-declaration -Wno-maybe-uninitialized -O3 -pedantic
PREFIX  ?= /usr
BINDIR  ?= $(PREFIX)/bin
MANDIR  ?= $(PREFIX)/share/man
CC      ?= gcc

all: ori

ori: ori.c ori.h termbox2.h Makefile
	$(CC) -O3 $(CFLAGS) -o $@ $< $(LDFLAGS) 

install: all
	install -Dm755 ori $(DESTDIR)$(BINDIR)/ori
	install -Dm644 ori.1 $(DESTDIR)$(MANDIR)/man1/ori.1

uninstall:
	rm -f $(DESTDIR)$(BINDIR)/ori
	rm -f $(DESTDIR)$(MANDIR)/man1/ori.1

clean:
	rm -f ori *.o 

.PHONY: all install uninstall clean

