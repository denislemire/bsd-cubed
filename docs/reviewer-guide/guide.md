# BSD³: A native desktop bridge for FreeBSD

## Abstract

BSD³, the BSD Desktop Daemon project, explores a small native boundary between the desktop interfaces GNOME expects and the session, device and console facilities available on FreeBSD. Its executable is `bsddd`. Original implementation code is C11 under BSD-2-Clause. FreeBSD retains its own init system, rc scripts, PAM, devfs, process model and kernel. The project does not implement PID 1, parse arbitrary systemd units or pretend to provide the complete libsystemd ABI.

The work has progressed from typed session discovery to authenticated controller lifetimes, descriptor transport, real kernel input revocation and VT switching, a native Mutter port, and full GNOME47 headless startup with a mapped Wayland application and logout. Integrating the actual display manager exposed additional boundaries: seat enumeration must work without an existing session, and the Wayland child needs a verified local registration before execution. Those changes and a separate bounded recovery tool are implemented in focused review branches.

The strongest completed exact-CI artifact checkpoint is CI33 at `dcf9744`. Its downloaded packages also passed real-session, input, VT and GNOME headless tests on a separately identified stable/15 VM. Later SDDM and recovery changes are pinned to `8bfa868`; their focused CI is still running. The exact reusable runner has now passed on the stable/15 VM using the separately identified local package. No physical DRM presentation, laptop login, regular-password authentication, suspend reliability or GNOME49-and-later user-service graph is claimed complete.

> Assessment: The implementation is a VM-validated experimental bridge and integration candidate. It is not yet a verified working GNOME Wayland desktop on the laptop.

[pagebreak]
## How to read this guide

This is an author-review draft prepared under Denis Lemire's name. Code, investigations and prose were developed with AI assistance and require the author's review; no maintainer endorsement or FreeBSD Project publication is implied. The report is intended to make the source, assumptions, negative results and verification boundaries inspectable rather than supply a completion percentage.

Read the architecture and identity invariants first, then the backend corrections, SDDM path and evidence matrix. The reviewer navigation appendix maps important claims to functions and tests. Commands describe isolated VM procedures, not permission to modify a running desktop or the laptop. Exact release assets must replace experimental local packages before any coordinated hardware trial.

Five evidence levels recur throughout: source inspection; private-bus protocol execution; native package/build checks; actual kernel/session behavior in a VM; and physical hardware acceptance. A higher-level name such as Wayland, a successful method reply, or an overall green build cannot silently substitute for a missing lower-level observation. Conversely, a deliberately failed negative test can verify the guard's refusal or recovery behavior without establishing general reliability.

[pagebreak]
[toc]
[pagebreak]

## 1. Objective and deliberately narrow scope

The immediate objective is a meaningful, recoverable GNOME Wayland trial on FreeBSD using BSD-native process and service facilities. The first compositor baseline is the existing GNOME47 package family: Shell47.10, session47.0.1 and settings-daemon47.2_1. This baseline permits an incremental experiment against a real ports recipe. It is not the final definition of support for newer GNOME.

There are at least four independent problems: implement the particular login1 and selected sd-login interfaces used by the compositor; compile and run the native compositor backend; register and supervise the ordinary user's desktop session; and validate real device ownership, console transitions and recovery. Newer GNOME adds another user-service startup problem. An existing SDDM menu item solves none of those by itself.

The project chooses C11 because the relevant interfaces already cross C, GLib/GIO, PAM and device APIs. It avoids introducing an additional runtime or FFI boundary before the ownership model requires one. Rust remains a possible future decision if the state machine grows; it is not necessary to make this boundary explicit today. Existing dependencies retain their licenses. The original bridge and the modified GPL dependencies must never be conflated as one relicensed codebase. [S1, S2]

| Boundary | Implemented or observed | Explicit limit |
| --- | --- | --- |
| Discovery | Typed CK calls; selected malloc-owned sd-login results | Selected ABI only; no monitors or cgroups |
| Controller/device IPC | Original peer checks, private backend lifetime, FD/signals | Not a complete logind service |
| Native compositor | Patched Mutter47.10 native backend; headless client/window | Physical DRM and accelerated presentation pending |
| Session entry | Strict local VT launcher and opt-in SDDM adapter | Regular password flow and other managers not established |
| Recovery | Native console snapshot/readback and bounded owned-process supervision | Cannot recover a frozen kernel or killed guard |
| Newer GNOME | Startup boundary audited at49; later version work planned | No replacement user-service graph implemented |

## 2. Source and environment identities

The report's implementation reference is signed commit `8bfa868f3a1c485954d35b9be28b2347b2e4bf30`, a stack of focused development changes. This is a source identity, not a claim that every test was run at that exact commit. CI33's earlier exact checkpoint is `dcf974485696e40381fa23a44db5a5d47d83ae3a`. Later cases identify their working-tree or focused-commit scope. The last completed verification must be distinguished from a newly edited source tree. [S1, E1]

