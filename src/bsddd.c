/* SPDX-License-Identifier: BSD-2-Clause
 * Copyright (c) 2026 Denis Lemire <denis@lemire.name>
 */
#include <errno.h>
#include <signal.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gio/gio.h>
#include <glib-unix.h>
#include "bsd3/sd-login.h"
#include "control.h"
#include "seats.h"

#define ROOT "/org/freedesktop/login1"
#define SESSION ROOT "/session"
#define SEAT ROOT "/seat"
#define LOGIN "org.freedesktop.login1"

static GMainLoop *loop;
static GDBusNodeInfo *schema;
static int exit_status;
static const char xml[] =
    "<node><interface name='org.freedesktop.login1.Manager'>"
    "<method name='GetSession'><arg type='s' direction='in'/>"
    "<arg type='o' direction='out'/></method>"
    "<method name='GetSessionByPID'><arg type='u' direction='in'/>"
    "<arg type='o' direction='out'/></method>"
    "<method name='ListSessions'><arg type='a(susso)' direction='out'/>"
    "</method>"
    "<method name='ListSeats'><arg type='a(so)' direction='out'/></method>"
    "<method name='GetSeat'><arg type='s' direction='in'/><arg type='o' direction='out'/></method>"
    "</interface><interface name='org.freedesktop.login1.Session'>"
    "<method name='TakeControl'><arg type='b' direction='in'/></method>"
    "<method name='ReleaseControl'/>"
    "<method name='TakeDevice'><arg type='u' direction='in'/><arg type='u' direction='in'/>"
    "<arg type='h' direction='out'/><arg type='b' direction='out'/></method>"
    "<method name='ReleaseDevice'><arg type='u' direction='in'/><arg type='u' direction='in'/></method>"
    "<method name='PauseDeviceComplete'><arg type='u' direction='in'/><arg type='u' direction='in'/></method>"
    "<signal name='PauseDevice'><arg type='u'/><arg type='u'/><arg type='s'/></signal>"
    "<signal name='ResumeDevice'><arg type='u'/><arg type='u'/><arg type='h'/></signal>"
    "<property name='Id' type='s' access='read'/>"
    "<property name='Seat' type='(so)' access='read'/>"
    "<property name='Active' type='b' access='read'/>"
    "<property name='Type' type='s' access='read'/>"
    "<property name='Class' type='s' access='read'/>"
    "<property name='State' type='s' access='read'/>"
    "</interface><interface name='org.freedesktop.login1.Seat'>"
    "<method name='SwitchTo'><arg type='u' direction='in'/></method>"
    "<property name='Id' type='s' access='read'/>"
    "<property name='CanTTY' type='b' access='read'/>"
    "<property name='CanGraphical' type='b' access='read'/>"
    "</interface></node>";

static void
return_error(GDBusMethodInvocation *invocation, int code)
{
	const char *name = "org.freedesktop.DBus.Error.Failed";
	if (code == -ENODATA || code == -ENXIO || code == -EINVAL)
		name = LOGIN ".NoSuchSession";
	else if (code == -EACCES)
		name = "org.freedesktop.DBus.Error.AccessDenied";
	g_dbus_method_invocation_return_dbus_error(invocation, name,
	    g_strerror(-code));
}

static int
caller_pid(GDBusConnection *bus, const char *sender, guint32 *pid)
{
	GError *error = NULL;
	GVariant *reply = g_dbus_connection_call_sync(bus,
	    "org.freedesktop.DBus", "/org/freedesktop/DBus",
	    "org.freedesktop.DBus", "GetConnectionUnixProcessID",
	    g_variant_new("(s)", sender), G_VARIANT_TYPE("(u)"),
	    G_DBUS_CALL_FLAGS_NONE, 3000, NULL, &error);
	if (reply == NULL) {
		g_clear_error(&error);
		return (-EIO);
	}
	g_variant_get(reply, "(u)", pid);
	g_variant_unref(reply);
	return (0);
}

