#!/bin/bash

function zipWithoutMap() {
  cat "$1" | grep -vF '//# sourceMappingURL=' | gzip -9 >"$1.gz"
}

for f; do
  zipWithoutMap "$f" && git add "$f.gz" && git rm --ignore-unmatch -f "$f" "$f.map" && rm -f "$f" "$f.map" || { echo "Unable to proces file '$f'" >&2; exit 1; }
  echo "Processed file '$f'"
done