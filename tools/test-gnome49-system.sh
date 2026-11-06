#!/bin/sh
# SPDX-License-Identifier: BSD-2-Clause
# Original backend + actual GPL GsmSystem interface on a private bus.
set -eu
[ "$#" -eq 1 ] || exit 64
source_root=$(CDPATH= cd -- "$1" && pwd)
repo_root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
[ "$(git -C "$source_root" rev-parse HEAD)" = 776dc42864ea233f732e0b5b72cc08b1d8664fdb ]
git -C "$source_root" diff --quiet
build_root=$(mktemp -d)
trap 'rm -rf "$build_root"' EXIT
trap 'exit 129' HUP
trap 'exit 130' INT
trap 'exit 143' TERM
git -C "$source_root" archive HEAD | tar -xf - -C "$build_root"
cd "$build_root"
patch -p1 < "$repo_root/packaging/freebsd/gnome-session49/native-manager.patch"
printf '#define HAVE_BSD3_USER_SESSION 1\n#define GETTEXT_PACKAGE "gnome-session-49"\n' >config.h
cc -std=c11 -Wall -Wextra -Werror -I"$repo_root/src" -Ignome-session \
    $(pkg-config --cflags gio-unix-2.0) -c "$repo_root/src/gnome49-system.c" -o native.o
cc -std=gnu11 -Wall -Wextra -Wno-unused-parameter -I. -I"$repo_root/src" \
    -Ignome-session $(pkg-config --cflags gio-unix-2.0) \
    gnome-session/gsm-system.c "$repo_root/tests/test-gnome49-system.c" native.o \
    $(pkg-config --libs gio-unix-2.0) -o "$build_root/test-system"
cd "$repo_root"
make mock-ck
dbus-run-session --config-file=tests/dbus.conf -- sh -s "$repo_root" "$build_root/test-system" <<'END'
set -eu
export DBUS_SYSTEM_BUS_ADDRESS="$DBUS_SESSION_BUS_ADDRESS"
"$1/mock-ck" >/dev/null 2>&1 &
mock_pid=$!
trap 'kill "$mock_pid" 2>/dev/null || :; wait "$mock_pid" || :' EXIT
gdbus wait --system --timeout 5 org.freedesktop.ConsoleKit
"$2"
END
