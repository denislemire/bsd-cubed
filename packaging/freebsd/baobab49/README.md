# baobab 49.1 candidate port

Overlay for `deskutils/baobab`, based on FreeBSD ports commit
`2072800715389ae49066946c960e6521bac37e6a`:

https://github.com/freebsd/freebsd-ports/tree/2072800715389ae49066946c960e6521bac37e6a/deskutils/baobab

The upstream version and SHA256 are selected by the pinned GNOME49 manifest:

https://github.com/GNOME/gnome-build-meta/blob/75d7cf1166db3e0e938d2c12a3195b9e8d8f34fa/elements/core/baobab.bst

Source: https://download.gnome.org/sources/baobab/49/baobab-49.1.tar.xz

SHA256: `6243c92002be7e91f5decd249612face2a4a12d3742afd88b086a94b875dffe0`; size: 611572 bytes.

Retains the FreeBSD post-patch removing the directory MIME association to avoid overriding the file manager. Source requires GTK>=4.15.1 and Adwaita>=1.6.alpha.

Dependencies and source/patch changes retain their upstream licenses and notices;
this consumer is not relicensed under the BSD3 bridge's license. PREFIX/DESTDIR
and ports framework package manifests are preserved.

Native FreeBSD15-STABLE build/stage/check-plist/stage-qa/package passed in the
primary VM, followed by bounded ordinary-user Wayland launch checks (or actual
GdkPixbuf decoding for Backgrounds). See the [dated validation record](../../../docs/gnome49-app-overlay-audit-2026-11-06.md)
for regression results, corrections and feature limits. Release publication
and hardware installation remain pending.
