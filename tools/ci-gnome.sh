#!/bin/sh
# SPDX-License-Identifier: BSD-2-Clause
set -eu
export PATH=/usr/local/sbin:/usr/local/bin:/sbin:/bin:/usr/sbin:/usr/bin
[ "$(uname -s)" = FreeBSD ] && [ "${CIRCLECI:-}" = true ] || exit 1
unset DBUS_SYSTEM_BUS_ADDRESS DBUS_SESSION_BUS_ADDRESS XDG_SESSION_COOKIE XDG_RUNTIME_DIR
mkdir -p dist/gnome
# No desktop or boot service is enabled: package scripts run only in this CI VM.
sudo pkg install -y gnome-shell gnome-session gnome-settings-daemon
# Dependencies must not silently replace the exact native candidate.
set -- dist/mutter/packages/All/mutter-*.pkg
[ "$#" -eq 1 ] && [ -f "$1" ]
sudo pkg add -f "$1"
pkg query '%n %v %q' gnome-shell gnome-session gnome-settings-daemon mutter consolekit2 bsd-cubed >dist/gnome/packages.txt
cc -std=c11 -Wall -Wextra -Werror $(pkg-config --cflags bsd3-login) \
    -o test-native-session tests/native-session.c $(pkg-config --libs bsd3-login)
cc -std=c11 -Wall -Wextra -Werror $(pkg-config --cflags gtk+-3.0) \
    -o test-native-window tests/native-window.c $(pkg-config --libs gtk+-3.0)
service_started=no
cleanup() {
    result=$?
    trap - EXIT HUP INT TERM
    [ "$service_started" = no ] || sudo service bsddd onestop || :
    exit "$result"
}
trap cleanup EXIT
trap 'exit 129' HUP
trap 'exit 130' INT
trap 'exit 143' TERM
sudo service dbus onestatus >/dev/null 2>&1 || sudo service dbus onestart
service_started=yes
sudo service bsddd onestart
# Activation must reach the real installed CK daemon, not a private-bus fixture.
timeout -k 1 15 gdbus call --system --dest org.freedesktop.ConsoleKit \
    --object-path /org/freedesktop/ConsoleKit/Manager \
    --method org.freedesktop.ConsoleKit.Manager.GetSessions >/dev/null
timeout -k 5 120 sh tests/native-gnome-session.sh
