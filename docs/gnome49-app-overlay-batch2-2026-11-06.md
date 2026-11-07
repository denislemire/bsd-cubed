# GNOME49 application overlays: second source batch — 2026-11-06

State: source preparation verified; native configure/package/runtime still open.

The source authority is GNOME BuildStream `gnome-49` at
`75d7cf1166db3e0e938d2c12a3195b9e8d8f34fa`; consumer recipes start from
FreeBSD ports `2072800715389ae49066946c960e6521bac37e6a`. These new overlays
are source candidates, not qualified or published packages. No live account data,
mail, browser profile, session configuration or installed package was changed.

[GNOME49 manifest](https://github.com/GNOME/gnome-build-meta/tree/75d7cf1166db3e0e938d2c12a3195b9e8d8f34fa/elements)
and [pinned FreeBSD ports](https://github.com/freebsd/freebsd-ports/tree/2072800715389ae49066946c960e6521bac37e6a)
provide provenance. Evolution and shell-extensions are **not** present in that
manifest: their versions come from the official GNOME49-compatible release
series and corresponding published checksum files, identified separately below.

| Component | Source version | Selection | SHA256 |
| --- | --- | --- | --- |
| `nautilus49` | 49.6 | Pinned GNOME49 manifest | `b68ab4185faa35e80d22825477d681a563a7606622eed24018b1bcbee83b26cc` |
| `epiphany49` | 49.9.1 | Pinned GNOME49 manifest | `0b6aafe1f51da155a19825135c8dc9955587c64f8dff3db8954e829a0f01006b` |
| `gnome-calendar49` | 49.1 | Pinned GNOME49 manifest | `0c1115ab294d532c4c19f56ce0d7aee53f8ea32b1430209dd1d9eda47a83d2c2` |
| `gnome-maps49` | 49.7 | Pinned GNOME49 manifest | `e6861ce65470fc8e1b933dafd71690feeccb84261ed7131d2175f8e917312815` |
| `gnome-console49` | 49.2 | Pinned GNOME49 manifest | `27b02ce8e890ff8f53ca85db786b1ae35903f0b6327812f0224dc0b54b1088db` |
| `papers49` | 49.8 | Pinned GNOME49 manifest | `1a9c426b0ead4779813f0e483a7f124f4b4f4264bc752bc426afb34c271072e6` |
| `gnome-shell-extensions49` | 49.1 | Official supplemental release | `c9718bcdd9ceec26913268c93c349f717a9a39945cac043e5cd88935b16c0423` |
| `gnome-online-accounts49` | 3.56.5 | Pinned GNOME49 manifest | `23e03dcfa03950b60e9f892f7b729ee31deeb827c655df8f78b8bca5bc197734` |
| `evolution-data-server49` | 3.58.3 | Pinned GNOME49 manifest | `da136d1190d9e8b09499452f6058e24eee6b77f441a8c769f74a52451bf379f1` |
| `evolution49` | 3.58.3 | Official supplemental release | `5d01b79a5bebdf2d3a9871b61350939cf2501f6ea03c241543f744a9286d4d79` |

Supplemental release checksums:

- https://download.gnome.org/sources/gnome-shell-extensions/49/gnome-shell-extensions-49.1.sha256sum
- https://download.gnome.org/sources/evolution/3.58/evolution-3.58.3.sha256sum

Every source archive was SHA256-verified. All ten carried portability patch
files apply to fresh pristine archives with fuzz explicitly zero. Console's
old patch is removed: upstream49 correctly uses NULL for the GPtrArray return
contract in `kgx_pages_get_children`, so removing the object check is obsolete.
EDS's Camel SQLite double-quoted-string compatibility patch was rebased across
nearby upstream function registration changes; the remaining five EDS patches
apply unchanged. Upstream consumer and dependency licenses remain their own;
none of this implementation is relicensed into the original BSD2 bridge.

## Concrete dependency and feature boundaries

- Maps49.7 requires **libshumate>=1.5.alpha**; the pinned port is1.4.1. This is
  a confirmed blocker. The candidate explicitly declares the version dependency;
  upstream API checks are retained. A dependency overlay must be qualified before
  Maps is buildable in the pinned tree.
- Nautilus49.6 requires GLib>=2.84, GTK>=4.17.5, desktop4 and Tracker/TinySPARQL.
  ICU is now explicitly declared. Existing Localsearch/GVfs policies remain.
- Epiphany49.9.1 uses WebKitGTK6.0>=2.43.4, GTK>=4.13.3 and Adwaita>=1.8.alpha.
  Blueprint is explicit. The pinned2.46.6 WebKit source satisfies configure's
  minimum, which is not a browser-security or runtime guarantee. Preserve the
  previously qualified hardware rendering environment during later rollout.
- Calendar49.1 needs GLib>=2.80, GTK>=4.15.2, Adwaita>=1.8.alpha and EDS GTK4 UI
  API>=3.45.1. Existing3.56 can satisfy its API floor; upgrading EDS is a coherent
  release update, not a requirement fabricated by weakening/raising source checks.
- Console49.2 requires VTE GTK4>=0.77, GTK>=4.19, Adwaita>=1.8 and GLib>=2.80.
  Pinned VTE0.80.5 satisfies the source minimum. No PTY/session test is claimed.
- Papers49.8 needs Rust>=1.83, Poppler>=25.01 and Blueprint>=0.16. Pinned Rust1.99
  and Poppler26.04 meet these minima. All253 registry archives in its actual
  Cargo.lock were downloaded and verified against locked SHA256; exact sizes and
  hashes replace stale48 Cargo metadata. There are no Git-sourced dependencies.
  Crate licenses/notices remain intact. No Cargo/Meson compilation was performed.
- Shell-extensions49.1 requires Shell>=49. Classic mode is disabled because the
  BSD3 native session graph has not qualified Classic session startup. The old
  Xorg-only post-install instruction is removed. Extension code is packaged
  without automatic enabling; the FreeBSD system-monitor-extension exclusion
  remains. This is not work on the deferred native greeter.
- GOA3.56.5 is the version GNOME49 selects. The removed `windows_live` option is
  dropped. Existing FreeBSD Kerberos/Fedora limitations are retained (Linux
  keyutils is unavailable); no fake provider support or token changes are added.
- EDS3.58.3 matches the manifest. Evolution3.58.3 requires EDS>=its own source
  version, so its recipe declares >=3.58.3 explicitly. Install/build EDS before
  Evolution. EDS retains both GTK3/GTK4 UI choices and native non-systemd unit
  policy. Inherited misspelled `ENBALE_GTK`/`ENABLED_INSTALLED_TESTS` options are
  corrected. EDS build tests remain upstream-enabled; installed-test payloads
  remain disabled by the recipe's existing intent.

Calendar's dependency order is GOA → EDS → Calendar; Evolution also follows EDS.
Nautilus precedes Console/Papers extension integration. Maps must wait for
libshumate. Epiphany and shell-extensions can be qualified separately against the
existing49 foundation. The single serial native-build capacity remains owned by
the parent; this source task starts no VMs/builds/CI and performs no signing/push.

The pinned Nautilus recipe's `tests=none`, Epiphany `unit_tests=disabled` and
`b_lundef=false`, and Papers Cargo delegation to Meson remain visible inherited
settings, not passing-test claims. Actual upstream tests need explicit native
qualification. Candidate plists include reviewed source catalog changes but
remain **unvalidated** until native staging/check-plist/QA. Rust vendoring,
consumer linking, runtime launch, actual browsing/file/document operations and
normal exit must be tested before release. No live mail may be sent or accounts
changed as part of package validation without specific authorization.

## Session49.3 maintenance assessment (read-only)

The same manifest selects Session49.3, source SHA256
`b424a90cfe51de4941b791a5102aeaadb2c62c185522a21f71cb485270053fe1`.
Its official archive was verified. The existing BSD3 native-manager patch applies
without fuzz to all source/tool/data hunks; only the top-level Meson dependency
hunk rejects because49.3 removed an unused JSON-GLib dependency from context.
This needs a narrow context rebase, followed by complete native lifecycle tests.
The installed/existing session49 overlay has **not** been edited.

Upstream49.1 fixes launched-app zombie reaping, inhibit argument parsing,
non-systemd session-bus address discovery and unset XDG_SESSION_TYPE handling.
49.2 fixes offline-update shutdown/reboot handling;49.3 adjusts MIME defaults and
removes unused dependencies. These changes must be reconciled against BSD3's
native manager, with actual private-bus/system-owner, autostart/display propagation,
strict clean logout and no orphan-process tests rerun before replacing49.0.

## First native dependency checkpoint

On 2026-11-06 (America/Edmonton), libshumate1.5.5 and GOA3.56.5 built, staged,
passed check-plist and stage-qa, and packaged in the primary generic15-STABLE VM.
Both were installed only in that VM. libshumate's DOCS-enabled staged inventory
required nine new documentation entries and removal of one replaced method page.
GOA's source no longer installs the old schema or MSN icons; their stale recipe
entries were removed. The actual source version/dependency checks remain intact.

All16 libshumate upstream suites passed with vector rendering enabled, including
GTK map/marker tests under an isolated headless Mutter49 Wayland compositor.
The compositor exited normally afterward. This qualifies those library tests,
not Maps tile/network/geolocation behavior. Consumer builds and the remaining
second-batch runtime checks are still open; no new release or laptop deployment
is implied by this checkpoint.

## Evolution Data Server native checkpoint

EDS3.58.3 now passes native build/stage/check-plist/stage-qa/package and all100
upstream CTest cases. Tests ran serially as the ordinary VM actor with a fresh
private home and session bus, without personal accounts/data. The new package
has not been installed in the VM or on the laptop; consumers must be upgraded
coherently for the new Camel66 ABI.

The first native stage exposed missing GTK3 files. Correcting an inherited
misspelled CMake option had unintentionally made GTK4 disable GTK3, although
Evolution still requires the GTK3 UI API. The corrected recipe explicitly keeps
GTK3 and adds GTK4 independently. With OAuth enabled, both corresponding WebKit
interfaces are declared and enabled. The tested CMake cache confirms all four
switches are on. Camel66, changed public headers and the new locale were also
reconciled with actual staged files; existing UI files were restored by fixing
the build configuration rather than deleting required APIs from the manifest.

The missing WebKit41 binary initially led the ports framework into a source
Ruby/WebKit dependency build. That build was stopped, the declared binary
prerequisite was installed, and the successful repeat used a bounded timeout and
USE_PACKAGE_DEPENDS_ONLY to prohibit source fallback. Initial process-selection
checks refused to signal anything because the FreeBSD suppressed-header format
did not produce the expected fields; using the verified header-bearing format
selected only that build's same-user descendants. This was tooling correction,
not a package, kernel or hardware failure. Combined CI and desktop integration
remain open.

## Remaining native package and session checkpoint

On 2026-11-06 (America/Edmonton), the remaining eleven second-batch recipes all
passed native build/stage/check-plist/stage-qa/package on the same generic
15-STABLE VM. That includes Files49.6, Web49.9.1, Calendar49.1, Console49.2,
Maps49.7, Papers49.8, extensions49.1 and Evolution3.58.3, plus the three
previously recorded dependencies. The 23 ecosystem overlays are now packaged
manually; the complete CI controller has not yet run and no alpha3 hardware
rollout has occurred.

Files required real FreeBSD filesystem adaptation: native sys/param.h and
sys/mount.h, O_PATH/fstatfs, and f_fstypename classification instead of Linux
superblock magic. A runtime-extracted actual-function probe failed before the
classification fix and passed six native descriptor/error/classification cases
afterward. Synthetic autofs metadata is explicit; no real automount is claimed.
The upstream headless tests are now enabled. Their thumbnail case also needs a
real display, and its Linux-only bubblewrap prerequisite incorrectly skipped
FreeBSD's actual non-bwrap GNOME Desktop thumbnailer. With that prerequisite
limited to Linux, all19 upstream suites pass under isolated Mutter Wayland,
including actual thumbnails, without skipped suites or warning suppression.

Maps, Web and extensions inventories were reconciled with staged icon/help/
metadata changes. Calendar's stage QA caught a missing direct Graphene dependency;
it is declared now. Papers completed its locked Rust build with two jobs and
now packages document ABI6/view ABI5, backend directory6, new public headers
and actual renamed metadata. Optional backend naming retains the option markers.
Evolution's new public headers and metadata were similarly reconciled. Native
stage-QA Error lines are checked independently of make's return status.

The installed VM packages pass the real Session49.3/Shell49.10/GSD49.1 graph
with14 GUI apps plus Settings49.9 About, in both isolated and normal system-
autostart modes, followed by normal session logout and owner/socket cleanup.
Files, Maps, Calendar and Evolution quit through their actual public app actions;
Console proves a real child terminal/PTY. Papers loads a generated private PDF
through its real document backend and renders a PNG thumbnail before its GUI
maps. Calculator, Console, Tecla and Papers use bounded fixture cleanup where
a normal remotely addressable quit contract is absent; interactive exit is not
claimed for them. Papers intentionally uses G_APPLICATION_NON_UNIQUE.

Two environment/contract corrections are retained: activated helpers initially
inherited the enclosing bus's old home/runtime, producing fatal GVfs path
mismatches; the private bus activation environment now follows the fixture's
private directories. Nautilus app.quit closes windows while retaining its
background service; its public app.kill action calls g_application_quit and is
used for actual process exit. No POSIX kill is substituted for a normal quit.
Calendar initially failed because this generic image had neither /etc/localtime
nor /var/db/zoneinfo. Native tzsetup -s UTC configured only this disposable VM;
Calendar then launched and quit normally, and prior default-timezone warnings
were absent in the new graph. No fictitious timedate1 service was introduced.

## Browser investigation before the correction below

Web49.9.1 maps a real Wayland window but aborts during normal quit from a fresh
about:blank page with the existing !shell->windows assertion. At the ten-second
quit deadline, procstat showed sigexit/elf64_coredump before test cleanup; a
large core write had disguised the abort as a timeout. Matching installed and
unstripped executable/libraries, package and patched source are preserved
privately with the possibly interrupted core. No complete-core claim is made.
Subsequent app tests disable core output so aborts report promptly without
exhausting VM disk. Raw memory and identifying diagnostics remain uncommitted.

The initial test mistakenly used only WEBKIT_DISABLE_DMABUF_RENDERER. The actual
working hardware profile uses WEBKIT_DISABLE_COMPOSITING_MODE=1. Matching that
setting still reproduces the blank-page failure. Changing only the page to
internal ephy-about:overview passes actual normal browser quit and complete
Session49.3 logout. An independent original C11 WebKit6 probe, with the matched
profile setting, executes unique local HTML/JavaScript tokens and validates a
nonuniform two-color visible snapshot; normal GTK cleanup passes. This verifies
the underlying library renderer, not actual browser page execution.

A separate original GTK held-reference probe confirms logical window removal
can precede disposal and finalization. It does not identify the browser's actual
retained owner. Pending-navigation/filter references are a falsifiable hypothesis;
a private diagnostics-only source variant was subsequently compared to select the
consumer fix recorded below. Assertions, form warnings and filter policy have not been disabled.
Tracker/miner startup still emits a SQLite nco:creator error in fresh VM search
fixtures; simple-search tests pass, which does not qualify the indexer.

## Browser ownership correction verified

The private trace established the retained owner: two queued navigation-policy
records each owned the same window while filters were uninitialized. They
remained when GTK removed the logical window; no dispose/finalize occurred before
Shell finalized with one tracked window. The internal overview case had no queue
and finalized normally. This distinguishes the concrete self-reference cycle
from the separate GTK held-reference lifetime demonstration.

The GPL consumer patch now cancels queued decisions on removal of that exact
application window. It detaches the queue before callbacks, disconnects and
resets the filter-handler ID, explicitly ignores ended navigation requests, and
frees the real owning records. The normal initialized callback similarly detaches
its batch and avoids resuming later entries after reentrant window removal.
It preserves the strict Shell assertion, form warnings and filter policy.

Private instrumentation was reversed before rebuilding the clean package.
The original blank-page case now quits normally, the internal overview still
quits normally, and a two-window blank/overview case also passes. The final clean
package passes all15 GUI app fixtures plus Settings49.9 About and complete
Session49.3/GSD49.1 normal logout/cleanup, in both isolated and system-autostart
modes. Page-content execution in the actual browser remains outside these window
checks; the independent WebKit DOM/pixel test is recorded separately.

The complete pipeline configuration validates against the actual EhWS CircleCI
Server using the official Server-compatible v0 CLI. The existing global CLI v1
was left unchanged; a checksum-verified temporary v0 executable is used only by
the task wrapper. This is configuration validation, not a successful combined
pipeline or a published alpha3 release. The source packaging and native CI
candidate are ready for that next validation step.
