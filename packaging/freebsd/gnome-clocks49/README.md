# gnome-clocks 49.0 candidate port

Overlay for `deskutils/gnome-clocks`, based on FreeBSD ports commit
`2072800715389ae49066946c960e6521bac37e6a`:

https://github.com/freebsd/freebsd-ports/tree/2072800715389ae49066946c960e6521bac37e6a/deskutils/gnome-clocks

The upstream version and SHA256 are selected by the pinned GNOME49 manifest:

https://github.com/GNOME/gnome-build-meta/blob/75d7cf1166db3e0e938d2c12a3195b9e8d8f34fa/elements/core/gnome-clocks.bst

Source: https://download.gnome.org/sources/gnome-clocks/49/gnome-clocks-49.0.tar.xz

SHA256: `bf76915f2a492e8a0592fe40b35346593aa39e4e6881d6176e0efd8771d4e6fa`; size: 465244 bytes.

Changes the desktop dependency from gnome-desktop3 to gnome-desktop4, matching upstream49. Retains PORTEPOCH=1. Source requires GTK>=4.15.3, Adwaita>=1.6.alpha, gweather4, geoclue and the libsoup3-compatible geocode-glib2 backend. Native geolocation service behavior is not established by a build.

Dependencies and source/patch changes retain their upstream licenses and notices;
this consumer is not relicensed under the BSD3 bridge's license. PREFIX/DESTDIR
and ports framework package manifests are preserved.

Native FreeBSD15-STABLE build/stage/check-plist/stage-qa/package passed in the
primary VM, followed by bounded ordinary-user Wayland launch checks (or actual
GdkPixbuf decoding for Backgrounds). See the [dated validation record](../../../docs/gnome49-app-overlay-audit-2026-11-06.md)
for upstream regression results, corrections and feature limits. Release publication and hardware installation remain pending.
