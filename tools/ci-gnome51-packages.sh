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
    glib glib-bootstrap gobject-introspection gobject-introspection-bootstrap dbus spidermonkey140 libffi readline cairo \
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
cp "$1" "$out/packages/All/"
cp -R packaging/freebsd/bsd-cubed "$ports_root/sysutils/bsd-cubed"
cp -R packaging/freebsd/dinit "$ports_root/sysutils/dinit"
cp -R packaging/freebsd/gnome-qr51 "$ports_root/graphics/gnome-qr"

if [ ! -d "$ports_root/graphics/glycin" ]; then
    cp -R packaging/freebsd/glycin51 "$ports_root/graphics/glycin"
fi

mkdir "$work/repos"
printf 'gnome51: { url: "file://%s", enabled: yes }\n' "$out/packages" >"$work/repos/gnome51.conf"
for origin in devel/glib20 devel/dconf x11-toolkits/gtk40 x11-toolkits/libadwaita devel/glibmm26 x11-toolkits/gtkmm40 sysutils/accountsservice \
    devel/gsettings-desktop-schemas x11-themes/gnome-backgrounds \
    deskutils/gnome-font-viewer x11/tecla lang/gjs graphics/gnome-qr x11/gnome-desktop sysutils/gnome-settings-daemon \
    devel/libsoup3 net/libcloudproviders security/libsecret security/gcr textproc/yelp-xsl x11/zenity4 \
    net/gnome-online-accounts databases/evolution-data-server net-im/folks sysutils/gnome-control-center deskutils/baobab math/gnome-calculator \
    x11-themes/adwaita-icon-theme deskutils/gnome-weather deskutils/gnome-clocks deskutils/gnome-calendar \
    archivers/gnome-autoar mail/evolution devel/tinysparql sysutils/localsearch graphics/glycin x11-fm/nautilus graphics/papers x11-fm/sushi accessibility/at-spi2-core accessibility/orca x11/yelp x11-toolkits/libshumate deskutils/gnome-maps \
    sysutils/gnome-system-monitor x11/gnome-console deskutils/gnome-characters \
    editors/gnome-text-editor misc/gnome-user-docs x11/xdg-desktop-portal-gnome x11-wm/mutter x11/gnome-shell x11/gnome-shell-extensions x11/gnome-session; do
    name=${origin##*/}
    port="$work/overlay/$origin"
    mkdir -p "${port%/*}"
    recipe="$repo/packaging/freebsd/${name}51"
    if [ -f "$recipe/port.patch" ]; then
        cp -R "$ports_root/$origin" "$port"
        patch -s -d "$port" -p1 <"$recipe/port.patch"
    else
        cp -R "$recipe" "$port"
    fi
    make -C "$port" PORTSDIR="$ports_root" DEPENDS_SHOW_FLAVOR=yes \
        build-depends-list run-depends-list >"$work/dependencies"
    sort -u "$work/dependencies" >"$work/origins"
    while IFS= read -r dependency; do
        case "$dependency" in "$ports_root"/*) ;; *) exit 1 ;; esac
        case "${dependency#"$ports_root"/}" in
            devel/glib20|devel/dconf|devel/glibmm26|x11-toolkits/gtkmm40|x11-toolkits/gtk40|x11-toolkits/libadwaita|sysutils/accountsservice|sysutils/gnome-settings-daemon|x11/tecla) continue ;;
            devel/libsoup3|net/libcloudproviders|security/libsecret|security/gcr|textproc/yelp-xsl|x11/zenity4|graphics/glycin|x11-fm/nautilus) continue ;;
            archivers/gnome-autoar|devel/tinysparql|databases/evolution-data-server|net-im/folks|net/gnome-online-accounts|accessibility/at-spi2-core|x11-toolkits/libshumate) continue ;;
            sysutils/bsd-cubed|graphics/gnome-qr|x11/gnome-desktop|lang/gjs|devel/gsettings-desktop-schemas|x11-wm/mutter|x11/gnome-shell) continue ;;
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
    pkg repo "$out/packages"
    pkg -R "$work/repos" update -f -r gnome51
    # Only the disposable builder needs the new Mutter before Shell can be built.
    if [ "$name" = mutter ] && pkg info -e 'gnome-shell<51'; then
        pkg add -f "$out/packages/All/$pkgname.pkg"
    else
        pkg -R "$work/repos" install -Uy -r gnome51 "$pkgname"
    fi
    pkgbase=$(make -C "$port" PORTSDIR="$ports_root" -V PKGBASE)
    [ "$(pkg query '%n-%v' "$pkgbase")" = "$pkgname" ]
    if [ "$name" != gsettings-desktop-schemas ]; then
        make -C "$port" PORTSDIR="$ports_root" WRKDIRPREFIX="$work/build" NOCLEANDEPENDS=yes clean
    fi
done

# Smoke-test the exact staged schema tree and the newly packaged GJS runtime.
schema_port="$work/overlay/devel/gsettings-desktop-schemas"
schema_stage=$(make -C "$schema_port" PORTSDIR="$ports_root" \
    WRKDIRPREFIX="$work/build" -V STAGEDIR)
schema_prefix=$(make -C "$schema_port" PORTSDIR="$ports_root" -V PREFIX)
glib-compile-schemas --strict --dry-run "$schema_stage$schema_prefix/share/glib-2.0/schemas"
[ "$(gjs -c 'print(imports.system.version)')" = 19001 ]
gjs -c 'if (typeof imports.gi.GLib.get_monotonic_time() !== "number") imports.system.exit(1)'

pkg install -y wayland-utils
mkdir "$work/runtime"
cc tests/native-window.c -o "$work/runtime/native-window" $(pkg-config --cflags --libs gtk+-3.0)
cp tests/native-shell49.py "$work/runtime/check-shell.py"
# Keep the packages and build inputs private to the controller.
runtime=$(mktemp -d)
cp "$work/runtime/"* "$runtime/"
chown -R circleci "$runtime"
status=0
(cd "$runtime" && sudo -H -u circleci /usr/bin/env -i HOME=/home/circleci PATH="$PATH" \
    timeout -k 5 90 dbus-run-session -- python3.12 ./check-shell.py ./native-window 51.1) || status=$?
if [ -d "$runtime/dist/shell51" ]; then cp -R "$runtime/dist/shell51" "$out/runtime"; fi
rm -rf "$runtime"
[ "$status" -eq 0 ]

(cd "$out/packages/All" && sha256 -r ./*.pkg) >"$out/SHA256SUMS"
cp -R "$work/distfiles/gnome" "$out/sources"
