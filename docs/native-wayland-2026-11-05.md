# Native Wayland startup verified — November 5, 2026

State: verified native headless compositor milestone. The full GNOME desktop and
physical laptop acceptance remain open.

## What actually ran

A patched Mutter 47.10, matching the existing GNOME generation, compiled and
linked on a FreeBSD development VM. Running/installed kernel and
userland all reported 15.1-RELEASE-p6; compiler was FreeBSD clang19.1.7 and ABI
FreeBSD:15:amd64. The image alias `current` means the current image, not the
FreeBSD CURRENT source branch. This is not a 15-STABLE runtime test.

The source is the checksum-matched Mutter47.10 release plus the base port and
BSD3 overlay at ports2072800715389ae49066946c960e6521bac37e6a. The code changes
recorded with this milestone add the remaining scheduling/link fixes. The warm
build used a private prefix and release optimization; the clean ports build must
independently verify the package configuration and installation.

Hypothesis: after enabling the native backend and supplying its selected login
API, a native headless compositor can create a virtual monitor and serve a real
Wayland client without a display server underneath it. The bounded unprivileged
`tests/native-compositor.sh` started the selected binary with headless/Wayland,
no X11, software rendering and a1280x720 virtual monitor. Runtime, configuration
and cache directories were private; GSettings used its memory backend.

Observed:

- Loader inspection selected the private native Mutter/Cogl/Clutter libraries;
  GLib/GIO and libiconv remained ordinary shared system dependencies.
- Mutter reported a surfaceless renderer without GPU, added the virtual monitor
  and created the requested Wayland display.
- `wayland-info` connected successfully and reported wl_compositor6, xdg_wm_base6
  and wl_output4. Its current/preferred mode was1280x720 at60Hz, with matching
  logical dimensions.
- The test returned success and shut down its compositor. A separate shared
  consumer linked using ordinary pkg-config metadata and `-z defs`.

This validates startup and protocol discovery, not a mapped application window,
pixel presentation, real DRM/input devices, kernel revoke semantics, VT switching,
a GNOME Shell/session, or laptop readiness. Headless mode deliberately bypasses
session/device launching. Session-manager/locale/color-service warnings were
observed in this isolated test; they must not be erased from the evidence.

## Decisive fixes and failed paths

The native thread code used Linux RLIMIT_RTTIME. FreeBSD has no such limit. The
port now returns an explicit unsupported error for realtime promotion when the
watchdog is unavailable. It leaves the existing scheduling state unchanged;
fresh threads start with ordinary scheduling. No resource-limit alias or fake
realtime capability was added. FreeBSD thread IDs use pthread_getthreadid_np.

Meson's `static:true` dependency forced transitive static GLib dependencies into
Mutter's shared library. Linking failed on non-PIC libiconv archive objects.
BSD3 remains a PIC static archive, while the selected dependency uses normal
link resolution and its pkg-config file publicly supplies GIO. This embeds only
the small BSD3 archive and uses the normal shared GLib dependency graph. The
successful native library and runtime linkage confirm the distinction.

Copying source from macOS initially introduced AppleDouble `._*.wrap` files;
Meson tried to parse those binary metadata files as wrap definitions and failed
UTF-8 decoding. Removing generated metadata corrected it; future Mac tar transfers
must use COPYFILE_DISABLE=1. This was a transfer failure, not a Meson defect.
A hot update also temporarily omitted the base port's Meson adjustments; rebuilding
the full patch stack restored its libgudev version and udev-directory changes.

The first uninstalled launch could not find its default plugin. Installing into
the VM user's private prefix supplied it, but the installed executable initially
loaded the stock system Mutter and rejected `--headless`. Loader inspection proved
that mismatch. The private test needs both `$prefix/lib` and
`$prefix/lib/mutter-15` in LD_LIBRARY_PATH. No system library was replaced in this
VM. The package test instead installs the exact candidate into the disposable CI
VM before launching it.

The development VM retains source and dependencies between compiler trials. It
has an unprivileged build account and fixed provisioning; no new unrestricted
sudo grant. First-boot provisioning was observed to reboot before the final
p6 environment settled. Background dependency provisioning did not complete;
synchronous fixed provisioning completed and survived that reboot. This VM's
disk is ephemeral and is not an independent backup.

Private evidence preserves exact binaries, source-file SHA256 identities, build
and install logs, package/compiler versions, loader output and the client report.
Checksums establish identity, not correctness or an independent backup. Raw
captures remain outside Git. No package was installed on the laptop or published
to the custom catalogue at this milestone.
