# Source audit — November 4, 2026 (America/Edmonton)

## Symptom and hypothesis

Goal: native GNOME on Wayland on FreeBSD without systemd. Hypothesis: a small
logind compatibility service can satisfy seat/session requirements. Counterpoint:
newer GNOME startup and the native compositor backend are independent requirements.
Prediction: source inspection will show whether a login1 bridge alone is enough.

## Pinned authoritative inputs

| Source | Revision | Decisive observation |
| --- | --- | --- |
| [Mutter 49.0](https://github.com/GNOME/mutter/tree/7697a7993e92dcef3b39b67051691a8b28f0b5ce) | `7697a7993e92dcef3b39b67051691a8b28f0b5ce` | `src/backends/meta-launcher.c` uses sd-login and login1; native device pool requests descriptors through TakeDevice. |
| [GNOME Session 49.0](https://github.com/GNOME/gnome-session/tree/776dc42864ea233f732e0b5b72cc08b1d8664fdb) | `776dc42864ea233f732e0b5b72cc08b1d8664fdb` | `meson.build` requires libsystemd; built-in service manager is gone. |
| [FreeBSD ports](https://github.com/freebsd/freebsd-ports/tree/2072800715389ae49066946c960e6521bac37e6a/x11-wm/mutter) | `2072800715389ae49066946c960e6521bac37e6a` | Mutter 47.10_2 sets `-Dnative_backend=false` and `-Dsystemd=false`, even with WAYLAND enabled. |
| [ConsoleKit2](https://github.com/ConsoleKit2/ConsoleKit2/tree/40ccb602b8bcf0b118d1cda218561904b05d9f99) | `40ccb602b8bcf0b118d1cda218561904b05d9f99` | Session XML includes typed discovery, CanControlSession, device control and pause/resume interfaces. Existence of API does not prove installed backend capability. |

Additional primary references:

- [GNOME 49 distribution changes](https://release.gnome.org/49/developers/):
  gnome-session startup and GDM userdb dependencies.
- [FreeBSD desktop discussion, February 2025](https://lists.freebsd.org/archives/freebsd-desktop/2025-February/005469.html):
  historical gaps in GNOME/Wayland and ConsoleKit compatibility.
- [sd_uid_get_sessions contract](https://www.freedesktop.org/software/systemd/man/latest/sd_uid_get_sessions.html):
  tri-state active filtering and oldest graphical session selection. This is a
  moving reference; reviewed November 4, not a claim of implementing its whole ABI.
- [Turnstile](https://github.com/chimera-linux/turnstile): BSD-2-Clause session
  tracker worth evaluating before building session tracking from scratch; its
  advertised scope separates seat management from session tracking.

## Target baseline, read-only inspection

As an ordinary user over SSH: `freebsd-version -kru`, `uname -v`,
`pkg query '%n %v' ...`, process inspection and typed ConsoleKit calls.
Kernel/installed kernel/userland all report 15.2-PRERELEASE. The exact running
custom kernel source/configuration is not established by that version string.
No kernel, boot selection or desktop configuration was changed for this project.

Installed packages: gnome-session 47.0.1, gnome-shell 47.10, mutter 47.10_2,
consolekit2 2.0.0_2, seatd 0.9.3_1, glib 2.88.3,2, dbus 1.16.2_4,1.
Compiler: FreeBSD clang 21.1.8, llvmorg-21.1.8-0-g2078da43e25a.
The ports revision above is an independent current source inspection; the package's
exact historical ports commit has not been recovered.

A service-name probe misleadingly reported no consolekit script and couldn't read
seatd's PID file. Process inspection independently confirmed console-kit-daemon,
seatd and SDDM running. ConsoleKit GetSessions returned its typed NoSessions
error: no registered login session at that observation, not a missing service.

## Conclusion

A shim is a useful component, but it cannot alone enable native GNOME. The current
port excludes the native backend, and GNOME49 startup is another compatibility
surface. Keep these falsifiable milestones separate. Do not describe a successful
private-bus test as hardware or desktop validation.

## Current stable target and laptop login check

[GNOME's stable release notes](https://release.gnome.org/stable/) identify GNOME51;
49 is the historical startup-boundary audit, not the eventual current desktop
target. Mutter51.1 was fetched at
`9134e3a5869ce232bf95b77519005865747ba603` for follow-up. Its Meson setup requires
a logind provider, and native backend files use DRM/input/udev and Linux-adjacent
interfaces. This is a real compositor port in addition to the bridge, not a missing
session-menu entry. The attempted gnome-session51.1 tag did not exist; do not claim
that source revision was inspected or built.

The laptop's GNOME Shell47.10 help includes `--wayland` and `--nested`, but not the
native-only display-server/headless options. Its SDDM wayland-sessions directory
already contains gnome-wayland.desktop, gnome.desktop and classic variants.
Adding another .desktop file cannot enable the compiled-out native backend.

## November6 native49 user-session milestone

Denis selected GNOME49 as the next compatibility and laptop target. The
original BSD3/Dinit user-session graph and native ConsoleKit system adapter
now run the actual49 manager and Shell49.10 on15-STABLE, with seven actual
SettingsDaemon47.2_1 services, a real autostarted Wayland app, exact service
PID ownership and strict normal logout cleanup. See the
[dated candidate validation record](gnome49-native-candidate-2026-11-06.md).
These are headless VM results. A coherent native CI release and actual49
DRM/input/VT/login-manager/laptop acceptance remain distinct open gates.
The working physical47 installation and its kernel/Wi-Fi remain unchanged.
