#!/bin/sh
# SPDX-License-Identifier: BSD-2-Clause
# Native GDM port experiment on the disposable FreeBSD CI VM only.
set -eu
export PATH=/usr/local/sbin:/usr/local/bin:/sbin:/bin:/usr/sbin:/usr/bin
[ "$(uname -s)" = FreeBSD ] && [ "$(id -u)" -eq 0 ] || exit 1
repo_root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
cd "$repo_root"
mkdir -p dist/gdm/packages
work_root=$(mktemp -d)
ports_root="$work_root/ports"
port="$work_root/overlay/x11/gdm"
inhibitor_pid=
finish() {
    status=$?
    trap - EXIT HUP INT TERM
    if [ -n "$inhibitor_pid" ]; then
        kill -TERM "$inhibitor_pid" 2>/dev/null || :
        wait "$inhibitor_pid" || :
    fi
    # Keep build diagnostics even when configuration/compilation fails.
    if [ -d "$work_root/work" ]; then
        find "$work_root/work" -name meson-log.txt -exec cp {} dist/gdm/meson-log.txt \;
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
"$work_root/native-inhibit" 1800 >dist/gdm/inhibitor.txt 2>&1 &
inhibitor_pid=$!
tries=0
until grep -q BOUNDED_INHIBITOR_READY dist/gdm/inhibitor.txt; do
    kill -0 "$inhibitor_pid"
    tries=$((tries + 1)); [ "$tries" -lt 30 ]; sleep 0.1
done
gdbus call --system --dest org.freedesktop.ConsoleKit \
    --object-path /org/freedesktop/ConsoleKit/Manager \
    --method org.freedesktop.ConsoleKit.Manager.ListInhibitors >dist/gdm/inhibitors.txt
grep -E "(uint32 )?$inhibitor_pid\)" dist/gdm/inhibitors.txt
revision=$(cat packaging/freebsd/gdm/PORTS_REVISION)
case "$revision" in *[!0-9a-f]*|'') exit 1 ;; esac
[ "${#revision}" -eq 40 ] || exit 1
{
    freebsd-version -kru
    uname -v
    cc --version
    pkg config ABI
    git rev-parse HEAD
} >dist/gdm/provenance.txt
# Pin the complete ports framework and consumer port, not just its Makefile.
git init -q "$ports_root"
git -C "$ports_root" remote add origin https://git.freebsd.org/ports.git
git -C "$ports_root" fetch --depth 1 origin "$revision"
git -C "$ports_root" checkout --detach FETCH_HEAD
# Recursive dependency enumeration still needs the origin even with its package installed.
cp -R packaging/freebsd/bsd-cubed "$ports_root/sysutils/bsd-cubed"
mkdir -p "$(dirname "$port")"
cp -R "$ports_root/x11/gdm" "$port"
(cd "$port" && patch < "$repo_root/packaging/freebsd/gdm/native-port.patch")
cp packaging/freebsd/gdm/files/* "$port/files/"
printf '%s\n' "$revision" >dist/gdm/ports-revision.txt
cp packaging/freebsd/gdm/native-port.patch dist/gdm/
# Install the exact bridge package built and checked earlier in this same job.
set -- dist/packages/All/bsd-cubed-*.pkg
[ "$#" -eq 1 ] && [ -f "$1" ]
sudo pkg add -f "$1"
make -C "$port" PORTSDIR="$ports_root" BATCH=yes -V MESON_ARGS >dist/gdm/meson-args.txt
# Request direct build/runtime packages and let pkg resolve their runtime closure.
# install-missing-packages recursively includes build tools of dependencies, even
# though those dependencies are binary packages (e.g. Rust and cross toolchains).
make -C "$port" PORTSDIR="$ports_root" BATCH=yes DEPENDS_SHOW_FLAVOR=yes \
    build-depends-list run-depends-list >"$work_root/direct-origins.raw"
sort -u "$work_root/direct-origins.raw" >"$work_root/direct-origins"
: >dist/gdm/direct-dependencies.txt
while IFS= read -r dependency; do
    case "$dependency" in "$ports_root"/*) ;; *) exit 1 ;; esac
    [ "$dependency" != "$ports_root/sysutils/bsd-cubed" ] || continue
    [ "$dependency" != "$ports_root/x11-wm/mutter" ] || continue
    case "$dependency" in
        *@*)
            flavor=${dependency##*@}
            dependency=${dependency%@*}
            package=$(make -C "$dependency" PORTSDIR="$ports_root" BATCH=yes \
                FLAVOR="$flavor" -V PKGBASE)
            ;;
        *) package=$(make -C "$dependency" PORTSDIR="$ports_root" BATCH=yes -V PKGBASE) ;;
    esac
    case "$package" in *[!A-Za-z0-9_+.-]*|'') exit 1 ;; esac
    printf '%s\n' "$package" >>dist/gdm/direct-dependencies.txt
done <"$work_root/direct-origins"
[ -s dist/gdm/direct-dependencies.txt ]
xargs sudo pkg install -yA <dist/gdm/direct-dependencies.txt
pkg query '%n %v %q' >dist/gdm/dependencies.txt
if ! make -C "$port" PORTSDIR="$ports_root" BATCH=yes DEVELOPER=yes \
    WRKDIRPREFIX="$work_root/work" PACKAGES="$repo_root/dist/gdm/packages" \
    stage check-plist stage-qa package >dist/gdm/build.log 2>&1; then
    tail -100 dist/gdm/build.log
    exit 1
fi
tail -60 dist/gdm/build.log
# Preserve corresponding GPL consumer source and the exact modified port recipe.
wrksrc=$(make -C "$port" PORTSDIR="$ports_root" WRKDIRPREFIX="$work_root/work" -V WRKSRC)
python_cmd=$(make -C "$ports_root/devel/meson" PORTSDIR="$ports_root" -V PYTHON_CMD)
[ -x "$python_cmd" ]
"$python_cmd" tests/test-gdm-factory.py "$wrksrc/daemon/gdm-local-display-factory.c" \
    >dist/gdm/factory-fixtures.txt 2>&1
"$python_cmd" tests/test-gdm-seat.py "$wrksrc/daemon/gdm-local-display-factory.c" \
    >dist/gdm/seat-fixtures.txt 2>&1
"$python_cmd" tests/test-gdm-registration.py "$wrksrc/daemon/gdm-session-worker.c" \
    >dist/gdm/registration-fixtures.txt 2>&1
cc -std=c11 -Wall -Wextra -Werror -I "$wrksrc/daemon" \
    tests/test-gdm-vt.c -o "$work_root/test-gdm-vt"
"$work_root/test-gdm-vt" >dist/gdm/vt-fixtures.txt
cc -std=c11 -Wall -Wextra -Werror -I "$wrksrc/daemon" \
    tests/test-gdm-kernel-vt.c "$wrksrc/daemon/gdm-freebsd-vt.c" \
    -o "$work_root/test-gdm-kernel-vt"
sudo timeout -k 2 30 "$work_root/test-gdm-kernel-vt" --disposable-kvm circleci \
    >dist/gdm/kernel-vt.txt 2>&1
cat dist/gdm/*fixtures.txt dist/gdm/kernel-vt.txt
source_name=${wrksrc##*/}
case "$source_name" in gdm-[0-9]*) ;; *) exit 1 ;; esac
tar --exclude "$source_name/_build" -C "${wrksrc%/*}" -czf \
    "$repo_root/dist/gdm/$source_name-bsd3-source.tar.gz" "$source_name"
tar -C "$work_root/overlay" -czf "$repo_root/dist/gdm/$source_name-bsd3-port.tar.gz" \
    x11/gdm -C "$repo_root" packaging/freebsd/bsd-cubed \
    packaging/freebsd/gdm tools/ci-gdm.sh tests/test-gdm-factory.py \
    tests/test-gdm-seat.py tests/test-gdm-registration.py tests/test-gdm-vt.c \
    tests/test-gdm-kernel-vt.c tests/native-inhibit.c
(cd dist/gdm/packages/All && sha256 -r gdm-*.pkg) >dist/gdm/GDM_SHA256SUMS
(cd dist/gdm && sha256 -r "$source_name-bsd3-source.tar.gz" \
    "$source_name-bsd3-port.tar.gz") >>dist/gdm/GDM_SHA256SUMS

# Runtime GDM acceptance is deliberately separate: this job establishes a native
# package, source-level regressions and real VT semantics, not a working greeter.
