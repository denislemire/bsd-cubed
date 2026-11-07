# gnome-font-viewer 49.0 candidate port

Overlay for `deskutils/gnome-font-viewer`, based on FreeBSD ports commit
`2072800715389ae49066946c960e6521bac37e6a`:

https://github.com/freebsd/freebsd-ports/tree/2072800715389ae49066946c960e6521bac37e6a/deskutils/gnome-font-viewer

The upstream version and SHA256 are selected by the pinned GNOME49 manifest:

https://github.com/GNOME/gnome-build-meta/blob/75d7cf1166db3e0e938d2c12a3195b9e8d8f34fa/elements/core/gnome-font-viewer.bst

Source: https://download.gnome.org/sources/gnome-font-viewer/49/gnome-font-viewer-49.0.tar.xz

SHA256: `7c018925c285771b55d7d1a6f15711c0c193d7450ed9871e20d44f2548562404`; size: 208816 bytes.

Retains native fontconfig, FreeType, HarfBuzz and FriBidi dependencies. Source requires GTK>=4.5 and Adwaita>=1.5. Font installation/thumbnail behavior still needs ordinary-user native validation.

Dependencies and source/patch changes retain their upstream licenses and notices;
this consumer is not relicensed under the BSD3 bridge's license. PREFIX/DESTDIR
and ports framework package manifests are preserved.

Native FreeBSD15-STABLE build/stage/check-plist/stage-qa/package passed in the
primary VM, followed by bounded ordinary-user Wayland launch checks (or actual
GdkPixbuf decoding for Backgrounds). See the [dated validation record](../../../docs/gnome49-app-overlay-audit-2026-11-06.md)
for upstream regression results, corrections and feature limits. Combined CI,
release publication and laptop installation remain pending.
