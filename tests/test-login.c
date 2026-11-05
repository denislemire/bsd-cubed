/* SPDX-License-Identifier: BSD-2-Clause */
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <gio/gio.h>
#include "bsd3/sd-login.h"
#define CK "org.freedesktop.ConsoleKit"
#define MANAGER "/org/freedesktop/ConsoleKit/Manager"
static GDBusConnection *bus;
static gboolean peer_pid_available = TRUE;

static GVariant *
call(const char *dest, const char *path, const char *iface,
    const char *method, GVariant *args, const char *type)
{
	GError *error = NULL;
	GVariant *r = g_dbus_connection_call_sync(bus, dest, path, iface, method,
	    args, G_VARIANT_TYPE(type), 0, 3000, NULL, &error);
	g_assert_no_error(error);
	g_assert_nonnull(r);
	return (r);
}

static void
mode(const char *value)
{
	GVariant *r = call(CK, MANAGER, "org.bsd3.Test", "SetMode",
	    g_variant_new("(s)", value), "()");
	g_variant_unref(r);
}

static void
test_discovery(void)
{
	char *id = NULL, *type = NULL, *seat = NULL;
	uid_t uid = 0;
	GVariant *r;
	guint32 pid;
	g_assert_cmpint(sd_pid_get_session(0, &id), ==, 0);
	g_assert_cmpstr(id, ==, "Session1");
	free(id);
	r = call(CK, MANAGER, "org.bsd3.Test", "GetLastPid", NULL, "(u)");
	g_variant_get(r, "(u)", &pid);
	g_assert_cmpuint(pid, ==, (guint32)getpid());
	g_variant_unref(r);
	g_assert_cmpint(sd_session_get_type("Session2", &type), ==, 0);
	g_assert_cmpstr(type, ==, "wayland"); free(type);
	g_assert_cmpint(sd_session_get_uid("Session1", &uid), ==, 0);
	g_assert_cmpuint(uid, ==, 1000);
	g_assert_cmpint(sd_session_get_seat("Session2", &seat), ==, 0);
	g_assert_cmpstr(seat, ==, "seat0"); free(seat);
	g_assert_cmpint(sd_session_is_active("Session2"), ==, 0);
	g_assert_cmpint(sd_session_is_active("Session3"), ==, 1);
}

static void
test_enumeration(void)
{
	char **list = NULL;
	g_assert_cmpint(sd_get_sessions(&list), ==, 5);
	g_assert_cmpuint(g_strv_length(list), ==, 5); g_strfreev(list);
	g_assert_cmpint(sd_uid_get_sessions(1000, 1, &list), ==, 3);
	g_assert_cmpuint(g_strv_length(list), ==, 3); g_strfreev(list);
	g_assert_cmpint(sd_uid_get_sessions(1000, 0, &list), ==, 4);
	g_assert_cmpuint(g_strv_length(list), ==, 4); g_strfreev(list);
	g_assert_cmpint(sd_uid_get_sessions(1000, -1, &list), ==, 5);
	g_assert_cmpuint(g_strv_length(list), ==, 5); g_strfreev(list);
	g_assert_cmpint(sd_uid_get_sessions(1000, 1, NULL), ==, 3);
	g_assert_cmpint(sd_uid_get_sessions(9999, 0, &list), ==, 0);
	g_assert_nonnull(list); g_assert_null(list[0]); g_strfreev(list);
}

static void
test_display(void)
{
	char *id = NULL;
	g_assert_cmpint(sd_uid_get_display(1000, &id), ==, 0);
	/* Oldest graphical user session wins over newer active session/TTY/greeter. */
	g_assert_cmpstr(id, ==, "Session2"); free(id);
	id = NULL;
	g_assert_cmpint(sd_uid_get_display(9999, &id), ==, -ENODATA);
	g_assert_null(id);
	mode("bad-time");
	g_assert_cmpint(sd_uid_get_display(1000, &id), ==, -EPROTO);
	g_assert_null(id);
	mode("normal");
}

