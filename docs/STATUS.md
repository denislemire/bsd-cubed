# What I have running

Last poked: 11 November 2026. Dated notes in this directory are older lab
scribbles. This page and the root README are the current story.

## On my machine

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

GNOME 47 was the first thing that booted. GNOME 49 is what I use. Some
libraries keep their own numbers (gnome-desktop 44.5 is a GNOME 49 pick, not
a leftover 47 port).
