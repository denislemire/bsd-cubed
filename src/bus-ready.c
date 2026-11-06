/* SPDX-License-Identifier: BSD-2-Clause */
#ifndef __FreeBSD__
#define _POSIX_C_SOURCE 200809L
#endif
#include <sys/types.h>
#ifdef __FreeBSD__
#include <sys/procctl.h>
#endif
#include <bsd3/user-session.h>
#include <gio/gio.h>
#include <glib-unix.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>

/* A foreground service is ready only when its own process owns the declared
 * session-bus name. No activation request, arbitrary proxy or process scan. */
struct service {
    GMainLoop *loop;
    GDBusConnection *bus;
    GSubprocess *child;
    pid_t pid;
    gboolean ready, stopping, failed;
    guint deadline, kill_deadline;
};
static gboolean kill_child(gpointer data)
{
    struct service *s = data;
    s->kill_deadline = 0;
    kill(-s->pid, SIGKILL);
    return G_SOURCE_REMOVE;
}
static void stop(struct service *s, gboolean failure)
{
    if (s->stopping) return;
    s->failed |= failure;
    s->stopping = TRUE;
    kill(-s->pid, SIGTERM);
    s->kill_deadline = g_timeout_add_seconds(2, kill_child, s);
}
static gboolean terminated(gpointer data) { stop(data, FALSE); return G_SOURCE_CONTINUE; }
static gboolean expired(gpointer data)
{
    struct service *s = data;
    s->deadline = 0;
    g_printerr("Service did not acquire its declared bus name within 30 seconds\n");
    stop(s, TRUE);
    return G_SOURCE_REMOVE;
}
static void exited(GObject *object, GAsyncResult *result, gpointer data)
{
    struct service *s = data;
    GError *error = NULL;
    if (!g_subprocess_wait_finish(G_SUBPROCESS(object), result, &error)) {
        g_printerr("Service wait failed: %s\n", error->message);
        g_clear_error(&error);
        s->failed = TRUE;
    } else if (!s->stopping && !g_subprocess_get_successful(s->child)) {
        s->failed = TRUE;
    }
    if (!s->ready) s->failed = TRUE;
    /* Reap the foreground process; terminate any remaining process-group peers.
     * The outer FreeBSD supervisor also reaps escaped descendants at teardown. */
    kill(-s->pid, SIGKILL);
    g_main_loop_quit(s->loop);
}
static void appeared(GDBusConnection *bus, const gchar *name,
                     const gchar *owner, gpointer data)
{
    struct service *s = data;
    GError *error = NULL;
    GVariant *reply;
    guint32 pid;
    (void)name;
    if (s->stopping) return;
    reply = g_dbus_connection_call_sync(bus, "org.freedesktop.DBus",
        "/org/freedesktop/DBus", "org.freedesktop.DBus", "GetConnectionUnixProcessID",
        g_variant_new("(s)", owner), G_VARIANT_TYPE("(u)"),
        G_DBUS_CALL_FLAGS_NO_AUTO_START, 3000, NULL, &error);
    if (!reply) {
        g_printerr("Cannot authenticate service bus owner: %s\n", error->message);
        g_error_free(error);
        stop(s, TRUE);
        return;
    }
    g_variant_get(reply, "(u)", &pid);
    g_variant_unref(reply);
    if (pid != (guint32)s->pid) {
        g_printerr("Declared bus name belongs to a different process\n");
        stop(s, TRUE);
        return;
    }
    if (!s->ready) {
        int status = bsd3_session_ready();
        if (status < 0) {
            g_printerr("Readiness notification failed: %d\n", status);
            stop(s, TRUE);
            return;
        }
        s->ready = TRUE;
        if (s->deadline) { g_source_remove(s->deadline); s->deadline = 0; }
    }
}
static void vanished(GDBusConnection *bus, const gchar *name, gpointer data)
{
    struct service *s = data;
    (void)bus; (void)name;
    if (s->ready) stop(s, TRUE);
}
static void closed(GDBusConnection *bus, gboolean remote, GError *error, gpointer data)
{
    (void)bus; (void)remote; (void)error;
    stop(data, TRUE);
}
static void child_setup(gpointer data)
{
    pid_t parent = GPOINTER_TO_INT(data);
    if (setpgid(0, 0) < 0) _exit(126);
#ifdef __FreeBSD__
    int sig = SIGKILL;
    if (procctl(P_PID, 0, PROC_PDEATHSIG_CTL, &sig) < 0) _exit(126);
#endif
    if (getppid() != parent) _exit(126);
}
int main(int argc, char **argv)
{
    struct service s = {0};
    GError *error = NULL;
    GSubprocessLauncher *launcher;
    GVariant *reply;
    gboolean owned;
    guint watch, term, interrupt;
    gulong close_handler;
    if (!getuid() || geteuid() != getuid()) return 77;
    if (argc < 4 || !g_dbus_is_name(argv[1]) || argv[1][0] == ':' ||
        g_strcmp0(argv[2], "--") || argv[3][0] != '/') return 64;
    s.bus = g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, &error);
    if (!s.bus) goto failure;
    g_dbus_connection_set_exit_on_close(s.bus, FALSE);
    reply = g_dbus_connection_call_sync(s.bus, "org.freedesktop.DBus",
        "/org/freedesktop/DBus", "org.freedesktop.DBus", "NameHasOwner",
        g_variant_new("(s)", argv[1]), G_VARIANT_TYPE("(b)"),
        G_DBUS_CALL_FLAGS_NO_AUTO_START, 3000, NULL, &error);
    if (!reply) goto failure;
    g_variant_get(reply, "(b)", &owned); g_variant_unref(reply);
    if (owned) { g_printerr("Declared service name already owned\n"); g_object_unref(s.bus); return 1; }
    launcher = g_subprocess_launcher_new(G_SUBPROCESS_FLAGS_NONE);
    g_subprocess_launcher_unsetenv(launcher, "BSD3_READY_FD");
    g_subprocess_launcher_set_child_setup(launcher, child_setup,
        GINT_TO_POINTER(getpid()), NULL);
    s.child = g_subprocess_launcher_spawnv(launcher, (const gchar * const *)&argv[3], &error);
    g_object_unref(launcher);
    if (!s.child) goto failure;
    s.pid = (pid_t)strtol(g_subprocess_get_identifier(s.child), NULL, 10);
    s.loop = g_main_loop_new(NULL, FALSE);
    term = g_unix_signal_add(SIGTERM, terminated, &s);
    interrupt = g_unix_signal_add(SIGINT, terminated, &s);
    close_handler = g_signal_connect(s.bus, "closed", G_CALLBACK(closed), &s);
    s.deadline = g_timeout_add_seconds(30, expired, &s);
    watch = g_bus_watch_name_on_connection(s.bus, argv[1], G_BUS_NAME_WATCHER_FLAGS_NONE,
                                          appeared, vanished, &s, NULL);
    g_subprocess_wait_async(s.child, NULL, exited, &s);
    g_main_loop_run(s.loop);
    g_bus_unwatch_name(watch);
    g_signal_handler_disconnect(s.bus, close_handler);
    if (s.deadline) g_source_remove(s.deadline);
    if (s.kill_deadline) g_source_remove(s.kill_deadline);
    g_source_remove(term); g_source_remove(interrupt);
    g_main_loop_unref(s.loop);
    g_object_unref(s.child); g_object_unref(s.bus);
    return s.failed ? 1 : 0;
failure:
    g_printerr("Service startup failed: %s\n", error ? error->message : "unknown error");
    g_clear_error(&error); g_clear_object(&s.bus);
    return 1;
}
