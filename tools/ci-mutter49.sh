#!/bin/sh
# SPDX-License-Identifier: BSD-2-Clause
# Native-backend port experiment on the disposable FreeBSD CI VM only.
set -eu
export PATH=/usr/local/sbin:/usr/local/bin:/sbin:/bin:/usr/sbin:/usr/bin
[ "$(uname -s)" = FreeBSD ] && [ "$(id -u)" -eq 0 ] && [ "${CIRCLECI:-}" = true ] || exit 1
repo_root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
cd "$repo_root"
mkdir -p dist/mutter49/packages
work_root=$(mktemp -d)
ports_root="$work_root/ports"
port="$work_root/overlay/x11-wm/mutter"
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
        find "$work_root/work" -name meson-log.txt -exec cp {} dist/mutter49/meson-log.txt \;
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
"$work_root/native-inhibit" 1800 >dist/mutter49/inhibitor.txt 2>&1 &
inhibitor_pid=$!
tries=0
until grep -q BOUNDED_INHIBITOR_READY dist/mutter49/inhibitor.txt; do
    kill -0 "$inhibitor_pid"
    tries=$((tries + 1)); [ "$tries" -lt 30 ]; sleep 0.1
done
gdbus call --system --dest org.freedesktop.ConsoleKit \
    --object-path /org/freedesktop/ConsoleKit/Manager \
    --method org.freedesktop.ConsoleKit.Manager.ListInhibitors >dist/mutter49/inhibitors.txt
grep -E "(uint32 )?$inhibitor_pid\)" dist/mutter49/inhibitors.txt
revision=$(cat packaging/freebsd/mutter49/PORTS_REVISION)
case "$revision" in *[!0-9a-f]*|'') exit 1 ;; esac
[ "${#revision}" -eq 40 ] || exit 1
{
    freebsd-version -kru
    uname -v
    cc --version
    pkg config ABI
    git rev-parse HEAD
} >dist/mutter49/provenance.txt
# Pin the complete ports framework and consumer port, not just its Makefile.
git init -q "$ports_root"
git -C "$ports_root" remote add origin https://git.freebsd.org/ports.git
git -C "$ports_root" fetch --depth 1 origin "$revision"
git -C "$ports_root" checkout --detach FETCH_HEAD
# Recursive dependency enumeration still needs the origin even with its package installed.
cp -R packaging/freebsd/bsd-cubed "$ports_root/sysutils/bsd-cubed"
mkdir -p "$(dirname "$port")"
cp -R packaging/freebsd/mutter49 "$port"
printf '%s\n' "$revision" >dist/mutter49/ports-revision.txt
cp packaging/freebsd/mutter49/files/patch-bsd3-native dist/mutter49/
# Install the exact bridge package built and checked earlier in this same job.
set -- dist/packages/All/bsd-cubed-*.pkg
[ "$#" -eq 1 ] && [ -f "$1" ]
sudo pkg add -f "$1"
make -C "$port" PORTSDIR="$ports_root" BATCH=yes -V MESON_ARGS >dist/mutter49/meson-args.txt
# Request direct build/runtime packages and let pkg resolve their runtime closure.
# install-missing-packages recursively includes build tools of dependencies, even
# though those dependencies are binary packages (e.g. Rust and cross toolchains).
make -C "$port" PORTSDIR="$ports_root" BATCH=yes DEPENDS_SHOW_FLAVOR=yes \
    build-depends-list run-depends-list >"$work_root/direct-origins.raw"
