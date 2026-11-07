# Guarded SDDM/GNOME trial — November 5, 2026

State: direct native VM trials verified; reusable end-to-end runner and exact
release CI still pending. No laptop actions or open hardware test window.

The existing component checks were insufficient to establish display-manager
integration. The actual SDDM0.21.0.36_7 autologin/PAM path now reached the
production ordinary-user launcher, verified its controlling VT/local CK identity,
and ran full GNOME47 RequiredComponents, a mapped Wayland GTK window and normal
session-manager logout. The Shell renderer was explicitly headless; native DRM,
physical pixels/input and ordinary password authentication remain unverified.
A real VESA Xorg greeter was visually observed after the session ended.

The independent root-only bsd3-vt-guard snapshots every available console before
launch, refuses a process-controlled baseline, and runs a separate timeout
controller under a native parent process reaper. It confines termination to its
own descendants. Reopened descriptors restore/check VT, keyboard/display mode,
termios, permissions, ownership and active VT after child teardown. The default
scope is KVM. Physical hardware requires an explicit operator flag and coordinated
window, not removal of the disposable test fixture's hardware guard. The binary
is mode0755, never setuid; installation grants no sudo or new administrative access.
It changes no network, boot, rc.conf or power policy.

An immutable root-owned producer was observed while still running: both guard
and controller ready markers existed, streamed producer-start was present,
producer-finished was absent, and the guard PID was alive. It then exited0,
reported no interventions and restored all12consoles/VT1 with complete=PASS.
A separate deadline case returned child124 and recovery=PASS, with final
FAIL_OR_ASSISTED. Both real SDDM trials also ended at their explicit deadlines,
restored VT1 and all saved console state, and were correctly classified assisted.
The GNOME logout result is separate from that later forced greeter shutdown.

The reusable native runner adds these positive/negative guard checks before a
real SDDM/GNOME trial. It uses a root-owned source snapshot, an existing ordinary
account and temporary KVM-only autologin/configuration, preserving existing GNOME
entries. It waits for both GNOME completion and the production launcher's final
zero status plus restored CK session inventory before requesting greeter shutdown.
Only the final complete barrier permits acceptance; failed/partial exports are
explicitly labelled. Provenance, immutable producer hashes and raw child/guard
results are preserved. Cleanup removes only this runner's configuration and
stops the bridge only if this runner started it. No service is enabled.

The complete candidate passed native ports stage/test/check-plist/stage-qa/package.
The latest VM-only SDK package identity is
97714fa82e12d7756083c6feaca15d0f005abacbab25df84ed554d1693590b01.
It is not a release artifact or a permitted laptop payload. Native CI must create
and validate the final release binaries and corresponding sources.

The final reusable-runner validation was interrupted at preparation by a
temporary CI authentication failure. No generic-runner completion is claimed
from that attempt. Earlier direct trial barriers completed and recovered.
An archive extraction attempt also returned errors before execution; a new
metadata-clean source archive was created instead, SHA256
6edbdb918b4d753c98f98b0386d4da856955198773cc612f7ab1428e37fd94a8.
The partially extracted tree is not counted as a successful experiment. The
latest test script adds explicit partial-export labels and therefore needs its
own freshly recorded snapshot before the remaining run.

The manually staged VM-only SDDM config/probe entry remain disabled and require
cleanup once access returns. They are not present on the laptop. The CI runner
uses distinct names and refuses pre-existing inputs. Physical i915/DRM recovery,
regular login and fallback-BE boot remain the coordinated hardware acceptance.

## November 5 update: reusable runner verified

CI authentication recovered without a code change in this repository. No
unrelated live state was overwritten.

A fresh archive of published commit8bfa868f3a1c485954d35b9be28b2347b2e4bf30,
SHA256 e999715df58c833560a8930357064aa2a8c9e82de5cbd8ee67ce8ad5c3867c88,
was extracted into a new root-owned VM source tree. The exact runner script,
SHA256 7313a1eb93f35a88c3b55399031c89f78e270a816b38b135113fbd4201302bfc,
was made read-only and launched detached. The separately observed final exit
status was0, after its evidence/cleanup barrier.

The streaming-readiness case completed PASS. The deliberate timeout recorded
child124, recovery PASS and complete FAIL_OR_ASSISTED. Actual SDDM then launched
ordinary an ordinary test UID on VT9 with seat0 active and own-session control. Full GNOME47
RequiredComponents, mapped headless Wayland window and normal session logout
passed. The runner observed the production launcher exit0 and original CK
inventory before requesting termination of the remaining greeter.

That final requested stop recorded signal15, child137, three terminated
and one killed descendant, no outer-deadline intervention, recovery PASS and
original VT1. Its final barrier remained FAIL_OR_ASSISTED, as required; the
aggregate runner label was ACCEPTANCE_PASS_WITH_REQUESTED_GREETER_STOP.
This proves the requested recovery path, not an unassisted greeter shutdown.

The earlier manual config/probe entries were removed before the run. The runner
removed its own inputs afterward. Root originals and a protected private export
were preserved; export SHA256
bd9cda21be0d8171d2edbdda27164c2f328e50685a7bb21939b02ba2dd6518c0.
These are VM-only local package results. CI36 must still qualify the exact release
artifacts; physical DRM/input, normal password login and the hardware window
remain open. Earlier interruption records above remain historical evidence.

## Cold backend ordering correction

A later exact-CI36 package trial after VM reboot initially stopped before SDDM:
ListSessions returned ENOTCONN (rendered as “Socket is not connected”). The
session SDK deliberately uses NO_AUTO_START, and CK was absent. This did not
establish a kernel or package binary fault. An explicit real CK GetSeats call
activated the backend, after which the session inventory worked.

The integration runner now records NameHasOwner and calls the same login1
ListSeats path SDDM uses before querying inventory. A controlled test terminated
an empty CK backend first; the immutable corrected script recorded backend=false,
discovered real seat0, and completed the full SDDM/GNOME test with exit0. The
production launcher exited0 and the original empty inventory returned. Requested
greeter stop recorded child0, signal15, no outer deadline, seven terminated and
zero killed descendants; recovery PASS and complete FAIL_OR_ASSISTED remain the
correct separate outcomes. The first failed attempt and partial export were
preserved. Test script SHA256:
2e5bc00087364b89c89d9fb41c6449548eb06c5024b9b9983d5cf1a1269b9bf0.

The rebooted VM reported all three version entries15.2-PRERELEASE and running
GENERIC stable/15-n286624-646a485c3b2f. That is a distinct runtime identity from
previous pre-reboot84aa3cb3204d results. Package ABI remained FreeBSD:15:amd64.
The on-disk kernel predated this trial; none of the three packages installs a
kernel. Do not silently attach the old kernel identity to the new trial.

CI36 at implementation8bfa868 completed every build/test/upload action. The
cold-ordering correction changes only the external integration runner; none of
the compiled source or package inputs changed. Its native controlled result must
be cited separately from the original CI36 runner result.
