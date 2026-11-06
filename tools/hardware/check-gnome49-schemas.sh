#!/bin/sh
# SPDX-License-Identifier: BSD-2-Clause
# Validate an installed root, including a mounted inactive boot environment.
# pkg -r may leave derived caches stale; refresh explicitly inside that root.
set -eu
[ "$(uname -s)" = FreeBSD ] && [ "$(id -u)" -eq 0 ]
[ "$#" -eq 2 ] || { echo "usage: $0 root --check|--refresh" >&2; exit 64; }
root=$(realpath "$1")
prefix=${PREFIX:-/usr/local}
case "$prefix" in /*) ;; *) exit 64 ;; esac
case "$2" in
    --refresh) chroot "$root" "$prefix/bin/glib-compile-schemas" "$prefix/share/glib-2.0/schemas" ;;
    --check) ;;
    *) exit 64 ;;
esac
chroot "$root" /usr/bin/env BSD3_SCHEMA_DIR="$prefix/share/glib-2.0/schemas" \
    "$prefix/bin/gjs" -c '
const {Gio, GLib} = imports.gi;
const source = Gio.SettingsSchemaSource.new_from_directory(GLib.getenv("BSD3_SCHEMA_DIR"), null, false);
const schema = source.lookup("org.gnome.shell.keybindings", false);
if (!schema) throw new Error("GNOME Shell keybinding schema absent from compiled cache");
for (const key of ["screen-brightness-up", "screen-brightness-up-monitor", "screen-brightness-down", "screen-brightness-down-monitor", "screen-brightness-cycle", "screen-brightness-cycle-monitor"])
    if (!schema.has_key(key)) throw new Error(`GNOME49 compiled schema key missing: ${key}`);
print("PASS: installed GNOME49 compiled brightness keybindings");'
