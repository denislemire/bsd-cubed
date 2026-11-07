# tecla 49.0 candidate port

Overlay for `x11/tecla`, based on FreeBSD ports commit
`2072800715389ae49066946c960e6521bac37e6a`:

https://github.com/freebsd/freebsd-ports/tree/2072800715389ae49066946c960e6521bac37e6a/x11/tecla

The upstream version and SHA256 are selected by the pinned GNOME49 manifest:

https://github.com/GNOME/gnome-build-meta/blob/75d7cf1166db3e0e938d2c12a3195b9e8d8f34fa/elements/core/tecla.bst

Source: https://download.gnome.org/sources/tecla/49/tecla-49.0.tar.xz

SHA256: `2ca424e402baf60cd6b13777703b701ebb1faf8f3d0f2f971144d823f651249f`; size: 83664 bytes.

Retains the FreeBSD pkg-config installation path patch. Source supports GTK Wayland integration when available; this is an ordinary user keyboard-layout viewer, not a session manager or privileged input backend.

Dependencies and source/patch changes retain their upstream licenses and notices;
this consumer is not relicensed under the BSD3 bridge's license. PREFIX/DESTDIR
and ports framework package manifests are preserved.

Native FreeBSD15-STABLE build/stage/check-plist/stage-qa/package passed in the
primary VM, followed by bounded ordinary-user Wayland launch checks (or actual
GdkPixbuf decoding for Backgrounds). See the [dated validation record](../../../docs/gnome49-app-overlay-audit-2026-11-06.md)
for upstream regression results, corrections and feature limits. Release publication and hardware installation remain pending.
