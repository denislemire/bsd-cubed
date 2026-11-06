# SPDX-License-Identifier: BSD-2-Clause
# Portable make syntax: works with FreeBSD make and GNU make.
PREFIX ?= /usr/local
LOCALBASE ?= /usr/local
CC ?= cc
AR ?= ar
PKG_CONFIG ?= pkg-config
PYTHON ?= python3
CFLAGS ?= -O2
BSD3_CFLAGS = -std=c11 -Wall -Wextra -Werror
BSD3_CPPFLAGS = -Iinclude `$(PKG_CONFIG) --cflags gio-unix-2.0`
BSD3_LIBS = `$(PKG_CONFIG) --libs gio-unix-2.0`
INSTALL ?= install
DINIT ?= $(LOCALBASE)/sbin/dinit
DINITCTL ?= $(LOCALBASE)/sbin/dinitctl

all: bsddd bsd3-vt-guard bsd3-session bsd3-sddm-session libbsd3-login.a bsd3-login.pc

# Explicit opt-in foundation; the GNOME 47/default install is unchanged.
user-session: bsd3-user-session libbsd3-user-session.a bsd3-user-session.pc

bsd3-user-session: src/user-session.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(BSD3_CFLAGS) -DBSD3_DINIT='"$(DINIT)"' -DBSD3_DINITCTL='"$(DINITCTL)"' $(LDFLAGS) -o $@ src/user-session.c

user-session-api.o: src/user-session-api.c include/bsd3/user-session.h
	$(CC) $(CPPFLAGS) -Iinclude $(CFLAGS) $(BSD3_CFLAGS) -pthread -fPIC -DBSD3_DINITCTL='"$(DINITCTL)"' -c src/user-session-api.c -o $@

libbsd3-user-session.a: user-session-api.o
	$(AR) rcs $@ user-session-api.o

bsd3-user-session.pc: packaging/bsd3-user-session.pc.in Makefile FORCE
	sed 's|@PREFIX@|$(PREFIX)|g' packaging/bsd3-user-session.pc.in > $@

test-user-session-client: tests/user-session-client.c libbsd3-user-session.a
	$(CC) $(CPPFLAGS) -Iinclude $(CFLAGS) $(BSD3_CFLAGS) -pthread $(LDFLAGS) -o $@ tests/user-session-client.c libbsd3-user-session.a

test-user-session: user-session test-user-session-client
	$(PYTHON) tests/test-user-session.py --dinitctl $(DINITCTL)

install-user-session: user-session
	$(INSTALL) -d $(DESTDIR)$(PREFIX)/bin $(DESTDIR)$(PREFIX)/lib $(DESTDIR)$(PREFIX)/include/bsd3 $(DESTDIR)$(PREFIX)/libdata/pkgconfig $(DESTDIR)$(PREFIX)/share/man/man1
	$(INSTALL) -m 755 bsd3-user-session $(DESTDIR)$(PREFIX)/bin/bsd3-user-session
	$(INSTALL) -m 644 libbsd3-user-session.a $(DESTDIR)$(PREFIX)/lib/libbsd3-user-session.a
	$(INSTALL) -m 644 include/bsd3/user-session.h $(DESTDIR)$(PREFIX)/include/bsd3/user-session.h
	$(INSTALL) -m 644 bsd3-user-session.pc $(DESTDIR)$(PREFIX)/libdata/pkgconfig/bsd3-user-session.pc
	$(INSTALL) -m 644 man/bsd3-user-session.1 $(DESTDIR)$(PREFIX)/share/man/man1/bsd3-user-session.1

login.o: src/login.c include/bsd3/sd-login.h
	$(CC) $(CPPFLAGS) $(BSD3_CPPFLAGS) $(CFLAGS) $(BSD3_CFLAGS) -fPIC -c src/login.c -o $@

libbsd3-login.a: login.o
	$(AR) rcs $@ login.o

bsddd: src/bsddd.c src/control.c src/control.h src/seats.c src/seats.h libbsd3-login.a
	$(CC) $(CPPFLAGS) $(BSD3_CPPFLAGS) $(CFLAGS) $(BSD3_CFLAGS) $(LDFLAGS) -o $@ src/bsddd.c src/control.c src/seats.c libbsd3-login.a $(BSD3_LIBS)

bsd3-vt-guard: src/vt-guard.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(BSD3_CFLAGS) $(LDFLAGS) -o $@ src/vt-guard.c

bsd3-session: src/session.c
	$(CC) $(CPPFLAGS) $(BSD3_CPPFLAGS) $(CFLAGS) $(BSD3_CFLAGS) $(LDFLAGS) -o $@ src/session.c $(BSD3_LIBS)

bsd3-sddm-session: packaging/bsd3-sddm-session.in Makefile FORCE
	sed 's|@PREFIX@|$(PREFIX)|g; s|@LOCALBASE@|$(LOCALBASE)|g' packaging/bsd3-sddm-session.in > $@

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
	$(INSTALL) -d $(DESTDIR)$(PREFIX)/bin $(DESTDIR)$(PREFIX)/libexec $(DESTDIR)$(PREFIX)/share/examples/bsd-cubed $(DESTDIR)$(PREFIX)/share/man/man1
	$(INSTALL) -m 755 bsd3-session $(DESTDIR)$(PREFIX)/bin/bsd3-session
	$(INSTALL) -m 755 bsd3-sddm-session $(DESTDIR)$(PREFIX)/libexec/bsd3-sddm-session
	sed 's|@PREFIX@|$(PREFIX)|g' packaging/sddm-wayland.conf.in > $(DESTDIR)$(PREFIX)/share/examples/bsd-cubed/sddm-wayland.conf
	$(INSTALL) -m 755 bsd3-vt-guard $(DESTDIR)$(PREFIX)/sbin/bsd3-vt-guard
	$(INSTALL) -m 755 bsddd $(DESTDIR)$(PREFIX)/sbin/bsddd
	$(INSTALL) -m 644 bsd3-login.pc $(DESTDIR)$(PREFIX)/libdata/pkgconfig/bsd3-login.pc
	$(INSTALL) -m 644 libbsd3-login.a $(DESTDIR)$(PREFIX)/lib/libbsd3-login.a
	$(INSTALL) -m 644 include/bsd3/sd-login.h $(DESTDIR)$(PREFIX)/include/bsd3/sd-login.h
	$(INSTALL) -m 644 man/bsd3-session.1 $(DESTDIR)$(PREFIX)/share/man/man1/bsd3-session.1
	$(INSTALL) -m 644 man/bsd3-vt-guard.8 $(DESTDIR)$(PREFIX)/share/man/man8/bsd3-vt-guard.8
	$(INSTALL) -m 644 man/bsddd.8 $(DESTDIR)$(PREFIX)/share/man/man8/bsddd.8

clean:
	rm -f bsddd bsd3-vt-guard bsd3-session bsd3-sddm-session login.o libbsd3-login.a mock-ck test-login bsd3-login.pc
	rm -f bsd3-user-session user-session-api.o libbsd3-user-session.a bsd3-user-session.pc test-user-session-client
