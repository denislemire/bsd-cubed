#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-2-Clause
"""Isolated foreground-service readiness and name-owner authentication."""
import os
from pathlib import Path
import select
import signal
import subprocess
import sys
root = Path(__file__).resolve().parent.parent
helper = str(root / 'bsd3-bus-ready')
client = str(root / 'test-bus-ready-client')
name = 'org.bsd3.ReadinessTest'
def start(mode='hold', service=name):
    readfd, writefd = os.pipe()
    proc = subprocess.Popen([helper, service, '--', client, service, mode],
        env=dict(os.environ, BSD3_READY_FD=str(writefd)), pass_fds=(writefd,),
        stdout=subprocess.DEVNULL, stderr=subprocess.PIPE)
    os.close(writefd)
    return proc, readfd
proc, fd = start('exit')
assert proc.wait(timeout=5) != 0
assert os.read(fd, 1) == b''
os.close(fd)
# A preexisting owner must be rejected before another process starts.
owner = subprocess.Popen([client, name, 'hold'])
try:
    subprocess.run(['gdbus', 'wait', '--session', '--timeout', '3', name], check=True)
    proc, fd = start()
    assert proc.wait(timeout=5) != 0
    assert os.read(fd, 1) == b''
    os.close(fd)
finally:
    owner.terminate()
    owner.wait(timeout=5)
if not sys.platform.startswith('freebsd'):
    print('PASS: early exit and preowned-name refusal; peer-PID positive requires FreeBSD')
    raise SystemExit(0)
for mode in ('hold', 'leave'):
    proc, fd = start(mode)
    try:
        assert select.select([fd], [], [], 5)[0], 'readiness timed out'
        assert os.read(fd, 1) == b'\n', proc.stderr.read().decode()
        if mode == 'hold':
            proc.send_signal(signal.SIGTERM)
            assert proc.wait(timeout=5) == 0
        else:
            assert proc.wait(timeout=5) != 0, 'bus ownership loss accepted as normal'
    finally:
        os.close(fd)
        if proc.poll() is None:
            proc.terminate()
            proc.wait(timeout=5)
# An arbitrary connection cannot spoof the manager's pinned signal sender.
proc, fd = start('leave')
try:
    assert select.select([fd], [], [], 5)[0] and os.read(fd, 1) == b'\n'
    subprocess.run(['gdbus', 'emit', '--session', '--object-path',
        '/org/gnome/SessionManager', '--signal',
        'org.gnome.SessionManager.SessionOver'], check=True)
    assert proc.wait(timeout=5) != 0
finally:
    os.close(fd)
    if proc.poll() is None:
        proc.terminate()
        proc.wait(timeout=5)
manager = subprocess.Popen([client, 'org.gnome.SessionManager', 'hold'])
try:
    subprocess.run(['gdbus', 'wait', '--session', '--timeout', '3',
                    'org.gnome.SessionManager'], check=True)
    for mode, expected, authenticated in (('leave', 1, False), ('leave', 0, True), ('leave-error', 1, True)):
        proc, fd = start(mode)
        try:
            assert select.select([fd], [], [], 5)[0]
            assert os.read(fd, 1) == b'\n'
            if authenticated:
                subprocess.run(['gdbus', 'call', '--session', '--dest',
                    'org.gnome.SessionManager', '--object-path', '/org/gnome/SessionManager',
                    '--method', 'org.bsd3.ReadinessTest.End'], check=True, capture_output=True)
            else:
                subprocess.run(['gdbus', 'emit', '--session', '--object-path',
                    '/org/gnome/SessionManager', '--signal',
                    'org.gnome.SessionManager.SessionOver'], check=True)
            assert proc.wait(timeout=5) == expected, proc.stderr.read().decode()
        finally:
            os.close(fd)
            if proc.poll() is None:
                proc.terminate()
                proc.wait(timeout=5)
finally:
    manager.terminate()
    manager.wait(timeout=5)
print('PASS: exact PID readiness, stop, premature exit, authenticated SessionOver, spoof refusal, post-SessionOver failure')
