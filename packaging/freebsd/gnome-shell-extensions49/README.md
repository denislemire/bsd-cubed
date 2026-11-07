# gnome-shell-extensions 49.1 candidate port

Overlay for `x11/gnome-shell-extensions`, based on FreeBSD ports commit
`2072800715389ae49066946c960e6521bac37e6a`:

https://github.com/freebsd/freebsd-ports/tree/2072800715389ae49066946c960e6521bac37e6a/x11/gnome-shell-extensions

Official supplemental release (not present in the pinned BuildStream manifest); checksum from the corresponding official GNOME `.sha256sum` file.

Source: https://download.gnome.org/sources/gnome-shell-extensions/49/gnome-shell-extensions-49.1.tar.xz

SHA256: `c9718bcdd9ceec26913268c93c349f717a9a39945cac043e5cd88935b16c0423`; size: 248340 bytes.

Requires actual GNOME Shell>=49. Preserves the FreeBSD exclusion of the system-monitor extension. Classic session files are disabled: the native BSD3 session graph has not qualified Classic mode, and installing an unsupported session would be misleading. Ordinary extension JS is packaged; none is automatically enabled. Removes the old Xorg-only post-install instructions. Source API compatibility and extension activation still need actual Shell49 tests.

Preserve all upstream consumer/dependency licenses and notices. This source is
not relicensed under the original BSD3 bridge license. PREFIX/DESTDIR and ports
package semantics remain intact.

Source checksums and applicable FreeBSD patches were checked against pristine
sources with fuzz disabled. Candidate plists retain the pinned inventory with
source-supported catalog/selection changes. Native configure/build/stage,
check-plist/QA/package, meaningful tests and ordinary-user runtime checks are
**pending**; no source-only recipe is claimed as a qualified desktop package.
