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

The final reusable-runner validation was interrupted at preparation by loss of
EhWS API authentication. A supported forced OIDC refresh diagnosed discovery503
“No available server”; 1Password is not the demonstrated cause. No generic-runner
completion is claimed. Earlier direct trial barriers completed and recovered.
A macOS-xattr extraction attempt also returned errors before execution; a new
metadata-clean source archive was created instead, SHA256
6edbdb918b4d753c98f98b0386d4da856955198773cc612f7ab1428e37fd94a8.
The partially extracted tree is not counted as a successful experiment. The
latest test script adds explicit partial-export labels and therefore needs its
own freshly recorded snapshot before the remaining run.

The manually staged VM-only SDDM config/probe entry remain disabled and require
cleanup once access returns. They are not present on the laptop. The CI runner
uses distinct names and refuses pre-existing inputs. Physical i915/DRM recovery,
regular login and fallback-BE boot remain the coordinated hardware acceptance.
