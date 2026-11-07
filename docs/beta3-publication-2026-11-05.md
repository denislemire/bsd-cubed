# Beta3 publication — November 5, 2026

State: released; exact-package VM qualification verified; hardware window closed.

The signed [v0.1.0-beta3](https://github.com/denislemire/bsd-cubed/releases/tag/v0.1.0-beta3)
tag targets8bfa868f3a1c485954d35b9be28b2347b2e4bf30. GitHub verifies its signature.
CI36 passed all
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

Beta3 packages and their complete source/recipe bundle were published as GitHub
release assets. Keep the complete GPL dependency sources accessible to the same
recipients as the binaries.

A native FreeBSD client, using a private temporary configuration/database,
queried the published bsd-cubed 0.1.0.b3, consolekit2-2.0.0_5 and mutter 47.10_3
payloads. No system repository configuration or installed package changed for
that client check. Trial hardware install used the verified GitHub release
assets, with the normal fallback preserved.

## Port source pin

The overlay now includes distinfo for the immutable SDK distfile. The GitHub
release is the MASTER_SITES location. This development overlay has the native
ports Makefile, plist, license, rc.d service, source pin and tested stage/QA
path. 
Physical DRM/display/input, regular-password login and GNOME49+ startup remain
open. No laptop connection, boot environment operation or desktop change was
performed during this publication. No kernel is contained in the three packages.

Native port-pin validation ran as the existing ordinary VM account against the
real pinned ports framework: checksum accepted the published distfile, DISTNAME
resolved to bsd-cubed-0.1.0-beta3, and PKGNAME to bsd-cubed-0.1.0.b3. The unchanged
compiled implementation already passed CI36's full native stage/test/plist/QA
and independent exact-package acceptance; this follow-up adds only source
location/distinfo and this publication record.
