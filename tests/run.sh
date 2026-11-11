#!/bin/sh
# SPDX-License-Identifier: BSD-2-Clause
set -eu
# Clients use the private session bus as their system bus. bsddd clears
# DBUS_SYSTEM_BUS_ADDRESS, so the harness redirects only its connect(2) of the
# compiled-in system bus path onto this same socket. A decoy bus receives the
# variable the daemon must ignore.
: "${DBUS_SESSION_BUS_ADDRESS:?run this under dbus-run-session}"
session_addr=${DBUS_SESSION_BUS_ADDRESS%%,*}
case $session_addr in
unix:path=/*) private_sock=${session_addr#unix:path=} ;;
*)
    echo 'FAIL: private bus is not a filesystem socket' >&2
    exit 1
    ;;
esac
export DBUS_SYSTEM_BUS_ADDRESS="$DBUS_SESSION_BUS_ADDRESS"
default_bus_path=$(./system-bus-path)
test_tmp=$(mktemp -d)
mock_pid=
daemon_pid=
decoy_pid=
monitor_pid=
preload=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)/hardening-preload.so
cleanup() {
    test_status=$?
    trap - EXIT HUP INT TERM
    if [ "$test_status" -ne 0 ]; then
        cat "$test_tmp/mock.log" "$test_tmp/daemon.log" "$test_tmp/decoy.log" \
            "$test_tmp/monitor.log" >&2 || :
    fi
    [ -z "$daemon_pid" ] || kill "$daemon_pid" 2>/dev/null || :
    [ -z "$mock_pid" ] || kill "$mock_pid" 2>/dev/null || :
    [ -z "$decoy_pid" ] || kill "$decoy_pid" 2>/dev/null || :
    [ -z "$monitor_pid" ] || kill "$monitor_pid" 2>/dev/null || :
    [ -z "$daemon_pid" ] || wait "$daemon_pid" || :
    [ -z "$mock_pid" ] || wait "$mock_pid" || :
    [ -z "$decoy_pid" ] || wait "$decoy_pid" || :
    [ -z "$monitor_pid" ] || wait "$monitor_pid" || :
    rm -rf "$test_tmp"
    exit "$test_status"
}
trap cleanup EXIT
trap 'exit 129' HUP
trap 'exit 130' INT
trap 'exit 143' TERM
cat >"$test_tmp/decoy.conf" <<EOF
<!DOCTYPE busconfig PUBLIC "-//freedesktop//DTD D-BUS Bus Configuration 1.0//EN"
 "http://www.freedesktop.org/standards/dbus/1.0/busconfig.dtd">
<busconfig>
  <type>session</type>
  <listen>unix:path=$test_tmp/decoy</listen>
  <auth>EXTERNAL</auth>
  <policy context="default">
    <allow own="*"/>
    <allow send_destination="*"/>
    <allow receive_sender="*"/>
  </policy>
</busconfig>
EOF
dbus-daemon --config-file="$test_tmp/decoy.conf" --nofork --nopidfile \
    >"$test_tmp/decoy.log" 2>&1 &
decoy_pid=$!
decoy_ready=0
i=0
while [ "$i" -lt 5 ]; do
    if DBUS_SYSTEM_BUS_ADDRESS="unix:path=$test_tmp/decoy" \
        gdbus call --system --dest org.freedesktop.DBus \
        --object-path /org/freedesktop/DBus --method org.freedesktop.DBus.GetId \
        >/dev/null 2>&1; then
        decoy_ready=1
        break
    fi
    i=$((i + 1))
    sleep 1
done
[ "$decoy_ready" -eq 1 ]
./test-hardening
dbus-monitor --system >"$test_tmp/monitor.log" 2>&1 &
monitor_pid=$!
./mock-ck >"$test_tmp/mock.log" 2>&1 &
mock_pid=$!
gdbus wait --system --timeout 5 org.freedesktop.ConsoleKit
env LD_PRELOAD="$preload${LD_PRELOAD:+:$LD_PRELOAD}" \
    DYLD_INSERT_LIBRARIES="$preload${DYLD_INSERT_LIBRARIES:+:$DYLD_INSERT_LIBRARIES}" \
    BSD3_REDIRECT_SYSTEM_BUS_FROM="$default_bus_path" \
    BSD3_REDIRECT_SYSTEM_BUS_TO="$private_sock" \
    DBUS_SYSTEM_BUS_ADDRESS="unix:path=$test_tmp/decoy" \
    ./bsddd --foreground >"$test_tmp/daemon.log" 2>&1 &
daemon_pid=$!
gdbus wait --system --timeout 5 org.freedesktop.login1
# login1 appeared on the redirected default socket. The variable still names
# the decoy; /proc and procstat keep the exec-time environment, so the decoy
# is the check that the daemon did not use it.
if DBUS_SYSTEM_BUS_ADDRESS="unix:path=$test_tmp/decoy" \
    gdbus wait --system --timeout 1 org.freedesktop.login1; then
    echo 'FAIL: daemon honored DBUS_SYSTEM_BUS_ADDRESS' >&2
    exit 1
fi
# --check-owner is a child of the test and also ignores the bus variable.
# Give it the same default-socket redirect. Clients still pass the private
# address explicitly, so their own connections are not rewritten.
export LD_PRELOAD="$preload${LD_PRELOAD:+:$LD_PRELOAD}"
export DYLD_INSERT_LIBRARIES="$preload${DYLD_INSERT_LIBRARIES:+:$DYLD_INSERT_LIBRARIES}"
export BSD3_REDIRECT_SYSTEM_BUS_FROM="$default_bus_path"
export BSD3_REDIRECT_SYSTEM_BUS_TO="$private_sock"
./test-login
if ! grep -q 'member=GetConnectionCredentials' "$test_tmp/monitor.log"; then
    echo 'FAIL: daemon did not call GetConnectionCredentials' >&2
    exit 1
fi
# A second instance must fail promptly and must not replace the first owner.
if env LD_PRELOAD="$preload${LD_PRELOAD:+:$LD_PRELOAD}" \
    DYLD_INSERT_LIBRARIES="$preload${DYLD_INSERT_LIBRARIES:+:$DYLD_INSERT_LIBRARIES}" \
    BSD3_REDIRECT_SYSTEM_BUS_FROM="$default_bus_path" \
    BSD3_REDIRECT_SYSTEM_BUS_TO="$private_sock" \
    DBUS_SYSTEM_BUS_ADDRESS="unix:path=$test_tmp/decoy" \
    ./bsddd --foreground >"$test_tmp/conflict.log" 2>&1; then
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
if env LD_PRELOAD="$preload${LD_PRELOAD:+:$LD_PRELOAD}" \
    DYLD_INSERT_LIBRARIES="$preload${DYLD_INSERT_LIBRARIES:+:$DYLD_INSERT_LIBRARIES}" \
    BSD3_REDIRECT_SYSTEM_BUS_FROM="$default_bus_path" \
    BSD3_REDIRECT_SYSTEM_BUS_TO="$private_sock" \
    DBUS_SYSTEM_BUS_ADDRESS="unix:path=$test_tmp/decoy" \
    ./bsddd --probe >"$test_tmp/probe.log" 2>&1; then
    echo 'FAIL: absent ConsoleKit reported success' >&2
    exit 1
fi
echo 'PASS: private-bus protocol, duplicate-owner, shutdown, missing-backend checks'
