/* SPDX-License-Identifier: BSD-2-Clause */
/* Ordinary-user local Wayland session registration and descendant supervision. */
#include <stdio.h>
#ifdef __FreeBSD__
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/consio.h>
#include <sys/procctl.h>
#include <sys/wait.h>
#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>
#include <gio/gio.h>

#define CK "org.freedesktop.ConsoleKit"
#define MANAGER "/org/freedesktop/ConsoleKit/Manager"
static volatile sig_atomic_t stopping;
static const char *backend;
static GDBusConnection *bus;

static void interrupted(int sig) { stopping = sig; }
static double monotonic(void) {
	struct timespec value;
	clock_gettime(CLOCK_MONOTONIC, &value);
	return (value.tv_sec + value.tv_nsec / 1000000000.0);
}

static GVariant *
call(const char *destination, const char *path, const char *interface,
    const char *method, GVariant *args, const char *type, GError **error)
{
	return (g_dbus_connection_call_sync(bus, destination, path, interface,
	    method, args, G_VARIANT_TYPE(type), 0, 3000, NULL, error));
}

static GVariant *
request(const char *path, const char *method, GVariant *args, const char *type)
{
	GError *error = NULL;
	GVariant *reply = call(backend, path, strcmp(path, MANAGER) == 0 ?
	    CK ".Manager" : CK ".Session", method, args, type, &error);
	if (reply == NULL) {
		/* Error text can contain a cookie; log only the operation name. */
		fprintf(stderr, "bsd3-session: ConsoleKit %s failed\n", method);
		g_clear_error(&error);
	}
	return (reply);
}

static gboolean
string_is(const char *path, const char *method, const char *expected)
{
	GVariant *reply = request(path, method, NULL, "(s)");
	const char *value;
	gboolean valid;
	if (reply == NULL)
		return (FALSE);
	g_variant_get(reply, "(&s)", &value);
	valid = strcmp(value, expected) == 0;
	g_variant_unref(reply);
	return (valid);
}

static gboolean
valid_session(const char *path, int vt, char **runtime)
{
	GVariant *reply;
	guint32 value;
	gboolean local;
	struct stat st;

	reply = request(path, "GetUnixUser", NULL, "(u)");
	if (reply == NULL)
		return (FALSE);
	g_variant_get(reply, "(u)", &value);
	g_variant_unref(reply);
	if (value != getuid())
		return (FALSE);
	reply = request(path, "GetVTNr", NULL, "(u)");
	if (reply == NULL)
		return (FALSE);
	g_variant_get(reply, "(u)", &value);
	g_variant_unref(reply);
	if (value != (guint32)vt)
		return (FALSE);
	reply = request(path, "IsLocal", NULL, "(b)");
	if (reply == NULL)
		return (FALSE);
	g_variant_get(reply, "(b)", &local);
	g_variant_unref(reply);
	if (!local || !string_is(path, "GetSessionType", "wayland") ||
	    !string_is(path, "GetSessionClass", "user"))
		return (FALSE);
	for (int i = 0; i < 30 && !stopping; i++) {
		reply = request(path, "IsActive", NULL, "(b)");
		if (reply == NULL)
			return (FALSE);
		g_variant_get(reply, "(b)", &local);
		g_variant_unref(reply);
		if (local)
			break;
		g_usleep(100000);
	}
	if (!local || stopping)
		return (FALSE);
	reply = request(path, "GetXDGRuntimeDir", NULL, "(s)");
	if (reply == NULL)
		return (FALSE);
	g_variant_get(reply, "(s)", runtime);
	g_variant_unref(reply);
	return ((*runtime)[0] == '/' && lstat(*runtime, &st) == 0 &&
	    S_ISDIR(st.st_mode) && st.st_uid == getuid() && (st.st_mode & 0777) == 0700);
}

static int
signal_descendants(int sig)
{
	struct procctl_reaper_kill action = { .rk_sig = sig };
	if (procctl(P_PID, 0, PROC_REAP_KILL, &action) == 0 || errno == ESRCH)
		return (0);
	perror("bsd3-session: signal descendants");
	return (-1);
}

