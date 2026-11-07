# GNOME49 runtime dependencies — November6,2026

State: native15-STABLE dependency packages verified; CI and complete GNOME49
integration remain open. No laptop package, service or boot environment changed.
These ports supply GJS1.86.0 and desktop schemas49.1 for Shell49; they do not
install a compositor or create a misleading49 desktop session entry.

## Sources and provenance

Recipes derive from FreeBSD ports2072800715389ae49066946c960e6521bac37e6a.
Release archives came from download.gnome.org and were compared with its matching
upstream sha256sum files. GJS source SHA256 is
63448f7a57804d4c2a8d0c7f5e90e224d04d4eb2d560142c076c65a8eda00799;
schemas source SHA256 is
777a7f83d5e5a8076b9bf809cb24101b1b1ba9c230235e3c3de8e13968ed0e63.
Third-party source retains its original licenses and attribution. These consumer
patches do not relicense GJS or the schemas as BSD. Binary CI artifacts include
complete matching modified source and exact recipes.

## Native validation and fixes

The dedicated VM runs FreeBSD15.2-PRERELEASE from stable/15, GENERIC
stable/15-n286624-646a485c3b2f, clang21.1.8, GLib2.88.3 and SpiderMonkey140.13.0_3.
Both ports passed native stage, check-plist, stage-QA and package. Existing
installed desktop libraries were not replaced during these build tests.

GJS initially failed in SpiderMonkey140's public file-handle headers because
its Unix platform macro was absent. The explicit XP_UNIX compiler definition
selects the provider's actual Unix descriptor representation. A focused header
compile and subsequent full GJS build passed. Its introspection closure wrapper
also returned void** while the installed API returns an opaque void* address;
the consumer's wrapper now uses that actual return type without a hiding cast.

Native functional tests exposed another real compatibility failure: the new
GIRepository2 lookup no longer supplied GLib.UnixPipe, GLib.unix_fd_source_new
and GLib.UnixPipeEnd, while the canonical GLibUnix APIs existed. An initial
stack-callsite-warning hypothesis was disproved: no warning code was reached,
and direct JS probes showed undefined legacy properties. A temporary diagnostic
was removed; its source checksum matched the pristine release afterward.

The namespace resolver now requires the real platform namespace and exports the
same canonical constructor/function/enum under the legacy property, retaining
GJS's deprecation warning. Merely finding its metadata was insufficient: the
canonical property name also had to be exported under the requested alias.
The actual upstream GLib compatibility test now passes all55 subtests. The
selected six native suites passed together:

| Suite | Passed subtests |
| --- | ---: |
| API tests |74|
| GIMarshalling/callbacks |732|
| GObject |25|
| Introspection |16|
| Gio |46|
| GLib |55|

The test launcher reported System.version18600, and its linked library resolved
to the new build. The old installed GJS was not accepted as evidence. This is
selected functional coverage, not a claim that every upstream test ran.

Schema stage QA identified the new screen-time-limits schema and Uzbek message
catalogue. Both are included in the recipe/manifest. Cached generated plist
metadata required a clean schema rebuild before the corrected QA pass.

## Identified native artifacts

| Artifact | SHA256 |
| --- | --- |
| gjs-1.86.0.pkg |0c9d9961c21b5c6a30fb0e1727ab62d0a02ac872096fc49f6a716671df56d1ff|
| gsettings-desktop-schemas-49.1.pkg |1c68333e503910e08c5e157dcce53e7d411a4471c9eb996d491a40d2379992e2|

These hashes identify the local native diagnostic artifacts, not a GitHub beta.
Only successful package artifacts with corresponding source/recipe manifests may be
used for a published hardware candidate. A separate native job rebuilds and
packages these consumers and repeats the selected actual-GJS tests. Its standard
executor is15.1-RELEASE; the manual15-STABLE results above are separate evidence.
Mutter/Shell49, native session/system integration and full desktop acceptance
remain subsequent work.

## CI correction and bounded scheduling

CI job50 built and packaged GJS successfully, then rejected its version probe:
the login shell printed a FreeBSD fortune before18600. The saved artifact
confirmed that this was output contamination, not a GJS version failure.
The controller now invokes the ordinary user through sudo with an explicit
minimal environment, without a login shell or inherited credential variables.
The selected native tests still run as the ordinary user. The workflow orders
runtime builds after the core job to limit peak VM memory on the shared lab.
The corrected complete CI run remains required before release.
