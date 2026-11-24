# What I have running

Updated 24 November 2026. Dated notes in this directory are older lab
scribbles. This page and the root README are the current story.

## GNOME 51 trial

The laptop is running FreeBSD 15.2-STABLE in the GNOME 51 trial boot environment. Files 51.1, Papers 51.1 and Sushi 51.2 are installed. Evolution and Evolution Data Server 3.62.1 are installed alongside GNOME Shell 51.1_1, Calendar 51.0_1 and Folks 0.15.12_1, rebuilt for the new shared libraries. Installed-file checks passed, and Files and Papers reported version 51.1. Interactive file previews, mail and the logout/login cycle still need desktop validation with this batch. Epiphany 51.1 and WebKitGTK 2.54.3 are now installed. With the FreeBSD IPC fix supplied to an isolated browser process, GTK 4.24.1_1 reported 495 DMA-BUF format/modifier combinations and WebKit painted the expected pixels on Intel ADL-N. WebKitGTK 2.54.3_1 is installed. Accelerated painting and WebGL readback passed without the IPC workaround. The GNOME 51 environment is selected for both the current and next boot. [Laptop deployment and verification](https://github.com/denislemire/hpbsd-lab/blob/d0c618f/docs/gnome51-laptop-2026-11-24.md).

The running BE also has `bsd3-gnome51-desktop-51.1` installed from the
laptop candidate repository. Its 36 direct dependencies were already present.
See the [architecture review and package priorities](gnome51-desktop-review.md).
The internal main-branch channel still awaits publication.

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
