#!/bin/sh
# SPDX-License-Identifier: BSD-2-Clause
# Unprivileged native-headless test. No package installation or host settings.
set -eu
export PATH=/usr/local/sbin:/usr/local/bin:/sbin:/bin:/usr/sbin:/usr/bin
[ "$(uname -s)" = FreeBSD ] || exit 1
if [ "${1:-}" != --session ]; then
    exec dbus-run-session -- sh "$0" --session
fi
mkdir -p dist/mutter
runtime=$(mktemp -d)
compositor_pid=
finish() {
    status=$?
    trap - EXIT HUP INT TERM
    if [ -n "$compositor_pid" ]; then
        kill "$compositor_pid" 2>/dev/null || :
        stop_tries=0
        while kill -0 "$compositor_pid" 2>/dev/null && [ "$stop_tries" -lt 3 ]; do
            stop_tries=$((stop_tries + 1))
            sleep 1
        done
        kill -KILL "$compositor_pid" 2>/dev/null || :
        wait "$compositor_pid" || :
    fi
    rm -rf "$runtime"
    if [ "$status" -ne 0 ]; then
        tail -100 dist/mutter/startup.log >&2 || :
    fi
    exit "$status"
}
trap finish EXIT
trap 'exit 129' HUP
trap 'exit 130' INT
trap 'exit 143' TERM
chmod 700 "$runtime"
export XDG_RUNTIME_DIR="$runtime" WAYLAND_DISPLAY=bsd3-smoke
export LIBGL_ALWAYS_SOFTWARE=true GSETTINGS_BACKEND=memory
export XDG_CONFIG_HOME="$runtime/config" XDG_CACHE_HOME="$runtime/cache"
export XDG_DATA_HOME="$runtime/data"
mkdir -p "$XDG_CONFIG_HOME" "$XDG_CACHE_HOME" "$XDG_DATA_HOME"
unset DISPLAY
"${MUTTER_BINARY:-mutter}" --headless --wayland --no-x11 --virtual-monitor=1280x720 \
    --wayland-display="$WAYLAND_DISPLAY" >dist/mutter/startup.log 2>&1 &
compositor_pid=$!
tries=0
while [ ! -S "$runtime/$WAYLAND_DISPLAY" ]; do
    kill -0 "$compositor_pid"
    tries=$((tries + 1))
    [ "$tries" -lt 30 ] || exit 1
    sleep 1
done
timeout 15 wayland-info >dist/mutter/wayland-info.txt 2>&1
kill -0 "$compositor_pid"
grep -F "interface: 'wl_compositor'" dist/mutter/wayland-info.txt
grep -F "interface: 'xdg_wm_base'" dist/mutter/wayland-info.txt
grep -F "interface: 'wl_output'" dist/mutter/wayland-info.txt
printf '%s\n' 'Native headless compositor accepts a Wayland client. Device/VT/GNOME session: NOT VERIFIED.'
