#!/bin/sh
# SPDX-License-Identifier: BSD-2-Clause
# Record kernel capability; never skip or retry the real session/VT checks.
set -eu
PATH=/usr/local/sbin:/usr/local/bin:/sbin:/bin:/usr/sbin:/usr/bin
export PATH
[ "$(uname -s)" = FreeBSD ] && [ "$(id -u)" -eq 0 ] && [ "${CIRCLECI:-}" = true ]
repo=$(CDPATH='' cd -- "$(dirname -- "$0")/.." && pwd)
output=$repo/dist/consolekit/process-environment
mkdir -p "$output"
work=$(mktemp -d /var/tmp/bsd3-env-probe.XXXXXXXX)
trap 'rm -rf "$work"' EXIT
trap 'exit 129' HUP
trap 'exit 130' INT
trap 'exit 143' TERM
cc -std=c11 -Wall -Wextra -Werror "$repo/tests/native-process-environment.c" -o "$work/probe"
chgrp "$(id -gn circleci)" "$work" "$work/probe"
chmod 750 "$work" "$work/probe"
uname -v >"$output/kernel.txt"
# Keep the actor out of the controller checkout, which can be under /root.
cd "$work"
# The root controller owns receipt redirections; only execution drops UID.
run_probe() { sudo -H -u circleci "$work/probe" "$@"; }
if run_probe --expect-fixed >"$output/fixed.log" 2>&1; then
    printf '%s\n' fixed >"$output/classification.txt"
elif run_probe --expect-legacy >"$output/legacy.log" 2>&1; then
    printf '%s\n' affected-pr297512 >"$output/classification.txt"
else
    cat "$output/fixed.log" "$output/legacy.log"
    exit 1
fi
cat "$output/classification.txt" "$output/fixed.log"
if [ -f "$output/legacy.log" ]; then cat "$output/legacy.log"; fi
