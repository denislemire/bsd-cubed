# gnome-backgrounds 49.0 candidate port

Overlay for `x11-themes/gnome-backgrounds`, based on FreeBSD ports commit
`2072800715389ae49066946c960e6521bac37e6a`:

https://github.com/freebsd/freebsd-ports/tree/2072800715389ae49066946c960e6521bac37e6a/x11-themes/gnome-backgrounds

The upstream version and SHA256 are selected by the pinned GNOME49 manifest:

https://github.com/GNOME/gnome-build-meta/blob/75d7cf1166db3e0e938d2c12a3195b9e8d8f34fa/elements/core/gnome-backgrounds.bst

Source: https://download.gnome.org/sources/gnome-backgrounds/49/gnome-backgrounds-49.0.tar.xz

SHA256: `01fe260de58ba00e44aba589a998e33e8bc7aecc701df0bee64cf4cfba44f4b5`; size: 24440652 bytes.

Artwork-only package; native wallpaper decoding and presentation still require qualification. The previous47 port already used JPEG XL. No image conversion, glycin support or HDR capability is claimed.

Dependencies and source/patch changes retain their upstream licenses and notices;
this consumer is not relicensed under the BSD3 bridge's license. PREFIX/DESTDIR
and ports framework package manifests are preserved.

Native FreeBSD15-STABLE build/stage/check-plist/stage-qa/package passed in the
primary VM, followed by bounded ordinary-user Wayland launch checks (or actual
GdkPixbuf decoding for Backgrounds). See the [dated validation record](../../../docs/gnome49-app-overlay-audit-2026-11-06.md)
for upstream regression results, corrections and feature limits. Combined CI,
release publication and laptop installation remain pending.
