# Beta3 publication — November 5, 2026

State: released; exact-package VM qualification verified; hardware window closed.

The signed [v0.1.0-beta3](https://github.com/denislemire/bsd-cubed/releases/tag/v0.1.0-beta3)
tag targets8bfa868f3a1c485954d35b9be28b2347b2e4bf30. GitHub verifies its signature.
[CI36](https://circle.ehws.generic.business/gh/denislemire/bsd-cubed/36) passed all
build/test/upload actions. Every one of the11 published payloads was downloaded
again and checked against both the published SHA256SUMS and staged originals.
The manifest itself is the12th asset. Earlier beta assets are unchanged.

The exact three packages passed independent STABLE CK/session/control, real
input revoke and production-launcher VT switch/pause/fresh-FD resume/release,
then full SDDM/GNOME47 headless window/logout and checked console recovery.
Post-reboot identity: all version entries15.2-PRERELEASE, GENERIC stable/15
646a485c3b2f. The external cold-backend runner correction is PR15; it changes
no compiled/package inputs. Requested greeter termination remains assisted,
and the failed initial cold attempt remains preserved.

## Existing package repository

Denis explicitly approved anonymous exposure of these packages and their
complete source/recipe bundle. Publication used create-only S3 writes and verified
anonymous HTTPS readback hashes for every object. It uses an isolated channel:

```text
https://packages.ehws.generic.business/FreeBSD:15:amd64/bsd3-beta3/
```

The three binaries are under `All/`; the published sources, exact dependency
recipes, full repository/port archive, provenance, validation and SHA256SUMS are
under `sources/`. The existing `latest` catalogue and unrelated entries were
not changed. No old packages were deleted. Keep the complete GPL dependency
sources accessible to the same recipients as the binaries.

A native FreeBSD client, using a private temporary configuration/database, updated
the catalogue and queried exactly bsd-cubed0.1.0.b3, consolekit2-2.0.0_5 and
mutter47.10_3. No system repository configuration or installed package changed
for that client check. The VM's default cluster DNS resolved the external edge,
which returned403; the existing lab split-horizon resolver succeeded via the
per-command pkg NAMESERVER option. This is anonymous lab/VPN access, not verified
Internet-wide reachability. Do not silently change guest or host DNS to hide it.

During an explicitly coordinated trial, an opt-in repository entry would be:

```ucl
bsd3_beta3: {
  url: "https://packages.ehws.generic.business/${ABI}/bsd3-beta3",
  enabled: yes,
  signature_type: "NONE"
}
```

This follows the existing unsigned EhWS repository model. No such file has been
installed on the laptop. The requested initial laptop install still uses the
verified GitHub release assets, with the normal fallback preserved.

## Port source pin

The overlay now includes distinfo for the immutable44807-byte SDK distfile,
SHA256 c6326494ddde90f105f29a7f1566c7d693efefd7d2fb423ac23aa9729a92f5b4.
Its architecture-neutral anonymous lab mirror is:

```text
https://packages.ehws.generic.business/sources/bsd-cubed/v0.1.0-beta3/bsd-cubed-0.1.0-beta3.tar.gz
```

The GitHub release remains a second MASTER_SITES location. This development
overlay has the native ports Makefile, plist, license, rc.d service, source pin
and tested stage/QA path. An upstream ports submission additionally needs source
hosting reachable by its build infrastructure; this lab mirror and private
GitHub repository do not establish that requirement.

Physical DRM/display/input, regular-password login and GNOME49+ startup remain
open. No laptop connection, boot environment operation or desktop change was
performed during this publication. No kernel is contained in the three packages.

Native port-pin validation ran as the existing ordinary VM account against the
real pinned ports framework: checksum accepted the published distfile, DISTNAME
resolved to bsd-cubed-0.1.0-beta3, and PKGNAME to bsd-cubed-0.1.0.b3. The unchanged
compiled implementation already passed CI36's full native stage/test/plist/QA
and independent exact-package acceptance; this follow-up adds only source
location/distinfo and this publication record.
