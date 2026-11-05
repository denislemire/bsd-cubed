#!/bin/sh
# SPDX-License-Identifier: BSD-2-Clause
# Native-backend port experiment on the disposable FreeBSD CI VM only.
set -eu
export PATH=/usr/local/sbin:/usr/local/bin:/sbin:/bin:/usr/sbin:/usr/bin
[ "$(uname -s)" = FreeBSD ] || exit 1
repo_root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
cd "$repo_root"
mkdir -p dist/mutter/packages
work_root=$(mktemp -d)
ports_root="$work_root/ports"
port="$work_root/overlay/x11-wm/mutter"
finish() {
    status=$?
    trap - EXIT HUP INT TERM
    # Keep build diagnostics even when configuration/compilation fails.
    if [ -d "$work_root/work" ]; then
        find "$work_root/work" -name meson-log.txt -exec cp {} dist/mutter/meson-log.txt \;
    fi
    rm -rf "$work_root"
    exit "$status"
}
trap finish EXIT
trap 'exit 129' HUP
trap 'exit 130' INT
trap 'exit 143' TERM
revision=$(cat packaging/freebsd/mutter/PORTS_REVISION)
case "$revision" in *[!0-9a-f]*|'') exit 1 ;; esac
[ "${#revision}" -eq 40 ] || exit 1
{
    freebsd-version -kru
    uname -v
    cc --version
    pkg config ABI
    git rev-parse HEAD
} >dist/mutter/provenance.txt
# Pin the complete ports framework and consumer port, not just its Makefile.
git init -q "$ports_root"
git -C "$ports_root" remote add origin https://git.freebsd.org/ports.git
git -C "$ports_root" fetch --depth 1 origin "$revision"
git -C "$ports_root" checkout --detach FETCH_HEAD
# Recursive dependency enumeration still needs the origin even with its package installed.
cp -R packaging/freebsd/bsd-cubed "$ports_root/sysutils/bsd-cubed"
mkdir -p "$(dirname "$port")"
cp -R "$ports_root/x11-wm/mutter" "$port"
(cd "$port" && patch < "$repo_root/packaging/freebsd/mutter/native-port.patch")
cp packaging/freebsd/mutter/patch-* "$port/files/"
printf '%s\n' "$revision" >dist/mutter/ports-revision.txt
cp packaging/freebsd/mutter/native-port.patch dist/mutter/
# Install the exact bridge package built and checked earlier in this same job.
set -- dist/packages/All/bsd-cubed-*.pkg
[ "$#" -eq 1 ] && [ -f "$1" ]
sudo pkg add "$1"
make -C "$port" PORTSDIR="$ports_root" BATCH=yes -V MESON_ARGS >dist/mutter/meson-args.txt
# Request direct build/runtime packages and let pkg resolve their runtime closure.
# install-missing-packages recursively includes build tools of dependencies, even
# though those dependencies are binary packages (e.g. Rust and cross toolchains).
make -C "$port" PORTSDIR="$ports_root" BATCH=yes DEPENDS_SHOW_FLAVOR=yes \
    build-depends-list run-depends-list | sort -u >"$work_root/direct-origins"
: >dist/mutter/direct-dependencies.txt
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
    printf '%s\n' "$package" >>dist/mutter/direct-dependencies.txt
done <"$work_root/direct-origins"
[ -s dist/mutter/direct-dependencies.txt ]
xargs sudo pkg install -yA <dist/mutter/direct-dependencies.txt
pkg query '%n %v %q' >dist/mutter/dependencies.txt
if ! make -C "$port" PORTSDIR="$ports_root" BATCH=yes DEVELOPER=yes \
    WRKDIRPREFIX="$work_root/work" PACKAGES="$repo_root/dist/mutter/packages" \
    stage check-plist stage-qa package >dist/mutter/build.log 2>&1; then
    tail -100 dist/mutter/build.log
    exit 1
fi
tail -60 dist/mutter/build.log
find dist/mutter/packages/All -name '*.pkg' -exec sha256 -r {} \; >dist/mutter/SHA256SUMS

set -- dist/mutter/packages/All/mutter-*.pkg
[ "$#" -eq 1 ] && [ -f "$1" ]
sh tools/test-native-mutter.sh "$1"
