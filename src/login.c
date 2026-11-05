/* SPDX-License-Identifier: BSD-2-Clause
 * Copyright (c) 2026 Denis Lemire <denis@lemire.name>
 */
#include <sys/types.h>
#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <gio/gio.h>
#include "bsd3/sd-login.h"

#define CK "org.freedesktop.ConsoleKit"
#define MANAGER "/org/freedesktop/ConsoleKit/Manager"
#define SESSION_PREFIX "/org/freedesktop/ConsoleKit/Session"
#define SESSION_IFACE CK ".Session"

static int
error_code(GError *error)
{
	int code = EIO;
	char *remote = g_dbus_error_get_remote_error(error);
	if (g_error_matches(error, G_IO_ERROR, G_IO_ERROR_TIMED_OUT) ||
	    g_error_matches(error, G_DBUS_ERROR, G_DBUS_ERROR_NO_REPLY))
		code = ETIMEDOUT;
	else if (g_error_matches(error, G_DBUS_ERROR, G_DBUS_ERROR_ACCESS_DENIED))
		code = EACCES;
	else if (g_error_matches(error, G_DBUS_ERROR, G_DBUS_ERROR_SERVICE_UNKNOWN) ||
	    g_error_matches(error, G_DBUS_ERROR, G_DBUS_ERROR_NAME_HAS_NO_OWNER))
		code = ENOTCONN;
	else if (g_error_matches(error, G_DBUS_ERROR, G_DBUS_ERROR_UNKNOWN_OBJECT))
		code = ENXIO;
	else if (remote != NULL && (g_str_has_suffix(remote, ".NoSessions") ||
	    g_str_has_suffix(remote, ".NoSessionForPid") ||
	    g_str_has_suffix(remote, ".NoSessionForCookie")))
		code = ENODATA;
	g_free(remote);
	g_error_free(error);
	return (-code);
}

static int
call(const char *path, const char *iface, const char *method,
    GVariant *args, const char *type, GVariant **result)
{
	GError *error = NULL;
	GDBusConnection *bus = g_bus_get_sync(G_BUS_TYPE_SYSTEM, NULL, &error);
	if (bus == NULL) {
		/* Consume a floating parameter even if no connection is available. */
		if (args != NULL)
			g_variant_unref(g_variant_ref_sink(args));
		return (error_code(error));
	}
	*result = g_dbus_connection_call_sync(bus, CK, path, iface, method,
	    args, G_VARIANT_TYPE(type), G_DBUS_CALL_FLAGS_NO_AUTO_START,
	    3000, NULL, &error);
	g_object_unref(bus);
	return (*result == NULL ? error_code(error) : 0);
}

static int
copy_string(const char *value, char **ret)
{
	char *copy = strdup(value);
	if (copy == NULL)
		return (-ENOMEM);
	*ret = copy;
	return (0);
}

static int
id_from_path(const char *path, char **ret)
{
	const char *id;
	if (!g_str_has_prefix(path, SESSION_PREFIX))
		return (-EPROTO);
	id = path + strlen("/org/freedesktop/ConsoleKit/");
	if (strcmp(id, "Session") == 0 || strchr(id, '/') != NULL)
		return (-EPROTO);
	return (copy_string(id, ret));
}

static int
session_path(const char *session, char **ret)
{
	const unsigned char *p;
	char *current = NULL;
	int rc;
	if (session == NULL) {
		rc = sd_pid_get_session(0, &current);
		if (rc < 0)
			return (rc);
		session = current;
	}
	/* ConsoleKit's IDs are object-path components, never arbitrary paths. */
	if (strncmp(session, "Session", 7) != 0 || session[7] == '\0') {
		free(current);
		return (-EINVAL);
	}
	for (p = (const unsigned char *)session; *p != '\0'; p++) {
		if (!g_ascii_isalnum(*p) && *p != '_') {
			free(current);
			return (-EINVAL);
		}
	}
	*ret = g_strconcat("/org/freedesktop/ConsoleKit/", session, NULL);
	free(current);
	return (0);
}

