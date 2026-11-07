# Physical GNOME Wayland acceptance — November 5, 2026

State: the laptop has a working GNOME47 Wayland desktop, input, webpages and
normal logout to its graphical login screen. This is a beta milestone, not
validation of GNOME49+.

The installed candidate is the published beta5 release at b61cbb6313084e02dd28f77ff6eb0cb9cf7883db:
Mutter47.10_5 from native job 45, with the unchanged exact beta3 BSD3 0.1.0.b3
and ConsoleKit2.0.0_5 artifacts from CI36. The generic 15-STABLE VM passed
its package/session/device/recovery checks before hardware installation.
All hardware package payloads were downloaded from the GitHub release and
verified. The kernel, Wi-Fi module and Wi-Fi experiment helper were unchanged.
The protected evidence records their SHA256 identities and boot environments.

On hardware, Mutter discovers the Intel i915 card, selects atomic KMS and GBM,
and starts the local Wayland/user session on VT10. Denis confirmed usable
input and desktop applications. Konqueror rendered webpages. Earlier beta4's
first-frame crash was fixed by PR19; beta5 reaches and maintains the desktop.

## Logout correction

The throwaway profile's EXIT trap used `rm -rf` while applications could still
write its contents. Cleanup reported Directory not empty and returned1.
SDDM0.21 treats helper status1 as an authentication error and does not restart
the display in that branch. GNOME had shut down, leaving a frozen last frame.
The original GNOME exit status was not independently recorded before the trap;
the evidence supports this cleanup failure, not a claim that every earlier
logout problem had that cause.

The new `tools/hardware/gnome-desktop-profile.sh` uses a private persistent
profile under the user's local data directory. It executes the session directly,
without an EXIT cleanup trap. It preserves desktop settings and the session exit
status. Denis confirmed two subsequent normal logouts returned to the greeter;
SDDM recorded successful helper exits and fresh greeter startup. No trial timer
or recovery guard caused those returns.

## Browser compatibility workaround

GTK4 reported DMA-BUF texture import disabled at compile time during the first
session. Web showed blank pages. Setting WebKit's force-shared-memory transport
alone did not fix launching: a local core showed SIGABRT in graphics preparation
through `gdk_display_prepare_gl` and WebKit acceleration setup. The installed
package reports webkit2-gtk_60-2.46.6_8, but runtime source correspondence has not
been fully established; do not infer the exact binary source from that name alone.
The package integrity check passed.

An explicit Wayland browser launch with `WEBKIT_DISABLE_COMPOSITING_MODE=1`
stayed running and Denis confirmed rendered webpages. The persistent desktop
wrapper now exports that browser-engine compatibility flag, including to the
session bus. This affects WebKit applications; Mutter's Intel GPU compositor
remains enabled. No browser sandbox switch or system power-policy change was made.
The earlier force-SHM flag is no longer used by the saved wrapper.

Denis then logged in again and confirmed Web launches from the ordinary GNOME
app launcher and renders webpages with the saved setting. Separately, bus-activated Epiphany search and Nautilus preview reported
missing display connections. A temporary update of the current bus's Wayland
variable was tried; this does not establish a durable activation-environment fix.
These service issues remain open, as do broader desktop acceptance tests.

## Deployment boundary

The customized SDDM greeter accepted passwords on hardware. Its clock/date,
optional battery model and removed layout selector are recorded in the greeter
preferences guide. The actual cursor worked with FreeBSD's icon search path.
The new desktop wrapper keeps that path and seeds 12-hour time and battery
percentage in its separate persistent profile.

The experimental boot environment remains active for this running session.
The normal HE-enabled environment remains the persistent reboot default.
SDDM is currently started manually through its native rc.d script. At handoff the temporary
boot-return and trial-inhibitor startup knobs were disabled in the clone. The
bounded sleep inhibitor was stopped and ConsoleKit confirmed only the normal
GNOME inhibitors remained. The broker still owned login1 and SDDM remained
running. Automatic desktop startup and reboot acceptance remain to validate;
the normal parent is still the persistent reboot default. No default boot
selection is changed by these example scripts.

Protected on-machine evidence is under `/var/crash/bsd3-beta5-20261105/`, including
`physical-trial-1`, `regular-session` and `browser-software`. Raw logs and cores
stay off Git. First guarded trial cleanup was assisted and is not counted as an
unassisted logout pass. Later regular-session successful logout evidence is
separate. Shell syntax checks passed for the installed persistent wrapper.
