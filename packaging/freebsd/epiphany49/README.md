# epiphany 49.9.1 candidate port

Overlay for `www/epiphany`, based on FreeBSD ports commit
`2072800715389ae49066946c960e6521bac37e6a`:

https://github.com/freebsd/freebsd-ports/tree/2072800715389ae49066946c960e6521bac37e6a/www/epiphany

Version and SHA256 selected by GNOME49 BuildStream commit `75d7cf1166db3e0e938d2c12a3195b9e8d8f34fa`.

Source: https://download.gnome.org/sources/epiphany/49/epiphany-49.9.1.tar.xz

SHA256: `0b6aafe1f51da155a19825135c8dc9955587c64f8dff3db8954e829a0f01006b`; size: 4241420 bytes.

Adds Blueprint compiler explicitly. Retains the FreeBSD sync-service portability patch, WebKitGTK6.0 dependency and inherited unit_tests=disabled/b_lundef=false recipe settings; these are not test/link validation claims. Source requires GTK>=4.13.3, Adwaita>=1.8.alpha and WebKitGTK>=2.43.4. The existing BSD3 hardware WebKit rendering environment remains a separate rollout concern and is not changed here.

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

The additional GPL consumer patch binds queued navigation-policy ownership to
actual matching-window removal. A fresh blank-page quit previously retained two
policy records and aborted at Shell finalization; the fixed clean package passes
normal quit, an internal overview control, a two-window check and both full
session modes with all15 app fixtures. Filter and form-warning policy remains
intact. Independent WebKit DOM/rendering verification passes, while these actual
browser window tests do not claim page-content execution coverage.
