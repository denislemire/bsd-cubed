/* SPDX-License-Identifier: BSD-2-Clause */
#ifndef BSD3_GREETER_SESSION_H
#define BSD3_GREETER_SESSION_H
#include <gio/gio.h>
#include <sys/types.h>

/* The authenticated display-manager worker owns this connection until logout.
 * Outputs are allocated with g_malloc and are assigned only on success. */
GDBusConnection *bsd3_open_display_session(uid_t, const char *service,
    const char *device, unsigned vt, gboolean greeter, char **cookie,
    char **session, char **runtime, GError **);
gboolean bsd3_activate_display_session(GDBusConnection *, const char *seat,
    const char *session, GCancellable *, GError **);
gboolean bsd3_unlock_display_session(GDBusConnection *, const char *session, GError **);
#endif
