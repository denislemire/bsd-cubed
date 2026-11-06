#!/bin/sh
# SPDX-License-Identifier: BSD-2-Clause
# Reuse an exact, unchanged ports framework inside one serial CI candidate.
set -eu
[ "$#" -eq 2 ] || exit 64
ports_root=$1
revision=$2
case "$revision" in *[!0-9a-f]*|'') exit 64;; esac
[ "${#revision}" -eq 40 ] || exit 64
if [ ! -e "$ports_root/.git" ]; then
    git init -q "$ports_root"
    git -C "$ports_root" remote add origin https://git.freebsd.org/ports.git
    git -C "$ports_root" fetch --depth 1 origin "$revision"
    git -C "$ports_root" checkout --detach FETCH_HEAD
fi
[ "$(git -C "$ports_root" rev-parse HEAD)" = "$revision" ]
# New local origins may be added for SDK dependency enumeration; tracked
# framework changes must never be silently accepted as the pinned revision.
git -C "$ports_root" diff --quiet HEAD --
[ -f "$ports_root/Mk/bsd.port.mk" ]
