/* SPDX-License-Identifier: BSD-2-Clause
 * Copyright (c) 2026 Denis Lemire <denis@lemire.name>
 * Peer identity from a D-Bus credential snapshot.
 * ProcessFD pins the connecting process. Without it, ProcessID can be
 * recycled before the caller looks the pid up; that residual window stays.
 */
#if defined(__linux__)
#define _GNU_SOURCE
#endif
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#if defined(__FreeBSD__)
#include <sys/procdesc.h>
#elif defined(__linux__)
#include <sys/syscall.h>
#ifndef SYS_pidfd_send_signal
#define SYS_pidfd_send_signal 424
#endif
#endif
#include "hardening.h"

#if defined(__linux__)
static bool
linux_pidfd_pid(int fd, pid_t *pid)
{
	char path[64], line[128];
	FILE *info;
	char *end;
	long value;

	snprintf(path, sizeof path, "/proc/self/fdinfo/%d", fd);
	info = fopen(path, "r");
	if (info == NULL)
		return (false);
	while (fgets(line, sizeof line, info) != NULL) {
		if (strncmp(line, "Pid:", 4) != 0)
			continue;
		errno = 0;
		value = strtol(line + 4, &end, 10);
		fclose(info);
		if (errno != 0 || end == line + 4 || value <= 0 ||
		    value > 0x7fffffffL)
			return (false);
		*pid = (pid_t)value;
		return (true);
	}
	fclose(info);
	return (false);
}
#endif

bool
bsd3_process_fd_identifies(int fd, pid_t *pid)
{
	if (fd < 0 || pid == NULL)
		return (false);
#if defined(__linux__)
	{
		pid_t first, second;
		/*
		 * Current pidfds do not share the /proc/<pid> inode. fdinfo's
		 * Pid becomes -1 once the task is gone, which is the D-Bus rule
		 * that an unresolvable ProcessFD must not be used. A zero signal
		 * rejects descriptors that are not pidfds.
		 */
		if (!linux_pidfd_pid(fd, &first))
			return (false);
		if (syscall(SYS_pidfd_send_signal, fd, 0, NULL, 0) < 0)
			return (false);
		if (!linux_pidfd_pid(fd, &second) || second != first)
			return (false);
		*pid = first;
		return (true);
	}
#elif defined(__FreeBSD__)
	{
		pid_t got;
		/*
		 * A process descriptor keeps the pid from being reused until
		 * it is closed. pdgetpid fails for an ordinary file descriptor.
		 */
		if (pdgetpid(fd, &got) != 0 || got <= 0)
			return (false);
		*pid = got;
		return (true);
	}
#else
	(void)fd;
	return (false);
#endif
}

bool
bsd3_process_fd_still(int fd, pid_t pid)
{
	pid_t got;
	if (pid <= 0 || !bsd3_process_fd_identifies(fd, &got))
		return (false);
	return (got == pid);
}

bool
bsd3_peer_identity(bool have_uid, uid_t uid, bool have_pid, pid_t pid,
    bool have_fd, int fd, pid_t *out_pid, bool *pinned)
{
	pid_t resolved;
	(void)uid;
	if (out_pid == NULL || pinned == NULL || !have_uid)
		return (false);
	if (have_fd) {
		/* Present but unusable: do not fall back to a recyclable pid. */
		if (!bsd3_process_fd_identifies(fd, &resolved) || resolved <= 0)
			return (false);
		if (have_pid && pid != resolved)
			return (false);
		*out_pid = resolved;
		*pinned = true;
		return (true);
	}
	/*
	 * Residual risk: the bus omitted ProcessFD. UnixUserID and ProcessID
	 * are one snapshot, but the pid may belong to a different process by
	 * the time the caller asks ConsoleKit which session it is in.
	 */
	if (!have_pid || pid <= 0)
		return (false);
	*out_pid = pid;
	*pinned = false;
	return (true);
}