static gboolean
backend_alive(void)
{
	GError *error = NULL;
	GVariant *reply = call("org.freedesktop.DBus", "/org/freedesktop/DBus",
	    "org.freedesktop.DBus", "NameHasOwner", g_variant_new("(s)", backend), "(b)", &error);
	gboolean alive = FALSE;
	if (reply != NULL) {
		g_variant_get(reply, "(b)", &alive);
		g_variant_unref(reply);
	}
	g_clear_error(&error);
	return (alive);
}

int
main(int argc, char **argv)
{
	GVariant *reply;
	GVariantBuilder params;
	GError *error = NULL;
	char *owner = NULL, *cookie = NULL, *path = NULL, *runtime = NULL;
	char **environment = NULL;
	const char *inherited;
	struct stat tty;
	struct sigaction action = { .sa_handler = interrupted };
	GPid child;
	int vt, status = 0, result = 1;
	gboolean owned = FALSE, reaper = FALSE, started = FALSE, forced_stop = FALSE;
	double deadline = 0, next_owner_check;

	if (argc < 3 || strcmp(argv[1], "--") != 0 || getuid() == 0 ||
	    geteuid() != getuid()) {
		fprintf(stderr, "usage: bsd3-session -- command [argument ...] (ordinary user)\n");
		return (64);
	}
	if (!isatty(0) || fstat(0, &tty) != 0 || !S_ISCHR(tty.st_mode) ||
	    tcgetsid(0) != getsid(0) || tcgetpgrp(0) != getpgrp() ||
	    ioctl(0, VT_GETINDEX, &vt) != 0 || vt <= 0) {
		fprintf(stderr, "bsd3-session: a foreground controlling FreeBSD VT is required\n");
		return (1);
	}
	/* Always use the real machine bus; no caller-supplied alternate backend. */
	g_unsetenv("DBUS_SYSTEM_BUS_ADDRESS");
	environment = g_get_environ();
	sigemptyset(&action.sa_mask);
	sigaction(SIGHUP, &action, NULL);
	sigaction(SIGINT, &action, NULL);
	sigaction(SIGTERM, &action, NULL);
	bus = g_bus_get_sync(G_BUS_TYPE_SYSTEM, NULL, &error);
	if (bus == NULL)
		goto done;
	reply = call("org.freedesktop.DBus", "/org/freedesktop/DBus", "org.freedesktop.DBus",
	    "StartServiceByName", g_variant_new("(su)", CK, 0U), "(u)", &error);
	if (reply == NULL)
		goto done;
	g_variant_unref(reply);
	reply = call("org.freedesktop.DBus", "/org/freedesktop/DBus", "org.freedesktop.DBus",
	    "GetNameOwner", g_variant_new("(s)", CK), "(s)", &error);
	if (reply == NULL)
		goto done;
	g_variant_get(reply, "(s)", &owner);
	g_variant_unref(reply);
	backend = owner;
	inherited = g_getenv("XDG_SESSION_COOKIE");
	if (inherited != NULL && inherited[0] != '\0') {
		cookie = g_strdup(inherited);
	} else {
		g_variant_builder_init(&params, G_VARIANT_TYPE("a(sv)"));
		g_variant_builder_add(&params, "(sv)", "session-type", g_variant_new_string("wayland"));
		g_variant_builder_add(&params, "(sv)", "session-class", g_variant_new_string("user"));
		g_variant_builder_add(&params, "(sv)", "session-service", g_variant_new_string("bsd3-sddm"));
		g_variant_builder_add(&params, "(sv)", "is-local", g_variant_new_boolean(TRUE));
		reply = request(MANAGER, "OpenSessionWithParameters",
		    g_variant_new("(a(sv))", &params), "(s)");
		if (reply == NULL)
			goto done;
		g_variant_get(reply, "(s)", &cookie);
		g_variant_unref(reply);
		owned = TRUE;
	}
	reply = request(MANAGER, "GetSessionForCookie", g_variant_new("(s)", cookie), "(o)");
	if (reply == NULL)
		goto done;
	g_variant_get(reply, "(o)", &path);
	g_variant_unref(reply);
	if (!owned) {
		reply = request(MANAGER, "GetSessionForUnixProcess",
		    g_variant_new("(u)", (guint32)getpid()), "(o)");
		if (reply == NULL)
			goto done;
		const char *actual;
		g_variant_get(reply, "(&o)", &actual);
		gboolean member = strcmp(actual, path) == 0;
		g_variant_unref(reply);
		if (!member)
			goto done;
	}
	if (!valid_session(path, vt, &runtime)) {
		fprintf(stderr, "bsd3-session: real session identity/locality/VT/runtime validation failed\n");
		goto done;
	}
	if (procctl(P_PID, 0, PROC_REAP_ACQUIRE, NULL) != 0) {
		perror("bsd3-session: acquire descendant supervision");
		goto done;
	}
	reaper = TRUE;
	environment = g_environ_setenv(environment, "XDG_SESSION_COOKIE", cookie, TRUE);
	environment = g_environ_setenv(environment, "XDG_RUNTIME_DIR", runtime, TRUE);
	environment = g_environ_setenv(environment, "XDG_SESSION_TYPE", "wayland", TRUE);
	environment = g_environ_setenv(environment, "XDG_SESSION_CLASS", "user", TRUE);
	if (stopping || !g_spawn_async(NULL, argv + 2, environment,
	    G_SPAWN_SEARCH_PATH | G_SPAWN_DO_NOT_REAP_CHILD | G_SPAWN_CHILD_INHERITS_STDIN,
	    NULL, NULL, &child, &error))
		goto done;
	started = TRUE;
	fprintf(stderr, "bsd3-session: verified local Wayland/user session on VT%d\n", vt);
	next_owner_check = monotonic() + 1;
	for (;;) {
		pid_t waited = waitpid(child, &status, WNOHANG);
		if (waited == child)
			break;
		if (waited < 0 && errno != EINTR)
			goto done;
		if (!stopping && monotonic() >= next_owner_check) {
			if (!backend_alive()) {
				fprintf(stderr, "bsd3-session: ConsoleKit owner lost; terminating session\n");
				stopping = SIGTERM;
			}
			next_owner_check = monotonic() + 1;
		}
		if (stopping && deadline == 0) {
			signal_descendants(stopping);
			deadline = monotonic() + 5;
		}
		if (deadline != 0 && monotonic() >= deadline) {
			if (forced_stop)
				goto done;
			signal_descendants(SIGKILL);
			forced_stop = TRUE;
			deadline = monotonic() + 2;
		}
		g_usleep(10000);
	}
	started = FALSE;
	g_spawn_close_pid(child);
	result = stopping ? 128 + stopping : WIFEXITED(status) ? WEXITSTATUS(status) :
	    WIFSIGNALED(status) ? 128 + WTERMSIG(status) : 1;
done:
	if (error != NULL) {
		fprintf(stderr, "bsd3-session: setup or child launch failed\n");
		g_clear_error(&error);
	}
	if (reaper) {
		if (started)
			result = 1;
		if (signal_descendants(SIGTERM) != 0)
			result = 1;
		double end = monotonic() + 2;
		gboolean forced = FALSE;
		for (;;) {
			pid_t waited = waitpid(-1, NULL, WNOHANG);
			if (waited < 0 && errno == ECHILD)
				break;
			if (monotonic() >= end) {
				if (forced) { result = 1; break; }
				signal_descendants(SIGKILL);
				fprintf(stderr, "bsd3-session: descendant cleanup required SIGKILL\n");
				result = 1;
				forced = TRUE;
				end = monotonic() + 2;
			}
			if (waited <= 0)
				g_usleep(10000);
		}
		if (procctl(P_PID, 0, PROC_REAP_RELEASE, NULL) != 0)
			result = 1;
	}
	if (owned) {
		reply = request(MANAGER, "CloseSession", g_variant_new("(s)", cookie), "(b)");
		gboolean closed = FALSE;
		if (reply != NULL) { g_variant_get(reply, "(b)", &closed); g_variant_unref(reply); }
		if (!closed)
			result = 1;
	}
	if (bus != NULL)
		g_object_unref(bus);
	g_strfreev(environment);
	g_free(owner); g_free(cookie); g_free(path); g_free(runtime);
	return (result);
}
#else
int main(void) {
	fputs("bsd3-session requires FreeBSD\n", stderr);
	return (69);
}
#endif
