#!/bin/sh
# SPDX-License-Identifier: BSD-2-Clause
# Disposable VM SDDM session command, reached through the production wrapper.
set -eu
umask 077
ulimit -c 0
repo_root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
cd "$repo_root"
./test-native-sddm-client
sh tests/native-gnome-session.sh --session
printf '%s\n' 'PASS: real SDDM registration and full GNOME session/window/logout' > "$HOME/.bsd3-sddm-probe/gnome-complete.txt"