int
sd_pid_get_session(pid_t pid, char **ret)
{
	GVariant *reply;
	const char *path;
	int rc;
	if (ret == NULL || pid < 0)
		return (-EINVAL);
	if (pid == 0)
		pid = getpid();
	rc = call(MANAGER, CK ".Manager", "GetSessionForUnixProcess",
	    g_variant_new("(u)", (guint32)pid), "(o)", &reply);
	if (rc < 0)
		return (rc);
	g_variant_get(reply, "(&o)", &path);
	rc = id_from_path(path, ret);
	g_variant_unref(reply);
	return (rc);
}

static int
session_value(const char *session, const char *method, const char *type,
    GVariant **reply)
{
	char *path;
	int rc = session_path(session, &path);
	if (rc < 0)
		return (rc);
	rc = call(path, SESSION_IFACE, method, NULL, type, reply);
	g_free(path);
	return (rc);
}

static int
session_string(const char *session, const char *method, char **ret)
{
	GVariant *reply;
	const char *value;
	int rc;
	if (ret == NULL)
		return (-EINVAL);
	rc = session_value(session, method, "(s)", &reply);
	if (rc < 0)
		return (rc);
	g_variant_get(reply, "(&s)", &value);
	rc = copy_string(value, ret);
	g_variant_unref(reply);
	return (rc);
}

int sd_session_get_type(const char *s, char **r)
{ return (session_string(s, "GetSessionType", r)); }
int sd_session_get_class(const char *s, char **r)
{ return (session_string(s, "GetSessionClass", r)); }
int sd_session_get_state(const char *s, char **r)
{ return (session_string(s, "GetSessionState", r)); }

int
sd_session_get_uid(const char *session, uid_t *ret)
{
	GVariant *reply;
	guint32 uid;
	int rc;
	if (ret == NULL)
		return (-EINVAL);
	rc = session_value(session, "GetUnixUser", "(u)", &reply);
	if (rc < 0)
		return (rc);
	g_variant_get(reply, "(u)", &uid);
	g_variant_unref(reply);
	*ret = (uid_t)uid;
	return (0);
}

int
sd_session_is_active(const char *session)
{
	GVariant *reply;
	gboolean active;
	int rc = session_value(session, "IsActive", "(b)", &reply);
	if (rc < 0)
		return (rc);
	g_variant_get(reply, "(b)", &active);
	g_variant_unref(reply);
	return (active ? 1 : 0);
}

int
sd_session_get_seat(const char *session, char **ret)
{
	GVariant *reply, *name_reply;
	const char *path, *name, *component;
	const unsigned char *p;
	int rc;
	if (ret == NULL)
		return (-EINVAL);
	rc = session_value(session, "GetSeatId", "(o)", &reply);
	if (rc < 0)
		return (rc);
	g_variant_get(reply, "(&o)", &path);
	if (strcmp(path, "/") == 0) {
		g_variant_unref(reply);
		return (-ENODATA);
	}
	/* ConsoleKit object labels changed from SeatN to seatN. The canonical
	 * name is a separate typed API and is what libinput's seat assignment uses. */
	if (!g_str_has_prefix(path, "/org/freedesktop/ConsoleKit/")) {
		g_variant_unref(reply);
		return (-EPROTO);
	}
	component = path + strlen("/org/freedesktop/ConsoleKit/");
	if (component[0] == '\0' || strchr(component, '/') != NULL) {
		g_variant_unref(reply);
		return (-EPROTO);
	}
	rc = call(path, CK ".Seat", "GetName", NULL, "(s)", &name_reply);
	g_variant_unref(reply);
	if (rc < 0)
		return (rc);
	g_variant_get(name_reply, "(&s)", &name);
	if (name[0] == '\0')
		rc = -ENODATA;
	else if (strlen(name) > 255)
		rc = -EPROTO;
	else {
		for (p = (const unsigned char *)name; *p != '\0'; p++) {
			if (!g_ascii_isalnum(*p) && *p != '_' && *p != '-')
				break;
		}
		rc = *p == '\0' ? copy_string(name, ret) : -EPROTO;
	}
	g_variant_unref(name_reply);
	return (rc);
}

static void
free_vector(char **items)
{
	size_t i;
	if (items == NULL)
		return;
	for (i = 0; items[i] != NULL; i++)
		free(items[i]);
	free(items);
}

