# GNOME 49 native session foundation — 2026-11-06

Status: experimental foundation verified in a separate native FreeBSD 15.1
VM, including real GNOME49 manager lifecycle and optional ports package QA.
Full desktop integration, 15-STABLE qualification and signed publication remain
open. The working GNOME47 desktop is unchanged. No laptop or shared desktop
VM was modified by this work.

## Problem and smallest test

GNOME Session 49 removed the earlier built-in service-management path. Its
manager announces readiness before other session services initialize, then
waits for `org.gnome.SessionManager.Initialized` to advance to application
startup and the running phase. User environment updates previously targeted
`org.freedesktop.systemd1.Manager`. Emulating successful unit operations would
hide the missing lifecycle rather than provide a desktop.

The falsifiable first target is narrower: the actual 49.0 manager must start
under an ordinary-user native service graph, announce readiness, receive the
initialization signal only afterward, reach running, and log out without
leaving session processes or runtime sockets. Real upstream environment code
must update the environment of a subsequently started service. This test does
not establish a working GNOME 49 compositor, Shell, display manager or desktop.

## Architecture and scope

`bsd3-user-session` launches a private **Dinit 0.19.4** instance in explicit
`--user --container` mode. Its owned mode-0700 runtime directory is unique to
this login. Only the explicitly supplied service directory is searched. There
is no system-wide daemon, new PID 1, rc.conf enablement or unit-file parser.
Existing FreeBSD init/rc and the default BSD3 package install remain unchanged.
The `USERSESSION` port option is disabled by default; enabling it requires the
included `sysutils/dinit` dependency overlay until an official port is available.

Dinit implements dependency ordering, readiness gates, startup deadlines,
restart limits and dependency-ordered stop deadlines. Native service
definitions must choose finite deadlines and explicit restart policy. On
top of these, the launcher requires an acknowledged root-service start after
the control socket becomes ready, bounds complete startup to 120 seconds and
manager shutdown to 30 seconds. Dinit deliberately exits zero for some initial
service failures; a regression reproduced that misleading success, and the
explicit start acknowledgement corrected it. Forced deadline cleanup is an
error, not a successful normal logout. On FreeBSD the launcher also acquires
a process reaper: after Dinit exits it
kills and reaps remaining descendants of this login, including children that
left their original process group. It never kills every process of the UID.

The separate original `bsd3-user-session` library supplies:

* `bsd3_session_ready()`: notify once through the service-specific inherited
  `BSD3_READY_FD` pipe, close it, and report errors including EPIPE without
  changing the application's process-wide SIGPIPE disposition.
* `bsd3_session_setenv()`: update or remove a variable in the future-service
  environment through the selected owned private socket. This uses an explicit
  dinitctl path and literal argument vector, with a five-second response bound.
  It rejects root, invalid/reserved names, unsafe sockets and oversized values.

The pinned Dinit control protocol limits assignments to 1020 bytes in this
adapter. Tests exposed a longer development-machine PATH; it was correctly
rejected. Test environments now contain only the variables needed for the
experiment. A production environment exceeding this limit needs a separately
reviewed backend improvement; it must not be truncated or silently ignored.
Environment changes do not retroactively modify already running processes.

The upstream 49.0 consumer patch replaces only the selected manager readiness,
environment and application-scope hooks under a build conditional. A native
diagnostic build uses the existing upstream null system backend; its power
capabilities remain unavailable; native Shutdown/Reboot requests explicitly
return the standard D-Bus NotSupported error. Initialization-time environment
errors propagate to callers. The graph calls the actual manager's
`Initialized` method after manager readiness. Production must put Shell and
settings-service readiness before that signal, rather than simply reusing
the headless diagnostic graph.

All session services must be bound to the manager lifetime. A deliberately
started environment-test service initially lacked this dependency and kept
Dinit alive after manager logout. Adding the required dependency corrected
the test graph; the next native logout completed cleanly. An independently
started, unbound service is not automatically part of that graph. Future
application-service activation must preserve this lifetime contract.

## Licensing and provenance

All new project C code, headers, harnesses and scripts are original
BSD-2-Clause work. No systemd implementation was copied, translated, linked
or vendored. No systemd headers, pkg-config identity, service bus identity or
success-returning privilege emulation is supplied. API behavior and lifecycle
relationships are interoperability requirements, not copied implementation.

