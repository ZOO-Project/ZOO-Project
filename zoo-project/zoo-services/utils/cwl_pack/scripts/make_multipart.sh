#!/usr/bin/env bash
#
# Written in 2026 by gerald.fenoy at geolabs.fr
#
# To the extent possible under law, the
# author(s) have dedicated all copyright and
# related and neighboring rights to this
# software to the public domain worldwide. This
# software is distributed without any warranty.
#
# You should have received a copy of the CC0 Public Domain
# Dedication along with this software. If not, see
# http://creativecommons.org/publicdomain/zero/1.0/.
#
# Builds a multipart/related body from local CWL files.
#
# Usage (from the root CWL directory, using paths as written in run:):
#   ./make-multipart.sh main.cwl composant1.cwl composant2.cwl
#
# Produces:
#   deploy-multipart.txt    the request body (CRLF)
#   deploy-multipart.ctype  the Content-Type header value
#
# Variables : BOUNDARY (défaut cwlbundle), CID_DOMAIN (défaut zoo.example.org), OUT
set -euo pipefail

[ $# -ge 1 ] || { echo "usage: $0 main.cwl [composant.cwl ...]" >&2; exit 1; }

B=${BOUNDARY:-cwlbundle}
DOMAIN=${CID_DOMAIN:-zoo.example.org}
OUT=${OUT:-deploy-multipart.txt}

media_type() { case "$1" in *.json) echo application/cwl+json ;; *) echo application/cwl+yaml ;; esac; }
cid()        { echo "${1%.*}@${DOMAIN}"; }

# Checks that each local run:/$import/$include reference points to a supplied file.
for f in "$@"; do
  [ -f "$f" ] || { echo "ERROR: $f not found" >&2; exit 1; }
  dir=$(dirname "$f")
  grep -oE '^[[:space:]-]*(run|\$import|\$include):[[:space:]]*[^[:space:]#]+' "$f" \
    | sed -E 's/^[^:]*:[[:space:]]*//; s/^["'\'']//; s/["'\'']$//' | grep -v '://' \
    | while read -r ref; do
        target=$( [ "$dir" = . ] && echo "$ref" || echo "$dir/$ref" )
        printf '%s\n' "$@" | grep -qxF "$target" \
          || echo "WARNING: $f references $ref, which is not in the list" >&2
      done || true
done

{
  for f in "$@"; do
    printf -- '--%s\r\n' "$B"
    printf 'Content-Type: %s\r\n' "$(media_type "$f")"
    printf 'Content-ID: <%s>\r\n' "$(cid "$f")"
    printf 'Content-Location: %s\r\n' "$f"
    printf '\r\n'
    cat "$f"
    printf '\r\n'
  done
  printf -- '--%s--\r\n' "$B"
} > "$OUT"

CTYPE="multipart/related; boundary=${B}; type=\"$(media_type "$1")\"; start=\"<$(cid "$1")>\""
printf '%s\n' "$CTYPE" > "${OUT%.txt}.ctype"

echo "Body: $OUT ($# parts, $(wc -c < "$OUT") bytes)" >&2
echo "Content-Type: $CTYPE" >&2
