# GNOME Session49 native consumer

The patch targets GNOME Session49.0, commit
`776dc42864ea233f732e0b5b72cc08b1d8664fdb` and official release tarball
SHA256 `5a75622e63d1b75145c9ebe67e828367a75f5ace4b6237f64e54661e6a545ef3`.
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

The graph currently targets the actual installed GNOME Settings Daemon47.2_1
binaries. Public names and foreground startup ordering were audited from
upstream47.2 (archive SHA256
`1eb758862e888f55a0869193087f1cfbcc7a1163654e630099ff4342dd3f6a5a`).
That is mixed-version compatibility to test, not a claim of all49 components.

Native SDK/backend/readiness tests have passed on FreeBSD15-STABLE. The new
full Meson consumer port, actual settings graph, rendering, normal logout and
display-manager re-entry still require native qualification. GDM49 userdb and
physical49 desktop acceptance remain separate open work.
