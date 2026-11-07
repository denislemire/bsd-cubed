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
for input in Makefile LICENSE README.md THIRD_PARTY.md src include man tests packaging/bsd3-login.pc.in packaging/bsd3-user-session.pc.in packaging/bsd3-sddm-session.in packaging/sddm-wayland.conf.in packaging/gnome49; do
    if [ "${input#packaging/}" != "$input" ]; then
        mkdir -p "$source_root/packaging"
        cp -R "$input" "$source_root/packaging/"
    else
        cp -R "$input" "$source_root/"
    fi
done
tar -C "$work_root/source" -czf "$work_root/distfiles/$distname.tar.gz" "$distname"
# This checksum identifies the CI snapshot, not an unpublished upstream release.
if [ "$(id -u)" -eq 0 ]; then
    # USERSESSION tests deliberately reject root. Keep their full ports target
    # sequence and run only this private, credential-free build as an actor.
    actor=${BSD3_PORTS_TEST_USER:-circleci}
    case "$actor" in ''|-*|*[!A-Za-z0-9_.-]*) echo 'Invalid ports test user' >&2; exit 64 ;; esac
    ports_root=$(CDPATH= cd -- "$ports_root" && pwd)
    actor_uid=$(id -u "$actor")
    actor_gid=$(id -g "$actor")
    [ "$actor_uid" -gt 0 ] || { echo 'Ordinary ports test user required' >&2; exit 77; }
    mkdir -p "$work_root/packages" "$work_root/home"
    cat >"$work_root/build-port.sh" <<'BUILD'
#!/bin/sh
set -eu
port=$1
ports_root=$2
build_root=$3
option_args=$4
[ "$(id -u)" -gt 0 ] || exit 77
make -C "$port" PORTSDIR="$ports_root" DISTDIR="$build_root/distfiles" \
    WRKDIRPREFIX="$build_root/work" PACKAGES="$build_root/packages" \
    MASTER_SITES="file://$build_root/distfiles/" "$option_args" makesum
make -C "$port" PORTSDIR="$ports_root" DISTDIR="$build_root/distfiles" \
    WRKDIRPREFIX="$build_root/work" PACKAGES="$build_root/packages" \
    MASTER_SITES="file://$build_root/distfiles/" DEVELOPER=yes "$option_args" \
    stage test check-plist stage-qa package
BUILD
    chown -R "$actor_uid:$actor_gid" "$work_root"
    # The checkout may be under /root. Enter an actor-accessible directory
    # before sudo; the child never needs access to the controller checkout.
    cd "$work_root"
    sudo -H -u "$actor" /usr/bin/env -i HOME="$work_root/home" \
        USER="$actor" LOGNAME="$actor" PATH="$PATH" LANG=C.UTF-8 \
        /bin/sh "$work_root/build-port.sh" "$port" "$ports_root" "$work_root" "$option_args"
    cd "$repo_root"
    package="$work_root/packages/All/$pkgname.pkg"
    [ -f "$package" ] && [ ! -L "$package" ] || { echo 'Regular package artifact required' >&2; exit 1; }
    mkdir -p "$repo_root/dist/packages/All"
    cp "$package" "$repo_root/dist/packages/All/$pkgname.pkg"
else
    make -C "$port" PORTSDIR="$ports_root" DISTDIR="$work_root/distfiles" \
        WRKDIRPREFIX="$work_root/work" PACKAGES="$repo_root/dist/packages" \
        MASTER_SITES="file://$work_root/distfiles/" $option_args makesum
    make -C "$port" PORTSDIR="$ports_root" DISTDIR="$work_root/distfiles" \
        WRKDIRPREFIX="$work_root/work" PACKAGES="$repo_root/dist/packages" \
        MASTER_SITES="file://$work_root/distfiles/" DEVELOPER=yes $option_args \
        stage test check-plist stage-qa package
fi
test -f "$repo_root/dist/packages/All/$pkgname.pkg"
cp "$port/distinfo" "$repo_root/dist/snapshot.distinfo"
cp "$work_root/distfiles/$distname.tar.gz" "$repo_root/dist/"
git -c safe.directory="$ports_root" -C "$ports_root" rev-parse HEAD >"$repo_root/dist/ports-revision.txt"
# Names match flattened GitHub release assets, without machine-specific paths.
(cd "$repo_root/dist/packages/All" && sha256 -r "$pkgname.pkg") >"$repo_root/dist/SHA256SUMS"
(cd "$repo_root/dist" && sha256 -r "$distname.tar.gz") >>"$repo_root/dist/SHA256SUMS"
