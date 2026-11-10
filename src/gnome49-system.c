/* SPDX-License-Identifier: BSD-2-Clause
 * Copyright (c) 2026 Denis Lemire
 * Original native ConsoleKit client implementing the GsmSystem interface.
 * No systemd or upstream GNOME implementation is copied into this file.
 */
#include "gnome49-system.h"
#include <gio/gunixfdlist.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#define CK "org.freedesktop.ConsoleKit"
#define ROOT "/org/freedesktop/ConsoleKit"
#define SESSION CK ".Session"
#define MANAGER CK ".Manager"
typedef struct {
    GObject parent;
    GDBusConnection *bus;
    char *owner, *path;
    guint active_watch, owner_watch, removed_watch;
    gulong closed_watch;
    gboolean active, valid;
    int inhibitor;
    GsmInhibitorFlag held_flags;
} GsmBsd3System;
typedef GObjectClass GsmBsd3SystemClass;
static void iface_init(GsmSystemInterface *);
G_DEFINE_TYPE_WITH_CODE(GsmBsd3System, gsm_bsd3_system, G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE(GSM_TYPE_SYSTEM, iface_init))

static GVariant *
request(GsmBsd3System *s, const char *path, const char *iface,
    const char *method, GVariant *args, const GVariantType *type, GError **error)
{
    return g_dbus_connection_call_sync(s->bus, s->owner, path, iface, method,
        args, type, G_DBUS_CALL_FLAGS_NO_AUTO_START, 3000, NULL, error);
}
static void close_inhibitor(GsmBsd3System *s)
{
    if (s->inhibitor >= 0) close(s->inhibitor);
    s->inhibitor = -1;
    s->held_flags = 0;
}
static void invalidate(GsmBsd3System *s)
{
    s->valid = FALSE;
    close_inhibitor(s);
    if (s->active) {
        s->active = FALSE;
        g_object_notify(G_OBJECT(s), "active");
    }
}
static void bus_closed(GDBusConnection *bus, gboolean remote, GError *error, gpointer data)
{
    (void)bus; (void)remote; (void)error;
    invalidate(data);
}
static gboolean refresh_active(GsmBsd3System *s, GError **error)
{
    GVariant *reply = request(s, s->path, SESSION, "IsActive", NULL,
        G_VARIANT_TYPE("(b)"), error);
    gboolean active;
    if (!reply) { invalidate(s); return FALSE; }
    g_variant_get(reply, "(b)", &active);
    g_variant_unref(reply);
    if (s->active != active) {
        s->active = active;
        g_object_notify(G_OBJECT(s), "active");
    }
    return TRUE;
}
static void changed(GDBusConnection *bus, const char *sender, const char *path,
    const char *iface, const char *signal, GVariant *args, gpointer data)
{
    GsmBsd3System *s = data;
    const char *name, *old_owner, *new_owner, *removed;
    GError *error = NULL;
    (void)bus; (void)sender; (void)path; (void)iface;
    if (!strcmp(signal, "NameOwnerChanged")) {
        g_variant_get(args, "(&s&s&s)", &name, &old_owner, &new_owner);
        (void)name; (void)old_owner;
        if (strcmp(new_owner, s->owner)) invalidate(s);
    } else if (!strcmp(signal, "SessionRemoved")) {
        if (!g_variant_is_of_type(args, G_VARIANT_TYPE("(so)"))) return;
        g_variant_get(args, "(&s&o)", &name, &removed);
        if (!strcmp(removed, s->path)) invalidate(s);
    } else if (s->valid) {
        /* Re-read current state rather than applying a queued older snapshot. */
        if (!refresh_active(s, &error)) {
            g_warning("ConsoleKit active-state refresh failed: %s", error->message);
            g_error_free(error);
        }
    }
}
static void get_property(GObject *object, guint property, GValue *value, GParamSpec *spec)
{
    GsmBsd3System *s = (GsmBsd3System *)object;
    if (property == 1) g_value_set_boolean(value,
        s->valid && !g_dbus_connection_is_closed(s->bus) && s->active);
    else G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property, spec);
}
static void set_property(GObject *object, guint property, const GValue *value, GParamSpec *spec)
{
    /* Ignore the upstream construct default TRUE: only native state is active. */
    (void)value;
    if (property != 1) G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property, spec);
}
static void finalize(GObject *object)
{
    GsmBsd3System *s = (GsmBsd3System *)object;
    if (s->active_watch) g_dbus_connection_signal_unsubscribe(s->bus, s->active_watch);
    if (s->removed_watch) g_dbus_connection_signal_unsubscribe(s->bus, s->removed_watch);
    if (s->owner_watch) g_dbus_connection_signal_unsubscribe(s->bus, s->owner_watch);
    if (s->closed_watch) g_signal_handler_disconnect(s->bus, s->closed_watch);
    close_inhibitor(s);
    g_clear_object(&s->bus);
    g_free(s->owner); g_free(s->path);
    G_OBJECT_CLASS(gsm_bsd3_system_parent_class)->finalize(object);
}
static void gsm_bsd3_system_class_init(GsmBsd3SystemClass *class)
{
    class->get_property = get_property; class->set_property = set_property;
    class->finalize = finalize;
    g_object_class_override_property(class, 1, "active");
}
static void gsm_bsd3_system_init(GsmBsd3System *s) { s->inhibitor = -1; }
static gboolean connected(GsmBsd3System *s, GError **error)
{
    /* A closed notification may still be queued on the main context. */
    if (g_dbus_connection_is_closed(s->bus)) invalidate(s);
    if (s->valid) return TRUE;
    g_set_error_literal(error, G_IO_ERROR, G_IO_ERROR_NOT_CONNECTED,
        "Native session registration is unavailable");
    return FALSE;
}
gboolean gsm_bsd3_system_idle_checked(GsmSystem *system, gboolean idle, GError **error)
{
    GsmBsd3System *s = (GsmBsd3System *)system;
    GVariant *reply;
    if (!connected(s, error)) return FALSE;
    reply = request(s, s->path, SESSION, "SetIdleHint", g_variant_new("(b)", idle),
        G_VARIANT_TYPE("()"), error);
    if (!reply) return FALSE;
    g_variant_unref(reply);
    return TRUE;
}
gboolean gsm_bsd3_system_inhibit_checked(GsmSystem *system, GsmInhibitorFlag flags, GError **error)
{
    GsmBsd3System *s = (GsmBsd3System *)system;
    GString *what;
    GUnixFDList *fds = NULL;
    GVariant *reply;
    struct stat st;
    int fd = -1, handle;
    flags &= GSM_INHIBITOR_FLAG_LOGOUT | GSM_INHIBITOR_FLAG_SUSPEND | GSM_INHIBITOR_FLAG_IDLE;
    if (!connected(s, error)) return FALSE;
    if (!flags) { close_inhibitor(s); return TRUE; }
    if (flags == s->held_flags) return TRUE;
    /* ConsoleKit documents one inhibitor per application. Replacement is a
     * release/reacquire operation, not an atomic upgrade; expose failures. */
    close_inhibitor(s);
    what = g_string_new(NULL);
    if (flags & GSM_INHIBITOR_FLAG_LOGOUT) g_string_append(what, "shutdown:");
    if (flags & GSM_INHIBITOR_FLAG_SUSPEND) g_string_append(what, "sleep:");
    if (flags & GSM_INHIBITOR_FLAG_IDLE) g_string_append(what, "idle:");
    g_string_truncate(what, what->len - 1);
    reply = g_dbus_connection_call_with_unix_fd_list_sync(s->bus, s->owner,
        ROOT "/Manager", MANAGER, "Inhibit", g_variant_new("(ssss)", what->str,
        "GNOME session", "Desktop session inhibitors", "block"),
        G_VARIANT_TYPE("(h)"), G_DBUS_CALL_FLAGS_NO_AUTO_START, 3000,
        NULL, &fds, NULL, error);
    g_string_free(what, TRUE);
    if (!reply) return FALSE;
    g_variant_get(reply, "(h)", &handle);
    if (!fds || g_unix_fd_list_get_length(fds) != 1 || handle != 0) {
        g_set_error_literal(error, G_IO_ERROR, G_IO_ERROR_INVALID_DATA,
            "Invalid ConsoleKit inhibitor descriptor bundle");
    } else {
        fd = g_unix_fd_list_get(fds, handle, error);
        if (fd >= 0 && (fstat(fd, &st) < 0 || !S_ISFIFO(st.st_mode) ||
            fcntl(fd, F_SETFD, FD_CLOEXEC) < 0)) {
            close(fd); fd = -1;
            g_set_error_literal(error, G_IO_ERROR, G_IO_ERROR_INVALID_DATA,
                "ConsoleKit inhibitor is not a usable pipe");
        }
    }
    g_variant_unref(reply);
    g_clear_object(&fds);
    if (fd < 0) return FALSE;
    close_inhibitor(s);
    s->inhibitor = fd; s->held_flags = flags;
    return TRUE;
}
static gboolean unavailable(GsmSystem *s) { (void)s; return FALSE; }
static gboolean can_ck(GsmSystem *system, const char *method)
{
    GsmBsd3System *s = (GsmBsd3System *)system;
    GError *error = NULL;
    GVariant *reply;
    const char *result;
    gboolean allowed;
    if (!connected(s, &error)) { g_clear_error(&error); return FALSE; }
    reply = request(s, ROOT "/Manager", MANAGER, method, NULL, G_VARIANT_TYPE("(s)"), &error);
    if (!reply) { g_clear_error(&error); return FALSE; }
    g_variant_get(reply, "(&s)", &result);
    allowed = !strcmp(result, "yes") || !strcmp(result, "challenge");
    g_variant_unref(reply);
    return allowed;
}
static void do_ck(GsmSystem *system, const char *method)
{
    GsmBsd3System *s = (GsmBsd3System *)system;
    GError *error = NULL;
    GVariant *reply;
    if (!connected(s, &error)) {
        g_signal_emit_by_name(s, "request-completed", error);
        g_clear_error(&error);
        return;
    }
    reply = request(s, ROOT "/Manager", MANAGER, method, g_variant_new("(b)", TRUE),
        G_VARIANT_TYPE("()"), &error);
    if (reply) g_variant_unref(reply);
    g_signal_emit_by_name(s, "request-completed", error);
    g_clear_error(&error);
}
static gboolean can_suspend(GsmSystem *s) { return can_ck(s, "CanSuspend"); }
static gboolean can_hibernate(GsmSystem *s) { return can_ck(s, "CanHibernate"); }
static void do_suspend(GsmSystem *s) { do_ck(s, "Suspend"); }
static void do_hibernate(GsmSystem *s) { do_ck(s, "Hibernate"); }
static void deny(GsmSystem *s)
{
    GError *error = g_error_new_literal(G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED,
        "Native privileged operation is not supported");
    g_signal_emit_by_name(s, "request-completed", error);
    g_error_free(error);
}
static void firmware(GsmSystem *s, gboolean enabled) { (void)enabled; deny(s); }
static void prepare(GsmSystem *s, gboolean restart)
{ (void)restart; g_signal_emit_by_name(s, "shutdown-prepared", FALSE); }
static void idle(GsmSystem *s, gboolean value)
{
    GError *error = NULL;
    if (!gsm_bsd3_system_idle_checked(s, value, &error)) {
        g_warning("Native idle update failed: %s", error->message); g_error_free(error);
    }
}
static void inhibit(GsmSystem *s, GsmInhibitorFlag flags)
{
    GError *error = NULL;
    if (!gsm_bsd3_system_inhibit_checked(s, flags, &error)) {
        g_warning("Native inhibitor update failed: %s", error->message); g_error_free(error);
    }
}
static void iface_init(GsmSystemInterface *iface)
{
    iface->can_switch_user = unavailable; iface->can_stop = unavailable;
    iface->can_restart = unavailable; iface->can_restart_to_firmware_setup = unavailable;
    iface->can_suspend = can_suspend; iface->can_hibernate = can_hibernate;
    iface->attempt_stop = deny; iface->attempt_restart = deny;
    iface->suspend = do_suspend; iface->hibernate = do_hibernate;
    iface->set_restart_to_firmware_setup = firmware;
    iface->prepare_shutdown = prepare; iface->complete_shutdown = deny;
    iface->set_session_idle = idle; iface->set_inhibitors = inhibit;
}
GsmSystem *gsm_bsd3_system_new_for_connection(GDBusConnection *bus, uid_t trusted_uid, GError **error)
{
    GsmBsd3System *s;
    GVariant *reply;
    guint32 uid;
    const char *text;
    gboolean allowed;
    if (getuid() == 0 || getuid() != geteuid()) {
        g_set_error_literal(error, G_IO_ERROR, G_IO_ERROR_PERMISSION_DENIED,
            "An ordinary session user is required");
        return NULL;
    }
    s = g_object_new(gsm_bsd3_system_get_type(), NULL);
    s->bus = g_object_ref(bus);
    s->closed_watch = g_signal_connect(bus, "closed", G_CALLBACK(bus_closed), s);
    reply = g_dbus_connection_call_sync(bus, "org.freedesktop.DBus",
        "/org/freedesktop/DBus", "org.freedesktop.DBus", "GetNameOwner",
        g_variant_new("(s)", CK), G_VARIANT_TYPE("(s)"), 0, 3000, NULL, error);
    if (!reply) goto fail;
    g_variant_get(reply, "(s)", &s->owner); g_variant_unref(reply);
    reply = g_dbus_connection_call_sync(bus, "org.freedesktop.DBus",
        "/org/freedesktop/DBus", "org.freedesktop.DBus", "GetConnectionUnixUser",
        g_variant_new("(s)", s->owner), G_VARIANT_TYPE("(u)"), 0, 3000, NULL, error);
    if (!reply) goto fail;
    g_variant_get(reply, "(u)", &uid); g_variant_unref(reply);
    if (uid != trusted_uid) {
        g_set_error_literal(error, G_IO_ERROR, G_IO_ERROR_PERMISSION_DENIED,
            "ConsoleKit owner has an unexpected UID");
        goto fail;
    }
    reply = request(s, ROOT "/Manager", MANAGER, "GetSessionForUnixProcess",
        g_variant_new("(u)", (guint32)getpid()), G_VARIANT_TYPE("(o)"), error);
    if (!reply) goto fail;
    g_variant_get(reply, "(o)", &s->path); g_variant_unref(reply);
    if (!g_str_has_prefix(s->path, ROOT "/Session") || !s->path[strlen(ROOT "/Session")]) {
        g_set_error_literal(error, G_IO_ERROR, G_IO_ERROR_INVALID_DATA, "Invalid native session object path");
        goto fail;
    }
    reply = request(s, s->path, SESSION, "GetUnixUser", NULL, G_VARIANT_TYPE("(u)"), error);
    if (!reply) goto fail;
    g_variant_get(reply, "(u)", &uid); g_variant_unref(reply);
    if (uid != getuid()) {
        g_set_error_literal(error, G_IO_ERROR, G_IO_ERROR_PERMISSION_DENIED, "Native session belongs to a different user");
        goto fail;
    }
    reply = request(s, s->path, SESSION, "GetSessionClass", NULL, G_VARIANT_TYPE("(s)"), error);
    if (!reply) goto fail;
    g_variant_get(reply, "(&s)", &text);
    allowed = !strcmp(text, "user") || !strcmp(text, "greeter");
    g_variant_unref(reply);
    if (!allowed) {
        g_set_error_literal(error, G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED, "Native session is not a desktop or greeter session");
        goto fail;
    }
    reply = request(s, s->path, SESSION, "GetSessionType", NULL, G_VARIANT_TYPE("(s)"), error);
    if (!reply) goto fail;
    g_variant_get(reply, "(&s)", &text);
    allowed = !strcmp(text, "wayland");
    g_variant_unref(reply);
    if (!allowed) {
        g_set_error_literal(error, G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED, "Native session is not registered as Wayland");
        goto fail;
    }
    s->active_watch = g_dbus_connection_signal_subscribe(bus, s->owner, SESSION,
        "ActiveChanged", s->path, NULL, 0, changed, s, NULL);
    s->removed_watch = g_dbus_connection_signal_subscribe(bus, s->owner, MANAGER,
        "SessionRemoved", ROOT "/Manager", NULL, 0, changed, s, NULL);
    s->owner_watch = g_dbus_connection_signal_subscribe(bus, "org.freedesktop.DBus",
        "org.freedesktop.DBus", "NameOwnerChanged", "/org/freedesktop/DBus", CK, 0, changed, s, NULL);
    s->valid = TRUE;
    if (!refresh_active(s, error)) goto fail;
    reply = g_dbus_connection_call_sync(bus, "org.freedesktop.DBus",
        "/org/freedesktop/DBus", "org.freedesktop.DBus", "GetNameOwner",
        g_variant_new("(s)", CK), G_VARIANT_TYPE("(s)"), 0, 3000, NULL, error);
    if (!reply) goto fail;
    g_variant_get(reply, "(&s)", &text); allowed = !strcmp(text, s->owner);
    g_variant_unref(reply);
    if (!allowed) {
        g_set_error_literal(error, G_IO_ERROR, G_IO_ERROR_NOT_CONNECTED, "ConsoleKit owner changed during session binding");
        goto fail;
    }
    return GSM_SYSTEM(s);
fail:
    g_object_unref(s);
    return NULL;
}
GsmSystem *gsm_bsd3_system_new(GError **error)
{
    GDBusConnection *bus = g_bus_get_sync(G_BUS_TYPE_SYSTEM, NULL, error);
    GsmSystem *result;
    if (!bus) return NULL;
    result = gsm_bsd3_system_new_for_connection(bus, 0, error);
    g_object_unref(bus);
    return result;
}