sort -u "$work_root/direct-origins.raw" >"$work_root/direct-origins"
: >dist/mutter49/direct-dependencies.txt
while IFS= read -r dependency; do
    case "$dependency" in "$ports_root"/*) ;; *) exit 1 ;; esac
    [ "$dependency" != "$ports_root/sysutils/bsd-cubed" ] || continue
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
    printf '%s\n' "$package" >>dist/mutter49/direct-dependencies.txt
done <"$work_root/direct-origins"
[ -s dist/mutter49/direct-dependencies.txt ]
xargs sudo pkg install -yA <dist/mutter49/direct-dependencies.txt
pkg query '%n %v %q' >dist/mutter49/dependencies.txt
if ! make -C "$port" PORTSDIR="$ports_root" BATCH=yes DEVELOPER=yes \
    WRKDIRPREFIX="$work_root/work" PACKAGES="$repo_root/dist/mutter49/packages" \
    stage check-plist stage-qa package >dist/mutter49/build.log 2>&1; then
    tail -100 dist/mutter49/build.log
    exit 1
fi
tail -60 dist/mutter49/build.log
# Preserve corresponding GPL consumer source and the exact modified port recipe.
wrksrc=$(make -C "$port" PORTSDIR="$ports_root" WRKDIRPREFIX="$work_root/work" -V WRKSRC)
python_cmd=$(make -C "$port" PORTSDIR="$ports_root" -V PYTHON_CMD)
"$python_cmd" tests/test-mutter49-drm.py "$wrksrc/src/backends/meta-udev.c" \
    >dist/mutter49/drm-discovery-fixtures.txt 2>&1
cat dist/mutter49/drm-discovery-fixtures.txt
"$python_cmd" tests/test-mutter49-frame-swap.py "$wrksrc/src/backends/meta-stage-impl.c" \
    >dist/mutter49/frame-swap-fixtures.txt 2>&1
cat dist/mutter49/frame-swap-fixtures.txt
"$python_cmd" tests/test-mutter49-background.py "$wrksrc/src/compositor/meta-background-image.c" \
    >dist/mutter49/background-fixtures.txt 2>&1
cat dist/mutter49/background-fixtures.txt
source_name=${wrksrc##*/}
case "$source_name" in mutter-[0-9]*) ;; *) exit 1 ;; esac
tar --exclude "$source_name/_build" -C "${wrksrc%/*}" -czf \
    "$repo_root/dist/mutter49/$source_name-bsd3-source.tar.gz" "$source_name"
tar -C "$work_root/overlay" -czf "$repo_root/dist/mutter49/$source_name-bsd3-port.tar.gz" \
    x11-wm/mutter -C "$repo_root" packaging/freebsd/bsd-cubed \
    tools/ci-mutter49.sh tests/native-mutter49.py tests/native-window.c \
    tests/test-mutter49-drm.py tests/test-mutter49-frame-swap.py \
    tests/test-mutter49-background.py tests/native-inhibit.c
(cd dist/mutter49/packages/All && sha256 -r mutter-*.pkg) >dist/mutter49/MUTTER_SHA256SUMS
(cd dist/mutter49 && sha256 -r "$source_name-bsd3-source.tar.gz" \
    "$source_name-bsd3-port.tar.gz") >>dist/mutter49/MUTTER_SHA256SUMS

set -- dist/mutter49/packages/All/mutter-*.pkg
[ "$#" -eq 1 ] && [ -f "$1" ]
[ "${CIRCLECI:-}" = true ] && [ "$(id -u)" -eq 0 ]
! pgrep -x gdm >/dev/null
! pgrep -x sddm >/dev/null
pkg -o REPOS_DIR=/var/empty add -f "$1"
runtime=$(mktemp -d)
mkdir -p "$runtime/tests"
cp tests/native-mutter49.py "$runtime/tests/"
cc -std=c11 -Wall -Wextra -Werror $(pkg-config --cflags gtk+-3.0) \
    tests/native-window.c -o "$runtime/native-window" $(pkg-config --libs gtk+-3.0)
chown -R circleci "$runtime"
status=0
su -l circleci -c "cd '$runtime' && timeout -k 5 90 dbus-run-session -- python3.12 tests/native-mutter49.py ./native-window" \
    >dist/mutter49/native-runtime.log 2>&1 || status=$?
if [ -d "$runtime/dist" ]; then cp -R "$runtime/dist" dist/mutter49/runtime; fi
rm -rf "$runtime"
cat dist/mutter49/native-runtime.log
[ "$status" -eq 0 ]
