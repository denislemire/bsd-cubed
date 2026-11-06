/* SPDX-License-Identifier: BSD-2-Clause */
#include <gio/gio.h>
#include <glib-unix.h>
#include <signal.h>
#include <unistd.h>
static void method(GDBusConnection *bus, const gchar *sender, const gchar *path,
                   const gchar *iface, const gchar *name, GVariant *args,
                   GDBusMethodInvocation *inv, gpointer data)
{
    (void)sender; (void)path; (void)iface; (void)name; (void)args; (void)data;
    g_dbus_connection_emit_signal(bus, NULL, "/org/gnome/SessionManager",
        "org.gnome.SessionManager", "SessionOver", NULL, NULL);
    g_dbus_method_invocation_return_value(inv, NULL);
}
static void acquired(GDBusConnection *bus, const gchar *name, gpointer data)
{
    GDBusNodeInfo *info = data;
    static const GDBusInterfaceVTable vtable = {method, NULL, NULL, {NULL}};
    (void)name;
    g_dbus_connection_register_object(bus, "/org/gnome/SessionManager",
        info->interfaces[0], &vtable, NULL, NULL, NULL);
}
static gboolean quit(gpointer loop) { g_main_loop_quit(loop); return G_SOURCE_REMOVE; }
int main(int argc, char **argv)
{
    GMainLoop *loop;
    guint owner;
    GDBusNodeInfo *info;
    if (argc != 3) return 64;
    if (!g_strcmp0(argv[2], "exit")) return 19;
    /* Foreground readiness wrapper must not leak its notification descriptor. */
    if (g_getenv("BSD3_READY_FD")) return 20;
    loop = g_main_loop_new(NULL, FALSE);
    g_unix_signal_add(SIGTERM, quit, loop);
    info = g_dbus_node_info_new_for_xml("<node><interface name='org.bsd3.ReadinessTest'><method name='End'/></interface></node>", NULL);
    owner = g_bus_own_name(G_BUS_TYPE_SESSION, argv[1], G_BUS_NAME_OWNER_FLAGS_NONE,
                          acquired, NULL, NULL, info, NULL);
    if (!g_strcmp0(argv[2], "leave") || !g_strcmp0(argv[2], "leave-error"))
        g_timeout_add(800, quit, loop);
    g_main_loop_run(loop);
    g_bus_unown_name(owner);
    g_dbus_node_info_unref(info);
    g_main_loop_unref(loop);
    return !g_strcmp0(argv[2], "leave-error") ? 23 : 0;
}
