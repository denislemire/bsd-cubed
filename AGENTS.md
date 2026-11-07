# BSD³ working rules

- Keep project code C11 and BSD-2-Clause unless the maintainer approves a
  change. Prefer native FreeBSD facilities and small, explicit interfaces.
  Dependencies retain their own licenses. Do not copy GPL implementation code
  into BSD files.
- The deliverable must fit the FreeBSD ports framework. Preserve DESTDIR/PREFIX,
  package manifests, explicit dependencies and native FreeBSD build/test
  coverage.
- Do not call a stubbed method supported. Refuse unsupported privilege
  operations. Name ownership, allocation and error contracts and test the
  protocol boundary.
- For device control, authenticate the original D-Bus sender, enforce session
  membership and controller ownership, bind cleanup to connection death, and
  verify VT/device revoke/resume semantics. A privileged proxy loses the
  original caller identity at its backend; blindly forwarding control methods
  is unsafe.
- Keep recovery access and working desktop sessions intact. Use private buses
  and isolated build roots before integration. Record source revisions, patch
  sets, toolchain, boot environment and validation limits for real desktop
  trials.
- Keep identifying logs out of the tree (gitignored `captures/private/`).
  Commit reviewed excerpts only.
- Check exact tested source, final diff and `git diff --check`. Run meaningful
  tests. Record failures and corrections instead of rewriting the work as
  inevitable.
- Author/committer: Denis Lemire <denis@lemire.name>. Use configured
  cryptographic signing and DCO sign-off for commits. Do not disable signing
  to work around it.
- Update [docs/STATUS.md](docs/STATUS.md) when what actually runs on the
  laptop changes. Do not oversell a VM result as a desktop.
- Prefer disposable FreeBSD VMs for routine work. Do not install ad hoc
  tree-built packages on a working desktop; use a signed release asset.
- Prefer SSH scripts through explicit `/bin/sh -s`, with the script on stdin,
  when the remote account's login shell cannot safely parse multiline POSIX
  quoting. Use explicit `/usr/local/sbin/pkg`. FreeBSD `pkg add` has no `-n`
  dry-run option.
- Console true-color and btop work belong in a separate repository. This
  project handles session and device integration, not terminal color rendering.
