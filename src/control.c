/* SPDX-License-Identifier: BSD-2-Clause
 * Copyright (c) 2026 Denis Lemire <denis@lemire.name>
 * Authenticated controller and device-descriptor protocol lifecycle.
 * Kernel device revocation/VT behavior requires separate integration validation.
 */
#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <gio/gio.h>
#include <gio/gunixfdlist.h>
#include "bsd3/sd-login.h"
#include "control.h"

#define CK "org.freedesktop.ConsoleKit"
#define CK_SESSION CK ".Session"
#define LOGIN "org.freedesktop.login1"

struct device {
	gboolean paused;
	gboolean awaiting_ack;
};

struct controller {
	gatomicrefcount refs;
	gboolean closing;
	GHashTable *devices;
	guint filter;
	guint signal_match;
	char *id;
	char *sender;
	char *path;
	char *backend_owner;
	GDBusConnection *backend;
	guint watch;
};
static GHashTable *controllers;
static GDBusConnection *front_bus;
static guint backend_watch;

static void
failure(GDBusMethodInvocation *call, const char *name, const char *message)
{
	g_dbus_method_invocation_return_dbus_error(call, name, message);
}

static struct controller *
controller_ref(struct controller *c)
{
	g_atomic_ref_count_inc(&c->refs);
	return (c);
}

static void
controller_unref(gpointer data)
{
	struct controller *c = data;
	if (!g_atomic_ref_count_dec(&c->refs))
		return;
	g_hash_table_unref(c->devices);
	g_free(c->id);
	g_free(c->sender);
	g_free(c->path);
	g_free(c->backend_owner);
	g_free(c);
}

static void
destroy_controller(gpointer data)
{
	struct controller *c = data;
	c->closing = TRUE;
	if (c->watch != 0)
		g_bus_unwatch_name(c->watch);
	if (c->backend != NULL) {
		if (c->signal_match != 0)
			g_dbus_connection_signal_unsubscribe(c->backend, c->signal_match);
		if (c->filter != 0)
			g_dbus_connection_remove_filter(c->backend, c->filter);
		/* ConsoleKit sees a distinct controller connection close and revokes
		 * only its leases. The proxy never retains a device descriptor. */
		g_dbus_connection_close_sync(c->backend, NULL, NULL);
		g_clear_object(&c->backend);
	}
	controller_unref(c);
}

static void
client_vanished(GDBusConnection *bus, const char *name, gpointer data)
{
	struct controller *c = data;
	char *id = g_strdup(c->id);
	(void)bus; (void)name;
	g_hash_table_remove(controllers, id);
	g_free(id);
}

static void
backend_changed(GDBusConnection *bus, const char *sender, const char *path,
    const char *iface, const char *signal, GVariant *parameters, gpointer data)
{
	const char *name, *old_owner, *new_owner;
	(void)bus; (void)sender; (void)path; (void)iface; (void)signal; (void)data;
	g_variant_get(parameters, "(&s&s&s)", &name, &old_owner, &new_owner);
	(void)new_owner;
	if (strcmp(name, CK) == 0 && old_owner[0] != '\0')
		g_hash_table_remove_all(controllers);
}

void
bsd3_controls_start(GDBusConnection *bus)
{
	front_bus = g_object_ref(bus);
	controllers = g_hash_table_new_full(g_str_hash, g_str_equal, NULL,
	    destroy_controller);
	backend_watch = g_dbus_connection_signal_subscribe(bus,
	    "org.freedesktop.DBus", "org.freedesktop.DBus", "NameOwnerChanged",
	    "/org/freedesktop/DBus", CK, G_DBUS_SIGNAL_FLAGS_NONE,
	    backend_changed, NULL, NULL);
}

void
bsd3_controls_stop(void)
{
	if (front_bus == NULL)
		return;
	g_dbus_connection_signal_unsubscribe(front_bus, backend_watch);
	g_clear_pointer(&controllers, g_hash_table_unref);
	g_clear_object(&front_bus);
}

static gboolean
credential(const char *sender, const char *method, guint32 *ret)
{
	GError *error = NULL;
	GVariant *reply = g_dbus_connection_call_sync(front_bus,
	    "org.freedesktop.DBus", "/org/freedesktop/DBus", "org.freedesktop.DBus",
	    method, g_variant_new("(s)", sender), G_VARIANT_TYPE("(u)"),
	    G_DBUS_CALL_FLAGS_NONE, 3000, NULL, &error);
	if (reply == NULL) {
		g_clear_error(&error);
		return (FALSE);
	}
	g_variant_get(reply, "(u)", ret);
	g_variant_unref(reply);
	return (TRUE);
}

