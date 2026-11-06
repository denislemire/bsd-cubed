/* SPDX-License-Identifier: BSD-2-Clause */
#include <string.h>
#include "seats.h"
#include "control.h"
#define CK "org.freedesktop.ConsoleKit"
#define ROOT "/org/freedesktop/ConsoleKit"
static void destroy(gpointer data) {
	struct bsd3_seat *seat = data;
	g_free(seat->name); g_free(seat->component); g_free(seat->path); g_free(seat->owner); g_free(seat);
}
static GVariant *request(GDBusConnection *bus, const char *owner, const char *path,
    const char *iface, const char *method, GVariant *args, const char *type, GError **error) {
	return (g_dbus_connection_call_sync(bus, owner, path, iface, method, args,
	    G_VARIANT_TYPE(type), 0, 3000, NULL, error));
}
GPtrArray *
bsd3_seats_list(GDBusConnection *bus, GError **error)
{
	GVariant *reply, *paths;
	char *owner;
	GPtrArray *seats = g_ptr_array_new_with_free_func(destroy);
	GHashTable *seen = g_hash_table_new(g_str_hash, g_str_equal);
	reply = request(bus, "org.freedesktop.DBus", "/org/freedesktop/DBus",
	    "org.freedesktop.DBus", "NameHasOwner", g_variant_new("(s)", CK), "(b)", error);
	if (reply == NULL) goto failed;
	gboolean running;
	g_variant_get(reply, "(b)", &running); g_variant_unref(reply);
	if (!running) {
		reply = request(bus, "org.freedesktop.DBus", "/org/freedesktop/DBus",
		    "org.freedesktop.DBus", "StartServiceByName", g_variant_new("(su)", CK, 0U), "(u)", error);
		if (reply == NULL) goto failed;
		g_variant_unref(reply);
	}
	reply = request(bus, "org.freedesktop.DBus", "/org/freedesktop/DBus",
	    "org.freedesktop.DBus", "GetNameOwner", g_variant_new("(s)", CK), "(s)", error);
	if (reply == NULL) goto failed;
	g_variant_get(reply, "(s)", &owner); g_variant_unref(reply);
	reply = request(bus, owner, ROOT "/Manager", CK ".Manager", "GetSeats", NULL, "(ao)", error);
	if (reply == NULL) { g_free(owner); goto failed; }
	paths = g_variant_get_child_value(reply, 0); g_variant_unref(reply);
	if (g_variant_n_children(paths) > 256) {
		g_set_error(error, G_DBUS_ERROR, G_DBUS_ERROR_LIMITS_EXCEEDED, "Too many backend seats");
		g_free(owner); g_variant_unref(paths); goto failed;
	}
	for (gsize i = 0; i < g_variant_n_children(paths); i++) {
		const char *path;
		g_variant_get_child(paths, i, "&o", &path);
		if (g_hash_table_contains(seen, path)) continue;
		if (!g_str_has_prefix(path, ROOT "/") || path[strlen(ROOT "/")] == '\0' ||
		    strchr(path + strlen(ROOT "/"), '/') != NULL) {
			g_set_error(error, G_DBUS_ERROR, G_DBUS_ERROR_FAILED, "Invalid backend seat path");
			g_free(owner); g_variant_unref(paths); goto failed;
		}
		reply = request(bus, owner, path, CK ".Seat", "GetName", NULL, "(s)", error);
		if (reply == NULL) { g_free(owner); g_variant_unref(paths); goto failed; }
		struct bsd3_seat *seat = g_new0(struct bsd3_seat, 1);
		g_variant_get(reply, "(s)", &seat->name); g_variant_unref(reply);
		const char *p;
		for (p = seat->name; *p; p++)
			if (!g_ascii_isalnum(*p) && *p != '_' && *p != '-') break;
		gboolean collision = FALSE;
		for (guint j = 0; j < seats->len; j++)
			if (strcmp(((struct bsd3_seat *)g_ptr_array_index(seats, j))->name, seat->name) == 0)
				collision = TRUE;
		if (*seat->name == '\0' || strlen(seat->name) > 255 || *p || collision) {
			destroy(seat);
			g_set_error(error, G_DBUS_ERROR, G_DBUS_ERROR_FAILED, "Invalid or ambiguous backend seat name");
			g_free(owner); g_variant_unref(paths); goto failed;
		}
		seat->component = bsd3_seat_component(seat->name);
		seat->path = g_strdup(path); seat->owner = g_strdup(owner);
		g_ptr_array_add(seats, seat); g_hash_table_add(seen, seat->path);
	}
	g_free(owner); g_variant_unref(paths); g_hash_table_unref(seen);
	return (seats);
failed:
	g_hash_table_unref(seen); g_ptr_array_unref(seats);
	return (NULL);
}
/* login1 uses the canonical seat name plus its exported bridge object path,
 * not ConsoleKit's differently named backend object. No default seat guess. */
