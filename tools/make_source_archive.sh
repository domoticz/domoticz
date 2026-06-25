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
# It works on a fresh clone of the requested ref in a temporary folder, so the
# current working tree is never touched and the submodules are always pinned to
# exactly what that ref records (no dependency on the checked-out branch).
#
# Usage:  tools/make_source_archive.sh [ref] [output-dir]
#   ref         git ref/tag/branch to package    (default: HEAD)
#   output-dir  where to write the tarball        (default: current directory)
#
# Only requires git and tar.

set -e

REF="${1:-HEAD}"
OUTDIR="${2:-$(pwd)}"

SRCROOT="$(git rev-parse --show-toplevel)"

# --- Clone the requested ref into a throwaway folder ------------------------
WORKDIR="$(mktemp -d)"
trap 'rm -rf "$WORKDIR"' EXIT
CLONE="$WORKDIR/clone"

# Force LF line endings so the archive is identical regardless of host OS
# (files with an explicit eol=crlf gitattribute, e.g. *.sln, still get CRLF).
EOL="-c core.autocrlf=false -c core.eol=lf"

# Clone from the local repo (fast, offline for the superproject); submodules are
# fetched from their upstream URLs as recorded in this ref's .gitmodules.
git $EOL clone --quiet "$SRCROOT" "$CLONE"
git $EOL -C "$CLONE" checkout --quiet --force "$REF"
# Pin submodules to the commits this ref records (no --remote, no floating).
git $EOL -C "$CLONE" submodule update --quiet --init --recursive

# --- Resolve version --------------------------------------------------------
# Build number = commit count + 2107 (matches CMakeLists.txt Gitversion macro).
COUNT="$(git -C "$CLONE" rev-list HEAD --count)"
BUILDNR=$((COUNT + 2107))

# Major prefix comes from the first line of History.txt ("Version 2026.xxxx").
FIRSTLINE="$(head -n 1 "$CLONE/History.txt")"
VERTOKEN="${FIRSTLINE#Version }"      # "2026.xxxx" / "2026.2 (May 31st 2026)"
MAJOR="${VERTOKEN%%.*}"               # "2026"
NEWVERLINE="Version ${MAJOR}.${BUILDNR}"

echo "Packaging ${REF} as build ${BUILDNR}"

# --- Stage a clean copy (no .git) and stamp the version ---------------------
STAGE="$WORKDIR/domoticz"
mkdir -p "$STAGE"
# Copy the working tree, dropping all .git dirs/files (superproject + submodules).
( cd "$CLONE" && tar --exclude='.git' -cf - . ) | tar -xf - -C "$STAGE"

# Stamp a parseable version into History.txt so the build number is correct
# without a .git directory (CMake's History_GET_REVISION reads line 1).
{ printf '%s\n' "$NEWVERLINE"; tail -n +2 "$STAGE/History.txt"; } > "$STAGE/History.txt.new"
mv "$STAGE/History.txt.new" "$STAGE/History.txt"

# --- Pack -------------------------------------------------------------------
mkdir -p "$OUTDIR"
ARCHIVE="$OUTDIR/domoticz_src_${MAJOR}.${BUILDNR}.tar.gz"

# Reproducible archive when GNU tar is available (CI runs on Linux/GNU tar).
TAR_REPRO=""
if tar --version 2>/dev/null | grep -q "GNU tar"; then
    COMMIT_DATE="$(git -C "$CLONE" show -s --format=%cI HEAD)"
    TAR_REPRO="--sort=name --owner=0 --group=0 --numeric-owner --mtime=$COMMIT_DATE"
fi

# shellcheck disable=SC2086
tar $TAR_REPRO -czf "$ARCHIVE" -C "$WORKDIR" domoticz

# --- Checksum ----------------------------------------------------------------
( cd "$OUTDIR" && sha256sum "$(basename "$ARCHIVE")" > "$(basename "$ARCHIVE").sha256sum" )

echo "Created: $ARCHIVE"
echo "         ${ARCHIVE}.sha256sum"
