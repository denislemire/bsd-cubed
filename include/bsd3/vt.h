/* SPDX-License-Identifier: BSD-2-Clause */
/* Copyright (c) 2026 Denis Lemire */
#ifndef BSD3_VT_H
#define BSD3_VT_H
#include <stddef.h>
int bsd3_vt_available(void);
/* Return an owned CLOEXEC descriptor, or -1 with errno. No VT switch occurs. */
int bsd3_vt_open(int initial_vt, int initial, int *selected,
    char *device, size_t size);
/* Only activate the VT matching this descriptor; wait at most three seconds. */
int bsd3_vt_activate(int fd, int vt);
/* Consume fd as stdin, retaining fd0 correctly and clearing CLOEXEC. */
int bsd3_vt_prepare_stdin(int fd);
/* Called in the newly setsid() child, before dropping privileges. */
int bsd3_vt_attach(int fd);
#endif
