#!/bin/sh
# SPDX-License-Identifier: BSD-2-Clause
# Test the actual licensed consumer parser and disabled/autostart boundary.
set -eu
[ "$(uname -s)" = FreeBSD ] || { echo 'native FreeBSD GDesktopAppInfo required' >&2; exit 1; }
[ "$#" -eq 1 ] || exit 64
source_root=$(CDPATH= cd -- "$1" && pwd)
repo_root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
[ "$(git -C "$source_root" rev-parse HEAD)" = 776dc42864ea233f732e0b5b72cc08b1d8664fdb ]
git -C "$source_root" diff --quiet
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT
trap 'exit 129' HUP
trap 'exit 130' INT
trap 'exit 143' TERM
git -C "$source_root" archive HEAD | tar -xf - -C "$work"
cd "$work"
patch -p1 < "$repo_root/packaging/freebsd/gnome-session49/native-manager.patch"
printf '#define HAVE_BSD3_USER_SESSION 1\n#define GETTEXT_PACKAGE "gnome-session-49"\n' >config.h
cc -std=gnu11 -DHAVE_CONFIG_H -Wall -Wextra -Werror=implicit-function-declaration \
    -I. -Ignome-session -I"$repo_root/include" $(pkg-config --cflags gio-unix-2.0) \
    "$repo_root/tests/gnome49-app.c" gnome-session/gsm-app.c gnome-session/gsm-util.c \
    "$repo_root/libbsd3-user-session.a" -pthread $(pkg-config --libs gio-unix-2.0) \
    -o "$work/test-app"
"$work/test-app"