static void
manager_call(GDBusConnection *bus, const char *sender, const char *path,
    const char *iface, const char *method, GVariant *parameters,
    GDBusMethodInvocation *invocation, gpointer data)
{
	char *id = NULL, *object;
	int rc;
	(void)path;
	(void)iface;
	(void)data;
	if (strcmp(method, "ListSeats") == 0 || strcmp(method, "GetSeat") == 0) {
		GError *error = NULL;
		GPtrArray *seats = bsd3_seats_list(bus, &error);
		if (seats == NULL) {
			g_dbus_method_invocation_return_gerror(invocation, error);
			g_error_free(error); return;
		}
		GVariantBuilder builder;
		g_variant_builder_init(&builder, G_VARIANT_TYPE("a(so)"));
		const char *requested = NULL;
		if (strcmp(method, "GetSeat") == 0) g_variant_get(parameters, "(&s)", &requested);
		for (guint i = 0; i < seats->len; i++) {
			struct bsd3_seat *seat = g_ptr_array_index(seats, i);
			char *object_path = g_strconcat(SEAT "/", seat->component, NULL);
			if (requested == NULL) g_variant_builder_add(&builder, "(so)", seat->name, object_path);
			else if (strcmp(requested, seat->name) == 0) {
				g_dbus_method_invocation_return_value(invocation, g_variant_new("(o)", object_path));
				g_free(object_path); g_ptr_array_unref(seats); g_variant_builder_clear(&builder); return;
			}
			g_free(object_path);
		}
		g_ptr_array_unref(seats);
		if (requested == NULL)
			g_dbus_method_invocation_return_value(invocation, g_variant_new("(a(so))", &builder));
		else {
			g_variant_builder_clear(&builder);
			g_dbus_method_invocation_return_dbus_error(invocation, LOGIN ".NoSuchSeat", "Unknown seat");
		}
		return;
	}
	if (strcmp(method, "GetSessionByPID") == 0) {
		guint32 pid;
		g_variant_get(parameters, "(u)", &pid);
		rc = pid == 0 ? caller_pid(bus, sender, &pid) : 0;
		if (rc >= 0 && (guint64)pid > G_MAXINT32)
			rc = -EINVAL;
		if (rc >= 0)
			rc = sd_pid_get_session((pid_t)pid, &id);
	} else if (strcmp(method, "GetSession") == 0) {
		const char *requested;
		uid_t uid;
		g_variant_get(parameters, "(&s)", &requested);
		rc = sd_session_get_uid(requested, &uid);
		if (rc >= 0)
			id = g_strdup(requested);
	} else if (strcmp(method, "ListSessions") == 0) {
		GVariantBuilder builder;
		char **sessions = NULL;
		int n = sd_get_sessions(&sessions), i;
		if (n < 0) {
			return_error(invocation, n);
			return;
		}
		g_variant_builder_init(&builder, G_VARIANT_TYPE("a(susso)"));
		for (i = 0; i < n; i++) {
			uid_t uid;
			char *seat = NULL;
			struct passwd *pw;
			rc = sd_session_get_uid(sessions[i], &uid);
			if (rc < 0)
				break;
			rc = sd_session_get_seat(sessions[i], &seat);
			if (rc < 0 && rc != -ENODATA)
				break;
			pw = getpwuid(uid);
			object = g_strconcat(SESSION "/", sessions[i], NULL);
			g_variant_builder_add(&builder, "(susso)", sessions[i],
			    (guint32)uid, pw == NULL ? "" : pw->pw_name,
			    seat == NULL ? "" : seat, object);
			g_free(object);
			free(seat);
		}
		g_strfreev(sessions);
		if (i != n) {
			g_variant_builder_clear(&builder);
			return_error(invocation, rc);
			return;
		}
		g_dbus_method_invocation_return_value(invocation,
		    g_variant_new("(a(susso))", &builder));
		return;
	} else {
		g_dbus_method_invocation_return_dbus_error(invocation,
		    "org.freedesktop.DBus.Error.UnknownMethod", "Not implemented");
		return;
	}
	if (rc < 0) {
		return_error(invocation, rc);
		return;
	}
	object = g_strconcat(SESSION "/", id, NULL);
	g_dbus_method_invocation_return_value(invocation,
	    g_variant_new("(o)", object));
	g_free(object);
	free(id);
}

static GVariant *
property(GDBusConnection *bus, const char *sender, const char *path,
    const char *iface, const char *name, GError **error, gpointer data)
{
	const char *id = strrchr(path, '/');
	char *value = NULL;
	GVariant *result = NULL;
	uid_t uid;
	int rc;
	(void)bus;
	(void)sender;
	(void)iface;
	(void)data;
	if (id == NULL || sd_session_get_uid(++id, &uid) < 0) {
		g_set_error_literal(error, G_DBUS_ERROR, G_DBUS_ERROR_UNKNOWN_OBJECT,
		    "No such ConsoleKit session");
		return (NULL);
	}
	if (strcmp(name, "Id") == 0)
		return (g_variant_new_string(id));
	if (strcmp(name, "Seat") == 0)
		return (bsd3_session_seat(bus, id, error));
	if (strcmp(name, "Active") == 0) {
		rc = sd_session_is_active(id);
		if (rc >= 0)
			return (g_variant_new_boolean(rc > 0));
	} else {
		if (strcmp(name, "Type") == 0)
			rc = sd_session_get_type(id, &value);
		else if (strcmp(name, "Class") == 0)
			rc = sd_session_get_class(id, &value);
		else if (strcmp(name, "State") == 0)
			rc = sd_session_get_state(id, &value);
		else
			rc = -ENOTSUP;
		if (rc >= 0)
			result = g_variant_new_string(value);
		free(value);
		if (result != NULL)
			return (result);
	}
	g_set_error(error, G_DBUS_ERROR, G_DBUS_ERROR_FAILED, "%s", g_strerror(-rc));
	return (NULL);
}

