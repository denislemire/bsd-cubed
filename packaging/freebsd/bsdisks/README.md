# bsdisks native swap visibility correction

Based on FreeBSD ports revision in `PORTS_REVISION`, bsdisks0.41. Upstream
consumer code and patches retain BSD-3-Clause. The original regression fixture
is BSD-2-Clause; no implementation from systemd or Linux udisks was copied.

FreeBSD GEOM identifies a partition as `freebsd-swap`, but filesystem probing
can still find a stale NTFS signature. On the tested laptop this exposed active
2GiB swap in GNOME as a mountable “2.1GB volume”. The fix makes the UDisks2
`HintIgnore` property true for an actual FreeBSD swap partition, independent of
filesystem signatures or drive bus. It preserves the existing internal/USB EFI
policy and ordinary filesystem visibility. No partition contents are changed.

The native test links the real Block class and reads its actual Qt property for
eight metadata combinations without opening or modifying a device. Native15-STABLE compile, stage, check-plist, stage-QA and packaging passed.
The same real-class fixture failed the three swap cases against pristine upstream
and passed all eight cases with the correction. Its first link attempt lacked
production configuration definitions; including the actual upstream entry-point
source under a renamed, uncalled main supplied those definitions without a mock
implementation. Native release qualification and verified-release installation
remain required before laptop integration. The partition has not been changed.
