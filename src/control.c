/* SPDX-License-Identifier: BSD-2-Clause
 * Copyright (c) 2026 Denis Lemire <denis@lemire.name>
 * Session-controller ownership only. Device access is deliberately absent.
 */
#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <gio/gio.h>
#include "bsd3/sd-login.h"
#include "control.h"

#define CK "org.freedesktop.ConsoleKit"
#define CK_SESSION CK ".Session"
#define LOGIN "org.freedesktop.login1"

struct controller {
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

static void
destroy_controller(gpointer data)
{
	struct controller *c = data;
	if (c->watch != 0)
		g_bus_unwatch_name(c->watch);
	/* Closing this unique backend connection releases only this controller.
	 * ConsoleKit owns device revocation and implicit ReleaseControl on death. */
	if (c->backend != NULL) {
		g_dbus_connection_close_sync(c->backend, NULL, NULL);
		g_object_unref(c->backend);
	}
	g_free(c->id);
	g_free(c->sender);
	g_free(c->path);
	g_free(c->backend_owner);
	g_free(c);
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

static struct controller *
new_controller(const char *id, const char *sender, const char *owner, GError **error)
{
	struct controller *c = g_new0(struct controller, 1);
	char *address;
	GVariant *reply;
	gboolean capable;
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
	if (strcmp(method, "ReleaseControl") != 0) {
		failure(invocation, "org.freedesktop.DBus.Error.UnknownMethod",
		    "Controller method is not implemented");
		return;
	}
	if (c == NULL || strcmp(c->sender, sender) != 0) {
		failure(invocation, "org.freedesktop.DBus.Error.AccessDenied",
		    "Only the controlling connection may release control");
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
