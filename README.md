# BSD³ — BSD Desktop Daemon

A little daemon with no ambition to become PID 1.

I wanted GNOME on FreeBSD without dragging systemd along, so I built a small
C11 bridge (`bsddd`) from the login1 / `sd-login` bits GNOME actually calls
to ConsoleKit, PAM, devfs and rc. It is a subset, not a logind clone. There
is no systemd in this tree.

On my FreeBSD 15-STABLE laptop this turned into a usable GNOME Shell 49.10
Wayland session: login, apps, logout. The greeter is SDDM on labwc; Xwayland
is there for older apps. [v0.2.0-alpha3](https://github.com/denislemire/bsd-cubed/releases/tag/v0.2.0-alpha3)
is the package set I actually ran.

If it works on your machine and you like it, I am glad. If it breaks, you
get to keep both pieces. BSD-2-Clause; see [LICENSE](LICENSE) and
[THIRD_PARTY.md](THIRD_PARTY.md) for GNOME/GPL consumers.

Things that are still rude on my box: [console redraw after VT games](https://github.com/denislemire/bsd-cubed/issues/27),
Ctrl+Alt+Fn, sleep/unlock, camera, and a real Wayland GDM greeter. Wi-Fi
quality is a different story than the Airwave applet showing up.

A slightly more itemized list is in [docs/STATUS.md](docs/STATUS.md). How the
pieces fit is in [docs/architecture.md](docs/architecture.md). Dated files
under `docs/` are lab notes from along the way.

## Build

On FreeBSD, install deps as root, then build as yourself:

```sh
pkg install pkgconf glib dbus consolekit2
make
make test
./bsddd --probe
make DESTDIR=/tmp/bsd3-stage PREFIX=/usr/local install
```

`make test` uses a private D-Bus and a fake ConsoleKit. It never talks to the
system bus. `--probe` only reads whatever ConsoleKit is already doing; no
sessions is a valid answer. The install honors `DESTDIR`/`PREFIX` and ships a
disabled rc.d service. Enabling it is your call.

The public header is `<bsd3/sd-login.h>`, not `<systemd/sd-login.h>`. There is
no fake `libsystemd.pc`. Do not link two libraries that both export `sd_*`.

GNOME 49 dropped its built-in service manager, so session startup here is an
original launcher plus a private Dinit user graph. Overlays for Mutter, Shell,
Session and friends live under `packaging/freebsd/`. Versions sometimes look
weird on purpose: GNOME 49's manifest still pins `gnome-desktop` 44.5.

## What the daemon actually does

Selected `sd-login` calls over typed ConsoleKit D-Bus, plus a partial
`org.freedesktop.login1` Manager/Seat/Session: authenticated TakeControl,
device handoff, pause/resume, Seat.SwitchTo for the active controller. It
will not steal an existing bus name. KVM input revoke and VT switching work
here; the pretty pixels were on my laptop.

## Next for me

A greeter that is actually Wayland-native, and the remaining GNOME 49 bits
that still annoy me. Patches welcome. Please do not send machine-identifying
logs or credentials.
