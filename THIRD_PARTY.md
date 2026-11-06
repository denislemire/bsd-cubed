# Licensing and source provenance

Original BSD³ C11 code, headers and original test/tool code use BSD-2-Clause,
except where a file explicitly identifies another license. The root LICENSE
covers that original work; it does not relicense dependency implementations or
upstream files changed by consumer patches.

| Component | Source and license boundary |
| --- | --- |
| GNOME Mutter, Shell, Session and apps | Upstream GNOME source plus explicitly maintained FreeBSD consumer patches. Upstream GPL/LGPL and asset licenses remain applicable. Exact versions, checksums, license declarations and patch provenance are in each `packaging/freebsd/*/` overlay. |
| GJS | Upstream mixed-license runtime with retained notices and small FreeBSD interoperability patches; see its overlay's license declaration and corresponding source. |
| ConsoleKit2 | Existing GPL-2.0-or-later session/device service with native FreeBSD control fixes. Its implementation is not BSD³-original or relicensed BSD. |
| Dinit0.19.4 | Separately packaged Apache-2.0 user-service supervisor. BSD³ supplies an original launcher/API and native service definitions, not a replacement PID1. |
| bsdisks0.41 | Existing BSD-3-Clause UDisks2 implementation. The swap-visibility consumer patch retains that license; its original regression fixture is BSD-2-Clause. |
| FreeBSD port recipes and patches | Derived from the immutable ports revision recorded in `PORTS_REVISION` and retained with their applicable attribution and licenses. These custom overlays are not official accepted FreeBSD ports submissions. |
| GLib, D-Bus, graphics/input libraries and other packages | Separately packaged dependencies with their respective upstream licenses. |

The project audit records no systemd implementation copied, translated, linked or
vendored into original BSD³ code. The selected `sd-login` API and login1 protocol
behavior are interoperability boundaries. We do not supply a fake libsystemd
pkg-config identity or claim the complete systemd/logind feature set.

Some regression fixtures read pinned upstream consumer code/XML at test time and
compile temporary bindings. Those inputs retain their licenses; generated or
extracted upstream implementation is not committed into BSD³-original files.

Each beta package set must ship matching patched consumer sources, build/port
recipes, SHA256SUMS and provenance. GPL/LGPL obligations apply to the applicable
consumer source/binaries; storing a patch beside BSD³ does not change them. The
original GNOME system adapter remains permissively licensed, while the resulting
linked GNOME Session executable retains GNOME's GPL obligations.

For exact release identities and validation limits, use the dated records in
`docs/` and the immutable release's source/recipe and provenance assets. Historical
source audits describe their dated state; later hardware acceptance is recorded
separately rather than rewriting failed investigations as successes.
