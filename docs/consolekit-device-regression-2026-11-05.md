# Real input lease regression — November 5, 2026

State: stock-backend failure reproduced on stable/15; port correction under test.
No laptop or kernel changes.

The ordinary-user test used the actual installed BSD3 daemon and ConsoleKit2
2.0.0_2 on FreeBSD15.2-PRERELEASE stable/15-n286217-84aa3cb3204d. It opened a real
Wayland/user CK session, acquired its controller and requested a VM input device.
The known locality-classification error was logged explicitly in this diagnostic;
the official locality acceptance assertion remains strict in the separate test.
No keyboard events were read.

Observed: session_active=0, inactive_reply=0, while EVIOCGVERSION succeeded. The
requested/handed character-device identities matched. The test released the
lease and control before asserting the result. Thus the wrong inactive reply is
not merely a mock/XML disagreement: it occurred at the actual backend boundary.
The descriptor also had not been revoked.

The matching kernel's evdev cdev ioctl entry rejects revoked clients with ENODEV;
its poll entry reports POLLHUP. A typed probe compiled against the installed
native headers reported EVIOCREVOKE=0x20044591. Disassembly of the installed
ConsoleKit binary instead showed the fallback request0x80044591, an input-pointer
ioctl invoked with NULL. Binary SHA256:
`ee5b21762f002ddb9a3615b24afa79bb8a8947904d357df6d04794ab4c3b053a`.

The correction uses FreeBSD's immediate-integer ioctl encoding when the optional
input header is absent. Initial inactive opens fail closed if revocation fails.
TakeDevice reports the actual device inactivity; fallback opens record their
inactive state. Linux's fallback encoding remains unchanged.

The native regression checks character-device identity, the inactivity reply,
read-only EVIOCGVERSION and poll state, and holds its handed descriptor across
ReleaseDevice to check that it remains revoked. It reads no input events. The
current automated fixture exercises an inactive session; an actual active VT,
pause/ack/resume, DRM and hardware recovery still require separate testing. DRM
fallback state is corrected in source but has not been exercised with a GPU.

The corrected dependency is consolekit2-2.0.0_4. The BSD3 candidate requires that
version, and CI builds it before checking the BSD3 port so package dependency
metadata cannot silently select the known-broken backend. Real service/session
and kernel-device regressions run before the longer Mutter/GNOME stages.
Complete modified dependency source and the exact port recipe accompany packages.

## November 5 verified follow-up

PR9 native job 31 passed the corrected backend's strict locality and actual kernel
inactive-input regression, then full GNOME47 session/window/logout. PR8 CI30
also passed strict full GNOME. These are headless session results. The subsequent
[real VT/device regression](native-vt-regression-2026-11-05.md) verifies active
input, pause/revoke and fresh-FD resume on stable/15 KVM with ConsoleKit revision5.
Physical DRM/input and laptop login remain unverified.
