# Automatic desktop startup — November 5, 2026

The working physical GNOME47 beta5 session can use FreeBSD's existing native
service startup. No replacement init system or additional service manager is
needed. This setup starts SDDM's graphical login screen, with a password-required
GNOME Wayland session; it does not configure automatic user login.

`tools/hardware/enable-desktop-startup.sh --check` checks the installed native
service graph and rejects another enabled display manager. `--enable`, as root,
saves a private backup of rc.conf and enables dbus, bsddd and sddm using sysrc.
It does not install packages, activate a boot environment, restart the running
desktop, or change authentication. Use it only after qualifying the packages and
session configuration. The native order check passed in both the primary STABLE
VM and on the laptop: dbus before bsddd before sddm. FreeBSD rcorder warned about
uninstalled optional firstboot provisions; the actual desktop order was valid.

The hardware environment was cloned from the already working 15-STABLE desktop
as `stable-15-desktop-20261105`. All running/installed/userland versions report
15.2-PRERELEASE. The installed published beta5 compositor and exact prior BSD3/
ConsoleKit package payloads were retained. The enhanced kernel, rtw89 module and
HE startup helper SHA256 identities match the earlier successful session.
The previous normal HE boot environment and a pre-startup snapshot are retained.

First boot is a one-shot candidate selection, with the normal HE environment
still the persistent default. The existing boot-return guard and bounded sleep
inhibitor are enabled only for acceptance. The guard's actual rc.d invocation
was exercised before reboot and its armed/acknowledged records verified. A
separate detached observer demonstrated streaming output before shutdown. The
existing boot evidence collector is present and enabled. Raw state/configuration
and boot logs remain in protected on-machine evidence.

## Reboot result and combined-acceptance hold

The one-shot reboot reached the graphical greeter without an operator starting
services. The user confirmed the login screen and later working sound. Remote
checks verified the new environment, native bsddd/sddm startup, login1 ownership,
unchanged kernel/radio/helper identities and package integrity. The boot-return
guard acknowledged the check. The qualified BSD3, ConsoleKit and Mutter packages
are now locked against accidental stock-repository replacement.

Combined acceptance remains open: Wi-Fi associated on 2.4 GHz HT/20 even though
the HE hook succeeded and HE1/EHT0/hardware crypto1 were present. A fresh scan
found multiple 5 GHz candidates for the saved network. No band constraint was
configured on that profile. Subsequent SSH became unreliable before the staged
5 GHz roam experiment could be transferred, so that experiment did not run.
The user was given temporary local profile-frequency/reassociation commands;
these do not save a configuration change. A band-selection difference has been
observed, not a proven explanation of every connection stall.

The desktop environment has not been promoted to persistent default. The former
normal HE environment remains R; the desktop is the currently running N. The
boot-test inhibitor/startup controls must be retired in the new environment once
combined networking acceptance is resolved; do not call this completed handoff.
The architecture paper records the same boundary.

## Recovery and maintenance

From the loader's boot-environment menu, select `stable-15-he-usable-20261105`
to recover the former normal system. The new desktop environment retains the
same kernel and Wi-Fi enhancements; the fallback differs in desktop deployment.
A failed early test boot returns to that default after five minutes if the
kernel can still schedule its guard. A frozen kernel requires a local reset
and boot-environment selection; the timer cannot recover a frozen kernel.

For ordinary desktop maintenance, stop the graphical service only after saving
work and logging out. `service sddm stop` stops the login manager and
`sysrc sddm_enable=NO` prevents graphical startup on the next boot. Leave the
message bus and session broker available for other desktop clients. To restore
all prior startup settings, review the private rc.conf backup and restore the
relevant values; do not overwrite unrelated later configuration blindly.

Package updates must retain the qualified native Mutter/ConsoleKit/BSD3 set.
A broad stock-repository upgrade can replace the native backend or device fixes;
new package sets require VM qualification before hardware rollout.

For a qualified update, explicitly unlock only the custom package set, install
the verified candidate, requalify and relock it. The exact lock command used was
`pkg lock -y bsd-cubed consolekit2 mutter`; existing unrelated locks remain intact.
