# GNOME Shell49 native consumer — November6,2026

State: native component verified; full49 session, CI release and laptop trial
remain open. The laptop retains its working GNOME47 environment.

## Inputs and boundaries

The port derives from FreeBSD ports2072800715389ae49066946c960e6521bac37e6a,
with GNOME Shell49.10 source SHA256
bf75c2d07063b14c08489ddf98b425436af555b93244a7dbc4941b6e7843fc1e.
The archive matched download.gnome.org's release checksum. Its GPL/LGPL terms
and attribution remain intact; the original BSD3 SDK remains BSD-2-Clause.
The port requires user-session SDK>=0.2.0.a1 and libmutter17,
with explicit static SDK linking and a configure-time version check. It does not
claim that an old BSD3 daemon package alone provides the new service graph.

The VM runs FreeBSD15.2-PRERELEASE stable/15-n286624-646a485c3b2f,
clang21.1.8, GLib2.88.3, Mutter49.8, GJS1.86.0 and desktop schemas49.1.
It has no DRM device. Full GPU/VT/input acceptance belongs to subsequent
native integration and hardware trials.

## Changes and observed failures

Shell uses the BSD3 readiness pipe when explicitly supplied by the native
session graph, and aborts on publication failure. Standalone invocations keep
the standalone path. No mandatory GDM dependency is introduced; the working
SDDM path can remain while GDM49 userdb compatibility is separate work.

Applicable FreeBSD ConsoleKit consumer changes were carried forward against
the actual49 source. New screen-time history requires a user-wide aggregate
idle-state object that ConsoleKit does not provide. The native backend
explicitly reports this accounting capability absent, so the state machine
stays disabled. One session is not falsely substituted for the whole user.
Boot-loader menu selection reports no capability and explicitly throws when
requested; it does not silently succeed. Real suspend support remains a
separate existing ConsoleKit contract.

The actual screencast service initially threw before discovering GStreamer
capabilities: its new introspection provider rejects null argv. Passing an
empty argument array initializes the actual GStreamer API. This correction
does not establish end-to-end recording or screen-sharing support.

Native compilation exposed an undefined executable environ symbol when the
SDK static archive was linked into Shell's shared library with -z defs. The
SDK now spawns with an explicit minimal environment. The corrected archive
passed an actual FreeBSD shared-library link, and Shell compiled afterward.

The new versioned Python app-list generator, libsass/sassc dependencies and
libmutter17 private typelibs are accounted for. Upstream49 no longer installs
org.gnome.Shell.desktop or the Shell/St/Shew GIR sources; the manifest follows
the actual install rules, retaining runtime typelibs. Package instructions
no longer advertise the obsolete gnome-xorg-only startup path.

## Verification and limits

Clean native compilation, stage, check-plist, stage-QA and package passed.
The final compatibility/API-version changes were rebuilt and restaged against
the actual0.2.0.a1 SDK package, with the same package QA and runtime tests.
A clean CI rebuild of these final bytes remains required before publication.
The actual-source fixture exercises absent user-accounting capability,
boot-menu denial and GStreamer argv initialization in real GJS.
An ordinary an ordinary test UID, private-bus headless Shell reached D-Bus ShellVersion49.10,
served a real Wayland GTK window and exited normally with status0. The test
waits separately for the Wayland socket and Shell D-Bus initialization;
a socket alone was initially too early to query ShellVersion.

The final diagnostic VM package SHA256 is
d1e8e483827b7c51e13eb7168f9086ecf754f8cbbbb70d2a4d83000e3be6cc98.
This identifies a VM prototype, not an authorized hardware release asset.
The private test deliberately lacks real system services; warnings about
Polkit, audio, AccountsService and GDM there are not service integration proof.

A shared-lab storage outage killed the VM during staging. A read-only
filesystem check identified damaged build-stage directories. The same disk
was repaired and rechecked clean; installed package integrity checks passed.
The interrupted Shell tree was removed and rebuilt cleanly before these
results. No laptop, kernel, Wi-Fi or boot environment was changed.

Remaining: coherent native session/settings graph, real CK ownership and
activity integration, normal full-session startup/logout, extension compatibility,
successful serialized native package, matching source/recipe beta artifacts, catalogue
publication and an agreed laptop login trial with recovery preserved.
