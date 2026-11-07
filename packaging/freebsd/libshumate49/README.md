# GNOME 49's libshumate 1.5.5 candidate port

This overlay supplies `x11-toolkits/libshumate` 1.5.5, the exact library selected
by GNOME's pinned GNOME 49 manifest. Its version numbering is independent of the
desktop's release number. Maps 49.7 requires libshumate >=1.5.alpha; the pinned
FreeBSD port's 1.4.1 does not satisfy that requirement. No consumer check is
removed or lowered.

Manifest authority:
https://github.com/GNOME/gnome-build-meta/blob/75d7cf1166db3e0e938d2c12a3195b9e8d8f34fa/elements/core-deps/libshumate.bst

Base FreeBSD recipe:
https://github.com/freebsd/freebsd-ports/tree/2072800715389ae49066946c960e6521bac37e6a/x11-toolkits/libshumate

Upstream source:
https://download.gnome.org/sources/libshumate/1.5/libshumate-1.5.5.tar.xz

SHA256: `ce27d88537a37cd5c45959c3bb7cc18c1c18b3d2e663d02bcf46fc66f7414a92`.
Size: 5914036 bytes. Archive SHA256 was verified against the manifest.

## Dependency and license boundary

The existing FreeBSD recipe carries no source patch files; none are invented
here. Its gperf, protobuf-c, JSON-GLib, libsoup3, SQLite, GTK4, Cairo,
introspection and Vala dependencies remain explicit. The source requires
GLib >=2.74 and libsoup >=3.0. The vector renderer is explicitly enabled to match
the manifest; it is not replaced with raster-only behavior to bypass APIs.
The existing sysprof exclusion and optional documentation behavior are retained.

The library's source license is LGPL-2.1-or-later. Preserve its notices and
dependency licenses. It is a normal installed library, not code copied into the
BSD-2-Clause BSD3 bridge. Source inspection found no new unconditional Linux
syscall/header dependency; that observation does not establish a native build.

## Qualification still required

The candidate plist starts from the pinned recipe, with source-declared public
headers and translation catalogs updated. It has **not** passed native staging
or check-plist. Optional documentation inventory also needs native validation
when DOCS is enabled. PREFIX/DESTDIR and ports framework dependency handling
are preserved.

Native configure/build/stage/check-plist/QA/package, actual Shumate GIR/VAPI
installation, and upstream tests are pending. Upstream includes coordinate,
file-cache, map, marker, viewport and vector expression/index/reader/renderer/
sprite/style tests; they remain enabled. GTK tests need an appropriate isolated
display fixture. After qualifying this library, Maps still needs its own native
launch, tile rendering and ordinary-user integration tests. No geolocation,
network service or desktop-readiness capability is inferred from this recipe.

No build, VM, root/laptop, signing, push or deployment action was performed
for this source-preparation task.

## Native checkpoint

Native build/stage/check-plist/QA/package passed on the primary generic
FreeBSD15-STABLE VM with DOCS and vector rendering enabled. The plist was
reconciled with actual staged documentation. All16 upstream suites passed under
an isolated headless Mutter49 Wayland display, including map/marker and vector
suites. The package is installed only in that VM. Maps runtime and package/
release qualification remain pending; see the dated second-batch audit above.
