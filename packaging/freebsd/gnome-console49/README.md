# gnome-console 49.2 candidate port

Overlay for `x11/gnome-console`, based on FreeBSD ports commit
`2072800715389ae49066946c960e6521bac37e6a`:

https://github.com/freebsd/freebsd-ports/tree/2072800715389ae49066946c960e6521bac37e6a/x11/gnome-console

Version and SHA256 selected by GNOME49 BuildStream commit `75d7cf1166db3e0e938d2c12a3195b9e8d8f34fa`.

Source: https://download.gnome.org/sources/gnome-console/49/gnome-console-49.2.tar.xz

SHA256: `27b02ce8e890ff8f53ca85db786b1ae35903f0b6327812f0224dc0b54b1088db`; size: 257828 bytes.

Upstream49 now returns NULL from kgx_pages_get_children on an invalid object, correcting the old pointer/integer issue. The former FreeBSD patch removed that validation entirely; it is obsolete and intentionally not carried. Source requires VTE GTK4>=0.77, GTK>=4.19, Adwaita>=1.8 and GLib>=2.80. PTY/session/paste behavior and child cleanup require native ordinary-user tests.

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
