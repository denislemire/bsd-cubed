/* SPDX-License-Identifier: BSD-2-Clause */
/* Actual SDDM Wayland child: no fake backend and no input events. */
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/consio.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>
#include <gio/gio.h>
#include "bsd3/sd-login.h"
int main(void) {
	char *session = NULL, *type = NULL, *class = NULL, *seat = NULL;
	uid_t uid;
	int vt;
	g_assert_cmpint(getuid(), !=, 0);
	g_assert_true(isatty(0));
	g_assert_cmpint(tcgetsid(0), ==, getsid(0));
	g_assert_cmpint(ioctl(0, VT_GETINDEX, &vt), ==, 0);
	g_assert_cmpint(sd_pid_get_session(0, &session), ==, 0);
	g_assert_cmpint(sd_session_get_uid(session, &uid), ==, 0);
	g_assert_cmpuint(uid, ==, getuid());
	g_assert_cmpint(sd_session_get_type(session, &type), ==, 0);
	g_assert_cmpstr(type, ==, "wayland");
	g_assert_cmpint(sd_session_get_class(session, &class), ==, 0);
	g_assert_cmpstr(class, ==, "user");
	g_assert_cmpint(sd_session_get_seat(session, &seat), ==, 0);
	g_assert_cmpstr(seat, ==, "seat0");
	g_assert_cmpint(sd_session_is_active(session), ==, 1);
	GError *error = NULL;
	GDBusConnection *bus = g_bus_get_sync(G_BUS_TYPE_SYSTEM, NULL, &error);
	g_assert_no_error(error);
	char *path = g_strconcat("/org/freedesktop/login1/session/", session, NULL);
	const char *methods[] = { "TakeControl", "ReleaseControl" };
	for (unsigned i = 0; i < G_N_ELEMENTS(methods); i++) {
		GVariant *reply = g_dbus_connection_call_sync(bus, "org.freedesktop.login1", path,
		    "org.freedesktop.login1.Session", methods[i],
		    i == 0 ? g_variant_new("(b)", FALSE) : NULL, G_VARIANT_TYPE("()"),
		    0, 3000, NULL, &error);
		g_assert_no_error(error); g_assert_nonnull(reply); g_variant_unref(reply);
	}
	char *result = g_build_filename(g_get_home_dir(), ".bsd3-sddm-probe", "result.txt", NULL);
	FILE *output = fopen(result, "wx");
	g_assert_nonnull(output);
	fprintf(output, "PASS: real SDDM Wayland child UID=%u VT=%d seat0 active own-session control\n", (unsigned)uid, vt);
	g_assert_cmpint(fclose(output), ==, 0);
	g_print("PASS: SDDM child retained its controlling VT and verified local Wayland/user session\n");
	g_object_unref(bus); g_free(path); g_free(result);
	free(session); free(type); free(class); free(seat);
	return (0);
}
