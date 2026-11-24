# FreeBSD GNOME desktop review, 24 November 2026

## Session architecture

The review covered the login launcher, Dinit user supervisor, bus readiness
helper, ConsoleKit device controller and GNOME 51 package job at `55b0d49`.
It was a source review and private-bus test run on FreeBSD 15.2-STABLE.
Hardware VT transitions, multi-seat operation and suspend were outside this run.

| Priority | Finding and user impact | Change and acceptance test |
| --- | --- | --- |
| High | After the manager shutdown deadline, `src/user-session.c:198` requests a descendant kill and then calls blocking `waitpid` even when the kill failed. A surviving or stalled child can leave logout waiting indefinitely. The lab black screen has no confirmed attribution to this path. | Use a monotonic deadline when reaping, preserve the kill error and identify the cleanup phase in the exit report. Exercise the failure and timeout branches, then repeat physical logout and login. |
| Medium | `src/session.c:252` polls the desktop child every 10 ms during its entire lifetime and synchronously queries backend ownership once per second. This adds idle wakeups and a stalled call can delay stop handling. | Use child-exit and D-Bus owner-change notifications with explicit stop deadlines. Test backend replacement during startup and during an active session. |
| Medium | `src/bus-ready.c:72` waits a fixed 100 ms after child exit before deciding whether the session-ending signal arrived. The two events are delivered independently; under load a clean logout can be classified as failure. | Give teardown an explicit acknowledgement or supervise it from the component that initiates logout. Test both delivery orders and a delay exceeding 100 ms. |
| Medium | The serial package job separately maintains the build order, exceptions for dependencies rebuilt by the overlay, and the desktop package selection. The browser packages were previously absent from the build order despite having working recipes. | Track origin, flavor and recipe together and validate the metapackage closure against the resulting catalogue. Add a clean-client solver check with the stock repository available. An install on the already upgraded builder cannot exercise first-install resolution. |

Keep the device ownership checks in `src/control.c:167`, the unique backend
owner and the handling of ambiguous device-grant replies at line 369. The
private-bus suite covers duplicate owner, missing backend, device lifecycle
and failure paths. Keep the ordinary-user supervisor distinct from the system
service. Limit consolidation to genuinely shared lifecycle logic such as
bounded descendant cleanup. The GNOME 49-named system adapter is also used by
the GNOME 51 build; retain its implementation and rename its source/build
references together if reorganizing it.

Tests run from a fresh archive: `make test`,
`make PYTHON=/usr/local/bin/python3.12 test-bus-ready` and
`make PYTHON=/usr/local/bin/python3.12 test-user-session`. All passed. The
host-credential absence case was skipped on the host that supplies peer PIDs.
The private-bus and supervisor results do not certify physical greeter recovery.

## Package selection

`bsd3-gnome51-desktop` is the install target for the qualified desktop and app
set. Its dependency floors cover the working GTK and WebKit revisions as well
as the BSD³ session provider. It has no service-enabling scripts. GDM rollout,
hardware-specific tools and the separate Orca coding application are independent.
The `accessibility/orca` dependency is the GNOME screen reader.

Plan the next additions against the [GNOME release manifest](https://gitlab.gnome.org/GNOME/gnome-build-meta/-/tree/db69d872ba90a7bb96b53bbcc8a88ea8e9c6c9f0/elements/core):

| Priority | Package gap on the laptop | Qualification |
| --- | --- | --- |
| 1 | Loupe 51.0 and Contacts 51.0 are missing. Eye of GNOME and Evolution Data Server are already installed. | Open common images from Files through glycin. Create, edit and reopen a local address-book entry; verify it in Evolution. |
| 2 | Showtime 50.0 and Decibels 49.6.1 are missing. Totem 43.2 is installed. | Verify playback, seek, audio output and subtitle handling against local media before changing default handlers. |
| 3 | Snapshot 51.0 and Simple Scan 50.0 are missing. Cheese 44.1 is installed. | Verify real camera capture and scanner discovery respectively; ship them as hardware-qualified additions. |
| Maintenance | GNOME Keyring 48.0, Tweaks 46.1_1 and File Roller 44.6,1 are installed. The upstream release directories offer 51.1, 49.0 and 44.7 respectively. | Test keyring unlock across logout with a disposable test secret before migrating credentials. Test preferences persistence and archive open/create/extract. |

Connections 51.0 and Music 50.0 are further core-app gaps. Evaluate GNOME
Software against a working FreeBSD pkg backend and GNOME Disks against the
FreeBSD block-device service before adding them as defaults. Weather, Baobab
and Font Viewer 50.0, and Yelp 49.2, already match this GNOME release's pins.
Keep older viewers available until the new handlers and reverse dependencies
have been checked. The existing stock `gnome-47` metapackage should be assessed
separately before any transition or autoremove operation.
