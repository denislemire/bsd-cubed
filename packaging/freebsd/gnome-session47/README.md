# GNOME47 session desktop-entry correction

The pinned FreeBSD port post-patch substitution appends a command argument to
both Exec and TryExec. TryExec accepts an executable path/name, not a command
line. GDM consequently rejects every installed Wayland GNOME session and aborts.
The port patch retains the intended acceleration-check option on Exec while
leaving TryExec as an executable. This is a recipe change to the existing
GPL-2.0-or-later GNOME session consumer, not a license change.

This directory deliberately targets47; GNOME49 startup changes are independent.
Native package build and source/recipe release are still required before rollout.
