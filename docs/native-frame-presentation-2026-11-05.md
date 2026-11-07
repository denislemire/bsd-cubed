# Native frame presentation investigation — November 5, 2026

State: proposed correction; physical desktop acceptance remains open.

## Observation and preserved identities

Published beta4 used signed source58b802fd8d8f0e0813cffc68f92b15cda1e8e951,
all-success native job 44 and independent15.2-PRERELEASE STABLE qualification.
The new Mutter47.10_4 package SHA256 is
`7c3508611b9209c57fffd22dee739ade8d85df447ed3b9ff0bde5c9487d929e5`.
SDKb3 and ConsoleKit2.0.0_5 were the exact unchanged released CI36 packages.
Generic session/input/VT, headless compositor, full GNOME RequiredComponents
window/logout and guarded SDDM tests passed. STABLE's deliberate greeter stop
needed SIGKILL for five descendants, but restored originalVT1; this is assisted
recovery, not a reliability pass.

On the isolated laptop, normal-password authentication and strict local
Wayland/user VT10 registration succeeded. Unlike beta3, beta4 discovers
`card0`, opens the Intel i915 device with atomic mode setting, creates a GBM
renderer, selects the builtin panel and creates `wayland-0`. The user still sees
a black screen with a cursor, followed by the greeter. Shell47.10 dies with
SIGSEGV. The wrapper exit0 does not establish desktop success.

The trial was stopped through its owned guard; final recoveryPASS/originalVT2,
child0, requestedSIGTERM15, seven terminated/zero killed descendants. No kernel,
Wi-Fi module or HE startup helper changed. The normal HE environment stays the
persistent default; the experimental clone and preupgrade beta3 checkpoint remain.

The private core and all237 mapped object files were preserved on the laptop,
with SHA256 manifests and package/loader identities before further replacement.
Raw memory and identifying logs stay in protected storage and are not committed
or published. LLDB prints function names without argument values. Top PC isNULL,
under a Cogl EGL region-swap implementation and `cogl_onscreen_swap_region`.
Disassembly matches rectangle conversion, framebuffer binding and the indirect
EGL region-swap call. Stripped internal symbols limit exact source-line claims;
no raw struct-offset memory probe was used. Matching fully patched source is
from the checksum-verified CI44 GPL source archive, not a mutable upstream tree.

## Hypotheses and decisive comparison

A cursor-theme warning is not proof of the crash cause. The preserved main-thread
stack instead points to frame presentation. Comparing the complete port-applied
source to upstream47.10 reveals the base port's `meta-stage-impl.c` patch replaces
the full-redraw empty-region branch with an unconditional full clip region.
With no buffer age and a nonempty region, the actual stage swap selector calls
the inherited EGL region-swap callback. That callback is absent for this native
presentation path, consistent with theNULL call in the core.

The existing port patch references GNOME issue2103. Retrieval returned406;
its original rationale was not independently established. Preserve its behavior
for all non-native stages rather than deleting it globally.

Prediction: restoring the upstream empty swap region for native full redraws
selects the native buffer-swap path even without an EGL region callback, while
retaining the established non-native workaround. This should eliminate this
specific first-presentation crash; visible rendering and input remain tests.

## Proposed correction and regression

Replace that base patch in the overlay. With native backend support compiled,
use the typed `META_IS_STAGE_NATIVE` check and restore the upstream full-redraw
branch only for native stages. Non-native stages and builds without native
support retain the previous region behavior. Native clipped redraws still use
their existing buffer-age/damage handling. Consumer package revision becomes5.

The regression compiles and executes the actual port-applied region construction
and swap-dispatch branches with a deliberately absent region callback. It covers
native full redraws with/without buffer age, shadowfb on/off, valid clipped
damage swaps, supported region swaps, non-native behavior and native-disabled
builds. The beta4 source fails the missing-callback fixture; the corrected source
passes both build modes locally. CI runs the fixtures against its fully patched
consumer source and retains the report. No ad hoc compositor binary is installed
on hardware; a new successful native build and published beta are required.
