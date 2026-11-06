#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-2-Clause
"""Actual upstream49 manager, headless Kiosk fixture and native Dinit graph.

This establishes manager lifecycle integration only, not Shell49 readiness,
graphics, settings daemons, GDM49, power management or a desktop login.
"""
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import time

manager, schemas = sys.argv[1:]
repo = Path(__file__).resolve().parent.parent

with tempfile.TemporaryDirectory(prefix="bsd3-gnome49-") as tmp:
    base = Path(tmp)
    os.chmod(base, 0o700)
    services = base / "services"
    services.mkdir()
    config = base / "config"
    session = config / "gnome-session" / "sessions"
    session.mkdir(parents=True)
    (session / "bsd3-test.session").write_text("[GNOME Session]\nName=BSD3 diagnostic\nKiosk=true\n")
    (services / "manager").write_text(
        f"type = process\ncommand = {manager} --session=bsd3-test\n"
        "ready-notification = pipevar:BSD3_READY_FD\nrestart = no\n"
        "start-timeout = 10\nstop-timeout = 3\n")
    (services / "initialized").write_text(
        "type = scripted\ndepends-on = manager\n"
        f"command = /usr/local/bin/python3 {repo}/tests/gnome49-initialize.py\n"
        "start-timeout = 5\nstop-timeout = 3\n")
    (services / "environment").write_text(
        f"type = scripted\ndepends-on = manager\ncommand = {repo}/test-user-session-client environment\n"
        "start-timeout = 3\nstop-timeout = 3\n")
    (services / "boot").write_text("type = internal\ndepends-on = initialized\n")
    env = {key: os.environ[key] for key in ("HOME", "DBUS_SESSION_BUS_ADDRESS")}
    env.update(PATH="/usr/local/bin:/usr/bin:/bin", XDG_RUNTIME_DIR=tmp,
               XDG_CONFIG_HOME=str(config), XDG_CONFIG_DIRS=str(config),
               GSETTINGS_BACKEND="memory", GSETTINGS_SCHEMA_DIR=schemas)
    env["TEST_EVENTS"] = str(base / "events")
    with (base / "manager.log").open("w") as log:
        proc = subprocess.Popen([str(repo / "bsd3-user-session"), str(services), "boot"],
                                env=env, stdout=log, stderr=log)
        try:
            def call(method, *args, check=True):
                return subprocess.run(["gdbus", "call", "--session", "--dest",
                                       "org.gnome.SessionManager", "--object-path",
                                       "/org/gnome/SessionManager", "--method",
                                       f"org.gnome.SessionManager.{method}", *args],
                                      env=env, capture_output=True, text=True,
                                      timeout=3, check=check)

            end = time.monotonic() + 15
            while time.monotonic() < end:
                assert proc.poll() is None, (base / "manager.log").read_text()
                running = call("IsSessionRunning", check=False)
                if running.returncode == 0 and "true" in running.stdout:
                    break
                time.sleep(.05)
            else:
                raise AssertionError((base / "manager.log").read_text())
            assert "false" in call("CanShutdown").stdout
            for unsupported in ("Shutdown", "Reboot"):
                denied = call(unsupported, check=False)
                assert denied.returncode != 0 and "NotSupported" in denied.stderr, denied
            channel = str(next(base.glob("bsd3-session.*")) / "control")
            subprocess.run(["/usr/local/sbin/dinitctl", "--user", "--socket-path",
                            channel, "start", "environment"], env=env, timeout=5,
                           check=True, capture_output=True)
            assert (base / "events").read_text().strip() == "native manager environment"
            # Native env update travels through the actual49 D-Bus method.
            # Late Setenv is forbidden once the manager has initialized.
            assert call("Setenv", "BSD3_LATE", "denied", check=False).returncode != 0
            call("Logout", "2")
            assert proc.wait(timeout=10) == 0, (base / "manager.log").read_text()
            assert not list(base.glob("bsd3-session.*"))
            print("GNOME49 manager: readiness, initial env/denials, Initialized, running, power refusal, logout and cleanup passed")
        finally:
            if proc.poll() is None:
                proc.terminate()
                proc.wait(timeout=10)
