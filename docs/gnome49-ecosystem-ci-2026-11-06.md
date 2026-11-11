# GNOME 49 ecosystem CI continuation — 2026-11-06

State: controller prepared and checked offline; the generic Wayland fixture has
passed a native Nautilus run; the combined ecosystem pipeline has not run.
All 23 ecosystem ports have now passed manual native package validation, and
the real session passes all 15 app fixtures in both startup modes. The controller
is wired after the existing core job; its complete native run remains
pending. Manual component verification is distinct from that combined CI result.

`tools/ci-ecosystem49.sh` is intended to follow `tools/ci-gnome49.sh` on the same
**disposable FreeBSD executor**, with root controller identity and CIRCLECI=true.
It requires the existing BSD3_CI_PORTS_ROOT at exact ports revision
`2072800715389ae49066946c960e6521bac37e6a`, native ConsoleKit2 2.0.0_5 and
bsd3-user-session SDK >=0.2.0.a3. It does not provision a new VM/framework or run
against the laptop. The already qualified preceding pipeline still supplies the
SDK, Dinit, GJS, schemas, Mutter, Shell, Session and corresponding sources.

## Order and dependency protection

The fixed serial order is GOA → EDS → libshumate → Tecla → Backgrounds → GSD →
bsdisks → Settings → Calculator → Clocks → Characters → Font Viewer → Baobab →
Text Editor → System Monitor → Nautilus → Epiphany → Calendar → Console → Maps →
Papers → Shell Extensions → Evolution. Canonical origin directories in the pinned
framework receive the actual snapshotted recipes before dependency enumeration.
Thus dependent ports see the overlay's versions, not older stock metadata.
Tracked framework changes must initially be clean; the resulting reviewed overlay
diff is retained as evidence, rather than pretending the framework stayed pristine.

The root controller snapshots its own program, recipes and test sources into
root-owned, actor-readable control storage before ordinary compilation. Root
Makefile evaluation is limited to protected framework/recipe paths. A source
Makefile cannot replace a controller/test recipe root later executes. The root
checkout/framework ownership checks intentionally fail instead of silently trusting
an actor-writable cache; the existing pipeline must prepare their ownership.

Stock provisioning enumerates only the direct build-depends-list and
run-depends-list, deduplicating canonical origins/flavors. It does not recursively
install build toolchains for already installed binary dependencies: pkg resolves
binary transitive dependencies itself. Only missing nonreserved direct package
names are explicitly requested. Every custom
provider is excluded from stock requests; existing core providers and subsequently
built apps are locked, with package identities and installed checksums verified
before/after provisioning and installs. A missing/incompatible dependency fails;
`USE_PACKAGE_DEPENDS_ONLY=yes` prevents building a hidden stock port fallback.
No public package replacement of a custom provider is accepted. Existing stock
packages below a source-required version still need an explicit reviewed dependency
update; this controller does not mask such a failure by lowering a check.

Only root performs trusted package/service provisioning. Builds, upstream tests,
source archiving and recipe archiving run as an ordinary test account with private
home/work/dist/package directories and a minimal environment. Rust has two Cargo
jobs and MAKE_JOBS_NUMBER=2; other builds use three jobs. All 23 components are
serial. Each component/graph phase obtains a bounded idle/sleep inhibitor, verifies
its PID in ConsoleKit ListInhibitors and releases it in finally cleanup. Commands
have explicit deadlines bounded by the component's 6600-second window; losing the
inhibitor aborts that command. Child process groups are terminated with a bounded
escalation. No permanent power-policy setting is changed.

## Checks wired into the candidate

Stage, check-plist, stage-qa and package must succeed. Any line beginning `Error:`
in the build log independently fails the candidate even if make returned zero.
The actual package name/version/origin is checked against protected recipe metadata
before the root-owned copy is installed from local artifacts with remote dependency
fetch disabled. Root copies actor outputs with no-follow directory descriptors and
requires ordinary actor-owned, single-link regular files; it never follows an
actor-controlled archive/log link into privileged files. Partial safe logs are
collected on failure without replacing the original worker failure.

