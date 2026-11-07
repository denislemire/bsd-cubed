# GNOME 49 ecosystem CI continuation — 2026-11-06

State: controller prepared and checked offline; the generic Wayland fixture has
passed a native Nautilus run; the combined ecosystem pipeline has not run.
All 23 ecosystem ports have now passed manual native package validation, and
the real session passes all 15 app fixtures in both startup modes. The controller
is wired after the existing core job in CircleCI; its complete native run remains
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
source archiving and recipe archiving run as ordinary circleci with private
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
