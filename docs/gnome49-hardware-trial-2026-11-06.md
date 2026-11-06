# GNOME49 first hardware trial — November 6, 2026

State: first hardware login failed; working GNOME47 recovery boot verified.
Alpha2 correction is locally and natively tested, not yet released or accepted
on hardware. CI57's complete headless validation remains valid within its scope.

The exact CI57 packages from signed release v0.2.0-alpha1 were freshly downloaded,
verified and installed in a clone of the working desktop boot environment.
The kernel, radio module and installed supplicant hashes matched the working
environment. The original environment stayed the default; the candidate used a
one-time boot with bounded recovery. Personal files remain on the shared home
dataset. GNOME49 received separate copied config/data and new cache/state folders;
a root-owned, per-user symlink inside its BE selects that profile.

## Settings cache failure

The first Shell49 process aborted while registering native brightness bindings.
The installed49 XML declared those keys, but the compiled schema cache still
dated from47. Package integrity alone did not cover this generated file.
`gsettings get org.gnome.shell.keybindings screen-brightness-up` failed before
an explicit schema rebuild inside the new environment and succeeded afterward.
The next login kept Shell49 and the complete session graph running.

The offline deployment used `pkg -r`; repository configuration and unsandboxed
absolute-path triggers need particular care in that mode. Subsequent offline
package work uses a real `pkg -c` chroot. The new
`tools/hardware/check-gnome49-schemas.sh` checks the installed root's compiled
brightness keys and optionally refreshes schemas inside that root. CI now also
checks these keys after installing its new Shell package. A refreshed cache did
not itself establish a working hardware desktop.

## Missing session seat property

The second login reported SessionManager running, Shell49.10, all seven required
settings services ready and Airwave enabled without an extension error. Its VT
and ConsoleKit session were active, but DisplayConfig reported no physical or
logical monitors. Reading login1 Session.Seat returned “No such property”.

Mutter49's actual launcher obtains the seat using that D-Bus property; the older
launcher used the C seat API already provided by BSD3. Without the new property,
the49 backend can continue headlessly. Alpha2 implements the real `(so)` tuple:
ConsoleKit's seat object is matched to the same owner's validated inventory and
canonical name, then mapped to the exported login1 seat object. Only an explicit
ConsoleKit no-seat result returns the empty-name/root-path tuple. Errors and
inventory inconsistencies fail rather than guessing seat0. No device-control
authorization is relaxed.

All19 protocol tests passed on FreeBSD15-STABLE, with the expected credential
fixture skip. The ordinary-user lifecycle suite passed with versioned Python3.12.
A new consumer fixture reads Mutter49's actual get_seat_proxy implementation and
upstream XML at runtime, generates its real proxy bindings, and exercises
GetAll→Session.Seat→Seat.Id. It fails against the old daemon and passes with the
correction, including legacy and escaped seat names and explicit no-seat behavior.
Upstream implementation/generated code is not copied into BSD source files.

## Other observations and remaining acceptance

Wi-Fi associated after boot and mDNS resolved hpbsd.local from the Mac. Airwave's
service ran and its49 extension reported active. This does not qualify radio
performance or an actual interactive Airwave connection change. Later band and
throughput concerns remain separate from the desktop blocker. Cheese is included
in the planned49 app set; camera operation still needs checking.

The user reports Ctrl+Alt+Fn VT switching stopped working with the BSD3 desktop,
including47; the HP Fn modifier made no difference. That remains a separate
unresolved issue. The prior live47 session also left a SIGSEGV record; its cause
is not established. Do not describe either issue as fixed by the seat change.

The recovery guard returned to the original47 environment. The49 BE, private
crash record and logs are retained. Native CI, a matching alpha2 prerelease and
fresh hardware login/input/apps/logout acceptance remain required.

## Alpha2 release qualification

Revision `fd0270824dfe96f3b1cfc1289934b2c54f18bbfe` passed combined native
CI58. Its eight exact packages passed installed-file integrity and both isolated
and system-autostart complete-session tests on FreeBSD15-STABLE
(`stable/15-n286624-646a485c3b2f`): Shell49.10, SettingsDaemon47.2_1, a real
Wayland application, ordinary logout and bus-owner cleanup. This VM has no DRM;
these results do not establish hardware display/input acceptance.

Signed prerelease `v0.2.0-alpha2` contains the packages, matching consumer sources
and ports recipes, complete BSD3 source, hashes and validation provenance. A fresh
download of every release asset matched the verified staging files. The immutable
public `bsd3-gnome49-beta2` channel also includes the previously released Airwave
alpha2. An isolated native package client read its nine entries, downloaded all
nine packages and compared them byte-for-byte with the release inputs. The first
client attempt lacked `INSTALL_AS_USER` and correctly refused the user-owned
private database; enabling that setting permitted validation without touching the
system package database.

Laptop alpha2 installation and a fresh interactive hardware trial remain open.
The initial upload was interrupted by a slow management connection; deployment
switched to laptop-initiated, hash-verified downloads from the public channel.
The running47 environment and default boot selection remain preserved.

## Alpha2 installed candidate

At approximately22:11UTC, the complete final package matched its release hash
after bounded resumed downloads. All eight release packages were installed with
`pkg -c` into the inactive `stable-15-gnome49-20261106` root. The schema refresh
and all six brightness-key checks passed. Installed-file integrity passed for
all eight packages, Cheese and Airwave. Kernel, radio-module and supplicant hashes
matched the currently running47 environment. No boot selection was changed.

The candidate's separate profile symlink and enabled desktop, Airwave, mDNS,
webcam and bounded recovery services were checked. Its recovery guard now kills
its background sleep when stopped, avoiding the orphaned pidfile lock seen in
the earlier trial; this correction has a shell syntax check but awaits the real
boot/stop path. An active user desktop was detected, so reboot awaits the owner's
save/logout confirmation. Hardware49 display/input and logout acceptance remain
open. The documentation-only CI59 run was canceled to preserve lab capacity;
the candidate remains the exact successful CI58 source and packages.

## First successful hardware49 startup

After the owner saved work and explicitly approved reboot, the candidate was
activated for one boot. At22:13:43UTC, the machine reported the expected49 root,
SDK0.2.0.a2, Shell49.10 and the running graphical greeter. The owner logged in,
reported a working desktop and confirmed the result. Read-only queries against
the actual user's session bus then returned:

```text
RUNNING_SHELL=49.10
PHYSICAL_MONITORS=1
LOGICAL_MONITORS=1
SESSION_RUNNING=true
AIRWAVE_STATE=1
```

This establishes the first actual hardware49 desktop startup, including display
discovery and an enabled Airwave extension. It supersedes alpha1's zero-monitor
failure; it does not establish application coverage, radio throughput, camera
capture, VT shortcut switching or normal logout acceptance.

The boot guard was marked accepted for this startup, stopped and disabled before
the owner continued exploring. Service status confirmed it stopped, and no
`sleep 900` child remained: the revised stop path was exercised successfully.
The working47 BE remains the permanent default pending the owner's input/apps,
Airwave interaction and ordinary logout checks. The greeter remains SDDM/Xorg;
SettingsDaemon remains the explicitly qualified47.2_1 component set.