static void
test_errors(void)
{
	char *out = NULL;
	char **list = NULL;
	g_assert_cmpint(sd_pid_get_session(-1, &out), ==, -EINVAL);
	g_assert_cmpint(sd_pid_get_session(0, NULL), ==, -EINVAL);
	g_assert_cmpint(sd_session_get_type("../../root", &out), ==, -EINVAL);
	g_assert_cmpint(sd_session_get_type("Session1/Other", &out), ==, -EINVAL);
	g_assert_cmpint(sd_session_get_type("Session", &out), ==, -EINVAL);
	g_assert_cmpint(sd_uid_get_sessions((uid_t)-1, 0, NULL), ==, -EINVAL);
	g_assert_cmpint(sd_pid_get_session(99999999, &out), ==, -ENODATA);
	g_assert_null(out);
	mode("denied");
	g_assert_cmpint(sd_session_get_type("Session1", &out), ==, -EACCES);
	g_assert_null(out);
	mode("bad-path");
	g_assert_cmpint(sd_get_sessions(&list), ==, -EPROTO);
	g_assert_null(list);
	mode("empty");
	g_assert_cmpint(sd_get_sessions(&list), ==, 0);
	g_assert_null(list[0]); g_strfreev(list);
	mode("normal");
}

static void
test_daemon(void)
{
	if (!peer_pid_available) {
		g_test_skip("Host D-Bus cannot report peer PIDs; requires native FreeBSD CI");
		return;
	}
	const char *path;
	GVariant *r, *value;
	GError *error = NULL;
	guint32 pid;
	r = call("org.freedesktop.login1", "/org/freedesktop/login1",
	    "org.freedesktop.login1.Manager", "GetSessionByPID",
	    g_variant_new("(u)", 0U), "(o)");
	g_variant_get(r, "(&o)", &path);
	g_assert_cmpstr(path, ==, "/org/freedesktop/login1/session/Session1");
	g_variant_unref(r);
	r = call(CK, MANAGER, "org.bsd3.Test", "GetLastPid", NULL, "(u)");
	g_variant_get(r, "(u)", &pid);
	g_assert_cmpuint(pid, ==, (guint32)getpid()); g_variant_unref(r);
	r = call("org.freedesktop.login1", "/org/freedesktop/login1/session/Session2",
	    "org.freedesktop.DBus.Properties", "Get",
	    g_variant_new("(ss)", "org.freedesktop.login1.Session", "Type"), "(v)");
	g_variant_get(r, "(v)", &value);
	g_assert_cmpstr(g_variant_get_string(value, NULL), ==, "wayland");
	g_variant_unref(value); g_variant_unref(r);
	r = call("org.freedesktop.login1", "/org/freedesktop/login1",
	    "org.freedesktop.login1.Manager", "ListSessions", NULL, "(a(susso))");
	value = g_variant_get_child_value(r, 0);
	g_assert_cmpuint(g_variant_n_children(value), ==, 5);
	g_variant_unref(value); g_variant_unref(r);
	r = g_dbus_connection_call_sync(bus, "org.freedesktop.login1",
	    "/org/freedesktop/login1/session/Session1", "org.freedesktop.login1.Session",
	    "TakeDevice", g_variant_new("(uu)", 1U, 2U), NULL, 0, 3000, NULL, &error);
	g_assert_null(r);
	g_assert_error(error, G_DBUS_ERROR, G_DBUS_ERROR_UNKNOWN_METHOD);
	g_clear_error(&error);
}

static GDBusConnection *
client_connection(void)
{
	GError *error = NULL;
	char *address = g_dbus_address_get_for_bus_sync(G_BUS_TYPE_SYSTEM, NULL, &error);
	GDBusConnection *client;
	g_assert_no_error(error);
	client = g_dbus_connection_new_for_address_sync(address,
	    G_DBUS_CONNECTION_FLAGS_AUTHENTICATION_CLIENT |
	    G_DBUS_CONNECTION_FLAGS_MESSAGE_BUS_CONNECTION, NULL, NULL, &error);
	g_free(address);
	g_assert_no_error(error);
	g_dbus_connection_set_exit_on_close(client, FALSE);
	return (client);
}

