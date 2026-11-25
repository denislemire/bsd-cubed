/* SPDX-License-Identifier: BSD-2-Clause
 * Copyright (c) 2026 Denis Lemire <denis@lemire.name>
 *
 * login1 power and inhibit. The system bus authenticates the peer socket.
 * This process's uid is not a caller and is never consulted. PolicyKit
 * CheckAuthorization is asked about the peer's unique bus name. ConsoleKit
 * is actuated only after that decision is allow; a ConsoleKit reply is not
 * the caller's permission.
 */
#include <errno.h>
#include <fcntl.h>
#include <grp.h>
#include <pwd.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <gio/gio.h>
#include <gio/gunixfdlist.h>
#include "bsd3/sd-login.h"
#include "power.h"
#include "control.h"
#include "hardening.h"
#include "power-auth.h"

#define CK "org.freedesktop.ConsoleKit"
#define CK_MANAGER "/org/freedesktop/ConsoleKit/Manager"
#define CK_IFACE CK ".Manager"
#define LOGIN "org.freedesktop.login1"
#define LOGIN_MANAGER LOGIN ".Manager"
#define ROOT "/org/freedesktop/login1"
#define POLKIT "org.freedesktop.PolicyKit1"
#define POLKIT_PATH "/org/freedesktop/PolicyKit1/Authority"
#define POLKIT_IFACE POLKIT ".Authority"

struct power_map {
	const char *login1;
	const char *consolekit;
	const char *action;
	gboolean capability;
};

static const struct power_map methods[] = {
	{ "CanSuspend", "CanSuspend", "org.freedesktop.login1.suspend", TRUE },
	{ "CanHibernate", "CanHibernate", "org.freedesktop.login1.hibernate", TRUE },
	{ "CanHybridSleep", "CanHybridSleep",
	    "org.freedesktop.login1.hybrid-sleep", TRUE },
	{ "CanPowerOff", "CanPowerOff", "org.freedesktop.login1.power-off", TRUE },
	{ "CanReboot", "CanReboot", "org.freedesktop.login1.reboot", TRUE },
	{ "CanSuspendThenHibernate", NULL,
	    "org.freedesktop.login1.suspend-then-hibernate", TRUE },
	{ "Suspend", "Suspend", "org.freedesktop.login1.suspend", FALSE },
	{ "Hibernate", "Hibernate", "org.freedesktop.login1.hibernate", FALSE },
	{ "HybridSleep", "HybridSleep", "org.freedesktop.login1.hybrid-sleep", FALSE },
	{ "PowerOff", "PowerOff", "org.freedesktop.login1.power-off", FALSE },
	{ "Reboot", "Reboot", "org.freedesktop.login1.reboot", FALSE },
};

static char *group_name;
static char *group_file;

static gboolean
valid_group_name(const char *name)
{
	size_t i, n;
	if (name == NULL)
		return (FALSE);
	n = strlen(name);
	if (n == 0 || n > 32)
		return (FALSE);
	for (i = 0; i < n; i++) {
		char c = name[i];
		if (!g_ascii_isalnum(c) && c != '_' && c != '-' && c != '.')
			return (FALSE);
	}
	return (TRUE);
}

int
bsd3_power_set_group(const char *name)
{
	char *copy;
	if (!valid_group_name(name))
		return (-1);
	copy = strdup(name);
	if (copy == NULL)
		return (-1);
	free(group_name);
	group_name = copy;
	return (0);
}

int
bsd3_power_set_group_file(const char *path)
{
	char *copy;
	if (path == NULL || path[0] != '/' || strlen(path) > 512)
		return (-1);
	copy = strdup(path);
	if (copy == NULL)
		return (-1);
	free(group_file);
	group_file = copy;
	return (0);
}

