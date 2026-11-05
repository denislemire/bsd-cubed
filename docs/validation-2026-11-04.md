# Initial validation — November 4, 2026

State: first read-only prototype verified; native GNOME desktop goal still open.

## Environments and source identity

- Mac: Apple Clang; GLib 2.90.0. `make clean all mock-ck test-login` succeeds with
  C11, `-Wall -Wextra -Werror`. No D-Bus daemon is installed here, so protocol
  tests were run natively on FreeBSD instead.
- Physical FreeBSD test machine: kernel, installed kernel and userland report
  15.2-PRERELEASE; compiler FreeBSD clang21.1.8. Package versions and source pins
  are in the [source audit](source-audit-2026-11-04.md).
- Exact original C/header/build/test inputs are listed in
  [validated-source.sha256](validated-source.sha256). Checksums identify the files;
  they do not independently prove correctness or preserve a second copy.
- Test source/build directories and port work roots are temporary, separate from
  installed software. Tests used an ordinary SSH user, no privileged operations.

## Commands, predictions and observations

On FreeBSD, as the ordinary build user in the source directory:

```sh
make clean all test
./bsddd --probe
make DESTDIR=/tmp/bsd3-stage PREFIX=/usr/local install
make DESTDIR=/tmp/bsd3-altstage PREFIX=/opt/bsd3 install
```

Prediction: private-bus discovery and property queries match the fake backend;
invalid input/backend errors fail; a second daemon cannot replace the first; normal
SIGTERM exits successfully. Observation: all five test groups and the shell
lifecycle/failure checks passed. The live read-only probe reported zero ConsoleKit
sessions, agreeing with the independent baseline query. Both install prefixes
staged the expected binary/library/header/manpage. No live login1 name was acquired.

The tests exercise PID-zero translation to the *calling client's* bus-verified PID,
not the daemon PID. They also verify active/online/closing list filters and oldest
graphical user-session selection with deliberately out-of-order sessions.

On FreeBSD, the candidate port was built against the pinned ports framework
`2072800715389ae49066946c960e6521bac37e6a` in a temporary root with installed
dependencies. This was a sparse framework copy, not a complete ports checkout.
Local source archive was supplied through DISTDIR, then:

```sh
make makesum
make DEVELOPER=yes stage test check-plist stage-qa package
```

Observation: checksum verification, build, stage QA, the same five protocol test
groups, plist validation and package creation passed. The framework warned that
category validation is skipped outside PORTSDIR. The port's intended category is
sysutils; an eventual in-tree/poudriere check is still required. Recursive clean
also warned about omitted dependency directories in that sparse copy; actual
installed runtime dependencies were independently checked in the package manifest
(ConsoleKit2, GLib and gettext-runtime). A full-tree CI build is still needed.
No package was
installed. Local snapshot distinfo is not a release distinfo.

The CircleCI configuration validated successfully against the configured EhWS
server. Configuration validation does not establish a completed pipeline run.
The project was then enabled on that server. Automatic build #1 succeeded:
https://circle.ehws.generic.business/gh/denislemire/bsd-cubed/1

It tested commit ff84228be649b018e4259f4e6400bafeca26d223 in a generic
FreeBSD KubeVirt VM, including the full-tree ports path, stage/test/check-plist/QA,
package creation and artifact upload. Cancellation was requested for duplicate pipeline #2, but the job finished
successfully before cancellation took effect; the authoritative job result is
success. The same occurred for duplicate job #4. Neither is a failed test. The current CLI watch
command could not read the legacy server response; direct server-compatible API
monitoring confirmed job success. No CI credentials were added to this project.

## Failed paths and corrections

1. `ssh hpbsd` did not resolve. Recalled the prior private address and verified it
   live. No identifying network address was added to project source.
2. Initial service-script checks were misleading; process and typed bus checks
   established running backend services independently.
3. First native builds failed because FreeBSD does not expose ENODATA. Removing
   the POSIX feature macro alone did not fix it. The namespaced API header now
   maps the no-data spelling to FreeBSD ENOATTR; the native rebuild/tests passed.
4. Initial package lookup under PACKAGES/All failed. The framework created the
   package in WRKDIR/pkg because the specified PACKAGES directory did not yet
   exist. CI now creates that directory before invoking make and requires the
   expected artifact to exist, preventing a false successful artifact collection.
5. The clean rebuilt package was verified at PACKAGES/All with SHA256
   `7056a25ec4800584db0cf65d9c7829ac5f8d338042fdbbce19608b5e794ee996`.
   Its file list has the intended binary, static library, header, manpage and
   framework-generated license/SBOM entries. The package was copied locally
   and its checksum matched.
6. Manual lint identified two style issues; adjusted the FreeBSD OS macro and
   wrapped the long line. Final mandoc lint passed.

## Limits

No native GNOME login, real VT/device switching, session registration, privileged
controller authorization, descriptor handoff, signals, suspend, power action,
GDM/userdb or GNOME49 user-service startup is implemented or verified. The current
port disables native Mutter. No public release, upstream port submission, release
fetch test, poudriere build or installed-package runtime trial is claimed.

## Repository checkpoint

Initial commit `ff84228be649b018e4259f4e6400bafeca26d223` was pushed to the
private `denislemire/bsd-cubed` repository. GitHub reports its SSH signature valid;
author and committer are Denis Lemire <denis@lemire.name>, with DCO sign-off.
Local verification initially lacked an allowed-signers trust file; this was not
an unsigned commit. No signing configuration was disabled.

## Discovery beta publication and installation boundary

Native FreeBSD CI build5 passed at 0be5de6ce7e98b9d9d1b0f8f822ccdc51deb1596.
Signed tag v0.1.0-beta1 and a GitHub prerelease were published with the actual
CI package, matching source tarball, SHA256SUMS, distinfo and build provenance.
Assets were downloaded again from GitHub and verified; both assets were also
verified after staging on the laptop. Package SHA256:
`15cb7a57a0b4d6d385af9ec6ca9c9556f1f556a95451982b1689f0bd0cb9e952`.
No package was installed: the later VM-first direction reserves the laptop for
final acceptance while parallel kernel/Wi-Fi work reboots it regularly.

Staging exposed an outer tcsh quoting failure for multiline /bin/sh -c scripts.
Use /bin/sh -s with script on stdin instead. Also, pkg add does not support -n;
the attempted dry-run returned usage without changing installed packages.
Do not reuse or stop another investigation's SSH control masters.
