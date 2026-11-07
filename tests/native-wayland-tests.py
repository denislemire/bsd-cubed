#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-2-Clause
# Copyright (c) 2026 Denis Lemire
"""Bound an ordinary-user command inside a private software Wayland compositor."""
import argparse
import os
from pathlib import Path
import signal
import subprocess
import sys
import tempfile
import time


def stop(process):
    if process.poll() is None:
        os.killpg(process.pid, signal.SIGTERM)
        try:
            process.wait(timeout=5)
        except subprocess.TimeoutExpired:
            os.killpg(process.pid, signal.SIGKILL)
            process.wait(timeout=5)


def command(argv, env, cwd, logfile, timeout, compositor=None):
    with Path(logfile).open('wb') as log:
        process = subprocess.Popen(argv, env=env, cwd=cwd, stdout=log,
                                   stderr=subprocess.STDOUT, start_new_session=True)
        try:
            deadline = time.monotonic() + timeout
            while process.poll() is None:
                if time.monotonic() >= deadline:
                    raise TimeoutError('test command exceeded its deadline')
                if compositor is not None and compositor.poll() is not None:
                    raise RuntimeError('compositor exited during test command')
                time.sleep(.05)
            if process.returncode:
                raise subprocess.CalledProcessError(process.returncode, argv)
        finally:
            stop(process)


def self_test():
    # Exercise real bounded child cleanup without pretending to be FreeBSD or
    # starting a compositor, bus, privileged service or native build.
    with tempfile.TemporaryDirectory() as directory:
        log = Path(directory) / 'test.log'
        command([sys.executable, '-c', 'print("child completed")'], os.environ.copy(), directory, log, 5)
        assert 'child completed' in log.read_text()
        try:
            command([sys.executable, '-c', 'raise SystemExit(7)'], os.environ.copy(), directory, log, 5)
        except subprocess.CalledProcessError as error:
            assert error.returncode == 7
        else:
            raise AssertionError('nonzero child accepted')
        try:
            command([sys.executable, '-c', 'import time; time.sleep(60)'], os.environ.copy(), directory, log, .1)
        except TimeoutError:
            pass
        else:
            raise AssertionError('unbounded child accepted')
    print('PASS: command completion, failure propagation and bounded cleanup; no native compositor test')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path)
    parser.add_argument('--cwd', type=Path)
    parser.add_argument('--timeout', type=int, default=600)
    parser.add_argument('--self-test', action='store_true')
    parser.add_argument('command', nargs=argparse.REMAINDER)
    args = parser.parse_args()
    if args.self_test:
        self_test()
        return
    if os.geteuid() == 0 or not sys.platform.startswith('freebsd'):
        parser.error('ordinary native FreeBSD user required')
    if not args.output or not args.cwd or not 1 <= args.timeout <= 6600:
        parser.error('explicit output/cwd and bounded timeout required')
    argv = args.command[1:] if args.command[:1] == ['--'] else args.command
    if not argv or not os.environ.get('DBUS_SESSION_BUS_ADDRESS'):
        parser.error('command and enclosing private dbus-run-session required')
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=True)
    cwd = args.cwd.resolve(strict=True)
    def interrupted(_sig, _frame):
        raise KeyboardInterrupt()
    signal.signal(signal.SIGTERM, interrupted)
    with tempfile.TemporaryDirectory(prefix='bsd3-wayland-tests-') as directory:
        runtime = Path(directory)
        runtime.chmod(0o700)
        env = os.environ.copy()
        for key in ('DISPLAY', 'BSD3_SESSION_SOCKET', 'BSD3_READY_FD', 'XDG_SESSION_COOKIE'):
            env.pop(key, None)
        # Some upstream tests use fixed names under GLib's temporary directory.
        env.update(TMPDIR=directory, XDG_RUNTIME_DIR=directory, WAYLAND_DISPLAY='bsd3-tests',
                   GDK_BACKEND='wayland', LIBGL_ALWAYS_SOFTWARE='true',
                   GSETTINGS_BACKEND='memory', MESON_TESTTHREADS='2')
        for key, subdir in [('HOME', 'home'), ('XDG_CONFIG_HOME', 'config'),
                            ('XDG_CACHE_HOME', 'cache'), ('XDG_DATA_HOME', 'data')]:
            (runtime / subdir).mkdir(mode=0o700)
            env[key] = str(runtime / subdir)
        # The private bus predates this runtime. Publish the future display and
        # private directories before either Mutter or GTK can activate GVfs.
        subprocess.run(['/usr/local/bin/dbus-update-activation-environment',
            'HOME','TMPDIR','XDG_RUNTIME_DIR','XDG_CONFIG_HOME','XDG_CACHE_HOME','XDG_DATA_HOME',
            'WAYLAND_DISPLAY','GDK_BACKEND','GSETTINGS_BACKEND'], env=env,
            capture_output=True, timeout=5, check=True)
        with (output / 'compositor.log').open('wb') as log:
            compositor = subprocess.Popen(['/usr/local/bin/mutter', '--headless', '--wayland',
                '--no-x11', '--virtual-monitor=1280x720', '--wayland-display=bsd3-tests'],
                env=env, stdout=log, stderr=subprocess.STDOUT, start_new_session=True)
            try:
                deadline = time.monotonic() + 30
                while not (runtime / 'bsd3-tests').is_socket():
                    if compositor.poll() is not None or time.monotonic() >= deadline:
                        raise RuntimeError('compositor exited/timed out before Wayland readiness')
                    time.sleep(.1)
                info = subprocess.run(['/usr/local/bin/wayland-info'], env=env, capture_output=True,
                                      timeout=15, check=True)
                (output / 'wayland-info.txt').write_bytes(info.stdout)
                for interface in (b'wl_compositor', b'xdg_wm_base', b'wl_output'):
                    if interface not in info.stdout:
                        raise RuntimeError('required Wayland interface absent')
                command(argv, env, cwd, output / 'command.log', args.timeout, compositor)
                if compositor.poll() is not None:
                    raise RuntimeError('compositor exited before requested normal shutdown')
                compositor.terminate()
                if compositor.wait(timeout=10) != 0:
                    raise RuntimeError('compositor did not shut down normally')
                (output / 'result.txt').write_text('PASS: bounded command under isolated software Wayland; DRM/VT/hardware not tested\n')
            finally:
                stop(compositor)


if __name__ == '__main__':
    main()
