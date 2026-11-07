# evolution 3.58.3 candidate port

Overlay for `mail/evolution`, based on FreeBSD ports commit
`2072800715389ae49066946c960e6521bac37e6a`:

https://github.com/freebsd/freebsd-ports/tree/2072800715389ae49066946c960e6521bac37e6a/mail/evolution

Official supplemental release (not present in the pinned BuildStream manifest); checksum from the corresponding official GNOME `.sha256sum` file.

Source: https://download.gnome.org/sources/evolution/3.58/evolution-3.58.3.tar.xz

SHA256: `5d01b79a5bebdf2d3a9871b61350939cf2501f6ea03c241543f744a9286d4d79`; size: 13878984 bytes.

Official supplemental3.58.3 matches EDS3.58.3. Source requires EDS>=its own3.58.3 version; the recipe explicitly requires this dependency rather than accepting the old3.56 installation. Retains GTK3/WebKitGTK4.1 and the native iconv search patch, alongside all existing mail/calendar/backend options. Upgrade must qualify local mail/calendar access without altering accounts, sending mail or migrating live user data during tests.

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
