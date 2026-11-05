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
mkdir -p "$work_root/overlay/sysutils/bsd-cubed" "$work_root/source/bsd-cubed-0.1.0" "$work_root/distfiles" "$repo_root/dist/packages"
cp packaging/freebsd/bsd-cubed/* "$work_root/overlay/sysutils/bsd-cubed/"
# Explicit source inputs: no .git, private captures, generated binaries or secrets.
for input in Makefile LICENSE README.md src include man tests; do
    cp -R "$input" "$work_root/source/bsd-cubed-0.1.0/"
done
tar -C "$work_root/source" -czf "$work_root/distfiles/bsd-cubed-0.1.0.tar.gz" bsd-cubed-0.1.0
port="$work_root/overlay/sysutils/bsd-cubed"
# This checksum identifies the CI snapshot, not an unpublished upstream release.
make -C "$port" PORTSDIR="$ports_root" DISTDIR="$work_root/distfiles" \
    WRKDIRPREFIX="$work_root/work" PACKAGES="$repo_root/dist/packages" \
    MASTER_SITES="file://$work_root/distfiles/" makesum
make -C "$port" PORTSDIR="$ports_root" DISTDIR="$work_root/distfiles" \
    WRKDIRPREFIX="$work_root/work" PACKAGES="$repo_root/dist/packages" \
    MASTER_SITES="file://$work_root/distfiles/" DEVELOPER=yes \
    stage test check-plist stage-qa package
test -f "$repo_root/dist/packages/All/bsd-cubed-0.1.0.pkg"
cp "$port/distinfo" "$repo_root/dist/snapshot.distinfo"
git -C "$ports_root" rev-parse HEAD >"$repo_root/dist/ports-revision.txt"
find "$repo_root/dist/packages" -name '*.pkg' -exec sha256 {} \; >"$repo_root/dist/package-sha256.txt"
