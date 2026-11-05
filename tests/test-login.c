/* SPDX-License-Identifier: BSD-2-Clause */
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <gio/gio.h>
#include <gio/gunixfdlist.h>
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
	    "SetBrightness", g_variant_new("(ssu)", "backlight", "test", 1U), NULL, 0, 3000, NULL, &error);
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

static GVariant *
device_request(GDBusConnection *client, const char *method, guint32 major,
    guint32 minor, GUnixFDList **fds, const char *expected_error)
{
	GError *error = NULL;
	GVariant *reply = g_dbus_connection_call_with_unix_fd_list_sync(client,
	    "org.freedesktop.login1", "/org/freedesktop/login1/session/Session6",
	    "org.freedesktop.login1.Session", method, g_variant_new("(uu)", major, minor),
	    G_VARIANT_TYPE(strcmp(method, "TakeDevice") == 0 ? "(hb)" : "()"),
	    0, 3000, NULL, fds, NULL, &error);
	if (expected_error != NULL) {
		char *remote;
		g_assert_null(reply);
		g_assert_nonnull(error);
		remote = g_dbus_error_get_remote_error(error);
		g_assert_cmpstr(remote, ==, expected_error);
		g_free(remote);
		g_error_free(error);
		return (NULL);
	}
	g_assert_no_error(error);
	g_assert_nonnull(reply);
	return (reply);
}

static void
consume_descriptor(GVariant *reply, GUnixFDList *fds, char expected, gboolean paused)
{
	GError *error = NULL;
	gint handle, fd;
	gboolean actual_paused;
	char marker = 0;
	g_assert_nonnull(fds);
	g_assert_cmpint(g_unix_fd_list_get_length(fds), ==, 1);
	g_variant_get(reply, "(hb)", &handle, &actual_paused);
	g_assert_cmpint(actual_paused, ==, paused);
	g_assert_cmpint(handle, ==, 0);
	fd = g_unix_fd_list_get(fds, handle, &error);
	g_assert_no_error(error);
	g_assert_cmpint(read(fd, &marker, 1), ==, 1);
	g_assert_cmpint(marker, ==, expected);
	close(fd);
	g_variant_unref(reply);
	g_object_unref(fds);
}

static void
take_pipe(GDBusConnection *client, guint32 minor, gboolean paused)
{
	GUnixFDList *fds = NULL;
	GVariant *reply = device_request(client, "TakeDevice", 0, minor, &fds, NULL);
	consume_descriptor(reply, fds, 'T', paused);
}

static void
plain_device(GDBusConnection *client, const char *method, guint32 minor, const char *error)
{
	GUnixFDList *fds = NULL;
	GVariant *reply = device_request(client, method, 0, minor, &fds, error);
	g_assert_null(fds);
	if (reply != NULL)
		g_variant_unref(reply);
}

struct signal_capture {
	GAsyncQueue *queue;
	char *owner;
	GDBusConnection *client;
	guint filter;
};

static GDBusMessage *
capture_signal(GDBusConnection *bus_connection, GDBusMessage *message,
    gboolean incoming, gpointer data)
{
	struct signal_capture *capture = data;
	(void)bus_connection;
	if (incoming && g_dbus_message_get_message_type(message) == G_DBUS_MESSAGE_TYPE_SIGNAL &&
	    g_strcmp0(g_dbus_message_get_sender(message), capture->owner) == 0 &&
	    (g_strcmp0(g_dbus_message_get_interface(message), "org.freedesktop.login1.Session") == 0 ||
	    g_strcmp0(g_dbus_message_get_interface(message), "org.freedesktop.DBus.Properties") == 0))
		g_async_queue_push(capture->queue, g_object_ref(message));
	return (message);
}

static void
free_capture(gpointer data)
{
	struct signal_capture *capture = data;
	g_async_queue_unref(capture->queue);
	g_free(capture->owner);
	g_free(capture);
}

static struct signal_capture *
start_capture(GDBusConnection *client)
{
	struct signal_capture *capture = g_new0(struct signal_capture, 1);
	GVariant *reply = call("org.freedesktop.DBus", "/org/freedesktop/DBus",
	    "org.freedesktop.DBus", "GetNameOwner",
	    g_variant_new("(s)", "org.freedesktop.login1"), "(s)");
	g_variant_get(reply, "(s)", &capture->owner);
	g_variant_unref(reply);
	capture->queue = g_async_queue_new_full(g_object_unref);
	capture->client = client;
	capture->filter = g_dbus_connection_add_filter(client, capture_signal, capture, free_capture);
	return (capture);
}

