#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-2-Clause
"""Map actual installed GNOME49 apps on the enclosing private Wayland session."""
import os
from pathlib import Path
import re
import resource
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
extras = {
    'nautilus': ('nautilus', '49.6', 'org.gnome.Nautilus', (), True),
    'gnome-console': ('gnome-console', '49.2', 'org.gnome.Console', (), False),
    'gnome-maps': ('gnome-maps', '49.7', 'org.gnome.Maps', (), True),
    'epiphany': ('epiphany', '49.9.1', 'org.gnome.Epiphany', ('about:blank',), True),
    'gnome-calendar': ('gnome-calendar', '49.1', 'org.gnome.Calendar', (), True),
    'papers': ('papers', '49.8', 'org.gnome.Papers', (), False),
    'evolution': ('evolution', '3.58.3', 'org.gnome.Evolution', (), True),
}
selected = list(filter(None, os.environ.get('BSD3_TEST_GNOME_APPS_EXTRA', '').split(',')))
assert len(selected) == len(set(selected)) and set(selected) <= extras.keys(), selected
applications += tuple(extras[name] for name in selected)
only = os.environ.get('BSD3_TEST_GNOME_APPS_ONLY')
if only:
    assert only in {row[0] for row in applications}, only
    applications = tuple(row for row in applications if row[0] == only)
