#!/bin/sh
# SPDX-License-Identifier: BSD-2-Clause
# Ordinary-user, real CK temporary headless session; no VT/device/power actions.
set -eu
[ "$(uname -s)" = FreeBSD ] && [ "$(id -u)" -gt 0 ] || exit 1
[ "$#" -eq 1 ] || exit 64
unset DBUS_SYSTEM_BUS_ADDRESS DBUS_SESSION_BUS_ADDRESS XDG_SESSION_COOKIE XDG_RUNTIME_DIR
source_root=$(CDPATH= cd -- "$1" && pwd)
repo_root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
[ "$(git -C "$source_root" rev-parse HEAD)" = 776dc42864ea233f732e0b5b72cc08b1d8664fdb ]
git -C "$source_root" diff --quiet
work_root=$(mktemp -d)
trap 'rm -rf "$work_root"' EXIT
trap 'exit 129' HUP
trap 'exit 130' INT
trap 'exit 143' TERM
git -C "$source_root" archive HEAD | tar -xf - -C "$work_root"
cd "$work_root"
patch -p1 < "$repo_root/packaging/freebsd/gnome-session49/native-manager.patch"
printf '#define HAVE_BSD3_USER_SESSION 1\n#define GETTEXT_PACKAGE "gnome-session-49"\n' >config.h
cc -std=c11 -Wall -Wextra -Werror -I"$repo_root/src" -Ignome-session \
    $(pkg-config --cflags gio-unix-2.0) -c "$repo_root/src/gnome49-system.c" -o native.o
cc -std=gnu11 -Wall -Wextra -Wno-unused-parameter -I. -I"$repo_root/src" \
    -Ignome-session $(pkg-config --cflags gio-unix-2.0) gnome-session/gsm-system.c \
    "$repo_root/tests/native-gnome49-system.c" native.o \
    $(pkg-config --libs gio-unix-2.0) -o "$work_root/native-system"
cd "$repo_root"
make libbsd3-login.a
cc -Iinclude $(pkg-config --cflags gio-unix-2.0) tests/native-session.c \
    libbsd3-login.a $(pkg-config --libs gio-unix-2.0) -o "$work_root/native-session"
timeout -k 3 45 "$work_root/native-session" "$work_root/native-system"