static void
control(GDBusConnection *client, const char *id, const char *method,
    gboolean force, const char *expected_error)
{
	GError *error = NULL;
	char *path = g_strconcat("/org/freedesktop/login1/session/", id, NULL);
	GVariant *r = g_dbus_connection_call_sync(client, "org.freedesktop.login1",
	    path, "org.freedesktop.login1.Session", method,
	    strcmp(method, "TakeControl") == 0 ? g_variant_new("(b)", force) : NULL,
	    G_VARIANT_TYPE("()"), 0, 3000, NULL, &error);
	g_free(path);
	if (expected_error == NULL) {
		g_assert_no_error(error);
		g_assert_nonnull(r);
		g_variant_unref(r);
	} else {
		char *remote;
		g_assert_null(r);
		g_assert_nonnull(error);
		remote = g_dbus_error_get_remote_error(error);
		g_assert_cmpstr(remote, ==, expected_error);
		g_free(remote);
		g_error_free(error);
	}
}

static void
wait_no_controllers(void)
{
	unsigned int i;
	guint32 takes, releases, active = 1;
	for (i = 0; i < 100; i++) {
		GVariant *r = call(CK, MANAGER, "org.bsd3.Test", "GetCounters", NULL, "(uuu)");
		g_variant_get(r, "(uuu)", &takes, &releases, &active);
		g_variant_unref(r);
		if (active == 0)
			return;
		g_usleep(20000);
	}
	g_assert_cmpuint(active, ==, 0);
}

static void
test_control_ownership(void)
{
	if (!peer_pid_available) {
		g_test_skip("Host D-Bus cannot report peer PIDs; requires native FreeBSD CI");
		return;
	}
	GDBusConnection *first = client_connection(), *second = client_connection();
	GVariant *r;
	guint32 takes, releases, active;
	mode("control");
	control(first, "Session6", "TakeControl", TRUE,
	    "org.freedesktop.DBus.Error.NotSupported");
	control(first, "Session7", "TakeControl", FALSE,
	    "org.freedesktop.DBus.Error.AccessDenied");
	mode("uid-mismatch");
	control(first, "Session8", "TakeControl", FALSE,
	    "org.freedesktop.DBus.Error.AccessDenied");
	mode("control");
	control(first, "Session6", "TakeControl", FALSE, NULL);
	control(first, "Session6", "TakeControl", FALSE, NULL);
	r = call(CK, MANAGER, "org.bsd3.Test", "GetCounters", NULL, "(uuu)");
	g_variant_get(r, "(uuu)", &takes, &releases, &active);
	g_assert_cmpuint(takes, ==, 1);
	g_assert_cmpuint(active, ==, 1);
	g_variant_unref(r);
	control(second, "Session6", "TakeControl", FALSE,
	    "org.freedesktop.login1.SessionBusy");
	control(second, "Session6", "ReleaseControl", FALSE,
	    "org.freedesktop.DBus.Error.AccessDenied");
	control(first, "Session7", "ReleaseControl", FALSE,
	    "org.freedesktop.DBus.Error.AccessDenied");
	control(first, "Session6", "ReleaseControl", FALSE, NULL);
	control(first, "Session6", "ReleaseControl", FALSE,
	    "org.freedesktop.DBus.Error.AccessDenied");
	control(second, "Session6", "TakeControl", FALSE, NULL);
	g_dbus_connection_close_sync(second, NULL, NULL);
	g_object_unref(second);
	wait_no_controllers();
	control(first, "Session6", "TakeControl", FALSE, NULL);
	control(first, "Session6", "ReleaseControl", FALSE, NULL);
	g_dbus_connection_close_sync(first, NULL, NULL);
	g_object_unref(first);
	mode("normal");
}

static void
test_control_failures(void)
{
	if (!peer_pid_available) {
		g_test_skip("Host D-Bus cannot report peer PIDs; requires native FreeBSD CI");
		return;
	}
	GDBusConnection *client = client_connection();
	mode("unsupported");
	control(client, "Session6", "TakeControl", FALSE,
	    "org.freedesktop.DBus.Error.NotSupported");
	mode("take-fail");
	control(client, "Session6", "TakeControl", FALSE,
	    "org.freedesktop.DBus.Error.Failed");
	wait_no_controllers();
	mode("control");
	control(client, "Session6", "TakeControl", FALSE, NULL);
	mode("release-fail");
	control(client, "Session6", "ReleaseControl", FALSE,
	    "org.freedesktop.DBus.Error.Failed");
	wait_no_controllers();
	mode("control");
	control(client, "Session6", "TakeControl", FALSE, NULL);
	mode("restart");
	control(client, "Session6", "ReleaseControl", FALSE,
	    "org.freedesktop.DBus.Error.AccessDenied");
	wait_no_controllers();
	mode("control");
	control(client, "Session6", "TakeControl", FALSE, NULL);
	control(client, "Session6", "ReleaseControl", FALSE, NULL);
	g_dbus_connection_close_sync(client, NULL, NULL);
	g_object_unref(client);
	mode("normal");
}

