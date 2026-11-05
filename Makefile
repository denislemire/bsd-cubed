# SPDX-License-Identifier: BSD-2-Clause
# Portable make syntax: works with FreeBSD make and GNU make.
PREFIX ?= /usr/local
LOCALBASE ?= /usr/local
CC ?= cc
AR ?= ar
PKG_CONFIG ?= pkg-config
CFLAGS ?= -O2
BSD3_CFLAGS = -std=c11 -Wall -Wextra -Werror
BSD3_CPPFLAGS = -Iinclude `$(PKG_CONFIG) --cflags gio-unix-2.0`
BSD3_LIBS = `$(PKG_CONFIG) --libs gio-unix-2.0`
INSTALL ?= install

all: bsddd libbsd3-login.a

login.o: src/login.c include/bsd3/sd-login.h
	$(CC) $(CPPFLAGS) $(BSD3_CPPFLAGS) $(CFLAGS) $(BSD3_CFLAGS) -c src/login.c -o $@

libbsd3-login.a: login.o
	$(AR) rcs $@ login.o

bsddd: src/bsddd.c libbsd3-login.a
	$(CC) $(CPPFLAGS) $(BSD3_CPPFLAGS) $(CFLAGS) $(BSD3_CFLAGS) $(LDFLAGS) -o $@ src/bsddd.c libbsd3-login.a $(BSD3_LIBS)

mock-ck: tests/mock-ck.c
	$(CC) $(CPPFLAGS) $(BSD3_CPPFLAGS) $(CFLAGS) $(BSD3_CFLAGS) $(LDFLAGS) -o $@ tests/mock-ck.c $(BSD3_LIBS)

test-login: tests/test-login.c libbsd3-login.a
	$(CC) $(CPPFLAGS) $(BSD3_CPPFLAGS) $(CFLAGS) $(BSD3_CFLAGS) $(LDFLAGS) -o $@ tests/test-login.c libbsd3-login.a $(BSD3_LIBS)

test: all mock-ck test-login
	dbus-run-session -- sh tests/run.sh

install: all
	$(INSTALL) -d $(DESTDIR)$(PREFIX)/sbin $(DESTDIR)$(PREFIX)/lib $(DESTDIR)$(PREFIX)/include/bsd3 $(DESTDIR)$(PREFIX)/share/man/man8
	$(INSTALL) -m 755 bsddd $(DESTDIR)$(PREFIX)/sbin/bsddd
	$(INSTALL) -m 644 libbsd3-login.a $(DESTDIR)$(PREFIX)/lib/libbsd3-login.a
	$(INSTALL) -m 644 include/bsd3/sd-login.h $(DESTDIR)$(PREFIX)/include/bsd3/sd-login.h
	$(INSTALL) -m 644 man/bsddd.8 $(DESTDIR)$(PREFIX)/share/man/man8/bsddd.8

clean:
	rm -f bsddd login.o libbsd3-login.a mock-ck test-login
