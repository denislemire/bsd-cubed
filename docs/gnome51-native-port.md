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

The opt-in CircleCI pipeline uses `build-gnome51-packages=true`. It checks the
source hashes, applies the consumer patches with zero fuzz, exercises the native
Session adapter against a private ConsoleKit fixture, and stages the full Session
build with the checkout's SDK. The package job also builds the initial GJS,
schemas, backgrounds and font viewer packages.

## Remaining package and boot work

1. Create the FreeBSD ports recipes and package manifests for Mutter 51, Shell 51
   and Session 51. Mutter and Shell now use ABI suffix `51`. Build them against
   the verified GNOME 51 schema and GJS packages. The pinned FreeBSD tree already
   has Wayland 1.26.0, wayland-protocols 1.49, libinput 1.31.1 and libei 1.6.0,
   which meet Mutter 51's declared minimums. Confirm the resolved versions in CI.
2. Qualify the Dinit graph end to end on a disposable VM. Confirm the chosen
   Settings Daemon components, readiness, Wayland and Xwayland application launch,
   idle and inhibit handling, and bounded logout. Test native DRM/input and VT
   takeover separately. Add adjacent GNOME packages after the desktop works.
3. Produce a signed package set, then clone the HP's working boot environment.
   Install into the mounted clone with `pkg -c`, refresh its schema cache,
   use a separate settings profile, and select the candidate for a one-time boot.
   The current environment remains the default until interactive acceptance.