static char *
backend_owner(void)
{
	GError *error = NULL;
	char *owner = NULL;
	GVariant *reply = g_dbus_connection_call_sync(front_bus,
	    "org.freedesktop.DBus", "/org/freedesktop/DBus", "org.freedesktop.DBus",
	    "GetNameOwner", g_variant_new("(s)", CK), G_VARIANT_TYPE("(s)"),
	    G_DBUS_CALL_FLAGS_NONE, 3000, NULL, &error);
	if (reply != NULL) {
		g_variant_get(reply, "(s)", &owner);
		g_variant_unref(reply);
	}
	g_clear_error(&error);
	return (owner);
}

static gboolean
authorized(const char *id, const char *sender)
{
	guint32 uid, pid;
	uid_t owner;
	char *actual_session = NULL;
	gboolean matches;
	if (!credential(sender, "GetConnectionUnixUser", &uid) ||
	    !credential(sender, "GetConnectionUnixProcessID", &pid) ||
	    pid == 0 || pid > G_MAXINT32)
		return (FALSE);
	if (sd_session_get_uid(id, &owner) < 0 || (uid_t)uid != owner)
		return (FALSE);
	if (sd_pid_get_session((pid_t)pid, &actual_session) < 0)
		return (FALSE);
	matches = strcmp(actual_session, id) == 0;
	free(actual_session);
	return (matches);
}

static GVariant *
backend_call(struct controller *c, const char *method, GVariant *args,
    const char *type, GError **error)
{
	return (g_dbus_connection_call_sync(c->backend, c->backend_owner, c->path, CK_SESSION,
	    method, args, G_VARIANT_TYPE(type), G_DBUS_CALL_FLAGS_NO_AUTO_START,
	    3000, NULL, error));
}

static guint64
device_key(guint32 major, guint32 minor)
{
	return (((guint64)major << 32) | minor);
}

struct queued_signal {
	struct controller *controller;
	GDBusMessage *message;
};

static void
free_queued_signal(gpointer data)
{
	struct queued_signal *queued = data;
	g_object_unref(queued->message);
	controller_unref(queued->controller);
	g_free(queued);
}

static gboolean
relay_signal(gpointer data)
{
	struct queued_signal *queued = data;
	struct controller *c = queued->controller;
	GDBusMessage *message = queued->message, *forward;
	GVariant *body = g_dbus_message_get_body(message);
	GUnixFDList *fds = g_dbus_message_get_unix_fd_list(message);
	const char *member = g_dbus_message_get_member(message);
	char *path;
	guint32 major, minor;
	guint64 key;
	struct device *device;
	gint handle;
	if (c->closing || g_hash_table_lookup(controllers, c->id) != c)
		return (G_SOURCE_REMOVE);
	path = g_strconcat("/org/freedesktop/login1/session/", c->id, NULL);
	if (body == NULL)
		goto invalid;
	if (strcmp(member, "ActiveChanged") == 0) {
		GVariantBuilder changed;
		gboolean active;
		if (!g_variant_is_of_type(body, G_VARIANT_TYPE("(b)")) ||
		    (fds != NULL && g_unix_fd_list_get_length(fds) != 0))
			goto invalid;
		g_variant_get(body, "(b)", &active);
		g_variant_builder_init(&changed, G_VARIANT_TYPE("a{sv}"));
		g_variant_builder_add(&changed, "{sv}", "Active", g_variant_new_boolean(active));
		g_dbus_connection_emit_signal(front_bus, NULL, path,
		    "org.freedesktop.DBus.Properties", "PropertiesChanged",
		    g_variant_new("(sa{sv}as)", LOGIN ".Session", &changed, NULL), NULL);
		g_free(path);
		return (G_SOURCE_REMOVE);
	}
	if (strcmp(member, "PauseDevice") == 0) {
		const char *kind;
		if (!g_variant_is_of_type(body, G_VARIANT_TYPE("(uus)")) ||
		    (fds != NULL && g_unix_fd_list_get_length(fds) != 0))
			goto invalid;
		g_variant_get(body, "(uu&s)", &major, &minor, &kind);
		if (strcmp(kind, "pause") != 0 && strcmp(kind, "force") != 0 &&
		    strcmp(kind, "gone") != 0)
			goto invalid;
		key = device_key(major, minor);
		device = g_hash_table_lookup(c->devices, &key);
		if (device == NULL)
			goto done;
		device->paused = TRUE;
		device->awaiting_ack = strcmp(kind, "pause") == 0;
		if (strcmp(kind, "gone") == 0)
			g_hash_table_remove(c->devices, &key);
	} else {
		if (!g_variant_is_of_type(body, G_VARIANT_TYPE("(uuh)")) ||
		    fds == NULL || g_unix_fd_list_get_length(fds) != 1)
			goto invalid;
		g_variant_get(body, "(uuh)", &major, &minor, &handle);
		if (handle != 0)
			goto invalid;
		key = device_key(major, minor);
		device = g_hash_table_lookup(c->devices, &key);
		if (device == NULL)
			goto done;
		if (!device->paused || device->awaiting_ack)
			goto invalid;
		device->paused = FALSE;
	}
	/* Resume descriptors are unicast only to the authenticated controller.
	 * Copy the ancillary list with the body, never just the integer handle. */
	forward = g_dbus_message_new_signal(path, LOGIN ".Session", member);
	g_dbus_message_set_destination(forward, c->sender);
	g_dbus_message_set_body(forward, body);
	if (fds != NULL)
		g_dbus_message_set_unix_fd_list(forward, fds);
	if (!g_dbus_connection_send_message(front_bus, forward,
	    G_DBUS_SEND_MESSAGE_FLAGS_NONE, NULL, NULL)) {
		g_object_unref(forward);
		goto invalid;
	}
	g_object_unref(forward);
done:
	g_free(path);
	return (G_SOURCE_REMOVE);
invalid:
	g_free(path);
	/* A malformed descriptor bundle or ordering error cannot preserve a safe
	 * ownership state. Closing this controller lets the backend revoke it. */
	g_hash_table_remove(controllers, c->id);
	return (G_SOURCE_REMOVE);
}

