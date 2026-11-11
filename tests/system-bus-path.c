/* SPDX-License-Identifier: BSD-2-Clause */
/* Print the system bus path GLib uses after DBUS_SYSTEM_BUS_ADDRESS is cleared. */
#include <stdio.h>
#include <string.h>
#include <gio/gio.h>

int
main(void)
{
	char *address, *comma;
	g_unsetenv("DBUS_SYSTEM_BUS_ADDRESS");
	address = g_dbus_address_get_for_bus_sync(G_BUS_TYPE_SYSTEM, NULL, NULL);
	if (address == NULL || !g_str_has_prefix(address, "unix:path=")) {
		g_printerr("system bus default is not a filesystem socket\n");
		return (1);
	}
	address += strlen("unix:path=");
	comma = strchr(address, ',');
	if (comma != NULL)
		*comma = '\0';
	if (address[0] != '/')
		return (1);
	g_print("%s\n", address);
	return (0);
}
