# Checkpoint — November 4, 2026

State: bridge protocol milestones merged; native GNOME desktop goal still open.

## Verified and integrated

- [PR1](https://github.com/denislemire/bsd-cubed/pull/1): authenticated session
  controller ownership, dedicated backend connections, restart/disconnect cleanup.
- [PR3](https://github.com/denislemire/bsd-cubed/pull/3): canonical ConsoleKit seat
  names through GetName, supporting current and legacy object-path layouts.
- [PR2](https://github.com/denislemire/bsd-cubed/pull/2): device descriptor transport,
  pause/ack/resume, active notifications, descriptor validation, limits and cleanup.
- Final combined code revision: `e7210136d4e524766adfe402a12efb4a1e73f449`.
  native FreeBSD CI13
  passed direct and ports-framework tests: twelve applicable groups passed, with
  the Mac-only missing-peer-PID negative group skipped. Stage/check-plist/QA,
  package creation and artifact upload passed. The same source passed local strict
  compilation and applicable Mac tests; credential-dependent groups skip there.
- Signed commits preserve Denis Lemire as author/committer with DCO sign-off;
  GitHub verified the integration signature. Both feature branches were integrated
  by fast-forward after resolving the test-suite conflict and retesting together.

The suite passes real pipe descriptors and validates protocol state/ownership.
It does not prove a real device's kernel revoke/DRM master behavior or VT switching.

## Distribution and laptop boundary

The published GitHub v0.1.0-beta1 is the earlier read-only discovery preview at
0be5de6. Its actual CI5 assets were downloaded from the release and verified,
including on the laptop after staging. No package was installed. Current code's
0.1.0-beta2 port version is a candidate; no beta2 release is claimed.

VMs are the primary development/validation target. The laptop is final acceptance,
reserved while a separate Wi-Fi/kernel investigation repeatedly reboots it.
Do not retry or reuse that investigation's SSH control masters for routine work.
When a real desktop candidate is ready: publish successful CI artifacts as a
GitHub beta, verify a fresh download, then install that exact artifact for the
laptop's final test. The existing lab package catalogue is also requested for
later tested upgrades; publication is not wired into this project yet and must
preserve unrelated packages, metadata, checksums and recoverability.

## Necessary next work

1. Export actual login1 Seat/User/session relationships and a correctly authorized
   VT switch path. Canonical seat naming is fixed, but Seat object export does
   not yet exist. Keep typed backend identity and original-caller authorization.
2. Provide explicit consumer/build metadata for the selected library API and audit
   shared-library linkage. Do not spoof libsystemd versions or advertise its whole
   ABI. Mutter51.1 also queries process user-unit/cgroup metadata; native FreeBSD
   should report unsupported data honestly rather than invent Linux paths.
3. Port/build the native Mutter backend in an isolated FreeBSD ports/CI environment.
   Current upstream audit pin is51.1 at9134e3a5869ce232bf95b77519005865747ba603.
   Inventory actual dependency versions first. The packaged FreeBSD Mutter47.10_2
   explicitly disables native_backend; an SDDM menu entry cannot change that.
4. Validate native headless/virtual-monitor startup in the VM as a compositor
   milestone. Upstream deliberately bypasses session tracking in headless mode;
   this must not be presented as hardware device/VT validation.
5. Audit/implement current GNOME51 session startup and required services. The older
   GNOME49 source pin explains the removal of its built-in manager, not complete
   current-version support. Preserve upstream licenses on consumer patches.
6. Only after VM milestones pass, prepare the laptop's final native GNOME Wayland
   login test with the exact released artifacts and recovery retained. Existing
   GNOME Wayland session entries already exist; no new working option was claimed.

Console high-color/btop work is exclusively in the separate vt-chroma repository.
No high-color kernel or native GNOME login is ready at this checkpoint.
