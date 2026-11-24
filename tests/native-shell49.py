#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-2-Clause
"""Ordinary-user standalone GNOME Shell49 rendering, not hardware/session acceptance."""
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import time

if os.getuid() == 0 or not sys.platform.startswith("freebsd"):
    raise SystemExit("ordinary native FreeBSD user required")
if len(sys.argv) not in (2, 3):
    raise SystemExit("usage: native-shell49.py /path/to/native-window-client [49.10|51.1]")
client = str(Path(sys.argv[1]).resolve(strict=True))
version_expected = sys.argv[2] if len(sys.argv) == 3 else "49.10"
if version_expected not in {"49.10", "51.1"}:
    raise SystemExit("unsupported Shell test version")
major = version_expected.split(".")[0]
output = Path("dist") / f"shell{major}"
output.mkdir(parents=True, exist_ok=True)
with tempfile.TemporaryDirectory(prefix="bsd3-shell49-") as directory:
    runtime = Path(directory)
    env = os.environ.copy()
    for key in ("DISPLAY", "BSD3_SESSION_SOCKET", "BSD3_READY_FD"):
        env.pop(key, None)
    env.update(XDG_RUNTIME_DIR=directory, WAYLAND_DISPLAY=f"bsd3-shell{major}",
               LIBGL_ALWAYS_SOFTWARE="true", GSETTINGS_BACKEND="memory",
               DBUS_SYSTEM_BUS_ADDRESS=os.environ["DBUS_SESSION_BUS_ADDRESS"],
               PULSE_SERVER="unix:/tmp/bsd3-shell49-no-pulse")
    for key, subdir in (("XDG_CONFIG_HOME", "config"), ("XDG_CACHE_HOME", "cache"),
                        ("XDG_DATA_HOME", "data")):
        (runtime / subdir).mkdir()
        env[key] = str(runtime / subdir)
    with (output / "startup.log").open("w") as log:
        process = subprocess.Popen(
            ["/usr/local/bin/gnome-shell", "--headless", "--wayland", "--no-x11",
             "--virtual-monitor=1280x720", f"--wayland-display=bsd3-shell{major}"],
            env=env, stdout=log, stderr=subprocess.STDOUT)
        try:
            deadline = time.monotonic() + 30
            while not (runtime / env["WAYLAND_DISPLAY"]).is_socket():
                if process.poll() is not None or time.monotonic() >= deadline:
                    raise RuntimeError("compositor failed before Wayland readiness")
                time.sleep(0.1)
            deadline = time.monotonic() + 45
            while True:
                version = subprocess.run([
                    "/usr/local/bin/gdbus", "call", "--session", "--dest", "org.gnome.Shell",
                    "--object-path", "/org/gnome/Shell", "--method",
                    "org.freedesktop.DBus.Properties.Get", "org.gnome.Shell", "ShellVersion"],
                    env=env, capture_output=True, timeout=5)
                (output / "shell-version.txt").write_bytes(version.stdout + version.stderr)
                if version.returncode == 0:
                    if f"'{version_expected}'".encode() not in version.stdout:
                        raise RuntimeError("unexpected actual Shell version")
                    break
                if process.poll() is not None or time.monotonic() >= deadline:
                    raise RuntimeError("Shell failed before D-Bus readiness")
                time.sleep(0.2)
            info = subprocess.run(["/usr/local/bin/wayland-info"], env=env,
                                  capture_output=True, timeout=15, check=True)
            (output / "wayland-info.txt").write_bytes(info.stdout)
            for interface in (b"wl_compositor", b"xdg_wm_base", b"wl_output"):
                if interface not in info.stdout:
                    raise RuntimeError("missing Wayland interface: " + interface.decode())
            window_env = env | {"GDK_BACKEND": "wayland"}
            window = subprocess.run([client], env=window_env, capture_output=True,
                                    timeout=15, check=True)
            (output / "window.txt").write_bytes(window.stdout + window.stderr)
            if process.poll() is not None:
                raise RuntimeError("compositor exited during real window test")
            process.terminate()
            status = process.wait(timeout=10)
            if status != 0:
                raise RuntimeError(f"unexpected compositor shutdown: {status}")
            (output / "result.txt").write_text(
                f"PASS: standalone GNOME Shell {version_expected}, real Wayland client/window, normal exit0\n"
                "DRM, VT ownership, full GNOME session and hardware: NOT VERIFIED\n")
            print((output / "result.txt").read_text(), end="")
        finally:
            if process.poll() is None:
                process.kill()
                process.wait(timeout=5)