Dinit remains a separately distributed Apache-2.0 dependency, pinned to
`029f54d007386c815aee6b6d458d691c3bf95e15` / tag `v0.19.4`. Its source/license
stay in the dependency distfile; BSD3 includes only its port recipe. The
package deliberately excludes shutdown replacement tools and boot integration.
Upstream: <https://github.com/davmac314/dinit/tree/v0.19.4>.

The GNOME consumer patch modifies GPL-2.0-or-later upstream files at
`776dc42864ea233f732e0b5b72cc08b1d8664fdb` / tag `49.0`. Those modifications
and resulting linked GNOME executables retain upstream GPL obligations; the
patch directory explicitly says so. They are not relicensed BSD merely
because they are stored beside BSD3. See the patch directory's README.

## Validation record

Local Darwin build: original C11 code compiled with `-Wall -Wextra -Werror`.
Pinned real Dinit built separately in a temporary directory. The lifecycle
suite passed dependency/readiness ordering, literal environment set/unset,
reserved-name and oversized-update rejection, a closed readiness pipe,
startup timeout, bounded restarts, forced stop of an uncooperative service,
and private runtime cleanup. The actual patched GNOME 49 `gsm-util.c` compiled
against real GLib 2.90.0, and its native environment propagation test passed
on a repository-configured private D-Bus session. The first default macOS
D-Bus launch failed due to launchd integration; the existing private test bus
configuration corrected the harness without touching the host bus.

Native disposable VM: **FreeBSD 15.1-RELEASE-p6**, clang **19.1.7**, GLib
**2.88.3,2**, D-Bus **1.16.2_4,1**, GNOME Desktop **44.5**, Dinit **0.19.4**.
Ports revision: `1d4e733682f328a1862006677db1a3a3dba6d23a`.
The tested source was an uncommitted prototype on signed baseline
`29970b4a3f8b4eb56d770db67fbf9897cb263f88`; it must not be mistaken for a
published signed change. Native checks passed:

* Actual Dinit port compile, upstream sanitizer/unit/control tests,
  check-plist, stage-QA, package and installation.
* Original C11 user lifecycle, including missing/unready initial-root failure,
  dependency readiness, environment updates and rejection, restart limit,
  bounded stop, and FreeBSD reaper cleanup of an escaped descendant.
* Actual 49.0 environment consumer and actual manager readiness,
  initialization-time invalid/valid Setenv, environment delivery to a future
  service, Initialized/running, unsupported power refusal, late Setenv denial,
  normal logout and runtime cleanup. This is a headless diagnostic, not Shell.
* Real BSD3 port with USERSESSION enabled: stage, default protocol tests
  (17 passed; the missing-peer-credentials case appropriately skipped because
  this bus supplies credentials), native user lifecycle tests, check-plist,
  stage-QA and package. Default local build/tests also passed with expected
  macOS credential-capability skips.

The native package hashes identify diagnostics, not a released desktop:

| Artifact | SHA256 |
| --- | --- |
| dinit-0.19.4.pkg | `3562e94fcf570e7382426d9d39479c7c617660c5d136bfda6530f3e668467138` |
| bsd-cubed-0.1.0.b3.pkg (USERSESSION enabled) | `a4b8499e3da5cb30b1263884bf02cf5f006c80fa01d7e1e0bb2b5eaf3e76529e` |
| Matching BSD3 source snapshot | `6fe6e92a76f346f8efcf4ce688692b206ee67ba8077be6b0b19de0fc1f53ab01` |

Port preflight failures were corrected before these passes: use the standard
GitHub DISTVERSIONPREFIX for the archive name, link FreeBSD's realtime library,
create the package output directory before invoking the ports framework, use
standard option target helpers, and declare the versioned Python build/test
dependency through USES. The temporary ConsoleKit idle/sleep inhibitor was
verified live, with its canonical suspend:idle spelling; native daemon
detachment and root-owned pid/log reads were necessary. Its release was
verified before the disposable VM and generated key were removed. Raw logs,
immutable controllers, complete source, artifacts and SHA256 manifest were
exported privately and verified. Critical tested source files were compared
byte-for-byte with this worktree.

The separate `freebsd-user-session49` CircleCI job is prepared but has not run.
Commit signing failed twice through the configured 1Password signer; signing
was not bypassed and no PR was published. The guest also resolved the package
mirror through external DNS and received 403 while the Mac's internal route
served the anonymous artifact. The qualified published ConsoleKit package was
transferred and checksum-verified for this VM. CI needs the lab's correct
resolver/artifact route. These results do not establish 15-STABLE coverage.

