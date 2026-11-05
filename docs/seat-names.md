# Canonical ConsoleKit seat names

ConsoleKit2 source40ccb602b8bcf0b118d1cda218561904b05d9f99 separates object IDs
from seat names. ck-manager.c generates lowercase seatN IDs; ck-seat.c sets the
static seat's name to seat0. Earlier layouts used uppercase SeatN object labels.
The typed Seat.GetName API, available since1.1.1, supplies the canonical name.

The initial mock used a legacy path and treated its basename as the seat name.
Review against current implementation revealed this mismatch before native
compositor integration. The corrected fixture tests both path layouts with the
same canonical name, rather than merely changing uppercase to lowercase.

sd_session_get_seat now validates a direct ConsoleKit child object path, resolves
GetName, and returns a caller-owned canonical name. The special root path or an
empty name returns no-data. Unrelated/nested paths and unsafe names return a
protocol error; a backend denial remains an access error. No fallback fabricates
a seat from path spelling.

This is necessary for libinput assignment, but does not implement seat object
export, VT switching or multi-seat device policy. ConsoleKit's own dynamic-seat
mapping limitations still need separate validation.