static int
sessions(uid_t uid, int filter_uid, int active_only, char ***ret)
{
	GVariant *reply;
	GVariantIter *iter;
	const char *path;
	char **items;
	size_t capacity, n = 0;
	int rc;
	rc = call(MANAGER, CK ".Manager", filter_uid ?
	    "GetSessionsForUnixUser" : "GetSessions", filter_uid ?
	    g_variant_new("(u)", (guint32)uid) : NULL, "(ao)", &reply);
	if (rc == -ENODATA) {
		if (ret != NULL) {
			items = calloc(1, sizeof(*items));
			if (items == NULL)
				return (-ENOMEM);
			*ret = items;
		}
		return (0);
	}
	if (rc < 0)
		return (rc);
	g_variant_get(reply, "(ao)", &iter);
	capacity = g_variant_iter_n_children(iter);
	if (capacity > 65536) {
		g_variant_iter_free(iter);
		g_variant_unref(reply);
		return (-EOVERFLOW);
	}
	items = calloc(capacity + 1, sizeof(*items));
	if (items == NULL) {
		rc = -ENOMEM;
		goto out;
	}
	while (g_variant_iter_loop(iter, "&o", &path)) {
		char *id = NULL;
		rc = id_from_path(path, &id);
		if (rc < 0)
			goto out;
		if (filter_uid && active_only >= 0) {
			char *state = NULL;
			rc = sd_session_get_state(id, &state);
			if (rc >= 0)
				rc = strcmp(state, "active") == 0 ||
				    (active_only == 0 && strcmp(state, "online") == 0);
			free(state);
			if (rc <= 0) {
				free(id);
				if (rc < 0)
					goto out;
				continue;
			}
		}
		items[n++] = id;
	}
	rc = (int)n;
	if (ret != NULL) {
		*ret = items;
		items = NULL;
	}
out:
	free_vector(items);
	g_variant_iter_free(iter);
	g_variant_unref(reply);
	return (rc);
}

int sd_get_sessions(char ***ret)
{ return (sessions(0, 0, 0, ret)); }
int sd_uid_get_sessions(uid_t uid, int active, char ***ret)
{
	if (uid == (uid_t)-1)
		return (-EINVAL);
	return (sessions(uid, 1, active, ret));
}

int
sd_uid_get_display(uid_t uid, char **ret)
{
	char **list = NULL, *chosen = NULL;
	int n, i, best = 0, rc = 0;
	gint64 oldest = G_MAXINT64;
	if (ret == NULL)
		return (-EINVAL);
	n = sd_uid_get_sessions(uid, 0, &list);
	if (n < 0)
		return (n);
	for (i = 0; i < n; i++) {
		char *type = NULL, *class_name = NULL, *state = NULL;
		char *created = NULL;
		GDateTime *time = NULL;
		gint64 timestamp;
		int score;
		rc = sd_session_get_class(list[i], &class_name);
		if (rc < 0)
			goto candidate_done;
		rc = sd_session_get_type(list[i], &type);
		if (rc < 0)
			goto candidate_done;
		rc = sd_session_get_state(list[i], &state);
		if (rc < 0)
			goto candidate_done;
		if (strcmp(class_name, "user") != 0 ||
		    (strcmp(state, "active") != 0 && strcmp(state, "online") != 0))
			goto candidate_done;
		rc = session_string(list[i], "GetCreationTime", &created);
		if (rc < 0)
			goto candidate_done;
		time = g_date_time_new_from_iso8601(created, NULL);
		if (time == NULL) {
			rc = -EPROTO;
			goto candidate_done;
		}
		timestamp = g_date_time_to_unix(time);
		score = 1;
		if (strcmp(type, "wayland") == 0 || strcmp(type, "x11") == 0 ||
		    strcmp(type, "mir") == 0)
			score += 4;
		if (score > best || (score == best && timestamp < oldest)) {
			oldest = timestamp;
			best = score;
			chosen = list[i];
		}
candidate_done:
		if (time != NULL)
			g_date_time_unref(time);
		free(created);
		free(type);
		free(class_name);
		free(state);
		if (rc < 0)
			break;
	}
	if (rc >= 0)
		rc = chosen == NULL ? -ENODATA : copy_string(chosen, ret);
	free_vector(list);
	return (rc);
}
