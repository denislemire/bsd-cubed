# evolution-data-server 3.58.3 candidate port

Overlay for `databases/evolution-data-server`, based on FreeBSD ports commit
`2072800715389ae49066946c960e6521bac37e6a`:

https://github.com/freebsd/freebsd-ports/tree/2072800715389ae49066946c960e6521bac37e6a/databases/evolution-data-server

Version and SHA256 selected by GNOME49 BuildStream commit `75d7cf1166db3e0e938d2c12a3195b9e8d8f34fa`.

Source: https://download.gnome.org/sources/evolution-data-server/3.58/evolution-data-server-3.58.3.tar.xz

SHA256: `da136d1190d9e8b09499452f6058e24eee6b77f441a8c769f74a52451bf379f1`; size: 5150224 bytes.

Version3.58.3 matches the GNOME49 manifest. Retains native UUID, pkg-config placement, SQL double-quoted-string compatibility and Microsoft auth portability patches. Corrects inherited ENBALE_GTK/ENABLED_INSTALLED_TESTS option spelling; build tests remain upstream-enabled while installed test payloads are disabled. Explicit systemd user-unit installation remains disabled. Keep both GTK3 and GTK4 UI APIs for Evolution and Calendar. BDB/LDAP/Kerberos/weather/OAuth options retain the pinned FreeBSD policy.

Preserve all upstream consumer/dependency licenses and notices. This source is
not relicensed under the original BSD3 bridge license. PREFIX/DESTDIR and ports
package semantics remain intact.

Source checksums and applicable FreeBSD patches were checked against pristine
sources with fuzz disabled. Candidate plists retain the pinned inventory with
source-supported catalog/selection changes. Native configure/build/stage,
check-plist/QA/package, meaningful tests and ordinary-user runtime checks are
**pending**; no source-only recipe is claimed as a qualified desktop package.
