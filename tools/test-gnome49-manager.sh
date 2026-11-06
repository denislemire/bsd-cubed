#!/bin/sh
# SPDX-License-Identifier: BSD-2-Clause
# Diagnostic manager build, not a replacement GNOME package/desktop launcher.
set -eu
[ "$(uname -s)" = FreeBSD ] || { echo 'native FreeBSD required' >&2; exit 1; }
[ "$#" -eq 1 ] || { echo "usage: $0 /path/to/gnome-session-49-checkout" >&2; exit 64; }
source_root=$(CDPATH= cd -- "$1" && pwd)
repo_root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
[ "$(git -C "$source_root" rev-parse HEAD)" = 776dc42864ea233f732e0b5b72cc08b1d8664fdb ]
git -C "$source_root" diff --quiet
build_root=$(mktemp -d)
trap 'rm -rf "$build_root"' EXIT HUP INT TERM
git -C "$source_root" archive HEAD | tar -xf - -C "$build_root"
cd "$build_root"
patch -p1 < "$repo_root/packaging/freebsd/gnome-session49/native-manager.patch"
cat > config.h <<'EOF'
#define HAVE_BSD3_USER_SESSION 1
#define BSD3_DIAGNOSTIC_NULL_SYSTEM 1
#define GETTEXT_PACKAGE "gnome-session-49"
#define PACKAGE_VERSION "49.0"
EOF
generated=
for iface in org.gnome.SessionManager org.gnome.SessionManager.ClientPrivate org.gnome.SessionManager.Inhibitor org.gnome.SessionManager.Presence; do
    gdbus-codegen --generate-c-code "$iface" --interface-prefix "$iface." \
        --c-namespace Gsm "gnome-session/$iface.xml"
    generated="$generated $iface.c"
done
sources=
for name in gsm-app gsm-client gsm-inhibitor gsm-manager gsm-presence gsm-session-fill gsm-shell gsm-store gsm-system gsm-util service-main; do
    sources="$sources gnome-session/$name.c"
done
# The selected upstream null backend advertises no supported power operations.
# Its use here is diagnostic only; no privileged operation is emulated.
cc -DHAVE_CONFIG_H -std=gnu11 -Wall -Wextra -Werror=implicit-function-declaration \
    -DLOCALE_DIR='"/usr/local/share/locale"' \
    -I. -Ignome-session -I"$repo_root/include" \
    $(pkg-config --cflags gio-unix-2.0 gnome-desktop-4) \
    $sources $generated "$repo_root/libbsd3-user-session.a" -pthread \
    $(pkg-config --libs gio-unix-2.0 gnome-desktop-4) \
    -o "$build_root/gnome-session-service"
mkdir -p "$build_root/schemas"
cp data/org.gnome.SessionManager.gschema.xml "$build_root/schemas/"
glib-compile-schemas "$build_root/schemas"
cd "$repo_root"
dbus-run-session --config-file=tests/dbus.conf -- \
    "${PYTHON:-python3}" tests/test-gnome49-manager.py "$build_root/gnome-session-service" \
    "$build_root/schemas"
