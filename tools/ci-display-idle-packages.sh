#!/bin/sh
# SPDX-License-Identifier: BSD-2-Clause
# Focused native FreeBSD build for the package-owned display-idle stack.
set -eu
export PATH=/usr/local/sbin:/usr/local/bin:/sbin:/bin:/usr/sbin:/usr/bin
[ "$(uname -s)" = FreeBSD ] && [ "$(id -u)" -eq 0 ] && [ "${CIRCLECI:-}" = true ] || exit 1

repo=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
cd "$repo"
out="$repo/dist/display-idle"
mkdir -p "$out/packages/All"
work=$(mktemp -d)
build_pid=
finish() {
    status=$?
    trap - EXIT HUP INT TERM
    if [ -n "$build_pid" ] && kill -0 "$build_pid" 2>/dev/null; then
        kill "$build_pid" 2>/dev/null || :
        wait "$build_pid" 2>/dev/null || :
    fi
    if [ -d "$work/build" ]; then
        find "$work/build" -name meson-log.txt -exec cp {} "$out/last-meson-log.txt" \;
    fi
    rm -rf "$work"
    exit "$status"
}
trap finish EXIT HUP INT TERM

revision=$(cat packaging/freebsd/gnome-settings-daemon49/PORTS_REVISION)
[ "$(cat packaging/freebsd/gsettings-desktop-schemas49/PORTS_REVISION)" = "$revision" ]
case "$revision" in *[!0-9a-f]*|'') exit 1;; esac
[ "${#revision}" -eq 40 ]
ports=${BSD3_CI_PORTS_ROOT:-"$work/ports"}
sh tools/ci-ports-tree.sh "$ports" "$revision"

for pair in \
    packaging/freebsd/dinit:sysutils/dinit \
    packaging/freebsd/bsd-cubed:sysutils/bsd-cubed \
    packaging/freebsd/gsettings-desktop-schemas49:devel/gsettings-desktop-schemas \
    packaging/freebsd/gnome-settings-daemon49:sysutils/gnome-settings-daemon
do
    source=${pair%%:*}
    origin=${pair#*:}
    mkdir -p "$ports/${origin%/*}"
    rm -rf "$ports/$origin"
    cp -R "$source" "$ports/$origin"
done

install_dependencies() {
    port=$1
    receipt=$2
    make -C "$port" PORTSDIR="$ports" BATCH=yes DEPENDS_SHOW_FLAVOR=yes \
        build-depends-list run-depends-list | sort -u >"$receipt.origins"
    : >"$receipt.packages"
    while IFS= read -r dependency; do
        case "$dependency" in "$ports"/*) ;; *) exit 1;; esac
        flavor=
        case "$dependency" in *@*) flavor=${dependency##*@}; dependency=${dependency%@*};; esac
        if [ -n "$flavor" ]; then
            case "$flavor" in *[!A-Za-z0-9_+-]*|'') exit 1;; esac
            package=$(make -C "$dependency" PORTSDIR="$ports" BATCH=yes \
                FLAVOR="$flavor" -V PKGBASE)
        else
            package=$(make -C "$dependency" PORTSDIR="$ports" BATCH=yes -V PKGBASE)
        fi
        case "$package" in *[!A-Za-z0-9_+.-]*|'') exit 1;; esac
        pkg info -e "$package" >/dev/null 2>&1 || printf '%s\n' "$package" >>"$receipt.packages"
    done <"$receipt.origins"
    sort -u "$receipt.packages" -o "$receipt.packages"
    [ ! -s "$receipt.packages" ] || xargs pkg install -yA <"$receipt.packages"
}

build_port() {
    origin=$1
    key=$2
    port="$ports/$origin"
    log="$out/$key-build.log"
    install_dependencies "$port" "$out/$key-dependencies"
    printf 'Building %s; full output is retained in %s\n' "$origin" "${log#$repo/}"
    make -C "$port" PORTSDIR="$ports" BATCH=yes DEVELOPER=yes \
        WRKDIRPREFIX="$work/build" PACKAGES="$out/packages" MAKE_JOBS_NUMBER=3 \
        stage test check-plist stage-qa package >"$log" 2>&1 &
    build_pid=$!
    while kill -0 "$build_pid" 2>/dev/null; do
        sleep 30
        kill -0 "$build_pid" 2>/dev/null || break
        printf 'Still building %s (%s lines logged)\n' "$origin" "$(wc -l <"$log")"
    done
    if wait "$build_pid"; then
        build_status=0
    else
        build_status=$?
    fi
    build_pid=
    if [ "$build_status" -ne 0 ]; then
        tail -100 "$log"
        exit 1
    fi
    ! grep -q '^Error:' "$log"
    source=$(make -C "$port" PORTSDIR="$ports" WRKDIRPREFIX="$work/build" -V WRKSRC)
    source_name=${source##*/}
    tar --exclude "$source_name/_build" -C "${source%/*}" -czf \
        "$out/$source_name-bsd3-source.tar.gz" "$source_name"
    tar -C "$ports" -czf "$out/$source_name-bsd3-port.tar.gz" "$origin" \
        -C "$repo" tools/ci-display-idle-packages.sh tools/ci-ports-tree.sh
}

