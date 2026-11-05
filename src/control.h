/* SPDX-License-Identifier: BSD-2-Clause */
#ifndef BSD3_CONTROL_H
#define BSD3_CONTROL_H
#include <gio/gio.h>
void bsd3_controls_start(GDBusConnection *);
void bsd3_controls_stop(void);
void bsd3_control_call(const char *, const char *, GVariant *,
    GDBusMethodInvocation *);
#endif
