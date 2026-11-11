/* SPDX-License-Identifier: BSD-2-Clause */
#ifndef BSD3_POWER_H
#define BSD3_POWER_H
#include <gio/gio.h>
int bsd3_power_set_group(const char *);
int bsd3_power_set_group_file(const char *);
gboolean bsd3_power_call(GDBusConnection *, const char *, const char *,
    GVariant *, GDBusMethodInvocation *);
#endif
