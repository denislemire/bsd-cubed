# GNOME Session 49 consumer experiment

This patch applies to GNOME Session **49.0**, commit
`776dc42864ea233f732e0b5b72cc08b1d8664fdb`. It modifies upstream GPL-2.0-or-later
files and remains **GPL-2.0-or-later**, including the upstream attribution in
each affected file. The root BSD3 license does not relicense this patch or
the resulting GNOME executable. Source: <https://gitlab.gnome.org/GNOME/gnome-session/-/tree/49.0>.

The `HAVE_BSD3_USER_SESSION` conditional selects original BSD3 readiness and
environment APIs in the real manager and disables its systemd application-scope
hook. The diagnostic manager build selects the existing upstream null system
backend, which advertises no power-operation capabilities. Native shutdown
and reboot requests explicitly return the D-Bus NotSupported error;
initialization-time environment failures propagate to the requesting client.
This is not a
device/session-tracking implementation, production GNOME port, or GNOME 49
desktop package. The ordinary GNOME/systemd build is preserved when the new
conditional is absent.

`tools/test-gnome49-util.sh` builds and executes the actual upstream environment
consumer against a private Dinit instance. `tools/test-gnome49-manager.sh`
builds the actual upstream manager subset with real GLib/GNOME Desktop
dependencies on FreeBSD. Its private headless Kiosk graph starts the manager,
waits for its native readiness pipe, sends its real `Initialized` D-Bus method,
checks environment/error contracts and the running phase, rejects unsupported
power requests, requests logout and checks cleanup. The test does
not replace missing GNOME implementations with fake headers or generated
translations. It does not launch Shell, Mutter or a display manager.

The next production integration must implement and test the native GsmSystem
backend, GNOME Shell 49 notification and environment handoff, compositor/settings
service definitions and their readiness, app activation/lifecycle, and the
real display-manager entry/exit path. GDM 49 userdb remains separate work.
