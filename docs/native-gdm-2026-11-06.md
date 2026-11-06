# Native GDM investigation — November 6, 2026

State: still open. GNOME47 beta5 already works through SDDM. This work ports the
GDM47 greeter lifecycle to FreeBSD; it is not GNOME49 desktop acceptance.
The laptop remains with the Wi-Fi/app-cleanup chat. All experiments below use
the disposable bsd3-stable KubeVirt VM, with an independent root serial console.

## Source and environment

GDM47.0 upstream tag revision: 9c78ff6d02195156a5b556eb2a6395d7ae3d6910.
FreeBSD ports revision: 2072800715389ae49066946c960e6521bac37e6a.
VM running kernel, installed kernel and userland all report 15.2-PRERELEASE;
GENERIC stable/15-n286624-646a485c3b2f, clang21.1.8. Installed integration baseline:
BSD3 0.1.0.b3, ConsoleKit2 2.0.0_5, Mutter47.10_5, Shell47.10, session47.0.1.
There is no DRM device, so a headless greeter fixture can test registration and
lifecycle but cannot establish hardware presentation or GPU input behavior.

Protected raw captures and stock-package/configuration rollback copies remain
under /root/bsd3-gdm-probe-1 on the VM. Only reviewed findings belong here.

## Native VT boundary

Hypothesis: the stock port's Linux-conditional VT setup leaves Wayland without a
validated local controlling terminal. The original helper opens actual ttyv
nodes, checks index and VT_AUTO ownership, selects a free VT when required,
activates with a three-second bound and attaches in the setsid child before
privilege drop. It initializes ownership to -1 and handles descriptor zero.
ConsoleKit registration receives actual display-device, class, locality and a
validated private runtime directory; incomplete local Wayland setup fails.

Actual-helper unit tests passed against native FreeBSD headers with warnings as
errors. The kernel probe opened a free VT, activated it, attached a new session,
dropped to the existing nobody account and restored the original VT. Result:
`native-vt=PASS restored=PASS forced-kill=0`. This is component acceptance,
not GDM login acceptance. The probe requires explicit disposable-KVM opt-in.

## Factory regression

The stock port disables Linux's user-display-server monitor. That also hid the
modern Wayland display constructor. The patch enables that constructor only for
Wayland on FreeBSD and preserves the old Xorg constructor. A test compiles the
actual consumer block in four FreeBSD/Linux build configurations. It reproduced
the original failure and passed after correction. An initial broader prototype
also selected modern Xorg; it was narrowed before the recorded package build.

## First runtime trials

GDM47.0_2 completed the native build, package stage, plist and stage-QA checks.
The first package SHA256 was
b6ef46a4cacd136868cb066828ae242e827c5ca79c1c3c8c85d5002cce737b41.
This is an experimental artifact, not a published release or final recipe hash.

The first guarded invocation rejected `--nodaemon`: GDM47 already stays in the
foreground and exposes no such option. The rc script was corrected to invoke
GDM directly under daemon(8). Shell syntax alone did not validate this contract.

The second trial requested Wayland but fell back to X11 after ten seconds. Its
log reported an invalid D-Bus object path during the graphics capability query.
ConsoleKit advertised seat name `seat0`, a distinct object path, and
CanGraphical=true. The legacy compatibility function takes the object path;
GDM passed the name. The correction resolves the advertised pair with a bounded
query, rejects missing or duplicate names, and propagates the actual capability.
It does not manufacture graphics support. A focused actual-function test covers
mapped paths, false/error capability, missing names, duplicate names and query
failure. The actual-source seat test passed natively after correction.

GDM was stopped explicitly after the diagnostic observation. The VT guard's
PASS establishes bounded workload exit/restoration only: no working greeter was
observed. Read complete.txt after final snapshots; command-exit.txt is earlier.
A test-only ConsoleKit sleep/idle inhibitor was acquired and its PID verified in
ListInhibitors. It has an 1800-second maximum and is released at handoff.

## Active Wayland greeter milestone