| Item | Identified version or revision | Meaning |
| --- | --- | --- |
| Native CI image | 15.1-RELEASE-p6, Clang19.1.7 | Running, installed and userland versions recorded |
| Independent VM | stable/15-n286217-84aa3cb3204d GENERIC | All version entries15.2-PRERELEASE |
| Dependency ports | 2072800715389ae49066946c960e6521bac37e6a | Pinned Mutter/ConsoleKit overlay base |
| Stock CK | ConsoleKit2 2.0.0_2 | Reproduced locality/device/console problems |
| Corrected CK | 2.0.0_5 | Locality, revoke/state and console fallback patches |
| Stock / native Mutter | 47.10_2 / 47.10_3 | Native backend disabled / explicitly enabled |
| BSD³ candidate | 0.1.0-beta3; pkg0.1.0.b3 | Unreleased iteration; hash/commit required |
| SDDM used in VM | 0.21.0.36_7 | Real FreeBSD port, autologin/PAM helper path |
| Wi-Fi trial environment | 16.0-CURRENT for all version entries | Separate ongoing work; not the ABI15 target |

The current laptop environment was initially described incorrectly as a CURRENT kernel with STABLE userland. A fresh coordinating-session audit corrected all `freebsd-version -kru` entries to CURRENT. The desktop trial therefore continues to require a separate verified STABLE boot environment. No package ABI override is an acceptable substitute for that separation. The desktop work does not change the Wi-Fi experiment's kernel or boot selection.

## 3. Architecture and ownership boundaries

[diagram:architecture]

There are two desktop-facing channels. A namespaced static development library supplies selected sd-login calls over typed ConsoleKit discovery. Mutter embeds that small PIC archive through explicit `bsd3-login` pkg-config metadata. Separately, `bsddd` owns the small login1 D-Bus surface needed for session control and descriptor lifecycle. The daemon performs its own authorization of the original frontend sender before using a privileged backend connection. [S2, S3]

ConsoleKit is the initial session/device backend rather than a second competing broker invented inside BSD³. Its implementation is not assumed correct merely because its XML declares a method. The investigations below demonstrate why actual locality, device-state and console behavior had to be measured and corrected in the dependency. ConsoleKit remains a trusted privileged component; this report is not a complete security audit of that backend or all direct callers.

The ordinary-user bootstrap has a different role. It holds a real session registration while the command runs, validates its identity and controlling VT, and supervises its descendants. It is neither PID 1 nor a general replacement for GNOME's later service graph. The optional root trial guard is outside the normal desktop process design: it bounds an experiment and restores console state. It must not be mistaken for a permanent login/session service.

## 4. Selected sd-login ABI and discovery semantics

The header is installed as `<bsd3/sd-login.h>`, not a fake systemd header. The project supplies `libbsd3-login.a` and `bsd3-login.pc`; it does not publish a fabricated libsystemd version. The archive is PIC so a consumer can embed it in a shared object. Public GIO requirements remain in pkg-config metadata. Mixing two libraries that export the same `sd_*` symbols is unsupported. [S2]

| API group | Implemented behavior | Review obligation |
| --- | --- | --- |
| PID lookup | sd_pid_get_session; zero means the calling library process | Frontend PID-zero requests must use the original D-Bus caller |
| Session attributes | UID, type, class, state, seat and active state | Typed reply and identifier validation; no synthetic defaults |
| Enumeration | sd_get_sessions and sd_uid_get_sessions | Empty inventory differs from backend failure |
| Filtering | Negative: all; zero: active/online; positive: active | Closing sessions excluded where the filter requires |
| Display choice | Oldest open graphical user session; otherwise oldest open user session | Creation timestamps parsed; greeters/closing excluded |
| Allocation/errors | Caller-owned strings/vectors; negative errno | Caller frees results; generic CK errors can remain EIO |

Session IDs are validated ConsoleKit object-path components, not Linux session numbers. A null session argument resolves the calling process's own session. Seat identity comes from the backend's typed GetName result, not a historical object label such as Seat1. Names are validated before conversion to a D-Bus component; underscore, hyphen and a leading digit are handled without aliasing. Tests deliberately distinguish canonical identity from backend object-path spelling.

The library uses bounded individual synchronous calls. It does not provide an aggregate transaction deadline, a coherent multi-call snapshot across session changes, or a production high-volume event API. Known backend errors map to useful errno values; a generic backend exception is not reclassified from its human-readable text. The exact installed backend can expose errors more coarsely than the intended API contract. [S2, S4]

## 5. Actual D-Bus surface

The surface is explicitly declared in source XML and dispatched through typed GDBus interfaces. Interface existence is not evidence of a full logind implementation. All interfaces below are the selected implemented subset at the source reference. [S3, S5]

