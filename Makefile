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

all: bsddd libbsd3-login.a bsd3-login.pc

login.o: src/login.c include/bsd3/sd-login.h
	$(CC) $(CPPFLAGS) $(BSD3_CPPFLAGS) $(CFLAGS) $(BSD3_CFLAGS) -fPIC -c src/login.c -o $@

libbsd3-login.a: login.o
	$(AR) rcs $@ login.o

bsddd: src/bsddd.c src/control.c src/control.h libbsd3-login.a
	$(CC) $(CPPFLAGS) $(BSD3_CPPFLAGS) $(CFLAGS) $(BSD3_CFLAGS) $(LDFLAGS) -o $@ src/bsddd.c src/control.c libbsd3-login.a $(BSD3_LIBS)

mock-ck: tests/mock-ck.c
	$(CC) $(CPPFLAGS) $(BSD3_CPPFLAGS) $(CFLAGS) $(BSD3_CFLAGS) $(LDFLAGS) -o $@ tests/mock-ck.c $(BSD3_LIBS)

test-login: tests/test-login.c libbsd3-login.a
	$(CC) $(CPPFLAGS) $(BSD3_CPPFLAGS) $(CFLAGS) $(BSD3_CFLAGS) $(LDFLAGS) -o $@ tests/test-login.c libbsd3-login.a $(BSD3_LIBS)

test: all mock-ck test-login
	dbus-run-session --config-file=tests/dbus.conf -- sh tests/run.sh

FORCE:

bsd3-login.pc: packaging/bsd3-login.pc.in Makefile FORCE
	sed 's|@PREFIX@|$(PREFIX)|g' packaging/bsd3-login.pc.in > $@

install: all
	$(INSTALL) -d $(DESTDIR)$(PREFIX)/sbin $(DESTDIR)$(PREFIX)/lib $(DESTDIR)$(PREFIX)/include/bsd3 $(DESTDIR)$(PREFIX)/share/man/man8 $(DESTDIR)$(PREFIX)/libdata/pkgconfig
	$(INSTALL) -m 755 bsddd $(DESTDIR)$(PREFIX)/sbin/bsddd
	$(INSTALL) -m 644 bsd3-login.pc $(DESTDIR)$(PREFIX)/libdata/pkgconfig/bsd3-login.pc
	$(INSTALL) -m 644 libbsd3-login.a $(DESTDIR)$(PREFIX)/lib/libbsd3-login.a
	$(INSTALL) -m 644 include/bsd3/sd-login.h $(DESTDIR)$(PREFIX)/include/bsd3/sd-login.h
	$(INSTALL) -m 644 man/bsddd.8 $(DESTDIR)$(PREFIX)/share/man/man8/bsddd.8

clean:
	rm -f bsddd login.o libbsd3-login.a mock-ck test-login bsd3-login.pc
