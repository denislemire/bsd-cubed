#!/bin/sh
# SPDX-License-Identifier: BSD-2-Clause
# Separate disposable profile; normal GNOME components and physical renderer.
set -eu
[ "$(id -u)" -ne 0 ]
: "${XDG_RUNTIME_DIR:?verified runtime required}"
umask 077
profile=$(mktemp -d "$XDG_RUNTIME_DIR/bsd3-gnome-profile.XXXXXXXX")
trap 'rm -rf "$profile"' EXIT
export XDG_CONFIG_HOME="$profile/config" XDG_CACHE_HOME="$profile/cache"
export XDG_DATA_HOME="$profile/data" XDG_STATE_HOME="$profile/state"
export XDG_CURRENT_DESKTOP=GNOME XDG_SESSION_DESKTOP=gnome
export XDG_CONFIG_DIRS=/usr/local/etc/xdg:/etc/xdg
export XDG_DATA_DIRS=/usr/local/share:/usr/share
export XCURSOR_PATH=${XCURSOR_PATH:-/usr/local/share/icons:/usr/share/icons}
unset DISPLAY SESSION_MANAGER
mkdir -m 700 "$XDG_CONFIG_HOME" "$XDG_CACHE_HOME" "$XDG_DATA_HOME" "$XDG_STATE_HOME"
/usr/local/bin/dbus-update-activation-environment XDG_CONFIG_HOME XDG_CACHE_HOME \
    XDG_DATA_HOME XDG_STATE_HOME XDG_CURRENT_DESKTOP XDG_SESSION_DESKTOP \
    XDG_SESSION_TYPE XDG_RUNTIME_DIR XDG_CONFIG_DIRS XDG_DATA_DIRS XCURSOR_PATH
/usr/local/libexec/gnome-session-binary --builtin --session=gnome --disable-acceleration-check