static void
expect_pause(struct signal_capture *capture, const char *kind)
{
	GDBusMessage *message = g_async_queue_timeout_pop(capture->queue, 3000000);
	guint32 major, minor;
	const char *actual;
	g_assert_nonnull(message);
	g_assert_cmpstr(g_dbus_message_get_member(message), ==, "PauseDevice");
	g_assert_null(g_dbus_message_get_unix_fd_list(message));
	g_variant_get(g_dbus_message_get_body(message), "(uu&s)", &major, &minor, &actual);
	g_assert_cmpuint(major, ==, 0);
	g_assert_cmpuint(minor, ==, 1);
	g_assert_cmpstr(actual, ==, kind);
	g_object_unref(message);
}

static void
expect_resume(struct signal_capture *capture)
{
	GDBusMessage *message = g_async_queue_timeout_pop(capture->queue, 3000000);
	GUnixFDList *fds;
	GError *error = NULL;
	guint32 major, minor;
	gint handle, fd;
	char marker = 0;
	g_assert_nonnull(message);
	g_assert_cmpstr(g_dbus_message_get_member(message), ==, "ResumeDevice");
	g_variant_get(g_dbus_message_get_body(message), "(uuh)", &major, &minor, &handle);
	g_assert_cmpuint(major, ==, 0); g_assert_cmpuint(minor, ==, 1);
	fds = g_dbus_message_get_unix_fd_list(message);
	g_assert_nonnull(fds); g_assert_cmpint(g_unix_fd_list_get_length(fds), ==, 1);
	g_assert_cmpint(handle, ==, 0);
	fd = g_unix_fd_list_get(fds, handle, &error);
	g_assert_no_error(error);
	g_assert_cmpint(read(fd, &marker, 1), ==, 1);
	g_assert_cmpint(marker, ==, 'R');
	close(fd);
	g_object_unref(message);
}

static void
emit_test(const char *name, GVariant *parameters)
{
	GVariant *reply = call(CK, MANAGER, "org.bsd3.Test", name, parameters, "()");
	g_variant_unref(reply);
}

static void
wait_no_devices(void)
{
	unsigned int i;
	guint32 count = 1, acks;
	for (i = 0; i < 100; i++) {
		GVariant *reply = call(CK, MANAGER, "org.bsd3.Test", "GetDeviceCounters", NULL, "(uu)");
		g_variant_get(reply, "(uu)", &count, &acks);
		g_variant_unref(reply);
		if (count == 0)
			return;
		g_usleep(20000);
	}
	g_assert_cmpuint(count, ==, 0);
}

static void
test_device_lifecycle(void)
{
	GDBusConnection *client, *other;
	struct signal_capture *capture, *observer;
	GUnixFDList *fds = NULL;
	if (!peer_pid_available) {
		g_test_skip("Peer PID credentials unavailable on this host"); return;
	}
	mode("control");
	client = client_connection(); other = client_connection();
	capture = start_capture(client); observer = start_capture(other);
	(void)device_request(client, "TakeDevice", 0, 1, &fds, "org.freedesktop.DBus.Error.AccessDenied");
	g_assert_null(fds);
	control(client, "Session6", "TakeControl", FALSE, NULL);
	take_pipe(client, 1, FALSE);
	(void)device_request(client, "TakeDevice", 0, 1, &fds, "org.freedesktop.DBus.Error.InvalidArgs");
	(void)device_request(other, "TakeDevice", 0, 1, &fds, "org.freedesktop.DBus.Error.AccessDenied");
	plain_device(other, "ReleaseDevice", 1, "org.freedesktop.DBus.Error.AccessDenied");
	plain_device(client, "PauseDeviceComplete", 1, "org.freedesktop.DBus.Error.InvalidArgs");
	emit_test("EmitPause", g_variant_new("(uus)", 0U, 1U, "pause"));
	expect_pause(capture, "pause");
	plain_device(client, "PauseDeviceComplete", 1, NULL);
	emit_test("EmitResume", g_variant_new("(uu)", 0U, 1U));
	expect_resume(capture);
	g_assert_null(g_async_queue_timeout_pop(observer->queue, 100000));
	plain_device(client, "ReleaseDevice", 1, NULL);
	plain_device(client, "ReleaseDevice", 1, "org.freedesktop.DBus.Error.InvalidArgs");
	mode("paused"); take_pipe(client, 1, TRUE);
	emit_test("EmitResume", g_variant_new("(uu)", 0U, 1U)); expect_resume(capture);
	emit_test("EmitPause", g_variant_new("(uus)", 0U, 1U, "force")); expect_pause(capture, "force");
	plain_device(client, "PauseDeviceComplete", 1, "org.freedesktop.DBus.Error.InvalidArgs");
	emit_test("EmitResume", g_variant_new("(uu)", 0U, 1U)); expect_resume(capture);
	emit_test("EmitPause", g_variant_new("(uus)", 0U, 1U, "gone")); expect_pause(capture, "gone");
	plain_device(client, "ReleaseDevice", 1, "org.freedesktop.DBus.Error.InvalidArgs");
	wait_no_devices();
	control(client, "Session6", "ReleaseControl", FALSE, NULL);
	g_dbus_connection_remove_filter(client, capture->filter);
	g_dbus_connection_remove_filter(other, observer->filter);
	g_dbus_connection_close_sync(client, NULL, NULL); g_object_unref(client);
	g_dbus_connection_close_sync(other, NULL, NULL); g_object_unref(other);
	mode("normal");
}