Known tests are explicit rather than inferred from a package build:

- Calculator: all 7 upstream Meson suites, unchanged expectations.
- System Monitor: all 3 suites, allowing Meson to build their actual test targets.
- libshumate: all 16 suites under isolated software Wayland.
- EDS: all 100 CTest cases, serial within a private home/bus/display fixture.
- bsdisks: its real CTest target exercising the 8 production-property cases.
- Nautilus: all 19 upstream suites under Wayland, including thumbnails, plus
  `native-nautilus49-autofs.py` against the actual patched source. This does not
  mount autofs or establish live automounter behavior.

Suite discovery counts alone are insufficient. After each actual Meson run, the
controller preserves meson-logs/testlog.json and requires exactly the expected
number of unique results, all result=OK and returncode=0. SKIP, EXPECTEDFAIL,
UNEXPECTEDPASS, failures, timeouts and missing records fail. CTest writes a retained
JUnit XML receipt: every expected case must have run, with no skipped/disabled,
error or failure cases or nonzero summary counters. Those checks happen before
package installation. Failed execution still retains available result receipts
and its original failure status.

`tests/native-wayland-tests.py` accepts `--output DIR --cwd BUILD --timeout N --
COMMAND ARGS` inside an ordinary private dbus-run-session. It creates a private
home/config/cache/runtime, starts headless Mutter with a virtual monitor and
software rendering, checks actual compositor protocols with wayland-info, publishes the future
private home/runtime/display to that bus before Mutter or GTK can activate GVfs,
and bounds command/compositor cleanup. Nautilus runs its 19 suites serially to
avoid introducing a shared GVfs race. No --systemd or local-GIO workaround is used.
It requires normal compositor exit after tests; no real DRM/VT/hardware claim is
made. It does not impersonate a GNOME session manager or provide success stubs.

After all installs, the existing real CK-backed GNOME graph runs twice, isolated
and system-autostart, with GSD 49.1, actual Settings 49.9 About UI and app fixtures.
BSD3_TEST_GNOME_APPS=yes and the extra selector
`nautilus,gnome-console,gnome-maps,epiphany,gnome-calendar,papers,evolution`
exercise the functional helper. The existing fixture remains responsible for real mapping/input/PTY/quit
checks; this controller does not invent coverage for apps not yet in that helper.
Strict logout/owner/socket cleanup still applies. The parent reported a Tracker/
Localsearch SQLite `nco:creator` warning during Nautilus search tests. It is not
silenced; a passing fallback test is not claimed as working indexer integration.

## Reproducible source and artifact boundary

Each component retains compiler/FreeBSD/ABI, exact recipe make arguments, package
inventory/provider identities, build/test/QA/install logs and these artifacts:

- the exact built package, checked against canonical origin/version;
- actual patched source, retaining original licenses/notices;
- actual canonical port recipe, including its patches/options/license metadata;
- checksum-qualified upstream distfiles used by that recipe.

Papers explicitly places CARGO_VENDOR_DIR inside WRKSRC before extraction/build.
Its source bundle includes that real vendor directory. All 253 registry sources
are additionally verified against the actual Cargo.lock, preserved as an explicit
locked-crates archive and included in the upstream-distfiles bundle. Git Cargo
inputs fail rather than remain unpreserved. Local Rust workspace crates remain
in the source tree; dependency licenses/notices are not removed or relicensed.
This captures the corresponding source for statically linked Cargo dependencies.
The prior pipeline's SDK/runtime sources remain in their existing dist artifacts;
a complete release must combine those with this continuation, not publish only
the new application packages or omit dependency source bundles.

Artifacts live in dist/ecosystem49 and are checksum-inventoried after successful
completion. Failure outputs remain partial evidence and have no qualification
marker. Raw diagnostic output stays in private CI artifacts; review identifying
excerpts/source publication separately. A successful marker refers to this native
pipeline run only: hardware, greeter, network services/indexing and laptop rollout
remain explicitly outside its claim.

