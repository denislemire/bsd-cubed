/* SPDX-License-Identifier: BSD-2-Clause */
#define _GNU_SOURCE
#include <errno.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#include <glib.h>
#include "hardening.h"
#if defined(__linux__)
#include <sys/syscall.h>
#ifndef SYS_pidfd_open
#define SYS_pidfd_open 434
#endif
#endif

static void
test_device_names(void)
{
	static const char *allowed[] = {
	    "drm/0", "drm/128", "dri/card0", "dri/card10", "dri/renderD128",
	    "dri/controlD64", "dri/control64", "input/event0", "input/event15",
	    "input/event256"
	};
	static const char *denied[] = {
	    "", "#", "#C:1:1", "mem", "kmem", "null", "zero", "ada0", "nvd0",
	    "pci", "usb", "ttyv0", "cuau0", "bpf0", "acpi", "sysmouse", "kbd0",
	    "kbdmux0", "atkbd0", "ukbd0", "ums0", "input/uinput", "uinput",
	    "input/event", "input/mice", "input/mouse0", "input/event0/extra",
	    "input/../mem", "dri/card", "dri/card0foo", "dri/renderD", "drm/",
	    "drm/0a", "drm", "dri/card-1", "../drm/0", "dri/card0\n", "drm/0 ",
	    "nvidia0", "nvidiactl", "dri/controlD", "input/js0"
	};
	guint i;
	g_assert_false(bsd3_device_name_allowed(NULL));
	for (i = 0; i < G_N_ELEMENTS(allowed); i++)
		g_assert_true(bsd3_device_name_allowed(allowed[i]));
	for (i = 0; i < G_N_ELEMENTS(denied); i++)
		g_assert_false(bsd3_device_name_allowed(denied[i]));
	/* No devfs node uses this dev_t. Resolution fails closed. */
	g_assert_false(bsd3_cdev_allowed(0xffffffffu, 0xffffffffu));
}

static void
test_unpinned_pid(void)
{
	pid_t pid = 0;
	bool pinned = true;
	g_assert_true(bsd3_peer_identity(true, getuid(), true, 4321, false, -1,
	    &pid, &pinned));
	g_assert_cmpint(pid, ==, 4321);
	/* Documented residual risk: no ProcessFD, so the pid is not pinned. */
	g_assert_false(pinned);
	g_assert_false(bsd3_peer_identity(false, getuid(), true, 1, false, -1,
	    &pid, &pinned));
	g_assert_false(bsd3_peer_identity(true, getuid(), false, 0, false, -1,
	    &pid, &pinned));
	g_assert_false(bsd3_peer_identity(true, getuid(), true, 0, false, -1,
	    &pid, &pinned));
	g_assert_false(bsd3_peer_identity(true, getuid(), true, -1, false, -1,
	    &pid, &pinned));
}

static int
open_pidfd(pid_t pid)
{
#if defined(__linux__)
	return ((int)syscall(SYS_pidfd_open, pid, 0));
#else
	(void)pid;
	errno = ENOTSUP;
	return (-1);
#endif
}

static void
test_process_fd(void)
{
	int fd, pipe_ends[2], child_fd;
	pid_t pid = 0, child;
	bool pinned = false;
	int status;
	fd = open_pidfd(getpid());
	if (fd < 0) {
		g_test_skip("ProcessFD identity needs a pidfd or process descriptor");
		return;
	}
	g_assert_true(bsd3_process_fd_identifies(fd, &pid));
	g_assert_cmpint(pid, ==, getpid());
	g_assert_true(bsd3_process_fd_still(fd, getpid()));
	g_assert_false(bsd3_process_fd_still(fd, getpid() + 1));
	pid = 0;
	pinned = false;
	g_assert_true(bsd3_peer_identity(true, getuid(), true, getpid(), true, fd,
	    &pid, &pinned));
	g_assert_true(pinned);
	g_assert_cmpint(pid, ==, getpid());
	/* ProcessFD wins over a contradictory ProcessID. No fallback. */
	g_assert_false(bsd3_peer_identity(true, getuid(), true, getpid() + 1, true,
	    fd, &pid, &pinned));
	g_assert_cmpint(pipe(pipe_ends), ==, 0);
	g_assert_false(bsd3_process_fd_identifies(pipe_ends[0], &pid));
	g_assert_false(bsd3_peer_identity(true, getuid(), true, getpid(), true,
	    pipe_ends[0], &pid, &pinned));
	close(pipe_ends[0]);
	close(pipe_ends[1]);
	child = fork();
	if (child == 0)
		_exit(0);
	g_assert_cmpint(child, >, 0);
	child_fd = open_pidfd(child);
	g_assert_cmpint(child_fd, >=, 0);
	g_assert_cmpint(waitpid(child, &status, 0), ==, child);
	g_assert_true(WIFEXITED(status));
	pid = 0;
	g_assert_false(bsd3_process_fd_identifies(child_fd, &pid));
	close(child_fd);
	close(fd);
}

int
main(int argc, char **argv)
{
	g_test_init(&argc, &argv, NULL);
	g_test_add_func("/bsd3/device-name-allowlist", test_device_names);
	g_test_add_func("/bsd3/peer-unpinned-process-id", test_unpinned_pid);
	g_test_add_func("/bsd3/peer-process-fd", test_process_fd);
	return (g_test_run());
}
