#!/bin/sh
# SPDX-License-Identifier: BSD-2-Clause
# Final sequential candidate consumer build/test on the isolated CI VM only.
set -eu
export PATH=/usr/local/sbin:/usr/local/bin:/sbin:/bin:/usr/sbin:/usr/bin
[ "$(uname -s)" = FreeBSD ] && [ "$(id -u)" -eq 0 ] && [ "${CIRCLECI:-}" = true ] || exit 1
repo_root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
cd "$repo_root"
mkdir -p dist/session49/packages
work_root=$(mktemp -d)
ports_root=${BSD3_CI_PORTS_ROOT:-"$work_root/ports"}
port="$work_root/overlay/x11/gnome-session"
inhibitor_pid=
finish() {
    status=$?
    trap - EXIT HUP INT TERM
    if [ -n "$inhibitor_pid" ]; then
        kill -TERM "$inhibitor_pid" 2>/dev/null || :
        wait "$inhibitor_pid" || :
    fi
    if [ -d "$work_root/runtime/dist" ]; then cp -R "$work_root/runtime/dist" dist/session49/runtime; fi
    if [ -d "$work_root/work" ]; then
        find "$work_root/work" -name meson-log.txt -exec cp {} dist/session49/meson-log.txt \;
    fi
    rm -rf "$work_root"
    exit "$status"
}
trap finish EXIT
trap 'exit 129' HUP
trap 'exit 130' INT
trap 'exit 143' TERM
cc -std=c11 -Wall -Wextra -Werror $(pkg-config --cflags gio-unix-2.0) \
    tests/native-inhibit.c -o "$work_root/native-inhibit" $(pkg-config --libs gio-unix-2.0)
"$work_root/native-inhibit" 1800 >dist/session49/inhibitor.txt 2>&1 &
inhibitor_pid=$!
tries=0
until grep -q BOUNDED_INHIBITOR_READY dist/session49/inhibitor.txt; do
    kill -0 "$inhibitor_pid"
    tries=$((tries + 1)); [ "$tries" -lt 30 ]; sleep 0.1
done
gdbus call --system --dest org.freedesktop.ConsoleKit \
    --object-path /org/freedesktop/ConsoleKit/Manager \
    --method org.freedesktop.ConsoleKit.Manager.ListInhibitors >dist/session49/inhibitors.txt
grep -E "(uint32 )?$inhibitor_pid\)" dist/session49/inhibitors.txt
pkg-config --atleast-version=0.2.0.a1 bsd3-user-session
test -x /usr/local/libexec/bsd3-bus-ready
test -f /usr/local/share/bsd-cubed/gnome49/services/gnome-session
revision=$(cat packaging/freebsd/gnome-session49/PORTS_REVISION)
case "$revision" in *[!0-9a-f]*|'') exit 1 ;; esac
[ "${#revision}" -eq 40 ]
sh tools/ci-ports-tree.sh "$ports_root" "$revision"
mkdir -p "$ports_root/sysutils/bsd-cubed"
cp -R packaging/freebsd/bsd-cubed/. "$ports_root/sysutils/bsd-cubed/"
mkdir -p "$(dirname "$port")"
cp -R packaging/freebsd/gnome-session49 "$port"
# All custom runtime/SDK providers must already be installed by preceding
# sequential candidate steps. Never replace those providers from the public repo.
make -C "$port" PORTSDIR="$ports_root" BATCH=yes \
    build-depends-list run-depends-list >"$work_root/direct-origins.raw"
