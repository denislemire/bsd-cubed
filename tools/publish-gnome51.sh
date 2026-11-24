#!/bin/sh
# SPDX-License-Identifier: BSD-2-Clause
set -eu
[ "${CIRCLECI:-}" = true ] && [ "${CIRCLE_BRANCH:-}" = main ] || exit 77
case "${CIRCLE_BUILD_NUM:-}" in ''|*[!0-9]*) exit 1 ;; esac
repo=$(git -c safe.directory="$(pwd)" rev-parse --show-toplevel)
cd "$repo"
revision=$(git -c safe.directory="$repo" rev-parse HEAD)
[ "$revision" = "$CIRCLE_SHA1" ]
out=$repo/dist/gnome51
base=FreeBSD:15:amd64/bsd3-gnome51
url=https://packages.ehws.generic.business/$base
dest=s3://ehws-freebsd-packages/$base
endpoint=https://s3-seaweedfs.ehws.generic.business
export AWS_DEFAULT_REGION=${AWS_DEFAULT_REGION:-us-east-1}
version=snapshots/$revision-$CIRCLE_BUILD_NUM
work=$(mktemp -d "$out/catalog.XXXXXX")
trap 'rm -rf "$work"' EXIT
trap 'exit 1' HUP INT TERM
mkdir -p "$work/root/$version/All" "$work/catalog" "$work/repos" "$work/db" "$work/fetch"
for package in "$out"/packages/All/*.pkg; do
    ln "$package" "$work/root/$version/All/"
done
pkg repo -o "$work/catalog" "$work/root"
for file in meta meta.conf packagesite.pkg data.pkg; do test -s "$out/packages/$file"; done
upload() {
    aws --endpoint-url "$endpoint" s3 cp --only-show-errors "$@"
}
# Old catalogues keep resolving their original package bytes across rebuilds.
upload --recursive "$work/root/$version/All" "$dest/$version/All/"
upload --recursive "$out/sources" "$dest/$version/sources/"
upload "$out/SHA256SUMS" "$dest/$version/SHA256SUMS"
upload "$out/provenance.txt" "$dest/$version/provenance.txt"

# Read the staged snapshot through the same HTTPS endpoint used by clients.
for file in meta meta.conf packagesite.pkg data.pkg; do
    upload "$out/packages/$file" "$dest/$version/$file"
done
printf 'bsd3: { url: "%s", enabled: yes }\n' "$url/$version" >"$work/repos/bsd3.conf"
client() {
    pkg -o PKG_DBDIR="$work/db" -R "$work/repos" "$@"
}
client update -fr bsd3
client rquery -U -r bsd3 '%n-%v %R' >"$work/list"
for name in gtk4 webkit2-gtk_60 epiphany gnome-shell evolution; do
    package=$(pkg query -F "$out"/packages/All/"$name"-[0-9]*.pkg '%n-%v')
    grep -Fx "$package bsd3" "$work/list" >/dev/null
done
client fetch -Uy -r bsd3 -o "$work/fetch" epiphany
epiphany=$(basename "$out"/packages/All/epiphany-*.pkg)
cmp "$out/packages/All/$epiphany" "$work/fetch/All/$epiphany"

# A later merge may have landed while this build was compiling.
head=$(git -c safe.directory="$repo" ls-remote --exit-code origin refs/heads/main)
[ "${head%%[[:space:]]*}" = "$revision" ] || { echo "Main advanced; retaining snapshot only."; exit 0; }
for file in meta meta.conf packagesite.pkg data.pkg; do
    upload "$work/catalog/$file" "$dest/$file"
done
printf 'bsd3: { url: "%s", enabled: yes }\n' "$url" >"$work/repos/bsd3.conf"
client update -fr bsd3
client rquery -U -r bsd3 '%n-%v %R' >"$work/live"
sort "$work/list" >"$work/expected"
sort "$work/live" | diff "$work/expected" -
echo "Published $url from $revision (build $CIRCLE_BUILD_NUM)"
