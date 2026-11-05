# Controller lifecycle — PR milestone

This extends the read-only discovery prototype with TakeControl and ReleaseControl.
It deliberately does not open, transport or revoke device descriptors.

Authorization uses D-Bus GetConnectionUnixUser/GetConnectionUnixProcessID for the
original frontend unique sender, then ConsoleKit's session UID and PID-to-session
mapping. The peer must both own and belong to the requested session. Missing
credentials fail closed. Forced takeover is unsupported, even for root.

Every controller has a dedicated authenticated backend bus connection. That
connection's unique sender is what ConsoleKit owns and releases. Sharing the
daemon's normal backend connection would merge independent frontend lifetimes;
blindly forwarding as the daemon would incorrectly substitute its credentials.

Capture the ConsoleKit unique name before authorization, verify it is still the
owner after authorization, and address control calls to that unique name. If it
vanishes, calls cannot silently reach a replacement service. ConsoleKit name loss
invalidates tracked controllers. Frontend name loss closes only that controller's
backend connection; a failed explicit release also drops stale local ownership.

The boundary still needs real FreeBSD backend validation before production use.
Device handoff requires separate tests for ancillary descriptors, device limits,
pause/resume acknowledgement, revocation, VT switching, and backend/client death.
No bus policy or rc service is installed by this milestone.

The test bus uses an explicit private Unix-socket configuration so it does not
inherit a host's launchd configuration. macOS dbus-daemon cannot report peer PIDs;
there the authorization-dependent groups are skipped and a separate test verifies
that missing credentials produce AccessDenied rather than a UID-only fallback.
All credential-dependent tests must run on the FreeBSD VM executor.