static int
group_from_file(const char *path, char **out)
{
	char buf[80], name[33], expect[80];
	ssize_t n;
	int fd, wrote;
	fd = open(path, O_RDONLY | O_CLOEXEC);
	if (fd < 0)
		return (-1);
	n = read(fd, buf, sizeof buf);
	close(fd);
	if (n <= 0 || n >= (ssize_t)sizeof buf)
		return (-1);
	buf[n] = '\0';
	if (sscanf(buf, "power_group=\"%32[A-Za-z0-9_.-]\"\n", name) != 1)
		return (-1);
	if (!valid_group_name(name))
		return (-1);
	wrote = snprintf(expect, sizeof expect, "power_group=\"%s\"\n", name);
	if (wrote < 0 || (ssize_t)wrote != n || memcmp(buf, expect, (size_t)n) != 0)
		return (-1);
	*out = strdup(name);
	return (*out == NULL ? -1 : 0);
}

static int
current_group(char **out)
{
	if (group_file != NULL)
		return (group_from_file(group_file, out));
	if (group_name != NULL) {
		*out = strdup(group_name);
		return (*out == NULL ? -1 : 0);
	}
	*out = strdup("operator");
	return (*out == NULL ? -1 : 0);
}

static int
member_of_group(uid_t uid, const char *group_name_value)
{
	struct passwd *pw;
	struct group *gr;
	gid_t *groups, *next, primary, group_gid;
	char *user;
	int cap = 32, used, rc, i, member;

	/* The argument uid is the peer. getuid() would name this process. */
	pw = getpwuid(uid);
	if (pw == NULL || pw->pw_name == NULL || pw->pw_name[0] == '\0')
		return (-1);
	user = strdup(pw->pw_name);
	primary = pw->pw_gid;
	if (user == NULL)
		return (-1);
	gr = getgrnam(group_name_value);
	if (gr == NULL) {
		free(user);
		return (-1);
	}
	group_gid = gr->gr_gid;
	groups = calloc((size_t)cap, sizeof(*groups));
	if (groups == NULL) {
		free(user);
		return (-1);
	}
	for (;;) {
		used = cap;
		rc = getgrouplist(user, primary, groups, &used);
		if (rc >= 0)
			break;
		if (used <= cap) {
			free(groups);
			free(user);
			return (-1);
		}
		cap = used;
		next = realloc(groups, (size_t)cap * sizeof(*groups));
		if (next == NULL) {
			free(groups);
			free(user);
			return (-1);
		}
		groups = next;
	}
	/* Linux returns the count. FreeBSD returns 0 and writes the count. */
	if (rc > 0)
		used = rc;
	member = 0;
	for (i = 0; i < used; i++) {
		if (groups[i] == group_gid)
			member = 1;
	}
	free(groups);
	free(user);
	return (member);
}

static int
session_is_local(GDBusConnection *bus, const char *session)
{
	const unsigned char *p;
	char *path;
	GError *error = NULL;
	GVariant *reply;
	gboolean local;

	if (session == NULL || strncmp(session, "Session", 7) != 0)
		return (-1);
	for (p = (const unsigned char *)session; *p != '\0'; p++) {
		if (!g_ascii_isalnum(*p) && *p != '_')
			return (-1);
	}
	path = g_strconcat("/org/freedesktop/ConsoleKit/", session, NULL);
	reply = g_dbus_connection_call_sync(bus, CK, path, CK ".Session",
	    "IsLocal", NULL, G_VARIANT_TYPE("(b)"),
	    G_DBUS_CALL_FLAGS_NO_AUTO_START, 3000, NULL, &error);
	g_free(path);
	if (reply == NULL) {
		g_clear_error(&error);
		return (-1);
	}
	g_variant_get(reply, "(b)", &local);
	g_variant_unref(reply);
	return (local ? 1 : 0);
}

