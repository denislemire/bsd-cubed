# Ports overlay

`packaging/freebsd/bsd-cubed` is a `sysutils/bsd-cubed`-shaped overlay: plist,
BSD-2-Clause, native make, GLib/pkgconf, ConsoleKit, `test` target, no systemd.
Install leaves the rc service off.

DISTVERSION is currently `0.2.0-alpha4`. See [STATUS.md](STATUS.md).

`tools/ci-user-session.sh` installs pinned `consolekit2-2.0.0_5` only when
the installed package is older. Set `BSD3_CI_PKG_URL` to the https URL of
that `.pkg`. There is no default. An unset variable is an error when a
download is still required. A `consolekit2` package this job already built
at 2.0.0_5 or newer is installed first, and a download is checked with
SHA256 before `pkg add`. Optional `BSD3_CI_DNS` is the resolver address
for that URL's host. Keep both in the executor environment. The CircleCI
job documents the same contract.

On a disposable FreeBSD box:

```sh
sh tools/ci-freebsd.sh
```

That snapshots source, then `makesum` / `stage` / `test` / `check-plist` /
`stage-qa` / `package`. I publish GitHub prereleases from those bits and
install the downloaded `.pkg` on the laptop, with a boot environment I can
fall back to. A tree sitting in `/usr/obj` is not a release.
