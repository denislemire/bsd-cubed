# Ports overlay

`packaging/freebsd/bsd-cubed` is a `sysutils/bsd-cubed`-shaped overlay: plist,
BSD-2-Clause, native make, GLib/pkgconf, ConsoleKit, `test` target, no systemd.
Install leaves the rc service off.

The next candidate is `0.2.0-alpha4`. It adds the package-owned display-idle
service policy and SDDM/labwc helper described in
[display-idle-packages-2026-11-18.md](display-idle-packages-2026-11-18.md).
The last published and installed base package remains `0.2.0-alpha3` until the
alpha4 native build and laptop rollout finish. See [STATUS.md](STATUS.md).

On a disposable FreeBSD box:

```sh
sh tools/ci-freebsd.sh
```

That snapshots source, then `makesum` / `stage` / `test` / `check-plist` /
`stage-qa` / `package`. I publish GitHub prereleases from those bits and
install the downloaded `.pkg` on the laptop, with a boot environment I can
fall back to. A tree sitting in `/usr/obj` is not a release.

`tools/ci-display-idle-packages.sh` is the shorter release gate for this stack.
It builds BSD3, its patched ConsoleKit and Dinit dependencies, GNOME Settings
Daemon 49.1_1, and its GNOME 49 schemas; runs ports test/plist/stage QA; installs
the exact packages;
checks the shipped opt-in and helper; and emits source, recipes, provenance and
checksums under `dist/display-idle/`.