static GDBusMessage *
filter_signal(GDBusConnection *bus, GDBusMessage *message, gboolean incoming,
    gpointer data)
{
	struct controller *c = data;
	const char *member = g_dbus_message_get_member(message);
	struct queued_signal *queued;
	GSource *source;
	(void)bus;
	if (!incoming || g_dbus_message_get_message_type(message) != G_DBUS_MESSAGE_TYPE_SIGNAL ||
	    g_strcmp0(g_dbus_message_get_sender(message), c->backend_owner) != 0 ||
	    g_strcmp0(g_dbus_message_get_path(message), c->path) != 0 ||
	    g_strcmp0(g_dbus_message_get_interface(message), CK_SESSION) != 0 ||
	    (g_strcmp0(member, "PauseDevice") != 0 && g_strcmp0(member, "ResumeDevice") != 0 &&
	    g_strcmp0(member, "ActiveChanged") != 0))
		return (message);
	/* Filters run in GDBus's worker thread. State and map mutations must run
	 * on the daemon main loop; queued work holds both controller and message. */
	queued = g_new0(struct queued_signal, 1);
	queued->controller = controller_ref(c);
	queued->message = g_object_ref(message);
	source = g_idle_source_new();
	g_source_set_priority(source, G_PRIORITY_DEFAULT);
	g_source_set_callback(source, relay_signal, queued, free_queued_signal);
	g_source_attach(source, NULL);
	g_source_unref(source);
	return (message);
}

static void
matched_signal(GDBusConnection *bus, const char *sender, const char *path,
    const char *iface, const char *member, GVariant *parameters, gpointer data)
{
	/* The subscription installs the match rule. The message filter retains
	 * SCM_RIGHTS data, which a normal GVariant-only signal callback cannot. */
	(void)bus; (void)sender; (void)path; (void)iface;
	(void)member; (void)parameters; (void)data;
}

