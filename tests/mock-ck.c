/* SPDX-License-Identifier: BSD-2-Clause */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <gio/gio.h>
#include <glib-unix.h>
#define CK "org.freedesktop.ConsoleKit"
#define ROOT "/org/freedesktop/ConsoleKit"
static GMainLoop *loop;
static guint32 last_pid;
static char *mode;
static GHashTable *owners;
static guint takes, releases;
static const char xml[] =
 "<node><interface name='org.freedesktop.ConsoleKit.Manager'>"
 "<method name='GetSessions'><arg type='ao' direction='out'/></method>"
 "<method name='GetSessionsForUnixUser'><arg type='u' direction='in'/><arg type='ao' direction='out'/></method>"
 "<method name='GetSessionForUnixProcess'><arg type='u' direction='in'/><arg type='o' direction='out'/></method>"
 "</interface><interface name='org.freedesktop.ConsoleKit.Session'>"
 "<method name='GetSessionType'><arg type='s' direction='out'/></method>"
 "<method name='GetSessionClass'><arg type='s' direction='out'/></method>"
 "<method name='GetSessionState'><arg type='s' direction='out'/></method>"
 "<method name='GetCreationTime'><arg type='s' direction='out'/></method>"
 "<method name='GetUnixUser'><arg type='u' direction='out'/></method>"
 "<method name='IsActive'><arg type='b' direction='out'/></method>"
 "<method name='GetSeatId'><arg type='o' direction='out'/></method>"
 "<method name='CanControlSession'><arg type='b' direction='out'/></method>"
 "<method name='TakeControl'><arg type='b' direction='in'/></method>"
 "<method name='ReleaseControl'/>"
 "</interface><interface name='org.bsd3.Test'>"
 "<method name='SetMode'><arg type='s' direction='in'/></method>"
 "<method name='GetLastPid'><arg type='u' direction='out'/></method>"
 "<method name='GetCounters'><arg type='u' direction='out'/><arg type='u' direction='out'/><arg type='u' direction='out'/></method>"
 "</interface></node>";