| Object/interface | Selected methods or properties | Deliberate exclusion |
| --- | --- | --- |
| Manager | GetSession, GetSessionByPID, ListSessions, GetSeat, ListSeats | No CreateSession/user manager/power API |
| Session methods | TakeControl, ReleaseControl, TakeDevice, ReleaseDevice, PauseDeviceComplete | No forced takeover, activation or unlocking |
| Session properties | Id, Active, Type, Class, State | No complete logind property set |
| Session signals | Directed PauseDevice/ResumeDevice; Active PropertiesChanged | No broadcast device descriptor |
| Seat | SwitchTo; Id, CanTTY, CanGraphical | Dynamic seat hotplug relay not implemented |
| User | No complete User object implementation | No userdb or cgroup compatibility claim |

The Manager's PID-zero path obtains the original sender's process ID from the bus. It must not accidentally resolve the daemon itself. Read-only enumeration may report metadata, but authorization of a controller is a separate decision. Unknown operations remain unsupported rather than receiving a plausible success reply.

SDDM makes seat discovery a startup boundary: it detects login1, requests ListSeats and reads each seat's CanGraphical before creating a display. The previous bridge could satisfy device/session calls while preventing SDDM from creating any display. The correction queries real backend seats independently of session inventory. A machine can have a real seat while it has no registered user session.

CanTTY forwards ConsoleKit's CanActivateSessions result. CanGraphical forwards the typed backend suitability hint, including false/error results. ConsoleKit2 currently implements that hint coarsely as true; forwarding it is not proof of a usable GPU or compositor. No local always-true property is added. Reviewers should keep capability advertisement and actual device acceptance separate.

## 6. Original identity and authorization

A privileged proxy changes the identity seen by its backend. ConsoleKit sees the daemon's backend connection, not the desktop client's original D-Bus sender. BSD³ therefore cannot treat a successful backend authorization check as proof that the original caller was authorized. [S3]

The controller path obtains both UID and PID from the frontend bus's credential methods. It verifies that the peer UID owns the requested session and that the original PID resolves to that same session. Missing PID credentials fail closed; a UID-only fallback is not accepted. A forced takeover request is rejected even from root. Repeating TakeControl on the controlling unique frontend connection is idempotent; a different connection receives a busy error.

```
frontend unique sender
  -> bus-authenticated UID and PID
  -> requested session UID equals peer UID
  -> peer PID resolves to requested session
  -> backend unique owner unchanged across authorization
  -> private backend connection acquires controller
```

The backend owner is captured before authorization, checked afterward, and used by unique name for control calls. A replacement process must not inherit a stale controller through a reused well-known service name. Each accepted frontend controller receives its own backend bus connection. Frontend disconnect closes only that connection; backend name loss invalidates tracked controllers. This protects lifetime isolation in addition to the initial credential checks.

Seat.SwitchTo is further restricted to the original authenticated controller while its own session is active on the matching canonical seat and pinned backend. VT zero is invalid. The bridge does not open a caller-supplied path, alter devfs permissions or run the desktop as root. It delegates actual VT switching to the backend after its own checks.

## 7. Controller and signal lifetime

Each controller holds its session ID, frontend unique sender, backend unique owner, dedicated bus connection, a bounded device map and callback/refcount state. Closing a controller invalidates it before removing signal subscriptions/filters and closing its backend connection. A release failure still drops stale local ownership; it does not leave a controller in a state that could later be mistaken for a valid lease. [S3]

The device map is bounded to256 leases per controller. It stores protocol metadata rather than retaining copies of client device descriptors. This is a per-controller limit, not a complete daemon-wide resource or denial-of-service budget. Aggregate controller limits, queue/backpressure policy and request-wide deadlines remain review topics.

GDBus filters run on a worker thread. The implementation retains the received message and controller reference, then schedules state mutation in the main context. Queued messages are discarded if the controller is closing or is no longer the map's current entry. This avoids changing shared hash tables directly from a filter callback while preserving ancillary FD data.

Signal forwarding verifies the backend sender, object path, interface, body type and descriptor bundle. Device signals are directed only to the controlling frontend connection. Active state changes are public typed PropertiesChanged notifications; device descriptors are not. Tests cover observer exclusion, client death, backend replacement and malformed ordering. They do not constitute a proof of every possible scheduler interleaving or exhaustion case.

## 8. Device leases and descriptor transport

[diagram:leases]

TakeDevice uses unsigned major/minor identifiers. It accepts exactly one ancillary descriptor at handle index0 and records the backend's actual inactive flag. A D-Bus handle is an index into the message's FD table, not the operating system descriptor number. A successful transport test must inspect the received descriptor, not merely compare an integer in the reply. [S3, S10]

If an acquisition returns an extra/malformed bundle, the bridge releases the newly acquired backend lease. If cleanup fails or ownership becomes uncertain, it drops the controller's dedicated backend connection. Resume similarly requires the expected paused state, no pending acknowledgement, and exactly one replacement descriptor. The daemon retains lease state, not a permanent duplicate of the device FD.

