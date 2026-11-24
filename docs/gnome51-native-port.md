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

The staged Shell 51.1 reported its version on a private D-Bus, served a GTK
Wayland window on a virtual monitor and exited normally on the HP. This used
Mutter 51.1, GJS 1.90.1, the GNOME 51 settings schemas and typings, and
GnomeQR 4.0. The graphics/gnome-qr overlay builds the QR library from the
pinned GNOME Desktop 51.0 distribution. This keeps the GNOME Desktop 3 ABI
used by installed applications including Evolution and Evince.

A second build used the pinned ports framework in a jail cloned from the HP's
15.2-STABLE boot environment. All eight packages passed the ports manifest
checks. After installation, pkg verified their dependencies and file checksums.
The installed Shell reported 51.1 on D-Bus, mapped a GTK Wayland window,
delivered five frame callbacks, and exited normally. A solver rehearsal using
a copy of the HP's package database selected seven upgrades and the new
gnome-qr package, with zero conflicts or removals. Hardware login, VT switching
and logout through the display manager still require a boot trial.

## Publishing the FreeBSD 15 amd64 GNOME 51 channel

The feature branch runs the native package and session checks. On main the GNOME
job builds from the checked-out revision and uploads directly from that builder.
There is no CircleCI artifact or workspace transfer for this job. Other build
workflows retain their own artifact settings.

The CircleCI context `ehws-freebsd-packages` must supply `AWS_ACCESS_KEY_ID`
and `AWS_SECRET_ACCESS_KEY` for the internal SeaweedFS package writer. Restrict
that context to the bsd-cubed main branch in CircleCI before enabling the job.
The S3 endpoint and public HTTPS reader are configured in
`tools/publish-gnome51.sh`. This follows the existing internal TLS repository
model; pkg catalogue signing is not configured for this channel.

Packages and corresponding sources are uploaded once per build beneath
`FreeBSD:15:amd64/bsd3-gnome51/snapshots/<commit>-<build>/`. The builder
queries that snapshot from an isolated pkg database and fetches Epiphany over
HTTPS for a byte comparison. After verifying that the commit is still the tip
of main, it publishes the channel metadata and compares the live package list
with the snapshot. Each catalogue references package paths unique to that build,
so clients with an older catalogue can still fetch their original packages.
The GNOME 49 and general latest channels use different prefixes.

After the first successful publication, a client can inspect the repository
with the following config in a temporary repos directory:

```ucl
bsd3_gnome51: {
  url: "https://packages.ehws.generic.business/${ABI}/bsd3-gnome51",
  enabled: yes
}
```

Use `pkg -o PKG_DBDIR=<temporary-db> -R <repos-dir> update -f`, then
`pkg -o PKG_DBDIR=<temporary-db> -R <repos-dir> rquery -U '%n %v'`.