static void
device_call(struct controller *c, const char *method, GVariant *args,
    GDBusMethodInvocation *invocation)
{
	guint32 major, minor;
	guint64 key;
	struct device *device;
	GVariant *reply = NULL;
	GError *error = NULL;
	g_variant_get(args, "(uu)", &major, &minor);
	key = device_key(major, minor);
	device = g_hash_table_lookup(c->devices, &key);
	if (strcmp(method, "TakeDevice") == 0) {
		GUnixFDList *fds = NULL;
		gboolean paused;
		gint handle;
		guint64 *stored_key;
		if (device != NULL) {
			failure(invocation, "org.freedesktop.DBus.Error.InvalidArgs", "Device already taken");
			return;
		}
		if (g_hash_table_size(c->devices) >= 256) {
			failure(invocation, "org.freedesktop.DBus.Error.LimitsExceeded", "Device limit reached");
			return;
		}
		reply = g_dbus_connection_call_with_unix_fd_list_sync(c->backend,
		    c->backend_owner, c->path, CK_SESSION, method, args,
		    G_VARIANT_TYPE("(hb)"), G_DBUS_CALL_FLAGS_NO_AUTO_START, 3000,
		    NULL, &fds, NULL, &error);
		if (reply == NULL) {
			/* A timeout may have granted a lease without returning it. Close
			 * the controller rather than retain untracked backend authority. */
			if (g_error_matches(error, G_IO_ERROR, G_IO_ERROR_TIMED_OUT) ||
			    g_error_matches(error, G_IO_ERROR, G_IO_ERROR_CLOSED) ||
			    g_error_matches(error, G_DBUS_ERROR, G_DBUS_ERROR_NO_REPLY) ||
			    g_error_matches(error, G_DBUS_ERROR, G_DBUS_ERROR_DISCONNECTED) ||
			    g_error_matches(error, G_DBUS_ERROR, G_DBUS_ERROR_NAME_HAS_NO_OWNER) ||
			    g_error_matches(error, G_DBUS_ERROR, G_DBUS_ERROR_SERVICE_UNKNOWN))
				g_hash_table_remove(controllers, c->id);
			failure(invocation, "org.freedesktop.DBus.Error.Failed", error->message);
			g_clear_object(&fds);
			g_error_free(error);
			return;
		}
		g_variant_get(reply, "(hb)", &handle, &paused);
		if (fds == NULL || g_unix_fd_list_get_length(fds) != 1 || handle != 0) {
			GVariant *released = backend_call(c, "ReleaseDevice", args, "()", &error);
			if (released == NULL) {
				g_clear_error(&error);
				g_hash_table_remove(controllers, c->id);
			} else
				g_variant_unref(released);
			failure(invocation, "org.freedesktop.DBus.Error.Failed", "Invalid backend descriptor bundle");
			g_clear_object(&fds);
			g_variant_unref(reply);
			return;
		}
		stored_key = g_new(guint64, 1);
		*stored_key = key;
		device = g_new0(struct device, 1);
		device->paused = paused;
		g_hash_table_insert(c->devices, stored_key, device);
		g_dbus_method_invocation_return_value_with_unix_fd_list(invocation, reply, fds);
		g_variant_unref(reply);
		g_object_unref(fds);
		return;
	}
	if (device == NULL || (strcmp(method, "PauseDeviceComplete") == 0 && !device->awaiting_ack)) {
		failure(invocation, "org.freedesktop.DBus.Error.InvalidArgs", "No matching device or pending pause");
		return;
	}
	reply = backend_call(c, method, args, "()", &error);
	if (reply == NULL) {
		g_hash_table_remove(controllers, c->id);
		failure(invocation, "org.freedesktop.DBus.Error.Failed", error->message);
		g_error_free(error);
		return;
	}
	g_variant_unref(reply);
	if (strcmp(method, "ReleaseDevice") == 0)
		g_hash_table_remove(c->devices, &key);
	else
		device->awaiting_ack = FALSE;
	g_dbus_method_invocation_return_value(invocation, NULL);
}

static struct controller *
new_controller(const char *id, const char *sender, const char *owner, GError **error)
{
	struct controller *c = g_new0(struct controller, 1);
	char *address;
	GVariant *reply;
	gboolean capable;
	g_atomic_ref_count_init(&c->refs);
	c->devices = g_hash_table_new_full(g_int64_hash, g_int64_equal, g_free, g_free);
	c->id = g_strdup(id);
	c->sender = g_strdup(sender);
	c->backend_owner = g_strdup(owner);
	c->path = g_strconcat("/org/freedesktop/ConsoleKit/", id, NULL);
	address = g_dbus_address_get_for_bus_sync(G_BUS_TYPE_SYSTEM, NULL, error);
	if (address == NULL)
		goto fail;
	/* Never reuse GLib's shared bus connection: ConsoleKit tracks the unique
	 * sender as controller. Separate frontend clients need separate lifetimes. */
	c->backend = g_dbus_connection_new_for_address_sync(address,
	    G_DBUS_CONNECTION_FLAGS_AUTHENTICATION_CLIENT |
	    G_DBUS_CONNECTION_FLAGS_MESSAGE_BUS_CONNECTION, NULL, NULL, error);
	g_free(address);
	if (c->backend == NULL)
		goto fail;
	g_dbus_connection_set_exit_on_close(c->backend, FALSE);
	reply = backend_call(c, "CanControlSession", NULL, "(b)", error);
	if (reply == NULL)
		goto fail;
	g_variant_get(reply, "(b)", &capable);
	g_variant_unref(reply);
	if (!capable) {
		g_set_error_literal(error, G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED,
		    "ConsoleKit session controller is unavailable");
		goto fail;
	}
	c->filter = g_dbus_connection_add_filter(c->backend, filter_signal,
	    controller_ref(c), controller_unref);
	c->signal_match = g_dbus_connection_signal_subscribe(c->backend,
	    c->backend_owner, CK_SESSION, NULL, c->path, NULL, G_DBUS_SIGNAL_FLAGS_NONE,
	    matched_signal, NULL, NULL);
	reply = backend_call(c, "TakeControl", g_variant_new("(b)", FALSE),
	    "()", error);
	if (reply == NULL)
		goto fail;
	g_variant_unref(reply);
	return (c);
fail:
	destroy_controller(c);
	return (NULL);
}

