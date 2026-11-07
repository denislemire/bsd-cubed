# gnome-calculator 49.2 candidate port

Overlay for `math/gnome-calculator`, based on FreeBSD ports commit
`2072800715389ae49066946c960e6521bac37e6a`:

https://github.com/freebsd/freebsd-ports/tree/2072800715389ae49066946c960e6521bac37e6a/math/gnome-calculator

The upstream version and SHA256 are selected by the pinned GNOME49 manifest:

https://github.com/GNOME/gnome-build-meta/blob/75d7cf1166db3e0e938d2c12a3195b9e8d8f34fa/elements/core/gnome-calculator.bst

Source: https://download.gnome.org/sources/gnome-calculator/49/gnome-calculator-49.2.tar.xz

SHA256: `ddf4cd2edda135c41c8af68f9c0cdcd9d9a9163cb9f7f8e28ca2c8e81fd87657`; size: 1258924 bytes.

Adds the explicit Blueprint compiler build dependency. Source requires GTK>=4.17, Adwaita>=1.8.alpha and GtkSourceView>=5.3. Existing MPC/MPFR, libsoup3, libgee and introspection dependencies are retained.

Dependencies and source/patch changes retain their upstream licenses and notices;
this consumer is not relicensed under the BSD3 bridge's license. PREFIX/DESTDIR
and ports framework package manifests are preserved.

Native FreeBSD15-STABLE build/stage/check-plist/stage-qa/package passed in the
primary VM, followed by bounded ordinary-user Wayland launch checks (or actual
GdkPixbuf decoding for Backgrounds). See the [dated validation record](../../../docs/gnome49-app-overlay-audit-2026-11-06.md)
for upstream regression results, corrections and feature limits. Combined CI,
release publication and laptop installation remain pending.

The native patches explicitly link libm and normalize incidental imaginary signed
zero for real root operands. Regression coverage includes positive/negative root
degrees, fractional powers and genuinely complex inputs. All seven upstream
suites pass; the new branch regression fails before the implementation fix.