static int
polkit_check(GDBusConnection *bus, const char *sender, const char *action)
{
	GError *error = NULL;
	GVariantBuilder subject, details;
	GVariant *reply, *result, *authorized_v, *challenge_v;
	gboolean authorized, challenge;

	/* Unique names only. A well-known name would be a different subject. */
	if (sender == NULL || sender[0] != ':' || action == NULL || action[0] == '\0')
		return (-1);
	g_variant_builder_init(&subject, G_VARIANT_TYPE("a{sv}"));
	g_variant_builder_add(&subject, "{sv}", "name", g_variant_new_string(sender));
	g_variant_builder_init(&details, G_VARIANT_TYPE("a{ss}"));
	reply = g_dbus_connection_call_sync(bus, POLKIT, POLKIT_PATH, POLKIT_IFACE,
	    "CheckAuthorization",
	    g_variant_new("((sa{sv})sa{ss}us)", "system-bus-name", &subject,
		action, &details, 0U, ""),
	    G_VARIANT_TYPE("((bba{ss}))"), G_DBUS_CALL_FLAGS_NO_AUTO_START,
	    3000, NULL, &error);
	if (reply == NULL) {
		g_clear_error(&error);
		return (-1);
	}
	result = g_variant_get_child_value(reply, 0);
	authorized_v = g_variant_get_child_value(result, 0);
	challenge_v = g_variant_get_child_value(result, 1);
	authorized = g_variant_get_boolean(authorized_v);
	challenge = g_variant_get_boolean(challenge_v);
	g_variant_unref(authorized_v);
	g_variant_unref(challenge_v);
	g_variant_unref(result);
	g_variant_unref(reply);
	/* flags is 0: a challenge must not become a grant. */
	if (!authorized || challenge)
		return (0);
	return (1);
}

static enum bsd3_power_decision
authorize(GDBusConnection *bus, const char *sender, const char *action)
{
	struct bsd3_power_facts facts, probe;
	const char *mine;
	uid_t uid;
	pid_t pid;
	bool pinned;
	int fd;
	char *id = NULL, *group = NULL;
	int rc;

	memset(&facts, 0, sizeof facts);
	if (sender == NULL || sender[0] != ':')
		return (bsd3_power_decide(&facts));
	mine = g_dbus_connection_get_unique_name(bus);
	if (mine == NULL)
		return (bsd3_power_decide(&facts));
	if (!bsd3_peer_credentials(bus, sender, &pid, &uid, &pinned, &fd))
		return (bsd3_power_decide(&facts));
	facts.credentials_ok = TRUE;
	facts.uid = (uid_t)uid;
	facts.daemon_bus = strcmp(sender, mine) == 0;
	/* uid 0 and this daemon's bus name never reach PolicyKit or ConsoleKit. */
	if (facts.uid == 0 || facts.daemon_bus) {
		if (fd >= 0)
			close(fd);
		return (bsd3_power_decide(&facts));
	}
	rc = sd_pid_get_session((pid_t)pid, &id);
	if (pinned && !bsd3_process_fd_still(fd, pid))
		rc = -ESRCH;
	if (fd >= 0)
		close(fd);
	if (rc == -ENODATA || rc == -ENXIO)
		facts.session = 0;
	else if (rc < 0)
		facts.session = -1;
	else
		facts.session = 1;
	if (facts.session == 1) {
		uid_t owner = 0;
		int active, local;
		rc = sd_session_get_uid(id, &owner);
		facts.owned = rc < 0 ? -1 : (owner == facts.uid ? 1 : 0);
		active = sd_session_is_active(id);
		facts.active = active < 0 ? -1 : (active > 0 ? 1 : 0);
		local = session_is_local(bus, id);
		facts.local = local;
	}
	free(id);
	probe = facts;
	probe.grouped = 1;
	probe.polkit = 1;
	if (bsd3_power_decide(&probe) != BSD3_POWER_ALLOW)
		return (bsd3_power_decide(&facts));
	if (current_group(&group) < 0 || !valid_group_name(group))
		facts.grouped = -1;
	else
		facts.grouped = member_of_group(facts.uid, group);
	free(group);
	probe = facts;
	probe.polkit = 1;
	if (bsd3_power_decide(&probe) != BSD3_POWER_ALLOW)
		return (bsd3_power_decide(&facts));
	facts.polkit = polkit_check(bus, sender, action);
	return (bsd3_power_decide(&facts));
}

