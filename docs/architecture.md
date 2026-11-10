# Architecture and acceptance boundaries

## Decision: C11 and a small native bridge

C matches PAM, libdrm, libinput, libseat, GLib and the relevant upstream ABI. Rust
could help a larger state machine, but it adds toolchain and FFI work before this
prototype has a confirmed need for it. Keep that decision open if the device and
session ownership model grows substantially.

The long-term boundary is:

```
GNOME/Mutter clients
    | typed login1 D-Bus + selected sd-login ABI
    v
BSD³ compatibility layer
    | session discovery          | device ownership       | session startup
    v                            v                        v
ConsoleKit/PAM              native broker or libseat   per-user supervisor
    |                            |                        |
FreeBSD sessions            DRM/input/devfs/VT         ordinary processes
```

Session discovery, authenticated controller lifetime and experimental descriptor
transport/pause-resume protocol now exist. Real device revocation/VT switching
and session startup remain unverified. Reuse
ConsoleKit2 as the initial source of session truth; independently assess whether
its device controller works on the target build before selecting it for handoff.
Do not run ConsoleKit, seatd and BSD³ as competing DRM/VT owners.

## Milestones

1. **Read-only discovery (implemented, validation recorded separately):** typed
   ConsoleKit calls, selected sd-login functions, Manager/session D-Bus objects,
   isolated protocol tests, proper staged package layout.
2. **Seat/device lifecycle:** verify CanControlSession on FreeBSD, active-seat
   association, original caller UID/PID and controller identity. Implement
   TakeControl/TakeDevice/ReleaseDevice/ReleaseControl, descriptor transport,
   PauseDevice/PauseDeviceComplete/ResumeDevice and client-death cleanup. Test
   unauthorized callers, multiple controllers, logout, VT switch and daemon/backend
   restart. No success replies until actual ownership and revocation work.
3. **Native Mutter port:** enable the disabled native backend in an isolated ports
   build, resolve actual compile/runtime gaps in launcher, DRM, input, VT and
   udev compatibility. Preserve upstream licenses on any patches. Prove a native
   GNOME session on a VT, not a nested compositor inside Xorg.
4. **GNOME 49 startup:** audit the actual user-service graph and environment,
   dependency order, readiness, restart and shutdown semantics. Choose a narrow
   explicit supervisor integration/ports patch; evaluate existing BSD-capable
   supervisors before writing one. No general systemd unit parser or PID 1.
   GDM's userdb dependency is a separate boundary; first assess whether SDDM plus
   PAM is sufficient for login before taking on GDM.
5. **Desktop acceptance:** keyboard/mouse, accelerated display, lock/unlock,
   VT switch away/back, clean logout/login, device release on compositor death,
   multi-session rejection or correct support, and reboot recovery. Suspend and
   power actions remain separate tests with honest limits.

## Current protocol limits

The daemon owns login1 but intentionally exposes a small interface.
Selected power/inhibit methods are forwarded to ConsoleKit after the original
caller is shown to own a session. They are not success stubs, and they are not
verified hardware sleep. SuspendThenHibernate remains `na`.
It does not yet export User objects. Device descriptor transport,
pause/resume and Active change signals are experimental protocol support,
not verified hardware revocation.
TakeControl/ReleaseControl authenticate the original peer and isolate backend
lifetimes; see [controller design](controller-lifecycle.md).
ListSessions resolves local usernames where available. Session IDs are ConsoleKit
object-path components, not Linux session numbers. Missing backend is an error;
no sessions is a valid empty result. There is a three-second timeout per backend
call, not an aggregate request deadline. Calls are synchronous and not suitable
for production high-volume service. Session-list/property races return an error
rather than fabricating a consistent snapshot.

`sd_uid_get_sessions` implements the three active/online/closing filter modes.
`sd_uid_get_display` uses the oldest graphical `user` session, falling back to the
oldest open `user` session, and excludes greeters/closing sessions. Other new
systemd session classes, monitors, cgroups and user-unit queries are not supported.
Do not mistake this subset for full sd-login ABI/semantic compatibility.

## Privilege boundary to preserve

D-Bus authenticates the client's bus connection. A proxy calling ConsoleKit under
its own identity cannot claim that ConsoleKit authenticated the original caller.
Before control methods exist, specify caller credentials, ownership, per-client
backend connections or a native device broker, revocation and disconnect handling.
Never accept a client-supplied UID as authorization. Do not gain device access by
making all input devices world-readable or running the desktop as root.

## Development environment boundary — November 4, 2026

Denis clarified that this project is hardware-independent until final acceptance.
Use generic FreeBSD VMs in EhWS KubeVirt, preferably provisioned by the existing
CircleCI native machine executor. The physical laptop has parallel kernel work;
reserve it for final VT/DRM/input integration trials with a recorded recovery plan.
Early prototype validation on that laptop used only ordinary-user temporary files
and private buses, with no installed package, kernel, boot or service changes.
