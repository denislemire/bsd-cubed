# Remaining GNOME49 ecosystem — November6,2026

State: in progress. The accepted alpha2 laptop remains the default and is not
modified by these generic-VM build experiments. New artifacts are not a published
or hardware-qualified release until the corresponding release record says so.

Authoritative inputs: FreeBSD ports `2072800715389ae49066946c960e6521bac37e6a`
and GNOME's `gnome-49` manifest `75d7cf1166db3e0e938d2c12a3195b9e8d8f34fa`.
The actual49 manifest selects desktop library44.5; its independent version is not
changed to49. Settings' About label derives from its own source major version,
so Settings49.9 is the genuine label correction.

## SettingsDaemon49.1

Clean15-STABLE ports stage, check-plist, stage-QA and packaging passed. The
upstream color unit test passed. The ordinary-user complete GNOME49 graph passed
in both isolated and real-system-autostart modes, including the real Wayland app,
normal logout and complete owner/socket cleanup. The strengthened fixture queries
the installed version, can require49.1 explicitly, and verifies that each of the
seven actual settings processes maps the corresponding49 `libgsd.so`.

An old hardcoded47 test-output string initially mislabeled that new run. It was
replaced with the queried version and strict library mapping, then both tests
were rerun successfully. The original manual build did not create PACKAGES first,
so ports put the archive in WRKDIR/pkg. The built artifact was collected from its
actual path; subsequent build roots explicitly create their package destination.

## Settings49.9

Native compilation completed all616tasks and staged successfully. Manifest QA
found a new GlobalShortcutsRebind interface and24obsolete avatar filenames. The
first correction removed too many face entries; regenerating the manifest exposed
the valid new49 assets as orphans. The correction now removes only the24proven
obsolete filenames and keeps the new installed assets. These were packaging
inventory errors, not a reason to disable manifest checks or remove app content.
Manifest checks then passed. Stage-QA caught an undeclared Wayland client
library used by the new GlobalShortcuts provider; the native dependency was
added explicitly and a repeated stage-QA had no errors. The package was rebuilt.
The real Settings49.9 About window mapped an actual Wayland surface and quit
through its normal app action in both isolated and system-autostart sessions.
Both complete graphs then logged out normally with owner/socket cleanup.

## Swap-volume visibility

The laptop's active2GiB GEOM `freebsd-swap` partition was advertised by bsdisks0.41
as filesystem/NTFS with HintIgnore=false, explaining GNOME's “2.1GB volume”.
No partition data, swap activation or fstab configuration was changed. The new
bsdisks0.41_1 overlay makes real swap partition metadata take priority for the
HintIgnore property, without changing normal filesystem or USB EFI visibility.

Native compile/stage/test/check-plist/stage-QA/package passed. A fixture linked
the actual consumer Block class and read its Qt HintIgnore property. Unmodified
upstream failed all three swap cases; the correction passed all eight cases,
including no drive object, stale NTFS signatures, USB swap, ordinary NTFS/UFS,
unpartitioned storage, and the existing internal-versus-USB EFI policy. The first
fixture link exposed configuration definitions in upstream main.cpp; the test
now compiles that real source under an uncalled renamed entry point instead of
inventing a substitute configuration implementation. No device is opened by the
fixture. Laptop rollout awaits the published, natively qualified package.

## Remaining work and publication

Nine initial app overlays now pass native packaging checks; eight GUI launches
and background decoding have also passed their bounded VM checks. See the
[app validation record](gnome49-app-overlay-audit-2026-11-06.md) for corrections
and feature limits. The [second batch](gnome49-app-overlay-batch2-2026-11-06.md)
adds the remaining app candidates and records native map/account dependency
results. Further application operations and integration services remain in scope;
the first batch is not a complete ecosystem.

The next SDK source snapshot uses alpha3 (package/pkg-config version0.2.0.a3),
preserving the immutable published alpha2 source key. Its explicit source inputs
now include THIRD_PARTY.md. The ordinary-user native ports stage/test/plist/QA/
package sequence passed, and the actual source archive contains that license
boundary document. This is a manual VM checkpoint, not a combined CI or release
claim; no alpha3 package has been installed on the laptop.

Changes are published as signed, DCO commits and reviewable PR updates. Matching
native CI packages, consumer source/recipes, hashes and provenance precede laptop
rollout. Repository-publication review runs in parallel. A Wayland-native greeter
and removal of the Xorg server are explicitly queued after these packages.


The later [second-batch native record](gnome49-app-overlay-batch2-2026-11-06.md)
now documents all23 ecosystem package builds, Session49.3 and all15 actual app
window fixtures in both session modes, the fixed browser navigation ownership
cycle, PDF backend rendering and the separate WebKit DOM/pixel probe. Combined
CI and release/hardware acceptance remain distinct next gates.
