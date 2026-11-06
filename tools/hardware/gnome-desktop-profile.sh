#!/bin/sh
# SPDX-License-Identifier: BSD-2-Clause
# Persistent GNOME profile for the BSD3 desktop session.
set -eu
[ "$(id -u)" -ne 0 ]
: "${HOME:?home required}" "${XDG_RUNTIME_DIR:?verified runtime required}"
umask 077
profile="$HOME/.local/share/bsd3-gnome"
[ ! -L "$profile" ]
mkdir -p "$profile"
[ "$(stat -f %u "$profile")" -eq "$(id -u)" ]
chmod 700 "$profile"
export XDG_CONFIG_HOME="$profile/config" XDG_CACHE_HOME="$profile/cache"
export XDG_DATA_HOME="$profile/data" XDG_STATE_HOME="$profile/state"
export XDG_CURRENT_DESKTOP=GNOME XDG_SESSION_DESKTOP=gnome
export XDG_CONFIG_DIRS=/usr/local/etc/xdg:/etc/xdg
export XDG_DATA_DIRS=/usr/local/share:/usr/share
export XCURSOR_PATH=${XCURSOR_PATH:-/usr/local/share/icons:/usr/share/icons}
# Browser-only fallback: this installed WebKit/GTK combination fails graphics
# initialization and cannot import DMA-BUF textures. The compositor stays on GPU.
export WEBKIT_DISABLE_COMPOSITING_MODE=1
unset DISPLAY SESSION_MANAGER
mkdir -p "$XDG_CONFIG_HOME" "$XDG_CACHE_HOME" "$XDG_DATA_HOME" "$XDG_STATE_HOME"
/usr/local/bin/dbus-update-activation-environment XDG_CONFIG_HOME XDG_CACHE_HOME \
    XDG_DATA_HOME XDG_STATE_HOME XDG_CURRENT_DESKTOP XDG_SESSION_DESKTOP \
    XDG_SESSION_TYPE XDG_RUNTIME_DIR XDG_CONFIG_DIRS XDG_DATA_DIRS XCURSOR_PATH \
    WEBKIT_DISABLE_COMPOSITING_MODE
if [ ! -e "$profile/.initialized" ]; then
    if /usr/local/bin/gsettings set org.gnome.desktop.interface clock-format '12h' &&
        /usr/local/bin/gsettings set org.gnome.desktop.interface show-battery-percentage true; then
        : > "$profile/.initialized"
    else
        printf '%s\n' 'bsd3-gnome: initial desktop preferences could not be saved' >&2
    fi
fi
exec /usr/local/libexec/gnome-session-binary --builtin --session=gnome --disable-acceleration-check
