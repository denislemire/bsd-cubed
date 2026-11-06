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
print('PASS: exact child PID readiness, ordinary stop, bus owner loss and failure')
