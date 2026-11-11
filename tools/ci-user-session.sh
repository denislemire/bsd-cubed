#!/bin/sh
# SPDX-License-Identifier: BSD-2-Clause
# Separate disposable FreeBSD executor; no laptop/desktop deployment.
set -eu
export PATH=/usr/local/sbin:/usr/local/bin:/sbin:/bin:/usr/sbin:/usr/bin
[ "$(uname -s)" = FreeBSD ] || exit 1
repo_root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
cd "$repo_root"
# The machine executor runs its controller as root. Give the ordinary-user
# suites an isolated checkout without copying CI remote credentials or keys.
if [ "$(id -u)" -eq 0 ]; then
    id circleci >/dev/null
    user_root=$(mktemp -d)
    trap 'rm -rf "$user_root"' EXIT
    trap 'exit 129' HUP
    trap 'exit 130' INT
    trap 'exit 143' TERM
    git clone --quiet --no-hardlinks "$repo_root" "$user_root/source"
    chown -R circleci "$user_root"
    set +e
    sudo -H -u circleci /usr/bin/env -i HOME=/home/circleci USER=circleci \
        LOGNAME=circleci PATH="$PATH" LANG=C.UTF-8 \
        PYTHON="${PYTHON:-/usr/local/bin/python3.12}" \
        PORTSDIR="${PORTSDIR:-}" BSD3_CI_DNS="${BSD3_CI_DNS:-}" \
        BSD3_CI_PKG_URL="${BSD3_CI_PKG_URL:-}" \
        /bin/sh "$user_root/source/tools/ci-user-session.sh"
    user_status=$?
    set -e
    mkdir -p "$repo_root/dist/user-session"
    if [ -d "$user_root/source/dist/user-session" ]; then
        cp -R "$user_root/source/dist/user-session/." "$repo_root/dist/user-session/"
    fi
    exit "$user_status"
