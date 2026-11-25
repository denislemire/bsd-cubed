# FreeBSD service account for bsddd

The package creates a locked service account named `_bsddd` with a private
group, no home directory and a nologin shell. The rc script launches the
login1 broker with this identity. The root supervisor retains the pidfile.
The broker receives no supplementary groups. The `bsddd_power_group` setting
identifies eligible callers and does not confer group membership on the daemon.

The proposed UID and GID are both 829. Both slots were marked free on
FreeBSD ports main on 2026-11-24. Until the assignment lands upstream, the
overlay provides the account records and checks the build's ports registries
for conflicts. For the upstream port submission, include
`packaging/freebsd/bsddd-ports-ids.patch`, recheck both slots against
current main, and use the standard ports UID_FILES and GID_FILES. See the
[FreeBSD Porter's Handbook](https://docs.freebsd.org/en/books/porters-handbook/special/#users-and-groups).

## Trial on FreeBSD 15.2-STABLE

A network-disabled jail cloned from the GNOME 51 boot environment upgraded
from package revision 5 to 8. Its own system bus and ConsoleKit served an
ordinary client's ListSeats (seat0) and ListSessions (empty). The installed
daemon owned login1 with effective UID 829 and primary GID 829. The rc
lifecycle test checked start, status and stop, rejected a name claim by
nobody, and refused to start over a separately managed owner. A power query
from nobody received AccessDenied.

## Physical laptop

The laptop now defaults to `hpbsd-gnome51-bsddd-829`, running package
revision 8 from commit `ca81335`. The previous `hpbsd-gnome51-gdm-trial`
BE is available for rollback. GDM password login and manual lock/unlock
worked. The broker owned login1 as UID/GID 829.

A timed switch to VT2 displayed the text console. On return to VT10 the
display stayed black with a cursor. ConsoleKit marked the GNOME session
inactive even though the kernel reported VT10. Restarting GDM alone produced
an inactive greeter. Restarting ConsoleKit with GDM stopped, then starting
GDM, restored an active greeter and a successful login. The broker stayed
up throughout. The role of the UID change in the VT failure is unresolved.

CanPowerOff from the active desktop session returned AccessDenied.
A direct PolicyKit check as the broker account confirmed that it lacks
permission to check the power-off action for another identity. Idle locking,
an ordinary logout/relogin cycle, and authorized inhibitor and power
operations still need verification with this package.
