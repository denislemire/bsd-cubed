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
dependencies. Local source archive was supplied through DISTDIR, then:

```sh
make makesum
make DEVELOPER=yes stage test check-plist stage-qa package
```

Observation: checksum verification, build, stage QA, the same five protocol test
groups, plist validation and package creation passed. The framework warned that
category validation is skipped outside PORTSDIR. The port's intended category is
sysutils; an eventual in-tree/poudriere check is still required. No package was
installed. Local snapshot distinfo is not a release distinfo.

The CircleCI configuration validated successfully against the configured EhWS
server. Configuration validation does not establish a completed pipeline run.
The native FreeBSD CI job must still be followed/enabled and run on that server.

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
5. Manual lint identified two style issues; adjusted the FreeBSD OS macro and
   wrapped the long line.

## Limits

No native GNOME login, real VT/device switching, session registration, privileged
controller authorization, descriptor handoff, signals, suspend, power action,
GDM/userdb or GNOME49 user-service startup is implemented or verified. The current
port disables native Mutter. No public release, upstream port submission, release
fetch test, poudriere build or installed-package runtime trial is claimed.
