#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-2-Clause
# Compile the actual patched GPL consumer function, preserving its source header.
import pathlib, subprocess, sys, tempfile
source = pathlib.Path(sys.argv[1]).read_text()
start = source.index("static int\nfreebsd_seat_can_graphical (")
end = source.index("\n}\n", start) + 3
header = source[:source.index('#include')]
fixture = r"""
#include <gio/gio.h>
#include <errno.h>
#include <assert.h>
#include <string.h>
#define CK_NAME "org.freedesktop.ConsoleKit"
#define CK_MANAGER_PATH "/org/freedesktop/ConsoleKit/Manager"
#define CK_MANAGER_INTERFACE "org.freedesktop.ConsoleKit.Manager"
typedef struct { GDBusConnection *connection; } GdmLocalDisplayFactory;
static int scenario, capability, calls;
static GVariant *test_call(GDBusConnection *connection, const gchar *bus,
 const gchar *path, const gchar *iface, const gchar *method, GVariant *params,
 const GVariantType *type, GDBusCallFlags flags, gint timeout,
 GCancellable *cancel, GError **error)
{
 (void)connection;(void)params;(void)cancel;(void)error;
 assert(!strcmp(bus, CK_NAME) && !strcmp(path, CK_MANAGER_PATH));
 assert(!strcmp(iface, CK_MANAGER_INTERFACE) && !strcmp(method,"ListSeats"));
 assert(g_variant_type_equal(type,G_VARIANT_TYPE("(a(so))")));
 assert(flags==G_DBUS_CALL_FLAGS_NO_AUTO_START && timeout==3000);
 if(scenario==1)return NULL;
 GVariantBuilder b;g_variant_builder_init(&b,G_VARIANT_TYPE("a(so)"));
 g_variant_builder_add(&b,"(so)","other","/org/freedesktop/ConsoleKit/seat0");
 if(scenario!=2)g_variant_builder_add(&b,"(so)","seat0","/org/freedesktop/ConsoleKit/custom");
 if(scenario==3)g_variant_builder_add(&b,"(so)","seat0","/org/freedesktop/ConsoleKit/duplicate");
 return g_variant_ref_sink(g_variant_new("(a(so))",&b));
}
static int sd_seat_can_graphical(const char *path)
{ assert(!strcmp(path,"/org/freedesktop/ConsoleKit/custom"));calls++;return capability; }
#define g_dbus_connection_call_sync test_call
"""
main = r"""
int main(void) {
 GdmLocalDisplayFactory f={NULL};
 for(capability=-EIO;capability<=1;capability++) {
  scenario=0;calls=0;
  assert(freebsd_seat_can_graphical(&f,"seat0")==capability && calls==1);
 }
 const int expected[]={0,-EIO,-ENOENT,-EINVAL};
 for(scenario=1;scenario<=3;scenario++) {
  calls=0;assert(freebsd_seat_can_graphical(&f,"seat0")==expected[scenario] && calls==0);
 }
 return 0;
}
"""
with tempfile.TemporaryDirectory(prefix="bsd3-gdm-seat-") as directory:
    root = pathlib.Path(directory)
    (root / "fixture.c").write_text(header + fixture + source[start:end] + main)
    flags = subprocess.check_output(["pkg-config", "--cflags", "--libs", "gio-2.0"], text=True).split()
    subprocess.run(["cc", "-std=c11", "-Wall", "-Wextra", "-Werror", str(root / "fixture.c"), "-o", str(root / "fixture"), *flags], check=True)
    subprocess.run([str(root / "fixture")], check=True)
print("PASS: actual GDM seat lookup, advertised path, missing/duplicate/error, capability propagation")
