#!/bin/sh
# SPDX-License-Identifier: BSD-2-Clause
# Existing ordinary account, disposable KVM only. No persistent service enablement.
set -eu
export PATH=/usr/local/sbin:/usr/local/bin:/sbin:/bin:/usr/sbin:/usr/bin
unset DBUS_SYSTEM_BUS_ADDRESS DBUS_SESSION_BUS_ADDRESS XDG_SESSION_COOKIE XDG_RUNTIME_DIR
[ "$(uname -s)" = FreeBSD ] && [ "$(id -u)" -eq 0 ] || exit 1
[ "$(sysctl -n kern.vm_guest)" = kvm ] || exit 1
[ "$#" -eq 2 ]
repo_root=$1
test_user=$2
case "$test_user" in *[!A-Za-z0-9_-]*|'') exit 1 ;; esac
[ "$(id -u "$test_user")" -gt 0 ]
test_home=$(getent passwd "$test_user" | cut -d: -f6)
case "$test_home" in /*) ;; *) exit 1 ;; esac
cd "$repo_root"
[ "$(stat -f %u .)" -eq 0 ] || { echo "Root-owned source snapshot required" >&2; exit 1; }
[ ! -e /usr/local/etc/sddm.conf.d/99-bsd3-test.conf ]
[ ! -e /usr/local/share/wayland-sessions/bsd3-ci-probe.desktop ]
[ ! -e "$test_home/.bsd3-sddm-probe" ]
! pgrep -x sddm >/dev/null
mkdir -p dist/sddm
root_dir=$(mktemp -d /var/tmp/bsd3-sddm-root.XXXXXXXX)
user_dir=$(mktemp -d /var/tmp/bsd3-sddm-user.XXXXXXXX)
guard_pid=
service_started=no
config_created=no
result_created=no
cleanup() {
    result=$?
    trap - EXIT HUP INT TERM
    if [ -n "$guard_pid" ]; then
        kill -TERM "$guard_pid" 2>/dev/null || :
        wait "$guard_pid" || :
    fi
    # Preserve evidence before removing only this test's temporary inputs.
    if [ "$result" -eq 0 ]; then
        printf '%s\n' 'ACCEPTANCE_PASS_WITH_REQUESTED_GREETER_STOP' >dist/sddm/capture-state.txt
    else
        printf '%s\n' 'FAILED_OR_PARTIAL: inspect each final barrier and exit status' >dist/sddm/capture-state.txt
    fi
    cp -R "$root_dir/." dist/sddm/ || :
    if [ "$result_created" = yes ]; then
        cp -R "$test_home/.bsd3-sddm-probe/." dist/sddm/ || :
        rm -rf "$test_home/.bsd3-sddm-probe"
    fi
    if [ -d "$user_dir/dist/gnome" ]; then
        mkdir -p dist/sddm/gnome
        cp -R "$user_dir/dist/gnome/." dist/sddm/gnome/ || :
    fi
    [ "$config_created" = no ] || rm -f /usr/local/etc/sddm.conf.d/99-bsd3-test.conf /usr/local/share/wayland-sessions/bsd3-ci-probe.desktop
    [ "$service_started" = no ] || service bsddd onestop || :
    rm -rf "$root_dir" "$user_dir"
    exit "$result"
}
trap cleanup EXIT
trap 'exit 129' HUP
trap 'exit 130' INT
trap 'exit 143' TERM
{
    freebsd-version -kru
    uname -v
    cc --version
    pkg query '%n %v %q' bsd-cubed consolekit2 mutter sddm xorg-server gnome-shell gnome-session
    if git rev-parse --verify HEAD >/dev/null 2>&1; then git rev-parse HEAD; git status --short;
    else printf '%s\n' 'Source snapshot without Git metadata; preserve archive checksum separately'; fi
    sha256 tools/test-native-sddm.sh tests/native-sddm-client.c tests/sddm-gnome-probe.sh
    sha256 /usr/local/sbin/bsd3-vt-guard /usr/local/bin/bsd3-session /usr/local/libexec/bsd3-sddm-session
} >"$root_dir/provenance.txt"
# Positive streaming-readiness check using a still-running immutable producer.
printf '%s\n' '#!/bin/sh' 'echo producer-start' 'sleep 5' 'echo producer-finished' >"$root_dir/producer.sh"
chmod 400 "$root_dir/producer.sh"
sha256 "$root_dir/producer.sh" >"$root_dir/producer.sha256"
/usr/local/sbin/bsd3-vt-guard 20 "$root_dir/streaming" -- /bin/sh "$root_dir/producer.sh" &
guard_pid=$!
tries=0
while :; do
    if [ -f "$root_dir/streaming/guard-ready.txt" ] && [ -f "$root_dir/streaming/controller-ready.txt" ] &&
        grep -q producer-start "$root_dir/streaming/workload.log"; then break; fi
    tries=$((tries+1)); [ "$tries" -lt 30 ]; sleep 0.1
done
kill -0 "$guard_pid"
! grep -q producer-finished "$root_dir/streaming/workload.log"
printf '%s\n' 'PASS: both readiness markers and streaming output observed while producer alive' >"$root_dir/streaming-observer.txt"
wait "$guard_pid"
guard_pid=
[ "$(cat "$root_dir/streaming/complete.txt")" = PASS ]
# A deadline is an assisted failure even when console restoration succeeds.
if /usr/local/sbin/bsd3-vt-guard 1 "$root_dir/timeout" -- /bin/sleep 30; then exit 1; fi
[ "$(cat "$root_dir/timeout/command-exit.txt")" = 124 ]
grep -q '^recovery=PASS$' "$root_dir/timeout/result.txt"
[ "$(cat "$root_dir/timeout/complete.txt")" = FAIL_OR_ASSISTED ]
mkdir -p "$user_dir/tests"
cc -std=c11 -Wall -Wextra -Werror $(pkg-config --cflags bsd3-login) \
    -o "$user_dir/test-native-sddm-client" tests/native-sddm-client.c $(pkg-config --libs bsd3-login)
cc -std=c11 -Wall -Wextra -Werror $(pkg-config --cflags bsd3-login) \
    -o "$user_dir/test-native-session" tests/native-session.c $(pkg-config --libs bsd3-login)
cc -std=c11 -Wall -Wextra -Werror $(pkg-config --cflags gtk+-3.0) \
    -o "$user_dir/test-native-window" tests/native-window.c $(pkg-config --libs gtk+-3.0)
cp tests/native-gnome-session.sh tests/sddm-gnome-probe.sh "$user_dir/tests/"
chmod 755 "$user_dir/tests/sddm-gnome-probe.sh"
chown -R "$test_user" "$user_dir"
env BSD3_TEST_RESULT="$test_home/.bsd3-sddm-probe" su -m "$test_user" -c 'exec /bin/mkdir -m 700 "$BSD3_TEST_RESULT"'
result_created=yes
cat >"$root_dir/xorg.conf" <<'XORG'
Section "Device"
 Identifier "VM VGA"
 Driver "vesa"
EndSection
Section "Screen"
 Identifier "VM screen"
 Device "VM VGA"
 DefaultDepth 24
 SubSection "Display"
  Depth 24
  Modes "1024x768"
 EndSubSection
EndSection
XORG
mkdir -p /usr/local/etc/sddm.conf.d
config_created=yes
cat >/usr/local/etc/sddm.conf.d/99-bsd3-test.conf <<CONFIG
[General]
DisplayServer=x11
[X11]
ServerPath=/usr/local/bin/Xorg
ServerArguments=-nolisten tcp -background none -noreset -config $root_dir/xorg.conf
[Wayland]
SessionCommand=/usr/local/libexec/bsd3-sddm-session
SessionLogFile=.bsd3-sddm-probe/session.log
[Autologin]
User=$test_user
Session=bsd3-ci-probe.desktop
Relogin=false
[Users]
ReuseSession=false
RememberLastSession=false
CONFIG
cat >/usr/local/share/wayland-sessions/bsd3-ci-probe.desktop <<DESKTOP
[Desktop Entry]
Name=BSD3 disposable CI session probe
Exec=$user_dir/tests/sddm-gnome-probe.sh
Type=Application
DesktopNames=GNOME
DESKTOP
chmod 600 /usr/local/etc/sddm.conf.d/99-bsd3-test.conf
if ! service bsddd onestatus >/dev/null 2>&1; then
    service bsddd onestart
    service_started=yes
fi
gdbus call --system --dest org.freedesktop.DBus --object-path /org/freedesktop/DBus \
    --method org.freedesktop.DBus.ReloadConfig >/dev/null
gdbus call --system --dest org.freedesktop.login1 --object-path /org/freedesktop/login1 \
    --method org.freedesktop.login1.Manager.ListSessions >"$root_dir/sessions-before.txt"
/usr/local/sbin/bsd3-vt-guard 180 "$root_dir/desktop" -- /usr/local/bin/sddm &
guard_pid=$!
tries=0
while [ ! -f "$test_home/.bsd3-sddm-probe/gnome-complete.txt" ]; do
    [ ! -f "$root_dir/desktop/complete.txt" ]
    kill -0 "$guard_pid"
    tries=$((tries+1)); [ "$tries" -lt 150 ]; sleep 1
done
# Wait for the production launcher itself to complete cleanup, not only its child.
tries=0
while :; do
    gdbus call --system --dest org.freedesktop.login1 --object-path /org/freedesktop/login1 \
        --method org.freedesktop.login1.Manager.ListSessions >"$root_dir/sessions-after.txt"
    if cmp -s "$root_dir/sessions-before.txt" "$root_dir/sessions-after.txt" &&
        grep -q '^bsd3-session: exit status=0$' "$test_home/.bsd3-sddm-probe/session.log"; then break; fi
    [ ! -f "$root_dir/desktop/complete.txt" ]
    tries=$((tries+1)); [ "$tries" -lt 20 ]; sleep 1
done
cat "$test_home/.bsd3-sddm-probe/result.txt" "$test_home/.bsd3-sddm-probe/gnome-complete.txt"
# GNOME already logged out normally. Request stop of the still-running greeter.
kill -TERM "$guard_pid"
if wait "$guard_pid"; then exit 1; fi
guard_pid=
[ -f "$root_dir/desktop/complete.txt" ]
grep -q '^recovery=PASS$' "$root_dir/desktop/result.txt"
[ "$(cat "$root_dir/desktop/complete.txt")" = FAIL_OR_ASSISTED ]
grep -q '^requested_signal=15$' "$root_dir/desktop/result.txt"
cat "$root_dir/desktop/result.txt"
printf '%s\n' 'PASS: real SDDM local registration, full GNOME headless window/logout, requested greeter shutdown and verified console recovery; physical DRM/input not verified'
