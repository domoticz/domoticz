#!/bin/sh
#
# Create a self-contained Domoticz source tarball for the GitHub Releases page.
#
# GitHub's auto-generated "Source code" archives do NOT contain git submodule
# contents (extern/jsoncpp, extern/minizip, extern/jwtcpp, ...), so they cannot
# be built. This script produces an archive that bundles every submodule at the
# commit pinned by the given ref, strips all .git metadata, and stamps the real
# build version into History.txt so the version is correct even without a .git
# directory (CMake falls back to History_GET_REVISION when .git is absent).
#
# Usage:  tools/make_source_archive.sh [ref] [output-dir]
#   ref         git ref/tag to package          (default: HEAD)
#   output-dir  where to write the tarball       (default: current directory)
#
# Side effect: submodules in the working tree are reset to their pinned commits
# (git submodule update --init --force, WITHOUT --remote).
#
# Only requires git and tar.

set -e

REF="${1:-HEAD}"
OUTDIR="${2:-$(pwd)}"

SRCROOT="$(git rev-parse --show-toplevel)"
cd "$SRCROOT"

# --- Resolve version (while .git is still available) ------------------------
# Build number = commit count + 2107 (matches CMakeLists.txt Gitversion macro).
COUNT="$(git rev-list "$REF" --count)"
BUILDNR=$((COUNT + 2107))

# Major/minor prefix comes from the first line of History.txt ("Version 2026.xxxx").
FIRSTLINE="$(head -n 1 History.txt)"
VERTOKEN="${FIRSTLINE#Version }"      # "2026.xxxx"
MAJORMINOR="${VERTOKEN%%.*}"          # "2026"
NEWVERLINE="Version ${MAJORMINOR}.${BUILDNR}"

echo "Packaging ${REF} as ${MAJORMINOR}.${BUILDNR}"

# --- Stage a clean export (no .git) -----------------------------------------
WORKDIR="$(mktemp -d)"
trap 'rm -rf "$WORKDIR"' EXIT
STAGE="$WORKDIR/domoticz"
mkdir -p "$STAGE"

# Superproject: git archive emits the committed tree only (never .git).
git archive "$REF" | tar -x -C "$STAGE"

# Submodules: force them to the pinned commit, then export each into the stage.
git submodule update --init --force
git submodule foreach --quiet --recursive \
    "git archive HEAD | tar -x -C \"$STAGE/\$displaypath\""

# --- Stamp the real version into the bundled History.txt ---------------------
{ printf '%s\n' "$NEWVERLINE"; tail -n +2 "$STAGE/History.txt"; } > "$STAGE/History.txt.new"
mv "$STAGE/History.txt.new" "$STAGE/History.txt"

# --- Pack -------------------------------------------------------------------
mkdir -p "$OUTDIR"
ARCHIVE="$OUTDIR/domoticz_src_${MAJORMINOR}.${BUILDNR}.tar.gz"

# Reproducible archive when GNU tar is available (CI runs on Linux/GNU tar).
TAR_REPRO=""
if tar --version 2>/dev/null | grep -q "GNU tar"; then
    COMMIT_DATE="$(git show -s --format=%cI "$REF")"
    TAR_REPRO="--sort=name --owner=0 --group=0 --numeric-owner --mtime=$COMMIT_DATE"
fi

# shellcheck disable=SC2086
tar $TAR_REPRO -czf "$ARCHIVE" -C "$WORKDIR" domoticz

# --- Checksum ----------------------------------------------------------------
( cd "$OUTDIR" && sha256sum "$(basename "$ARCHIVE")" > "$(basename "$ARCHIVE").sha256sum" )

echo "Created: $ARCHIVE"
echo "         ${ARCHIVE}.sha256sum"
