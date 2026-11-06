# Native GNOME49 candidate qualification — November6,2026

State: complete native headless user session verified; combined CI, release,
repository rollout and laptop49 acceptance remain open. The physical laptop
continues its working GNOME47 boot environment. No laptop package, service,
kernel, Wi-Fi configuration or boot selection changed during this qualification.

## Exact native environment

FreeBSD15.2-PRERELEASE, stable/15-n286624-646a485c3b2f, clang21.1.8,
GLib2.88.3. The generic KubeVirt VM has no DRM device. The selected consumers
are GNOME Session49.0, Shell49.10, Mutter49.8, GJS1.86.0, desktop schemas49.1,
and the installed FreeBSD GNOME Settings Daemon47.2_1. Settings services are
qualified at that exact47 version; this is not a claim that all settings or
other GNOME applications were upgraded to49.

The native source references, portability decisions and licenses are recorded in
[the session investigation](gnome49-native-session-2026-11-06.md),
[Shell](gnome49-consumer-ports-2026-11-06.md),
[Mutter](native-mutter49-2026-11-06.md), and
[runtime dependencies](gnome49-runtime-dependencies-2026-11-06.md).
The original BSD3 core remains C11/BSD-2-Clause. Dinit0.19.4 retains Apache-2.0;
GNOME consumer changes retain their GPL/LGPL terms. No systemd implementation
is copied into the original BSD files, and Dinit is an ordinary per-login
supervisor with no boot service or PID1 replacement.

## Verified behavior

The actual native ConsoleKit adapter authenticates its unique root-owned bus
peer and current process/session UID/class/type, monitors activity and lease
loss, updates real idle hints and manages actual inhibitor pipe descriptors.
Private-bus negative tests and real ConsoleKit tests passed. A closed bus
invalidates leases and closes descriptors without requiring another request.
Unsupported privileged power operations are refused rather than advertised.

The installed SDK graph starts the actual49 manager, then49 Shell, then seven
foreground settings services. Readiness checks authenticate each exact child
PID owning its declared bus name. Only after those checks does the real
Initialized call advance the manager. The full trial used a real temporary
ConsoleKit session, a private session bus/profile and software rendering.

Both isolated and actual system-autostart directory trials passed: the manager
reported running, Shell's actual D-Bus version was49.10, an ordinary autostart
app received the future Wayland display and mapped a real GTK window, and each
of the seven service names had exactly one matching foreground process.
Logout returned0, all graph-owned bus names disappeared, and the private
control socket was removed. The final test rejects per-service failures,
unexpected termination signals and forced stop-deadline cleanup.

The last clean native GNOME Session build included the actual GsmApp phase fix:
exactly seven graph-managed Initialization entries are accepted and disabled
for legacy autostart, while normal applications remain enabled. Unsupported
phases and near-match IDs are refused. Its actual API test and subsequent
production-directory full session trial passed. Other optional legacy47
initialization services are not silently claimed as provided.

## Failures preserved and corrected

A first full graph rendered and logged out but reported a settings wrapper
failure during teardown. That pass was insufficient for clean-service acceptance.
The wrapper had interpreted the service's legitimate session-end name release
as unexpected failure. It now pins the real manager's unique owner and accepts
its authenticated SessionOver followed by clean child exit. Spoofed signals,
premature exit and nonzero exit after SessionOver remain failures. The native
fixture and the stricter full graph subsequently passed without service errors.

An earlier scripted Dinit shutdown service deadlocked: dinitctl waited for the
supervisor while scripted startup waited for dinitctl. A process shutdown
service removes that cycle; normal logout, fatal manager termination and
stubborn independently started service cleanup were tested. FreeBSD helper
compilation also required BSD feature visibility for procctl types.

A lab storage outage interrupted staging. The same persistent VM disk was
recovered, checked clean, and installed package integrity verified. Interrupted
build products were discarded; the affected consumers were rebuilt and tested.
These are VM incidents, not evidence of a laptop or BSD3 device failure.

