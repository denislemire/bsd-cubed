# GDM 51 on FreeBSD

The x11/gdm port builds the GDM 51.0 daemon, PAM session worker, Wayland
session launcher, utilities and libgdm from the GNOME release tarball.
The port's distinfo pins the source. BSD³ 0.2.0.a3_4 supplies the seat,
session and virtual terminal interfaces. Build the SDK with
`BSD3_USERSESSION=yes sh tools/ci-port.sh`. The GNOME package pipeline
builds GDM after its GNOME Shell, session and keyring providers.

The worker opens a dedicated ConsoleKit connection for each authenticated
session and holds it until logout. It passes the session cookie, native VT
and validated runtime directory to its child. BSD³ handles VT allocation,
activation and attachment. Session activation checks seat membership and
retains the calling connection for ConsoleKit authorization. The upstream
daemon still owns the greeter lifecycle and PAM conversations.

The greeter uses the local gdm account on one seat. The package installs
separate OpenPAM services for the greeter and the user's password login.
A dinit service directory starts the GNOME session manager and GNOME Shell
in GDM mode. The rc service defaults to disabled and checks for a running
SDDM before starting.

## Qualification

On FreeBSD 15.2-STABLE amd64, the ports build passed stage, check-plist,
stage-qa and package. In an isolated boot environment, the installed daemon
acquired org.gnome.DisplayManager on a private bus and exposed the 51.0
Manager interface. OpenPAM accepted the gdm greeter account, rejected root
for the greeter, accepted the test user's password and rejected an incorrect
password. These tests can run with display devices hidden.

Before enabling this port on a workstation, verify a visible greeter,
password login into GNOME Shell, lock and unlock, logout back to the
greeter, a second login and recovery to the original VT. Remote login,
multiple greeter seats, automatic login and fingerprint authentication
are outside this port's tested scope. FreeBSD lacks Linux session cgroup
freezing, which upstream checks when choosing a reauthentication path.
Forced termination of a conflicting session is unsupported.

On a disposable FreeBSD 15.2-STABLE KVM guest the native VT probe activated
a spare terminal, attached it to a new session, dropped to an unprivileged
user and restored the original VT without a forced kill.

On the same VM, a private FreeBSD root with an initialized machine ID and
Mesa software rendering ran the packaged GNOME 51 stack. For this test
the greeter compositor used a 1280×720 virtual monitor. GNOME Shell 51.1
registered both its session and display with GDM. ConsoleKit reported UID
92, class greeter, type wayland, active on /dev/ttyv8. Both the Shell and
greeter services reached ready. The guard restored VT1 with a clean child
exit and zero forced terminations. A separate registration probe checked
the actual session class, PAM service and removal on bus connection close.

This trial exposed two packaging fixes. The Shell imports GnomeAutoar on
startup, including in GDM mode, so its port requires gnome-autoar. The
Wayland-only greeter runs Shell with --no-x11. A failed greeter previously
reentered display creation during teardown. Port revision 1 bounds new
seat0 display launches to five per 30-second window.

GDM 51.0_2, BSD³ 0.2.0.a3_4 and GNOME Shell 51.1_2 also passed an
authenticated desktop round trip on the virtual monitor. A greeter-side
test client answered the real GDM password conversation for a disposable
user and selected the installed GNOME desktop. The user session ran Shell
51.1 as UID 1001 on VT10. A call to the session manager's Logout method
returned the guest to an active GDM greeter on VT9. Revision 4 uses
ConsoleKit's Session.Activate after verifying seat membership. The earlier
Seat.ActivateSession call failed on this backend even with a valid session
object path. Physical scanout, input and unlock on the laptop remain open.

Repeating the login exposed a race: the outgoing display was still releasing
the greeter account when the next display tried to acquire it. Revision 3
defers and coalesces retries and checks for an active user session before
restoring the login screen. With one GDM daemon running, two consecutive
password conversations started distinct GNOME Shell 51.1 user sessions on
VT10. Both logged out via the session manager and the guest returned to an
active, ready greeter on VT9. The VM used virtual monitors for both user and
greeter Shells. PAM successfully unlocked the test user's login keyring.
Its PAM-started daemon can outlive the graphical session, so an 85-second
whole-process-tree test deadline expired even after the screen had returned.

The physical FreeBSD 15.2-STABLE laptop subsequently passed password login,
logout to the branded greeter, switching to a console and back, and password
unlock after an idle lock. GDM is the default boot environment on that
machine. See the [desktop patch guide](../../../docs/gnome51-patches.md).