The corrected package SHA256 is
59c3028e182d2d388efa8b9c4048914a0accff607cbe865b38a63178b6be33e0.
Its headless Shell greeter registered with GDM. Actual ConsoleKit output verified
UID92, class greeter, type wayland, local=true, seat0, state active, VTNr9,
/dev/ttyv8 and /var/run/xdg/gdm. The actual Shell process ran as UID/GID92 with
that controlling terminal. GDM reported `session registered: yes`.
Requested GDM shutdown returned child exit0; the guard's final barrier was PASS,
recovery PASS, original VT1 restored, with zero terminated/killed descendants,
zero deadline intervention and zero requested signal to the guard.

Two further corrections were necessary. A Wayland session must leave its X11
metadata empty, or ConsoleKit selects a dynamic seat. The pre-fork worker also
must advertise its validated native VT number explicitly: the future child's
terminal cannot be inferred from the registering worker. Typed-GVariant tests
compile the actual registration block and pass for Wayland/X11 and owned/missing
VT descriptors. These are metadata from a real validated descriptor, not assumed
seat activity or manufactured capabilities.

The installed GNOME47 port separately appends --disable-acceleration-check to
TryExec as well as Exec. GDM rejects that invalid executable name and aborts if
no valid session remains. A temporary valid VM-only session entry isolated GDM
from this defect. A minimal pinned gnome-session47 recipe correction now limits
the substitution to Exec. It is not built or released yet.

The headless fixture initially had mode0600 from the protected root shell's
umask, so the ordinary greeter could not read it and launched stock Shell in X11
mode. Giving only that non-secret desktop file mode0644 fixed the fixture. This
was a test setup error, not a product fix. No hardware GPU greeter is established.

## rc.d lifecycle and recovery

The next native package, including an explicit FreeBSD local-program search path
in rc.d, has SHA256
016cb8f83cff5555b4a1fdfd2f6b757db43168153bd919bf6108d9c51fd3fee9.
The original rc test exposed a real boot-context defect: service(8)'s minimal PATH
left gdm-wayland-session unable to execute dbus-daemon. Exporting PREFIX/bin and
PREFIX/sbin fixes this without depending on a root interactive shell.

The protected lifecycle controller exercised onestart, faststart and quietstart,
verified the bus-name PID, waited for an active greeter, and stopped only the
PID-file-selected GDM after each invocation. All three completed; the final guard
barrier was PASS with child exit0, original VT1 restored, and zero deadline or
forced-descendant intervention. Startup variants were invoked directly through
the rc script with the temporary gdm_enable=YES environment. Earlier attempts to
pass this override through service(8) were discarded by service's environment
reset, so the disabled faststart correctly did nothing; those incomplete fixture
runs are recorded as FAIL_OR_ASSISTED, not successful service acceptance.

After the test, the original GDM configuration was restored, both temporary
desktop entries removed, the test-started BSD3 service stopped and the bounded
inhibitor released. No GDM, Shell, BSD3 daemon or test inhibitor remained running.
The experimental package remains installed only in the VM; no service was enabled
persistently. Hardware and full authentication/logout acceptance remain open.

## Remaining

Add the guarded runtime greeter fixture to reproducible CI; the manual VM
qualification above does not establish CI or hardware acceptance.
Add reproducible native CI source/recipe artifacts before publication. Physical
GPU greeter and a complete user login/logout cycle require a separately agreed
hardware window after the VM tests pass. No laptop package or service changed.

## Laptop provisioning handoff received November 6

The provisioning owner reports stable-15-desktop-20261105 is now the persistent
NR default, with stable-15-desktop-before-apps-20261105 retained for recovery.
GNOME-focused applications were installed and84 legacy XFCE/Plasma packages
removed after protected-core checks. Working GNOME47/Mutter/ConsoleKit/BSD3/SDDM
and kernel/radio/WPA identities were unchanged. A native read-only Qt battery
adapter replaced the SDDM theme's Plasma import; the owner reports actual-SDDM
and offscreen checks passed. Xorg/Xwayland remain installed.

This is an attributed handoff, not a fresh inspection by this GDM investigation.
The authoritative provisioning record is hpbsd-lab's dated
docs/desktop-workstation-2026-11-06.md. Wi-Fi roaming remains unresolved. Cursor
CLI works under Linux compatibility; its persistent GUI sandbox exception awaits
Denis's approval and the keyring workflow remains open. This handoff authorizes
no additional laptop mutation; GDM and GNOME49 work continue in separate VMs.