A normal pause waits for PauseDeviceComplete from the controller. A force pause requires no acknowledgement. Gone removes the lease. Resume provides a fresh descriptor after a pause. Invalid ordering, release failure or an unusable acquisition transport is not silently repaired by inventing a state. Pipes with distinct take/resume markers exercise the protocol boundary; actual kernel revocation is a separate experiment.

The native input test checks the character-device identity, a read-only version ioctl and poll state. On the matched kernel, a revoked client returns ENODEV for EVIOCGVERSION and POLLHUP from poll. The real VT test keeps the old descriptor across pause/resume: the fresh descriptor becomes usable while the old one remains revoked. Release then revokes the fresh descriptor. No keyboard or mouse events are read or retained.

## 9. Three backend discrepancies found by measurement

### 9.1 A false locality parameter became true

The real headless harness explicitly requested a nonlocal session, but the installed backend returned local. Source inspection showed the final decision tested whether an is-local parameter existed rather than its boolean value. Omitting the explicit parameter was not a reliable workaround because collected data could still include it. The narrow dependency patch uses the typed boolean. The regression queries actual IsLocal and VT zero before launching anything. CI30 and CI31 subsequently passed strict full GNOME startup/window/logout. [S6, E2]

### 9.2 Inactivity and FreeBSD revoke encoding disagreed

The stock backend reported an inactive session's device as active, and the received input descriptor still answered EVIOCGVERSION. This was not merely a mock/XML disagreement. The requested and handed character-device identities matched, while the real kernel object had not been revoked.

A typed native-header probe identified EVIOCREVOKE as `0x20044591`; the installed backend contained the pointer-style fallback `0x80044591`, invoked with null. The correction uses FreeBSD's immediate-integer encoding, fails an initial inactive open if revocation fails, tracks inactive fallback opens and reports the actual device inactivity. Linux's fallback is not blindly inverted. CI31 checked ENODEV/POLLHUP and retained revocation after release. [S6, E3]

### 9.3 The console monitor had no usable console

A real controlling VT was active and the session was correctly local, but CK active state remained false. The stock daemon had no console descriptor. Typed probes found that /dev/console rejected console/VT operations on this serial-primary VM; /dev/consolectl passed one console check but still rejected VT_GETACTIVE. /dev/ttyv0 supported both. The source tried Linux-style candidates and standard streams without a FreeBSD ttyv0 fallback; the all-failed loop could return descriptor3 rather than minus one.

The port adds ttyv0 after existing preferred console candidates and returns minus one on total failure. Actual CK active tracking then allowed the real switch/pause/revoke/resume cycle. ConsoleKit revision5 combines these corrections. Each belongs in the dependency/port, not a compensating lie in the frontend's boolean handling. Modified dependency sources and exact recipes must accompany the binary packages. [S6, E4]

## 10. Native Mutter porting

The pinned FreeBSD port set native_backend=false even with Wayland support enabled. A nested compositor or a menu entry could therefore exist without the required native launcher being built. The overlay enables the native backend and explicitly selects the BSD3 provider. It adds the needed GLESv2/Python build support and FreeBSD header/thread-ID adaptations. Unsupported real-time-limit behavior returns an error rather than a fake success. [S7]

The small archive is built PIC and consumed through normal dependency lookup. An earlier static-link request pulled non-PIC dependency archives into a shared object; using the intended embedding boundary and public GIO metadata corrected that build failure. An installed shared-consumer link test catches this ABI/package distinction.

The modified Mutter package served real wayland-info clients from its native headless backend, then Shell47.10 and a mapped GTK window with frame-clock ticks. The full session harness retained normal GNOME RequiredComponents and used a private desktop-file override only to select headless Shell with a virtual monitor. It did not replace the system session graph with a mock or claim physical pixels from a headless render.

A missing or untested DRM path cannot be inferred from ldd alone. The CK device code uses ioctls and headers; an apparently unused libdrm library link does not establish that the device backend is compiled out. Native headers, binary/source identity and kernel behavior supplied the decisive evidence.

## 11. SDDM and strict ordinary-user startup

[diagram:startup]

The FreeBSD SDDM port's Xsession wrapper did not imply an equivalent Wayland registration path. The tested stock Wayland script directly executes the command, and its PAM sample includes login. On the VM the stock login/system stack supplied pam_xdg without a CK connector. Existing laptop configuration has not been treated as identical without inspection. [S8, S9]

The opt-in adapter calls bsd3-session, then dbus-run-session and SDDM's existing profile script, preserving the selected desktop entry and command semantics. It does not install a misleading duplicate GNOME menu item, modify global PAM or enable itself. The example disables SDDM session reuse because activation and unlock operations are outside the supported login1 subset.

Before creating or reusing registration, the launcher requires an ordinary effective/real UID, a character tty on standard input, the actual controlling session, the foreground process group and typed VT_GETINDEX. Both new and inherited paths validate backend UID, locality, VT, Wayland/user identity, active state and runtime owner/mode. An inherited cookie must resolve to the caller's PID registration. The backend owner is pinned. PID association here is a typed backend report, not an independent kernel process-membership proof. No child is launched when these checks fail.

