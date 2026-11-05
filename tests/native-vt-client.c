/* SPDX-License-Identifier: BSD-2-Clause */
/* Unprivileged real-VT/device client; no input events are read. */
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/sysctl.h>
#include <dev/evdev/input.h>
#include <errno.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <gio/gio.h>
#include <gio/gunixfdlist.h>
#include "bsd3/sd-login.h"
#define LOGIN "org.freedesktop.login1"
static GAsyncQueue *signals;
static char *path, *owner;
struct signal_sink { GAsyncQueue *queue; char *path; char *owner; };
static void free_sink(gpointer data) {
	struct signal_sink *sink = data;
	g_async_queue_unref(sink->queue);
	g_free(sink->path); g_free(sink->owner); g_free(sink);
}

static GVariant *
request(GDBusConnection *bus, const char *method, GVariant *args, const char *type,
    GUnixFDList **fds)
{
	GError *error = NULL;
	GVariant *reply = g_dbus_connection_call_with_unix_fd_list_sync(bus, LOGIN, path,
	    LOGIN ".Session", method, args, G_VARIANT_TYPE(type), 0, 5000, NULL, fds, NULL, &error);
	g_assert_no_error(error);
	g_assert_nonnull(reply);
	return (reply);
}

static GDBusMessage *
filter(GDBusConnection *bus, GDBusMessage *message, gboolean incoming, gpointer data)
{
	struct signal_sink *sink = data;
	(void)bus;
	if (incoming && g_dbus_message_get_message_type(message) == G_DBUS_MESSAGE_TYPE_SIGNAL &&
	    g_strcmp0(g_dbus_message_get_sender(message), sink->owner) == 0 &&
	    g_strcmp0(g_dbus_message_get_path(message), sink->path) == 0 &&
	    g_strcmp0(g_dbus_message_get_interface(message), LOGIN ".Session") == 0)
		g_async_queue_push(sink->queue, g_object_ref(message));
	return (message);
}

static void
revoked(int fd)
{
	int version;
	struct pollfd check = { .fd = fd, .events = POLLIN };
	for (unsigned i = 0; i < 200; i++) {
		errno = 0;
		if (ioctl(fd, EVIOCGVERSION, &version) == -1 && errno == ENODEV) {
			g_assert_cmpint(poll(&check, 1, 0), ==, 1);
			g_assert_true((check.revents & POLLHUP) != 0);
			return;
		}
		g_usleep(10000);
	}
	g_error("Kernel did not revoke the handed input descriptor");
}

