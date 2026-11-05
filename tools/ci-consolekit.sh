#!/bin/sh
# SPDX-License-Identifier: BSD-2-Clause
# Rebuild and exercise the actual dependency on a disposable CI VM.
set -eu
export PATH=/usr/local/sbin:/usr/local/bin:/sbin:/bin:/usr/sbin:/usr/bin
[ "$(uname -s)" = FreeBSD ] && [ "${CIRCLECI:-}" = true ] || exit 1
unset DBUS_SYSTEM_BUS_ADDRESS DBUS_SESSION_BUS_ADDRESS XDG_SESSION_COOKIE XDG_RUNTIME_DIR
repo_root=$(pwd)
sudo service dbus onestatus >/dev/null 2>&1 || sudo service dbus onestart
mkdir -p dist/consolekit/packages
work_root=$(mktemp -d)
service_started=no
finish() {
    result=$?
    trap - EXIT HUP INT TERM
    [ "$service_started" = no ] || sudo service bsddd onestop || :
    rm -rf "$work_root"
    exit "$result"
}
trap finish EXIT
trap 'exit 129' HUP
trap 'exit 130' INT
trap 'exit 143' TERM
ports_root="$work_root/ports"
port="$work_root/overlay/sysutils/consolekit2"
revision=$(cat packaging/freebsd/mutter/PORTS_REVISION)
case "$revision" in *[!0-9a-f]*|'') exit 1 ;; esac
[ "${#revision}" -eq 40 ]
{
    id -u
    freebsd-version -kru
    uname -v
    cc --version
    pkg config ABI
    git rev-parse HEAD
} >dist/consolekit/provenance.txt
git init -q "$ports_root"
git -C "$ports_root" remote add origin https://git.freebsd.org/ports.git
git -C "$ports_root" fetch --depth 1 origin "$revision"
git -C "$ports_root" checkout --detach FETCH_HEAD
mkdir -p "$(dirname "$port")"
cp -R "$ports_root/sysutils/consolekit2" "$port"
(cd "$port" && patch < "$repo_root/packaging/freebsd/consolekit2/native-port.patch")
cp packaging/freebsd/consolekit2/patch-* "$port/files/"
printf '%s\n' "$revision" >dist/consolekit/ports-revision.txt
make -C "$port" PORTSDIR="$ports_root" BATCH=yes \
    build-depends-list run-depends-list >"$work_root/origins.raw"
sort -u "$work_root/origins.raw" >"$work_root/origins"
: >dist/consolekit/direct-dependencies.txt
while IFS= read -r dependency; do
    case "$dependency" in "$ports_root"/*) ;; *) exit 1 ;; esac
    case "$dependency" in
        *@*)
            flavor=${dependency##*@}
            dependency=${dependency%@*}
            package=$(make -C "$dependency" PORTSDIR="$ports_root" BATCH=yes FLAVOR="$flavor" -V PKGBASE)
            ;;
        *) package=$(make -C "$dependency" PORTSDIR="$ports_root" BATCH=yes -V PKGBASE) ;;
    esac
    case "$package" in *[!A-Za-z0-9_+.-]*|'') exit 1 ;; esac
    printf '%s\n' "$package" >>dist/consolekit/direct-dependencies.txt
done <"$work_root/origins"
[ -s dist/consolekit/direct-dependencies.txt ]
xargs sudo pkg install -yA <dist/consolekit/direct-dependencies.txt
# Archive patched source before configure/build adds machine-specific products.
make -C "$port" PORTSDIR="$ports_root" BATCH=yes WRKDIRPREFIX="$work_root/work" patch
wrksrc=$(make -C "$port" PORTSDIR="$ports_root" WRKDIRPREFIX="$work_root/work" -V WRKSRC)
source_name=${wrksrc##*/}
case "$source_name" in ConsoleKit2-[0-9]*) ;; *) exit 1 ;; esac
tar -C "${wrksrc%/*}" -czf "$repo_root/dist/consolekit/consolekit2-bsd3-source.tar.gz" "$source_name"
tar -C "$work_root/overlay" -czf "$repo_root/dist/consolekit/consolekit2-bsd3-port.tar.gz" \
    sysutils/consolekit2 -C "$repo_root" packaging/freebsd/consolekit2 \
    packaging/freebsd/mutter/PORTS_REVISION tools/ci-consolekit.sh tools/test-native-consolekit.sh tests/native-session.c tests/native-device.c
if ! make -C "$port" PORTSDIR="$ports_root" BATCH=yes DEVELOPER=yes \
    WRKDIRPREFIX="$work_root/work" PACKAGES="$repo_root/dist/consolekit/packages" \
    stage check-plist stage-qa package >dist/consolekit/build.log 2>&1; then
    tail -80 dist/consolekit/build.log
    exit 1
fi
set -- dist/consolekit/packages/All/consolekit2-*.pkg
[ "$#" -eq 1 ] && [ -f "$1" ]
(cd dist/consolekit/packages/All && sha256 -r consolekit2-*.pkg) >dist/consolekit/CONSOLEKIT_SHA256SUMS
(cd dist/consolekit && sha256 -r consolekit2-bsd3-source.tar.gz consolekit2-bsd3-port.tar.gz) >>dist/consolekit/CONSOLEKIT_SHA256SUMS
# No pre-existing real CK process may hide which binary is being tested.
owner=$(gdbus call --system --dest org.freedesktop.DBus --object-path /org/freedesktop/DBus \
    --method org.freedesktop.DBus.NameHasOwner org.freedesktop.ConsoleKit)
[ "$owner" = '(false,)' ] || { echo 'Unexpected existing CK process' >&2; exit 1; }
sudo pkg add -f "$1"
sudo gdbus call --system --dest org.freedesktop.DBus --object-path /org/freedesktop/DBus \
    --method org.freedesktop.DBus.ReloadConfig >/dev/null
