/* SPDX-License-Identifier: BSD-2-Clause */
/* Test harness: hold a real non-local CK session while its child runs. */
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <gio/gio.h>
#include "bsd3/sd-login.h"

#define CK "org.freedesktop.ConsoleKit"
#define MANAGER "/org/freedesktop/ConsoleKit/Manager"
#define MANAGER_IF CK ".Manager"
#define SESSION_IF CK ".Session"

static GVariant *
request(GDBusConnection *bus, const char *path, const char *interface,
    const char *method, GVariant *args, const char *type)
{
	GError *error = NULL;
	GVariant *reply;
	reply = g_dbus_connection_call_sync(bus, CK, path, interface, method,
	    args, G_VARIANT_TYPE(type), G_DBUS_CALL_FLAGS_NONE, 10000, NULL, &error);
	if (reply == NULL)
		g_error("Real CK %s failed: %s", method, error->message);
	return (reply);
}

static int
verify_session(void)
{
	char *session = NULL, *type = NULL, *class = NULL, *path;
	GError *error = NULL;
	GDBusConnection *bus;
	const char *methods[] = { "TakeControl", "ReleaseControl" };
	uid_t uid;

	g_assert_cmpint(sd_pid_get_session(0, &session), ==, 0);
	g_assert_cmpint(sd_session_get_uid(session, &uid), ==, 0);
	g_assert_cmpuint(uid, ==, getuid());
	g_assert_cmpint(sd_session_get_type(session, &type), ==, 0);
	g_assert_cmpstr(type, ==, "wayland");
	g_assert_cmpint(sd_session_get_class(session, &class), ==, 0);
	g_assert_cmpstr(class, ==, "user");
	g_print("PASS: child resolves to real CK Wayland/user session with its own UID\n");
	/* Exercise the installed privileged bridge with the original child identity. */
	bus = g_bus_get_sync(G_BUS_TYPE_SYSTEM, NULL, &error);
	g_assert_no_error(error);
	g_assert_nonnull(bus);
	path = g_strconcat("/org/freedesktop/login1/session/", session, NULL);
	for (unsigned i = 0; i < G_N_ELEMENTS(methods); i++) {
		GVariant *reply = g_dbus_connection_call_sync(bus,
		    "org.freedesktop.login1", path, "org.freedesktop.login1.Session",
		    methods[i], i == 0 ? g_variant_new("(b)", FALSE) : NULL,
		    G_VARIANT_TYPE("()"), G_DBUS_CALL_FLAGS_NO_AUTO_START, 10000,
		    NULL, &error);
		g_assert_no_error(error);
		g_assert_nonnull(reply);
		g_variant_unref(reply);
	}
	g_print("PASS: real session child acquired and released installed bridge control\n");
	g_free(path);
	g_object_unref(bus);
	free(session);
	free(type);
	free(class);
	return (0);
}

int
main(int argc, char **argv)
{
	GDBusConnection *bus;
	GVariantBuilder params;
	GVariant *reply;
	GError *error = NULL;
	char *cookie, *path, *runtime;
	struct stat st;
	gboolean closed, local;
	guint32 vt;
	int status;

	if (argc == 2 && strcmp(argv[1], "--verify") == 0)
		return (verify_session());
	if (argc < 2 || getuid() == 0)
		return (64);
	bus = g_bus_get_sync(G_BUS_TYPE_SYSTEM, NULL, &error);
	if (bus == NULL)
		g_error("Cannot connect real system bus: %s", error->message);
	g_variant_builder_init(&params, G_VARIANT_TYPE("a(sv)"));
	g_variant_builder_add(&params, "(sv)", "session-type", g_variant_new_string("wayland"));
	g_variant_builder_add(&params, "(sv)", "session-class", g_variant_new_string("user"));
	g_variant_builder_add(&params, "(sv)", "session-service", g_variant_new_string("bsd3-ci"));
	/* Request a non-local session and verify the backend honors the boolean. */
	g_variant_builder_add(&params, "(sv)", "is-local", g_variant_new_boolean(FALSE));
	reply = request(bus, MANAGER, MANAGER_IF, "OpenSessionWithParameters",
	    g_variant_new("(a(sv))", &params), "(s)");
	g_variant_get(reply, "(s)", &cookie);
	g_variant_unref(reply);
	reply = request(bus, MANAGER, MANAGER_IF, "GetSessionForCookie",
	    g_variant_new("(s)", cookie), "(o)");
	g_variant_get(reply, "(o)", &path);
	g_variant_unref(reply);
	reply = request(bus, path, SESSION_IF, "IsLocal", NULL, "(b)");
	g_variant_get(reply, "(b)", &local);
	g_variant_unref(reply);
	g_assert_false(local);
	reply = request(bus, path, SESSION_IF, "GetVTNr", NULL, "(u)");
	g_variant_get(reply, "(u)", &vt);
	g_variant_unref(reply);
	g_assert_cmpuint(vt, ==, 0);
	g_print("PASS: actual CK registration is non-local with VT zero\n");
	reply = request(bus, path, SESSION_IF, "GetXDGRuntimeDir", NULL, "(s)");
	g_variant_get(reply, "(s)", &runtime);
	g_variant_unref(reply);
	g_assert_cmpint(lstat(runtime, &st), ==, 0);
	g_assert_true(S_ISDIR(st.st_mode));
	g_assert_cmpuint(st.st_uid, ==, getuid());
	g_assert_cmpuint(st.st_mode & 0777, ==, 0700);
	g_setenv("XDG_SESSION_COOKIE", cookie, TRUE);
	g_setenv("XDG_RUNTIME_DIR", runtime, TRUE);
	g_setenv("XDG_SESSION_TYPE", "wayland", TRUE);
	g_setenv("XDG_SESSION_CLASS", "user", TRUE);
	/* Cookie stays in child environment: never print it into artifacts. */
	if (!g_spawn_sync(NULL, argv + 1, NULL, G_SPAWN_SEARCH_PATH, NULL,
	    NULL, NULL, NULL, &status, &error))
		g_error("Session child failed to launch: %s", error->message);
	reply = request(bus, MANAGER, MANAGER_IF, "CloseSession",
	    g_variant_new("(s)", cookie), "(b)");
	g_variant_get(reply, "(b)", &closed);
	g_variant_unref(reply);
	g_assert_true(closed);
	g_dbus_connection_close_sync(bus, NULL, NULL);
	g_object_unref(bus);
	g_free(cookie);
	g_free(path);
	g_free(runtime);
	if (!WIFEXITED(status))
		return (1);
	return (WEXITSTATUS(status));
}
