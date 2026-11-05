#!/bin/sh
# SPDX-License-Identifier: BSD-2-Clause
# Run on a disposable FreeBSD CircleCI machine, as its ordinary build user.
set -eu
export PATH=/usr/local/sbin:/usr/local/bin:/sbin:/bin:/usr/sbin:/usr/bin
[ "$(uname -s)" = FreeBSD ] || { echo 'FreeBSD required' >&2; exit 1; }
repo_root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
cd "$repo_root"
mkdir -p dist
# Artifacts describe the exact tested checkout, including whether it was dirty.
{
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
test -f "$stage_root/usr/local/include/bsd3/sd-login.h"
test -f "$stage_root/usr/local/share/man/man8/bsddd.8"
sh tools/ci-port.sh
