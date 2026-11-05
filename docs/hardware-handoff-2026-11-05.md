# GNOME Wayland hardware handoff — November 5, 2026

State: preparation only; **not cleared for installation or login**. The Wi-Fi coordinator reports complete CURRENT IPv4/IPv6 VHT/HE cohorts
passed, evidence preservation is in progress, and a STABLE validation reboot
is next. No desktop window is open. No laptop
connection, installation, reboot, service change or desktop workload was performed
while preparing this document. Keep implementation and GNOME evidence here.

## Candidate and evidence

Source: `dcf974485696e40381fa23a44db5a5d47d83ae3a`, signed/DCO,
[PR10](https://github.com/denislemire/bsd-cubed/pull/10), stacked on PR9.

[CI33](https://circle.ehws.generic.business/gh/denislemire/bsd-cubed/33) is
**overall success** at that exact commit. Every build/test/upload step passed,
including real VT switching, input revoke/fresh-FD resume, full GNOME47
RequiredComponents, a mapped Wayland window and clean logout.
[CI32](https://circle.ehws.generic.business/gh/denislemire/bsd-cubed/32) passed its
individual actions but ended infrastructure_fail with a task-agent error. Its
artifacts remain quarantined; the table below supersedes the earlier CI32 table.
The v1 retry endpoint returned404; the v2 workflow rerun succeeded as CI33.

The CI33 binaries and complete corresponding sources/recipes were downloaded,
verified against all three CI manifests, and checked against exact-source
provenance. These are authenticated **CI assets**, not yet GitHub release assets.
There is no beta3 release URL to install from yet.

| CI33 artifact | Version | SHA256 |
| --- | --- | --- |
| bsd-cubed-0.1.0.b3.pkg | 0.1.0.b3 | 0dd5a5439adb901aa345f2732d7fbf675623ba849f6465e884d1cf53b0a86b80 |
| consolekit2-2.0.0_5.pkg | 2.0.0_5 | 5c907f809ec3eae7bc030b660b79a1d9719126ebe0b81912d49a78755caf29d5 |
| mutter-47.10_3.pkg | 47.10_3 | b63b4c436f8d8866437c1abdaf9cd4ac636d9c0265bd2b2e546579a3ebd1f4aa |

Within CI33's `freebsd` artifact directory, the binary paths are
`packages/All/bsd-cubed-0.1.0.b3.pkg`,
`consolekit/packages/All/consolekit2-2.0.0_5.pkg`, and
`mutter/packages/All/mutter-47.10_3.pkg`. The authenticated
[artifact inventory](https://circle.ehws.generic.business/api/v1.1/project/github/denislemire/bsd-cubed/33/artifacts)
provides their actual download locations. No credential-bearing URL is saved here.

Corresponding source/recipe SHA256 identities, using flattened filenames:

```text
f80f692a11beaa0a554be1547e582249458a54874f35faa105bb5c2e53153e31  bsd-cubed-0.1.0-beta3.tar.gz
a7bf264a8d026f3db37c5967d33246cdafda8abe054e0223425167b03f05beee  consolekit2-bsd3-source.tar.gz
ba232187ae5b3e5c7c43ac3b21101a98d78b4788fccc1c83ea6cd678e6569cc0  consolekit2-bsd3-port.tar.gz
4fae499b8897901ae6b0489fbddec8042cf2d701e64f15811688b24a04ecbf9f  mutter-47.10-bsd3-source.tar.gz
829e26ce2be5b16d49d15b51324a9eb6be1b257dee91bce976a344da150448c9  mutter-47.10-bsd3-port.tar.gz
```

All three package manifests declare ABI **FreeBSD:15:amd64**
(arch freebsd:15:x86:64). CI ran kernel/installed/userland15.1-RELEASE-p6,
clang19.1.7. The exact CI33 packages were subsequently installed from protected,
hash-checked copies in the isolated stable/15 KVM at84aa3cb3204d,
FreeBSD15.2-PRERELEASE for kernel/installed/userland. Strict real-session/control,
actual kernel inactive-input revocation, and the real active VT9 switch/pause/
fresh-FD resume/release test passed; VT1 restoration passed and launcher exit0
was captured. The strict full GNOME47 session/window/logout test then passed
with these same installed packages, including five mapped GTK frame ticks.
No laptop or physical GPU acceptance is established by those tests.

The Wi-Fi coordinator's fresh live check reports16.0-CURRENT for **all**
`freebsd-version -kru` entries in its current-16-he-lock-20261105 environment,
including userland. The earlier mixed CURRENT/STABLE description was incorrect.
Do not force ABI15 packages onto that CURRENT environment; use a matching known
STABLE kernel/module/userland set.

Dependency changes are narrowly scoped:

- Install BSD3 and its rc script/system-bus policy; disabled by default.
- Replace ConsoleKit2_2 with_5: typed locality, correct FreeBSD input revoke ABI,
  actual inactivity and ttyv0 console fallback. Original dependency retains GPL.
- Replace Mutter47.10_2 with_3: native backend and embedded BSD3 sd-login provider.
  Mutter retains GPL. Its package explicitly depends on BSD3 0.1.0.b3.
- Keep GNOME Shell47.10, session47.0.1 and settings-daemon47.2_1 as the tested
  baseline. The dated laptop inventory also matches the core GLib/dbus/gettext,
  GTK4, libinput, libei, libdisplay-info and XML library versions. Verify live
  dependencies in the actual trial BE; do not run an unrestricted pkg upgrade.

No systemd, kernel, DRM module, loader setting or network configuration change
is part of these three packages. Reviewed package scripts refresh ldconfig;
ConsoleKit also touches its history log. They do not start BSD3 or SDDM.
Logs and home directories may be shared between BEs and are not rolled back.

## Delivery gate before arranging the trial

Require overall-success native CI at the exact candidate commit, verified
package/source checksums, and exact-artifact validation on stable/15. Publish a
signed `v0.1.0-beta3` GitHub prerelease first, with the three binary packages,
matching BSD3 source, complete modified ConsoleKit/Mutter corresponding source,
exact port recipes, provenance and one unambiguous SHA256SUMS. Download the release
assets again and verify them. Record the actual published URLs and hashes here.
No beta3 release currently exists. Do not substitute earlier beta1, the failed
CI32 artifacts, or a locally built CK package. Catalogue publication remains
separate; do not update the shared package repository while this gate is open.

## Coordinated boot-environment preparation

The following is the agreed **future procedure**, not actions already performed.
The Wi-Fi coordinator must first finish its evidence barriers and IPv6 work,
release its inhibitor at handoff, identify the known STABLE base BE and persistent
normal fallback, and explicitly open the desktop test window. Historical BE
names are not authoritative. Recheck `bectl list` immediately before changes.

1. Preserve the normal fallback and record its R flag. Clone the coordinator's
   verified STABLE BE into a distinct `stable-15-bsd3-beta3-20261105` trial BE.
   Do not clone the CURRENT pilot or copy its kernel/modules. Use
   `bectl create -e VERIFIED_STABLE_BASE stable-15-bsd3-beta3-20261105` only after
   replacing the symbolic base with the verified actual name.
2. Mount only the new clone at a private mountpoint. Verify /etc, /usr/local,
   /var/db/pkg and /boot belong to its root dataset; stop if any are shared with
   the fallback. Save the clone's rc.conf, package/version inventory and SDDM/PAM
   configuration in protected evidence. Set only the clone's sddm_enable=NO and
   bsddd_enable=NO so the first trial boot stays at a text console. Preserve its
   network, SSH, power and Intel graphics settings.
3. Unmount it, use `bectl activate -t` for this trial only, confirm the persistent
   fallback remains selected, and reboot only in the arranged window. Verify
   `freebsd-version -kru`, kernel revision, loaded DRM modules, root dataset and
   `pkg config ABI`. All must match the selected STABLE environment and ABI15.
4. With no desktop session running, save restorable copies of existing Mutter
   and ConsoleKit packages using pkg create. Download **the published release**,
   check every SHA256 and package manifest, inspect dependencies, then install
   the three exact files in ConsoleKit→BSD3→Mutter order. Do not ignore ABI,
   dependencies or shared-library failures. No mass upgrade or force-ABI setting.
5. Reboot the trial BE once more using one-shot activation, with SDDM still
   disabled, to ensure no old ConsoleKit process or loaded Mutter library hides
   the installed version. The normal persistent BE remains unchanged. Capture
   the exact installed three-package versions and binary identities.

BE rollback is the main recovery route, not only package downgrade: it also
restores configuration, kernel/modules and package database together, provided
the verified dataset layout supports that. Shared /home, logs and runtime paths
are outside that guarantee. Do not modify the normal user's shared profile or
write permanent GNOME preferences during the smoke test; use a private test
profile or dedicated ordinary desktop test account established in the trial BE.

## Session-registration gate and bounded launch

The actual installed GNOME package contains
`/usr/local/share/wayland-sessions/gnome-wayland.desktop`, named “GNOME on Wayland”,
with Exec `/usr/local/bin/gnome-session --disable-acceleration-check`.
An X11 SDDM greeter can offer that Wayland session; no duplicate entry or greeter
conversion is needed. Preserve the existing X11 greeter.

However, the pinned FreeBSD SDDM0.21.0.36_7 port adds its ConsoleKit/dbus wrapper
only to Xsession. Its stock Wayland script directly executes the session, and
its PAM sample includes login. Stock VM login/system PAM supplies pam_xdg but no
pam_ck_connector. The laptop's actual PAM/Wayland script may differ and has not
been inspected during the active Wi-Fi window. The CI's explicit CK harness
does **not** prove this display-manager path. Existing ck-launch-session also
continues executing after a registration failure; do not treat it as a strict
readiness check or blindly substitute the test harness on hardware.

Before login, verify on the actual trial configuration that the Wayland child
gets a real local CK session on its actual controlling VT, with its own UID,
correct seat, session type=wayland, class=user, owned0700 runtime directory and
active state. If registration is missing, stop and implement/test a focused
bootstrap/PAM integration in this repository before presenting the option as
ready. Do not change global PAM or infer registration from a session menu entry.
Enabling login1 can also change SDDM's backend selection; SDDM integration itself
remains unverified. Unsupported Manager/session methods must not be stubbed.

After that gate and an agreed window, the first attempt should be one bounded
foreground SDDM invocation (180seconds, 10second forced-stop grace), supervised
by an immutable, root-owned local test runner detached from SSH. Do not enable
SDDM or BSD3 persistently. Start BSD3 via `service bsddd onestart`; its status
must confirm its own process owns login1. Do not restart the system bus under a
running session. Hold a bounded temporary sleep/idle inhibitor and verify it
through ConsoleKit ListInhibitors, then release it at handoff.

The runner/independent recovery guard must be tested in the VM before hardware:
verify live readiness while the producer is running, save exact scripts/hashes,
record child exit status, stop only owned desktop processes, restore saved VT
state, close the session and stop the manually started bridge. Allow a separate
longer outer deadline for cleanup and final snapshots; collect complete markers,
not merely command exit. Do not reuse the KVM-only VT fixture on hardware or
remove its hardware guard. Concrete launch/recovery scripts remain a required
pre-login deliverable; the prose above is not a verified recovery implementation.

Success requires visible GNOME, actual keyboard/pointer input, a Wayland client,
leaving/returning to its VT, clean logout, original console restoration and working
fallback. A timed termination/guard intervention is assisted recovery, not a
clean logout or reliability pass. Preserve failures and matching module/kernel
identity without blaming Wi-Fi unless evidence connects them.

## Owner-visible recovery

Keep an independent SSH/control connection and an agreed local operator present.
If the display is unusable but SSH works, the owned runner/guard stops its desktop
and records cleanup before reboot. Do not improvise a global kill or stop the
machine bus. If the kernel or access freezes, use the laptop's power control as
previously agreed, then boot the unchanged normal BE from the loader's Boot
Environments menu. Since the trial is one-shot, a normal reboot should return to
the persistent fallback; verify that R selection before every trial reboot.
Do not destroy the trial BE or replace its kernel before failure evidence is
preserved. No hardware run is authorized merely by this preparation document.
