/* SPDX-License-Identifier: BSD-2-Clause */
#include <sys/stat.h>
#include <string.h>
#include <unistd.h>
#include "bsd3/greeter-session.h"

#define CK "org.freedesktop.ConsoleKit"
#define ROOT "/org/freedesktop/ConsoleKit/"
#define MANAGER ROOT "Manager"

GDBusConnection *
bsd3_open_display_session(uid_t uid, const char *service, const char *device,
    unsigned vt, gboolean greeter, char **cookie, char **session, char **runtime,
    GError **error)
{
	GVariantBuilder params;
	struct stat st;
	g_autoptr(GDBusConnection) bus = NULL;
	g_autoptr(GVariant) owner_reply = NULL, opened = NULL, resolved = NULL, dir = NULL;
	g_autofree char *address = NULL;
	const char *owner, *token, *path, *directory;
	const char terminals[] = "0123456789abcdefghijklmnopqrstuv";

	if (geteuid() != 0 || uid == 0 || uid == (uid_t)-1 || !service || !*service ||
	    !device || vt == 0 || vt >= sizeof(terminals) ||
	    strlen(device) != 10 || strncmp(device, "/dev/ttyv", 9) ||
	    device[9] != terminals[vt - 1] || !cookie || !session || !runtime) {
		g_set_error_literal(error, G_IO_ERROR, G_IO_ERROR_INVALID_ARGUMENT,
		    "Expected an authenticated user and matching FreeBSD VT");
		return NULL;
	}
	address = g_dbus_address_get_for_bus_sync(G_BUS_TYPE_SYSTEM, NULL, error);
	if (!address) return NULL;
	bus = g_dbus_connection_new_for_address_sync(address,
	    G_DBUS_CONNECTION_FLAGS_AUTHENTICATION_CLIENT | G_DBUS_CONNECTION_FLAGS_MESSAGE_BUS_CONNECTION,
	    NULL, NULL, error);
	if (!bus) return NULL;
	owner_reply = g_dbus_connection_call_sync(bus, "org.freedesktop.DBus", "/org/freedesktop/DBus",
	    "org.freedesktop.DBus", "GetNameOwner", g_variant_new("(s)", CK),
	    G_VARIANT_TYPE("(s)"), 0, 3000, NULL, error);
	if (!owner_reply) return NULL;
	g_variant_get(owner_reply, "(&s)", &owner);
	g_variant_builder_init(&params, G_VARIANT_TYPE("a(sv)"));
	g_variant_builder_add(&params, "(sv)", "unix-user", g_variant_new_int32(uid));
	g_variant_builder_add(&params, "(sv)", "session-type", g_variant_new_string("wayland"));
	g_variant_builder_add(&params, "(sv)", "session-class", g_variant_new_string(greeter ? "greeter" : "user"));
	g_variant_builder_add(&params, "(sv)", "session-service", g_variant_new_string(service));
	g_variant_builder_add(&params, "(sv)", "is-local", g_variant_new_boolean(TRUE));
	g_variant_builder_add(&params, "(sv)", "display-device", g_variant_new_string(device));
	g_variant_builder_add(&params, "(sv)", "vtnr", g_variant_new_uint32(vt));
	opened = g_dbus_connection_call_sync(bus, owner, MANAGER, CK ".Manager",
	    "OpenSessionWithParameters", g_variant_new("(a(sv))", &params), G_VARIANT_TYPE("(s)"),
	    0, 10000, NULL, error);
	if (!opened) return NULL;
	g_variant_get(opened, "(&s)", &token);
	resolved = g_dbus_connection_call_sync(bus, owner, MANAGER, CK ".Manager", "GetSessionForCookie",
	    g_variant_new("(s)", token), G_VARIANT_TYPE("(o)"), 0, 3000, NULL, error);
	if (!resolved) goto failed;
	g_variant_get(resolved, "(&o)", &path);
	if (!g_str_has_prefix(path, ROOT "Session") || !path[strlen(ROOT "Session")] ||
	    strchr(path + strlen(ROOT), '/')) {
		g_set_error_literal(error, G_IO_ERROR, G_IO_ERROR_INVALID_DATA, "Invalid session object");
		goto failed;
	}
	dir = g_dbus_connection_call_sync(bus, owner, path, CK ".Session", "GetXDGRuntimeDir",
	    NULL, G_VARIANT_TYPE("(s)"), 0, 3000, NULL, error);
	if (!dir) goto failed;
	g_variant_get(dir, "(&s)", &directory);
	if (!g_path_is_absolute(directory) || lstat(directory, &st) || !S_ISDIR(st.st_mode) ||
	    st.st_uid != uid || (st.st_mode & 0777) != 0700) {
		g_set_error_literal(error, G_IO_ERROR, G_IO_ERROR_INVALID_DATA, "Invalid session runtime directory");
		goto failed;
	}
	*session = g_strdup(path + strlen(ROOT));
	*cookie = g_strdup(token);
	*runtime = g_strdup(directory);
	return g_steal_pointer(&bus);
failed:
	g_dbus_connection_close_sync(bus, NULL, NULL);
	return NULL;
}