{
    freebsd-version -kru
    uname -v
    cc --version
    git rev-parse HEAD
    printf 'ports=%s\n' "$revision"
} >"$out/provenance.txt"

# The main BSD3 port requires the already-qualified ConsoleKit device and
# session semantics, not the stock package. Its existing CI controller built,
# tested and installed this exact package in the preceding step.
set -- dist/consolekit/packages/All/consolekit2-*.pkg
[ "$#" -eq 1 ] && [ -f "$1" ]
[ "$(pkg query '%v' consolekit2)" = 2.0.0_5 ]
cp "$1" "$out/packages/All/"
cp dist/consolekit/consolekit2-bsd3-source.tar.gz \
    dist/consolekit/consolekit2-bsd3-port.tar.gz \
    dist/consolekit/ports-revision.txt "$out/"

# Dinit is not in the stock FreeBSD ports tree. Build and install the reviewed
# BSD3 consumer recipe before the alpha4 package exercises its user graph.
build_port sysutils/dinit dinit
set -- "$out"/packages/All/dinit-*.pkg
[ "$#" -eq 1 ] && [ -f "$1" ]
pkg -o REPOS_DIR=/var/empty add -f "$1"
[ "$(pkg query '%v' dinit)" = 0.19.4 ]

# Build the exact BSD3 source snapshot first; it owns the service descriptor and
# greeter helper. Keep its source archive beside the consumer packages.
PORTSDIR="$ports" BSD3_USERSESSION=yes sh tools/ci-port.sh
cp dist/packages/All/bsd-cubed-*.pkg "$out/packages/All/"
cp dist/bsd-cubed-*.tar.gz dist/snapshot.distinfo "$out/"
set -- "$out"/packages/All/bsd-cubed-*.pkg
[ "$#" -eq 1 ] && [ -f "$1" ]
pkg -o REPOS_DIR=/var/empty add -f "$1"
pkg lock -y bsd-cubed
grep -q 'BSD3_IDLE_DISPLAY=1' /usr/local/share/bsd-cubed/gnome49/services/gsd-power
test -x /usr/local/libexec/bsd3-sddm-greeter-idle

build_port devel/gsettings-desktop-schemas gsettings-desktop-schemas
set -- "$out"/packages/All/gsettings-desktop-schemas-*.pkg
[ "$#" -eq 1 ] && [ -f "$1" ]
pkg -o REPOS_DIR=/var/empty add -f "$1"
pkg lock -y gsettings-desktop-schemas
[ "$(pkg query '%v' gsettings-desktop-schemas)" = 49.1 ]

build_port sysutils/gnome-settings-daemon gnome-settings-daemon
set -- "$out"/packages/All/gnome-settings-daemon-*.pkg
[ "$#" -eq 1 ] && [ -f "$1" ]
[ "$(pkg query -F "$1" '%v')" = 49.1_1 ]
pkg -o REPOS_DIR=/var/empty add -f "$1"
strings /usr/local/libexec/gsd-power | grep -q BSD3_IDLE_DISPLAY
strings /usr/local/libexec/gsd-power | grep -q handle-lid-switch
pkg check -s consolekit2 dinit bsd-cubed gsettings-desktop-schemas gnome-settings-daemon

pkg query '%n %v %o' consolekit2 dinit bsd-cubed gsettings-desktop-schemas gnome-settings-daemon \
    >>"$out/provenance.txt"
printf 'PASS: package-owned GNOME idle, display-only lid, and greeter stack\n' >"$out/complete.txt"
(cd "$out" && find . -type f ! -name SHA256SUMS -print | sort | \
    while IFS= read -r file; do sha256 -r "${file#./}"; done) >"$out/SHA256SUMS"