Offline checks performed: shell syntax and shellcheck; controller self-tests for
23 unique origins/provider order, exit-zero stage-QA Error rejection and no-follow
artifact collection; fixture self-tests for actual bounded command completion,
nonzero status propagation and timeout cleanup. No platform identity was faked,
and no compositor/bus/native test/build was started in these self-tests. Native
combined controller/executor/ports/package and final graph execution remain pending.


Native fixture checkpoint: the parent ran this generic Wayland helper successfully
on the existing FreeBSD lab VM with all 19 Nautilus suites. The parent also
corrected the consumer test's Linux bubblewrap prerequisite on FreeBSD, where
native gnome-desktop44.5 was verified built without HAVE_BWRAP; the actual thumbnail
test then passed rather than returning skip77. These changes belong to the
parent's port, not this controller. The fixture's native result does not establish
the combined 23-component controller, package pipeline, indexer, hardware or greeter.
Root-owned checkout/framework ancestors remain an explicit execution prerequisite;
that boundary is not relaxed to accommodate an unknown CI checkout location.


### WebKit library content probe

The parent compiled `tests/native-webkit-view.c` with C11/-Wall/-Wextra/-Werror
and the installed webkitgtk-6.0 flags, then ran it under the actual FreeBSD
Wayland fixture with the working desktop setting
`WEBKIT_DISABLE_COMPOSITING_MODE=1`. It passed FINISHED, unique per-run DOM
title/body tokens and a visible 800x561 GdkTexture snapshot: 227647/448800 pixels
differed from the reference pixel and the two CSS bands differed. The PNG was
saved privately; the GTK window and enclosing compositor closed normally.

The ecosystem worker now compiles that original BSD2 fixture as the ordinary
actor after the Epiphany build and runs it under the bounded Wayland helper with
that exact setting. Receipts include fixture source, compile command/flags and
WebKit version, compile/runtime logs and the actual PNG. The complete controller
has not run. This establishes a local **WebKit library** DOM/render baseline;
it does not prove Epiphany's own injected script, page content, network requests,
forms handling or normal quit. The parent's separate Epiphany normal-quit
assertion investigation remains open and is not bypassed by this successful
library probe.


The parent identified Calendar startup failure on an otherwise bare VM with no
/etc/localtime or /var/db/zoneinfo: the real source fallback could not construct
a local time zone. CI now provisions UTC with tzsetup -s UTC **only when
/etc/localtime is absent**, before GUI tests, and records its action/checksum and
zoneinfo receipt. An existing file or symlink is preserved, even if invalid and
requiring a separate diagnosis. No configured timezone or timedate1 service is
replaced or impersonated. This provisioning has not been run by the agent.

### Preflight controller correction

Review against the exact pinned ports framework caught a controller integration
error before the ecosystem phase: Meson places tests in TEST_WRKSRC (_build),
while CONFIGURE_WRKSRC is the source tree. The controller now queries TEST_WRKSRC
for both Meson and CMake tests; CMake sets it to its configured .build directory.
CI61 was superseded before reaching this phase to avoid consuming the rest of a
known-invalid run. This corrects test orchestration, not a consumer build.

The final receipt review also found that root collected complete.json without
enforcing it. Before installation, root now requires the exact controller-defined
source/recipe/upstream/package/provenance list, plus both Papers Cargo inputs,
and nonempty regular copies of each. Offline checks remove and truncate every
required input and reject actor attempts to shorten the contract. The release
curator independently requires the same corresponding-source inventory.

### CI67 provider handoff

CI67 passed the core build and native Session49.3 tests, then stopped before
building ecosystem ports because its provider check used ConsoleKit2, the
upstream source name. The actual CI67 package manifest and a native installed
package query report `consolekit2-2.0.0_5 sysutils/consolekit2`. The controller
now uses that exact package identity for lookup, locking and provider protection.
The failed combined run is evidence for diagnosis only, not a release candidate.

### CI70 package collection and native worker reproduction

