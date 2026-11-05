/* SPDX-License-Identifier: BSD-2-Clause */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <gio/gunixfdlist.h>
#include <gio/gio.h>
#include <glib-unix.h>
#define CK "org.freedesktop.ConsoleKit"
#define ROOT "/org/freedesktop/ConsoleKit"
static GMainLoop *loop;
static guint32 last_pid;
static char *mode;
static GHashTable *owners;
static guint takes, releases, acknowledgements, switches, last_vt;
static GHashTable *devices;
static gboolean session6_active = TRUE;
struct fake_device { char *owner; gboolean paused; gboolean awaiting_ack; };
static const char xml[] =
 "<node><interface name='org.freedesktop.ConsoleKit.Manager'>"
 "<method name='GetSeats'><arg type='ao' direction='out'/></method>"
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
 "<method name='TakeDevice'><arg type='u' direction='in'/><arg type='u' direction='in'/><arg type='h' direction='out'/><arg type='b' direction='out'/></method>"
 "<method name='ReleaseDevice'><arg type='u' direction='in'/><arg type='u' direction='in'/></method>"
 "<method name='PauseDeviceComplete'><arg type='u' direction='in'/><arg type='u' direction='in'/></method>"
 "<signal name='PauseDevice'><arg type='u'/><arg type='u'/><arg type='s'/></signal>"
 "<signal name='ResumeDevice'><arg type='u'/><arg type='u'/><arg type='h'/></signal>"
 "<signal name='ActiveChanged'><arg type='b'/></signal>"
 "</interface><interface name='org.bsd3.Test'>"
 "<method name='SetMode'><arg type='s' direction='in'/></method>"
 "<method name='GetSwitchCounters'><arg type='u' direction='out'/><arg type='u' direction='out'/></method>"
 "<method name='GetLastPid'><arg type='u' direction='out'/></method>"
 "<method name='EmitPause'><arg type='u' direction='in'/><arg type='u' direction='in'/><arg type='s' direction='in'/></method>"
 "<method name='EmitResume'><arg type='u' direction='in'/><arg type='u' direction='in'/></method>"
 "<method name='EmitActive'><arg type='b' direction='in'/></method>"
 "<method name='GetDeviceCounters'><arg type='u' direction='out'/><arg type='u' direction='out'/></method>"
 "<method name='GetCounters'><arg type='u' direction='out'/><arg type='u' direction='out'/><arg type='u' direction='out'/></method>"
 "</interface><interface name='org.freedesktop.ConsoleKit.Seat'>"
 "<method name='GetName'><arg type='s' direction='out'/></method>"
 "<method name='CanActivateSessions'><arg type='b' direction='out'/></method>"
 "<property name='CanGraphical' type='b' access='read'/>"
 "<method name='SwitchTo'><arg type='u' direction='in'/></method>"
 "</interface></node>";

static void
free_device(gpointer data)
{
	struct fake_device *device = data;
	g_free(device->owner);
	g_free(device);
}

static void
clear_devices(const char *owner)
{
	GHashTableIter iter;
	gpointer value;
	g_hash_table_iter_init(&iter, devices);
	while (g_hash_table_iter_next(&iter, NULL, &value)) {
		struct fake_device *device = value;
		if (strcmp(device->owner, owner) == 0)
			g_hash_table_iter_remove(&iter);
	}
}

static GUnixFDList *
pipe_bundle(char marker, gboolean extra)
{
	GError *error = NULL;
	GUnixFDList *fds = g_unix_fd_list_new();
	int ends[2];
	g_assert_cmpint(pipe(ends), ==, 0);
	g_assert_cmpint(write(ends[1], &marker, 1), ==, 1);
	close(ends[1]);
	g_assert_cmpint(g_unix_fd_list_append(fds, ends[0], &error), ==, 0);
	g_assert_no_error(error);
	if (extra) {
		g_assert_cmpint(g_unix_fd_list_append(fds, ends[0], &error), ==, 1);
		g_assert_no_error(error);
	}
	close(ends[0]);
	return (fds);
}

