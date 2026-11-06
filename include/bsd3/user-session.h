/* SPDX-License-Identifier: BSD-2-Clause */
#ifndef BSD3_USER_SESSION_H
#define BSD3_USER_SESSION_H

/* Native ordinary-user session API; returns 0 or a negative errno value.
 * No systemd ABI, bus name, system service or privileged operation is provided.
 */
int bsd3_session_ready(void);
/* A NULL value removes the variable from FUTURE services, not running ones. */
int bsd3_session_setenv(const char *name, const char *value);

#endif
