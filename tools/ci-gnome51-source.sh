#!/bin/sh
# SPDX-License-Identifier: BSD-2-Clause
set -eu
export PATH=/usr/local/sbin:/usr/local/bin:/sbin:/bin:/usr/sbin:/usr/bin
[ "$(id -u)" -ne 0 ] || exit 77
repo=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
patches="$repo/packaging/freebsd/gnome51"
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT
trap 'exit 129' HUP
trap 'exit 130' INT
trap 'exit 143' TERM

while read -r checksum source; do
    archive="$work/${source##*/}"
    curl --fail --silent --show-error --location --retry 2 --max-time 120 \
        "https://download.gnome.org/sources/$source" -o "$archive"
    [ "$(sha256 -q "$archive")" = "$checksum" ]
    tar -xf "$archive" -C "$work"
done <"$patches/sources.sha256"

apply_patch()
{
    printf 'Applying %s to %s\n' "$2" "$1"
    (cd "$work/$1" && patch -F 0 -p1 <"$patches/$2")
}
apply_patch mutter-51.1 mutter-login.patch
apply_patch mutter-51.1 mutter-freebsd.patch
apply_patch gnome-shell-51.1 shell-native.patch
apply_patch gnome-session-51.0 session-native.patch

# Exercise the real upstream class and existing ConsoleKit fixture on a private bus.
cd "$work/gnome-session-51.0"
printf '#define HAVE_BSD3_USER_SESSION 1\n#define GETTEXT_PACKAGE "gnome-session"\n' >config.h
cc -std=c11 -Wall -Wextra -Werror -DBSD3_GSM_SYSTEM_CLASS \
    -I"$repo/src" -Ignome-session $(pkg-config --cflags gio-unix-2.0) \
    -c "$repo/src/gnome49-system.c" -o "$work/adapter.o"
cc -std=gnu11 -Wall -Wextra -Wno-unused-parameter -DBSD3_GSM_SYSTEM_CLASS \
    -I. -I"$repo/src" -Ignome-session $(pkg-config --cflags gio-unix-2.0) \
    gnome-session/gsm-system.c "$repo/tests/test-gnome49-system.c" "$work/adapter.o" \
    $(pkg-config --libs gio-unix-2.0) -o "$work/test-system"
cc -std=gnu11 $(pkg-config --cflags gio-unix-2.0) "$repo/tests/mock-ck.c" \
    $(pkg-config --libs gio-unix-2.0) -o "$work/mock-ck"
dbus-run-session --config-file="$repo/tests/dbus.conf" -- /bin/sh -s "$work" <<'END'
set -eu
export DBUS_SYSTEM_BUS_ADDRESS="$DBUS_SESSION_BUS_ADDRESS"
"$1/mock-ck" &
mock_pid=$!
trap 'kill "$mock_pid" 2>/dev/null || :; wait "$mock_pid" || :' EXIT
gdbus wait --system --timeout 5 org.freedesktop.ConsoleKit
"$1/test-system"
END

# Compile the full manager against this checkout's user-session SDK. Keep the
# install image separate from both the host packages and the running desktop.
make -C "$repo" PREFIX="$work/sdk" install-user-session
cp "$repo/src/gnome49-system.c" "$repo/src/gnome49-system.h" gnome-session/
export PKG_CONFIG_PATH="$work/sdk/libdata/pkgconfig${PKG_CONFIG_PATH:+:$PKG_CONFIG_PATH}"
meson setup "$work/build" --prefix=/usr/local --wrap-mode=nodownload \
    -Dbsd3=true -Ddocbook=false -Dman=false
meson compile -C "$work/build" -j3
meson install -C "$work/build" --destdir "$work/stage"
test -x "$work/stage/usr/local/libexec/gnome-session-service"

mkdir -p "$repo/dist/gnome51"
(cd "$work/stage" && find . -type f | LC_ALL=C sort) \
    >"$repo/dist/gnome51/session-stage-manifest.txt"
