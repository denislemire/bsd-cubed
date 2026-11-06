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
