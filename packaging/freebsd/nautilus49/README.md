# nautilus 49.6 candidate port

Overlay for `x11-fm/nautilus`, based on FreeBSD ports commit
`2072800715389ae49066946c960e6521bac37e6a`:

https://github.com/freebsd/freebsd-ports/tree/2072800715389ae49066946c960e6521bac37e6a/x11-fm/nautilus

Version and SHA256 selected by GNOME49 BuildStream commit `75d7cf1166db3e0e938d2c12a3195b9e8d8f34fa`.

Source: https://download.gnome.org/sources/nautilus/49/nautilus-49.6.tar.xz

SHA256: `b68ab4185faa35e80d22825477d681a563a7606622eed24018b1bcbee83b26cc`; size: 3337508 bytes.

Adds explicit ICU library dependencies required by the49 source. GLib>=2.84, GTK>=4.17.5, desktop4, Tracker/TinySPARQL and the existing Localsearch/GVfs integration remain required. The inherited pinned recipe disables its test build with -Dtests=none; no test coverage is claimed here. Native file operations/search/extensions require qualification.

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
