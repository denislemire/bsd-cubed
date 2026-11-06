#!/bin/sh
# SPDX-License-Identifier: BSD-2-Clause
# Independent consumer dependency packages; no desktop or hardware deployment.
set -eu
export PATH=/usr/local/sbin:/usr/local/bin:/sbin:/bin:/usr/sbin:/usr/bin
[ "$(uname -s)" = FreeBSD ] && [ "$(id -u)" -eq 0 ]
repo=$(pwd)
out="$repo/dist/gnome49-runtime"
mkdir -p "$out/packages"
: >"$out/SHA256SUMS"
work=$(mktemp -d)
inhibitor_pid=
finish() {
    status=$?
    trap - EXIT HUP INT TERM
    if [ -n "$inhibitor_pid" ]; then
        kill -TERM "$inhibitor_pid" 2>/dev/null || :
        wait "$inhibitor_pid" || :
    fi
    if [ -d "$work/build" ]; then
        find "$work/build" -name meson-log.txt -exec cp {} "$out/last-meson-log.txt" \;
        find "$work/build" -name testlog.txt -exec cp {} "$out/last-testlog.txt" \;
    fi
    rm -rf "$work"
    exit "$status"
}
trap finish EXIT
trap 'exit 129' HUP
trap 'exit 130' INT
trap 'exit 143' TERM
pkg install -y git pkgconf glib dbus consolekit2 meson ninja python312 \
    spidermonkey140 libffi readline cairo gobject-introspection libX11 libXext
service dbus onestatus >/dev/null 2>&1 || service dbus onestart
cc -std=c11 -Wall -Wextra -Werror $(pkg-config --cflags gio-unix-2.0) \
    tests/native-inhibit.c -o "$work/native-inhibit" $(pkg-config --libs gio-unix-2.0)
"$work/native-inhibit" 1800 >"$out/inhibitor.log" 2>&1 &
inhibitor_pid=$!
tries=0
until grep -q BOUNDED_INHIBITOR_READY "$out/inhibitor.log"; do
    kill -0 "$inhibitor_pid"
    tries=$((tries + 1)); [ "$tries" -lt 30 ]; sleep 0.1
done
gdbus call --system --dest org.freedesktop.ConsoleKit \
    --object-path /org/freedesktop/ConsoleKit/Manager \
    --method org.freedesktop.ConsoleKit.Manager.ListInhibitors >"$out/inhibitors.txt"
grep -E "(uint32 )?$inhibitor_pid\)" "$out/inhibitors.txt"
revision=$(cat packaging/freebsd/gjs49/PORTS_REVISION)
[ "$(cat packaging/freebsd/gsettings-desktop-schemas49/PORTS_REVISION)" = "$revision" ]
case "$revision" in *[!0-9a-f]*|'') exit 1;; esac
[ "${#revision}" -eq 40 ]
{
    freebsd-version -kru
    uname -v
    cc --version
    git rev-parse HEAD
    printf 'ports=%s\n' "$revision"
    pkg query '%n %v' glib spidermonkey140 gobject-introspection
} >"$out/provenance.txt"
ports_root=${BSD3_CI_PORTS_ROOT:-"$work/ports"}
sh tools/ci-ports-tree.sh "$ports_root" "$revision"
for origin in lang/gjs devel/gsettings-desktop-schemas; do
    name=${origin##*/}
    mkdir -p "$work/overlay/${origin%/*}"
    cp -R "packaging/freebsd/${name}49" "$work/overlay/$origin"
    port="$work/overlay/$origin"
    make -C "$port" PORTSDIR="$ports_root" BATCH=yes DEVELOPER=yes \
        WRKDIRPREFIX="$work/build" PACKAGES="$out/packages" MAKE_JOBS_NUMBER=3 \
        stage check-plist stage-qa package >"$out/$name-build.log" 2>&1 || {
        tail -80 "$out/$name-build.log"; exit 1;
    }
    source=$(make -C "$port" PORTSDIR="$ports_root" WRKDIRPREFIX="$work/build" -V WRKSRC)
    source_name=${source##*/}
    if [ "$name" = gjs ]; then
        # Use the actual newly built library; do not accidentally test installed GJS.
        id circleci >/dev/null
        chmod 755 "$work"
        chown -R circleci "$source/_build"
        # Run the exact binary without login greetings or inherited credentials.
        sudo -H -u circleci /usr/bin/env -i HOME=/home/circleci USER=circleci \
            LOGNAME=circleci PATH=/usr/local/bin:/usr/bin:/bin LANG=C.UTF-8 \
            LD_LIBRARY_PATH="$source/_build" GI_TYPELIB_PATH="$source/_build" \
            "$source/_build/gjs-console" -c 'print(imports.system.version)' >"$out/gjs-version.txt"
        [ "$(cat "$out/gjs-version.txt")" = 18600 ]
        sudo -H -u circleci /usr/bin/env -i HOME=/home/circleci USER=circleci \
            LOGNAME=circleci PATH=/usr/local/bin:/usr/bin:/bin LANG=C.UTF-8 \
            meson test -C "$source/_build" --no-rebuild --print-errorlogs \
            'API tests' GIMarshalling GObject Introspection Gio GLib \
            >"$out/gjs-functional-tests.txt" 2>&1 || {
            tail -80 "$out/gjs-functional-tests.txt"; exit 1;
        }
        cat "$out/gjs-functional-tests.txt"
    fi
    # Consumers retain their upstream licenses and complete corresponding source.
    tar --exclude "$source_name/_build" -C "${source%/*}" -czf \
        "$out/$source_name-bsd3-source.tar.gz" "$source_name"
    tar -C "$work/overlay" -czf "$out/$source_name-bsd3-port.tar.gz" "$origin" \
        -C "$repo" tools/ci-gnome49-runtime.sh tools/ci-ports-tree.sh tests/native-inhibit.c
    (cd "$out" && sha256 -r "$source_name-bsd3-source.tar.gz" \
        "$source_name-bsd3-port.tar.gz") >>"$out/SHA256SUMS"
done
(cd "$out/packages/All" && sha256 -r gjs-*.pkg gsettings-desktop-schemas-*.pkg) >>"$out/SHA256SUMS"
printf 'PASS: native consumer dependency packages and GJS functional tests\n' >"$out/complete.txt"
