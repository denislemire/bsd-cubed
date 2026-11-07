# gnome-calendar 49.1 candidate port

Overlay for `deskutils/gnome-calendar`, based on FreeBSD ports commit
`2072800715389ae49066946c960e6521bac37e6a`:

https://github.com/freebsd/freebsd-ports/tree/2072800715389ae49066946c960e6521bac37e6a/deskutils/gnome-calendar

Version and SHA256 selected by GNOME49 BuildStream commit `75d7cf1166db3e0e938d2c12a3195b9e8d8f34fa`.

Source: https://download.gnome.org/sources/gnome-calendar/49/gnome-calendar-49.1.tar.xz

SHA256: `0c1115ab294d532c4c19f56ce0d7aee53f8ea32b1430209dd1d9eda47a83d2c2`; size: 558240 bytes.

Adds Blueprint compiler explicitly. Source requires GTK>=4.15.2, Adwaita>=1.8.alpha, GLib>=2.80 and EDS/libedataserverui4>=3.45.1. Existing EDS3.56 can meet the API minimum when built with GTK4 UI; matching EDS3.58 is part of the coherent GNOME49 release bundle, not a fabricated Calendar minimum.

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
for actual app/test results, corrections and remaining limits. Combined CI,
release publication and hardware rollout are still pending.
