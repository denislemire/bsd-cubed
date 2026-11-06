# Greeter preferences — November 5, 2026

The laptop trial uses SDDM. The user requested12-hour time, ISO8601 date,
a battery percentage if straightforward, and no keyboard-layout selector.
The normal HE-enabled boot environment and normal user settings remain intact;
these preferences apply only to the experimental desktop environment.

The optional example in `tools/hardware/sddm-theme` keeps SDDM's existing
fallback login form and authentication call. Main.qml retains its upstream MIT
license and attribution; PROVENANCE.txt records its source and digest. The small
optional BatteryStatus.qml is original BSD-2-Clause and consumes the already
installed KDE battery model API. It adds no daemon or power-policy mutation.
A Loader keeps an absent optional battery module from preventing the login form.
The original BSD3 broker/library remain C11/BSD-2-Clause; dependency licenses are
unchanged.

Clock strings are explicit `h:mm AP` and `yyyy-MM-dd`; the device timezone is
unchanged. The layout selector and its label are removed and tab order repaired.
The clock fits smaller displays. Existing embedded SDDM resources are referenced,
not redistributed. Battery text uses the toolbar's dark foreground, shows a real
percentage and identifies plugged-in state without claiming it is charging.

A bounded offscreen test-mode preview on the generic STABLE VM loaded the actual
QML and captured the resulting form. Initial800px clipping after addingAM/PM was
corrected with responsive sizing and visually rechecked. No layout selector
remains. SDDM's test-mode process did not handle the QML quit signal and was
stopped by the explicit deadline; this was a preview, not a real authentication
or an unassisted shutdown test. The VM lacks the optional KDE battery plugin and
its absence did not prevent the form. On the laptop, the existing model returned
80%, matching native ACPI; it also returned the actual level under the sddm UID
(79% at that later sample). No extra battery service was installed.

The first archive extraction rejected macOS provenance extended attributes
before theme activation. Contents and original configuration were preserved;
the finalizer used a private hash-pinned extraction with `--no-xattrs --no-acls
--no-fflags --no-same-owner`, then installed the known files with root ownership.
COPYFILE_DISABLE alone does not exclude every BSD tar extended attribute.
Normal Git archives and the FreeBSD-built package artifacts are unaffected.

The laptop now selects `bsd3-trial` in the clone-only SDDM override. Prior config
and wrapper, candidate archive, source provenance and installed hashes are kept
in protected evidence. Actual customized greeter display and password submission
remain to verify at the next hardware login; preview evidence does not prove them.

The isolated session wrapper also adds FreeBSD's standard icon search location to
XCURSOR_PATH (honoring an explicit preexisting value), including the session-bus
activation environment. The installed Adwaita cursor files exist; Mutter's bundled
native cursor loader otherwise defaults to Linux/X11R6 locations and warned that
no cursor theme was available. This configuration does not change the normal
user's GNOME profile. Shell syntax was checked; visible cursor acceptance remains
part of the next physical trial.
