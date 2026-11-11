#!/bin/sh
# SPDX-License-Identifier: BSD-2-Clause
# Read-only ACPI snapshot for the greeter. No power policy.
set -eu
base=${BSD3_SDDM_BATTERY_FILE:-/tmp/bsd3-sddm-battery}
sysctl=/sbin/sysctl
[ -x "$sysctl" ] || exit 0

svg_escape() {
	printf '%s' "$1" | sed 's/&/\&amp;/g;s/</\&lt;/g;s/>/\&gt;/g'
}

publish() {
	life=$($sysctl -n hw.acpi.battery.life 2>/dev/null) || return 1
	ac=$($sysctl -n hw.acpi.acline 2>/dev/null || printf '%s' '')
	case $life in
	''|*[!0-9]*) return 1 ;;
	esac
	label=$life%
	if [ "$ac" = 1 ]; then
		label="$label · AC"
	fi
	width=$((${#label} * 8 + 8))
	[ "$width" -ge 48 ] || width=48
	umask 022
	printf '%s %s\n' "$life" "$ac" >"$base.tmp"
	mv "$base.tmp" "$base"
	escaped=$(svg_escape "$label")
	printf '%s\n' "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"$width\" height=\"16\"><text x=\"0\" y=\"13\" fill=\"#9aa1ab\" font-family=\"Noto Sans\" font-size=\"13\">$escaped</text></svg>" >"$base.svg.tmp"
	mv "$base.svg.tmp" "$base.svg"
}

publish || exit 0
while :; do
	sleep 15
	publish || exit 0
done
