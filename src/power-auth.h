/* SPDX-License-Identifier: BSD-2-Clause */
#ifndef BSD3_POWER_AUTH_H
#define BSD3_POWER_AUTH_H
#include <stdbool.h>
#include <sys/types.h>
/* Facts describe the original D-Bus peer, never this process.
 * session, owned, active, local, grouped and polkit use 1 for yes, 0 for no
 * and -1 when that lookup failed. session 0 means the peer has no ConsoleKit
 * session. uid 0 and this daemon's own bus name are denials even when every
 * other fact, including a PolicyKit "authorized" result, is yes. */
enum bsd3_power_decision {
	BSD3_POWER_ALLOW,
	BSD3_POWER_DENY,
	BSD3_POWER_UNAVAILABLE
};
struct bsd3_power_facts {
	bool credentials_ok;
	uid_t uid;
	bool daemon_bus;
	int session;
	int owned;
	int active;
	int local;
	int grouped;
	int polkit;
};
enum bsd3_power_decision bsd3_power_decide(const struct bsd3_power_facts *);
#endif
