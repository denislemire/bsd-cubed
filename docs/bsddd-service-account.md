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

Before a desktop rollout, test GDM login, logout and relogin, device access
and VT switching, idle lock and unlock, and authorized power and inhibitor
requests with a real active seat in a trial BE. ConsoleKit and PolicyKit see
the new daemon identity for backend calls. Record any denied operation and
its original caller before considering a narrowly scoped privilege grant.