static void
test_device_failures(void)
{
	GDBusConnection *client;
	struct signal_capture *capture;
	GUnixFDList *fds = NULL;
	if (!peer_pid_available) {
		g_test_skip("Peer PID credentials unavailable on this host"); return;
	}
	client = client_connection(); mode("control");
	capture = start_capture(client);
	control(client, "Session6", "TakeControl", FALSE, NULL);
	mode("extra-fd");
	(void)device_request(client, "TakeDevice", 0, 1, &fds, "org.freedesktop.DBus.Error.Failed");
	g_assert_null(fds); wait_no_devices();
	mode("control"); take_pipe(client, 1, FALSE);
	mode("device-release-fail");
	plain_device(client, "ReleaseDevice", 1, "org.freedesktop.DBus.Error.Failed");
	wait_no_controllers(); wait_no_devices();
	mode("control"); control(client, "Session6", "TakeControl", FALSE, NULL);
	mode("paused"); take_pipe(client, 1, TRUE);
	mode("extra-resume-fd"); emit_test("EmitResume", g_variant_new("(uu)", 0U, 1U));
	wait_no_controllers(); wait_no_devices();
	g_assert_null(g_async_queue_timeout_pop(capture->queue, 100000));
	mode("control"); control(client, "Session6", "TakeControl", FALSE, NULL);
	take_pipe(client, 1, FALSE);
	g_dbus_connection_remove_filter(client, capture->filter);
	g_dbus_connection_close_sync(client, NULL, NULL); g_object_unref(client);
	wait_no_controllers(); wait_no_devices();
	mode("normal");
}

static void
test_device_limit(void)
{
	GDBusConnection *client;
	GUnixFDList *fds = NULL;
	guint32 i;
	if (!peer_pid_available) {
		g_test_skip("Peer PID credentials unavailable on this host"); return;
	}
	mode("control"); client = client_connection();
	control(client, "Session6", "TakeControl", FALSE, NULL);
	for (i = 1; i <= 256; i++)
		take_pipe(client, i, FALSE);
	(void)device_request(client, "TakeDevice", 0, 257, &fds, "org.freedesktop.DBus.Error.LimitsExceeded");
	g_assert_null(fds);
	g_dbus_connection_close_sync(client, NULL, NULL); g_object_unref(client);
	wait_no_controllers(); wait_no_devices();
	mode("normal");
}

static void
ignore_signal(GDBusConnection *connection, const char *sender, const char *path,
    const char *iface, const char *member, GVariant *parameters, gpointer data)
{
	(void)connection; (void)sender; (void)path; (void)iface;
	(void)member; (void)parameters; (void)data;
}

static void
expect_active(struct signal_capture *capture, gboolean expected)
{
	GDBusMessage *message = g_async_queue_timeout_pop(capture->queue, 3000000);
	GVariant *changes, *invalidated;
	const char *iface;
	gboolean active;
	g_assert_nonnull(message);
	g_assert_cmpstr(g_dbus_message_get_member(message), ==, "PropertiesChanged");
	g_variant_get(g_dbus_message_get_body(message), "(&s@a{sv}@as)", &iface, &changes, &invalidated);
	g_assert_cmpstr(iface, ==, "org.freedesktop.login1.Session");
	g_assert_true(g_variant_lookup(changes, "Active", "b", &active));
	g_assert_cmpint(active, ==, expected);
	g_assert_cmpuint(g_variant_n_children(invalidated), ==, 0);
	g_variant_unref(changes); g_variant_unref(invalidated); g_object_unref(message);
}

