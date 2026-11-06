#!/bin/sh
# SPDX-License-Identifier: BSD-2-Clause
# Opt-in configuration for an already installed and validated BSD3/SDDM desktop.
set -eu
export PATH=/usr/local/sbin:/usr/local/bin:/sbin:/bin:/usr/sbin:/usr/bin
case "${1:-}" in --check|--enable) mode=$1 ;; *) echo "usage: $0 --check|--enable" >&2; exit 64 ;; esac
[ "$#" -eq 1 ] && [ "$(uname -s)" = FreeBSD ]
for file in /usr/local/etc/rc.d/bsddd /usr/local/etc/rc.d/dbus /usr/local/etc/rc.d/sddm; do
    [ -x "$file" ] || { echo "Missing installed service: $file" >&2; exit 1; }
done
for other in gdm lightdm xdm slim; do
    value=$(sysrc -n "${other}_enable" 2>/dev/null || :)
    case "$value" in [Yy][Ee][Ss]) echo "Disable conflicting login manager: $other" >&2; exit 1 ;; esac
done
order=$(mktemp -t bsd3-rcorder)
trap 'rm -f "$order"' EXIT
rcorder /etc/rc.d/* /usr/local/etc/rc.d/* > "$order"
awk '
/\/dbus$/ {bus=NR}
/\/bsddd$/ {broker=NR}
/\/sddm$/ {greeter=NR}
END {
    if (!(bus && broker && greeter && bus < broker && broker < greeter)) {
        print "Invalid boot order: require dbus before bsddd before sddm" > "/dev/stderr"
        exit 1
    }
    print "PASS: native rcorder starts dbus, bsddd, then sddm"
}' "$order"
if [ "$mode" = --enable ]; then
    [ "$(id -u)" -eq 0 ] || { echo 'Enabling startup requires root' >&2; exit 1; }
    # The administrator chooses the boot environment; this tool never activates one.
    backup=$(mktemp -d /var/backups/bsd3-desktop.XXXXXX)
    chmod 700 "$backup"
    cp -p /etc/rc.conf "$backup/rc.conf"
    sysrc dbus_enable=YES bsddd_enable=YES sddm_enable=YES
    printf 'Saved pre-change rc.conf: %s/rc.conf\n' "$backup"
    echo 'Desktop startup enabled. No session interrupted; verify with a guarded reboot.'
fi
