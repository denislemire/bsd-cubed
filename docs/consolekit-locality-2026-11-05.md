# ConsoleKit false-locality regression — November 5, 2026

State: source-confirmed correction under native test. Full GNOME session remains
open. This is a dependency port change, not systemd emulation.

CI26/27 stopped before opening a session because the activation probe interpreted
NoSessions as fatal. CI28 used correct activation and reached the real session
harness. Its IsLocal assertion failed: the backend reported true. Omitting the
explicit false input had not established non-local registration.

ConsoleKit2 2.0.0 verify_and_open_session_for_leader sets is_local=true when an
is-local key exists, without checking its boolean value. Its session collector
can also supply that key when a caller omits it. The small correction reads the
typed boolean. The regression explicitly supplies false, then requires IsLocal
false and VT zero before launching its child. This preserves the distinction
between a headless session and authority over a real console.

The sysutils/consolekit2 overlay uses the same complete pinned ports revision as
the Mutter experiment and increments the dependency port revision. Native CI
builds/stages/checks/packages it, preserves complete patched source before build
products are added, and supplies the exact modified port recipe and checksums.
Before the long Mutter rebuild it exercises real ConsoleKit creation, child
session UID/type/class discovery and authenticated BSD3 control as the existing
non-root test account. Original BSD3 code remains BSD-2-Clause; the modified
ConsoleKit dependency retains its GPL license.

This does not fix the separately documented TakeDevice inactivity reply, verify
real input/DRM state or prove VT recovery. The laptop remains untouched and no
beta3 or catalogue package has been published.
