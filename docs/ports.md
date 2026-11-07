# Ports overlay

`packaging/freebsd/bsd-cubed` is a `sysutils/bsd-cubed`-shaped overlay: plist,
BSD-2-Clause, native make, GLib/pkgconf, ConsoleKit, `test` target, no systemd.
Install leaves the rc service off.

DISTVERSION is currently `0.2.0-alpha3`. See [STATUS.md](STATUS.md).

On a disposable FreeBSD box:

```sh
sh tools/ci-freebsd.sh
```

That snapshots source, then `makesum` / `stage` / `test` / `check-plist` /
`stage-qa` / `package`. I publish GitHub prereleases from those bits and
install the downloaded `.pkg` on the laptop, with a boot environment I can
fall back to. A tree sitting in `/usr/obj` is not a release.