/* Bind the action to the backend inventory consulted for the same request.
 * ConsoleKit sees the caller's own connection and applies its access rules. */
static gboolean
session_action(GDBusConnection *bus, const char *session, const char *seat,
    GCancellable *cancel, GError **error)
{
	g_autoptr(GVariant) owner_reply = NULL, seat_reply = NULL, name_reply = NULL, result = NULL;
	g_autofree char *path = NULL;
	const char *owner, *seat_path, *name;
	if (!G_IS_DBUS_CONNECTION(bus) || !session || !g_str_has_prefix(session, "Session") ||
	    !session[7] || strspn(session, "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_") != strlen(session)) {
		g_set_error_literal(error, G_IO_ERROR, G_IO_ERROR_INVALID_ARGUMENT, "Invalid session ID");
		return FALSE;
	}
	path = g_strconcat(ROOT, session, NULL);
	owner_reply = g_dbus_connection_call_sync(bus, "org.freedesktop.DBus", "/org/freedesktop/DBus",
	    "org.freedesktop.DBus", "GetNameOwner", g_variant_new("(s)", CK), G_VARIANT_TYPE("(s)"),
	    0, 3000, cancel, error);
	if (!owner_reply) return FALSE;
	g_variant_get(owner_reply, "(&s)", &owner);
	if (seat) {
		seat_reply = g_dbus_connection_call_sync(bus, owner, path, CK ".Session", "GetSeatId",
		    NULL, G_VARIANT_TYPE("(o)"), 0, 3000, cancel, error);
		if (!seat_reply) return FALSE;
		g_variant_get(seat_reply, "(&o)", &seat_path);
		name_reply = g_dbus_connection_call_sync(bus, owner, seat_path, CK ".Seat", "GetName",
		    NULL, G_VARIANT_TYPE("(s)"), 0, 3000, cancel, error);
		if (!name_reply) return FALSE;
		g_variant_get(name_reply, "(&s)", &name);
		if (strcmp(name, seat)) {
			g_set_error_literal(error, G_IO_ERROR, G_IO_ERROR_PERMISSION_DENIED, "Session is on another seat");
			return FALSE;
		}
	}
	result = g_dbus_connection_call_sync(bus, owner, path, CK ".Session",
	    seat ? "Activate" : "Unlock", NULL, G_VARIANT_TYPE_UNIT, 0, 5000, cancel, error);
	return result != NULL;
}

gboolean
bsd3_activate_display_session(GDBusConnection *bus, const char *seat,
    const char *session, GCancellable *cancel, GError **error)
{
	g_return_val_if_fail(seat && *seat, FALSE);
	return session_action(bus, session, seat, cancel, error);
}

gboolean
bsd3_unlock_display_session(GDBusConnection *bus, const char *session, GError **error)
{
	return session_action(bus, session, NULL, NULL, error);
}
