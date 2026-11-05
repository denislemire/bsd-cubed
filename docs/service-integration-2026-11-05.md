# System-bus service candidate — November 5, 2026

State: implementation under test; not a desktop-readiness milestone.

The native Mutter port in PR4 was merged at
`a4a127be41254b1eec53cea00ea72e18149d8ab0`. Native CI20 passed the bridge tests,
ports package QA, ordinary shared-consumer link and a real headless Wayland-client
startup using its exact installed Mutter47.10 package. Downloaded package/source
artifacts matched both checksum manifests and the exact source provenance.
The environment was 15.1-RELEASE-p6, not 15-STABLE.

A separate unprivileged warm-VM experiment also started GNOME Shell47.10 against
that native Mutter. It logged “GNOME Shell started”, served a Wayland client and
reported ShellVersion47.10 over D-Bus. This was component startup with a private
bus, not a real ConsoleKit session or complete GNOME login. Staged binaries needed
test-only ICU data and GI search-path adapters because they embed /usr/local paths.
Those adapters neither mock API results nor belong in the production package.
Missing real services/helper paths produced warnings that remain in private logs.

## Smallest integration boundary

Hypothesis: a disabled rc.subr service can start the installed daemon on the real
system bus, refuse an unrelated login1 owner and report readiness only for its own
child. NameHasOwner alone cannot establish that identity.

The candidate adds a port-installed root-only bus-name ownership policy, while
method-level original-caller authorization remains in the daemon. There is no
D-Bus activation. The rc script uses run_rc_command, daemon's child pidfile and a
bounded typed GetConnectionUnixProcessID check. It discards caller-supplied bus
addresses. Its status means only that this process owns login1.

Private-bus tests cover malformed PID arguments, the correct owner and a live
non-owner. Native CI additionally tests the installed policy, ordinary-user
ownership denial, onestart/quietstart/faststart/status/stop, and an independent
foreign owner that must survive a rejected service start. Cleanup is confined to
processes/paths created by the test. No host is persistently configured or enabled.

The candidate version advances to beta3; beta2 artifacts from CI20 are preserved
and have not been published as a release. The earlier beta1 release is unchanged.
Before hardware readiness, remaining work includes an actual GNOME session with
real services, ConsoleKit device-state correctness, device/VT lifecycle and
recovery, 15-STABLE validation, and a verified beta delivery path. The laptop
remains reserved for a coordinated final trial with the Wi-Fi/kernel work.

## Pre-publication observations

The local macOS build passed seven applicable groups and skipped eight whose
peer-PID contract cannot be exercised on that host. On the warm FreeBSD VM,
`make clean all test` passed fourteen groups, including correct-owner/non-owner
checks and all controller/device protocol groups; the deliberate missing-PID
case was skipped because the native bus provides credentials. Shell syntax and
`git diff --check` passed. Installed rc/policy checks are pending native CI.

## Next integration experiment

The follow-up CI experiment installs the GNOME47 packages into the disposable VM
and reinstalls the exact native Mutter artifact afterward. A small unprivileged
harness opens a real non-local ConsoleKit session, retains its connection,
verifies the child resolves to its own UID and Wayland/user session through BSD3,
and closes it after the test. Unlike ck-launch-session's failure path, this
harness refuses to run a child without a confirmed session. Cookies remain in
child environments and are not printed.

The installed GNOME session manager runs with its builtin manager and the normal
GNOME RequiredComponents. A private desktop-file override selects headless Shell;
no system desktop file is rewritten. The check requires IsSessionRunning, the
Shell version, a real mapped GTK Wayland window with frame ticks, then clean
session-manager logout. It uses real system services rather than the component
test's empty bus or prefix adapters. Results remain pending until CI runs.

The window harness was compiled natively and separately passed against the warm
VM's private-prefix Shell component: a mapped GTK Wayland window received five
frame ticks. This does not prove physical pixel presentation, input or VT use.
The warm VM has no machine bus running; the genuine-session experiment belongs
in CI with its existing package/service administration access. No new privilege
grant or laptop change is needed.

## CI22 correction

CI22 passed protocol tests, ports package QA and native Mutter/Wayland startup,
but failed the first installed ownership assertion. The supposedly ordinary
foreground attempt actually acquired login1 and ran until the bound expired.
This does not distinguish a root CI process, caller-supplied bus routing or a
policy error. The revised test records numeric build/probe UIDs, explicitly drops
to the existing nobody account for the denial check and clears bus-address
overrides. It creates no account or new administrative grant. The installed
service check now runs immediately after the small bridge package, before the
Mutter rebuild, so failures are resolved earlier. Retest pending.

## CI26 correction

All package, service and compositor gates passed. The first genuine GNOME attempt
stopped before launching its session harness: ConsoleKit's typed NoSessions error
from GetSessions is a valid initial state, not a failed activation. The corrected
wrapper uses the bus's StartServiceByName and reloads policies installed with the
GNOME packages. The session harness then has to create/verify a real session.

Source review also showed that passing is-local=false is insufficient evidence
of non-local registration in this CK release: the final locality calculation
checks property presence. The harness now omits that override, queries IsLocal
and GetVTNr afterward, and refuses to launch unless the actual session is
non-local with VT zero. This keeps headless startup distinct from console/device
authority. Real-backend retest is pending; no desktop or laptop success is claimed.
