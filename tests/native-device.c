/* SPDX-License-Identifier: BSD-2-Clause */
/* Disposable VM only: inspect a new input lease without reading input events. */
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <dev/evdev/input.h>
#include <errno.h>
#include <poll.h>
#include <stdlib.h>
#include <unistd.h>
#include <gio/gio.h>
#include <gio/gunixfdlist.h>
#include "bsd3/sd-login.h"

static GVariant *
request(GDBusConnection *bus, const char *path, const char *method,
    GVariant *args, const char *type, GUnixFDList **fds)
{
	GError *error = NULL;
	GVariant *reply;
	reply = g_dbus_connection_call_with_unix_fd_list_sync(bus,
	    "org.freedesktop.login1", path, "org.freedesktop.login1.Session",
	    method, args, G_VARIANT_TYPE(type), G_DBUS_CALL_FLAGS_NO_AUTO_START,
	    10000, NULL, fds, NULL, &error);
	if (reply == NULL)
		g_error("Real device %s failed: %s", method, error->message);
	return (reply);
}

int
main(int argc, char **argv)
{
	GDBusConnection *bus;
	GUnixFDList *fds = NULL;
	GVariant *reply;
	GError *error = NULL;
	char *session = NULL, *path;
	struct stat original, handed;
	int handle, fd, active, version, result, saved_errno, poll_result, initial_events;
	struct pollfd check;
	gboolean inactive;

	if (argc != 2 || getuid() == 0)
		return (64);
	g_assert_cmpint(lstat(argv[1], &original), ==, 0);
	g_assert_true(S_ISCHR(original.st_mode));
	g_assert_cmpint(sd_pid_get_session(0, &session), ==, 0);
	active = sd_session_is_active(session);
	g_assert_cmpint(active, >=, 0);
	bus = g_bus_get_sync(G_BUS_TYPE_SYSTEM, NULL, &error);
	g_assert_no_error(error);
	g_assert_nonnull(bus);
	path = g_strconcat("/org/freedesktop/login1/session/", session, NULL);
	reply = request(bus, path, "TakeControl", g_variant_new("(b)", FALSE), "()", NULL);
	g_variant_unref(reply);
	reply = request(bus, path, "TakeDevice", g_variant_new("(uu)",
	    (guint32)major(original.st_rdev), (guint32)minor(original.st_rdev)), "(hb)", &fds);
	g_variant_get(reply, "(hb)", &handle, &inactive);
	g_variant_unref(reply);
	g_assert_nonnull(fds);
	g_assert_cmpint(g_unix_fd_list_get_length(fds), ==, 1);
	fd = g_unix_fd_list_get(fds, handle, &error);
	g_assert_no_error(error);
	g_assert_cmpint(fd, >=, 0);
	g_assert_cmpint(fstat(fd, &handed), ==, 0);
	g_assert_true(S_ISCHR(handed.st_mode));
	g_assert_cmpuint(handed.st_rdev, ==, original.st_rdev);
	errno = 0;
	result = ioctl(fd, EVIOCGVERSION, &version);
	saved_errno = errno;
	check = (struct pollfd){ .fd = fd, .events = POLLIN };
	poll_result = poll(&check, 1, 0);
	initial_events = check.revents;
	g_print("Real input lease: session_active=%d inactive_reply=%d version_ioctl=%d errno=%d poll=%d revents=%#x\n",
	    active, inactive, result, saved_errno, poll_result, check.revents);
	/* Release before interpreting results, including a backend semantic failure. */
	g_object_unref(fds);
	reply = request(bus, path, "ReleaseDevice", g_variant_new("(uu)",
	    (guint32)major(original.st_rdev), (guint32)minor(original.st_rdev)), "()", NULL);
	g_variant_unref(reply);
	/* Keep the handed descriptor across release: the kernel must revoke it. */
	errno = 0;
	int released_result = ioctl(fd, EVIOCGVERSION, &version);
	int released_errno = errno;
	check.revents = 0;
	int released_poll = poll(&check, 1, 0);
	int released_events = check.revents;
	close(fd);
	reply = request(bus, path, "ReleaseControl", NULL, "()", NULL);
	g_variant_unref(reply);
	g_object_unref(bus);
	g_free(path);
	free(session);
	g_assert_cmpint(inactive, ==, !active);
	if (active == 0) {
		g_assert_cmpint(result, ==, -1);
		g_assert_cmpint(saved_errno, ==, ENODEV);
		g_assert_cmpint(poll_result, ==, 1);
		g_assert_true((initial_events & POLLHUP) != 0);
	} else {
		g_assert_cmpint(result, ==, 0);
	}
	g_assert_cmpint(released_result, ==, -1);
	g_assert_cmpint(released_errno, ==, ENODEV);
	g_assert_cmpint(released_poll, ==, 1);
	g_assert_true((released_events & POLLHUP) != 0);
	g_print("PASS: input descriptor matches session state and remains revoked after release; no events read\n");
	return (0);
}