CI70 passed all preceding core stages, then built/staged/checked/packaged Online
Accounts successfully. The worker failed collecting its package because PACKAGES
did not exist when make first parsed bsd.port.mk. The pinned framework only
enables repository output when that directory already exists; the package was
created under WRKDIR/pkg instead of the expected repository. The worker now
creates its private package directory before evaluating make metadata or targets.

A focused ordinary-user reproduction on the preserved generic 15-STABLE VM
then exposed PosixPath arguments in the JSON build-command receipt. Those are
now explicitly normalized to strings, and the provenance query names the native
package executable by its absolute path. The first lab harness mistakenly
trimmed PATH below the real controller's environment, generating false unowned-
library QA errors. Repeating with the controller's actual full system path
restored valid QA; no QA check was weakened.

The corrected real worker now emits complete packages, patched source, recipes,
upstream distfiles and provenance for Online Accounts3.56.5 and Shumate1.5.5.
Shumate's16 actual Meson results are all OK/returncode0. Completion manifests,
nonempty artifacts, ABI and serialized command arguments were read back from
those native receipts. The lab probe used existing source overlays directly; it
is not a successful combined CI run or a release.

The same actual worker subsequently passed EDS3.58.3 stage/plist/QA/package and
all100 CTest cases under the isolated Wayland fixture, with complete nonempty
source/recipe/provenance receipts. A separate actual Papers extraction using
the worker's WRKSRC/vendor layout populated that directory and checksum-verified
all253 original registry archives against Cargo.lock; this was not a second
Rust compile or a full Papers-worker claim. The transferred worker script's
checksum matched the final locally checked source.

### CI73 old-kernel process-environment failure

CI73 stopped in the unchanged VT test before the ecosystem stage: CK registered
a local VT9 session, but the client could not resolve its session ID. Recovery
to VT1 passed. CK logged kvm_getenvv failure without its underlying errno; this
diagnostic alone does not prove which kernel error occurred.

The exact RELEASE-p6 kernel source95719d8a895b retains the faulty partial-read
path described in FreeBSD PR297512. The upstream report links this defect to
ConsoleKit process lookup and GNOME unlock failures. Reviewed stable/15 fixes
c9b5dd2eb820 and75bf78a0ed8d landed August16. Official Git ancestry confirms the
recorded lab and laptop source bases include the fixes. This
does not establish the cause of the laptop's separate frozen-resume incident.

An original C11/BSD2 bounded probe temporarily gives only its own synthetic child
a single environment string ending at an unmapped page boundary, with a mapped-
following-page control. It never reads actual environment values or other users'
processes. The real STABLE VM passes both cases with exact content and length.
The explicit legacy mode requires the control to pass and boundary read to fail
with ENOMEM; its output explicitly says NOT A QUALIFICATION. CI records this
capability before building. An unrecognized outcome fails the diagnostic.

The observed CI73 lookup failure is consistent with this known older-kernel
defect; its original errno was not retained, so attribution remains an inference.
Neither session identity checks nor VT tests are bypassed, relaxed or retried
inside their fixtures. ASLR is unchanged. The CI RELEASE image still needs the
upstream kernel fixes; the target remains STABLE. A successful new combined run
and exact-package STABLE/hardware validation are still required.

Sources: https://lists.freebsd.org/archives/freebsd-bugs/2026-August/036119.html
and https://github.com/freebsd/freebsd-src/commit/c9b5dd2eb8200eb9be6795fd2b0774991d85e1ef
plus https://github.com/freebsd/freebsd-src/commit/75bf78a0ed8d3e108532c8893f5dad8bdb2b0b3e.

The same original probe was then compiled and run as the ordinary laptop user
inside a private temporary directory, which was removed afterward. Both mapped
and unmapped-boundary reads returned the exact29 synthetic bytes successfully.
This directly verifies that the actual unchanged laptop kernel does not exhibit
this specific defect; it does not explain its separate resume/input freeze.
The STABLE VM also correctly rejects the explicit legacy expectation.