The child explicitly inherits stdin; a generic spawn default could otherwise replace the controlling VT with /dev/null. A native process reaper supervises descendants across setsid while preserving the foreground group. Termination is forwarded, escalation and final reap waits are bounded, and a newly owned registration is closed on exit. A reused registration is retained. Backend loss terminates the command. The final status line follows cleanup and contains no cookie. [S9, S11]

Direct native tests passed a complete device/VT cycle through this production launcher, a nested inherited-registration reuse, invalid-cookie refusal and non-console refusal. A detached descendant that ignored TERM was killed/reaped; the launcher reported failure and the outer fixture restored the console. Real SDDM autologin/PAM then reached an ordinary local active child and full GNOME headless window/logout. That is evidence for session integration, not ordinary-password or physical DRM acceptance.

## 12. Bounded recovery as a separate experiment

[diagram:recovery]

The root-only guard is a manually invoked trial utility, never setuid and not a permanent service. It refuses an existing process-controlled VT, snapshots available consoles, and launches a separate timeout controller. Its parent remains a native process reaper with a longer deadline and confines termination to descendants. KVM is the default scope; physical use requires an explicit operator flag and an already coordinated window. [S12]

After teardown it reopens console descriptors, verifies their device identities, restores modes, keyboard/display state, terminal settings, ownership and permissions, and checks readback and the original active VT. The maximum requested workload is600 seconds. The inner controller has a five-second kill grace; the parent has fifteen seconds beyond the requested deadline, followed by bounded descendant cleanup and restoration waits. These bounds do not make a frozen kernel interruptible or protect the guard from SIGKILL.

An initial fixture incorrectly reused a revoked session-tty handle for global recovery. That test failed to restore the active console and an existing root console explicitly restored VT1. The corrected fixture uses independent ttyv0 control and reopens handles after session teardown. Its negative stock-backend scenario then reported recovery=PASS before the positive backend scenario was accepted. A configured recovery idea was not counted as tested recovery.

The new guard's successful producer test observed both ready markers, streaming producer-start without producer-finished, and a live supervisor before completion. It ended with child0, no interventions, recovery=PASS and final PASS. A deadline test ended child124 and restored all consoles but remained FAIL_OR_ASSISTED. Direct SDDM runs also restored the original console at their bounds; that later greeter termination is separate from normal GNOME logout.

The reusable runner waits for GNOME's result, the production launcher's final zero status and the original CK inventory before requesting greeter stop. A command-exit file is an intermediate record; complete.txt is the final recovery barrier. Failed/partial exports are labelled explicitly. An initial run was blocked during preparation by OIDC discovery503 and a separate macOS-xattr extraction error; neither attempt counts as a pass. After access recovered, a fresh immutable root-owned snapshot of 8bfa868 passed the complete runner with exit0. Streaming completed PASS; deliberate timeout and requested greeter stop remained FAIL_OR_ASSISTED with recovery PASS/original VT1. The final stop required descendant escalation (child137, three terminated and one killed), so this is verified requested recovery, not unassisted greeter shutdown. Physical DRM/input remains unverified. [E4]

[pagebreak]

## 13. Validation matrix and what each result proves

| Evidence layer | Concrete observation | What it cannot prove |
| --- | --- | --- |
| Source/ABI review | Typed calls, exact methods, flags and patch scope | Actual privilege/device behavior |
| Private D-Bus | Real peer credentials on FreeBSD; pipes and state-machine failures | Kernel input/DRM revocation |
| Native package | Compiler, installed consumer link, stage/test/plist/QA/package | A physical desktop login |
| Real CK session | Own UID/type/class/runtime and authenticated control | GPU rendering or normal password entry |
| Real input/VT | Actual identity, ENODEV/POLLHUP, switch and fresh FD | Every DRM/input driver or suspend case |
| Full GNOME headless | Normal RequiredComponents, Wayland window ticks, logout | Accelerated physical display/input |
| Real SDDM path | Autologin/PAM helper to strict local child and GNOME | General authentication/multi-session behavior |
| Recovery | Observed positive and negative restore paths | Recovery from a frozen kernel |
| Physical trial | Still pending coordinated window | No physical acceptance asserted |

The expanded private suite has18 groups. On the native VM17 applicable groups passed and the deliberate missing-PID group was skipped because the bus supplies those credentials. On macOS9 applicable groups passed and9 credential-dependent groups were skipped. Those native results are necessary: a host that cannot provide peer PIDs must not report the original-identity tests as executed.

CI25 established that commands on this executor actually run as root. Ownership-denial tests therefore explicitly use the existing nobody account, while session/GNOME acceptance uses the existing circleci account. A previous test described as ordinary-user execution had actually acquired the name as root and timed out. The revised procedure measures identity rather than inferring it from an executor label. No new passwordless-sudo account was created.

