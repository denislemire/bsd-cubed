/* SPDX-License-Identifier: BSD-2-Clause */
#include <gio/gio.h>
#include <unistd.h>

/* Run only after the compositor and required desktop services report ready. */
int main(int argc, char **argv)
{
    GError *error = NULL;
    GDBusConnection *bus;
    GVariant *reply;
    (void)argv;
    if (!getuid() || geteuid() != getuid()) return 77;
    if (argc != 1) return 64;
    bus = g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, &error);
    if (!bus) goto failed;
    reply = g_dbus_connection_call_sync(bus, "org.gnome.SessionManager",
        "/org/gnome/SessionManager", "org.gnome.SessionManager", "Initialized",
        NULL, G_VARIANT_TYPE_UNIT, G_DBUS_CALL_FLAGS_NO_AUTO_START, 3000, NULL, &error);
    g_object_unref(bus);
    if (!reply) goto failed;
    g_variant_unref(reply);
    return 0;
failed:
    g_printerr("Cannot initialize GNOME session: %s\n", error->message);
    g_error_free(error);
    return 1;
}
