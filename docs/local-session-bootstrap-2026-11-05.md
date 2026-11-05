# Strict local session bootstrap — November 5, 2026

State: native VM tests passed; exact-commit package CI pending. Original C11 and
shell integration code is BSD-2-Clause; no upstream GPL implementation was copied.

The pinned FreeBSD SDDM port registers its X11 path through a wrapper but its
Wayland script directly executes the selected command. Stock VM PAM supplies
pam_xdg, without a ConsoleKit connector. The solution is an opt-in SessionCommand
adapter, preserving the existing desktop entries and upstream profile script.
Installation never enables the adapter or modifies PAM/rc.conf.

bsd3-session refuses root, non-foreground execution and a missing/noncontrolling
FreeBSD VT. Typed VT_GETINDEX is checked against ConsoleKit's reported VT; the
matching kernel source defines it as the console index plus one. A new session
uses the caller's backend-derived UID and explicit Wayland/user/local parameters.
An inherited cookie must resolve to the caller's PID registration. Both paths
validate UID, locality, actual VT, active state and owned0700 runtime directory.
The launcher retains its registration connection and passes the verified cookie
only through the child's environment, never its log or argv. Backend calls are
pinned to the unique owner; loss terminates the session.

Standard input explicitly remains inherited: generic spawn defaults could
otherwise replace SDDM's real controlling VT with /dev/null. A FreeBSD procctl
reaper supervises actual descendants across setsid, without changing the child's
foreground process group or acquiring new privilege. Signals are forwarded,
cleanup escalation is bounded and an owned registration is closed on exit.
A reused registration is never closed. Forced descendant cleanup is a failure,
not an unassisted pass. Normal runtime has no arbitrary session time limit.

The root-only disposable fixture exercised this production binary on a real VT:
active device acquisition, switch/pause/revocation, inactive switch refusal,
fresh-FD resume and release passed, followed by restoration to VT1. A nested
launcher successfully reused the inherited registration through the same complete
cycle without prematurely closing it. An invalid cookie was refused before the
VT client ran, and a non-console SSH invocation did not execute its requested
marker command. A termination fixture detached a descendant into a new session
and ignored TERM: the launcher reaped it after escalation, returned failure,
closed its registration and left the independent fixture able to restore VT1.
The detached PID no longer existed after completion.

A separate real SDDM autologin/PAM test reached an ordinary child on actual VT9,
with its own UID, active seat0, Wayland/user type/class and bridge control.
The same SDDM path launched the full GNOME47 RequiredComponents, mapped GTK
Wayland window and normal session-manager logout, with a private headless Shell
override. Regular password authentication, physical GPU/DRM/input, other display
managers and dynamic seat hotplug are not established by these tests.

The example disables SDDM's session reuse because activation/unlock operations
are outside the supported login1 subset. They are not stubbed. Existing packaged
GNOME Wayland entries remain the user's selection. The wrapper is opt-in within
a coordinated trial BE; the normal fallback remains untouched.
