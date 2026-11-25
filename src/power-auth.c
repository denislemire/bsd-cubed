/* SPDX-License-Identifier: BSD-2-Clause
 * Copyright (c) 2026 Denis Lemire <denis@lemire.name>
 * Pure power-authorization decision. No bus I/O. Any incomplete fact denies.
 */
#include "power-auth.h"

enum bsd3_power_decision
bsd3_power_decide(const struct bsd3_power_facts *facts)
{
	if (facts == 0 || !facts->credentials_ok)
		return (BSD3_POWER_UNAVAILABLE);
	/* PolicyKit treats uid 0 as authorized. This proxy must not. */
	if (facts->uid == 0 || facts->daemon_bus)
		return (BSD3_POWER_DENY);
	if (facts->session < 0)
		return (BSD3_POWER_UNAVAILABLE);
	if (facts->session == 0)
		return (BSD3_POWER_DENY);
	if (facts->owned < 0 || facts->active < 0 || facts->local < 0)
		return (BSD3_POWER_UNAVAILABLE);
	if (facts->owned == 0 || facts->active == 0 || facts->local == 0)
		return (BSD3_POWER_DENY);
	if (facts->grouped < 0)
		return (BSD3_POWER_UNAVAILABLE);
	if (facts->grouped == 0)
		return (BSD3_POWER_DENY);
	if (facts->polkit < 0)
		return (BSD3_POWER_UNAVAILABLE);
	if (facts->polkit != 1)
		return (BSD3_POWER_DENY);
	return (BSD3_POWER_ALLOW);
}