static void
method(GDBusConnection *bus, const char *sender, const char *path,
    const char *iface, const char *name, GVariant *args,
    GDBusMethodInvocation *invocation, gpointer data)
{
	GVariant *reply = NULL;
	int id = 0;
	(void)bus; (void)sender; (void)data;
	if (strcmp(iface, "org.bsd3.Test") == 0) {
		if (strcmp(name, "SetMode") == 0) {
			g_free(mode);
			g_variant_get(args, "(s)", &mode);
			if (strcmp(mode, "restart") == 0) {
				GError *error = NULL;
				GVariant *r;
				r = g_dbus_connection_call_sync(bus, "org.freedesktop.DBus",
				    "/org/freedesktop/DBus", "org.freedesktop.DBus", "ReleaseName",
				    g_variant_new("(s)", CK), G_VARIANT_TYPE("(u)"), 0,
				    3000, NULL, &error);
				g_assert_no_error(error); g_variant_unref(r);
				r = g_dbus_connection_call_sync(bus, "org.freedesktop.DBus",
				    "/org/freedesktop/DBus", "org.freedesktop.DBus", "RequestName",
				    g_variant_new("(su)", CK, 4U), G_VARIANT_TYPE("(u)"), 0,
				    3000, NULL, &error);
				g_assert_no_error(error); g_variant_unref(r);
			}
			reply = g_variant_new("()");
		} else if (strcmp(name, "GetCounters") == 0)
			reply = g_variant_new("(uuu)", takes, releases, g_hash_table_size(owners));
		else
			reply = g_variant_new("(u)", last_pid);
	} else if (strcmp(mode, "denied") == 0) {
		g_dbus_method_invocation_return_dbus_error(invocation,
		    "org.freedesktop.DBus.Error.AccessDenied", "Fixture denial");
		return;
	} else if (strcmp(iface, CK ".Manager") == 0) {
		if (strcmp(name, "GetSessionForUnixProcess") == 0) {
			g_variant_get(args, "(u)", &last_pid);
			if (last_pid == 99999999) {
				g_dbus_method_invocation_return_dbus_error(invocation,
				    CK ".Manager.Error.NoSessionForPid", "Absent");
				return;
			}
			reply = g_variant_new("(o)", strcmp(mode, "normal") == 0 ? ROOT "/Session1" :
			    strcmp(mode, "uid-mismatch") == 0 ? ROOT "/Session8" : ROOT "/Session6");
		} else {
			guint32 uid = 1000;
			GVariantBuilder builder;
			if (strcmp(name, "GetSessionsForUnixUser") == 0)
				g_variant_get(args, "(u)", &uid);
			if (strcmp(mode, "empty") == 0 || uid != 1000) {
				g_dbus_method_invocation_return_dbus_error(invocation,
				    CK ".Manager.Error.NoSessions", "Empty");
				return;
			}
			g_variant_builder_init(&builder, G_VARIANT_TYPE("ao"));
			if (strcmp(mode, "bad-path") == 0)
				g_variant_builder_add(&builder, "o", "/unrelated/Session1");
			else {
				/* Deliberately return the newer graphical session first. */
				const int order[] = {1, 3, 4, 5, 2};
				unsigned int i;
				for (i = 0; i < G_N_ELEMENTS(order); i++) {
					char *p = g_strdup_printf(ROOT "/Session%d", order[i]);
					g_variant_builder_add(&builder, "o", p);
					g_free(p);
				}
			}
			reply = g_variant_new("(ao)", &builder);
		}
	} else {
		const char *type, *class_name, *state, *created;
		const char *component = strrchr(path, '/');
		if (component != NULL)
			(void)sscanf(component, "/Session%d", &id);
		type = id == 1 ? "tty" : "wayland";
		class_name = id == 4 ? "greeter" : "user";
		state = id == 5 ? "closing" : id == 2 ? "online" : "active";
		created = id == 2 ? "2026-11-04T02:00:00Z" : "2026-11-04T03:00:00Z";
		if (strcmp(name, "GetSessionType") == 0)
			reply = g_variant_new("(s)", type);
		else if (strcmp(name, "GetSessionClass") == 0)
			reply = g_variant_new("(s)", class_name);
		else if (strcmp(name, "GetSessionState") == 0)
			reply = g_variant_new("(s)", state);
		else if (strcmp(name, "GetCreationTime") == 0)
			reply = g_variant_new("(s)", strcmp(mode, "bad-time") == 0 ? "garbage" : created);
		else if (strcmp(name, "GetUnixUser") == 0)
			reply = g_variant_new("(u)", id == 6 || id == 7 ? (guint32)getuid() :
			    id == 8 ? (guint32)getuid() + 1 : 1000U);
		else if (strcmp(name, "IsActive") == 0)
			reply = g_variant_new("(b)", strcmp(state, "active") == 0);
		else if (strcmp(name, "GetSeatId") == 0)
			reply = g_variant_new("(o)", ROOT "/Seat1");
		else if (strcmp(name, "CanControlSession") == 0)
			reply = g_variant_new("(b)", strcmp(mode, "unsupported") != 0);
		else if (strcmp(name, "TakeControl") == 0) {
			gboolean force;
			g_variant_get(args, "(b)", &force);
			g_assert_false(force);
			if (strcmp(mode, "take-fail") == 0 || g_hash_table_contains(owners, path)) {
				g_dbus_method_invocation_return_dbus_error(invocation,
				    CK ".Session.Error.Failed", "Fixture refused control");
				return;
			}
			g_hash_table_insert(owners, g_strdup(path), g_strdup(sender));
			takes++;
			reply = g_variant_new("()");
		} else if (strcmp(name, "ReleaseControl") == 0) {
			if (g_strcmp0(g_hash_table_lookup(owners, path), sender) != 0 ||
			    strcmp(mode, "release-fail") == 0) {
				g_dbus_method_invocation_return_dbus_error(invocation,
				    CK ".Session.Error.Failed", "Fixture refused release");
				return;
			}
			g_hash_table_remove(owners, path);
			releases++;
			reply = g_variant_new("()");
		}
	}
	if (reply != NULL)
		g_dbus_method_invocation_return_value(invocation, reply);
	else
		g_dbus_method_invocation_return_dbus_error(invocation,
		    "org.freedesktop.DBus.Error.UnknownMethod", "Fixture missing method");
}

