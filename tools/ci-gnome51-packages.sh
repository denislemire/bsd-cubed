#!/bin/sh
# SPDX-License-Identifier: BSD-2-Clause
set -eu
export PATH=/usr/local/sbin:/usr/local/bin:/sbin:/bin:/usr/sbin:/usr/bin
[ "${CIRCLECI:-}" = true ] && [ "$(uname -s)" = FreeBSD ] && [ "$(id -u)" -eq 0 ] || exit 77

repo=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
cd "$repo"
out="$repo/dist/gnome51"
mkdir -p "$out"
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT
trap 'exit 129' HUP
trap 'exit 130' INT
trap 'exit 143' TERM

pkg install -y git
revision=$(cat packaging/freebsd/mutter49/PORTS_REVISION)
ports_root=${BSD3_CI_PORTS_ROOT:-"$work/ports"}
sh tools/ci-ports-tree.sh "$ports_root" "$revision"
# Build dependencies come from the VM's configured FreeBSD package repository.
pkg install -y pkgconf meson ninja python312 gettext-tools gettext-runtime \
    glib gobject-introspection dbus spidermonkey140 libffi readline cairo \
    libX11 libXext fontconfig freetype2 harfbuzz fribidi gtk4 libadwaita desktop-file-utils
{
    freebsd-version -kru
    cc --version
    git -c safe.directory="$repo" rev-parse HEAD
    printf 'ports=%s\n' "$revision"
    printf 'gnome-build-meta=%s\n' db69d872ba90a7bb96b53bbcc8a88ea8e9c6c9f0
    pkg query '%n %v' | sort
} >"$out/provenance.txt"

mkdir -p "$work/distfiles" "$out/packages/All"
set -- dist/packages/All/bsd-cubed-*.pkg
[ "$#" -eq 1 ] && [ -f "$1" ]
pkg add -f "$1"
cp -R packaging/freebsd/bsd-cubed "$ports_root/sysutils/bsd-cubed"
cp -R packaging/freebsd/dinit "$ports_root/sysutils/dinit"

for origin in devel/gsettings-desktop-schemas x11-themes/gnome-backgrounds \
    deskutils/gnome-font-viewer lang/gjs x11-wm/mutter x11/gnome-shell x11/gnome-session; do
    name=${origin##*/}
    port="$work/overlay/$origin"
    mkdir -p "${port%/*}"
    cp -R "packaging/freebsd/${name}51" "$port"
    make -C "$port" PORTSDIR="$ports_root" DEPENDS_SHOW_FLAVOR=yes \
        build-depends-list run-depends-list >"$work/dependencies"
    sort -u "$work/dependencies" >"$work/origins"
    while IFS= read -r dependency; do
        case "$dependency" in "$ports_root"/*) ;; *) exit 1 ;; esac
        case "${dependency#"$ports_root"/}" in
            sysutils/bsd-cubed|lang/gjs|devel/gsettings-desktop-schemas|x11-wm/mutter|x11/gnome-shell) continue ;;
        esac
        case "$dependency" in
            *@*) make -C "${dependency%@*}" PORTSDIR="$ports_root" FLAVOR="${dependency##*@}" -V PKGBASE ;;
            *) make -C "$dependency" PORTSDIR="$ports_root" -V PKGBASE ;;
        esac
    done <"$work/origins" >"$work/pkg-names"
    xargs pkg install -yA <"$work/pkg-names"
    make -C "$port" PORTSDIR="$ports_root" BATCH=yes DEVELOPER=yes \
        BSD3_SERIES="$repo/packaging/freebsd/gnome51" BSD3_SOURCE="$repo/src" \
        DISTDIR="$work/distfiles" WRKDIRPREFIX="$work/build" \
        PACKAGES="$out/packages" MAKE_JOBS_NUMBER=3 \
        stage check-plist stage-qa package >"$out/$name-build.log" 2>&1 || {
        tail -100 "$out/$name-build.log"
        exit 1
    }
    pkgname=$(make -C "$port" PORTSDIR="$ports_root" -V PKGNAME)
    pkg add -f "$out/packages/All/$pkgname.pkg"
done

# Smoke-test the exact staged schema tree and the newly packaged GJS runtime.
schema_port="$work/overlay/devel/gsettings-desktop-schemas"
schema_stage=$(make -C "$schema_port" PORTSDIR="$ports_root" \
    WRKDIRPREFIX="$work/build" -V STAGEDIR)
schema_prefix=$(make -C "$schema_port" PORTSDIR="$ports_root" -V PREFIX)
glib-compile-schemas --strict --dry-run "$schema_stage$schema_prefix/share/glib-2.0/schemas"
[ "$(gjs -c 'print(imports.system.version)')" = 19001 ]
gjs -c 'if (typeof imports.gi.GLib.get_monotonic_time() !== "number") imports.system.exit(1)'

(cd "$out/packages/All" && sha256 -r ./*.pkg) >"$out/SHA256SUMS"
cp -R "$work/distfiles/gnome" "$out/sources"
