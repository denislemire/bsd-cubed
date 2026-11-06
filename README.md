# BSD³ — BSD Desktop Daemon

A little daemon with no ambition to become PID 1.

BSD³ (`bsddd`; repository `bsd-cubed`) aims to bridge the desktop interfaces
GNOME expects to native FreeBSD services. Original project code is C11 under
the BSD-2-Clause license. FreeBSD keeps rc, PAM, devfs and its own kernel.

**November 6, 2026: the released alpha2 candidate runs GNOME Shell49.10 on
Wayland on a FreeBSD15-STABLE laptop. The owner confirmed input, apps, Airwave
and ordinary logout. The accepted environment is now the default boot target.**
The current greeter still uses SDDM/Xorg; Settings47 and SettingsDaemon47 remain
in that accepted package set. The next candidate upgrades the remaining GNOME49
components and apps. See the [hardware acceptance record](docs/gnome49-hardware-trial-2026-11-06.md)
and [alpha2 release](https://github.com/denislemire/bsd-cubed/releases/tag/v0.2.0-alpha2).
Wi-Fi performance, camera capture and physical VT shortcuts remain separate open
checks. These results establish the identified laptop trial, not every FreeBSD
machine or all GNOME features.

No systemd implementation or source is bundled. The daemon is a selected
session/device compatibility subset, not a complete logind replacement.

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
there is no D-Bus activation. Enable services only after qualifying the native package set and local session;
installation leaves startup disabled by default.
`bsddd --check-owner PID` verifies that the bus name belongs to that exact process;
it does not establish desktop readiness.

## First implemented boundary

- `libbsd3-login.a`: selected `sd-login` functions over typed ConsoleKit D-Bus
  calls, with negative errno returns and caller-owned strings.
- `bsddd --foreground`: partial `org.freedesktop.login1` Manager and Session objects,
  authenticated TakeControl/ReleaseControl, and isolated controller lifetimes.
  Experimental descriptor handoff and pause/resume protocol are implemented;
  Seat.SwitchTo requires the active original controller on the matching seat;
  real input revocation and console switching pass in KVM; physical GPU presentation is verified for the identified beta5 laptop only.
  Existing bus
  owners are never replaced.
- Tests for session selection/filtering, PID-zero caller identity, properties,
  backend failure, invalid identifiers, duplicate ownership and shutdown.

The header lives at `<bsd3/sd-login.h>` and is **not** installed as
`<systemd/sd-login.h>`. There is no fake `libsystemd.pc` claiming a compatibility
version. Consumer ports must explicitly opt in once the interfaces they use
are implemented. Linking both libraries that export the same `sd_*` symbols is
unsupported.

The published v0.1.0-beta1 remains the earlier read-only discovery preview.
Later native controller/session work is included in the alpha2 candidate.
Historical beta assets remain immutable; use a coherent tested release set.

## What a working desktop still requires

See the [guarded SDDM trial](docs/guarded-sddm-trial-2026-11-05.md),
[strict local bootstrap](docs/local-session-bootstrap-2026-11-05.md),
[gated hardware handoff](docs/hardware-handoff-2026-11-05.md),
[real VT/device regression](docs/native-vt-regression-2026-11-05.md),
[native Wayland startup milestone](docs/native-wayland-2026-11-05.md),
[native Mutter port experiment](docs/native-mutter-port.md),
[architecture and milestones](docs/architecture.md),
[source audit](docs/source-audit-2026-11-04.md), and
[ports/CI instructions](docs/ports-and-ci.md).

The first acceptance target is a native GNOME Wayland session on a real VT, with
keyboard/mouse, DRM access, VT switching and clean logout. The FreeBSD GNOME47 packages supplied the initial baseline. GNOME49 is the
current project target. Its removed built-in service manager is replaced by an
original BSD3 launcher and a private Dinit user-service graph. A login1 service
alone cannot supply session startup.

This repository is independent of the private laptop notebook. Public bug reports
and patches should stand alone and avoid machine-identifying logs or credentials.

The current physical rendering investigation is recorded in
[native frame presentation](docs/native-frame-presentation-2026-11-05.md).

Optional trial login-screen preferences are documented in
[greeter preferences](docs/greeter-preferences-2026-11-05.md).

Native boot configuration is documented in
[automatic desktop startup](docs/automatic-startup-2026-11-05.md).

A learning-oriented [architecture paper](docs/papers/bsd3-architecture/README.md)
explains the control/rendering paths and evidence boundaries.

Native GDM Wayland greeter and rc.d work is tracked in the
[November 6 VM investigation](docs/native-gdm-2026-11-06.md).
## GNOME49 ports and development

The [native session architecture](docs/gnome49-native-session-2026-11-06.md)
explains Dinit lifecycle, BSD3 APIs and licensing. The
[hardware trial record](docs/gnome49-hardware-trial-2026-11-06.md) preserves the
failed first49 attempts, their fixes and the accepted alpha2 desktop.
Earlier dated headless/component records retain their original validation limits.

Consumer overlays live under `packaging/freebsd/`: Mutter49, Shell49, Session49,
GJS, settings schemas, ConsoleKit and Dinit. New Settings49.9, SettingsDaemon49.1
and application overlays are being validated for the next candidate. Independent
libraries retain upstream version numbers: GNOME49's manifest selects
`gnome-desktop44.5`, for example. These overlays are not submissions accepted into
the official FreeBSD ports tree. Read each overlay's README before building.

Original project code is BSD-2-Clause. GNOME consumers, inherited FreeBSD patches,
ConsoleKit and Dinit retain their own licenses; see [third-party provenance](THIRD_PARTY.md).
GNOME executables containing GPL code remain GPL, even when linked with our
permissively licensed adapter. No GPL implementation is relicensed as BSD.

Current order of work:

1. Finish native validation, packages and publication for the remaining GNOME49
   components and apps, preserving the accepted desktop and recovery BE.
2. Prepare the repository and matching release for public publication.
3. Replace the Xorg-based greeter with a Wayland-native greeter, then remove the
   Xorg server dependency. Xwayland compatibility is a separate choice.

The generic FreeBSD VM and serial CircleCI executor are primary build/test targets.
Hardware installation uses exact published release assets after native validation.
