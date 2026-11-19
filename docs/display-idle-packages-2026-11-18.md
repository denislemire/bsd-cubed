# Package-owned display idle

Date: 18 November 2026. Candidate state: native CI and publication pending.

The HPBSD trial proved the complete path on the N150 laptop: a logged-in GNOME
49 Wayland desktop turned the panel backlight fully off after five idle minutes,
and the FreeBSD text console did the same. This change moves the desktop pieces
out of private trial files and into reproducible FreeBSD packages.

## Ownership

`bsd-cubed-0.2.0.a4` owns two policy pieces:

- The GNOME 49 Dinit service starts `gsd-power` with
  `BSD3_IDLE_DISPLAY=1`. This is deliberately opt-in; it cannot alter unrelated
  GNOME installations that use the same upstream binary. The same policy keeps
  the system `handle-lid-switch` inhibitor held and maps UPower lid close/open
  events to Mutter display off/on. Closing the lid therefore powers down the
  panel without requesting system suspend; opening it restores the panel.
- `/usr/local/libexec/bsd3-sddm-greeter-idle` runs the packaged `swayidle` and
  `wlopm` programs inside the labwc greeter compositor. The port declares both
  runtime dependencies instead of copying their binaries.

`gnome-settings-daemon-49.1_1` owns the small upstream consumer patch. When the
opt-in is set, its power plugin derives the display timeout from GNOME's
`org.gnome.desktop.session idle-delay`, observes active-session and inhibitor
changes, and uses Mutter's `PowerSaveMode` property for off and wake. Suspend and
system sleep remain separate policies. The host also sets
`hw.acpi.lid_switch_state=NONE`, so neither the kernel nor the opted-in desktop
policy treats lid close as a machine-sleep request.

The GSD patch remains under its upstream GPL/LGPL licensing. BSD3's service
descriptor and greeter wrapper are BSD-2-Clause. `swayidle` and `wlopm` remain
separate FreeBSD packages under their own licenses.

## CI and release artifacts

The focused native FreeBSD job runs `tools/ci-display-idle-packages.sh` before
the longer desktop qualification. It uses one pinned FreeBSD ports revision and
builds:

1. ConsoleKit 2.0.0_5 with BSD3's qualified native device/session semantics.
2. Dinit 0.19.4 from BSD3's reviewed consumer recipe (it is not in the stock
   FreeBSD ports tree).
3. BSD3 alpha4 from an explicit source snapshot.
4. GNOME 49.1 desktop schemas needed by the consumer build.
5. GNOME Settings Daemon 49.1_1 with the display-idle patch.

Every port runs `stage`, its test target, `check-plist`, `stage-qa`, and
`package`. CI installs the exact results, verifies package integrity, confirms
the BSD3 service opt-in and greeter helper, confirms the GSD binary contains the
opt-in path, and stores packages, corresponding source, port recipes,
provenance, dependency receipts, build logs, and `SHA256SUMS` under
`dist/display-idle/`.

The successful artifacts must be published together as a GitHub prerelease.
The laptop rollout must download and verify that exact release rather than copy
a workspace binary.

## Boundary with HPBSD

BSD3 now owns the GNOME session and greeter policy, but it does not own kernel
or DRM drivers. The console idle timer and DRM framebuffer DPMS bridge remain
HPBSD kernel/driver patches in `hpbsd-lab`. The currently promoted boot
environment already contains those verified pieces. Updating these packages
does not replace the kernel or DRM module.

Hardware evidence is intentionally split: physical logged-in GNOME and console
idle-off passed on 18 November; the greeter path passed with real headless
labwc/swayidle/wlopm and pointer wake, but the physical SDDM greeter observation
is still open.
