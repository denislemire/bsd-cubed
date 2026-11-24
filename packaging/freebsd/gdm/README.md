# Native FreeBSD GDM47 experiment

This overlay targets GDM47.0 in the ports tree pinned by `PORTS_REVISION`.
Apply `native-port.patch` to x11/gdm/Makefile, copy `files/*` into its files
subdirectory, and build with the ordinary ports framework. Installation leaves
startup disabled. Existing working display managers must be stopped explicitly
before selecting GDM; the service refuses to compete with SDDM.

The original `gdm-freebsd-vt.[ch]` helper and rc.d script are BSD-2-Clause.
Patches to GDM are modifications of the GPL-2.0-or-later consumer. Its existing
copyright and license remain in force. Building a package does not make that
package, GNOME, or its dependencies BSD licensed. Distribute the complete matching
modified source and exact recipe alongside a binary release.

The helper uses FreeBSD sys/consio.h, sys/kbio.h, termios and controlling-terminal
interfaces. Its tests compile the actual helper against the native headers and
exercise both mocked failures and the disposable VM kernel. It contains no
systemd implementation. The seat lookup uses ConsoleKit's advertised ListSeats
name/path pair rather than assuming that a seat name is a D-Bus object path.

See [validation](../../../docs/native-gdm-2026-11-06.md) before interpreting this
as a deployable graphical greeter. No hardware GDM acceptance is claimed.

## GDM 51 source investigation (24 November 2026)

The GNOME 51 release manifest selects upstream GDM 51.0. The source archive
SHA-256 is `2ce5acf5a438b8f6dc1d14fd777a911be5eeb2f7ab9a7c4600166205b15d6536`.
On FreeBSD 15.2-STABLE with the BSD³ login SDK 0.2.0.a3, configure the
unmodified source with:

```sh
meson setup build gdm-51.0 -Dsystemd-journal=false \
  -Dsystemdsystemunitdir=no -Dsystemduserunitdir=no
```

Meson stops at line 75 looking for libsystemd. A fresh setup with
`-Dlogind-provider=elogind` stops at line 84 looking for libelogind. Those are
the two provider choices in the release. Disabling the journal and service
unit installation leaves the runtime seat/session dependency in place.

Start the modern patch set with a distinct BSD³ provider and a local Wayland
display. Three decisions in the source have observable greeter consequences:

* `daemon/gdm-local-display-factory.c` checks graphical seat capability before
  creating a display and queries the active session and VT when replacing a
  greeter. The current public BSD³ header exposes session-to-seat lookup. It
  still needs a verified way to answer those seat capability and active-VT
  queries. Reuse the tested FreeBSD VT backend from the earlier work where
  its semantics match.
* `common/gdm-common.c` enumerates sessions on a specific seat and checks the
  initiating service. Test with two sessions and a disappearing session.
  `libgdm/gdm-sessions.c` also checks a systemd user unit before falling back
  to display/session lookup; select the native lookup explicitly on FreeBSD.
* Review the session worker's PAM, VT handoff and teardown separately. Exercise
  a failed password, a successful login, logout returning to a fresh greeter,
  and recovery of the original VT if launching the compositor fails. Run this
  in a disposable VM before enabling GDM on a physical seat.

This investigation used only source configuration and call-site inspection.
The laptop's SDDM service and PAM configuration were left in place.
