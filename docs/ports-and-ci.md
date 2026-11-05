# Ports and EhWS native CI

The candidate port is `packaging/freebsd/bsd-cubed`, intended for
`sysutils/bsd-cubed`. It has an explicit plist, BSD2CLAUSE license, native make
build, GLib/pkgconf dependencies, ConsoleKit runtime dependency and `test` target.
There is no systemd dependency. No daemon is enabled on install.

The current candidate is 0.1.0-beta3, still experimental.
The published 0.1.0-beta1 is the earlier read-only discovery preview.
The matching release artifact must be published from successful native CI. The release URL in MASTER_SITES is the
planned distribution layout, and **is not fetchable until a release is published**.
Do not submit this overlay upstream without a real source release and distinfo.

## CI on existing lab infrastructure

`.circleci/config.yml` follows Denis's existing EhWS CircleCI Server configuration:
`machine.image: freebsd-15.1:current`, `resource_class: freebsd.medium`.
The job builds with the native compiler, runs private-bus tests, verifies a staged
install, then builds a package through the actual ports framework. The pipeline
does not publish a package catalogue, deploy to hosts or modify their rc settings.

`tools/ci-port.sh` copies the candidate port to a temporary overlay. It uses
`/usr/ports` when available, otherwise clones a ports tree into its temporary root.
It builds an explicit local source snapshot, runs `makesum`, `stage`, `test`,
`check-plist`, `stage-qa` and `package`, and preserves snapshot checksums,
package checksums and the ports revision under `dist/`. These are snapshot
artifacts, not proof that a release distfile was fetched.

Run as an ordinary user on a disposable lab FreeBSD machine with dependencies:

```sh
sh tools/ci-freebsd.sh
```

Package construction is ordinary-user work. Dependency/candidate installation and
bounded service lifecycle checks use the disposable CI image's existing sudo
access. The service test starts the machine bus and tests root-only login1
ownership, onestart/quietstart/faststart, status/stop and refusal of another owner;
it does not persistently enable the daemon. These checks do not run on the laptop.
CI's machine image and the physical
laptop are separate validation environments. CI cannot prove real VT switching,
DRM master revocation, input permissions or a native GNOME login.

## Release/ports submission gate

1. Finish and validate the advertised compatibility scope.
2. Cut a signed source release, make its source tarball public, and generate
   distinfo from those exact bytes. Verify fetching from the public URL.
3. Run portlint/portfmt as appropriate and test with poudriere on supported
   FreeBSD releases/architectures; retain logs and exact source revision.
4. Check package ownership, uninstall behavior, nondefault PREFIX and runtime
   dependencies. The current library is static; do not add USE_LDCONFIG until a
   shared library with a deliberate ABI exists.
5. Keep the rc.subr service disabled by default. Validate its real system-bus
   ownership policy and startup/shutdown on a disposable VM, alongside the private
   protocol tests for controller identity, client death and backend failure.
   Physical device/VT recovery still requires separate acceptance testing.
6. Submit one proper new-port change with accurate scope and test limits.

[FreeBSD Porter's Handbook](https://docs.freebsd.org/en/books/porters-handbook/)
is the upstream authority; the overlay is a candidate, not an accepted port.

## Beta-to-laptop delivery policy

Denis requested this iteration path on November 4, 2026:

1. Set the candidate port's DISTVERSION to the intended beta version (for example,
   `0.1.0-beta1`) before the release build. The ports framework supplies the
   corresponding package version; CI derives archive/package names from it.
2. Require a successful native FreeBSD VM build at the exact candidate commit.
   CI preserves the actual source tarball it packaged, the `.pkg`, snapshot
   distinfo, `SHA256SUMS`, build provenance and ports revision.
3. Publish a GitHub prerelease with a signed version tag and explicit implemented
   scope/limits. Upload the CI-produced `.pkg`, matching source archive, checksums
   and provenance. Verify the uploaded assets by downloading them again.
4. Only then download the package from that GitHub release on the laptop,
   verify its checksum against the release manifest, inspect ABI and dependencies,
   and install the downloaded `.pkg`. Use that same release URL for reproduction.
5. Coordinate final native VT/device testing with the parallel kernel work;
   preserve a working boot/session recovery path and distinguish package rollback
   from rollback of any future configuration changes.

The first beta candidate only supports discovery tests; its release notes must
state this scope. The first package is not a working GNOME desktop.
Do not publish the laptop-built exploratory artifact as the CI beta artifact.
