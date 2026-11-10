/* SPDX-License-Identifier: BSD-2-Clause
 * Copyright (c) 2026 Denis Lemire <denis@lemire.name>
 * login1 power/inhibit subset over ConsoleKit. Original caller identity is
 * checked before a privileged backend call. This is not full logind.
 */
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <gio/gio.h>
#include <gio/gunixfdlist.h>
#include "bsd3/sd-login.h"
#include "power.h"

#define CK "org.freedesktop.ConsoleKit"
#define CK_MANAGER "/org/freedesktop/ConsoleKit/Manager"
#define CK_IFACE CK ".Manager"
#define LOGIN "org.freedesktop.login1"
#define LOGIN_MANAGER LOGIN ".Manager"
#define ROOT "/org/freedesktop/login1"

struct power_map {
	const char *login1;
	const char *consolekit;
	gboolean capability;
	gboolean action;
	gboolean always_na;
};

static const struct power_map methods[] = {
	{ "CanSuspend", "CanSuspend", TRUE, FALSE, FALSE },
	{ "CanHibernate", "CanHibernate", TRUE, FALSE, FALSE },
	{ "CanHybridSleep", "CanHybridSleep", TRUE, FALSE, FALSE },
	{ "CanPowerOff", "CanStop", TRUE, FALSE, FALSE },
	{ "CanReboot", "CanRestart", TRUE, FALSE, FALSE },
	{ "CanSuspendThenHibernate", NULL, TRUE, FALSE, TRUE },
	{ "Suspend", "Suspend", FALSE, TRUE, FALSE },
	{ "Hibernate", "Hibernate", FALSE, TRUE, FALSE },
	{ "HybridSleep", "HybridSleep", FALSE, TRUE, FALSE },
	{ "PowerOff", "Stop", FALSE, TRUE, FALSE },
	{ "Reboot", "Restart", FALSE, TRUE, FALSE },
};

static void
fail(GDBusMethodInvocation *call, int code)
{
	const char *name = "org.freedesktop.DBus.Error.Failed";
	if (code == -EACCES || code == -EPERM)
		name = "org.freedesktop.DBus.Error.AccessDenied";
	g_dbus_method_invocation_return_dbus_error(call, name, g_strerror(-code));
}

static int
caller_pid(GDBusConnection *bus, const char *sender, guint32 *pid)
{
	GError *error = NULL;
	GVariant *reply = g_dbus_connection_call_sync(bus,
	    "org.freedesktop.DBus", "/org/freedesktop/DBus",
	    "org.freedesktop.DBus", "GetConnectionUnixProcessID",
	    g_variant_new("(s)", sender), G_VARIANT_TYPE("(u)"),
	    G_DBUS_CALL_FLAGS_NONE, 3000, NULL, &error);
	if (reply == NULL) {
		g_clear_error(&error);
		return (-EIO);
	}
	g_variant_get(reply, "(u)", pid);
	g_variant_unref(reply);
	return (0);
}

static int
require_session(GDBusConnection *bus, const char *sender)
{
	char *id = NULL;
	guint32 pid;
	int rc;

	rc = caller_pid(bus, sender, &pid);
	if (rc < 0)
		return (rc);
	if ((guint64)pid > G_MAXINT32)
		return (-EINVAL);
	rc = sd_pid_get_session((pid_t)pid, &id);
	if (rc < 0)
		return (rc);
	free(id);
	return (0);
}

static void
prepare_for_sleep(GDBusConnection *bus, gboolean entering)
{
	GError *error = NULL;
	g_dbus_connection_emit_signal(bus, NULL, ROOT, LOGIN_MANAGER,
	    "PrepareForSleep", g_variant_new("(b)", entering), &error);
	g_clear_error(&error);
}

static const struct power_map *
lookup(const char *method)
{
	guint i;
	for (i = 0; i < G_N_ELEMENTS(methods); i++)
		if (strcmp(methods[i].login1, method) == 0)
			return (&methods[i]);
	return (NULL);
}

static void
forward_capability(GDBusConnection *bus, const struct power_map *map,
    GDBusMethodInvocation *call)
{
	GError *error = NULL;
	GVariant *reply;
	const char *result;

	if (map->always_na) {
		g_dbus_method_invocation_return_value(call, g_variant_new("(s)", "na"));
		return;
	}
	reply = g_dbus_connection_call_sync(bus, CK, CK_MANAGER, CK_IFACE,
	    map->consolekit, NULL, G_VARIANT_TYPE("(s)"),
	    G_DBUS_CALL_FLAGS_NO_AUTO_START, 3000, NULL, &error);
	if (reply == NULL) {
		g_dbus_method_invocation_return_gerror(call, error);
		g_error_free(error);
		return;
	}
	g_variant_get(reply, "(&s)", &result);
	if (strcmp(result, "yes") != 0 && strcmp(result, "no") != 0 &&
	    strcmp(result, "challenge") != 0 && strcmp(result, "na") != 0) {
		g_variant_unref(reply);
		g_dbus_method_invocation_return_dbus_error(call,
		    "org.freedesktop.DBus.Error.Failed",
		    "ConsoleKit returned an unknown capability");
		return;
	}
	g_dbus_method_invocation_return_value(call, g_variant_new("(s)", result));
	g_variant_unref(reply);
}

static void
forward_action(GDBusConnection *bus, const struct power_map *map,
    GVariant *parameters, GDBusMethodInvocation *call)
{
	GError *error = NULL;
	GVariant *reply;
	gboolean interactive;

	g_variant_get(parameters, "(b)", &interactive);
	prepare_for_sleep(bus, TRUE);
	reply = g_dbus_connection_call_sync(bus, CK, CK_MANAGER, CK_IFACE,
	    map->consolekit, g_variant_new("(b)", interactive),
	    G_VARIANT_TYPE("()"), G_DBUS_CALL_FLAGS_NO_AUTO_START,
	    G_MAXINT, NULL, &error);
	prepare_for_sleep(bus, FALSE);
	if (reply == NULL) {
		g_dbus_method_invocation_return_gerror(call, error);
		g_error_free(error);
		return;
	}
	g_variant_unref(reply);
	g_dbus_method_invocation_return_value(call, NULL);
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

	g_variant_get(parameters, "(&s&s&s&s)", &what, &who, &why, &mode);
	if (strcmp(mode, "block") != 0 && strcmp(mode, "delay") != 0) {
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
		g_dbus_method_invocation_return_gerror(call, error);
		g_error_free(error);
		return;
	}
	g_variant_get(reply, "(h)", &handle);
	g_dbus_method_invocation_return_value_with_unix_fd_list(call,
	    g_variant_new("(h)", handle), fds);
	g_variant_unref(reply);
	g_clear_object(&fds);
}

gboolean
bsd3_power_call(GDBusConnection *bus, const char *sender, const char *method,
    GVariant *parameters, GDBusMethodInvocation *call)
{
	const struct power_map *map;
	int rc;

	if (strcmp(method, "Inhibit") == 0) {
		rc = require_session(bus, sender);
		if (rc < 0) {
			fail(call, rc);
			return (TRUE);
		}
		forward_inhibit(bus, parameters, call);
		return (TRUE);
	}
	map = lookup(method);
	if (map == NULL)
		return (FALSE);
	rc = require_session(bus, sender);
	if (rc < 0) {
		fail(call, rc);
		return (TRUE);
	}
	if (map->capability)
		forward_capability(bus, map, call);
	else
		forward_action(bus, map, parameters, call);
	return (TRUE);
}
