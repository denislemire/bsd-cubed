#!/bin/sh
# SPDX-License-Identifier: BSD-2-Clause
set -eu
export PATH=/usr/local/sbin:/usr/local/bin:/sbin:/bin:/usr/sbin:/usr/bin
[ "$(uname -s)" = FreeBSD ] && [ "${CIRCLECI:-}" = true ] || exit 1
# This executor already runs as root; never add an administrative account grant.
[ "$(id -u)" -eq 0 ] || exit 1
unset DBUS_SYSTEM_BUS_ADDRESS DBUS_SESSION_BUS_ADDRESS XDG_SESSION_COOKIE XDG_RUNTIME_DIR
work_root=$(mktemp -d)
fixture_root=$(mktemp -d)
service_started=no
finish() {
    result=$?
    trap - EXIT HUP INT TERM
    if [ "$result" -ne 0 ]; then
        # These fixtures never log cookies. Surface the failed launcher phase
        # as well as retaining its full artifact; preserve the original status.
        for diagnostic in dist/consolekit/bootstrap-*.txt; do
            [ -f "$diagnostic" ] || continue
            printf '\nLauncher diagnostic: %s\n' "${diagnostic##*/}"
            tail -40 "$diagnostic" || :
        done
        if [ -r /var/log/messages ]; then
            tail -200 /var/log/messages | \
                grep -E 'console-kit-daemon.*(kvm_openfiles|get_kinfo_proc|kvm_getenvv)' \
                >dist/consolekit/process-environment-diagnostic.txt || :
            cat dist/consolekit/process-environment-diagnostic.txt || :
        fi
    fi
    [ "$service_started" = no ] || sudo service bsddd onestop || :
    rm -rf "$work_root" "$fixture_root"
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
# Keep the privileged launcher in a separate root-owned directory. Session and
# device requests still run after an irreversible drop to the existing test UID.
cc -std=c11 -Wall -Wextra -Werror -o "$fixture_root/test-native-vt-launch" \
    tests/native-vt-launch.c
cc -std=c11 -Wall -Wextra -Werror $(pkg-config --cflags bsd3-login) \
    -o "$work_root/test-native-vt-client" tests/native-vt-client.c $(pkg-config --libs bsd3-login)
cc -std=c11 -Wall -Wextra -Werror -o "$work_root/test-native-session-stop" tests/native-session-stop.c
cat >"$work_root/reuse-session" <<REUSE
#!/bin/sh
exec /usr/local/bin/bsd3-session -- "$work_root/test-native-vt-client"
REUSE
chmod 755 "$work_root/reuse-session"
# Complete all root writes before handing the private payload to the test UID.
sudo chown -R circleci "$work_root"
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
if ! sudo env BSD3_DISPOSABLE_VM_TEST=yes "$fixture_root/test-native-vt-launch" \
    circleci "$work_root/test-native-session" "$work_root/test-native-vt-client" \
    >dist/consolekit/vt-regression.txt 2>&1; then
    cat dist/consolekit/vt-regression.txt
    exit 1
fi
cat dist/consolekit/vt-regression.txt
# Test the installed production launcher on the same real VT, not only a harness.
sudo -H -u circleci gdbus call --system --dest org.freedesktop.login1 \
    --object-path /org/freedesktop/login1 --method org.freedesktop.login1.Manager.ListSessions \
    >"$fixture_root/sessions-before.txt"
if sudo -H -u circleci /usr/local/bin/bsd3-session -- /usr/bin/touch "$work_root/should-not-exist" \
    >dist/consolekit/bootstrap-nonconsole.txt 2>&1; then exit 1; fi
[ ! -e "$work_root/should-not-exist" ]
if sudo /usr/local/bin/bsd3-session -- /usr/bin/touch "$fixture_root/should-not-exist" \
    >dist/consolekit/bootstrap-root.txt 2>&1; then exit 1; fi
[ ! -e "$fixture_root/should-not-exist" ]
sudo env BSD3_DISPOSABLE_VM_TEST=yes "$fixture_root/test-native-vt-launch" circleci \
    /usr/local/bin/bsd3-session "$work_root/test-native-vt-client" -- \
    >dist/consolekit/bootstrap-vt.txt 2>&1
sudo env BSD3_DISPOSABLE_VM_TEST=yes "$fixture_root/test-native-vt-launch" circleci \
    /usr/local/bin/bsd3-session "$work_root/reuse-session" -- \
    >dist/consolekit/bootstrap-reuse.txt 2>&1
if sudo env XDG_SESSION_COOKIE=invalid-test-cookie BSD3_DISPOSABLE_VM_TEST=yes \
    "$fixture_root/test-native-vt-launch" circleci /usr/local/bin/bsd3-session \
    "$work_root/test-native-vt-client" -- >dist/consolekit/bootstrap-cookie.txt 2>&1; then exit 1; fi
grep -q 'GetSessionForCookie failed' dist/consolekit/bootstrap-cookie.txt
grep -q 'recovery=PASS scenario=FAIL' dist/consolekit/bootstrap-cookie.txt
if sudo env BSD3_DISPOSABLE_VM_TEST=yes "$fixture_root/test-native-vt-launch" circleci \
    /usr/local/bin/bsd3-session /nonexistent/bsd3-test-command -- \
    >dist/consolekit/bootstrap-spawn.txt 2>&1; then exit 1; fi
grep -q 'setup or child launch failed' dist/consolekit/bootstrap-spawn.txt
grep -q 'recovery=PASS scenario=FAIL' dist/consolekit/bootstrap-spawn.txt
if sudo env BSD3_DISPOSABLE_VM_TEST=yes "$fixture_root/test-native-vt-launch" circleci \
    /usr/local/bin/bsd3-session "$work_root/test-native-session-stop" -- \
    >dist/consolekit/bootstrap-stop.txt 2>&1; then exit 1; fi
grep -q 'recovery=PASS scenario=FAIL' dist/consolekit/bootstrap-stop.txt
detached_pid=$(sed -n 's/^BSD3_STOP:detached=\([0-9][0-9]*\)$/\1/p' dist/consolekit/bootstrap-stop.txt)
[ -n "$detached_pid" ]
! sudo kill -0 "$detached_pid" 2>/dev/null
sudo -H -u circleci gdbus call --system --dest org.freedesktop.login1 \
    --object-path /org/freedesktop/login1 --method org.freedesktop.login1.Manager.ListSessions \
    >"$fixture_root/sessions-after.txt"
cmp "$fixture_root/sessions-before.txt" "$fixture_root/sessions-after.txt"
printf '%s\n' 'PASS: production VT bootstrap, reuse, refusal, failure cleanup and detached descendant termination'