CI33 completed overall-success, including service, actual CK/input/VT, Mutter, full GNOME and upload. CI32 passed individual actions but ended infrastructure_fail and was quarantined. Downloading its files or observing green steps did not make it a release gate. Later focused branch CI must qualify the new SDDM/launcher/recovery source, not inherit CI33's result by proximity. [E1, E5]

## 14. Native packaging, service lifecycle and release gate

The candidate fits a FreeBSD ports overlay with an explicit plist, dependencies, license and test target. Installation obeys DESTDIR and PREFIX; external desktop dependencies use LOCALBASE. The stage contains the daemon, PIC development archive, header/pkg-config data, ordinary-user session adapter, opt-in example, root-only guard and manual pages. Native QA checks the real staged filesystem, not a synthetic package manifest. [S13]

The rc script uses rc.subr/run_rc_command, REQUIRE=dbus and daemon(8) with a pidfile/syslog tag. bsddd_enable defaults to NO. Standard onestart/quietstart/faststart, status and stop were exercised. A post-start check verifies that the exact process owns login1 and refuses a foreign owner. It clears caller bus-address overrides for privileged management and does not use automatic D-Bus activation or silently enable itself. Name ownership establishes service identity, not desktop readiness.

Stock ConsoleKit is sufficient for build/private-bus work only. Real service/device acceptance requires the corrected backend and matching native Mutter packages. Installing build dependencies is not a substitute for that runtime qualification.

```
# Isolated FreeBSD VM, root dependency preparation:
pkg install pkgconf glib dbus consolekit2

# Ordinary user, source build and private-bus tests:
make
make test

# Staged install; does not activate the service:
make DESTDIR=/tmp/bsd3-stage PREFIX=/usr/local install
```

The final release must come from successful native CI at the exact candidate source. Publish a signed beta tag with its package, source, checksums and provenance; include complete corresponding modified Mutter/CK sources and exact recipes. Download the published assets again and verify them. A package catalogue must preserve unrelated entries and make the corresponding sources available to its recipients. No failed-CI or ad hoc VM build is a laptop release substitute.

The old v0.1.0-beta1 remains the read-only preview. Candidate beta3 has not been published at this report's checkpoint. CI33's exact historical binary identities are preserved in the appendix, but those earlier binaries do not contain the new SDDM launcher/recovery solution. Package versions alone are insufficient when unreleased iterations use the same candidate version.

[pagebreak]

## 15. Controlled hardware acceptance and newer GNOME

No laptop installation, reboot, display-manager/backend change or desktop workload is authorized by this report. The coordinating Wi-Fi work retains the device until it opens a concrete window. The latest report says the current environment is all CURRENT; ABI15 packages require a separate verified STABLE boot environment, with matching kernel/modules/userland and the persistent normal fallback unchanged. [E6]

The future procedure is to clone the verified STABLE base, confirm which datasets contain /etc, /boot, /usr/local and the package database, and boot the trial once with SDDM disabled. Shared home/log/tmp datasets are not restored by a BE rollback. Save baseline packages/configuration, fetch the published beta, verify hashes/ABI/dependencies and install only the exact candidate changes. No mass upgrade, forced ABI override, kernel replacement or network tuning belongs in this trial.

The first login attempt uses the real existing GNOME Wayland entry with the opt-in adapter, a private test profile, a bounded local runner detached from SSH, observed guard readiness and a separate control connection. A bounded sleep/idle inhibitor must be checked and released; permanent power policy is not changed. Success requires visible GNOME, actual keyboard/pointer use, a Wayland application, leaving/returning to the VT, normal logout and console/fallback recovery. A deadline intervention is useful recovery evidence, not an unassisted reliability pass.

The eventual newer-GNOME boundary remains independent. GNOME49's published distribution notes and pinned session source remove the earlier built-in startup assumption and require a user-service solution; GDM's userdb dependency is another surface. The small local reaper does not provide that graph. A narrow explicit integration with an appropriate BSD-capable supervisor should be assessed before adding a general unit interpreter. No GNOME51 build/runtime success is asserted. [S14]

[pagebreak]

## 16. Reviewer navigation and open questions

| Start here | Inspect | Falsifiable question |
| --- | --- | --- |
| src/login.c | session_path, sessions, sd_uid_get_display | Are invalid IDs and error/empty states distinct? |
| src/control.c | authorized, new_controller, destroy_controller | Is the original peer checked and each lifetime isolated? |
| src/control.c | device_call, relay_signal, filter_signal | Is FD cardinality/state/recipient checked before forwarding? |
| src/control.c | bsd3_seat_call | Can an inactive/foreign controller switch a VT? |
| src/seats.c | bsd3_seats_list/property | Can a seat exist without sessions, without inventing capability? |
| src/session.c | valid_session and main cleanup | Does failed registration prevent exec and preserve stdin? |
| src/vt-guard.c | snapshot, timeout/reaper, readback and mark | Does a final marker follow actual restoration? |
| tools and packaging | CI wrappers, overlays, plist and rc script | Does the installed exact package satisfy the tested contract? |

