# Native Mutter49 — November6,2026

State: native15-STABLE package and standalone rendering verified; CI publication
and complete GNOME49 integration remain open. No laptop installation or reboot.
This builds on the separate JavaScript/schema dependency change, not a claim
that a session-manager prototype alone supplies a desktop.

## Source and license boundary

Mutter49.8 archive SHA256:
c8dec5b78c5ab5e5f77ea55aaa6eff125e0200d778fbc81345591463cb54db30.
It was verified against download.gnome.org's matching sha256sum file. Recipe
baseline: FreeBSD ports2072800715389ae49066946c960e6521bac37e6a.
The consolidated consumer patch retains applicable FreeBSD patches and the
upstream GPL/LGPL notices in corresponding source. It does not relicense Mutter
as BSD. CI exports complete matching modified source and exact recipes.

## Native port changes

The49 launcher and udev code moved into src/backends. Logind is an independent
build option; the FreeBSD recipe explicitly selects bsd3-login, keeps validated
DRM discovery and native buffer presentation, and does not fabricate libsystemd
metadata or Linux cgroup/user-unit identities. Missing native seat launcher fails
closed. Linux paths retain their upstream semantics. The package always builds
Wayland and Xwayland application support; a native Xorg desktop is not selected.
Its public ABI and manifest are Mutter17, not47's Mutter15.

The native udev tag API is the one supplied by FreeBSD's compatibility library;
no requirement is lowered while leaving a call to the unavailable current-tag
API. The directly linked libudev is explicitly declared. Runtime Python display
tools have versioned shebangs and declared bindings; optional completion/developer
tools are excluded from this runtime recipe.

Pango1.58 supplies its own renderer automatic-cleanup definition. The port
compiles a real feature probe and exposes the result through config.h.meson,
then avoids redefining it. An initial probe result was correct but absent from
the generated configuration template; that wiring error was corrected. A
nonportable uint spelling is replaced by unsigned int.

FreeBSD continues GdkPixbuf8-bit RGB/RGBA wallpaper loading and orientation,
rather than importing glycin's Linux seccomp/bubblewrap backend or claiming
that sandbox exists. HDR/color-metadata wallpaper support is not provided by
this fallback. A Linux-only sysfs backlight executable and its policy are not
installed on FreeBSD; native49 backlight support remains unported. These limits
must remain visible when describing the eventual beta.

Compositor display-environment publication calls have three-second deadlines.
When native service-graph membership is explicit, a missing session manager or
failed Wayland/X11 environment update aborts startup before Shell can advertise
readiness. Publishing only to D-Bus while leaving future native services without
WAYLAND_DISPLAY is not accepted as a successful graph startup.

## Validation

VM: FreeBSD15.2-PRERELEASE, GENERIC stable/15-n286624-646a485c3b2f,
clang21.1.8, GLib2.88.3. No DRM device is present. Native compile, stage,
check-plist, stage-QA and package passed. QA caught the direct libudev dependency,
changed17 ABI/layout and duplicate schema plist entries; all were corrected.
The identified package SHA256 is
2b0d8570b56c1c7f21ec38bd03ccc620426481ee23100ac7ac8dca8d1012bc5d.
This is a local native diagnostic artifact, not a published CI beta.

The previous47 packages were preserved with a checksum manifest before VM-only
installation. Actual49 source fixtures passed natively for canonical DRM nodes,
foreign/absent seats, Linux versus FreeBSD metadata and region/full-buffer swap
selection. The old47 fixtures initially failed to compile against49's changed
backend structure and region-argument signature; separate49 fixtures model the
new interfaces and still execute the actual source branches.

The actual FreeBSD wallpaper load function was compiled against real GdkPixbuf
and GTask. RGB and RGBA decoded pixels/alpha, cancellation, malformed input and
missing-file errors passed. This does not claim testing every image codec.

As ordinary UID1002 on a private session bus, the installed49 compositor created
a native headless Wayland socket, accepted wayland-info and a real GTK Wayland
window, then exited0 on requested termination. The test waits/reaps normal exit;
a deadline or forced kill fails. Existing desktop settings were isolated through
private runtime/profile directories and the memory settings backend.

This proves standalone rendering on the virtual software backend, not physical
GPU/input/VT takeover, full GNOME49 startup or user logout. Those remain required
with the production session graph and exact successful CI release artifacts.

## Continuous integration

The additional native job step runs after the existing47 regressions, so those
checks complete before its disposable machine is upgraded to the49 compositor.
It repeats port QA, actual-source fixtures and an ordinary-user Wayland/window/
normal-exit test, preserving corresponding source/recipes and diagnostic logs.
The standard executor is15.1-RELEASE; manual15-STABLE coverage above is distinct.
