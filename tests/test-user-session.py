#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-2-Clause
"""Real Dinit lifecycle and native adapter boundary tests; no host services."""
import argparse
import errno
import os
from pathlib import Path
import signal
import subprocess
import sys
import tempfile
import time

args = argparse.ArgumentParser()
args.add_argument("--dinitctl", required=True)
args.add_argument("--gnome49-client")
options = args.parse_args()
ctl = options.dinitctl
root = Path(__file__).resolve().parent.parent
client = str(root / "test-user-session-client")
launcher = str(root / "bsd3-user-session")


def eventually(predicate, seconds=8):
    end = time.monotonic() + seconds
    while time.monotonic() < end:
        if predicate():
            return
        time.sleep(.025)
    raise AssertionError("deadline waiting for session event")


if os.getuid() == 0:
    assert subprocess.run([launcher, "/tmp", "boot"]).returncode == 77
    print("root refusal passed; run lifecycle suite as an ordinary user")
    raise SystemExit(1)

with tempfile.TemporaryDirectory(prefix="bsd3-test-") as tmp:
    base = Path(tmp)
    os.chmod(base, 0o700)
    services = base / "services"
    services.mkdir()
    events = base / "events"
    env = {key: os.environ[key] for key in
           ("HOME", "DBUS_SESSION_BUS_ADDRESS") if key in os.environ}
    env.update(PATH="/usr/local/bin:/opt/homebrew/bin:/usr/bin:/bin",
               XDG_RUNTIME_DIR=tmp, TEST_EVENTS=str(events))

    def lines():
        return events.read_text().splitlines() if events.exists() else []

    def service(name, mode, extra=""):
        (services / name).write_text(
            f"type = process\ncommand = {client} {mode}\n"
            "restart = no\nstart-timeout = 1\nstop-timeout = 0.3\n" + extra)

    service("dependency", "dependency", "ready-notification = pipevar:BSD3_READY_FD\n")
    service("dependent", "dependent", "depends-on = dependency\n")
    service("environment", "environment")
    service("never", "never-ready", "ready-notification = pipevar:BSD3_READY_FD\n")
    service("stubborn", "stubborn", "ready-notification = pipevar:BSD3_READY_FD\n")
    service("orphan", "orphan", "ready-notification = pipevar:BSD3_READY_FD\n")
    service("crash", "crash", "restart = yes\nrestart-limit-count = 2\nrestart-limit-interval = 10\nrestart-delay = 0.05\n")
    (services / "boot").write_text("type = internal\ndepends-on = dependent\n")
    for failed_root in ("missing-service", "never"):
        failed = subprocess.run([launcher, str(services), failed_root], env=env,
                                timeout=8, capture_output=True, text=True)
        assert failed.returncode != 0, (failed_root, failed)
        assert not list(base.glob("bsd3-session.*"))
    with (base / "log").open("w") as log:
        proc = subprocess.Popen([launcher, str(services), "boot"], env=env,
                                stdout=log, stderr=log)
        try:
            eventually(lambda: "dependent-started" in lines())
            assert lines().index("dependency-ready") < lines().index("dependent-started")
            private = next(base.glob("bsd3-session.*"))
            channel = str(private / "control")
            env["BSD3_SESSION_SOCKET"] = channel
            assert private.stat().st_mode & 0o777 == 0o700

            def control(*words, check=True):
                return subprocess.run([ctl, "--user", "--socket-path", channel,
                                       *words], env=env, timeout=7, capture_output=True,
                                      text=True, check=check)

            def api(*words, expected=0):
                answer = subprocess.run([client, *words], env=env, timeout=7,
                                        capture_output=True, text=True)
                assert answer.stdout.strip() == str(expected), answer

            api("ready", expected=-errno.ENOTCONN)
            api("setenv", "bad=name", "x", expected=-errno.EINVAL)
            api("setenv", "DINIT_SOCKET_PATH", "/tmp/other", expected=-errno.EINVAL)
            api("setenv", "NAME", "x" * 1024, expected=-errno.E2BIG)
            value = "literal spaces; $(no shell) 'quotes' = and newline\nsecond line"
            api("setenv", "BSD3_TEST_VALUE", value)
            control("start", "environment")
            eventually(lambda: "second line" in lines())
            assert value in events.read_text()
            api("setenv", "BSD3_TEST_VALUE")
            control("start", "environment")
            eventually(lambda: "unset" in lines())
            if options.gnome49_client:
                subprocess.run([options.gnome49_client, "real GNOME49 environment"],
                               env=env, timeout=15, check=True)
                control("start", "environment")
                eventually(lambda: "real GNOME49 environment" in lines())
            # Existing runtime processes keep their original environment.
            assert proc.poll() is None
            os.chmod(private, 0o755)
            api("setenv", "NAME", "x", expected=-errno.EPERM)
            os.chmod(private, 0o700)
            # A closed ready pipe reports EPIPE without killing its caller.
            readfd, writefd = os.pipe()
            os.close(readfd)
            broken_env = dict(env, BSD3_READY_FD=str(writefd))
            for mode in ("ready", "ready-ignored"):
                answer = subprocess.run([client, mode], env=broken_env,
                                        pass_fds=(writefd,), capture_output=True,
                                        text=True, timeout=3)
                assert answer.stdout.strip() == str(-errno.EPIPE), answer
            os.close(writefd)
            assert control("start", "never", check=False).returncode != 0
            control("start", "crash", check=False)
            eventually(lambda: lines().count("crashed") >= 3)
            time.sleep(.3)
            assert lines().count("crashed") == 3, lines()
            control("start", "stubborn")
            orphan = None
            if sys.platform.startswith("freebsd"):
                control("start", "orphan")
                eventually(lambda: any(line.startswith("orphan-pid=") for line in lines()))
                orphan = int(next(line.split("=")[1] for line in lines()
                                  if line.startswith("orphan-pid=")))
            started = time.monotonic()
            proc.send_signal(signal.SIGTERM)
            assert proc.wait(timeout=5) == 0
            assert time.monotonic() - started < 5
            assert not list(base.glob("bsd3-session.*")), list(base.iterdir())
            if orphan:
                try:
                    os.kill(orphan, 0)
                except ProcessLookupError:
                    pass
                else:
                    raise AssertionError("FreeBSD reaper left escaped descendant alive")
        finally:
            if proc.poll() is None:
                proc.terminate()
                try:
                    proc.wait(timeout=5)
                except subprocess.TimeoutExpired:
                    proc.kill()
                    proc.wait()
            if proc.returncode:
                print((base / "log").read_text())
print("user session: readiness/dependencies, env set/unset, failure, restart limit, bounded stop and cleanup passed")
