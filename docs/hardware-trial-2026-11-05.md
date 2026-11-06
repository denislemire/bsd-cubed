# First beta3 hardware trial — November 5, 2026 (America/Edmonton)

State: exact packages installed in an isolated clone; physical login pending.

The Wi-Fi coordinator explicitly released the laptop after its final normal-boot
and data/integrity evidence. The desktop experiment created a separate
stable-15-bsd3-beta3-20261105 clone from the verified HE-usable STABLE environment.
The original remains the persistent normal default. Its old CK/Mutter versions
were subsequently observed after recovery, independently confirming package
isolation. /etc, /usr/local, /var/db/pkg and /boot were verified in the clone's root
dataset; kernel/module and HE startup/helper hashes matched the parent.

A bounded sleep inhibitor was acquired and observed in ConsoleKit before the old
Wi-Fi inhibitor was stopped. A clone-only rc.subr service rearmed it after reboot.
No persistent power policy, network ownership, WPA/HE startup or PAM change was made.
The existing boot evidence collector remained enabled. An actual faststart of the
boot-return hook was observed alive from two connections before relying on it.

The first clone boot was verified and acknowledged. Exact re-downloaded GitHub
beta3 package hashes were checked again on the device, original CK/Mutter packages
backed up, then CK5, BSD3b3 and native Mutter3 installed without a repository or ABI
upgrade. A clean boot ensured the new backend was used. During extra diagnostics
the300-second boot-return timer expired once and safely rebooted the normal
parent. That is assisted recovery, not an unassisted boot pass. A repeated clone
boot used a concise immutable check of BE/kernel/HE identities, connected WPA,
v4/v6 routes and a live inhibitor before acknowledging the timer.

Kernel, split debug file, Wi-Fi and DRM binaries were preserved in protected
on-machine evidence. GNU Build ID9c115fccff86595ef9bca4895f7021e1f2281982 matched
between the kernel and its boot/kernel/kernel.debug. An earlier text-section
comparison was inappropriate because split debug text is NOBITS; the kernel also
had no GNU debuglink section. The older usr/lib/debug file was not selected. A
small diagnostic executable was compiled into executable temporary storage after
the evidence dataset correctly refused execution; its no-exec property was kept.

## Physical console recovery

The installed root-only guard passed its positive physical-console case. Its
negative deadline case returned child124 with recovery PASS and final
FAIL_OR_ASSISTED. All12 available consoles and original VT1 were read back.
The first real SDDM trial independently timed out after600 seconds, restored VT1,
and recorded recovery PASS with no additional descendant kills. Deadline recovery
is not a clean desktop logout or proof of recovery from a frozen kernel.

## Greeter and first login

The installed Breeze greeter showed only wallpaper; pointer wake and a direct
window capture confirmed the symptom. This did not establish a BSD3 session or
Mutter fault. A clone-only Theme.Current empty setting selected SDDM's embedded
stock greeter and visibly restored its login form at1920x1080. The normal parent
keeps its original theme and configuration.

The manual entry is named “GNOME Wayland — BSD³ beta3 trial”. It uses the installed
strict BSD3 SDDM adapter and the ordinary-user profile wrapper now preserved as
`tools/hardware/gnome-trial-profile.sh`. That wrapper creates a disposable XDG
profile, updates the private session bus environment and runs the installed
GNOME47 builtin RequiredComponents session with its physical renderer. No
headless Shell override or automatic login is used. The user's existing settings
remain separate. The deployed wrapper SHA256 is
7304d7a5fade324a5f59d9796bb3d21a859e292618933ca6c326e1ceb17d8887.

A second600-second trial has live native parent/controller markers, streamed
startup output, a bridge that owns login1, and a real Xorg greeter on VT9. It was
observed independently before asking Denis to select the labeled session and
enter his password locally. Actual GNOME display/input, regular-password
acceptance and session logout are still pending his attempt; no success is
inferred from the menu or screenshot.

The operational restart wrapper also needed an existing-service check: rc.subr
onestart returns nonzero for an already running bridge. The corrected restart
checks onestatus first. This changes no compiled runtime or published beta asset.
Raw captures, screenshots, configuration copies and matching binaries remain
protected on-machine; no identifying raw logs or authentication cookies are
included here. Only the experimental clone was modified.
