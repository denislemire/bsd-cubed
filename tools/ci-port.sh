#!/bin/sh
# SPDX-License-Identifier: BSD-2-Clause
# Local source snapshot through the real ports framework. No package publishing.
set -eu
export PATH=/usr/local/sbin:/usr/local/bin:/sbin:/bin:/usr/sbin:/usr/bin
[ "$(uname -s)" = FreeBSD ] || { echo 'FreeBSD required' >&2; exit 1; }
repo_root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
cd "$repo_root"
work_root=$(mktemp -d)
trap 'rm -rf "$work_root"' EXIT HUP INT TERM
ports_root=${PORTSDIR:-/usr/ports}
if [ ! -f "$ports_root/Mk/bsd.port.mk" ]; then
    ports_root="$work_root/ports"
    git clone --depth 1 https://git.freebsd.org/ports.git "$ports_root"
fi
port="$work_root/overlay/sysutils/bsd-cubed"
mkdir -p "$port" "$work_root/distfiles" "$repo_root/dist/packages"
cp -R packaging/freebsd/bsd-cubed/. "$port/"
option_args=OPTIONS_UNSET=USERSESSION
if [ "${BSD3_USERSESSION:-no}" = yes ]; then
    option_args=OPTIONS_SET=USERSESSION
fi
distname=$(make -C "$port" PORTSDIR="$ports_root" -V DISTNAME)
pkgname=$(make -C "$port" PORTSDIR="$ports_root" -V PKGNAME)
# Limit framework-derived names before using them as filesystem components.
case "$distname:$pkgname" in *[!A-Za-z0-9_.:+-]*|:*|*:) exit 1 ;; esac
source_root="$work_root/source/$distname"
mkdir -p "$source_root"
# Explicit source inputs: no .git, private captures, generated binaries or secrets.
for input in Makefile LICENSE README.md src include man tests packaging/bsd3-login.pc.in packaging/bsd3-user-session.pc.in packaging/bsd3-sddm-session.in packaging/sddm-wayland.conf.in; do
    if [ "${input#packaging/}" != "$input" ]; then
        mkdir -p "$source_root/packaging"
        cp "$input" "$source_root/packaging/"
    else
        cp -R "$input" "$source_root/"
    fi
done
tar -C "$work_root/source" -czf "$work_root/distfiles/$distname.tar.gz" "$distname"
# This checksum identifies the CI snapshot, not an unpublished upstream release.
make -C "$port" PORTSDIR="$ports_root" DISTDIR="$work_root/distfiles" \
    WRKDIRPREFIX="$work_root/work" PACKAGES="$repo_root/dist/packages" \
    MASTER_SITES="file://$work_root/distfiles/" $option_args makesum
make -C "$port" PORTSDIR="$ports_root" DISTDIR="$work_root/distfiles" \
    WRKDIRPREFIX="$work_root/work" PACKAGES="$repo_root/dist/packages" \
    MASTER_SITES="file://$work_root/distfiles/" DEVELOPER=yes $option_args \
    stage test check-plist stage-qa package
test -f "$repo_root/dist/packages/All/$pkgname.pkg"
cp "$port/distinfo" "$repo_root/dist/snapshot.distinfo"
cp "$work_root/distfiles/$distname.tar.gz" "$repo_root/dist/"
git -C "$ports_root" rev-parse HEAD >"$repo_root/dist/ports-revision.txt"
# Names match flattened GitHub release assets, without machine-specific paths.
(cd "$repo_root/dist/packages/All" && sha256 -r "$pkgname.pkg") >"$repo_root/dist/SHA256SUMS"
(cd "$repo_root/dist" && sha256 -r "$distname.tar.gz") >>"$repo_root/dist/SHA256SUMS"
