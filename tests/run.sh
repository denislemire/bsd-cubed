#!/bin/sh
# SPDX-License-Identifier: BSD-2-Clause
set -eu
# Every process sees the isolated test bus as its system bus. Never use the live bus.
: "${DBUS_SESSION_BUS_ADDRESS:?run this under dbus-run-session}"
export DBUS_SYSTEM_BUS_ADDRESS="$DBUS_SESSION_BUS_ADDRESS"
test_tmp=$(mktemp -d)
mock_pid=
daemon_pid=
cleanup() {
    test_status=$?
    trap - EXIT HUP INT TERM
    if [ "$test_status" -ne 0 ]; then
        cat "$test_tmp/mock.log" "$test_tmp/daemon.log" >&2 || :
    fi
    [ -z "$daemon_pid" ] || kill "$daemon_pid" 2>/dev/null || :
    [ -z "$mock_pid" ] || kill "$mock_pid" 2>/dev/null || :
    [ -z "$daemon_pid" ] || wait "$daemon_pid" || :
    [ -z "$mock_pid" ] || wait "$mock_pid" || :
    rm -rf "$test_tmp"
    exit "$test_status"
}
trap cleanup EXIT
trap 'exit 129' HUP
trap 'exit 130' INT
trap 'exit 143' TERM
./mock-ck >"$test_tmp/mock.log" 2>&1 &
mock_pid=$!
gdbus wait --system --timeout 5 org.freedesktop.ConsoleKit
./bsddd --foreground >"$test_tmp/daemon.log" 2>&1 &
daemon_pid=$!
gdbus wait --system --timeout 5 org.freedesktop.login1
./test-login
# A second instance must fail promptly and must not replace the first owner.
if ./bsddd --foreground >"$test_tmp/conflict.log" 2>&1; then
    echo 'FAIL: duplicate login1 owner succeeded' >&2
    exit 1
fi
kill "$daemon_pid"
wait "$daemon_pid"
daemon_pid=
# A backend outage must report failure rather than report an empty working backend.
kill "$mock_pid"
wait "$mock_pid"
mock_pid=
if ./bsddd --probe >"$test_tmp/probe.log" 2>&1; then
    echo 'FAIL: absent ConsoleKit reported success' >&2
    exit 1
fi
echo 'PASS: private-bus protocol, duplicate-owner, shutdown, missing-backend checks'
