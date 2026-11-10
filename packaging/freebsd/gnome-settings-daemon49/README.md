# GNOME Settings Daemon 49 on FreeBSD

Consumer overlay based on FreeBSD ports revision in `PORTS_REVISION`, updated to
upstream49.1 selected by GNOME's `gnome-49` manifest at
`75d7cf1166db3e0e938d2c12a3195b9e8d8f34fa`. The distinfo matches the official
GNOME archive and manifest checksum. GPL/LGPL upstream notices are retained;
these are consumer patches, not BSD relicensing.

This build uses gnome-desktop-4 and the GNOME49 global-shortcuts schema, retains
Xwayland settings support, and disables systemd, ALSA, Linux rfkill, WWAN,
NetworkManager and USBGuard integration. The existing FreeBSD exclusions for
Datetime, Sharing and Housekeeping remain explicit. Their systemd service APIs
are not implemented or advertised by this port.

Unlike the old FreeBSD47 patches, power and media-key login1 requests retain
their upstream destination and reach BSD3's compatibility API. That API now
forwards CanSuspend/Suspend/Inhibit to ConsoleKit after authenticating the
original session peer. No success-returning stubs are installed; hardware
suspend/resume remains a separate trial. Session49's native Dinit
graph owns the seven required settings services; upstream49 no longer installs
the old desktop autostart files. This port therefore belongs to the matching
BSD3 native49 candidate, not an arbitrary legacy GNOME login.

Native15-STABLE clean stage, check-plist, stage-QA and package succeeded. The
upstream color unit test passed. Both isolated and system-autostart actual
GNOME49 session graphs passed with all seven processes mapping the installed
`gnome-settings-daemon-49/libgsd.so`, a real Wayland app, normal logout and owner
cleanup. The integration fixture now reports the queried installed package
version rather than its former hardcoded47 label. This is headless VM evidence;
release qualification and new hardware acceptance remain separate gates.
