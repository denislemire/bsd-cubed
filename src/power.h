/* SPDX-License-Identifier: BSD-2-Clause */
#ifndef BSD3_POWER_H
#define BSD3_POWER_H
#include <gio/gio.h>
gboolean bsd3_power_call(GDBusConnection *, const char *, const char *,
    GVariant *, GDBusMethodInvocation *);
#endif
