# Ports and EhWS native CI

The candidate port is `packaging/freebsd/bsd-cubed`, intended for
`sysutils/bsd-cubed`. It has an explicit plist, BSD2CLAUSE license, native make
build, GLib/pkgconf dependencies, ConsoleKit runtime dependency and `test` target.
There is no systemd dependency. No daemon is enabled on install.

The initial repository version is 0.1.0 under development. No upstream 0.1.0
release artifact or checksum is claimed. The release URL in MASTER_SITES is the
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

Dependency installation is the only privileged step in the CircleCI configuration.
The package stage is ordinary-user work. CI's machine image and the physical
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
5. Only ship rc.subr integration and system-bus policy once privilege-bearing
   operations, client death, recovery, startup and shutdown have been validated.
6. Submit one proper new-port change with accurate scope and test limits.

[FreeBSD Porter's Handbook](https://docs.freebsd.org/en/books/porters-handbook/)
is the upstream authority; the overlay is a candidate, not an accepted port.
