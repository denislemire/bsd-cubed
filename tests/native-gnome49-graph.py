#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-2-Clause
"""Real CK-backed GNOME49 manager/Shell/settings graph, headless only."""
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import time

if os.getuid() == 0 or not sys.platform.startswith('freebsd'):
    raise SystemExit('ordinary native FreeBSD user required')
if len(sys.argv) != 2:
    raise SystemExit('usage: native-gnome49-graph.py /path/to/native-window')
window = str(Path(sys.argv[1]).resolve(strict=True))
assert os.environ.get('XDG_SESSION_COOKIE'), 'real ConsoleKit session required'
runtime = Path(os.environ['XDG_RUNTIME_DIR'])
assert runtime.stat().st_uid == os.getuid() and runtime.stat().st_mode & 0o777 == 0o700
output = Path('dist/session49')
output.mkdir(parents=True, exist_ok=True)

def call(*args, check=True, env=None):
    return subprocess.run(['/usr/local/bin/gdbus', 'call', '--session', *args],
                          env=env, capture_output=True, timeout=5, check=check)

def eventually(test, process, deadline=50):
    limit = time.monotonic() + deadline
    while not test():
        if process.poll() is not None or time.monotonic() > limit:
            raise RuntimeError('GNOME49 graph exited or timed out before readiness')
        time.sleep(.1)

with tempfile.TemporaryDirectory(prefix='bsd3-graph49-') as directory:
    base = Path(directory)
    services = base / 'services'
    shutil.copytree('/usr/local/share/bsd-cubed/gnome49/services', services)
    shell = services / 'gnome-shell'
    shell.write_text(shell.read_text().replace(
        'command = /usr/local/bin/gnome-shell --wayland',
        'command = /usr/local/bin/gnome-shell --headless --wayland --no-x11 '
        '--virtual-monitor=1280x720 --wayland-display=bsd3-graph49'))
    env = os.environ.copy()
    for key in ('DISPLAY', 'XAUTHORITY', 'WAYLAND_DISPLAY', 'WAYLAND_SOCKET',
                'BSD3_SESSION_SOCKET', 'BSD3_READY_FD'):
        env.pop(key, None)
    env.update(XDG_SESSION_TYPE='wayland', XDG_CURRENT_DESKTOP='GNOME',
               XDG_SESSION_DESKTOP='gnome', GNOME_SHELL_SESSION_MODE='user',
               LIBGL_ALWAYS_SOFTWARE='true', GSETTINGS_BACKEND='memory',
               PULSE_SERVER='unix:/tmp/bsd3-graph49-no-pulse')
    for key, name in (('XDG_CONFIG_HOME', 'config'), ('XDG_CACHE_HOME', 'cache'),
                      ('XDG_DATA_HOME', 'data'), ('XDG_CONFIG_DIRS', 'empty-config')):
        (base / name).mkdir()
        env[key] = str(base / name)
    # An actual ordinary autostart application must receive the compositor's
    # future-service environment and map a real Wayland window.
    autostart = base / 'config' / 'autostart'
    autostart.mkdir()
    application = base / 'window-app'
    marker = base / 'window-result'
    application.write_text('#!/bin/sh\nset -eu\n'
        f'[ "$WAYLAND_DISPLAY" = bsd3-graph49 ]\n'
        f'GDK_BACKEND=wayland "{window}"\n'
        f'echo WINDOW_OK >"{marker}"\n')
    application.chmod(0o700)
    (autostart / 'org.bsd3.GraphWindow.desktop').write_text(
        '[Desktop Entry]\nType=Application\nName=BSD3 actual Wayland app\n'
        f'Exec={application}\nOnlyShowIn=GNOME;\n')
    with (output / 'graph.log').open('w') as log:
        process = subprocess.Popen(['/usr/local/bin/bsd3-user-session',
            str(services), 'gnome-session'], env=env, stdout=log, stderr=subprocess.STDOUT)
        try:
            eventually(lambda: marker.exists(), process)
            assert marker.read_text().strip() == 'WINDOW_OK'
            answer = call('--dest', 'org.gnome.SessionManager', '--object-path',
                '/org/gnome/SessionManager', '--method',
                'org.gnome.SessionManager.IsSessionRunning', env=env)
            assert b'true' in answer.stdout, answer
            version = call('--dest', 'org.gnome.Shell', '--object-path', '/org/gnome/Shell',
                '--method', 'org.freedesktop.DBus.Properties.Get', 'org.gnome.Shell',
                'ShellVersion', env=env)
            assert b'49.10' in version.stdout, version
            (output / 'shell-version.txt').write_bytes(version.stdout)
            call('--dest', 'org.gnome.SessionManager', '--object-path',
                '/org/gnome/SessionManager', '--method',
                'org.gnome.SessionManager.Logout', '1', env=env)
            assert process.wait(timeout=25) == 0
            assert not list(runtime.glob('bsd3-session.*')), 'private manager socket survived logout'
            # All graph-owned session bus services must be gone after logout.
            for name in ('org.gnome.SessionManager', 'org.gnome.Shell',
                         *(f'org.gnome.SettingsDaemon.{name}' for name in (
                             'A11ySettings', 'Color', 'Keyboard', 'MediaKeys',
                             'Power', 'ScreensaverProxy', 'Sound'))):
                answer = call('--dest', 'org.freedesktop.DBus', '--object-path',
                    '/org/freedesktop/DBus', '--method',
                    'org.freedesktop.DBus.NameHasOwner', name, env=env)
                assert b'false' in answer.stdout, (name, answer.stdout)
            (output / 'result.txt').write_text(
                'PASS: actual CK-backed GNOME49 manager, Shell49.10, '
                'SettingsDaemon47.2_1 graph, future display environment, '
                'autostart Wayland window, normal logout and owner cleanup\n'
                'DRM/VT, display-manager re-entry and hardware49 remain NOT VERIFIED\n')
            print((output / 'result.txt').read_text(), end='')
        finally:
            if process.poll() is None:
                process.terminate()
                try:
                    process.wait(timeout=35)
                except subprocess.TimeoutExpired:
                    process.kill()
                    process.wait(timeout=5)
