# login1 power subset — 2026-11-10

GNOME Settings Daemon 49 talks to `org.freedesktop.login1` for idle suspend.
BSD³ previously owned that name without power methods, so the Shell idle warning
appeared and `Suspend` failed as `UnknownMethod`.

This change forwards selected Manager methods to ConsoleKit2 after proving the
original D-Bus caller has a session:

- `CanSuspend` / `CanHibernate` / `CanHybridSleep` / `CanPowerOff` / `CanReboot`
- `Suspend` / `Hibernate` / `HybridSleep` / `PowerOff` / `Reboot`
- `Inhibit` (`block` or `delay`)
- `PrepareForSleep`

`CanSuspendThenHibernate` returns `na` because ConsoleKit does not implement it.
The gnome-session native backend now asks ConsoleKit for suspend/hibernate
capability and actions. Stop/restart remain refused.

Private-bus protocol tests cover capability strings, action forwarding, delay
inhibitors, `PrepareForSleep`, backend refusal and missing-session denial.
They do not run `acpiconf` and do not establish laptop suspend/resume.

Inhibit leases share the daemon ConsoleKit connection (one login1 inhibitor at
a time). A successful method reply is not hardware acceptance.
