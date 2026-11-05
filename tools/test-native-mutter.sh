#!/bin/sh
# SPDX-License-Identifier: BSD-2-Clause
# Exact package installation is confined to the disposable CI VM.
set -eu
export PATH=/usr/local/sbin:/usr/local/bin:/sbin:/bin:/usr/sbin:/usr/bin
[ "$(uname -s)" = FreeBSD ] && [ "${CIRCLECI:-}" = true ] || exit 1
[ "$#" -eq 1 ] && [ -f "$1" ] || exit 1
sudo pkg install -y wayland-utils mesa-dri
sudo pkg add -f "$1"
exec sh tests/native-compositor.sh
