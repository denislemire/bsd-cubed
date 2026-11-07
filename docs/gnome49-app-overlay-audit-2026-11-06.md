# GNOME49 application overlay source audit — 2026-11-06

State: native VM packages and bounded Wayland launch/lifecycle checks verified;
combined release CI, publication and laptop integration still open.

These nine candidate overlays use FreeBSD ports commit
`2072800715389ae49066946c960e6521bac37e6a` and GNOME's authoritative
`gnome-49` manifest at commit
`75d7cf1166db3e0e938d2c12a3195b9e8d8f34fa`. They supplement the existing
BSD3 session/compositor work; they do not change the kernel, session graph,
installed desktop, or release artifacts. Upstream consumers retain their own
licenses. No implementation was copied into the BSD-licensed bridge.

[GNOME49 manifest](https://github.com/GNOME/gnome-build-meta/tree/75d7cf1166db3e0e938d2c12a3195b9e8d8f34fa/elements/core)
and [pinned FreeBSD ports](https://github.com/freebsd/freebsd-ports/tree/2072800715389ae49066946c960e6521bac37e6a)
are the source authorities. Each recipe README identifies its exact upstream URL,
origin, hash, dependency changes and validation boundary.

| Overlay | Version | FreeBSD origin | SHA256 |
| --- | --- | --- | --- |
| `gnome-backgrounds49` | 49.0 | `x11-themes/gnome-backgrounds` | `01fe260de58ba00e44aba589a998e33e8bc7aecc701df0bee64cf4cfba44f4b5` |
| `tecla49` | 49.0 | `x11/tecla` | `2ca424e402baf60cd6b13777703b701ebb1faf8f3d0f2f971144d823f651249f` |
| `gnome-calculator49` | 49.2 | `math/gnome-calculator` | `ddf4cd2edda135c41c8af68f9c0cdcd9d9a9163cb9f7f8e28ca2c8e81fd87657` |
| `gnome-characters49` | 49.1 | `deskutils/gnome-characters` | `795c0fe434e602dc783391fb5830dbc7d8e1ea05ca77210fb140e2f6f40a145c` |
| `gnome-font-viewer49` | 49.0 | `deskutils/gnome-font-viewer` | `7c018925c285771b55d7d1a6f15711c0c193d7450ed9871e20d44f2548562404` |
| `baobab49` | 49.1 | `deskutils/baobab` | `6243c92002be7e91f5decd249612face2a4a12d3742afd88b086a94b875dffe0` |
| `gnome-text-editor49` | 49.2 | `editors/gnome-text-editor` | `f838356939203cbd88fbf4f65790734b8915f5fada320a7c6a252b1ed3eb89fa` |
| `gnome-system-monitor49` | 49.1 | `sysutils/gnome-system-monitor` | `915b6a321ada12eba7bf578c20c9fe5e41f55d532847cbd124bbddaaec11d70f` |
| `gnome-clocks49` | 49.0 | `deskutils/gnome-clocks` | `bf76915f2a492e8a0592fe40b35346593aa39e4e6881d6176e0efd8771d4e6fa` |

All nine archives were downloaded from official GNOME source URLs and checked
against the SHA256 references in that pinned manifest. Tecla's one FreeBSD patch
and System Monitor's three patches were applied to freshly extracted sources with
patch fuzz explicitly set to zero. System Monitor's math and duplicate-launcher
patches required rebasing. Its NI_IDN fallback retains the FreeBSD behavior while
correcting the inherited misspelled feature-test macro `NI_IDM` to `NI_IDN`.
Calculator additionally carries the native fixes documented below; the other six
recipes have no carried source patch files. Baobab's existing
post-patch removing the directory MIME association remains in the recipe.

Calculator now explicitly build-depends on Blueprint compiler. System Monitor
now explicitly build-depends on Catch2; its upstream tests require
`catch2-with-main`. No tests or minimum-version checks were disabled. The existing
System Monitor `-Dsystemd=false` option remains supported by upstream; Linux
systemd process annotations are not advertised as available on FreeBSD.

Clocks now requests `gnome-desktop-4`. Characters' JavaScript entry point selects
`GnomeDesktop`4.0, so it also requests the matching `gnomedesktop4` ports
component. Both are supplied by independently versioned gnome-desktop44.5;
there is no fabricated library49 package version. Clocks keeps its ports epoch.

The pinned dependency recipes satisfy the newly relevant minima: GtkSourceView
5.18.0 >=5.15.0, libspelling0.4.10 >=0.4.0, libgtop2.41.3 >=2.41.2 and
`glibmm26`2.86.0 >=2.68. The installed GTK4.20/Adwaita1.8 foundation reported by
the parent meets the source requirements, but this is not a native configure test.
A clean build must independently resolve all dependencies through the ports
framework; source fallback downloads must not hide a missing dependency.

Candidate plists preserve the pinned port inventory, with source-supported
translation/metainfo updates. Backgrounds' candidate plist is derived from the
upstream explicit image/XML install lists. These inventories are not claimed to
be staged or validated: native stage/check-plist/QA is mandatory before packaging.
Both the old47 background recipe and49 use JPEG XL; the update is not a newly
introduced format switch. Actual wallpaper loading through the FreeBSD Mutter
GdkPixbuf path remains part of runtime qualification, with no claim of glycin/HDR
support or silent image conversion.

Suggested serial qualification order: Backgrounds/Tecla, Calculator, Clocks,
Characters, Font Viewer, Baobab, Text Editor, then System Monitor. Native app
launch, ordinary file/font operations, calculator upstream tests, clock alarms,
text spelling/editorconfig and system-monitor process/CPU/memory behavior need
checks appropriate to each package. In particular, geolocation availability,
process signaling and affinity must be observed, not inferred from compilation.

The statements above describe the initial source-preparation checkpoint. The
following native checkpoint supersedes its pending build/launch status, while
preserving the remaining feature limits.

## Native qualification checkpoint

On 2026-11-06 (America/Edmonton), the primary generic EhWS FreeBSD15-STABLE VM
built and staged all nine recipes, passed check-plist and stage-qa, and produced
packages. Stage QA output was checked for errors explicitly; its command exit
status alone is insufficient. The VM reports15.2-PRERELEASE, stable/15 revision
`646a485c3b2f`, Clang21.1.8, and uses the pinned ports revision above. These are
manual VM candidates; a fresh combined CI build and immutable prerelease are
required before laptop installation.

Eight actual GUI programs mapped Wayland toplevels and attached buffers under
the real CK-backed Session49/Shell49/SettingsDaemon49 graph, in both isolated
and normal system-autostart modes. The checks also verified Settings49.9 About,
future display propagation, ordinary autostart, normal session logout and bus/
socket cleanup. Six apps exposed a quit action and exited normally; Calculator
is intentionally non-unique and Tecla exposes no quit action, so those two used
bounded test-process cleanup. This does not claim their normal interactive exit.
Backgrounds' actual `adwaita-l.jxl` decoded through GdkPixbuf as4096×4096; no
hardware wallpaper/HDR qualification is inferred.

Corrections and failures retained for review:

- Calculator required an explicit libm dependency for native `isfinite` linking.
  Its upstream Equation suite then exposed five negative-real square-root/power
  failures. Unary negation can leave an incidental negative-zero imaginary part;
  the root implementation now normalizes real inputs before reciprocal/root
  evaluation while preserving nonzero complex branch sides. New Number tests
  failed against the original implementation and passed with the fix. All seven
  upstream suites then passed, including unchanged Equation expectations.
- Font Viewer's native stage QA found its missing Cairo dependency; the recipe
  now declares it. Text Editor and System Monitor plists were reconciled with
  actual staged files. All corrected packages passed subsequent plist/QA checks.
- System Monitor's initial no-rebuild test invocation lacked an upstream desktop
  validation target. Allowing Meson to build its declared test targets produced
  three passing suites without disabling tests or installing the KDE launcher.
- The first GUI fixture incorrectly assumed Calculator owned a unique bus name.
  Its actual NON_UNIQUE application model is now handled explicitly; its live
  process and Wayland traffic remain required.

Ordinary file/font/document operations, alarms/geolocation, editor spelling,
process signaling/affinity, hardware presentation and laptop resume are not
qualified by these launch checks. Shell calendar-server timezone warnings were
observed in the isolated VM and are not described as fixed. No laptop packages,
services, kernel or boot selection were changed during this checkpoint.