GVariant *
bsd3_session_seat(GDBusConnection *bus, const char *id, GError **error)
{
	GVariant *reply, *result = NULL;
	GPtrArray *seats;
	char *owner, *seat_path, *session_path;
	reply = request(bus, "org.freedesktop.DBus", "/org/freedesktop/DBus",
	    "org.freedesktop.DBus", "GetNameOwner", g_variant_new("(s)", CK), "(s)", error);
	if (reply == NULL) return (NULL);
	g_variant_get(reply, "(s)", &owner); g_variant_unref(reply);
	session_path = g_strconcat(ROOT "/", id, NULL);
	reply = request(bus, owner, session_path, CK ".Session", "GetSeatId", NULL, "(o)", error);
	g_free(session_path);
	if (reply == NULL) { g_free(owner); return (NULL); }
	g_variant_get(reply, "(o)", &seat_path); g_variant_unref(reply);
	if (strcmp(seat_path, "/") == 0) {
		g_free(seat_path); g_free(owner);
		return (g_variant_new("(so)", "", "/"));
	}
	seats = bsd3_seats_list(bus, error);
	if (seats == NULL) { g_free(seat_path); g_free(owner); return (NULL); }
	for (guint i = 0; i < seats->len; i++) {
		struct bsd3_seat *seat = g_ptr_array_index(seats, i);
		if (strcmp(seat_path, seat->path) != 0 || strcmp(owner, seat->owner) != 0)
			continue;
		char *path = g_strconcat("/org/freedesktop/login1/seat/", seat->component, NULL);
		result = g_variant_new("(so)", seat->name, path);
		g_free(path);
		break;
	}
	g_ptr_array_unref(seats); g_free(seat_path); g_free(owner);
	if (result == NULL)
		g_set_error(error, G_DBUS_ERROR, G_DBUS_ERROR_FAILED,
		    "Session seat is absent from its backend owner's valid inventory");
	return (result);
}

GVariant *
bsd3_seat_property(GDBusConnection *bus, const char *component, const char *name, GError **error)
{
	GPtrArray *seats = bsd3_seats_list(bus, error);
	GVariant *result = NULL;
	if (seats == NULL) return (NULL);
	for (guint i = 0; i < seats->len; i++) {
		struct bsd3_seat *seat = g_ptr_array_index(seats, i);
		if (strcmp(component, seat->component) != 0) continue;
		if (strcmp(name, "Id") == 0) result = g_variant_new_string(seat->name);
		else if (strcmp(name, "CanTTY") == 0) {
			GVariant *reply = request(bus, seat->owner, seat->path, CK ".Seat", "CanActivateSessions", NULL, "(b)", error);
			if (reply != NULL) { result = g_variant_get_child_value(reply, 0); g_variant_unref(reply); }
		} else if (strcmp(name, "CanGraphical") == 0) {
			GVariant *reply = request(bus, seat->owner, seat->path, "org.freedesktop.DBus.Properties", "Get",
			    g_variant_new("(ss)", CK ".Seat", "CanGraphical"), "(v)", error);
			if (reply != NULL) {
				GVariant *value; g_variant_get(reply, "(v)", &value); g_variant_unref(reply);
				if (g_variant_is_of_type(value, G_VARIANT_TYPE_BOOLEAN)) result = value;
				else { g_variant_unref(value); g_set_error(error, G_DBUS_ERROR, G_DBUS_ERROR_FAILED, "Invalid capability type"); }
			}
		} else g_set_error(error, G_DBUS_ERROR, G_DBUS_ERROR_UNKNOWN_PROPERTY, "Unsupported seat property");
		g_ptr_array_unref(seats); return (result);
	}
	g_ptr_array_unref(seats);
	g_set_error(error, G_DBUS_ERROR, G_DBUS_ERROR_UNKNOWN_OBJECT, "Unknown seat");
	return (NULL);
}
