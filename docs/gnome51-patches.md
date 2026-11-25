# GNOME 51 and GDM on FreeBSD

The desktop uses GNOME Shell and Mutter 51.1, GNOME Session and GDM 51.0,
and BSD³ over ConsoleKit. The desktop metapackage and display manager are
separate packages. Upstream components keep their own version numbers.

| Component | Where to find the changes | What they do |
| --- | --- | --- |
| Seat, devices and VT handoff | [BSD³ sources](../src), [ConsoleKit overlay](../packaging/freebsd/consolekit2) | Bind sessions to seats, control devices and hand the VT between greeter and user. The greeter adapter and VT guard live in BSD³. |
| Mutter, Shell and Session | [Shared consumer patches and source hashes](../packaging/freebsd/gnome51), [native port notes](gnome51-native-port.md) | Connect Mutter to native seat/device management, Shell to ConsoleKit signals and Session to the BSD³ system adapter. |
| Session startup and idle | [Session launcher and Dinit graph](../packaging/gnome49), [power plugin patches](../packaging/freebsd/gnome-settings-daemon51/files) | The same launcher serves GNOME 49 and 51. On a GDM password login Shell handles idle, lock and blanking. The optional early display-power fallback serves desktops launched through SDDM. |
| GDM and PAM | [GDM recipe and qualification](../packaging/freebsd/gdm51), [patches, PAM policies and service files](../packaging/freebsd/gdm51/files) | Build upstream GDM with the BSD³ provider, start the Wayland greeter under Dinit, authenticate with OpenPAM, and release the seat before restarting the greeter. |
| User list and branding | [AccountsService patches](../packaging/freebsd/accountsservice51/files), [Shell branding and D-Bus interfaces](../packaging/freebsd/gnome-shell51/files) | Refresh users on ConsoleKit seat changes and position the configured wordmark above the login prompt. Shell supplies the gray background. |
| Graphics and browser | [GTK 4](../packaging/freebsd/gtk4051/files), [WebKitGTK](../packaging/freebsd/webkit2-gtk51/files), [IBus](../packaging/freebsd/ibus51) | Import DMA-BUF textures through the FreeBSD DRM ABI, preserve WebKit IPC record boundaries, and avoid recursive GTK input-context setup. |
| Apps and other dependencies | [Desktop metapackage](../packaging/freebsd/bsd3-gnome51-desktop/Makefile), [ports overlays](../packaging/freebsd), [build order](../tools/ci-gnome51-packages.sh) | The metapackage lists the supported desktop set. Each component's recipe carries its source checksums, patches or pinned-ports delta, and package manifest as needed. |

## Reproduction and checks

The [source check](../tools/ci-gnome51-source.sh) verifies archive hashes,
applies the four shared consumer patches with zero fuzz, and exercises the
native Session adapter. The [package job](../tools/ci-gnome51-packages.sh)
stages ports, checks their manifests, installs each provider, and exercises
the resulting Shell with a Wayland client. On main the
[publisher](../tools/publish-gnome51.sh) stages and verifies the repository
snapshot before promoting the catalog.

On the laptop, a shortened idle timeout exercised the actual session.
GNOME Session reported idle, Shell raised its screen shield, and ConsoleKit
reported the user's session locked. Waking the display required a password.
The original 900-second idle timeout was restored. Merely observing a dark
panel cannot establish that the session is locked. Suspend/resume and remote
login require separate tests.
