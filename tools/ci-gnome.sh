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
work_root=$(mktemp -d /var/tmp/bsd3-gnome.XXXXXXXX)
# Explicit ordinary identity, even if the executor launches commands as root.
test_user=circleci
test_uid=$(id -u "$test_user")
[ "$test_uid" -gt 0 ]
printf '%s\n' "$test_uid" >dist/gnome/test-uid.txt
mkdir -p "$work_root/tests"
cp test-native-session test-native-window "$work_root/"
cp tests/native-gnome-session.sh "$work_root/tests/"
sudo chown -R "$test_user" "$work_root"
cleanup() {
    result=$?
    trap - EXIT HUP INT TERM
    [ "$service_started" = no ] || sudo service bsddd onestop || :
    if [ -d "$work_root/dist/gnome" ]; then
        sudo cp -R "$work_root/dist/gnome/." dist/gnome/ || :
    fi
    case "$work_root" in /var/tmp/bsd3-gnome.????????) sudo rm -rf "$work_root" ;; esac
    exit "$result"
}
trap cleanup EXIT
trap 'exit 129' HUP
trap 'exit 130' INT
trap 'exit 143' TERM
sudo service dbus onestatus >/dev/null 2>&1 || sudo service dbus onestart
service_started=yes
sudo service bsddd onestart
# Load policies installed with GNOME after the bus was first started.
sudo gdbus call --system --dest org.freedesktop.DBus \
    --object-path /org/freedesktop/DBus --method org.freedesktop.DBus.ReloadConfig >/dev/null
# Empty session inventory is valid. Activate without treating NoSessions as failure.
timeout -k 1 15 gdbus call --system --dest org.freedesktop.DBus \
    --object-path /org/freedesktop/DBus --method org.freedesktop.DBus.StartServiceByName \
    org.freedesktop.ConsoleKit 0 >/dev/null
sudo -H -u "$test_user" /bin/sh -c     'cd "$1" && exec timeout -k 5 120 sh tests/native-gnome-session.sh' sh "$work_root"
