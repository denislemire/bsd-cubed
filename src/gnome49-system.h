/* SPDX-License-Identifier: BSD-2-Clause */
#ifndef BSD3_GNOME49_SYSTEM_H
#define BSD3_GNOME49_SYSTEM_H
#include <sys/types.h>
#include <gio/gio.h>
#include "gsm-system.h"
/* Original adapter to the GsmSystem ABI. The linked GNOME executable remains
 * GPL. This private integration header is not installed as public SDK ABI.
 * New references or NULL/GError; production requires root-owned ConsoleKit. */
GsmSystem *gsm_bsd3_system_new(GError **);
GType gsm_bsd3_system_get_type(void);
/* Explicit dependency injection for isolated protocol tests. */
GsmSystem *gsm_bsd3_system_new_for_connection(GDBusConnection *, uid_t, GError **);
gboolean gsm_bsd3_system_idle_checked(GsmSystem *, gboolean, GError **);
gboolean gsm_bsd3_system_inhibit_checked(GsmSystem *, GsmInhibitorFlag, GError **);
#endif
