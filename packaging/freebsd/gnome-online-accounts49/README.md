# gnome-online-accounts 3.56.5 candidate port

Overlay for `net/gnome-online-accounts`, based on FreeBSD ports commit
`2072800715389ae49066946c960e6521bac37e6a`:

https://github.com/freebsd/freebsd-ports/tree/2072800715389ae49066946c960e6521bac37e6a/net/gnome-online-accounts

Version and SHA256 selected by GNOME49 BuildStream commit `75d7cf1166db3e0e938d2c12a3195b9e8d8f34fa`.

Source: https://download.gnome.org/sources/gnome-online-accounts/3.56/gnome-online-accounts-3.56.5.tar.xz

SHA256: `23e03dcfa03950b60e9f892f7b729ee31deeb827c655df8f78b8bca5bc197734`; size: 496848 bytes.

Version3.56.5 is intentional GNOME49 numbering. Removes the deleted windows_live Meson option while retaining the pinned FreeBSD Kerberos/Fedora exclusion (Linux keyutils are unavailable), Exchange enabled, Microsoft Graph disabled and documentation disabled. OAuth/token storage/provider behavior is unqualified; no credentials or client configuration are changed.

Preserve all upstream consumer/dependency licenses and notices. This source is
not relicensed under the original BSD3 bridge license. PREFIX/DESTDIR and ports
package semantics remain intact.

Source checksums and applicable FreeBSD patches were checked against pristine
sources with fuzz disabled. Candidate plists retain the pinned inventory with
source-supported catalog/selection changes. Native configure/build/stage,
check-plist/QA/package, meaningful tests and ordinary-user runtime checks are
**pending**; no source-only recipe is claimed as a qualified desktop package.

Native checkpoint: build/stage/check-plist/stage-qa/package passed in the primary
generic FreeBSD15-STABLE VM. Removed stale MSN icons and the absent schema from
the packaging declarations after inspecting the actual upstream install. The
package is installed only in the VM. Account/provider/authentication behavior,
combined CI and release deployment remain unqualified.