static void
session_call(GDBusConnection *bus, const char *sender, const char *path,
    const char *iface, const char *method, GVariant *args,
    GDBusMethodInvocation *invocation, gpointer data)
{
	const char *id = strrchr(path, '/');
	(void)bus; (void)sender; (void)iface; (void)data;
	if (id == NULL) {
		return_error(invocation, -EINVAL);
		return;
	}
	bsd3_control_call(id + 1, method, args, invocation);
}

static void
seat_call(GDBusConnection *bus, const char *sender, const char *path,
    const char *iface, const char *method, GVariant *args,
    GDBusMethodInvocation *invocation, gpointer data)
{
	(void)bus; (void)sender; (void)iface; (void)method; (void)data;
	bsd3_seat_call(strrchr(path, '/') + 1, args, invocation);
}

static GVariant *
seat_property(GDBusConnection *bus, const char *sender, const char *path,
    const char *iface, const char *name, GError **error, gpointer data)
{
	(void)sender; (void)iface; (void)data;
	return (bsd3_seat_property(bus, strrchr(path, '/') + 1, name, error));
}
static const GDBusInterfaceVTable seat_vtable = {
	.method_call = seat_call, .get_property = seat_property
};

static const GDBusInterfaceVTable manager_vtable = {
	.method_call = manager_call
};
static const GDBusInterfaceVTable session_vtable = {
	.method_call = session_call, .get_property = property
};

static gchar **
enumerate(GDBusConnection *bus, const gchar *sender, const gchar *path, gpointer data)
{
	(void)sender; (void)path;
	if (data != NULL) {
		GError *error = NULL;
		GPtrArray *seats = bsd3_seats_list(bus, &error);
		if (seats == NULL) { g_clear_error(&error); return (g_new0(gchar *, 1)); }
		char **names = g_new0(char *, seats->len + 1);
		for (guint i = 0; i < seats->len; i++)
			names[i] = g_strdup(((struct bsd3_seat *)g_ptr_array_index(seats, i))->component);
		g_ptr_array_unref(seats); return (names);
	}
	char **sessions = NULL;
	if (sd_get_sessions(&sessions) < 0) return (g_new0(gchar *, 1));
	return (sessions);
}

static GDBusInterfaceInfo **
introspect(GDBusConnection *bus, const gchar *sender, const gchar *path,
    const gchar *node, gpointer data)
{
	GDBusInterfaceInfo **info;
	(void)bus; (void)sender; (void)path; (void)data;
	if (node == NULL)
		return (NULL);
	info = g_new0(GDBusInterfaceInfo *, 2);
	info[0] = g_dbus_interface_info_ref(
	    schema->interfaces[data != NULL ? 2 : 1]);
	return (info);
}

static const GDBusInterfaceVTable *
dispatch(GDBusConnection *bus, const gchar *sender, const gchar *path,
    const gchar *iface, const gchar *node, gpointer *out_data, gpointer data)
{
	(void)bus; (void)sender; (void)path; (void)node; (void)data;
	*out_data = NULL;
	if (data != NULL && strcmp(iface, LOGIN ".Seat") == 0)
		return (&seat_vtable);
	return (data == NULL && strcmp(iface, LOGIN ".Session") == 0 ?
	    &session_vtable : NULL);
}

static const GDBusSubtreeVTable subtree = {
	.enumerate = enumerate, .introspect = introspect, .dispatch = dispatch
};

static void
bus_ready(GDBusConnection *bus, const char *name, gpointer data)
{
	GError *error = NULL;
	(void)name; (void)data;
	bsd3_controls_start(bus);
	if (g_dbus_connection_register_object(bus, ROOT, schema->interfaces[0],
	    &manager_vtable, NULL, NULL, &error) == 0 ||
	    g_dbus_connection_register_subtree(bus, SESSION, &subtree,
	    G_DBUS_SUBTREE_FLAGS_DISPATCH_TO_UNENUMERATED_NODES,
	    NULL, NULL, &error) == 0 ||
	    g_dbus_connection_register_subtree(bus, SEAT, &subtree,
	    G_DBUS_SUBTREE_FLAGS_DISPATCH_TO_UNENUMERATED_NODES,
	    GINT_TO_POINTER(1), NULL, &error) == 0) {
		g_printerr("bsddd: %s\n", error->message);
		g_clear_error(&error);
		exit_status = 1;
		g_main_loop_quit(loop);
	}
}

static void
name_ready(GDBusConnection *bus, const char *name, gpointer data)
{
	(void)bus; (void)name; (void)data;
	g_print("READY: BSD3 session-controller prototype\n");
	fflush(stdout);
}

