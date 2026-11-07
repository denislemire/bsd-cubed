# GNOME Session49 native consumer

The current candidate targets the official GNOME Session49.3 release tarball,
SHA256 `b424a90cfe51de4941b791a5102aeaadb2c62c185522a21f71cb485270053fe1`.
It follows GNOME49 BuildStream commit
`75d7cf1166db3e0e938d2c12a3195b9e8d8f34fa`. All native adapter hunks apply
with fuzz disabled; the only context rebase accounts for upstream removing the
unused JSON-GLib dependency. Native49.3 build/stage/plist/QA/package and full private-bus session graphs
now pass with GSD49.1, Settings49.9 and14 app fixtures, including normal
autostart/logout/cleanup. Combined CI and hardware49.3 qualification remain pending.
The previously accepted49.0 source and package remain preserved in alpha2.
Upstream consumer modifications retain GPL-2.0-or-later. The original
BSD3 backend/helper/graph sources remain BSD-2-Clause; their linked GNOME
executable remains GPL. No systemd implementation is incorporated.

The explicit Meson `bsd3` option selects the installed native SDK and
current-process ConsoleKit GsmSystem backend, original readiness/environment
channels, and omits Linux leader/init-worker/control tools and systemd units.
The normal upstream build is preserved when the option is absent. Native power
operations return NotSupported. The null backend exists only behind the
explicit diagnostic test define, never the production Meson option.

The recipe installs the native launcher as `gnome-session`; a display manager
must supply its authenticated native session and private bus. The BSD3 package
provides explicit Dinit manager/Shell/settings definitions, a foreground
service helper that authenticates the child PID owning each public bus name,
and the bounded real SessionManager.Initialized client. This does not create
a system daemon or replace PID1. Seven graph-owned settings autostart entries
are explicitly excluded from duplicate launches; ordinary application autostart
remains in the GNOME manager.

The original alpha2 graph targeted the actual installed GNOME Settings Daemon47.2_1
binaries. Public names and foreground startup ordering were audited from
upstream47.2 (archive SHA256
`1eb758862e888f55a0869193087f1cfbcc7a1163654e630099ff4342dd3f6a5a`).
That historical mixed-version qualification is preserved. The newer ecosystem
candidate has separately verified the same seven foreground services against
SettingsDaemon49.1; it does not claim every optional settings service.

Historical49.0 checkpoint: native SDK/backend/readiness tests passed on
FreeBSD15-STABLE. The full Meson consumer port passed native build/stage/manifest/QA/package.
The first actual settings graph rendered an autostart Wayland window and
logged out with all owners gone; a per-service teardown error triggered the
authenticated SessionOver correction and strict negative retest. Clean strict headless logout and real system-autostart tests now pass.
The narrow exact-ID Initialization metadata followup also passed the actual
native GsmApp API test. Its final consumer rebuild, combined CI qualification
and physical display-manager re-entry remain acceptance gates. GDM49 userdb and
physical49 desktop acceptance remain separate open work.
