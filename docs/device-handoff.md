# Device descriptor handoff and pause/resume protocol

This milestone proxies ConsoleKit device leases for the authenticated session
controller. It adds TakeDevice, ReleaseDevice, PauseDeviceComplete, unicast
PauseDevice/ResumeDevice, and Active property change notifications.

It does not prove FreeBSD DRM master/input revocation or native VT switching.
No production system-bus policy, startup service or laptop configuration is added.

## Ownership and descriptor boundary

Only the unique frontend connection that acquired TakeControl may use device
methods. Devices remain owned by ConsoleKit on the controller's dedicated backend
connection. Device IDs are typed unsigned major/minor numbers. TakeDevice
resolves them with devname(3) and forwards only DRM and evdev nodes; FreeBSD
majors are dynamic and are not an allowlist. The bridge does not open
client-provided paths or change devfs permissions.

TakeDevice accepts exactly one ancillary descriptor with handle0 and preserves
the backend's paused flag. Extra descriptors are rejected; the newly acquired
backend lease is released, or the whole controller is dropped if cleanup fails.
The bridge retains a bounded map of 256 leases, not a copy of the device FDs.
The client receives the actual Unix descriptor, not only a D-Bus handle number.

## State and cleanup

A taken device is active or paused. A normal pause sets awaiting-ack until the
controlling connection calls PauseDeviceComplete. A force pause needs no ack.
A gone notification removes the local lease. Resume requires a paused device,
no outstanding ack, and exactly one replacement descriptor.

Resume is unicast to the authenticated controller, never broadcast to observers.
ActiveChanged is translated to a typed PropertiesChanged notification for Active.
Invalid descriptor/signal ordering or release failure closes the dedicated
controller connection so ConsoleKit can revoke its leases. Timeouts/unusable
transport on acquisition also drop uncertain controller state.

GDBus message filters retain Unix FD ancillary data. They execute in a worker
thread, so retained messages/controllers are queued to the main context before
any state or hash-table mutation. Refcounts preserve queued lifetimes across
client death; backend identity is still pinned to the authenticated unique owner.

## Protocol tests and limits

New private-bus tests use real pipes with different take/resume markers. They
exercise descriptor data, paused acquisition, pause/ack/resume, forced pause,
gone, unauthorized/duplicate operations, observer exclusion, extra FD rejection,
release failure, client-death cleanup, the 256-lease bound and Active changes.
They do not simulate a kernel's real revoke/DRM ioctls. Native FreeBSD CI must
exercise all peer-PID-dependent groups; macOS skips them and verifies fail-closed
missing-credential behavior instead.

Hardware integration, aggregate deadlines/backpressure, controller-loss signaling,
seat/user objects, power/inhibitors, compositor porting and modern GNOME startup
remain separate work. Do not publish or install a production login service on the
strength of these mock-backend protocol tests.

Native FreeBSD VM CI build9 at fb36606bcd5cd5ac0fb9be9575f111f301f3b030
passed direct and ports-framework runs: eleven groups passed and the Mac-only
missing-PID group was skipped. This includes actual pipe take/resume markers,
acknowledgement/force/gone, observer exclusion, extra FD rejection, cleanup,
256-lease limits and Active notifications. Ports stage, check-plist, stage QA
and package creation passed. This does not establish kernel revocation behavior.