static void
test_missing_credentials(void)
{
	GDBusConnection *client;
	if (peer_pid_available) {
		g_test_skip("Host provides peer process IDs");
		return;
	}
	client = client_connection();
	mode("control");
	control(client, "Session6", "TakeControl", FALSE,
	    "org.freedesktop.DBus.Error.AccessDenied");
	wait_no_controllers();
	g_dbus_connection_close_sync(client, NULL, NULL);
	g_object_unref(client);
	mode("normal");
}

static void
test_seat_names(void)
{
	char *seat = NULL;
	mode("normal");
	g_assert_cmpint(sd_session_get_seat("Session1", &seat), ==, 0);
	g_assert_cmpstr(seat, ==, "seat0"); free(seat); seat = NULL;
	mode("legacy-seat");
	g_assert_cmpint(sd_session_get_seat("Session1", &seat), ==, 0);
	g_assert_cmpstr(seat, ==, "seat0"); free(seat); seat = NULL;
	mode("custom-seat-name");
	g_assert_cmpint(sd_session_get_seat("Session1", &seat), ==, 0);
	g_assert_cmpstr(seat, ==, "seat-lab"); free(seat); seat = NULL;
	mode("no-seat");
	g_assert_cmpint(sd_session_get_seat("Session1", &seat), ==, -ENODATA);
	g_assert_null(seat);
	mode("empty-seat-name");
	g_assert_cmpint(sd_session_get_seat("Session1", &seat), ==, -ENODATA);
	g_assert_null(seat);
	mode("bad-seat-path");
	g_assert_cmpint(sd_session_get_seat("Session1", &seat), ==, -EPROTO);
	g_assert_null(seat);
	mode("unsafe-seat-name");
	g_assert_cmpint(sd_session_get_seat("Session1", &seat), ==, -EPROTO);
	g_assert_null(seat);
	mode("seat-name-error");
	g_assert_cmpint(sd_session_get_seat("Session1", &seat), ==, -EACCES);
	g_assert_null(seat);
	mode("normal");
}

int
main(int argc, char **argv)
{
	GError *error = NULL;
	int rc;
	g_test_init(&argc, &argv, NULL);
	bus = g_bus_get_sync(G_BUS_TYPE_SYSTEM, NULL, &error);
	g_assert_no_error(error);
#ifdef __APPLE__
	/* macOS dbus-daemon lacks peer PID credentials. Skip only this exact
	 * platform limitation; the daemon itself must continue to fail closed. */
	{
		GVariant *r = g_dbus_connection_call_sync(bus, "org.freedesktop.DBus",
		    "/org/freedesktop/DBus", "org.freedesktop.DBus",
		    "GetConnectionUnixProcessID", g_variant_new("(s)",
		    g_dbus_connection_get_unique_name(bus)), G_VARIANT_TYPE("(u)"),
		    0, 3000, NULL, &error);
		if (r == NULL) {
			g_assert_error(error, G_DBUS_ERROR, G_DBUS_ERROR_UNIX_PROCESS_ID_UNKNOWN);
			peer_pid_available = FALSE;
			g_clear_error(&error);
		} else
			g_variant_unref(r);
	}
#endif
	g_test_add_func("/bsd3/discovery", test_discovery);
	g_test_add_func("/bsd3/seat-names", test_seat_names);
	g_test_add_func("/bsd3/enumeration", test_enumeration);
	g_test_add_func("/bsd3/display", test_display);
	g_test_add_func("/bsd3/errors", test_errors);
	g_test_add_func("/bsd3/daemon", test_daemon);
	g_test_add_func("/bsd3/controller-ownership", test_control_ownership);
	g_test_add_func("/bsd3/controller-failures", test_control_failures);
	g_test_add_func("/bsd3/controller-missing-credentials", test_missing_credentials);
	rc = g_test_run();
	g_object_unref(bus);
	return (rc);
}
