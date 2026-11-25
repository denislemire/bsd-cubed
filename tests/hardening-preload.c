/* SPDX-License-Identifier: BSD-2-Clause
 * Test harness only. Not installed.
 * bsddd clears DBUS_SYSTEM_BUS_ADDRESS and resolves devices with devname(3).
 * This library isolates those two calls so unprivileged protocol tests never
 * open the host system bus and do not need real DRM or evdev nodes:
 * connect(2) to the compiled-in system bus path is redirected, and devname_r
 * reports fixture names for the major/minor pairs the protocol tests use.
 */
#if defined(__linux__)
#define _GNU_SOURCE
#endif
#include <dlfcn.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/un.h>
#if defined(__linux__)
#include <sys/sysmacros.h>
#endif

static int (*real_connect)(int, const struct sockaddr *, socklen_t);

__attribute__((constructor)) static void
init_preload(void)
{
	real_connect = dlsym(RTLD_NEXT, "connect");
}

static int
copy_name(char *buf, int len, const char *name)
{
	size_t n;
	if (buf == NULL || len <= 1 || name == NULL)
		return (-1);
	n = strlen(name);
	if (n >= (size_t)len)
		return (-1);
	memcpy(buf, name, n + 1);
	return (0);
}

char *
devname_r(dev_t dev, mode_t type, char *buf, int len)
{
	unsigned int maj, min;
	char numbered[64];
	const char *name = NULL;
	if (type != S_IFCHR)
		return (NULL);
	maj = (unsigned int)major(dev);
	min = (unsigned int)minor(dev);
	if (maj == 0 && min >= 1 && min <= 256) {
		snprintf(numbered, sizeof numbered, "input/event%u", min);
		if (copy_name(buf, len, numbered) < 0)
			return (NULL);
		return (buf);
	}
	if (maj == 1 && min == 1)
		name = "mem";
	else if (maj == 2 && min == 1)
		name = "dri/card0";
	else if (maj == 2 && min == 2)
		name = "drm/0";
	else if (maj == 2 && min == 3)
		name = "dri/renderD128";
	else if (maj == 2 && min == 4)
		name = "input/event0";
	else if (maj == 2 && min == 5)
		name = "input/uinput";
	else if (maj == 2 && min == 6)
		name = "dri/controlD64";
	else if (maj == 2 && min == 7)
		name = "dri/control64";
	else if (maj == 3 && min == 1)
		name = "#C:3:1";
	else if (maj == 4 && min == 1)
		name = "ada0";
	else if (maj == 4 && min == 2)
		name = "ttyv0";
	else if (maj == 4 && min == 3)
		name = "sysmouse";
	if (name == NULL || copy_name(buf, len, name) < 0)
		return (NULL);
	return (buf);
}

int
connect(int socket, const struct sockaddr *address, socklen_t length)
{
	const struct sockaddr_un *un;
	const char *from, *to;
	struct sockaddr_un replaced;
	socklen_t replaced_len;
	size_t from_len, to_len;
	if (real_connect == NULL)
		init_preload();
	from = getenv("BSD3_REDIRECT_SYSTEM_BUS_FROM");
	to = getenv("BSD3_REDIRECT_SYSTEM_BUS_TO");
	if (from == NULL || to == NULL || from[0] == '\0' || to[0] == '\0' ||
	    address == NULL || address->sa_family != AF_UNIX || real_connect == NULL)
		return (real_connect == NULL ? -1 : real_connect(socket, address, length));
	un = (const struct sockaddr_un *)address;
	if (length < offsetof(struct sockaddr_un, sun_path) + 2 ||
	    un->sun_path[0] == '\0')
		return (real_connect(socket, address, length));
	for (from_len = 0; from_len < sizeof un->sun_path; from_len++)
		if (un->sun_path[from_len] == '\0')
			break;
	if (from_len == sizeof un->sun_path || strcmp(un->sun_path, from) != 0)
		return (real_connect(socket, address, length));
	to_len = strlen(to);
	if (to_len == 0 || to_len >= sizeof replaced.sun_path)
		return (real_connect(socket, address, length));
	memset(&replaced, 0, sizeof replaced);
	replaced.sun_family = AF_UNIX;
#ifdef __FreeBSD__
	replaced.sun_len = (unsigned char)(offsetof(struct sockaddr_un, sun_path) +
	    to_len + 1);
#endif
	memcpy(replaced.sun_path, to, to_len + 1);
	replaced_len = (socklen_t)(offsetof(struct sockaddr_un, sun_path) + to_len + 1);
	(void)length;
	return (real_connect(socket, (struct sockaddr *)&replaced, replaced_len));
}
