# Seat VT-switch boundary — November 5, 2026

State: protocol implementation verified against a private backend; actual console
switching and hardware readiness remain open.

Mutter47.10 creates a login1 Seat proxy and calls SwitchTo for its VT keybinding.
The bridge previously had no Seat object. The candidate now exports this narrow
method and enumerates canonical seat names from actual session membership. It
encodes hyphens, underscores and leading digits for consumer-compatible D-Bus
components and de-duplicates seats shared by several sessions. Legacy CK object
labels such as Seat1 are resolved through GetName rather than used as seat IDs.

The privileged proxy cannot forward a seat method solely because a caller names
seat0. It obtains the original bus PID and UID, resolves current session
membership, requires that same connection to own its controller, checks the
pinned backend owner, queries active state and seat membership through that
controller's backend, and matches the canonical seat component. Zero is rejected;
ConsoleKit and the kernel supply platform-specific VT bounds. Inactive callers,
another connection and another seat fail before a backend SwitchTo call.

The fixture asserts that a successful switch uses the controller's dedicated
backend connection. It counts requests so denials cannot silently become root
backend operations. Tests cover absent/released control, a different connection,
changed original-session membership, inactive sessions, absent/invalid seats,
legacy/custom seat names, zero VT and a backend refusal. Independent introspection
checks cover duplicate seats, absent seats and component escaping.

The first native attempt incorrectly selected the subtree interface from callback
path text and returned UnknownMethod. Explicit registration data now identifies
Session versus Seat callbacks. Adding the new control test first also disturbed
an older test's absolute request counter; placing it after the existing lifecycle
groups preserved that baseline. These were test/dispatch corrections, not actual
VT failures.

Warm FreeBSD15.1-RELEASE-p6 ordinary-user testing passed sixteen applicable groups
and skipped the deliberate missing-PID group because the real bus supplies PIDs.
The macOS host passed eight applicable groups and skipped nine requiring native
peer credentials. No device, kernel, display-manager or boot configuration was
changed. A separate actual-input lease harness has compiled natively but has not
run; the known ConsoleKit TakeDevice inactivity-reply mismatch still needs real
backend validation/correction before hardware acceptance.

The earlier installed service test in CI25 independently passed root-only
ownership denial, onestart/quietstart/faststart/status/stop and foreign-owner
refusal. Its recorded build UID was zero, explaining CI22's mistaken ordinary-user
assumption. GNOME acceptance explicitly uses the existing non-root test
account and private test payload, without adding administrative grants.
