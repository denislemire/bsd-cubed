# BSD³ — BSD Desktop Daemon

A little daemon with no ambition to become PID 1.

I wanted GNOME on FreeBSD without dragging systemd along, so I built a small
C11 bridge (`bsddd`) from the login1 / `sd-login` bits GNOME actually calls
to ConsoleKit, PAM, devfs and rc. It is a subset, not a logind clone. There
is no systemd in this tree.

GNOME Shell 51.1 and upstream GDM 51.0 run on my FreeBSD 15.2-STABLE
laptop. I can sign in to a Wayland desktop, switch to a text console and
back, let the desktop lock on idle, unlock it with my password, and log out
to the FreeBSD-branded greeter. This is now the default boot environment.
The earlier GNOME 49 and SDDM release is available as
[v0.2.0-alpha3](https://github.com/denislemire/bsd-cubed/releases/tag/v0.2.0-alpha3).

The [GNOME 51 patch guide](docs/gnome51-patches.md) maps the desktop and
login paths to their FreeBSD port recipes and tests.

If it works on your machine and you like it, I am glad. If it breaks, you
get to keep both pieces. BSD-2-Clause; see [LICENSE](LICENSE) and
[THIRD_PARTY.md](THIRD_PARTY.md) for GNOME/GPL consumers.

Suspend/resume with a locked session, camera support, and remote GDM login
still need separate qualification.

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

## Contributing

Patches welcome. Please keep machine-identifying logs and credentials private.
