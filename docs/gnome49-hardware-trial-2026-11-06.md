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
