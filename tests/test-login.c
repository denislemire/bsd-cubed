/* SPDX-License-Identifier: BSD-2-Clause */
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <gio/gio.h>
#include "bsd3/sd-login.h"
#define CK "org.freedesktop.ConsoleKit"
#define MANAGER "/org/freedesktop/ConsoleKit/Manager"
static GDBusConnection *bus;

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
	g_assert_cmpstr(seat, ==, "Seat1"); free(seat);
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
	    "TakeControl", g_variant_new("(b)", FALSE), NULL, 0, 3000, NULL, &error);
	g_assert_null(r);
	g_assert_error(error, G_DBUS_ERROR, G_DBUS_ERROR_UNKNOWN_METHOD);
	g_clear_error(&error);
}

int
main(int argc, char **argv)
{
	GError *error = NULL;
	int rc;
	g_test_init(&argc, &argv, NULL);
	bus = g_bus_get_sync(G_BUS_TYPE_SYSTEM, NULL, &error);
	g_assert_no_error(error);
	g_test_add_func("/bsd3/discovery", test_discovery);
	g_test_add_func("/bsd3/enumeration", test_enumeration);
	g_test_add_func("/bsd3/display", test_display);
	g_test_add_func("/bsd3/errors", test_errors);
	g_test_add_func("/bsd3/daemon", test_daemon);
	rc = g_test_run();
	g_object_unref(bus);
	return (rc);
}
