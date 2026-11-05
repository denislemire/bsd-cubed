# ConsoleKit device-state mismatch — November 4, 2026

State: source-confirmed integration issue; no backend fix or device runtime test
has been applied yet. Keep the native Mutter build as the active priority.

The bridge tests deliberately implement the documented ConsoleKit TakeDevice
contract: the boolean output is `inactive`, matching login1. A source audit of the
actual backend found that this assumption does not match ConsoleKit 2.0.0's
implementation. The release archive matches the pinned FreeBSD port's distinfo:

- Archive SHA256: `f3f4a424f2a4fbb67696b4011caf2fd4c02e5e67c24ef1c86cecb2301bccdba4`.
- `src/org.freedesktop.ConsoleKit.Session.xml` declares `inactive` and documents
  a true value for an inactive device.
- `src/ck-session.c`, dbus_take_device, completes the call with
  `console_kit_session_get_active(object)` without negation.
- The existing FreeBSD patch75867d5ec7187b60e5732843acbe0f67c61421cf only changes
  suspend support; its SHA256 matches distinfo and it does not fix this reply.

Prediction: a real active-session device request can be reported to Mutter as
inactive even when descriptor acquisition succeeds. A native device test must
observe both active and inactive sessions and verify the actual descriptor/device
state, not merely repeat the XML's intended semantics in a fake backend.

The narrow candidate correction belongs in ConsoleKit's implementation/port,
with appropriate upstream duplicate/contribution review, rather than a blind
inversion inside BSD3 that would break a backend implementing the documented
contract. This does not invalidate descriptor transport and ownership tests; it
limits their evidence to the tested protocol contract and simulated backend.

No working hardware handoff is claimed. The mock's passing tests cannot establish
that an unmodified ConsoleKit package returns the correct inactivity value.
