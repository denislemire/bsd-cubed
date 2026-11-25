/* SPDX-License-Identifier: BSD-2-Clause */
#ifndef BSD3_HARDENING_H
#define BSD3_HARDENING_H
#include <sys/types.h>
#include <stdbool.h>
/* devfs name allowlist. Major numbers are not a device class on FreeBSD. */
bool bsd3_device_name_allowed(const char *name);
bool bsd3_cdev_allowed(unsigned int dev_major, unsigned int dev_minor);
/*
 * have_fd selects ProcessFD. A usable descriptor pins the pid. Otherwise
 * UnixUserID and ProcessID are used, and the pid may be recycled before the
 * caller finishes its session lookup.
 */
bool bsd3_peer_identity(bool have_uid, uid_t uid, bool have_pid, pid_t pid,
    bool have_fd, int fd, pid_t *out_pid, bool *pinned);
/* Resolve the process pinned by fd. False if the descriptor is not usable. */
bool bsd3_process_fd_identifies(int fd, pid_t *pid);
/* True only while fd still names pid. Closes the reuse window around a lookup. */
bool bsd3_process_fd_still(int fd, pid_t pid);
#endif