int
main(void)
{
	GDBusConnection *bus;
	GDBusMessage *message;
	GUnixFDList *fds = NULL;
	GVariant *reply, *body;
	GError *error = NULL;
	char *session = NULL, *seat = NULL;
	struct stat original, actual;
	guint32 major_num, minor_num, got_major, got_minor;
	gboolean inactive;
	int first_fd, next_fd, handle, version;
	const char *kind;
	char guest[32];
	size_t guest_size = sizeof(guest);
	const char *text = g_getenv("BSD3_ORIGINAL_VT");
	if (text == NULL || g_strcmp0(g_getenv("BSD3_DISPOSABLE_VM_TEST"), "yes") != 0 ||
	    sysctlbyname("kern.vm_guest", guest, &guest_size, NULL, 0) != 0 || strcmp(guest, "kvm") != 0)
		return (64);
	guint32 original_vt = (guint32)strtoul(text, NULL, 10);
	g_assert_cmpuint(original_vt, >, 0);
	g_assert_cmpint(getuid(), !=, 0);
	g_assert_cmpint(stat("/dev/input/event0", &original), ==, 0);
	g_assert_cmpint(sd_pid_get_session(0, &session), ==, 0);
	g_assert_cmpint(sd_session_get_seat(session, &seat), ==, 0);
	g_assert_cmpstr(seat, ==, "seat0");
	for (unsigned i = 0; i < 500 && sd_session_is_active(session) != 1; i++)
		g_usleep(10000);
	g_assert_cmpint(sd_session_is_active(session), ==, 1);
	bus = g_bus_get_sync(G_BUS_TYPE_SYSTEM, NULL, &error);
	g_assert_no_error(error);
	reply = g_dbus_connection_call_sync(bus, "org.freedesktop.DBus", "/org/freedesktop/DBus",
	    "org.freedesktop.DBus", "GetNameOwner", g_variant_new("(s)", LOGIN),
	    G_VARIANT_TYPE("(s)"), 0, 5000, NULL, &error);
	g_assert_no_error(error);
	g_variant_get(reply, "(s)", &owner);
	g_variant_unref(reply);
	path = g_strconcat("/org/freedesktop/login1/session/", session, NULL);
	signals = g_async_queue_new_full(g_object_unref);
	struct signal_sink *sink = g_new0(struct signal_sink, 1);
	sink->queue = g_async_queue_ref(signals);
	sink->path = g_strdup(path);
	sink->owner = g_strdup(owner);
	guint hook = g_dbus_connection_add_filter(bus, filter, sink, free_sink);
	reply = request(bus, "TakeControl", g_variant_new("(b)", FALSE), "()", NULL);
	g_variant_unref(reply);
	major_num = (guint32)major(original.st_rdev);
	minor_num = (guint32)minor(original.st_rdev);
	reply = request(bus, "TakeDevice", g_variant_new("(uu)", major_num, minor_num), "(hb)", &fds);
	g_variant_get(reply, "(hb)", &handle, &inactive);
	g_assert_false(inactive);
	g_assert_cmpint(g_unix_fd_list_get_length(fds), ==, 1);
	first_fd = g_unix_fd_list_get(fds, handle, &error);
	g_assert_no_error(error);
	g_assert_cmpint(fstat(first_fd, &actual), ==, 0);
	g_assert_cmpuint(actual.st_rdev, ==, original.st_rdev);
	g_assert_cmpint(ioctl(first_fd, EVIOCGVERSION, &version), ==, 0);
	g_variant_unref(reply);
	g_object_unref(fds);
	reply = g_dbus_connection_call_sync(bus, LOGIN, "/org/freedesktop/login1/seat/seat0",
	    LOGIN ".Seat", "SwitchTo", g_variant_new("(u)", original_vt), G_VARIANT_TYPE("()"),
	    0, 5000, NULL, &error);
	g_assert_no_error(error);
	g_variant_unref(reply);
	message = g_async_queue_timeout_pop(signals, 8 * G_TIME_SPAN_SECOND);
	g_assert_nonnull(message);
	g_assert_cmpstr(g_dbus_message_get_member(message), ==, "PauseDevice");
	g_assert_cmpstr(g_dbus_message_get_destination(message), ==, g_dbus_connection_get_unique_name(bus));
	body = g_dbus_message_get_body(message);
	g_variant_get(body, "(uu&s)", &got_major, &got_minor, &kind);
	g_assert_cmpuint(got_major, ==, major_num);
	g_assert_cmpuint(got_minor, ==, minor_num);
	if (strcmp(kind, "pause") == 0) {
		reply = request(bus, "PauseDeviceComplete", g_variant_new("(uu)", major_num, minor_num), "()", NULL);
		g_variant_unref(reply);
	} else
		g_assert_cmpstr(kind, ==, "force");
	g_object_unref(message);
	revoked(first_fd);
	for (unsigned i = 0; i < 200 && sd_session_is_active(session) != 0; i++)
		g_usleep(10000);
	g_assert_cmpint(sd_session_is_active(session), ==, 0);
	reply = g_dbus_connection_call_sync(bus, LOGIN, "/org/freedesktop/login1/seat/seat0",
	    LOGIN ".Seat", "SwitchTo", g_variant_new("(u)", 9U), G_VARIANT_TYPE("()"),
	    0, 5000, NULL, &error);
	g_assert_null(reply);
	g_assert_error(error, G_DBUS_ERROR, G_DBUS_ERROR_ACCESS_DENIED);
	g_clear_error(&error);
	puts("BSD3VT:PAUSED"); fflush(stdout);
	message = g_async_queue_timeout_pop(signals, 8 * G_TIME_SPAN_SECOND);
	g_assert_nonnull(message);
	g_assert_cmpstr(g_dbus_message_get_member(message), ==, "ResumeDevice");
	g_assert_cmpstr(g_dbus_message_get_destination(message), ==, g_dbus_connection_get_unique_name(bus));
	body = g_dbus_message_get_body(message);
	g_variant_get(body, "(uuh)", &got_major, &got_minor, &handle);
	g_assert_cmpuint(got_major, ==, major_num);
	g_assert_cmpuint(got_minor, ==, minor_num);
	fds = g_dbus_message_get_unix_fd_list(message);
	g_assert_nonnull(fds);
	g_assert_cmpint(g_unix_fd_list_get_length(fds), ==, 1);
	next_fd = g_unix_fd_list_get(fds, handle, &error);
	g_assert_no_error(error);
	g_assert_cmpint(fstat(next_fd, &actual), ==, 0);
	g_assert_cmpuint(actual.st_rdev, ==, original.st_rdev);
	g_assert_cmpint(ioctl(next_fd, EVIOCGVERSION, &version), ==, 0);
	revoked(first_fd);
	g_object_unref(message);
	puts("BSD3VT:RESUMED"); fflush(stdout);
	reply = request(bus, "ReleaseDevice", g_variant_new("(uu)", major_num, minor_num), "()", NULL);
	g_variant_unref(reply);
	revoked(next_fd);
	close(first_fd); close(next_fd);
	reply = request(bus, "ReleaseControl", NULL, "()", NULL);
	g_variant_unref(reply);
	g_dbus_connection_remove_filter(bus, hook);
	g_dbus_connection_close_sync(bus, NULL, NULL);
	g_object_unref(bus);
	g_async_queue_unref(signals);
	g_free(path); g_free(owner); free(session); free(seat);
	puts("PASS: real VT switch, inactive refusal, pause/revoke, fresh resume FD and release");
	return (0);
}
