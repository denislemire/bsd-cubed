# gnome-system-monitor 49.1 candidate port

Overlay for `sysutils/gnome-system-monitor`, based on FreeBSD ports commit
`2072800715389ae49066946c960e6521bac37e6a`:

https://github.com/freebsd/freebsd-ports/tree/2072800715389ae49066946c960e6521bac37e6a/sysutils/gnome-system-monitor

The upstream version and SHA256 are selected by the pinned GNOME49 manifest:

https://github.com/GNOME/gnome-build-meta/blob/75d7cf1166db3e0e938d2c12a3195b9e8d8f34fa/elements/core/gnome-system-monitor.bst

Source: https://download.gnome.org/sources/gnome-system-monitor/49/gnome-system-monitor-49.1.tar.xz

SHA256: `915b6a321ada12eba7bf578c20c9fe5e41f55d532847cbd124bbddaaec11d70f`; size: 1105840 bytes.

Adds Catch2 as an explicit mandatory build dependency; upstream links its test binary with catch2-with-main. Retains -Dsystemd=false (no systemd requirement), FreeBSD NI_IDN fallback, network graph math compatibility and suppression of the duplicate KDE desktop launcher. The inherited NI_IDM macro typo is corrected to NI_IDN. Source requires libgtop>=2.41.2 (pinned2.41.3), glibmm>=2.68 (pinned2.86 via glibmm26), gtkmm4 and Adwaita>=1.6.alpha. Process enumeration, signals and affinity remain native runtime qualification items.

Dependencies and source/patch changes retain their upstream licenses and notices;
this consumer is not relicensed under the BSD3 bridge's license. PREFIX/DESTDIR
and ports framework package manifests are preserved.

Native FreeBSD15-STABLE build/stage/check-plist/stage-qa/package passed in the
primary VM, followed by bounded ordinary-user Wayland launch checks (or actual
GdkPixbuf decoding for Backgrounds). See the [dated validation record](../../../docs/gnome49-app-overlay-audit-2026-11-06.md)
for upstream regression results, corrections and feature limits. Combined CI,
release publication and laptop installation remain pending.