Outstanding review topics include full backend authorization behavior for direct callers, aggregate resource limits/backpressure, synchronous call latency and property races, more complete session properties, dynamic seat changes, controller-loss notifications, DRM master revoke/resume on real hardware, ordinary authentication, lock/unlock, suspend/power/inhibitors and the later user-service graph. Unsupported functions must remain explicit, not filled with optimistic return values.

Review the minimal dependency patches against original source and measured behavior. Verify that a descriptor belonged to the requested character device and that revocation remained true across resume/release. Reproduce both positive and refusal cases. Keep an actual source revision, binary/configuration identities and final barriers with every result. The purpose of a technical guide is to make the remaining experiment smaller and reviewable, not turn a compile or a callback into an acceptance claim.

[pagebreak]
## Appendix A. Historical artifact ledger

These are verified CI33 checkpoint identities, not the manifest for a later SDDM-capable release. The matching source is dcf9744, ABI FreeBSD:15:amd64. All three downloaded packages were installed and exercised in the identified STABLE VM. Later source at8bfa868 needs its own complete CI and release manifest. [E1, E6]

| CI33 artifact | SHA256 |
| --- | --- |
| bsd-cubed-0.1.0.b3.pkg | 0dd5a5439adb901aa345f2732d7fbf675623ba849f6465e884d1cf53b0a86b80 |
| consolekit2-2.0.0_5.pkg | 5c907f809ec3eae7bc030b660b79a1d9719126ebe0b81912d49a78755caf29d5 |
| mutter-47.10_3.pkg | b63b4c436f8d8866437c1abdaf9cd4ac636d9c0265bd2b2e546579a3ebd1f4aa |

The CK2.0.0 release archive used by the pinned overlay has SHA256 `f3f4a424f2a4fbb67696b4011caf2fd4c02e5e67c24ef1c86cecb2301bccdba4`. The Mutter47.10 release source has SHA256 `ee8a583c2b6ff309b501dc97e7c0b4f11d6197a9529ed22247ee95e89663e969`. Preserve the corresponding modified trees and recipes, not only upstream tarball links.

Focused later source identities are seat discovery94ef2dd, local bootstrap9f76df9, and bounded trial8bfa868. All are signed with the configured author identity and DCO sign-off; a signature establishes authorship/integrity metadata, not correctness, maintainer review or upstream integration. The published PR URLs identify review state separately from source ancestry.

[pagebreak]
## Appendix B. Reproduction and evidence collection

Use an isolated FreeBSD VM with the relevant packages and actual system services. The private protocol test is safe by design because its script routes all processes to a private Unix-socket bus; it must not accidentally operate the host bus. Native system tests instead use the actual machine bus and explicitly record the ordinary/root identities.

```
# Read-only identity before integration:
freebsd-version -kru
uname -v
cc --version
pkg config ABI
pkg query '%n %v %q' bsd-cubed consolekit2 mutter

# Ordinary-user kernel input check under a real CK test session:
./test-native-session ./test-native-device /dev/input/event0

# Native CI entry points; disposable executor only:
sh tools/ci-freebsd.sh
sh tools/test-native-consolekit.sh
sh tools/ci-mutter.sh
sh tools/ci-gnome.sh
sh tools/ci-sddm.sh
```

The test binaries are compiled by the wrappers from the same source and installed metadata. Do not copy these commands onto an active laptop as a substitute for the release and coordination gate. The KVM-only VT fixture must retain its hardware guard. The root trial tool's explicit hardware flag does not relax the need to validate the exact runner, observe readiness and preserve the normal fallback.

Preserve the exact source/patches, package and compiler identities, kernel/module identities, command invocation and immutable script hashes. Keep raw captures private and commit only reviewed excerpts. Wait for collector/guard final complete barriers before exporting. If interrupted, label the export partial and include observed interventions. A checksum demonstrates which bytes were used; it is not an independent backup or a proof that the test was correct.

[pagebreak]
## Appendix C. Failed paths worth preserving

| Attempt or assumption | Decisive observation | Correction or limit |
| --- | --- | --- |
| Menu entry means native support | Native backend compiled out in stock port | Build and test the native backend explicitly |
| XML inactivity contract is enough | Real inactive descriptor still usable | Fix backend state and FreeBSD ioctl encoding |
| Omitting locality parameter works | Collector could still add it | Verify actual boolean/VT after typed backend fix |
| No libdrm in ldd means no backend | Ioctl/header-based code present | Inspect exact build/source and kernel behavior |
| Console control device is interchangeable | consolectl rejected VT_GETACTIVE | Use proven ttyv0 fallback |
| Reuse the session tty for recovery | Teardown revoked the handle | Independent control and fresh handles |
| CI account implies ordinary execution | Executor measured UID0 | Explicitly drop identity for each test |
| Green actions imply green build | CI32 ended infrastructure_fail | Require overall success at exact source |
| Existing Wayland script registers CK | Real SDDM path lacked that wrapper | Add strict opt-in local bootstrap |
| Session methods alone enable SDDM | ListSeats absent; no display created | Enumerate real seats without requiring sessions |
| StartServiceByName is harmless for owned mock | Nonactivatable name required a service file | Check owner before activation |
| Prepared source extraction is a run | macOS xattrs made extraction error | Clean archive; no false completion marker |