for binary, expected, app_id, arguments, has_quit in applications:
    version = subprocess.check_output(
        ['/usr/local/sbin/pkg', 'query', '%v', binary], text=True, timeout=5).strip()
    assert version.split(',')[0].split('_')[0] == expected, (binary, version)
    executable = 'kgx' if binary == 'gnome-console' else binary
    app_env = env.copy()
    pty_marker = None
    if binary == 'gnome-console':
        pty_marker = (output / 'console-pty.txt').resolve()
        pty_marker.unlink(missing_ok=True)
        app_env['BSD3_CONSOLE_PTY_MARKER'] = str(pty_marker)
        arguments = ('--', '/bin/sh', '-c',
                     'test -t 0 && test -t 1 && tty >"$BSD3_CONSOLE_PTY_MARKER"; '
                     'exec sleep 20')
    if binary in ('epiphany', 'evolution'):
        app_env['WEBKIT_DISABLE_COMPOSITING_MODE'] = '1'
    if binary == 'papers':
        # Original one-page test document, with no remote content or user data.
        document = output / 'papers-fixture.pdf'
        content = b'BT /F1 24 Tf 30 120 Td (BSD3 native document fixture) Tj ET\n'
        objects = [b'<< /Type /Catalog /Pages 2 0 R >>',
                   b'<< /Type /Pages /Kids [3 0 R] /Count 1 >>',
                   b'<< /Type /Page /Parent 2 0 R /MediaBox [0 0 500 200] '
                   b'/Resources << /Font << /F1 4 0 R >> >> /Contents 5 0 R >>',
                   b'<< /Type /Font /Subtype /Type1 /BaseFont /Helvetica >>',
                   b'<< /Length ' + str(len(content)).encode() + b' >>\nstream\n' + content + b'endstream']
        data = bytearray(b'%PDF-1.4\n')
        offsets = [0]
        for index, value in enumerate(objects, 1):
            offsets.append(len(data))
            data.extend(f'{index} 0 obj\n'.encode() + value + b'\nendobj\n')
        xref = len(data)
        data.extend(f'xref\n0 {len(offsets)}\n0000000000 65535 f \n'.encode())
        for offset in offsets[1:]:
            data.extend(f'{offset:010d} 00000 n \n'.encode())
        data.extend(f'trailer\n<< /Size {len(offsets)} /Root 1 0 R >>\nstartxref\n{xref}\n%%EOF\n'.encode())
        document.write_bytes(data)
        thumbnail = output / 'papers-fixture.png'
        subprocess.run(['/usr/local/bin/papers-thumbnailer', '-s', '256',
                        str(document.resolve()), str(thumbnail.resolve())],
                       env=app_env, capture_output=True, check=True, timeout=30)
        assert thumbnail.read_bytes().startswith(b'\x89PNG\r\n\x1a\n'), thumbnail
        print('PASS: actual Papers backend loads the private PDF and renders a PNG thumbnail', flush=True)
        arguments = (str(document.resolve()),)
    log_path = output / (binary + '.log')
    with log_path.open('w') as log:
        # Fail promptly on an app abort; a multi-gigabyte core write otherwise
        # looks like a quit timeout and can exhaust the disposable VM's disk.
        def no_core_dump():
            resource.setrlimit(resource.RLIMIT_CORE, (0, 0))
        process = subprocess.Popen(['/usr/local/bin/' + executable, *arguments],
                                   env=app_env, stdout=log, stderr=subprocess.STDOUT,
                                   preexec_fn=no_core_dump)
        try:
            deadline = time.monotonic() + 25
            while True:
                assert process.poll() is None, (binary, 'exited before mapping', log_path)
                protocol = log_path.read_text(errors='replace')
                owner = subprocess.run(['/usr/local/bin/gdbus', 'call', '--session',
                    '--dest', 'org.freedesktop.DBus', '--object-path', '/org/freedesktop/DBus',
                    '--method', 'org.freedesktop.DBus.NameHasOwner', app_id],
                    env=env, capture_output=True, timeout=5, check=True)
                # Calculator and Papers use G_APPLICATION_NON_UNIQUE.
                # Its live process and actual Wayland traffic are the boundary;
                # do not invent a well-known application bus owner for it.
                owner_ready = b'true' in owner.stdout or binary in ('gnome-calculator', 'papers')
                if (owner_ready and 'xdg_toplevel' in protocol and
                        re.search(r'wl_surface[^\n]*\.attach\(', protocol)):
                    break
                assert time.monotonic() < deadline, (binary, 'no mapped Wayland surface', log_path)
                time.sleep(.1)
            if pty_marker is not None:
                while not pty_marker.exists():
                    assert process.poll() is None and time.monotonic() < deadline, (
                        binary, 'no working child PTY', log_path)
                    time.sleep(.1)
                assert pty_marker.read_text().strip().startswith('/dev/'), pty_marker
                print('PASS: actual Console child has terminal stdin/stdout and reports its PTY', flush=True)
            if binary == 'epiphany':
                print('Web UI uses the hardware profile\'s WebKit compositing fallback; page-content rendering is unqualified', flush=True)
                # A second window exercises the matching-window cleanup guard:
                # closing it must not corrupt the first window's pending queue.
                subprocess.run(['/usr/local/bin/epiphany', '--new-window', 'ephy-about:overview'],
                               env=app_env, capture_output=True, check=True, timeout=10)
                second_deadline = time.monotonic() + 15
                while len(re.findall(r'\.get_toplevel\(', log_path.read_text(errors='replace'))) < 2:
                    assert process.poll() is None and time.monotonic() < second_deadline, (
                        binary, 'second actual window did not map', log_path)
                    time.sleep(.1)
            if has_quit:
                # Nautilus app.quit retains its background service. Its app.kill
                # action calls g_application_quit (the --quit CLI uses it too).
                quit_action = 'kill' if binary == 'nautilus' else 'quit'
                subprocess.run(['/usr/local/bin/gapplication', 'action', app_id, quit_action],
                               env=env, capture_output=True, timeout=5, check=True)
                try:
                    assert process.wait(timeout=10) == 0, (binary, 'quit failed')
                except subprocess.TimeoutExpired:
                    with (output / (binary + '-quit-timeout.txt')).open('w') as diagnostic:
                        for command in (['/usr/bin/procstat', '-kk', str(process.pid)],
                                        ['/usr/bin/procstat', '-t', str(process.pid)]):
                            subprocess.run(command, stdout=diagnostic, stderr=subprocess.STDOUT,
                                           timeout=5, check=False)
                    raise
                print(f'PASS: actual {binary} {version} maps on Wayland and quits normally', flush=True)
            else:
                # Apps without a public quit action use bounded fixture cleanup.
                # Their normal interactive exit is not claimed here.
                print(f'PASS: actual {binary} {version} maps on Wayland; bounded fixture cleanup', flush=True)
        finally:
            if process.poll() is None:
                process.terminate()
                try:
                    process.wait(timeout=10)
                except subprocess.TimeoutExpired:
                    process.kill()
                    process.wait(timeout=5)