static const struct power_map *
lookup(const char *method)
{
	guint i;
	for (i = 0; i < G_N_ELEMENTS(methods); i++) {
		if (strcmp(methods[i].login1, method) == 0)
			return (&methods[i]);
	}
	return (NULL);
}

static void
denied(GDBusMethodInvocation *call, enum bsd3_power_decision decision,
    gboolean capability)
{
	if (capability && decision == BSD3_POWER_DENY) {
		g_dbus_method_invocation_return_value(call, g_variant_new("(s)", "no"));
		return;
	}
	g_dbus_method_invocation_return_dbus_error(call,
	    "org.freedesktop.DBus.Error.AccessDenied",
	    decision == BSD3_POWER_UNAVAILABLE ?
	    "Power authorization is unavailable" : "Permission denied");
}

static void
relay_transition(GDBusConnection *bus, const char *sender, const char *path,
    const char *iface, const char *signal, GVariant *parameters, gpointer data)
{
	(void)sender; (void)path; (void)iface; (void)data;
	if (g_variant_is_of_type(parameters, G_VARIANT_TYPE("(b)")))
		g_dbus_connection_emit_signal(bus, NULL, ROOT, LOGIN_MANAGER,
		    signal, parameters, NULL);
}

static GThreadPool *power_pool;
static gint outstanding;
static void power_request(gpointer, gpointer);

void
bsd3_power_start(GDBusConnection *bus)
{
	const char *signals[] = { "PrepareForSleep", "PrepareForShutdown" };
	GError *error = NULL;

	power_pool = g_thread_pool_new(power_request, NULL, 1, TRUE, &error);
	if (error) {
		g_warning("Cannot start power request worker: %s", error->message);
		g_clear_error(&error);
		if (power_pool)
			g_thread_pool_free(power_pool, TRUE, FALSE);
		power_pool = NULL;
	}
	for (guint i = 0; i < G_N_ELEMENTS(signals); i++)
		g_dbus_connection_signal_subscribe(bus, CK, CK_IFACE, signals[i],
		    CK_MANAGER, NULL, G_DBUS_SIGNAL_FLAGS_NONE, relay_transition,
		    NULL, NULL);
}

static void
return_capability(GDBusConnection *bus, const char *ck_method,
    GDBusMethodInvocation *call)
{
	GError *error = NULL;
	GVariant *reply;
	const char *result, *answer;

	if (ck_method == NULL) {
		g_dbus_method_invocation_return_value(call, g_variant_new("(s)", "na"));
		return;
	}
	reply = g_dbus_connection_call_sync(bus, CK, CK_MANAGER, CK_IFACE,
	    ck_method, NULL, G_VARIANT_TYPE("(s)"),
	    G_DBUS_CALL_FLAGS_NO_AUTO_START, 3000, NULL, &error);
	if (reply == NULL) {
		g_clear_error(&error);
		denied(call, BSD3_POWER_UNAVAILABLE, TRUE);
		return;
	}
	g_variant_get(reply, "(&s)", &result);
	if (strcmp(result, "yes") == 0)
		answer = "yes";
	else if (strcmp(result, "na") == 0)
		answer = "na";
	else
		answer = "no";
	g_dbus_method_invocation_return_value(call, g_variant_new("(s)", answer));
	g_variant_unref(reply);
}

static void
action_done(GObject *source, GAsyncResult *result, gpointer data)
{
	GDBusMethodInvocation *call = data;
	GError *error = NULL;
	GVariant *reply = g_dbus_connection_call_finish(G_DBUS_CONNECTION(source),
	    result, &error);
	if (reply == NULL) {
		g_dbus_method_invocation_return_gerror(call, error);
		g_error_free(error);
	} else {
		g_dbus_method_invocation_return_value(call, reply);
		g_variant_unref(reply);
	}
	g_atomic_int_add(&outstanding, -1);
}

