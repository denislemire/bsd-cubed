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
