/* SPDX-License-Identifier: BSD-2-Clause
 * Copyright (c) 2026 Denis Lemire <denis@lemire.name>
 * Link a real selected API into a shared-library consumer using installed metadata.
 */
#include <bsd3/sd-login.h>

int
bsd3_consumer_session(pid_t pid, char **session)
{
	return (sd_pid_get_session(pid, session));
}
