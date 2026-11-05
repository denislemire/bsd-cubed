#!/bin/sh
# SPDX-License-Identifier: BSD-2-Clause
set -eu
export PATH=/usr/local/sbin:/usr/local/bin:/sbin:/bin:/usr/sbin:/usr/bin
[ "$(uname -s)" = FreeBSD ] && [ "${CIRCLECI:-}" = true ] || exit 1
unset DBUS_SYSTEM_BUS_ADDRESS DBUS_SESSION_BUS_ADDRESS XDG_SESSION_COOKIE XDG_RUNTIME_DIR
work_root=$(mktemp -d)
service_started=no
finish() {
    result=$?
    trap - EXIT HUP INT TERM
    [ "$service_started" = no ] || sudo service bsddd onestop || :
    rm -rf "$work_root"
    exit "$result"
}
trap finish EXIT
trap 'exit 129' HUP
trap 'exit 130' INT
trap 'exit 143' TERM
mkdir -p dist/consolekit
cc -std=c11 -Wall -Wextra -Werror $(pkg-config --cflags bsd3-login) \
    -o "$work_root/test-native-session" tests/native-session.c $(pkg-config --libs bsd3-login)
# The parent temporary directory starts private; allow only this existing test user.
cc -std=c11 -Wall -Wextra -Werror $(pkg-config --cflags bsd3-login) \
    -o "$work_root/test-native-device" tests/native-device.c $(pkg-config --libs bsd3-login)
sudo chown circleci "$work_root" "$work_root/test-native-session" "$work_root/test-native-device"
[ "$(id -u circleci)" -gt 0 ]
service_started=yes
sudo service bsddd onestart
if ! sudo -H -u circleci timeout -k 2 30 "$work_root/test-native-session" \
    "$work_root/test-native-session" --verify >dist/consolekit/session-regression.txt 2>&1; then
    cat dist/consolekit/session-regression.txt
    exit 1
fi
cat dist/consolekit/session-regression.txt
if ! sudo -H -u circleci timeout -k 2 30 "$work_root/test-native-session" \
    "$work_root/test-native-device" /dev/input/event0 >dist/consolekit/device-regression.txt 2>&1; then
    cat dist/consolekit/device-regression.txt
    exit 1
fi
cat dist/consolekit/device-regression.txt
