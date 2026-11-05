/* SPDX-License-Identifier: BSD-2-Clause */
#ifndef BSD3_SD_LOGIN_H
#define BSD3_SD_LOGIN_H
#include <sys/types.h>
#include <errno.h>
/* FreeBSD has ENOATTR rather than the Linux/System V ENODATA spelling.
 * Consumers of this selected sd-login API need a consistent no-data code. */
#if defined(__FreeBSD__) && !defined(ENODATA)
#define ENODATA ENOATTR
#endif
#ifdef __cplusplus
extern "C" {
#endif
/* Experimental subset. Strings/vectors are malloc-owned; use free().
 * Returns nonnegative success or negative errno. Does not set errno.
 * NULL session means the caller's session; pid 0 means the caller's PID.
 * This is not a complete replacement for libsystemd. */
int sd_pid_get_session(pid_t, char **);
int sd_get_sessions(char ***);
int sd_uid_get_sessions(uid_t, int, char ***);
int sd_uid_get_display(uid_t, char **);
int sd_session_is_active(const char *);
int sd_session_get_uid(const char *, uid_t *);
int sd_session_get_type(const char *, char **);
int sd_session_get_class(const char *, char **);
int sd_session_get_state(const char *, char **);
int sd_session_get_seat(const char *, char **);
#ifdef __cplusplus
}
#endif
#endif