static void
test_device_active(void)
{
	GDBusConnection *client;
	struct signal_capture *capture;
	GVariant *barrier;
	GError *error = NULL;
	guint subscription;
	if (!peer_pid_available) {
		g_test_skip("Peer PID credentials unavailable on this host"); return;
	}
	mode("control"); client = client_connection(); capture = start_capture(client);
	control(client, "Session6", "TakeControl", FALSE, NULL);
	subscription = g_dbus_connection_signal_subscribe(client, NULL,
	    "org.freedesktop.DBus.Properties", "PropertiesChanged",
	    "/org/freedesktop/login1/session/Session6", NULL, G_DBUS_SIGNAL_FLAGS_NONE,
	    ignore_signal, NULL, NULL);
	/* Establish that the bus has processed AddMatch before the fixture emits. */
	barrier = g_dbus_connection_call_sync(client, "org.freedesktop.DBus",
	    "/org/freedesktop/DBus", "org.freedesktop.DBus", "GetId", NULL,
	    G_VARIANT_TYPE("(s)"), 0, 3000, NULL, &error);
	g_assert_no_error(error); g_variant_unref(barrier);
	emit_test("EmitActive", g_variant_new("(b)", FALSE)); expect_active(capture, FALSE);
	emit_test("EmitActive", g_variant_new("(b)", TRUE)); expect_active(capture, TRUE);
	control(client, "Session6", "ReleaseControl", FALSE, NULL);
	g_dbus_connection_signal_unsubscribe(client, subscription);
	g_dbus_connection_remove_filter(client, capture->filter);
	g_dbus_connection_close_sync(client, NULL, NULL); g_object_unref(client);
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

static void
expect_owner_status(const char *pid, int expected)
{
	char *args[] = { "./bsddd", "--check-owner", (char *)pid, NULL };
	char *output = NULL, *errors = NULL;
	GError *error = NULL;
	int status;
	g_assert_true(g_spawn_sync(NULL, args, NULL, 0, NULL, NULL,
	    &output, &errors, &status, &error));
	g_assert_no_error(error);
	g_assert_true(WIFEXITED(status));
	g_assert_cmpint(WEXITSTATUS(status), ==, expected);
	g_free(output);
	g_free(errors);
}

static void
test_owner_arguments(void)
{
	const char *bad[] = { "", "0", "-1", "+1", "1x", " 1", "4294967295",
	    "18446744073709551616", NULL };
	for (int i = 0; bad[i] != NULL; i++)
		expect_owner_status(bad[i], 64);
}

static void
test_owner_readiness(void)
{
	GVariant *reply;
	guint32 pid;
	char *text;
	if (!peer_pid_available) {
		g_test_skip("Host cannot provide the owner process ID");
		return;
	}
	reply = call("org.freedesktop.DBus", "/org/freedesktop/DBus",
	    "org.freedesktop.DBus", "GetConnectionUnixProcessID",
	    g_variant_new("(s)", "org.freedesktop.login1"), "(u)");
	g_variant_get(reply, "(u)", &pid);
	g_variant_unref(reply);
	text = g_strdup_printf("%u", pid);
	expect_owner_status(text, 0);
	g_free(text);
	/* The test client is alive but does not own the service's bus name. */
	text = g_strdup_printf("%u", (guint32)getpid());
	expect_owner_status(text, 1);
	g_free(text);
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
	g_test_add_func("/bsd3/readiness-arguments", test_owner_arguments);
	g_test_add_func("/bsd3/readiness-owner", test_owner_readiness);
	g_test_add_func("/bsd3/discovery", test_discovery);
	g_test_add_func("/bsd3/seat-names", test_seat_names);
	g_test_add_func("/bsd3/enumeration", test_enumeration);
	g_test_add_func("/bsd3/display", test_display);
	g_test_add_func("/bsd3/errors", test_errors);
	g_test_add_func("/bsd3/daemon", test_daemon);
	g_test_add_func("/bsd3/controller-ownership", test_control_ownership);
	g_test_add_func("/bsd3/controller-failures", test_control_failures);
	g_test_add_func("/bsd3/controller-missing-credentials", test_missing_credentials);
	g_test_add_func("/bsd3/device-lifecycle", test_device_lifecycle);
	g_test_add_func("/bsd3/device-failures", test_device_failures);
	g_test_add_func("/bsd3/device-limit", test_device_limit);
	g_test_add_func("/bsd3/device-active", test_device_active);
	rc = g_test_run();
	g_object_unref(bus);
	return (rc);
}
