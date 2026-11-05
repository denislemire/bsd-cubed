# BSD³ working rules

- Apply Denis's Hindsight operator memory skill before substantial work. Memory
  supplies continuity; inspect live source and machines independently.
- Keep project code C11 and BSD-2-Clause unless Denis approves a change. Prefer
  native FreeBSD facilities and small, explicit interfaces. Dependencies retain
  their own licenses. Do not copy GPL implementation code into BSD files.
- The deliverable must fit the FreeBSD ports framework. Preserve DESTDIR/PREFIX,
  package manifests, explicit dependencies and native FreeBSD build/test coverage.
- Use the existing ehws-infra CircleCI FreeBSD machine executor for CI. Read
  current infrastructure configuration before changing it. Never commit tokens.
- Do not call a stubbed method supported. Refuse unsupported privilege operations.
  Name ownership, allocation and error contracts and test the protocol boundary.
- For device control, authenticate the original D-Bus sender, enforce session
  membership and controller ownership, bind cleanup to connection death, and
  verify VT/device revoke/resume semantics. A privileged proxy loses the original
  caller identity at its backend; blindly forwarding control methods is unsafe.
- Keep recovery access and working desktop sessions intact. Use private buses and
  isolated build roots before integration. Record source revisions, patch sets,
  toolchain, boot environment and validation limits for real desktop trials.
- Keep identifying logs in gitignored captures/private. Commit reviewed excerpts.
- Check exact tested source, final diff and git diff --check. Run meaningful tests.
  Record failures and corrections instead of rewriting the work as inevitable.
- Author/committer: Denis Lemire <denis@lemire.name>. Use configured cryptographic
  signing and DCO sign-off for commits. Do not disable signing to work around it.
- At meaningful milestones, update the source audit/validation record and retain
  an accurately dated outcome with lifecycle state. A compiled prototype is not
  a verified native GNOME Wayland desktop or a submitted FreeBSD port.
- Development and protocol/package validation belong in generic EhWS KubeVirt
  FreeBSD VMs (normally the CircleCI machine executor). The physical laptop has
  parallel kernel investigations; reserve it for agreed final hardware tests.
  Do not change its kernel, boot selection, services or desktop for routine work.
- When a package is ready for meaningful laptop integration, publish a GitHub
  beta/prerelease first with the successful native CI package, matching source
  archive, SHA256SUMS and provenance. Then download that exact release asset,
  verify it and install it on the laptop. Do not install an ad hoc build or a
  source-tree binary instead. A diagnostics-only prototype is not desktop-ready.
- Denis subsequently explicitly authorized installing the published beta package
  and preparing a GNOME Wayland option on the laptop. This authorizes desktop
  integration work but does not authorize claiming readiness for an unsupported
  compositor. Existing GNOME Wayland entries already exist; do not add a misleading
  duplicate or start a disruptive login test before the native backend is ready.

- Latest direction: VMs are primary and the laptop is the final 'I am done'
  acceptance target. Defer laptop installation/session integration until a real
  native GNOME Wayland candidate is ready, despite the earlier preview-install
  request. Its frequent parallel Wi-Fi/kernel reboots are not a BSD3 failure.
- Prefer SSH scripts through explicit /bin/sh -s, with script on stdin; the
  account's outer tcsh cannot safely parse multiline POSIX command quoting.
  Use explicit /usr/local/sbin/pkg. FreeBSD pkg add has no -n dry-run option.
- Reuse the EhWS package catalogue for tested rollout/upgrades alongside GitHub
  beta assets. Publication is not authorization to install on other hosts.
- Console true-color/btop belongs in the separate vt-chroma repository.
  BSD3 handles session/device integration, not terminal color rendering.
  Keep console research, kernel patches and btop work out of this repository.
