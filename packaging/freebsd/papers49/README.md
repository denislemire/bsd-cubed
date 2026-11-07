# papers 49.8 candidate port

Overlay for `graphics/papers`, based on FreeBSD ports commit
`2072800715389ae49066946c960e6521bac37e6a`:

https://github.com/freebsd/freebsd-ports/tree/2072800715389ae49066946c960e6521bac37e6a/graphics/papers

Version and SHA256 selected by GNOME49 BuildStream commit `75d7cf1166db3e0e938d2c12a3195b9e8d8f34fa`.

Source: https://download.gnome.org/sources/papers/49/papers-49.8.tar.xz

SHA256: `1a9c426b0ead4779813f0e483a7f124f4b4f4264bc752bc426afb34c271072e6`; size: 4513828 bytes.

Adds Blueprint>=0.16 explicitly. Cargo metadata is regenerated from the49.8 source Cargo.lock: all253 registry archives were downloaded and checked against their locked SHA256, with exact sizes recorded; none use a floating Git source. Rust>=1.83 and Poppler>=25.01 are required. Retains upstream/default Meson tests and inherited CARGO_BUILD/INSTALL/TEST=no because Meson drives the Rust build. Existing format/backend options are retained. PDF rendering/printing/thumbnail/Nautilus integration remain unqualified.

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
