#!/bin/sh
# SPDX-License-Identifier: BSD-2-Clause
# Run on a disposable FreeBSD CI machine; record the actual executor UID.
set -eu
export PATH=/usr/local/sbin:/usr/local/bin:/sbin:/bin:/usr/sbin:/usr/bin
[ "$(uname -s)" = FreeBSD ] || { echo 'FreeBSD required' >&2; exit 1; }
repo_root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
cd "$repo_root"
mkdir -p dist
# Artifacts describe the exact tested checkout, including whether it was dirty.
{
    id -u
    freebsd-version -kru
    uname -v
    cc --version
    git rev-parse HEAD
    git status --short
    pkg query '%n %v' glib dbus consolekit2 pkgconf
} >dist/build-provenance.txt
make clean all test
stage_root=$(mktemp -d)
trap 'rm -rf "$stage_root"' EXIT HUP INT TERM
make DESTDIR="$stage_root" PREFIX=/usr/local install
test -x "$stage_root/usr/local/sbin/bsddd"
test -f "$stage_root/usr/local/lib/libbsd3-login.a"
test -f "$stage_root/usr/local/libdata/pkgconfig/bsd3-login.pc"
test -f "$stage_root/usr/local/include/bsd3/sd-login.h"
test -f "$stage_root/usr/local/share/man/man8/bsddd.8"
# Exercise relocated installed metadata and embedding the static archive in a DSO.
consumer_flags=$(PKG_CONFIG_PATH="$stage_root/usr/local/libdata/pkgconfig" \
    pkg-config --define-prefix --cflags --libs bsd3-login)
# pkg-config emits compiler arguments; intentional field splitting here.
cc -fPIC -shared -Wl,-z,defs -o "$stage_root/consumer.so" \
    tests/shared-consumer.c $consumer_flags
sh tools/ci-port.sh