void
bsd3_control_call(const char *id, const char *method, GVariant *args,
    GDBusMethodInvocation *invocation)
{
	const char *sender = g_dbus_method_invocation_get_sender(invocation);
	struct controller *c = g_hash_table_lookup(controllers, id);
	GError *error = NULL;
	GVariant *reply;
	if (strcmp(method, "TakeControl") == 0) {
		gboolean force;
		g_autofree char *owner = NULL;
		g_autofree char *after = NULL;
		g_variant_get(args, "(b)", &force);
		if (force) {
			failure(invocation, "org.freedesktop.DBus.Error.NotSupported",
			    "Forced takeover is not implemented");
			return;
		}
		owner = backend_owner();
		if (owner == NULL || !authorized(id, sender)) {
			failure(invocation, "org.freedesktop.DBus.Error.AccessDenied",
			    "Caller must own and belong to the requested session");
			return;
		}
		after = backend_owner();
		if (g_strcmp0(owner, after) != 0) {
			failure(invocation, "org.freedesktop.DBus.Error.Failed",
			    "Session backend changed during authorization");
			return;
		}
		if (c != NULL) {
			if (strcmp(c->sender, sender) == 0)
				g_dbus_method_invocation_return_value(invocation, NULL);
			else
				failure(invocation, LOGIN ".SessionBusy",
				    "A different connection controls this session");
			return;
		}
		c = new_controller(id, sender, owner, &error);
		if (c == NULL) {
			const char *name = g_error_matches(error, G_IO_ERROR,
			    G_IO_ERROR_NOT_SUPPORTED) ?
			    "org.freedesktop.DBus.Error.NotSupported" :
			    "org.freedesktop.DBus.Error.Failed";
			failure(invocation, name, error->message);
			g_error_free(error);
			return;
		}
		g_hash_table_insert(controllers, c->id, c);
		c->watch = g_bus_watch_name_on_connection(front_bus, sender,
		    G_BUS_NAME_WATCHER_FLAGS_NONE, NULL, client_vanished, c, NULL);
		g_dbus_method_invocation_return_value(invocation, NULL);
		return;
	}
	if (strcmp(method, "ReleaseControl") != 0 && strcmp(method, "TakeDevice") != 0 &&
	    strcmp(method, "ReleaseDevice") != 0 && strcmp(method, "PauseDeviceComplete") != 0) {
		failure(invocation, "org.freedesktop.DBus.Error.UnknownMethod",
		    "Controller method is not implemented");
		return;
	}
	if (c == NULL || strcmp(c->sender, sender) != 0) {
		failure(invocation, "org.freedesktop.DBus.Error.AccessDenied",
		    "Only the controlling connection may release control");
		return;
	}
	if (strcmp(method, "ReleaseControl") != 0) {
		device_call(c, method, args, invocation);
		return;
	}
	reply = backend_call(c, "ReleaseControl", NULL, "()", &error);
	/* Drop local ownership even if the backend fails; close releases the unique
	 * backend sender. A stale local controller must never survive a failure. */
	g_hash_table_remove(controllers, id);
	if (reply == NULL) {
		failure(invocation, "org.freedesktop.DBus.Error.Failed", error->message);
		g_error_free(error);
		return;
	}
	g_variant_unref(reply);
	g_dbus_method_invocation_return_value(invocation, NULL);
}

