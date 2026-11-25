# What I have running

Updated 24 November 2026. Dated notes in this directory are older lab
scribbles. This page and the root README are the current story.

## GNOME 51 with GDM

The persistent default is now `hpbsd-gnome51-bsddd-829`. Package revision 8
runs the login1 broker as `_bsddd` (UID/GID 829). GDM login and manual
lock/unlock passed. VT return failed and the power capability query returned
AccessDenied. See the [service-account trial](bsddd-service-account.md).

On the preceding GDM environment, the physical laptop passed GDM password login, GNOME Wayland startup,
logout back to the greeter, console switching and password unlock after
idle. During the lock check, the Shell screensaver became active and the
ConsoleKit session reported `LockedHint=true`. The test restored the
15-minute idle timeout. See the [GNOME 51 patch guide](gnome51-patches.md).

## Earlier GNOME 51 package trials

The laptop is running FreeBSD 15.2-STABLE in the GNOME 51 trial boot environment. Files 51.1, Papers 51.1 and Sushi 51.2 are installed. Evolution and Evolution Data Server 3.62.1 are installed alongside GNOME Shell 51.1_1, Calendar 51.0_1 and Folks 0.15.12_1, rebuilt for the new shared libraries. Installed-file checks passed, and Files and Papers reported version 51.1. Interactive file previews, mail and the logout/login cycle still need desktop validation with this batch. Epiphany 51.1 and WebKitGTK 2.54.3 are now installed. With the FreeBSD IPC fix supplied to an isolated browser process, GTK 4.24.1_1 reported 495 DMA-BUF format/modifier combinations and WebKit painted the expected pixels on Intel ADL-N. WebKitGTK 2.54.3_1 is installed. Accelerated painting and WebGL readback passed without the IPC workaround. The GNOME 51 environment is selected for both the current and next boot. [Laptop deployment and verification](https://github.com/denislemire/hpbsd-lab/blob/d0c618f/docs/gnome51-laptop-2026-11-24.md).

The running BE has `bsd3-gnome51-desktop-51.1_2` installed from the
laptop candidate repository. Its 40 direct dependencies include Loupe,
Contacts, GNOME Keyring and the corrected IBus GTK 4 input method.
See the [architecture review and package priorities](gnome51-desktop-review.md).
The internal main-branch channel still awaits publication.

Contacts 51.0 and Keyring 51.1 are installed from the laptop candidate
repository. Contacts passed all 13 upstream core and vCard tests. A disposable
local Evolution address book passed create/read/delete. A fresh 51.1 Secret
Service process unlocked and retrieved a test item written by 48.0, using a
private bus and disposable home. This check exercised stored data compatibility;
PAM unlock on the next real login remains a separate check.

Loupe 51.0 rendered a PNG and a JPEG on the laptop. Both tests sampled the
displayed red and blue regions under Xwayland. The stock IBus 1.5.34_1
GTK 4 module recursively constructed a text view during input-context setup.
The first test exhausted memory and the kernel killed Loupe and a Firefox
process. IBus 1.5.34_2 gets styling from the existing client widget. With the
new module, the pixel checks passed under a virtual memory limit. A native
Wayland GTK 4 text view also constructed and accepted text with IBus selected.
The stock GTK 3 module source was left intact.

## GNOME 49 baseline

GNOME Shell 49.10 on Wayland, FreeBSD 15-STABLE. SDDM on labwc gets me in;
ordinary apps and logout work. Package set:
[v0.2.0-alpha3](https://github.com/denislemire/bsd-cubed/releases/tag/v0.2.0-alpha3).
Overlays under `packaging/freebsd/`. `make test` is a private-bus protocol
check, not a desktop.

## Still broken or unloved here

- Console does not redraw after some VT switches ([issue 27](https://github.com/denislemire/bsd-cubed/issues/27))
- Ctrl+Alt+Fn VT shortcuts
- Sleep/unlock and camera
- GDM as a Wayland greeter
- Pretending this is full logind (it is not; no fake `libsystemd.pc`)

Your hardware will invent new problems. That is fine.

GNOME 47 was the first thing that booted. GNOME 49 was the previous desktop. Some
libraries keep their own numbers (gnome-desktop 44.5 is a GNOME 49 pick, not
a leftover 47 port).
