# GDM 51.0 on FreeBSD

The upstream GDM 51.0 client library builds with the BSD³ login SDK. The
FreeBSD ports build of `bsd-cubed-0.2.0.a3_2` passed its private-bus tests,
user-session tests, packing-list check and package target. Its SDK adds four
read-only calls backed by ConsoleKit: session service, terminal, VT number and
remote status. The SDK returns a terminal name relative to /dev and obtains
the VT number independently. On the GNOME laptop it returned `bsd3-sddm`,
`ttyv9`, VT 10 and local. No SDK daemon update was required to perform these
reads.

To reproduce the client build, put the new SDK's pkg-config file on
`PKG_CONFIG_PATH` (or install the SDK on a builder) and start with the
GNOME GDM 51.0 source tarball, SHA-256
`2ce5acf5a438b8f6dc1d14fd777a911be5eeb2f7ab9a7c4600166205b15d6536`.

```sh
for p in packaging/freebsd/gdm51/files/patch-*; do
    patch -s -d gdm-51.0 -p0 <"$p" || exit 1
done
meson setup build gdm-51.0 --prefix=/usr/local -Dlogind-provider=bsd3 \
    -Dsystemd-journal=false -Dsystemdsystemunitdir=no -Dsystemduserunitdir=no
ninja -C build libgdm/libgdm.so.1.0.0
```

The resulting shared library enumerated the installed BSD³ Wayland desktop
from the running local session. The upstream common library found the mock
greeter for its own seat and rejected a different seat. The chooser looks up
the calling process's session and the service reads the registered PAM
service. The existing BSD³ bridge already listed seat0 and returned true for
its CanGraphical property on this laptop. These checks exercised discovery
without starting GDM or changing the active login.

## Scope of the local Wayland daemon

Estimates below are engineering time for a single local seat and a static
greeter account. They include FreeBSD integration tests. They exclude remote
login, dynamic per-display accounts and multi-seat qualification.

| Work | Minimum implementation and qualification | Estimate |
| --- | --- | --- |
| Seat and display factory | Use the existing BSD³ seat inventory, CanTTY and CanGraphical. Resolve the active session and observe real seat/session/VT changes. Replace the Linux sysfs watch with native events and the tested FreeBSD VT helper. Test removal during a lookup and failure to acquire the seat. | 1–2 days |
| Greeter and worker | Use the package-owned, unprivileged gdm account for a single local greeter. Disable unsupported remote/dynamic account paths explicitly. Replace Linux shadow and VT dependencies at the consumer, integrate OpenPAM and ConsoleKit registration, and launch the greeter through the native session supervisor. Verify the registered class, service and VT for both greeter and user. | 2–3 days |
| Session control and recovery | GDM calls ActivateSessionOnSeat, UnlockSession and TerminateSession. Bind requests to the intended seat, session and authorized caller. Use native tracking of the session process tree instead of reading cgroup.events. Test denied requests, bad credentials, a crashed compositor, return to the greeter and VT recovery in a GPU/VT-capable VM. | 2–3 days |

A diagnostic build selected the BSD³ dependency and compiled the daemon and
worker targets far enough to encounter unconditional systemd headers,
Linux shadow.h and sys/vt.h. Disabling systemd userdb still leaves the
dynamic-user fallback in the build. The local factory also opens
/sys/class/tty/tty0/active. Removing includes alone cannot supply the missing
account, VT and session-control behavior. Implement these in the consuming
GDM paths or existing native services. A general systemd event loop, journal,
user-unit database and cgroup compatibility layer are outside this scope.
