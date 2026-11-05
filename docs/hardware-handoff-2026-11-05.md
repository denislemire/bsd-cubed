# GNOME Wayland hardware handoff — November 5, 2026

State: preparation only; **not cleared for installation or login**. The matched
Wi-Fi VHT/HE pilot and subsequent IPv6 acceptance have priority. No laptop
connection, installation, reboot, service change or desktop workload was performed
while preparing this document. Keep implementation and GNOME evidence here.

## Candidate and evidence

Source: `dcf974485696e40381fa23a44db5a5d47d83ae3a`, signed/DCO,
[PR10](https://github.com/denislemire/bsd-cubed/pull/10), stacked on PR9.

[CI32](https://circle.ehws.generic.business/gh/denislemire/bsd-cubed/32) passed all
build/test/upload actions, including actual kernel input revocation, real VT
switch/pause/fresh-FD resume, full GNOME47 RequiredComponents, a mapped Wayland
window and clean logout. Overall status is **infrastructure_fail**, with a
terminal task-agent failure. Do not call this run green or release its packages.
[CI33](https://circle.ehws.generic.business/gh/denislemire/bsd-cubed/33) reruns the
same exact commit from the beginning. The unsupported legacy retry endpoint
returned404; the workflow v2 rerun endpoint accepted the rerun.

The quarantined CI32 binaries were downloaded and verified against all three
CI checksum manifests and exact-source provenance. This table identifies those
bytes for investigation, **not a permitted installation payload**. Successful
rerun hashes may differ and must replace this candidate identity before release.

| Artifact | Version | CI32 SHA256 |
| --- | --- | --- |
| bsd-cubed-0.1.0.b3.pkg | 0.1.0.b3 | edc52ee947e65da7c1e6213012535cf56ed869e96b39487d8d23fe3da90dea0f |
| consolekit2-2.0.0_5.pkg | 2.0.0_5 | 5f0824647018522aaaec1048e3689861e52c4e4eacf04faf6fb5d2b51b7d6ab0 |
| mutter-47.10_3.pkg | 47.10_3 | 3e109148330ceebb07dc8cb2e514ba023eaaa9363563e4db10eeed5ba26bef6b |

All three package manifests declare ABI **FreeBSD:15:amd64**
(arch freebsd:15:x86:64). CI ran kernel/installed/userland15.1-RELEASE-p6,
clang19.1.7. Separate stable/15 testing at84aa3cb3204d verified the production
bridge plus the corrected locally built CK5, actual VT lifecycle and full strict
headless GNOME47. That is not yet an exact-CI33-binary stable test, nor a physical
GPU test. Do not force this ABI onto the Wi-Fi trial's CURRENT kernel/STABLE
userland combination; boot a matching known STABLE kernel/module/userland set.

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
   Do not clone the mixed CURRENT pilot or copy its kernel/modules. Use
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
