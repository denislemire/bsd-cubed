# gnome-characters 49.1 candidate port

Overlay for `deskutils/gnome-characters`, based on FreeBSD ports commit
`2072800715389ae49066946c960e6521bac37e6a`:

https://github.com/freebsd/freebsd-ports/tree/2072800715389ae49066946c960e6521bac37e6a/deskutils/gnome-characters

The upstream version and SHA256 are selected by the pinned GNOME49 manifest:

https://github.com/GNOME/gnome-build-meta/blob/75d7cf1166db3e0e938d2c12a3195b9e8d8f34fa/elements/core/gnome-characters.bst

Source: https://download.gnome.org/sources/gnome-characters/49/gnome-characters-49.1.tar.xz

SHA256: `795c0fe434e602dc783391fb5830dbc7d8e1ea05ca77210fb140e2f6f40a145c`; size: 608800 bytes.

The JavaScript entry point selects GnomeDesktop4.0, so the recipe changes gnomedesktop3 to gnomedesktop4. Source now requires GLib>=2.83. Retains upstream BSD3CLAUSE licensing, distinct from the BSD3 bridge BSD2 license.

Dependencies and source/patch changes retain their upstream licenses and notices;
this consumer is not relicensed under the BSD3 bridge's license. PREFIX/DESTDIR
and ports framework package manifests are preserved.

Native FreeBSD15-STABLE build/stage/check-plist/stage-qa/package passed in the
primary VM, followed by bounded ordinary-user Wayland launch checks (or actual
GdkPixbuf decoding for Backgrounds). See the [dated validation record](../../../docs/gnome49-app-overlay-audit-2026-11-06.md)
for upstream regression results, corrections and feature limits. Combined CI,
release publication and laptop installation remain pending.
