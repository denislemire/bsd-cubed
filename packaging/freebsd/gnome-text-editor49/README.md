# gnome-text-editor 49.2 candidate port

Overlay for `editors/gnome-text-editor`, based on FreeBSD ports commit
`2072800715389ae49066946c960e6521bac37e6a`:

https://github.com/freebsd/freebsd-ports/tree/2072800715389ae49066946c960e6521bac37e6a/editors/gnome-text-editor

The upstream version and SHA256 are selected by the pinned GNOME49 manifest:

https://github.com/GNOME/gnome-build-meta/blob/75d7cf1166db3e0e938d2c12a3195b9e8d8f34fa/elements/core/gnome-text-editor.bst

Source: https://download.gnome.org/sources/gnome-text-editor/49/gnome-text-editor-49.2.tar.xz

SHA256: `f838356939203cbd88fbf4f65790734b8915f5fada320a7c6a252b1ed3eb89fa`; size: 699360 bytes.

Source requires GLib>=2.80, GTK>=4.19.4, Adwaita>=1.8.alpha, GtkSourceView>=5.15 and libspelling>=0.4. The pinned ports supply GtkSourceView5.18 and libspelling0.4.10. Editorconfig remains explicitly enabled upstream; no missing API check is bypassed.

Dependencies and source/patch changes retain their upstream licenses and notices;
this consumer is not relicensed under the BSD3 bridge's license. PREFIX/DESTDIR
and ports framework package manifests are preserved.

Native FreeBSD15-STABLE build/stage/check-plist/stage-qa/package passed in the
primary VM, followed by bounded ordinary-user Wayland launch checks (or actual
GdkPixbuf decoding for Backgrounds). See the [dated validation record](../../../docs/gnome49-app-overlay-audit-2026-11-06.md)
for upstream regression results, corrections and feature limits. Combined CI,
release publication and laptop installation remain pending.