static void
emit_device(GDBusConnection *bus, const char *name, GVariant *args)
{
	guint32 major, minor;
	const char *kind = NULL;
	char *key;
	struct fake_device *device;
	GError *error = NULL;
	if (strcmp(name, "EmitPause") == 0)
		g_variant_get(args, "(uu&s)", &major, &minor, &kind);
	else
		g_variant_get(args, "(uu)", &major, &minor);
	key = g_strdup_printf(ROOT "/Session6:%u:%u", major, minor);
	device = g_hash_table_lookup(devices, key);
	g_assert_nonnull(device);
	if (kind != NULL) {
		device->paused = TRUE;
		device->awaiting_ack = strcmp(kind, "pause") == 0;
		g_dbus_connection_emit_signal(bus, device->owner, ROOT "/Session6",
		    CK ".Session", "PauseDevice", args, &error);
		g_assert_no_error(error);
		if (strcmp(kind, "gone") == 0)
			g_hash_table_remove(devices, key);
	} else {
		GUnixFDList *fds = pipe_bundle('R', strcmp(mode, "extra-resume-fd") == 0);
		GDBusMessage *message = g_dbus_message_new_signal(ROOT "/Session6",
		    CK ".Session", "ResumeDevice");
		g_assert_true(device->paused);
		g_assert_false(device->awaiting_ack);
		device->paused = FALSE;
		g_dbus_message_set_destination(message, device->owner);
		g_dbus_message_set_body(message, g_variant_new("(uuh)", major, minor, 0));
		g_dbus_message_set_unix_fd_list(message, fds);
		g_assert_true(g_dbus_connection_send_message(bus, message, 0, NULL, &error));
		g_assert_no_error(error);
		g_object_unref(message);
		g_object_unref(fds);
	}
	g_free(key);
}

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
		} else if (strcmp(name, "EmitPause") == 0 || strcmp(name, "EmitResume") == 0) {
			emit_device(bus, name, args);
			reply = g_variant_new("()");
		} else if (strcmp(name, "EmitActive") == 0) {
			GError *error = NULL;
			g_variant_get(args, "(b)", &session6_active);
			g_dbus_connection_emit_signal(bus, NULL, ROOT "/Session6",
			    CK ".Session", "ActiveChanged", args, &error);
			g_assert_no_error(error);
			reply = g_variant_new("()");
		} else if (strcmp(name, "GetSwitchCounters") == 0)
			reply = g_variant_new("(uu)", switches, last_vt);
		else if (strcmp(name, "GetDeviceCounters") == 0)
			reply = g_variant_new("(uu)", g_hash_table_size(devices), acknowledgements);
		else if (strcmp(name, "GetCounters") == 0)
			reply = g_variant_new("(uuu)", takes, releases, g_hash_table_size(owners));
		else
			reply = g_variant_new("(u)", last_pid);
	} else if (strcmp(mode, "denied") == 0) {
		g_dbus_method_invocation_return_dbus_error(invocation,
		    "org.freedesktop.DBus.Error.AccessDenied", "Fixture denial");
		return;
	} else if (strcmp(iface, CK ".Manager") == 0) {
		if (strcmp(name, "GetSeats") == 0) {
			GVariantBuilder seats;
			g_variant_builder_init(&seats, G_VARIANT_TYPE("ao"));
			if (strcmp(mode, "no-seat") != 0) {
				const char *seat_path = strcmp(mode, "bad-seat-list") == 0 ? "/unrelated/seat" :
				    strcmp(mode, "legacy-seat") == 0 ? ROOT "/Seat1" : ROOT "/seat0";
				g_variant_builder_add(&seats, "o", seat_path);
				g_variant_builder_add(&seats, "o", seat_path);
			}
			reply = g_variant_new("(ao)", &seats);
		} else if (strcmp(name, "GetSessionForUnixProcess") == 0) {
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
	} else if (strcmp(iface, CK ".Seat") == 0) {
		if (strcmp(name, "CanActivateSessions") == 0) {
			g_dbus_method_invocation_return_value(invocation,
			    g_variant_new("(b)", strcmp(mode, "no-capabilities") != 0));
			return;
		}
		if (strcmp(name, "SwitchTo") == 0) {
			g_assert_cmpstr(g_hash_table_lookup(owners, ROOT "/Session6"), ==, sender);
			g_variant_get(args, "(u)", &last_vt);
			switches++;
			if (strcmp(mode, "switch-fail") == 0)
				g_dbus_method_invocation_return_dbus_error(invocation,
				    CK ".Seat.Error.Failed", "Fixture switch failure");
			else
				g_dbus_method_invocation_return_value(invocation, NULL);
			return;
		}
		if (strcmp(mode, "seat-name-error") == 0) {
			g_dbus_method_invocation_return_dbus_error(invocation,
			    "org.freedesktop.DBus.Error.AccessDenied", "Fixture seat denial");
			return;
		}
		reply = g_variant_new("(s)", strcmp(mode, "empty-seat-name") == 0 ? "" :
		    strcmp(mode, "unsafe-seat-name") == 0 ? "seat/unsafe" :
		    strcmp(mode, "custom-seat-name") == 0 ? "seat-lab" :
		    strcmp(mode, "underscore-seat-name") == 0 ? "seat_lab" :
		    strcmp(mode, "numeric-seat-name") == 0 ? "1seat" : "seat0");
	} else {
		const char *type, *class_name, *state, *created;
		const char *component = strrchr(path, '/');
		if (component != NULL)
			(void)sscanf(component, "/Session%d", &id);
		type = id == 1 ? "tty" : "wayland";
		class_name = id == 4 ? "greeter" : "user";
		state = id == 5 ? "closing" : id == 2 || (id == 6 && !session6_active) ? "online" : "active";
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
			reply = g_variant_new("(o)", strcmp(mode, "no-seat") == 0 ? "/" :
			    strcmp(mode, "bad-seat-path") == 0 ? "/unrelated/seat0" :
			    strcmp(mode, "legacy-seat") == 0 ? ROOT "/Seat1" : ROOT "/seat0");
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
			clear_devices(sender);
			releases++;
			reply = g_variant_new("()");
		} else if (strcmp(name, "TakeDevice") == 0 || strcmp(name, "ReleaseDevice") == 0 ||
		    strcmp(name, "PauseDeviceComplete") == 0) {
			guint32 major, minor;
			char *key;
			struct fake_device *device;
			g_variant_get(args, "(uu)", &major, &minor);
			key = g_strdup_printf("%s:%u:%u", path, major, minor);
			device = g_hash_table_lookup(devices, key);
			if (g_strcmp0(g_hash_table_lookup(owners, path), sender) != 0 ||
			    (strcmp(name, "TakeDevice") == 0 && device != NULL) ||
			    (strcmp(name, "TakeDevice") != 0 && device == NULL) ||
			    strcmp(mode, "device-release-fail") == 0) {
				g_free(key);
				g_dbus_method_invocation_return_dbus_error(invocation,
				    CK ".Session.Error.Failed", "Fixture rejected device operation");
				return;
			}
			if (strcmp(name, "TakeDevice") == 0) {
				GUnixFDList *fds = pipe_bundle('T', strcmp(mode, "extra-fd") == 0);
				device = g_new0(struct fake_device, 1);
				device->owner = g_strdup(sender);
				device->paused = strcmp(mode, "paused") == 0 || !session6_active;
				g_hash_table_insert(devices, key, device);
				g_dbus_method_invocation_return_value_with_unix_fd_list(invocation,
				    g_variant_new("(hb)", 0, device->paused), fds);
				g_object_unref(fds);
				return;
			}
			if (strcmp(name, "ReleaseDevice") == 0)
				g_hash_table_remove(devices, key);
			else {
				g_assert_true(device->awaiting_ack);
				device->awaiting_ack = FALSE;
				acknowledgements++;
			}
			g_free(key);
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
	clear_devices(name);
	g_hash_table_iter_init(&iter, owners);
	while (g_hash_table_iter_next(&iter, NULL, &value)) {
		if (strcmp(value, name) == 0) {
			g_hash_table_iter_remove(&iter);
			releases++;
		}
	}
}

static GVariant *
mock_property(GDBusConnection *bus, const char *sender, const char *path,
    const char *iface, const char *name, GError **error, gpointer data)
{
	(void)bus; (void)sender; (void)path; (void)data;
	if (strcmp(iface, CK ".Seat") == 0 && strcmp(name, "CanGraphical") == 0) {
		if (strcmp(mode, "capability-error") == 0) {
			g_set_error(error, G_DBUS_ERROR, G_DBUS_ERROR_ACCESS_DENIED, "Fixture capability failure");
			return (NULL);
		}
		return (g_variant_new_boolean(strcmp(mode, "no-capabilities") != 0));
	}
	g_set_error(error, G_DBUS_ERROR, G_DBUS_ERROR_UNKNOWN_PROPERTY, "Unknown fixture property");
	return (NULL);
}
static const GDBusInterfaceVTable vtable = { .method_call = method, .get_property = mock_property };
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
	devices = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, free_device);
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
	g_assert_cmpuint(g_dbus_connection_register_object(bus, ROOT "/seat0",
	    schema->interfaces[3], &vtable, NULL, NULL, &error), >, 0);
	g_assert_no_error(error);
	g_assert_cmpuint(g_dbus_connection_register_object(bus, ROOT "/Seat1",
	    schema->interfaces[3], &vtable, NULL, NULL, &error), >, 0);
	g_assert_no_error(error);
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
	g_hash_table_unref(devices);
	return (0);
}
