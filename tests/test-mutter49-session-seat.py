#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-2-Clause
"""Run actual Mutter49 seat-proxy consumer with its own generated D-Bus API."""
from pathlib import Path
import os
import shlex
import subprocess
import sys
import tempfile

if len(sys.argv) != 2:
    raise SystemExit('usage: test-mutter49-session-seat.py /path/to/mutter-49.8')
repo = Path(__file__).resolve().parent.parent
root = Path(sys.argv[1]).resolve(strict=True)
source = (root / 'src/backends/meta-launcher.c').read_text()
a = source.index('static MetaDBusLogin1Seat *\nget_seat_proxy (')
b = source.index('\nstatic void\nsync_active (', a)
consumer = source[a:b]
# Read the upstream GPL consumer and XML only at runtime; neither implementation
# nor generated translation is copied into the project's BSD source files.
main = r'''
static void mode(GDBusConnection *bus, const char *value) {
    GError *error = NULL;
    GVariant *reply = g_dbus_connection_call_sync(bus, "org.freedesktop.ConsoleKit",
        "/org/freedesktop/ConsoleKit/Manager", "org.bsd3.Test", "SetMode",
        g_variant_new("(s)", value), G_VARIANT_TYPE("()"), 0, 3000, NULL, &error);
    g_assert_no_error(error); g_assert_nonnull(reply); g_variant_unref(reply);
}
int main(void) {
    const char *modes[] = {"normal", "legacy-seat", "custom-seat-name",
                           "underscore-seat-name", "numeric-seat-name", "no-seat"};
    const char *names[] = {"seat0", "seat0", "seat-lab", "seat_lab", "1seat", NULL};
    const char *paths[] = {"/org/freedesktop/login1/seat/seat0",
        "/org/freedesktop/login1/seat/seat0", "/org/freedesktop/login1/seat/seat_2dlab",
        "/org/freedesktop/login1/seat/seat_5flab", "/org/freedesktop/login1/seat/_31seat", "/"};
    GError *error = NULL;
    GDBusConnection *bus = g_bus_get_sync(G_BUS_TYPE_SYSTEM, NULL, &error);
    g_assert_no_error(error); g_assert_nonnull(bus);
    for (guint i = 0; i < G_N_ELEMENTS(modes); i++) {
        MetaDBusLogin1Session *session;
        MetaDBusLogin1Seat *seat;
        mode(bus, modes[i]);
        session = meta_dbus_login1_session_proxy_new_for_bus_sync(G_BUS_TYPE_SYSTEM,
            G_DBUS_PROXY_FLAGS_DO_NOT_AUTO_START, "org.freedesktop.login1",
            "/org/freedesktop/login1/session/Session1", NULL, &error);
        g_assert_no_error(error); g_assert_nonnull(session);
        /* Old BSD3 omits Seat from GetAll, making the actual49 launcher go
         * headless. Refuse that absence before invoking its unguarded parser. */
        g_assert_nonnull(meta_dbus_login1_session_get_seat(session));
        seat = get_seat_proxy(session, NULL, &error);
        g_assert_no_error(error); g_assert_nonnull(seat);
        g_assert_cmpstr(g_dbus_proxy_get_object_path(G_DBUS_PROXY(seat)), ==, paths[i]);
        g_assert_cmpstr(meta_dbus_login1_seat_get_id(seat), ==, names[i]);
        g_object_unref(seat); g_object_unref(session);
    }
    mode(bus, "normal");
    g_object_unref(bus);
    g_print("PASS: actual Mutter49 GetAll Seat tuple / get_seat_proxy / Seat.Id, legacy/custom/escaped seats, explicit no-seat\n");
    return 0;
}
'''
flags = shlex.split(subprocess.check_output(
    ['pkg-config', '--cflags', '--libs', 'gio-unix-2.0'], text=True))
subprocess.run(['make', '-C', str(repo), 'bsddd', 'mock-ck'], check=True)
with tempfile.TemporaryDirectory(prefix='bsd3-mutter49-seat-') as tmp:
    work = Path(tmp)
    generated = work / 'meta-dbus-login1'
    subprocess.run(['gdbus-codegen', '--generate-c-code', str(generated),
                    '--interface-prefix', 'org.freedesktop.', '--c-namespace', 'MetaDBus',
                    str(root / 'data/dbus-interfaces/org.freedesktop.login1.xml')], check=True)
    code = work / 'consumer.c'
    code.write_text('#include <gio/gio.h>\n#include "meta-dbus-login1.h"\n' + consumer + main)
    executable = work / 'consumer'
    subprocess.run([os.environ.get('CC', 'cc'), '-std=gnu11', '-Wall', '-Wextra',
                    '-Werror=implicit-function-declaration', '-I' + str(work), str(code),
                    str(generated.with_suffix('.c')), *flags, '-o', str(executable)], check=True)
    fixture = r'''
set -eu
ulimit -c 0
cd "$1"
export DBUS_SYSTEM_BUS_ADDRESS="$DBUS_SESSION_BUS_ADDRESS"
mock_pid=
daemon_pid=
cleanup() {
    result=$?
    trap - EXIT HUP INT TERM
    [ -z "$daemon_pid" ] || kill "$daemon_pid" 2>/dev/null || :
    [ -z "$mock_pid" ] || kill "$mock_pid" 2>/dev/null || :
    [ -z "$daemon_pid" ] || wait "$daemon_pid" || :
    [ -z "$mock_pid" ] || wait "$mock_pid" || :
    exit "$result"
}
trap cleanup EXIT
trap 'exit 129' HUP
trap 'exit 130' INT
trap 'exit 143' TERM
./mock-ck >/dev/null 2>&1 & mock_pid=$!
gdbus wait --system --timeout 5 org.freedesktop.ConsoleKit
"${BSD3_TEST_SEAT_DAEMON:-./bsddd}" --foreground >/dev/null 2>&1 & daemon_pid=$!
gdbus wait --system --timeout 5 org.freedesktop.login1
"$2"
'''
    subprocess.run(['dbus-run-session', '--config-file=' + str(repo / 'tests/dbus.conf'),
                    '--', 'sh', '-s', str(repo), str(executable)],
                   input=fixture, text=True, check=True, timeout=30)