static void
forward_inhibit(GDBusConnection *bus, GVariant *parameters,
    GDBusMethodInvocation *call)
{
	GError *error = NULL;
	GUnixFDList *fds = NULL;
	GVariant *reply;
	const char *what, *who, *why, *mode;
	int handle;

	if (parameters == NULL ||
	    !g_variant_is_of_type(parameters, G_VARIANT_TYPE("(ssss)"))) {
		g_dbus_method_invocation_return_dbus_error(call,
		    "org.freedesktop.DBus.Error.InvalidArgs",
		    "Inhibit arguments are invalid");
		return;
	}
	g_variant_get(parameters, "(&s&s&s&s)", &what, &who, &why, &mode);
	if ((strcmp(mode, "block") != 0 && strcmp(mode, "delay") != 0) ||
	    what[0] == '\0' || strlen(what) > 256 || strlen(who) > 256 ||
	    strlen(why) > 256) {
		g_dbus_method_invocation_return_dbus_error(call,
		    "org.freedesktop.DBus.Error.InvalidArgs",
		    "Inhibit mode must be block or delay");
		return;
	}
	reply = g_dbus_connection_call_with_unix_fd_list_sync(bus, CK, CK_MANAGER,
	    CK_IFACE, "Inhibit", g_variant_new("(ssss)", what, who, why, mode),
	    G_VARIANT_TYPE("(h)"), G_DBUS_CALL_FLAGS_NO_AUTO_START, 3000,
	    NULL, &fds, NULL, &error);
	if (reply == NULL) {
		g_clear_object(&fds);
		g_dbus_method_invocation_return_gerror(call, error);
		g_error_free(error);
		return;
	}
	g_variant_get(reply, "(h)", &handle);
	if (fds == NULL || handle < 0 || handle >= g_unix_fd_list_get_length(fds)) {
		g_variant_unref(reply);
		g_clear_object(&fds);
		denied(call, BSD3_POWER_UNAVAILABLE, FALSE);
		return;
	}
	g_dbus_method_invocation_return_value_with_unix_fd_list(call,
	    g_variant_new("(h)", handle), fds);
	g_variant_unref(reply);
	g_object_unref(fds);
}

/* Serialize credential and NSS lookups off the bus dispatch thread. */
static void
power_request(gpointer data, gpointer unused)
{
	GDBusMethodInvocation *call = data;
	GDBusConnection *bus = g_dbus_method_invocation_get_connection(call);
	const char *sender = g_dbus_method_invocation_get_sender(call);
	const char *method = g_dbus_method_invocation_get_method_name(call);
	GVariant *parameters = g_dbus_method_invocation_get_parameters(call);
	const struct power_map *map = lookup(method);
	const char *action = map ? map->action : "org.freedesktop.login1.inhibit";
	enum bsd3_power_decision decision = authorize(bus, sender, action);
	(void)unused;
	if (decision != BSD3_POWER_ALLOW)
		denied(call, decision, map != NULL && map->capability);
	else if (!map)
		forward_inhibit(bus, parameters, call);
	else if (map->capability)
		return_capability(bus, map->consolekit, call);
	else {
		g_dbus_connection_call(bus, CK, CK_MANAGER, CK_IFACE,
		    map->consolekit, parameters, G_VARIANT_TYPE("()"),
		    G_DBUS_CALL_FLAGS_NO_AUTO_START, 30000, NULL, action_done, call);
		return;
	}
	g_atomic_int_add(&outstanding, -1);
}

gboolean
bsd3_power_call(GDBusMethodInvocation *call)
{
	const char *method = g_dbus_method_invocation_get_method_name(call);

	if (!lookup(method) && strcmp(method, "Inhibit") != 0)
		return (FALSE);
	if (!power_pool)
		denied(call, BSD3_POWER_UNAVAILABLE, FALSE);
	else if (g_atomic_int_get(&outstanding) >= 32)
		g_dbus_method_invocation_return_dbus_error(call,
		    "org.freedesktop.DBus.Error.LimitsExceeded", "Power request queue full");
	else {
		g_atomic_int_inc(&outstanding);
		g_thread_pool_push(power_pool, call, NULL);
	}
	return (TRUE);
}