static void
name_lost(GDBusConnection *bus, const char *name, gpointer data)
{
	(void)bus; (void)name; (void)data;
	g_printerr("bsddd: cannot own login1; check bus policy or existing owner\n");
	exit_status = 1;
	g_main_loop_quit(loop);
}

static gboolean
stop(gpointer data)
{
	(void)data;
	g_main_loop_quit(loop);
	return (G_SOURCE_REMOVE);
}

static void
ignore_bus_override(void)
{
	/* GLib selects G_BUS_TYPE_SYSTEM from DBUS_SYSTEM_BUS_ADDRESS when it
	 * is set. The daemon always uses the system bus GLib was built with.
	 * The rc wrapper also clears the variable; that is not the mechanism. */
	g_unsetenv("DBUS_SYSTEM_BUS_ADDRESS");
}

static int
probe(void)
{
	char **sessions = NULL;
	ignore_bus_override();
	int n = sd_get_sessions(&sessions), i;
	if (n < 0) {
		g_printerr("ConsoleKit: %s (%d)\n", g_strerror(-n), n);
		return (1);
	}
	g_print("ConsoleKit sessions: %d\n", n);
	for (i = 0; i < n; i++) {
		char *type = NULL, *state = NULL;
		int trc = sd_session_get_type(sessions[i], &type);
		int src = sd_session_get_state(sessions[i], &state);
		g_print("%s type=%s state=%s\n", sessions[i],
		    trc < 0 ? g_strerror(-trc) : type,
		    src < 0 ? g_strerror(-src) : state);
		free(type); free(state);
	}
	g_strfreev(sessions);
	g_print("Session discovery; device handoff and GNOME startup remain open.\n");
	return (0);
}

/* A service readiness check, not a claim that a desktop/session is usable. */
static int
check_owner(const char *text)
{
	GDBusConnection *bus;
	GVariant *reply;
	GError *error = NULL;
	const char *p;
	guint64 expected;
	guint32 actual;

	if (*text == '\0')
		return (64);
	for (p = text; *p != '\0'; p++)
		if (!g_ascii_isdigit(*p))
			return (64);
	errno = 0;
	expected = g_ascii_strtoull(text, NULL, 10);
	if (errno != 0 || expected == 0 || expected > G_MAXINT32)
		return (64);
	ignore_bus_override();
	bus = g_bus_get_sync(G_BUS_TYPE_SYSTEM, NULL, &error);
	if (bus == NULL)
		goto failed;
	reply = g_dbus_connection_call_sync(bus, "org.freedesktop.DBus",
	    "/org/freedesktop/DBus", "org.freedesktop.DBus",
	    "GetConnectionUnixProcessID", g_variant_new("(s)", LOGIN),
	    G_VARIANT_TYPE("(u)"), G_DBUS_CALL_FLAGS_NO_AUTO_START, 1000,
	    NULL, &error);
	g_object_unref(bus);
	if (reply == NULL)
		goto failed;
	g_variant_get(reply, "(u)", &actual);
	g_variant_unref(reply);
	if (actual != expected) {
		g_printerr("bsddd: login1 belongs to a different process\n");
		return (1);
	}
	return (0);
failed:
	g_printerr("bsddd: cannot verify login1 owner: %s\n", error->message);
	g_error_free(error);
	return (1);
}

int
main(int argc, char **argv)
{
	GError *error = NULL;
	guint owner;
	ignore_bus_override();
	if (argc == 2 && strcmp(argv[1], "--probe") == 0)
		return (probe());
	if (argc == 3 && strcmp(argv[1], "--check-owner") == 0)
		return (check_owner(argv[2]));
	if (argc != 2 || strcmp(argv[1], "--foreground") != 0) {
		g_printerr("usage: bsddd --probe | --foreground | --check-owner PID\n");
		return (64);
	}
	schema = g_dbus_node_info_new_for_xml(xml, &error);
	if (schema == NULL) {
		g_printerr("bsddd: %s\n", error->message);
		g_error_free(error);
		return (1);
	}
	loop = g_main_loop_new(NULL, FALSE);
	g_unix_signal_add(SIGINT, stop, NULL);
	g_unix_signal_add(SIGTERM, stop, NULL);
	ignore_bus_override();
	owner = g_bus_own_name(G_BUS_TYPE_SYSTEM, LOGIN,
	    G_BUS_NAME_OWNER_FLAGS_DO_NOT_QUEUE, bus_ready, name_ready, name_lost,
	    NULL, NULL);
	g_main_loop_run(loop);
	bsd3_controls_stop();
	g_bus_unown_name(owner);
	g_main_loop_unref(loop);
	g_dbus_node_info_unref(schema);
	return (exit_status);
}
