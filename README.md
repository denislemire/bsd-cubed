# BSD³ — BSD Desktop Daemon

A little daemon with no ambition to become PID 1.

BSD³ (`bsddd`; repository `bsd-cubed`) aims to bridge the desktop interfaces
GNOME expects to native FreeBSD services. Original project code is C11 under
the BSD-2-Clause license. FreeBSD keeps rc, PAM, devfs and its own kernel.

**Status: experimental session/device bridge. A patched Mutter47.10 has served
a Wayland client from its native headless backend on FreeBSD. Full GNOME47
headless session/window/logout passes native CI; real input revocation and VT
switching pass on a stable/15 KVM. Physical GPU presentation and laptop login
remain unverified.** No systemd implementation or source is bundled.
The daemon is not a complete logind replacement. Do not enable it as the machine's
login service or use its presence as proof of native compositor readiness.

## Build and test

On FreeBSD, as root install dependencies, then build as an ordinary user:

```sh
pkg install pkgconf glib dbus consolekit2
```

```sh
make
make test
./bsddd --probe
make DESTDIR=/tmp/bsd3-stage PREFIX=/usr/local install
```

`make test` creates a private D-Bus with a fake ConsoleKit server. It never
connects the prototype daemon to the real system bus. `--probe` only reads the
existing ConsoleKit service; no desktop session is a valid result. Dependencies
on ConsoleKit/GLib are runtime library or IPC dependencies with their own licenses;
BSD³'s license does not relicense those projects.

The install obeys `DESTDIR`, `PREFIX`, `CC`, `AR`, `CPPFLAGS`, `CFLAGS`, `LDFLAGS`,
and `PKG_CONFIG`. It supplies a binary, static development library, namespaced
header, explicit bsd3-login pkg-config metadata and manual page. The FreeBSD port
also installs a disabled rc.d service and a root-only login1 ownership policy;
there is no D-Bus activation. Keep the service disabled outside isolated testing.
`bsddd --check-owner PID` verifies that the bus name belongs to that exact process;
it does not establish desktop readiness.

## First implemented boundary

- `libbsd3-login.a`: selected `sd-login` functions over typed ConsoleKit D-Bus
  calls, with negative errno returns and caller-owned strings.
- `bsddd --foreground`: partial `org.freedesktop.login1` Manager and Session objects,
  authenticated TakeControl/ReleaseControl, and isolated controller lifetimes.
  Experimental descriptor handoff and pause/resume protocol are implemented;
  Seat.SwitchTo requires the active original controller on the matching seat;
  real input revocation and console switching pass in KVM; DRM and physical
  hardware behavior remain unverified. Existing bus
  owners are never replaced.
- Tests for session selection/filtering, PID-zero caller identity, properties,
  backend failure, invalid identifiers, duplicate ownership and shutdown.

The header lives at `<bsd3/sd-login.h>` and is **not** installed as
`<systemd/sd-login.h>`. There is no fake `libsystemd.pc` claiming a compatibility
version. Consumer ports must explicitly opt in once the interfaces they use
are implemented. Linking both libraries that export the same `sd_*` symbols is
unsupported.

The signed [v0.1.0-beta3 prerelease](https://github.com/denislemire/bsd-cubed/releases/tag/v0.1.0-beta3)
contains the exact CI36 package set and corresponding sources/recipes. Native CI
and independent stable/15 session/input/VT and SDDM/GNOME headless qualification
pass; physical display/input and password login remain unverified. The earlier
beta1 remains a read-only preview. Published beta assets are immutable.

## What a working desktop still requires

[guarded SDDM trial](docs/guarded-sddm-trial-2026-11-05.md),
[strict local bootstrap](docs/local-session-bootstrap-2026-11-05.md),
[gated hardware handoff](docs/hardware-handoff-2026-11-05.md),
[real VT/device regression](docs/native-vt-regression-2026-11-05.md),
[native Wayland startup milestone](docs/native-wayland-2026-11-05.md),
[native Mutter port experiment](docs/native-mutter-port.md),
[architecture and milestones](docs/architecture.md),
[source audit](docs/source-audit-2026-11-04.md), and
[ports/CI instructions](docs/ports-and-ci.md).

The first acceptance target is a native GNOME Wayland session on a real VT, with
keyboard/mouse, DRM access, VT switching and clean logout. The current FreeBSD
GNOME 47 packages are a baseline, not the final definition of modern GNOME.
GNOME 51 is the current stable desktop target. The initial GNOME 49 source audit
establishes where session startup changed; newer interfaces must also be ported
and validated. GNOME 49 and later need a user-session startup solution. A login1 service alone cannot supply that.

This repository is independent of the private laptop notebook. Public bug reports
and patches should stand alone and avoid machine-identifying logs or credentials.
