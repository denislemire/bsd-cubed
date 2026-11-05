# ConsoleKit2 locality correction

Overlay for sysutils/consolekit2 at the ports revision pinned by the Mutter
experiment. The existing release sets final locality true whenever an is-local
property exists, including a false boolean. CI28 observed IsLocal=true for its
headless session even after omitting the override, so omission cannot establish
non-local registration either. The candidate uses the actual typed boolean.

The native regression explicitly requests false, checks the real IsLocal reply
and VT zero, then verifies child UID/type/class and authenticated bridge control.
This is a change to the GPL ConsoleKit dependency; BSD3's original C remains
BSD-2-Clause. Preserve the complete modified dependency source with its package.
This does not fix the separate TakeDevice inactivity reply or prove VT/devices.
