#!/bin/sh
# SPDX-License-Identifier: BSD-2-Clause
# Real system-bus and rc.subr lifecycle, confined to this disposable CI VM.
set -eu
export PATH=/usr/local/sbin:/usr/local/bin:/sbin:/bin:/usr/sbin:/usr/bin
[ "$(uname -s)" = FreeBSD ] && [ "${CIRCLECI:-}" = true ] || exit 1
unset DBUS_SYSTEM_BUS_ADDRESS DBUS_SESSION_BUS_ADDRESS
mkdir -p dist/service
id -u | tee dist/service/build-uid.txt
set -- dist/packages/All/bsd-cubed-*.pkg
[ "$#" -eq 1 ] && [ -f "$1" ]
sudo pkg add -f "$1"
sudo gdbus call --system --dest org.freedesktop.DBus --object-path /org/freedesktop/DBus \
    --method org.freedesktop.DBus.ReloadConfig >/dev/null
service_started=no
foreign_pid=
guard_root=
cleanup() {
    result=$?
    trap - EXIT HUP INT TERM
    [ "$service_started" = no ] || sudo service bsddd onestop || :
    case "$foreign_pid" in
        *[!0-9]*|'') ;;
        *) if /usr/local/sbin/bsddd --check-owner "$foreign_pid" >/dev/null 2>&1; then
               sudo kill -TERM "$foreign_pid" 2>/dev/null || :
           fi ;;
    esac
    if [ -n "$guard_root" ]; then
        case "$guard_root" in /var/run/bsddd-test.??????) sudo rm -rf "$guard_root" ;; esac
    fi
    exit "$result"
}
trap cleanup EXIT
trap 'exit 129' HUP
trap 'exit 130' INT
trap 'exit 143' TERM
sudo service dbus onestatus >/dev/null 2>&1 || sudo service dbus onestart
owner=$(gdbus call --system --dest org.freedesktop.DBus --object-path /org/freedesktop/DBus \
    --method org.freedesktop.DBus.NameHasOwner org.freedesktop.login1)
[ "$owner" = '(false,)' ] || { echo 'Unexpected login1 owner; leaving it untouched' >&2; exit 1; }
# Explicitly drop identity: the CI runner may itself execute as root.
# No account or additional administrative grant is created.
probe_uid=$(sudo -u nobody id -u)
[ "$probe_uid" -gt 0 ]
printf '%s\n' "$probe_uid" >dist/service/probe-uid.txt
set +e
sudo -u nobody timeout -k 1 5 /usr/local/sbin/bsddd --foreground >dist/service/unprivileged-owner.log 2>&1
result=$?
set -e
[ "$result" -eq 1 ] || { echo "Root-only bus ownership policy failed (exit $result, probe UID $probe_uid)" >&2; exit 1; }
for invocation in onestart quietstart faststart; do
    service_started=yes
    if [ "$invocation" = onestart ]; then
        sudo service bsddd onestart
    else
        sudo env bsddd_enable=YES /usr/local/etc/rc.d/bsddd "$invocation"
    fi
    sudo service bsddd onestatus
    pid=$(sudo cat /var/run/bsddd.pid)
    case "$pid" in *[!0-9]*|'') exit 1 ;; esac
    /usr/local/sbin/bsddd --check-owner "$pid"
    sudo service bsddd onestop
    service_started=no
    if sudo service bsddd onestatus; then
        echo 'Stopped service reported ready' >&2
        exit 1
    fi
done
# Hold the bus name with a separately managed process. The rc service must fail
# rather than accept NameHasOwner=true as evidence that its own child is ready.
guard_root=$(sudo mktemp -d /var/run/bsddd-test.XXXXXX)
case "$guard_root" in /var/run/bsddd-test.??????) ;; *) exit 1 ;; esac
sudo /usr/sbin/daemon -c -f -p "$guard_root/foreign.pid" -- /usr/local/sbin/bsddd --foreground
tries=0
while :; do
    if foreign_pid=$(sudo cat "$guard_root/foreign.pid" 2>/dev/null) && \
        /usr/local/sbin/bsddd --check-owner "$foreign_pid" >/dev/null 2>&1; then
        break
    fi
    tries=$((tries + 1))
    [ "$tries" -lt 10 ] || exit 1
    sleep 1
done
case "$foreign_pid" in *[!0-9]*|'') exit 1 ;; esac
if sudo service bsddd onestart >dist/service/foreign-owner.log 2>&1; then
    echo 'Service accepted an existing foreign owner' >&2
    exit 1
fi
/usr/local/sbin/bsddd --check-owner "$foreign_pid"
printf '%s\n' 'PASS: root-only policy, onestart/quietstart/faststart/status/stop, foreign-owner refusal' \
    | tee dist/service/result.txt
