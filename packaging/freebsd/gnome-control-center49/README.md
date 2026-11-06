# GNOME Settings 49 consumer port

This is an overlay for `sysutils/gnome-control-center`, based on the FreeBSD
ports recipe at commit `2072800715389ae49066946c960e6521bac37e6a`. It builds
the genuine upstream 49.9 source; it does not change version labels in older
binaries. `meson.build` derives `MAJOR_VERSION` from the source version and
`panels/system/about/cc-system-details-window.c` displays that value in About.

Upstream selection and SHA256 come from GNOME's GNOME 49 BuildStream manifest,
commit `75d7cf1166db3e0e938d2c12a3195b9e8d8f34fa`:

- https://github.com/GNOME/gnome-build-meta/blob/75d7cf1166db3e0e938d2c12a3195b9e8d8f34fa/elements/core/gnome-control-center.bst
- https://download.gnome.org/sources/gnome-control-center/49/gnome-control-center-49.9.tar.xz
- https://github.com/freebsd/freebsd-ports/tree/2072800715389ae49066946c960e6521bac37e6a/sysutils/gnome-control-center

The source archive SHA256 is
`b7987aa4d6899584bbc9ff9a698d1adc1b557bcec09168ebebb624585f3cdf77`.

## Dependencies and supported scope

Install the matching BSD3 ecosystem overlay for settings-daemon 49.1 and desktop
schemas 49.1 first. The upstream minimums also include GTK >=4.15.2,
libadwaita >=1.8.alpha, Blueprint compiler >=0.17, Tecla >=47.0,
accountsservice >=23.11.69, UPower >=1.90.6 and GOA >=3.51.0. Blueprint compiler
is a new explicit build dependency; the pinned FreeBSD port supplies 0.22.2.
GNOME desktop library **44.5** is correct for GNOME 49, as recorded in the
adjacent `gnome-desktop49` recipe. These library and desktop versions are
independent.

The previous port's WAYLAND Meson option no longer exists upstream. Wayland
support is normal behavior; the optional X11 support now uses the upstream
`x11` option and is enabled by default here to preserve X11 application support.

The pinned FreeBSD patches were rebased onto the exact 49.9 source, retaining
ConsoleKit capability queries/inhibitors, FreeBSD OS information, printer paths,
Kerberos lookup, pkg-config placement, and existing power-profile limitations.
The About hostname editor is hidden because FreeBSD does not provide the
hostname1 mutation service; its template object remains present for bindings.

NetworkManager, Sharing and WWAN remain excluded on FreeBSD, as in the original
port. Bluetooth, Wacom and Thunderbolt retain upstream platform exclusions.
Wellbeing is additionally excluded on FreeBSD: the BSD3 ConsoleKit Shell backend
does not offer the screen-time history/limits APIs this panel needs. This does
not implement remote desktop, systemd unit control, fingerprint support or any
other missing service merely by compiling its client UI. Optional panel runtime
behavior still needs native validation. Airwave remains the separate Wi-Fi UI.

## Licensing and validation

This directory contains upstream consumer patches, not BSD-licensed replacement
GNOME implementations. Preserve upstream GPL/LGPL notices. The archive contains
GPL-2.0-or-later, GPL-3.0-or-later, LGPL-2.0-or-later (bundled libgxdp)
and LGPL-3.0-or-later sources; the recipe records
these terms separately. BSD3's original bridge remains BSD-2-Clause.

Source archive SHA256 and every patch were checked locally against pristine
49.9, without fuzz. The plist was rebased for the new global-shortcuts provider,
metainfo filename, translation languages and removed legacy faces. Native
configure/build/stage/check-plist/QA/package and runtime About/display/power tests
are **pending**; this is a candidate recipe, not a published or qualified package.

## Native VM checkpoint

FreeBSD15-STABLE compiled all616tasks. Native staging corrected24obsolete face
entries, retained the new49 images, added the GlobalShortcutsRebind interface
and exposed the provider dependency on libwayland-client. Manifest QA and a
repeated stage-QA now pass; the package was rebuilt with explicit dependencies.
Both real GNOME49 session modes opened the actual49.9 About UI on Wayland,
observed a committed surface, quit through the normal action, and then completed
ordinary session logout and cleanup. No laptop installation is claimed by these
VM results; the next published CI candidate remains required.