sort -u "$work_root/direct-origins.raw" >"$work_root/direct-origins"
: >dist/session49/direct-dependencies.txt
while IFS= read -r dependency; do
    case "$dependency" in "$ports_root"/*) ;; *) exit 1 ;; esac
    [ "$dependency" != "$ports_root/sysutils/bsd-cubed" ] || continue
    case "$dependency" in
        *@*)
            flavor=${dependency##*@}
            dependency=${dependency%@*}
            package=$(make -C "$dependency" PORTSDIR="$ports_root" BATCH=yes FLAVOR="$flavor" -V PKGBASE)
            ;;
        *) package=$(make -C "$dependency" PORTSDIR="$ports_root" BATCH=yes -V PKGBASE) ;;
    esac
    case "$package" in mutter|gjs|gsettings-desktop-schemas|gnome-shell|bsd-cubed) continue;; esac
    case "$package" in *[!A-Za-z0-9_+.-]*|'') exit 1 ;; esac
    printf '%s\n' "$package" >>dist/session49/direct-dependencies.txt
done <"$work_root/direct-origins"
if [ -s dist/session49/direct-dependencies.txt ]; then
    xargs pkg install -yA <dist/session49/direct-dependencies.txt
fi
{
    freebsd-version -kru
    uname -v
    cc --version
    pkg config ABI
    git rev-parse HEAD
    pkg query '%n %v %q'
} >dist/session49/provenance.txt
if ! make -C "$port" PORTSDIR="$ports_root" BATCH=yes DEVELOPER=yes \
    WRKDIRPREFIX="$work_root/work" PACKAGES="$repo_root/dist/session49/packages" \
    stage check-plist stage-qa package >dist/session49/build.log 2>&1; then
    tail -100 dist/session49/build.log
    exit 1
fi
wrksrc=$(make -C "$port" PORTSDIR="$ports_root" WRKDIRPREFIX="$work_root/work" -V WRKSRC)
source_name=${wrksrc##*/}
case "$source_name" in gnome-session-49.0) ;; *) exit 1 ;; esac
tar --exclude "$source_name/_build" -C "${wrksrc%/*}" -czf \
    "$repo_root/dist/session49/$source_name-bsd3-source.tar.gz" "$source_name"
tar -C "$work_root/overlay" -czf "$repo_root/dist/session49/$source_name-bsd3-port.tar.gz" \
    x11/gnome-session -C "$repo_root" tools/ci-gnome49.sh tools/ci-ports-tree.sh tests/native-gnome49-graph.py \
    tests/native-session.c tests/native-window.c tests/native-inhibit.c
(cd dist/session49/packages/All && sha256 -r gnome-session-*.pkg) >dist/session49/SESSION_SHA256SUMS
(cd dist/session49 && sha256 -r "$source_name-bsd3-source.tar.gz" \
    "$source_name-bsd3-port.tar.gz") >>dist/session49/SESSION_SHA256SUMS
set -- dist/session49/packages/All/gnome-session-*.pkg
[ "$#" -eq 1 ] && [ -f "$1" ]
! pgrep -x gdm >/dev/null
! pgrep -x sddm >/dev/null
pkg -o REPOS_DIR=/var/empty add -f "$1"
mkdir -p "$work_root/runtime/tests"
cp tests/native-gnome49-graph.py "$work_root/runtime/tests/"
cc -std=c11 -Wall -Wextra -Werror $(pkg-config --cflags bsd3-login) \
    tests/native-session.c $(pkg-config --libs bsd3-login) \
    -o "$work_root/runtime/native-session"
cc -std=c11 -Wall -Wextra -Werror $(pkg-config --cflags gtk+-3.0) \
    tests/native-window.c -o "$work_root/runtime/native-window" $(pkg-config --libs gtk+-3.0)
chown -R circleci "$work_root/runtime"
chmod 755 "$work_root"
status=0
sudo -H -u circleci /usr/bin/env -i HOME=/home/circleci USER=circleci LOGNAME=circleci \
    PATH=/usr/local/bin:/usr/bin:/bin LANG=C.UTF-8 /bin/sh -c \
    "cd '$work_root/runtime' && timeout -k 5 120 ./native-session dbus-run-session -- python3.12 tests/native-gnome49-graph.py ./native-window" \
    >dist/session49/native-runtime.log 2>&1 || status=$?
cat dist/session49/native-runtime.log
[ "$status" -eq 0 ]
