/* SPDX-License-Identifier: BSD-2-Clause */
/* Copyright (c) 2026 Denis Lemire */
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/consio.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>
#include "gdm-freebsd-vt.h"

/* FreeBSD ttyv suffixes are not hexadecimal above ttyvf. */
static const char terminals[] = "0123456789abcdefghijklmnopqrstuv";

static int
open_console(int vt, int require_auto, char *device, size_t size)
{
    struct stat st;
    struct vt_mode mode;
    char path[32];
    int fd, index, saved;

    if (vt < 1 || vt > (int)sizeof(terminals) - 1) {
        errno = EINVAL;
        return -1;
    }
    snprintf(path, sizeof(path), "/dev/ttyv%c", terminals[vt - 1]);
    if (size <= strlen(path)) {
        errno = ENAMETOOLONG;
        return -1;
    }
    fd = open(path, O_RDWR | O_NOCTTY | O_CLOEXEC);
    if (fd < 0)
        return -1;
    if (fstat(fd, &st) < 0 || !S_ISCHR(st.st_mode)) {
        errno = ENOTTY;
        goto fail;
    }
    if (ioctl(fd, VT_GETINDEX, &index) < 0)
        goto fail;
    if (index != vt) {
        errno = ENOTTY;
        goto fail;
    }
    if (require_auto) {
        if (ioctl(fd, VT_GETMODE, &mode) < 0)
            goto fail;
        if (mode.mode != VT_AUTO) {
            errno = EBUSY;
            goto fail;
        }
    }
    memcpy(device, path, strlen(path) + 1);
    return fd;
fail:
    saved = errno;
    close(fd);
    errno = saved;
    return -1;
}

int
gdm_freebsd_vt_available(void)
{
    char path[32];
    int fd, version, active, valid;
    fd = open_console(1, 0, path, sizeof(path));
    if (fd < 0)
        return 0;
    valid = ioctl(fd, CONS_GETVERS, &version) == 0 &&
        ioctl(fd, VT_GETACTIVE, &active) == 0 && active > 0;
    close(fd);
    return valid;
}

int
gdm_freebsd_vt_open(int initial_vt, int initial, int *selected,
    char *device, size_t size)
{
    char initial_device[32];
    int anchor, fd, vt, saved;
    if (selected == NULL || device == NULL) {
        errno = EINVAL;
        return -1;
    }
    /* Keep the initial console open while querying so it stays reserved. */
    anchor = open_console(initial_vt, initial, initial_device,
        sizeof(initial_device));
    if (anchor < 0)
        return -1;
    vt = initial_vt;
    if (!initial) {
        if (ioctl(anchor, VT_OPENQRY, &vt) < 0)
            goto fail;
        if (vt == initial_vt) {
            errno = EAGAIN;
            goto fail;
        }
    }
    if (initial) {
        if (size <= strlen(initial_device)) {
            errno = ENAMETOOLONG;
            goto fail;
        }
        memcpy(device, initial_device, strlen(initial_device) + 1);
        fd = anchor;
    } else {
        fd = open_console(vt, 1, device, size);
        if (fd < 0)
            goto fail;
        close(anchor);
    }
    *selected = vt;
    return fd;
fail:
    saved = errno;
    close(anchor);
    errno = saved;
    return -1;
}

int
gdm_freebsd_vt_activate(int fd, int vt)
{
    int actual;
    struct timespec delay = { .tv_sec = 0, .tv_nsec = 50000000 };
    if (vt < 1 || vt > (int)sizeof(terminals) - 1) {
        errno = EINVAL;
        return -1;
    }
    if (ioctl(fd, VT_GETINDEX, &actual) < 0)
        return -1;
    if (actual != vt) {
        errno = ENOTTY;
        return -1;
    }
    if (ioctl(fd, VT_ACTIVATE, vt) < 0)
        return -1;
    for (int i = 0; i < 60; i++) {
        if (ioctl(fd, VT_GETACTIVE, &actual) < 0)
            return -1;
        if (actual == vt)
            return 0;
        nanosleep(&delay, NULL);
    }
    errno = ETIMEDOUT;
    return -1;
}

int
gdm_freebsd_vt_attach(int fd)
{
    if (ioctl(fd, TIOCSCTTY, 0) < 0)
        return -1;
    if (tcgetsid(fd) != getsid(0)) {
        errno = ENOTTY;
        return -1;
    }
    return 0;
}

int
gdm_freebsd_vt_prepare_stdin(int fd)
{
    int flags;
    if (fd < 0) {
        errno = EBADF;
        return -1;
    }
    if (fd != STDIN_FILENO) {
        if (dup2(fd, STDIN_FILENO) < 0)
            return -1;
        close(fd);
    } else {
        flags = fcntl(fd, F_GETFD);
        if (flags < 0 || fcntl(fd, F_SETFD, flags & ~FD_CLOEXEC) < 0)
            return -1;
    }
    return 0;
}
