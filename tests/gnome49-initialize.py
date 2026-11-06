#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-2-Clause
"""Call real manager methods during its initialization phase."""
import subprocess

prefix = ["gdbus", "call", "--session", "--dest", "org.gnome.SessionManager",
          "--object-path", "/org/gnome/SessionManager", "--method"]


def call(method, *arguments):
    return subprocess.run([*prefix, f"org.gnome.SessionManager.{method}", *arguments],
                          capture_output=True, text=True, timeout=5)


invalid = call("Setenv", "bad=name", "refused")
assert invalid.returncode != 0, invalid
valid = call("Setenv", "BSD3_TEST_VALUE", "native manager environment")
assert valid.returncode == 0, valid
initialized = call("Initialized")
assert initialized.returncode == 0, initialized
print("GNOME49 initial Setenv: invalid refused, valid propagated")