Still open: native GsmSystem tracking and privilege-operation backend,
Shell/Mutter 49 native ports, Shell readiness and session environment updates,
settings-service graph, application/session lifecycle, GDM49 userdb and login
integration, native 15-STABLE coverage, and eventual physical acceptance. No
GNOME49 desktop entry or laptop package is installed by this experiment.

## Publication and shared-library follow-up

Later on November 6, signed commit
`e7364dc73eaeac55cd0e0de39f0efd00a3d16be0` was published as
[draft PR23](https://github.com/denislemire/bsd-cubed/pull/23); GitHub verified
the signature, Denis's author/committer identity and DCO. CI's root controller
needed a separate ordinary-user checkout, corrected in signed `68b7b6b`.

The actual Shell49 shared-library link exposed an executable-only `environ`
dependency in the SDK. The helper now receives a minimal explicit PATH/LANG
environment; it inherits no caller credentials, bus or agent environment.
Literal set/unset propagation tests and the actual49 consumer passed locally.
The actual FreeBSD15-STABLE `-shared -Wl,-z,defs` consumer link also passed
with pthread. Tested API source SHA256:
`d1c4e2a767d60cf741d61d9944b3584e2ab0677ad0bd881dc5d5fc428fc870c2`.
The corresponding native test archive hash is
`5d8e2efaeaa50f37983ef7270ed999558089bf1504f98da5d57d3503d457c94d`.
This is a library-link qualification, not complete GNOME49 desktop acceptance.


## Native ConsoleKit system backend and alpha version

The original C11/BSD-2-Clause `gnome49-system.c` implements the upstream
GsmSystem interface without libsystemd. Production binds only to the root-owned
ConsoleKit service and its current-process session, requiring the caller UID and
Wayland session type. Activity and idle state are real queries/operations.
Inhibitors hold the actual ConsoleKit FIFO lease, with checked replacement,
release and finalization. Power operations remain explicitly unsupported.
Session removal, service-owner loss and bus connection closure invalidate the
binding and close its lease. The native backend sources install with the SDK;
GNOME consumer patches retain GPL-2.0-or-later and do not relicense GNOME.

The private-bus test passed on 2026-11-06, including spoofed identity/session,
malformed inhibitor descriptors, backend loss, closed-bus notification without
a further method call, and power denial. The real ConsoleKit test passed on
FreeBSD 15.2-STABLE in the parent validation VM: current-process binding,
activity, reversible idle state, inhibitor replacement/release/finalization,
and unsupported power capability checks. This is protocol validation, not
production GNOME49 graph or hardware acceptance.

The next candidate version is `0.2.0-alpha1` (`0.2.0.a1` in pkg/pkg-config),
with USERSESSION enabled by default so the normal port supplies Shell49's SDK
dependency. The existing beta3/beta5 artifacts remain immutable. There is no
release distinfo for the unpublished alpha: CI generates a checksum for its
exact source snapshot via makesum and exports `dist/snapshot.distinfo`. Release
publication must retain that source and checksum and restore release distinfo
before the port is submitted or used as an upstream release port. The SDK's
installed original backend sources are input to the licensed GNOME consumer
build, not a claim of a complete session49 port.


## Foreground readiness and initial production graph

The native FreeBSD helper test now passes exact child-PID bus ownership,
readiness, preowned-name refusal, early process failure, ordinary SIGTERM exit0,
and unexpected owner-loss failure. Native compilation first exposed hidden
FreeBSD typedefs under POSIX feature macros; the macro is now scoped to other
platforms. Runtime then exposed expected bus-name loss during normal shutdown
being counted as failure; an already-stopping guard fixes this and the negative
owner-loss case still fails. No dummy readiness is reported.

Actual Dinit tests pass session-manager SIGTERM and SIGKILL with an independently
started stubborn service, bounded cleanup and supervisor exit0. A scripted
shutdown service initially deadlocked: dinitctl waits for Dinit closure while
Dinit waits for scripted startup. A process shutdown service removes that cycle.
The explicit graph definitions use this tested form. The ordinary root target
starts only after required services' readiness, then calls the real Initialized
method. Compositor and manager loss chain to whole-user-graph teardown.

The GPL consumer patch now includes an explicit FreeBSD-only Meson option and
real native backend source, skipping Linux launchers and systemd unit install.
A recipe/checksum/manifest is present for actual49.0, but its full native build
and production mixed49/SettingsDaemon47.2_1 graph are still open qualification.
The seven settings services' public names were audited from upstream47.2.
Their exact desktop IDs are excluded only in the native consumer to prevent
duplicate autostart; upstream systemd behavior is preserved otherwise.
