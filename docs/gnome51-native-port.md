# GNOME 51 native FreeBSD port

The source baseline is GNOME BuildStream manifest
`db69d872ba90a7bb96b53bbcc8a88ea8e9c6c9f0`. The checksums for Mutter 51.1,
Shell 51.1 and Session 51.0 are in
[`sources.sha256`](../packaging/freebsd/gnome51/sources.sha256).

| Patch | Purpose |
| --- | --- |
| `mutter-login.patch` | Link the BSD³ login SDK and retain the existing ConsoleKit-backed login1 path. Use on-demand Xwayland on FreeBSD and leave Linux cgroup attribution on Linux. |
| `mutter-freebsd.patch` | Apply after the login patch. Rebase devd device discovery, FreeBSD graphics and frame timing compatibility, filesystem Xwayland sockets, and BSD³ display environment publication. Keep the new explicit sync descriptor ownership in Mutter 51. |
| `shell-native.patch` | Use ConsoleKit and BSD³ readiness, keep the journal optional on FreeBSD, and decode backgrounds via GdkPixbuf. GNOME 51 moved image decoding from Mutter into Shell. Native ConsoleKit lacks the user-wide observation required by Shell's screen-time accounting. |
| `session-native.patch` | Run the BSD³/Dinit graph and use the ConsoleKit adapter with GNOME 51's abstract `GsmSystem` class. Keep supported session state and inhibitors, and explicitly reject unavailable privileged power operations. The existing GNOME 49 build still selects the interface ABI. |

The GNOME 51 topic branch runs this CircleCI pipeline automatically. Other branches can select it with `build-gnome51-packages=true`. It checks the
source hashes, applies the consumer patches with zero fuzz, exercises the native
Session adapter against a private ConsoleKit fixture, and stages the full Session
build with the checkout's SDK. The package job builds the bootstrap packages, then installs each candidate provider in build order for Mutter, Shell and Session. A complete package set still needs VM qualification.

## Remaining package and boot work

1. Verify ports stage QA, package dependencies and the Mutter and Shell ABI 51 manifests on the pinned FreeBSD builder.
2. Qualify the Dinit graph end to end on a disposable VM. Confirm the chosen
   Settings Daemon components, readiness, Wayland and Xwayland application launch,
   idle and inhibit handling, and bounded logout. Test native DRM/input and VT
   takeover separately. Add adjacent GNOME packages after the desktop works.
3. Produce a signed package set, then clone the HP's working boot environment.
   Install into the mounted clone with `pkg -c`, refresh its schema cache,
   use a separate settings profile, and select the candidate for a one-time boot.
   The current environment remains the default until interactive acceptance.
