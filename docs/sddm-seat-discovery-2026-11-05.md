# SDDM seat discovery — November 5, 2026

State: real VM behavior verified; package CI for this change pending.

SDDM0.21.0.36_7 with the existing BSD3 login1 owner started but created no display.
Its pinned SeatManager source selects login1 when available and requests ListSeats,
then reads each seat's CanGraphical. The old bridge supplied neither; a working
session/device protocol alone was insufficient for display-manager startup.

The correction enumerates real ConsoleKit Manager.GetSeats independently of
GetSessions, resolves canonical names with Seat.GetName, de-duplicates identical
paths, rejects conflicting names and unsafe paths, and pins a backend owner for
each typed query. Manager.ListSeats/GetSeat and seat Id/CanTTY/CanGraphical are
read-only. CanTTY forwards CanActivateSessions; CanGraphical forwards the typed
backend property. Failures remain failures and false capabilities remain false.
No local always-true property or unsupported privilege-operation stub is added.

ConsoleKit2's current CanGraphical implementation is a coarse always-true hint.
Forwarding that backend hint does not prove DRM, pixels, input or compositor
readiness. The bridge deliberately does not reinterpret it as hardware acceptance.
Dynamic seat-add/remove relaying is outside this initial static-seat scope.

The private regression covers a real seat with zero sessions, canonical escaping,
de-duplication, false capability values, capability errors and invalid backend
paths. The first test failed because StartServiceByName can require an activation
file even if a nonactivatable mock already owns the name. Checking NameHasOwner
before activation fixed discovery without bypassing the private bus or changing
its assertions. Native FreeBSD passed17 applicable groups plus the deliberate
missing-credential skip out of18. macOS passed9 applicable groups and9 credential
skips. Final native CI is still required.

On the isolated stable/15 VM at84aa3cb3204d, a daemon with this correction returned
the actual seat0 before any user session. With a separately tested opt-in session
launcher, real SDDM autologin/PAM then started an ordinary Wayland child with
its own UID, actual VT, active seat and authenticated bridge control. A VESA Xorg
greeter was visually observed after logout. This validates the missing discovery
boundary; the launcher and bounded recovery are separately implemented changes,
and no physical hardware or complete logind compatibility is claimed.
