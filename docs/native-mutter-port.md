# Native Mutter port experiment — November 4, 2026

Priority: obtain a real GNOME Wayland session in a disposable FreeBSD VM, then
perform final laptop acceptance. Console color exploration is deferred.

The stock FreeBSD Mutter port disables its native backend. This experiment
changes that setting and rebuilds the package; it does not treat the setting as
an immutable platform restriction. Start with Mutter 47.10, matching the laptop's
GNOME generation and the complete pinned ports recipe, before attempting a GNOME
major-version upgrade at the same time.

Ports revision: `2072800715389ae49066946c960e6521bac37e6a`.
The release archive's SHA256 is
`ee8a583c2b6ff309b501dc97e7c0b4f11d6197a9529ed22247ee95e89663e969`.
The ports framework verifies its existing distinfo during the build.

The overlay enables `native_backend`, adds the GLESv2 dependency and selects the
explicit `bsd3-login` static pkg-config dependency on FreeBSD. The bridge archive
is compiled as position-independent code for linking into Mutter's shared library.
It does not impersonate libsystemd, enable systemd services or claim systemd's ABI.
Consumer patches modify GPL-licensed Mutter source and retain that upstream
licensing; the original bridge code remains BSD-2-Clause.

`tools/ci-mutter.sh` applies the overlay to the pinned complete port, installs the
exact bridge package produced by the preceding native build, installs binary
build dependencies and runs ports stage/check-plist/QA/package. It records actual
kernel/userland versions, ABI, compiler, dependencies, configuration and failures
as private CI artifacts. No native Mutter package is installed into the VM yet;
only the bridge package and build dependencies are installed. No laptop changes.

First falsifiable prediction: the modified port will configure against BSD3 and
reach compilation of the native backend. Compilation errors establish the next
platform adaptations; a successful build still needs native VM runtime testing,
real device/VT ownership, GNOME session startup and final hardware acceptance.
A headless compositor test alone will not establish device/VT correctness.

Working packages compatible with 15-STABLE are authorized for Denis's existing
EhWS package catalogue as well as GitHub beta assets. Verify actual ABI, dependency
closure and appropriate native runtime results first. Do not publish failed or
untested candidates, overwrite unrelated catalogue entries, or call a CURRENT
build proof of 15-STABLE runtime compatibility. Package publication remains pending.

Pre-build native source review also found Linux-only header includes in the device
pool/launcher and an unused dma-buf header in the KMS implementation. The overlay
guards those includes on FreeBSD. The native thread code uses FreeBSD's documented
[pthread_getthreadid_np](https://man.freebsd.org/cgi/man.cgi?query=pthread_getthreadid_np)
in place of Linux gettid; this is a real kernel thread ID, not an opaque pthread
handle. These adaptations still require the native compile/runtime trial.

Patch containers retain required context whitespace. `.gitattributes` excludes
those containers from generic whitespace checks; the complete applied source
patch stack was checked separately without whitespace findings.
