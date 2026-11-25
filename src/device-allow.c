/* SPDX-License-Identifier: BSD-2-Clause
 * Copyright (c) 2026 Denis Lemire <denis@lemire.name>
 * Seat-device allowlist. FreeBSD devfs assigns major numbers at boot, so a
 * major is not a DRM or evdev class. Identity is the devfs node name from
 * devname(3), which is what libdrm uses for the same reason.
 */
#if !defined(__FreeBSD__)
#define _GNU_SOURCE
#endif
#include <stdbool.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#if defined(__FreeBSD__)
#include <stdlib.h>
#else
#include <dlfcn.h>
#include <sys/sysmacros.h>
#endif
#include "hardening.h"

static bool
prefixed_number(const char *name, const char *prefix)
{
	size_t length = strlen(prefix);
	const unsigned char *rest;
	if (strncmp(name, prefix, length) != 0)
		return (false);
	rest = (const unsigned char *)name + length;
	if (*rest < '0' || *rest > '9')
		return (false);
	for (; *rest != '\0'; rest++)
		if (*rest < '0' || *rest > '9')
			return (false);
	return (true);
}

bool
bsd3_device_name_allowed(const char *name)
{
	if (name == NULL || name[0] == '\0' || name[0] == '#')
		return (false);
	if (strchr(name, '.') != NULL || strchr(name, '\\') != NULL)
		return (false);
	/* DRM cdevs are drm/N. dri/ names are the compatibility nodes. */
	if (prefixed_number(name, "drm/") ||
	    prefixed_number(name, "dri/card") ||
	    prefixed_number(name, "dri/renderD") ||
	    prefixed_number(name, "dri/controlD") ||
	    prefixed_number(name, "dri/control"))
		return (true);
	/* FreeBSD evdev nodes are input/eventN. uinput is not a seat device. */
	return (prefixed_number(name, "input/event"));
}

bool
bsd3_cdev_allowed(unsigned int dev_major, unsigned int dev_minor)
{
	char name[256];
	char *resolved = NULL;
	memset(name, 0, sizeof name);
#if defined(__FreeBSD__)
	/*
	 * Direct libc lookup. A synthetic "#C:major:minor" fallback means the
	 * kernel has no cdev; the allowlist rejects it.
	 */
	resolved = devname_r(makedev((int)dev_major, (int)dev_minor), S_IFCHR,
	    name, (int)sizeof name);
#else
	/*
	 * Non-FreeBSD protocol hosts have no devfs. Fail closed unless a test
	 * harness has interposed devname_r. Production builds are FreeBSD.
	 */
	typedef char *(*bsd3_devname_fn)(dev_t, mode_t, char *, int);
	bsd3_devname_fn fn = (bsd3_devname_fn)dlsym(RTLD_DEFAULT, "devname_r");
	if (fn != NULL)
		resolved = fn(makedev(dev_major, dev_minor), S_IFCHR, name,
		    (int)sizeof name);
#endif
	if (resolved == NULL || name[0] == '\0')
		return (false);
	name[sizeof name - 1] = '\0';
	return (bsd3_device_name_allowed(name));
}
