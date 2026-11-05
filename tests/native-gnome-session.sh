#!/bin/sh
# SPDX-License-Identifier: BSD-2-Clause
# Real GNOME47 session/services, headless only: no VT or device acceptance.
set -eu
[ "$(uname -s)" = FreeBSD ] || exit 1
[ "$(id -u)" -ne 0 ] || exit 1
if [ "${1:-}" != --session ]; then
    exec ./test-native-session dbus-run-session -- sh "$0" --session
fi
: "${XDG_SESSION_COOKIE:?real CK session required}"
: "${XDG_RUNTIME_DIR:?real CK runtime required}"
./test-native-session --verify
mkdir -p dist/gnome
profile=$(mktemp -d)
session_pid=
cleanup() {
    result=$?
    trap - EXIT HUP INT TERM
    if [ -n "$session_pid" ]; then
        gdbus call --session --dest org.gnome.SessionManager --object-path /org/gnome/SessionManager \
            --method org.gnome.SessionManager.Logout 1 >/dev/null 2>&1 || :
        tries=0
        while kill -0 "$session_pid" 2>/dev/null && [ "$tries" -lt 5 ]; do
            tries=$((tries + 1)); sleep 1
        done
        kill -TERM "$session_pid" 2>/dev/null || :
        sleep 1
        kill -KILL "$session_pid" 2>/dev/null || :
        wait "$session_pid" || :
    fi
    rm -rf "$profile"
    if [ "$result" -ne 0 ]; then tail -100 dist/gnome/session.log >&2 || :; fi
    exit "$result"
}
trap cleanup EXIT
trap 'exit 129' HUP
trap 'exit 130' INT
trap 'exit 143' TERM
export XDG_CONFIG_HOME="$profile/config" XDG_CACHE_HOME="$profile/cache" XDG_DATA_HOME="$profile/data"
export XDG_CURRENT_DESKTOP=GNOME XDG_SESSION_DESKTOP=gnome
export XDG_CONFIG_DIRS=/usr/local/etc/xdg:/etc/xdg XDG_DATA_DIRS=/usr/local/share:/usr/share
export WAYLAND_DISPLAY=bsd3-session LIBGL_ALWAYS_SOFTWARE=true GSETTINGS_BACKEND=memory
export LANG=C.UTF-8
unset DISPLAY SESSION_MANAGER
mkdir -p "$XDG_CONFIG_HOME" "$XDG_CACHE_HOME" "$XDG_DATA_HOME/applications"
# Retain the installed full GNOME RequiredComponents. Only adapt its Shell
# launch to the VM's headless renderer through a private desktop-file override.
sed 's|^Exec=.*|Exec=/usr/local/bin/gnome-shell --headless --wayland --no-x11 --virtual-monitor=1280x720 --wayland-display=bsd3-session|' \
    /usr/local/share/applications/org.gnome.Shell.desktop >"$XDG_DATA_HOME/applications/org.gnome.Shell.desktop"
dbus-update-activation-environment XDG_CONFIG_HOME XDG_CACHE_HOME XDG_DATA_HOME \
    XDG_CURRENT_DESKTOP XDG_SESSION_DESKTOP XDG_SESSION_TYPE XDG_RUNTIME_DIR \
    XDG_CONFIG_DIRS XDG_DATA_DIRS \
    WAYLAND_DISPLAY LIBGL_ALWAYS_SOFTWARE GSETTINGS_BACKEND LANG
/usr/local/libexec/gnome-session-binary --builtin --session=gnome --disable-acceleration-check \
    >dist/gnome/session.log 2>&1 &
session_pid=$!
tries=0
while :; do
    kill -0 "$session_pid"
    if running=$(gdbus call --session --dest org.gnome.SessionManager --object-path /org/gnome/SessionManager \
        --method org.gnome.SessionManager.IsSessionRunning 2>/dev/null) && [ "$running" = '(true,)' ]; then break; fi
    tries=$((tries + 1)); [ "$tries" -lt 60 ] || exit 1; sleep 1
done
timeout -k 1 10 gdbus call --session --dest org.gnome.Shell --object-path /org/gnome/Shell \
    --method org.freedesktop.DBus.Properties.Get org.gnome.Shell ShellVersion >dist/gnome/shell-version.txt
timeout -k 1 15 wayland-info >dist/gnome/wayland-info.txt 2>&1
GDK_BACKEND=wayland timeout -k 1 15 ./test-native-window >dist/gnome/window.txt 2>&1
sleep 3
kill -0 "$session_pid"
gdbus call --session --dest org.gnome.SessionManager --object-path /org/gnome/SessionManager \
    --method org.gnome.SessionManager.Logout 1 >dist/gnome/logout.txt
tries=0
while kill -0 "$session_pid" 2>/dev/null; do
    tries=$((tries + 1)); [ "$tries" -lt 15 ] || exit 1; sleep 1
done
wait "$session_pid"
session_pid=
printf '%s\n' 'PASS: full GNOME RequiredComponents session startup, real CK discovery, Wayland window, clean logout; DRM/VT NOT VERIFIED' | tee dist/gnome/result.txt
