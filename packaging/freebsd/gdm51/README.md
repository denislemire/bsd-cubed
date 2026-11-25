# GDM 51 on FreeBSD

The x11/gdm port builds the GDM 51.0 daemon, PAM session worker, Wayland
session launcher, utilities and libgdm from the GNOME release tarball.
The port's distinfo pins the source. BSD³ 0.2.0.a3_3 supplies the seat,
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
