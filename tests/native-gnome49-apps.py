#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-2-Clause
"""Map actual installed GNOME49 apps on the enclosing private Wayland session."""
import os
from pathlib import Path
import re
import subprocess
import sys
import time

if os.getuid() == 0 or not sys.platform.startswith('freebsd'):
    raise SystemExit('ordinary native FreeBSD user required')
if len(sys.argv) != 2 or not os.environ.get('WAYLAND_DISPLAY'):
    raise SystemExit('usage: native-gnome49-apps.py output-directory (inside Wayland session)')
output = Path(sys.argv[1])
output.mkdir(parents=True, exist_ok=True)
env = os.environ.copy()
env.update(GDK_BACKEND='wayland', WAYLAND_DEBUG='client')
applications = (
    ('gnome-calculator', '49.2', 'org.gnome.Calculator', (), False),
    ('gnome-clocks', '49.0', 'org.gnome.clocks', (), True),
    ('gnome-characters', '49.1', 'org.gnome.Characters', (), True),
    ('gnome-font-viewer', '49.0', 'org.gnome.font-viewer', (), True),
    ('baobab', '49.1', 'org.gnome.baobab', (), True),
    ('gnome-text-editor', '49.2', 'org.gnome.TextEditor', (), True),
    ('gnome-system-monitor', '49.1', 'org.gnome.SystemMonitor', (), True),
    ('tecla', '49.0', 'org.gnome.Tecla', ('us',), False),
)
for binary, expected, app_id, arguments, has_quit in applications:
    version = subprocess.check_output(
        ['/usr/local/sbin/pkg', 'query', '%v', binary], text=True, timeout=5).strip()
    assert version.split(',')[0].split('_')[0] == expected, (binary, version)
    log_path = output / (binary + '.log')
    with log_path.open('w') as log:
        process = subprocess.Popen(['/usr/local/bin/' + binary, *arguments],
                                   env=env, stdout=log, stderr=subprocess.STDOUT)
        try:
            deadline = time.monotonic() + 25
            while True:
                assert process.poll() is None, (binary, 'exited before mapping', log_path)
                protocol = log_path.read_text(errors='replace')
                owner = subprocess.run(['/usr/local/bin/gdbus', 'call', '--session',
                    '--dest', 'org.freedesktop.DBus', '--object-path', '/org/freedesktop/DBus',
                    '--method', 'org.freedesktop.DBus.NameHasOwner', app_id],
                    env=env, capture_output=True, timeout=5, check=True)
                # Calculator intentionally uses G_APPLICATION_NON_UNIQUE.
                # Its live process and actual Wayland traffic are the boundary;
                # do not invent a well-known application bus owner for it.
                owner_ready = b'true' in owner.stdout or binary == 'gnome-calculator'
                if (owner_ready and 'xdg_toplevel' in protocol and
                        re.search(r'wl_surface[^\n]*\.attach\(', protocol)):
                    break
                assert time.monotonic() < deadline, (binary, 'no mapped Wayland surface', log_path)
                time.sleep(.1)
            if has_quit:
                subprocess.run(['/usr/local/bin/gapplication', 'action', app_id, 'quit'],
                               env=env, capture_output=True, timeout=5, check=True)
                assert process.wait(timeout=10) == 0, (binary, 'quit failed')
                print(f'PASS: actual {binary} {version} maps on Wayland and quits normally', flush=True)
            else:
                # Non-unique Calculator and transient Tecla expose no app quit
                # action. Their normal interactive exit is not claimed here.
                print(f'PASS: actual {binary} {version} maps on Wayland; bounded fixture cleanup', flush=True)
        finally:
            if process.poll() is None:
                process.terminate()
                try:
                    process.wait(timeout=10)
                except subprocess.TimeoutExpired:
                    process.kill()
                    process.wait(timeout=5)
