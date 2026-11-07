# gnome-maps 49.7 candidate port

Overlay for `deskutils/gnome-maps`, based on FreeBSD ports commit
`2072800715389ae49066946c960e6521bac37e6a`:

https://github.com/freebsd/freebsd-ports/tree/2072800715389ae49066946c960e6521bac37e6a/deskutils/gnome-maps

Version and SHA256 selected by GNOME49 BuildStream commit `75d7cf1166db3e0e938d2c12a3195b9e8d8f34fa`.

Source: https://download.gnome.org/sources/gnome-maps/49/gnome-maps-49.7.tar.xz

SHA256: `e6861ce65470fc8e1b933dafd71690feeccb84261ed7131d2175f8e917312815`; size: 1235136 bytes.

BLOCKED dependency: source requires libshumate>=1.5.alpha; pinned FreeBSD has1.4.1. The recipe explicitly requires the newer libshumate package and leaves upstream checks intact. No dependency bypass is supplied. Retains FreeBSD GIR/typelib placement. Map tiles, geolocation, routes and online services require separate native runtime/network validation.

Preserve all upstream consumer/dependency licenses and notices. This source is
not relicensed under the original BSD3 bridge license. PREFIX/DESTDIR and ports
package semantics remain intact.

Source checksums and applicable FreeBSD patches were checked against pristine
sources with fuzz disabled. Candidate plists retain the pinned inventory with
source-supported catalog/selection changes. Native configure/build/stage,
check-plist/QA/package, meaningful tests and ordinary-user runtime checks are
**pending**; no source-only recipe is claimed as a qualified desktop package.

Native checkpoint: build/stage/check-plist/stage-qa/package passed in the
primary generic FreeBSD15-STABLE VM. See the [dated second-batch record](../../../docs/gnome49-app-overlay-batch2-2026-11-06.md)
for app/test results, corrections and remaining limits. release publication and hardware rollout are still pending.
