# Real VT/device lifecycle — November 5, 2026

State: verified on the isolated stable/15 KVM; native CI integration pending.
No laptop changes or physical GPU presentation are claimed.

The test VM runs FreeBSD15.2-PRERELEASE,
`stable/15-n286217-84aa3cb3204d`, GENERIC. The bridge production code is unchanged
from PR9 (`87b98d1e7fa382bae9466d18a2b1ba2cbe30a002`). ConsoleKit2 is rebuilt
from the ports tree pinned at `2072800715389ae49066946c960e6521bac37e6a`, with
this repository's locality, device-state/revocation and console-selection patches.
Original test code is BSD-2-Clause; ConsoleKit retains its upstream license.

## Failure and prediction

A root launcher attached an unused ttyv8 as the child's actual controlling tty,
then irreversibly dropped to the existing ordinary user. Kernel VT9 was active,
and ConsoleKit reported the correct local session/VT9, but its active-session
state stayed false. The stock daemon had no console descriptor open.

Typed probes distinguished the available controls: `/dev/console` failed both
CONS_GETVERS and VT_GETACTIVE with ENOTTY; `/dev/ttyv0` supported both;
`/dev/consolectl` supported CONS_GETVERS but failed VT_GETACTIVE. Thus consolectl
would not fix the active-VT monitor. Source inspection found no FreeBSD ttyv0
fallback and a separate error path returning fd3 after all candidates failed.

Prediction: retaining the existing preferred console candidates, then trying
FreeBSD ttyv0 before inherited standard descriptors, restores real VT tracking.
Returning -1 after all candidates fail prevents use of an unrelated descriptor.
The distinct port patch preserves the base port's runtime-directory patch.
ConsoleKit PORTREVISION advances to5; BSD3 requires at least2.0.0_5.

## Actual stable/15 result

As the ordinary user, build the complete port with DEVELOPER=yes and run
`stage check-plist stage-qa package`. The first sparse checkout omitted framework
Keywords and packaging failed; adding Keywords resolved that tooling failure.
No plist issues were found. Stage QA warned about apparently unused library
links; this does not disable the ioctl-based device code. The generated VM-only
package SHA256 is
`af97b59bb46cd3a8a63871748819dfe4b0d3f33ca4c1a0aa911c4e68546c4168`.
The existing VM root console installed a protected, hash-verified copy after
stopping only the verified ConsoleKit bus owner's process. This local package
is experimental evidence, not a release artifact for the laptop.

The strict nonlocal session regression passed with actual IsLocal=false and VT0.
The inactive input test passed: inactive=true, EVIOCGVERSION returned ENODEV,
and poll reported POLLHUP, including after release. No input events were read.

The root fixture then attached ttyv8, dropped privilege and activated VT9.
The ordinary client verified its real local session was active, acquired an
input descriptor, switched to the original VT through BSD3, observed a directed
PauseDevice signal and kernel revocation, and was refused an inactive SwitchTo.
After the supervisor returned to VT9, the client received a directed ResumeDevice
with a fresh usable descriptor. The old descriptor remained revoked. Release
revoked the fresh descriptor. The fixture reported scenario=PASS and recovery=PASS,
with the original VT1 restored.

The unchanged full GNOME harness also passed on this corrected stable/15 VM:
strict nonlocal registration, normal RequiredComponents, Shell47.10, a mapped
GTK Wayland window receiving five frame ticks, and clean logout. Calendar timezone
and IBus shutdown warnings remain in private evidence; this is not broad desktop
feature acceptance.

## Recovery and limits

The launcher refuses execution without an explicit disposable-test flag, root,
KVM, an ordinary target account, and a disabled ttyv8 entry. Only attachment and
recovery run privileged; session/device calls run after setuid. A bounded parent
supervises the child and restores VT mode, keyboard/display modes, termios,
permissions, ownership and the original active VT. It reads back modes,
permissions, ownership and the active VT. The test never reads input events.

An initial negative test exposed a fixture error: reusing the session tty for
global console recovery failed after session teardown. The existing root console
restored VT1 explicitly. The corrected fixture uses independent ttyv0 control
and reopens handles after teardown. The same invocation then returned
recovery=PASS even while the stock-backend scenario failed. This negative-path
verification preceded the successful corrected-backend scenario.

CI keeps the privileged launcher in a separate private directory from the
ordinary-user test programs and records the VT result before Mutter/GNOME builds.
This verifies actual kernel VT/input behavior in KVM. DRM leases, physical
keyboard/mouse behavior, visible GPU presentation, display-manager login and
laptop recovery remain acceptance work. GNOME47 is the baseline; newer GNOME
session startup remains separate work.
