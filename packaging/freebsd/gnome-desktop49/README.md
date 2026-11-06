# GNOME 49's independently versioned desktop library

GNOME 49 uses **gnome-desktop 44.5**. The authoritative release manifest selects
that version, not a nonexistent gnome-desktop 49 tarball:

https://github.com/GNOME/gnome-build-meta/blob/75d7cf1166db3e0e938d2c12a3195b9e8d8f34fa/elements/core/gnome-desktop.bst

This is the FreeBSD `x11/gnome-desktop` recipe and portability patch
(with trailing whitespace removed from the Makefile) from ports commit `2072800715389ae49066946c960e6521bac37e6a`, retained here for a
reproducible ecosystem bundle. No runtime upgrade is needed when 44.5 is already
installed. It supplies both `gnome-desktop-3.0` and `gnome-desktop-4` APIs.

https://github.com/freebsd/freebsd-ports/tree/2072800715389ae49066946c960e6521bac37e6a/x11/gnome-desktop

https://download.gnome.org/sources/gnome-desktop/44/gnome-desktop-44.5.tar.xz

SHA256: `20e0995a6e3a03e8c1026c5a27bc3f45e69ffcc392ad743dcab6107a541d232f`.
The archive hash was checked against GNOME's manifest and pinned ports distinfo.
The FreeBSD patch was checked against pristine source without fuzz; a new native
package build has not been run for this directory. Existing upstream GPL/LGPL
licenses and notices remain in force; this library is not relicensed as BSD.

About's GNOME version comes from the control-center source version. Changing this
library's package version or branding would neither upgrade Settings nor honestly
represent GNOME compatibility.
