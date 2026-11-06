#!/bin/sh
# SPDX-License-Identifier: BSD-2-Clause
# Only the disposable CI candidate, after old-desktop regression tests finish.
set -eu
export PATH=/usr/local/sbin:/usr/local/bin:/sbin:/bin:/usr/sbin:/usr/bin
[ "$(uname -s)" = FreeBSD ] && [ "$(id -u)" -eq 0 ] && [ "${CIRCLECI:-}" = true ]
! pgrep -x gdm >/dev/null
! pgrep -x sddm >/dev/null
! pgrep -x gnome-shell >/dev/null
for name in gjs gsettings-desktop-schemas; do
    set -- dist/gnome49-runtime/packages/All/"$name"-*.pkg
    [ "$#" -eq 1 ] && [ -f "$1" ]
    [ "$(pkg query -F "$1" '%n')" = "$name" ]
    pkg -o REPOS_DIR=/var/empty add -f "$1"
done
[ "$(pkg query '%v' gjs)" = 1.86.0 ]
[ "$(pkg query '%v' gsettings-desktop-schemas)" = 49.1 ]
