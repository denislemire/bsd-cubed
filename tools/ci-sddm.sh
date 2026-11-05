#!/bin/sh
# SPDX-License-Identifier: BSD-2-Clause
set -eu
[ "${CIRCLECI:-}" = true ] || exit 1
[ "$(uname -s)" = FreeBSD ] && [ "$(id -u)" -eq 0 ] || exit 1
export PATH=/usr/local/sbin:/usr/local/bin:/sbin:/bin:/usr/sbin:/usr/bin
pkg install -y sddm xorg-server xf86-video-vesa
# Execute a protected copy; do not run a script that can change during the trial.
runner=$(mktemp /var/tmp/bsd3-sddm-runner.XXXXXXXX)
trap 'rm -f "$runner"' EXIT
trap 'exit 129' HUP
trap 'exit 130' INT
trap 'exit 143' TERM
install -m 400 tools/test-native-sddm.sh "$runner"
sha256 "$runner"
/bin/sh "$runner" "$(pwd)" circleci
