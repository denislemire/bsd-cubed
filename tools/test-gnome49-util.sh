#!/bin/sh
# SPDX-License-Identifier: BSD-2-Clause
# Exercise the actual pinned GPL consumer, never copied into the BSD library.
set -eu
[ "$#" -eq 1 ] || { echo "usage: $0 /path/to/gnome-session-49-checkout" >&2; exit 64; }
source_root=$(CDPATH= cd -- "$1" && pwd)
repo_root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
[ "$(git -C "$source_root" rev-parse HEAD)" = 776dc42864ea233f732e0b5b72cc08b1d8664fdb ]
git -C "$source_root" diff --quiet
build_root=$(mktemp -d)
trap 'rm -rf "$build_root"' EXIT HUP INT TERM
git -C "$source_root" archive HEAD | tar -xf - -C "$build_root"
cd "$build_root"
patch -p1 < "$repo_root/packaging/freebsd/gnome-session49/native-manager.patch"
cat > config.h <<'EOF'
#define HAVE_BSD3_USER_SESSION 1
#define GETTEXT_PACKAGE "gnome-session-49"
EOF
# Upstream source keeps its own warning policy; original BSD3 code builds with
# -Wall -Wextra -Werror separately. No fake systemd header or metadata is used.
cc -std=gnu11 -Wall -Wextra -Werror=implicit-function-declaration \
    -I. -Ignome-session -I"$repo_root/include" \
    $(pkg-config --cflags gio-unix-2.0) \
    "$repo_root/tests/gnome49-util-client.c" gnome-session/gsm-util.c \
    "$repo_root/libbsd3-user-session.a" -pthread \
    $(pkg-config --libs gio-unix-2.0) -o "$build_root/gnome49-util-client"
cd "$repo_root"
dbus-run-session --config-file=tests/dbus.conf -- "${PYTHON:-python3}" tests/test-user-session.py \
    --dinitctl "${DINITCTL:-/usr/local/sbin/dinitctl}" \
    --gnome49-client "$build_root/gnome49-util-client"
