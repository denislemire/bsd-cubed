/* SPDX-License-Identifier: BSD-2-Clause */
#ifndef BSD3_SEATS_H
#define BSD3_SEATS_H
#include <gio/gio.h>
struct bsd3_seat { char *name, *component, *path, *owner; };
GPtrArray *bsd3_seats_list(GDBusConnection *, GError **);
GVariant *bsd3_seat_property(GDBusConnection *, const char *, const char *, GError **);
#endif