[pagebreak]
## References and evidence index

[S1] BSD³ implementation reference and focused changes: https://github.com/denislemire/bsd-cubed/tree/8bfa868f3a1c485954d35b9be28b2347b2e4bf30 ; PR11 https://github.com/denislemire/bsd-cubed/pull/11 ; PR12 https://github.com/denislemire/bsd-cubed/pull/12 ; PR13 https://github.com/denislemire/bsd-cubed/pull/13 .

[S2] Selected ABI: src/login.c, include/bsd3/sd-login.h, packaging/bsd3-login.pc.in and tests/shared-consumer.c at the implementation reference.

[S3] Controller/device implementation: src/control.c and src/control.h at the implementation reference. Private protocol cases: tests/mock-ck.c and tests/test-login.c.

[S4] Discovery/selection evidence: docs/architecture.md and tests/test-login.c. Historical prose is dated evidence; source at the implementation reference is authoritative.

[S5] D-Bus object model and rc readiness: src/bsddd.c, src/seats.c, packaging/freebsd/bsd-cubed/files/bsddd.in and org.bsd3.login1.conf at the implementation reference.

[S6] ConsoleKit dependency overlay: packaging/freebsd/consolekit2 and tools/ci-consolekit.sh. Original release: https://github.com/ConsoleKit2/ConsoleKit2 ; pinned archive identity is recorded in Appendix A.

[S7] Native Mutter overlay: packaging/freebsd/mutter, tools/ci-mutter.sh and tools/test-native-mutter.sh. Pinned ports: https://github.com/freebsd/freebsd-ports/tree/2072800715389ae49066946c960e6521bac37e6a/x11-wm/mutter .

[S8] Exact SDDM source: https://github.com/sddm/sddm/tree/c2b97dd ; inspect src/daemon/SeatManager.cpp, src/helper/UserSession.cpp and data/scripts/wayland-session together with the pinned FreeBSD x11/sddm port patches.

[S9] Local bootstrap: src/session.c, packaging/bsd3-sddm-session.in and docs/local-session-bootstrap-2026-11-05.md. Refusal/descendant tests: tests/native-session-stop.c and tools/test-native-consolekit.sh.

[S10] D-Bus specification, typed Unix descriptor transport: https://dbus.freedesktop.org/doc/dbus-specification.html . The implementation and native descriptor experiments, rather than the specification alone, support the transport/revocation claims.

[S11] FreeBSD procctl(2): https://man.freebsd.org/cgi/man.cgi?query=procctl&sektion=2&format=html . Read the reaper operations and signaling scope with the actual loops in src/session.c and src/vt-guard.c.

[S12] Guard and SDDM runner: src/vt-guard.c, tools/test-native-sddm.sh, tools/ci-sddm.sh, tests/native-sddm-client.c and tests/sddm-gnome-probe.sh. See docs/guarded-sddm-trial-2026-11-05.md for direct results, the later successful reusable runner, source/export checksums and exact-release limits.

[S13] Package/service recipes: packaging/freebsd/bsd-cubed, tools/ci-port.sh, tools/ci-service.sh and docs/ports-and-ci.md. The overlay is a candidate, not an accepted FreeBSD port.

[S14] GNOME49 distribution changes: https://release.gnome.org/49/developers/ ; pinned gnome-session49 source https://github.com/GNOME/gnome-session/tree/776dc42864ea233f732e0b5b72cc08b1d8664fdb . The source audit in docs/source-audit-2026-11-04.md separates the newer service graph from the GNOME47 baseline.

[E1] Successful exact checkpoint CI33: https://circle.ehws.generic.business/gh/denislemire/bsd-cubed/33 . The final handoff, not this historical table, must identify the actual released hardware-trial assets.

[E2] Locality investigation: docs/consolekit-locality-2026-11-05.md ; CI30 https://circle.ehws.generic.business/gh/denislemire/bsd-cubed/30 .

[E3] Real input regression: docs/consolekit-device-regression-2026-11-05.md ; CI31 https://circle.ehws.generic.business/gh/denislemire/bsd-cubed/31 .

[E4] Real VT/device experiment: docs/native-vt-regression-2026-11-05.md, tests/native-vt-launch.c and tests/native-vt-client.c.

[E5] Service and identity correction: docs/service-integration-2026-11-05.md and tools/ci-service.sh. Failed CI32: https://circle.ehws.generic.business/gh/denislemire/bsd-cubed/32 .

[E6] Current hardware gate and exact artifact identities: docs/hardware-handoff-2026-11-05.md. The laptop remains reserved for coordinated kernel/Wi-Fi work until a separate STABLE desktop window is opened.
