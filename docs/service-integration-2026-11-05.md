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