## Identified VM prototypes

Combined CI55 passed the ordinary-user SDK and GNOME49 consumer tests but
failed the later ports target because its controller ran the lifecycle suite
as root. The intentional root refusal remained correct. The ports controller
now snapshots inputs into a private build directory and delegates the complete
stage/test/plist/QA/package sequence to an ordinary account with a minimal
environment. Root copies the successful regular package artifact back afterward.
On the same native STABLE VM, this controller passed from a root-only checkout
using `BSD3_PORTS_TEST_USER=bsd3`, with USERSESSION enabled and no skipped
lifecycle tests. Package creation and manifest/QA checks passed; existing
unstripped-binary warnings remain. Combined CI must still repeat this result.

CI56 subsequently passed that packaging step, service policy, CK registration,
input revoke/resume and the production VT bootstrap. Its nested production
launcher reuse check failed at ConsoleKit GetSessionForUnixProcess; the captured
VT recovery passed. This remains an investigated regression, not a successful
combined candidate. The pipeline stopped before building the49 consumers.

A separate real GJS probe showed GStreamer init_check returns a two-element
GI tuple. The Shell adaptation now checks its first boolean explicitly. The
new extracted-method test failed against the prior source for a false tuple,
then passed with the correction, including missing-element and complete-pipeline
fixtures and the actual installed GI return shape. This establishes capability
handling, not a working screencast or PipeWire capture implementation.

The prepared49 hardware profile wrapper uses its own bsd3-gnome49 directory
because the laptop shares its home dataset between boot environments. It retains
the verified WebKit fallback and launches the packaged49 session entry point.
Only syntax has been checked for this wrapper; hardware integration remains open.

These identify diagnostic native artifacts, not published hardware assets:

| Package | SHA256 |
| --- | --- |
| bsd-cubed-0.2.0.a1.pkg, final teardown helper | cc0b64c72d4b0e7e95e0a4f77c9f55643033536ec65931febad6931f70e248aa |
| gnome-session-49.0_1.pkg, final phase handling | a03833c4f9129ee9ec598f087b038ba2104d6de72517511aa12f9707022d66dd |
| gnome-shell-49.10.pkg, SDK version guard | d1e8e483827b7c51e13eb7168f9086ecf754f8cbbbb70d2a4d83000e3be6cc98 |

The combined pipeline rebuilds one coherent candidate in one disposable
FreeBSD machine, retaining old47/device/VT regressions before replacing the
runtime. It repeats native49 tests and exports matching source, recipe,
checksums and actual toolchain/OS provenance. Its configured image alias is
freebsd-15.1:current; that must not be mislabeled as the separate15-STABLE VM.

## Remaining hardware boundary

Publish successful native CI assets and corresponding source/recipes before
hardware installation. Preserve the working kernel/Wi-Fi packages, recovery
boot environment, desktop applications and per-user profile. The existing
SDDM greeter uses Xorg; the native49 launcher currently supports the user
session, not GDM47's separate greeter session. No GDM49/userdb support or
Xorg removal is claimed by this candidate.

The next accepted device trial must establish actual DRM/input/VT behavior,
login/logout to the greeter, browser rendering and normal boot in a separate
candidate environment. Avahi still needs enabling there for hpbsd.local.
Airwave alpha2 is separately published from successful native CI15 and tested
with the49 VM libraries; its real-radio reliability remains a separate test.

## First combined CI followup

CI54 at03b6419 failed in the native user-session consumer tests. After
1Password was restarted, the actual CI log confirmed: dbus-run-session failed
to exec python3 because that executable was absent. The job installs python312
and selects its versioned executable, but two actual-GNOME diagnostic controllers
and their initialization child still hardcoded python3. They now honor PYTHON
and sys.executable. Both actual upstream consumer environment and diagnostic-manager
lifecycle tests subsequently passed on the native VM with only the versioned
interpreter. A new complete CI run is required; the laptop has not been changed.