fi
mkdir -p dist/user-session/packages
work_root=$(mktemp -d)
inhibit_pid=
cleanup()
{
    result=$?
    trap - EXIT HUP INT TERM
    if [ -n "$inhibit_pid" ]; then
        sudo kill "$inhibit_pid" 2>/dev/null || :
        for retry in 1 2 3; do
            sudo kill -0 "$inhibit_pid" 2>/dev/null || break
            sleep 1
        done
    fi
    rm -rf "$work_root"
    exit "$result"
}
trap cleanup EXIT
trap 'exit 129' HUP
trap 'exit 130' INT
trap 'exit 143' TERM
# Reuse the qualified native dependency; do not rebuild or modify it here.
# The package URL comes only from BSD3_CI_PKG_URL. There is no built-in mirror.
ck_sha=08d42eb5055f6cdf3dd91d3613c24f16fa91a33180b8bee7473e1cd737454fd9
ck_installed=$(pkg query '%v' consolekit2 2>/dev/null || :)
if [ -z "$ck_installed" ] || [ "$(pkg version -t "$ck_installed" 2.0.0_5)" = '<' ]; then
    pkg_url=${BSD3_CI_PKG_URL:-}
    case $pkg_url in
    https://*/*.pkg) ;;
    '')
        echo 'BSD3_CI_PKG_URL is unset; refusing to fetch consolekit2' >&2
        exit 1
        ;;
    *)
        echo 'BSD3_CI_PKG_URL must be an https URL ending in .pkg' >&2
        exit 1
        ;;
    esac
    case $pkg_url in
    *[!A-Za-z0-9._~:/?#%+=-]*)
        echo 'BSD3_CI_PKG_URL contains characters this script will not pass to fetch' >&2
        exit 1
        ;;
    esac
    pkg_rest=${pkg_url#https://}
    pkg_authority=${pkg_rest%%/*}
    case $pkg_authority in
    *@*)
        echo 'BSD3_CI_PKG_URL must not include userinfo' >&2
        exit 1
        ;;
    esac
    pkg_port=443
    case $pkg_authority in
    \[*)
        echo 'BSD3_CI_PKG_URL must use a DNS name, not an address literal' >&2
        exit 1
        ;;
    *:*)
        pkg_host=${pkg_authority%%:*}
        pkg_port=${pkg_authority#*:}
        ;;
    *)
        pkg_host=$pkg_authority
        ;;
    esac
    case $pkg_host in
    ''|-*|.*|*.|*..*|*[!A-Za-z0-9.-]*)
        echo 'BSD3_CI_PKG_URL host is not a DNS name' >&2
        exit 1
        ;;
    *[!0-9.]*) ;;
    *)
        echo 'BSD3_CI_PKG_URL must use a DNS name, not an address literal' >&2
        exit 1
        ;;
    esac
    case $pkg_port in
    ''|0*|*[!0-9]*)
        echo 'BSD3_CI_PKG_URL port is not a decimal port' >&2
        exit 1
        ;;
    esac
    if [ "$pkg_port" -gt 65535 ]; then
        echo 'BSD3_CI_PKG_URL port is out of range' >&2
        exit 1
    fi
    if [ -n "${BSD3_CI_DNS:-}" ]; then
        "${PYTHON:-python3}" -c 'import ipaddress,sys; ipaddress.ip_address(sys.argv[1])' "$BSD3_CI_DNS"
        # Query the configured resolver explicitly and scope the answer to this
        # curl invocation. Never modify /etc/resolv.conf, /etc/hosts or TLS policy.
        package_ip=$(dig +short +time=3 +tries=1 @"$BSD3_CI_DNS" \
            "$pkg_host" A | "${PYTHON:-python3}" -c '
import ipaddress,sys
answers=[]
for line in sys.stdin:
    try: ip=ipaddress.ip_address(line.strip())
    except ValueError: continue
    if ip.version==4 and ip.is_private: answers.append(str(ip))
if not answers: raise SystemExit("No private package address returned")
print(answers[0])')
        curl --fail --location --silent --show-error --connect-timeout 10 --max-time 60 \
            --resolve "${pkg_host}:${pkg_port}:${package_ip}" \
            -o "$work_root/consolekit2.pkg" "$pkg_url"
    else
        fetch -o "$work_root/consolekit2.pkg" "$pkg_url"
    fi
    [ "$(sha256 -q "$work_root/consolekit2.pkg")" = "$ck_sha" ]
    sudo pkg add -f "$work_root/consolekit2.pkg"
fi
[ -S /var/run/dbus/system_bus_socket ] || sudo service dbus onestart
cc -Wall -Wextra -Werror $(pkg-config --cflags gio-unix-2.0) \
    tests/native-inhibit.c $(pkg-config --libs gio-unix-2.0) \
    -o "$work_root/native-inhibit"
sudo daemon -f -o "$work_root/inhibit.log" -p "$work_root/inhibit.pid" \
    "$work_root/native-inhibit" 3600
for retry in 1 2 3 4 5; do
    [ ! -f "$work_root/inhibit.pid" ] || break
    sleep 1
done
inhibit_pid=$(sudo cat "$work_root/inhibit.pid")
for retry in 1 2 3 4 5; do
    if [ -f "$work_root/inhibit.log" ] && \
        sudo grep -q BOUNDED_INHIBITOR_READY "$work_root/inhibit.log"; then
        break
    fi
    sleep 1
done
sudo grep -q BOUNDED_INHIBITOR_READY "$work_root/inhibit.log"
sudo gdbus call --system --dest org.freedesktop.ConsoleKit \
    --object-path /org/freedesktop/ConsoleKit/Manager \
    --method org.freedesktop.ConsoleKit.Manager.ListInhibitors \
    >"$work_root/inhibitors.txt"
grep -q 'BSD3 VM tests' "$work_root/inhibitors.txt"
grep -q idle "$work_root/inhibitors.txt"
grep -Eq 'sleep|suspend' "$work_root/inhibitors.txt"
ports_root=${PORTSDIR:-/usr/ports}
if [ ! -f "$ports_root/Mk/bsd.port.mk" ]; then
    if [ -z "${PORTSDIR:-}" ]; then
        ports_root="$work_root/ports"
    fi
    git clone --depth 1 https://git.freebsd.org/ports.git "$ports_root"
fi
port="$work_root/overlay/sysutils/dinit"
mkdir -p "$port" "$work_root/distfiles"
cp -R packaging/freebsd/dinit/. "$port/"
make -C "$port" PORTSDIR="$ports_root" WRKDIRPREFIX="$work_root/work" \
    DISTDIR="$work_root/distfiles" PACKAGES="$repo_root/dist/user-session/packages" DEVELOPER=yes \
    stage test check-plist stage-qa package
# Dinit is unmodified upstream Apache-2.0 source plus this exact recipe.
# Preserve it before the private build root is removed.
cp "$work_root/distfiles/davmac314-dinit-v0.19.4_GH0.tar.gz" \
    dist/user-session/dinit-0.19.4-upstream.tar.gz
tar -C packaging/freebsd -czf dist/user-session/dinit-0.19.4-bsd3-port.tar.gz dinit \
    -C "$repo_root" tools/ci-user-session.sh tests/native-inhibit.c
(cd dist/user-session && sha256 -r dinit-0.19.4-upstream.tar.gz \
    dinit-0.19.4-bsd3-port.tar.gz) >dist/user-session/DINIT_SHA256SUMS
(cd dist/user-session/packages/All && sha256 -r dinit-0.19.4.pkg) \
    >>dist/user-session/DINIT_SHA256SUMS
sudo pkg add "$repo_root/dist/user-session/packages/All/dinit-0.19.4.pkg"
make clean user-session test-user-session
# Shell embeds the archive in a --no-undefined shared library. An executable
# link alone can hide dependencies supplied by FreeBSD's executable CRT.
cc -fPIC -shared -Wl,-z,defs -Iinclude tests/shared-user-session.c \
    libbsd3-user-session.a -pthread -o "$work_root/user-session-consumer.so"
if sudo ./bsd3-user-session /tmp boot; then
    echo 'root session unexpectedly allowed' >&2
    exit 1
else
    test "$?" -eq 77
fi
git clone --depth 1 --branch 49.0 https://gitlab.gnome.org/GNOME/gnome-session.git "$work_root/gnome-session"
sh tools/test-gnome49-util.sh "$work_root/gnome-session"
sh tools/test-gnome49-app.sh "$work_root/gnome-session"
make test-bus-ready
sh tools/test-gnome49-system.sh "$work_root/gnome-session"
sh tools/test-gnome49-manager.sh "$work_root/gnome-session"
PORTSDIR="$ports_root" BSD3_USERSESSION=yes sh tools/ci-port.sh
mkdir -p dist/user-session/bsd3
cp dist/packages/All/bsd-cubed-*.pkg dist/bsd-cubed-*.tar.gz \
    dist/SHA256SUMS dist/user-session/bsd3/
stage="$work_root/stage"
make DESTDIR="$stage" PREFIX=/usr/local install-user-session
test -x "$stage/usr/local/bin/bsd3-user-session"
test -f "$stage/usr/local/include/bsd3/user-session.h"
test -f "$stage/usr/local/libdata/pkgconfig/bsd3-user-session.pc"
{
    freebsd-version -kru
    cc --version
    git rev-parse HEAD
    git status --short
    git diff --stat
    git -c safe.directory="$ports_root" -C "$ports_root" rev-parse HEAD
    git -C "$work_root/gnome-session" rev-parse HEAD
    pkg query '%n %v' glib dbus gnome-desktop dinit
} >dist/user-session/provenance.txt