### CI76: Papers toolchain selection (November 7 follow-up)

CI76 classified its RELEASE kernel as affected by PR297512 (mapped control
succeeded, unmapped boundary returned ENOMEM), then passed the unchanged core,
VT, compositor, Shell and Session checks. The ecosystem controller built,
checked, installed and collected complete receipts for the first20 ports,
including the actual upstream suites and WebKit DOM/pixel probe. The remaining
Papers, Shell Extensions and Evolution stages and final graph were not reached.
The downloaded20 inventories verified328 nonempty files; the job omitted only
28 empty diagnostic logs, recognizable by their exact empty-file digest. Required
package/source/test receipts remain nonempty and independently enforced.

Papers stopped before compiling: pinned cargo.mk requires Rust >=1.99.0, while
the ordinary quarterly resolver installed1.98.1. Signed FreeBSD latest exposes
rust1.99.1_1; its only runtime package dependency is curl8.23.0, already present
in CI76. The controller checks the toolchain before rebuilding20 other ports.
It uses a root-private, fingerprint-verified latest catalogue to fetch exactly
one Rust archive and installs that local file with repositories disabled. The
normal dependency repo remains unchanged. It verifies package identity, records
archive SHA256 and compiler version, and rejects any non-Rust package change.

A native rehearsal showed that installing directly from the scoped latest repo
still upgraded pkg itself; that approach was discarded. Fetch output can use
All/Hashed rather than the plain All path, so the final helper selects exactly
one regular archive inside its trusted fetch root and validates its manifest.
No Rust minimum, source checksum, package signature, QA or session test was
relaxed. These changes affect CI toolchain provisioning, not laptop software.

On the native 15-STABLE rehearsal VM, the final helper installed signed Rust
1.99.1_1 from an absent compiler state and kept every other installed package
identity unchanged. The ordinary worker then compiled Papers 49.8 from scratch,
passed stage QA and check-plist, and produced the package and all seven required
receipts. Its provenance reports rustc 1.99.1. The locked-crate archive contains
253 unique crates matching the Cargo.lock checksum count, and the patched source
contains the vendored dependencies and their license files. This rehearsal used
cached, checksum-verified distfiles; a combined successful CI run remains the
release gate.

The same native worker then staged, checked and packaged GNOME Shell Extensions
49.1 and Evolution 3.58.3, with complete package, recipe, patched-source,
upstream-distfile and provenance receipts for each. Separately, the existing
VM stack (BSD³ SDK alpha2, Session 49.3, Shell 49.10, Settings Daemon 49.1)
passed the current headless integration fixture in both startup modes: 15 apps,
including Web's two-window quit test, plus Settings 49.9 About and normal session
logout and ownership cleanup. These were development checks against the VM's
locally built stack, not qualification of the final CI package bytes, a graphical
greeter, or laptop hardware.

### November 7: final desktop replay

CI79 and CI82 built the eight core packages and all 23 ecosystem ports. Both
stopped at the first Web window on 15.1-RELEASE-p6. The exact CI79 packages
passed integrity and dependency checks and both full 15-app graphs on
15-STABLE, including Settings 49.9 About and normal logout. A separate VM
using the CI image reproduced the failure with the CI82 packages. Its
backtrace reaches GIO's D-Bus cancellation source during Web initialization.
The underlying cause remains open.

The target VM's first complete rebuild stopped in Evolution Data Server.
Earlier test runs left fixed-name cache directories in `/tmp` under another
account. The same binaries passed all 100 upstream tests with a private
temporary directory, and all 100 passed again with the updated Wayland test
helper providing that directory. The helper also passes it to services
activated by the private session bus.

The native controller then completed on 15-STABLE `646a485c3b2f` at
`43b1659` plus the private temp-directory fix. It used the eight verified
CI79 core packages and rebuilt all 23 ecosystem ports. Both full Wayland
graphs passed, including Web's two-window quit check, Settings About, and
session logout. The controller emitted a qualification receipt and full
artifact hashes. The native runner still uses 15.1-RELEASE-p6.
