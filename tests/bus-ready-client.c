/* SPDX-License-Identifier: BSD-2-Clause */
#include <gio/gio.h>
#include <glib-unix.h>
#include <signal.h>
#include <unistd.h>
static gboolean quit(gpointer loop) { g_main_loop_quit(loop); return G_SOURCE_REMOVE; }
int main(int argc, char **argv)
{
    GMainLoop *loop;
    guint owner;
    if (argc != 3) return 64;
    if (!g_strcmp0(argv[2], "exit")) return 19;
    /* Foreground readiness wrapper must not leak its notification descriptor. */
    if (g_getenv("BSD3_READY_FD")) return 20;
    loop = g_main_loop_new(NULL, FALSE);
    g_unix_signal_add(SIGTERM, quit, loop);
    owner = g_bus_own_name(G_BUS_TYPE_SESSION, argv[1], G_BUS_NAME_OWNER_FLAGS_NONE,
                          NULL, NULL, NULL, NULL, NULL);
    if (!g_strcmp0(argv[2], "leave")) g_timeout_add(400, quit, loop);
    g_main_loop_run(loop);
    g_bus_unown_name(owner);
    g_main_loop_unref(loop);
    return 0;
}