static void
owner_changed(GDBusConnection *bus, const char *sender, const char *path,
    const char *iface, const char *signal, GVariant *parameters, gpointer data)
{
	const char *name, *old_owner, *new_owner;
	GHashTableIter iter;
	gpointer value;
	(void)bus; (void)sender; (void)path; (void)iface; (void)signal; (void)data;
	g_variant_get(parameters, "(&s&s&s)", &name, &old_owner, &new_owner);
	(void)old_owner;
	if (new_owner[0] != '\0')
		return;
	g_hash_table_iter_init(&iter, owners);
	while (g_hash_table_iter_next(&iter, NULL, &value)) {
		if (strcmp(value, name) == 0) {
			g_hash_table_iter_remove(&iter);
			releases++;
		}
	}
}

static const GDBusInterfaceVTable vtable = { .method_call = method };
static gboolean stop(gpointer data)
{ (void)data; g_main_loop_quit(loop); return (G_SOURCE_REMOVE); }

int
main(void)
{
	GError *error = NULL;
	GDBusConnection *bus = g_bus_get_sync(G_BUS_TYPE_SYSTEM, NULL, &error);
	GDBusNodeInfo *schema;
	GVariant *reply;
	int i;
	g_assert_no_error(error);
	mode = g_strdup("normal");
	owners = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);
	g_dbus_connection_signal_subscribe(bus, "org.freedesktop.DBus",
	    "org.freedesktop.DBus", "NameOwnerChanged", "/org/freedesktop/DBus",
	    NULL, G_DBUS_SIGNAL_FLAGS_NONE, owner_changed, NULL, NULL);
	schema = g_dbus_node_info_new_for_xml(xml, &error);
	g_assert_no_error(error);
	g_assert_cmpuint(g_dbus_connection_register_object(bus, ROOT "/Manager",
	    schema->interfaces[0], &vtable, NULL, NULL, &error), >, 0);
	g_assert_no_error(error);
	g_assert_cmpuint(g_dbus_connection_register_object(bus, ROOT "/Manager",
	    schema->interfaces[2], &vtable, NULL, NULL, &error), >, 0);
	g_assert_no_error(error);
	for (i = 1; i <= 8; i++) {
		char *path = g_strdup_printf(ROOT "/Session%d", i);
		g_assert_cmpuint(g_dbus_connection_register_object(bus, path,
		    schema->interfaces[1], &vtable, NULL, NULL, &error), >, 0);
		g_assert_no_error(error);
		g_free(path);
	}
	reply = g_dbus_connection_call_sync(bus, "org.freedesktop.DBus",
	    "/org/freedesktop/DBus", "org.freedesktop.DBus", "RequestName",
	    g_variant_new("(su)", CK, 4U), G_VARIANT_TYPE("(u)"), 0, 3000,
	    NULL, &error);
	g_assert_no_error(error);
	g_variant_unref(reply);
	g_print("READY\n"); fflush(stdout);
	loop = g_main_loop_new(NULL, FALSE);
	g_unix_signal_add(SIGTERM, stop, NULL);
	g_main_loop_run(loop);
	g_main_loop_unref(loop);
	g_dbus_node_info_unref(schema);
	g_object_unref(bus);
	g_free(mode);
	g_hash_table_unref(owners);
	return (0);
}