/* Canonical bus component encoding: underscores and leading digits are escaped. */
char *
bsd3_seat_component(const char *name)
{
	GString *encoded = g_string_new(NULL);
	const unsigned char *p;
	for (p = (const unsigned char *)name; *p != '\0'; p++) {
		if (g_ascii_isalpha(*p) ||
		    (p != (const unsigned char *)name && g_ascii_isdigit(*p)))
			g_string_append_c(encoded, *p);
		else
			g_string_append_printf(encoded, "_%02x", *p);
	}
	return (g_string_free(encoded, FALSE));
}

void
bsd3_seat_call(const char *component, GVariant *args,
    GDBusMethodInvocation *invocation)
{
	const char *sender = g_dbus_method_invocation_get_sender(invocation);
	struct controller *c;
	guint32 pid, vt;
	gboolean active;
	char *id = NULL, *seat_path = NULL, *seat_name = NULL, *encoded = NULL;
	GVariant *reply = NULL;
	GError *error = NULL;
	g_autofree char *owner = NULL;

	g_variant_get(args, "(u)", &vt);
	if (vt == 0) {
		failure(invocation, "org.freedesktop.DBus.Error.InvalidArgs",
		    "VT numbers start at one");
		return;
	}
	if (!credential(sender, "GetConnectionUnixProcessID", &pid) ||
	    pid == 0 || pid > G_MAXINT32 || sd_pid_get_session((pid_t)pid, &id) < 0)
		goto denied;
	c = g_hash_table_lookup(controllers, id);
	if (c == NULL || c->closing || strcmp(c->sender, sender) != 0 ||
	    !authorized(id, sender))
		goto denied;
	owner = backend_owner();
	if (g_strcmp0(owner, c->backend_owner) != 0)
		goto denied;
	/* Query through this controller's pinned backend, never a replacement owner. */
	reply = backend_call(c, "IsActive", NULL, "(b)", &error);
	if (reply == NULL)
		goto failed;
	g_variant_get(reply, "(b)", &active);
	g_clear_pointer(&reply, g_variant_unref);
	if (!active)
		goto denied;
	reply = backend_call(c, "GetSeatId", NULL, "(o)", &error);
	if (reply == NULL)
		goto failed;
	g_variant_get(reply, "(o)", &seat_path);
	g_clear_pointer(&reply, g_variant_unref);
	if (!g_str_has_prefix(seat_path, "/org/freedesktop/ConsoleKit/") ||
	    seat_path[strlen("/org/freedesktop/ConsoleKit/")] == '\0' ||
	    strchr(seat_path + strlen("/org/freedesktop/ConsoleKit/"), '/') != NULL)
		goto denied;
	reply = g_dbus_connection_call_sync(c->backend, c->backend_owner,
	    seat_path, CK ".Seat", "GetName", NULL, G_VARIANT_TYPE("(s)"),
	    G_DBUS_CALL_FLAGS_NO_AUTO_START, 3000, NULL, &error);
	if (reply == NULL)
		goto failed;
	g_variant_get(reply, "(s)", &seat_name);
	g_clear_pointer(&reply, g_variant_unref);
	if (seat_name[0] == '\0' || strlen(seat_name) > 255)
		goto denied;
	for (const unsigned char *p = (const unsigned char *)seat_name; *p != '\0'; p++)
		if (!g_ascii_isalnum(*p) && *p != '_' && *p != '-')
			goto denied;
	encoded = bsd3_seat_component(seat_name);
	if (strcmp(encoded, component) != 0)
		goto denied;
	/* Backend/kernel supplies the platform's VT range. No Linux VT limit. */
	reply = g_dbus_connection_call_sync(c->backend, c->backend_owner,
	    seat_path, CK ".Seat", "SwitchTo", g_variant_new("(u)", vt),
	    G_VARIANT_TYPE("()"), G_DBUS_CALL_FLAGS_NO_AUTO_START, 3000, NULL, &error);
	if (reply == NULL)
		goto failed;
	g_dbus_method_invocation_return_value(invocation, NULL);
	goto done;
denied:
	failure(invocation, "org.freedesktop.DBus.Error.AccessDenied",
	    "VT switching requires the active controller on this seat");
	goto done;
failed:
	failure(invocation, "org.freedesktop.DBus.Error.Failed", error->message);
done:
	g_clear_pointer(&reply, g_variant_unref);
	g_clear_error(&error);
	free(id);
	g_free(seat_path);
	g_free(seat_name);
	g_free(encoded);
}
