/* SPDX-License-Identifier: BSD-2-Clause */
#include <bsd3/user-session.h>
int bsd3_shared_ready(void) { return bsd3_session_ready(); }
int bsd3_shared_environment(const char *name, const char *value)
{ return bsd3_session_setenv(name, value); }
