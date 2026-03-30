#!/bin/sh

# Usage: ./build.sh <arch> [beta|release] [force]   - full build + upload (CI)
#        ./build.sh [beta|release]                   - build only, no upload (developer)
# Example: ./build.sh armv7l
#          ./build.sh aarch64 release
#          ./build.sh beta              - developer build, no upload

set -e

BUILD_ONLY=false

# Detect if first argument is a build type instead of an architecture
case "$1" in
    beta|release|"")
        BUILD_ONLY=true
        MACH="$(uname -m)"
        BUILD_TYPE="${1:-beta}"
        FORCE="${2:-}"
        ;;
    *)
        MACH="$1"
        BUILD_TYPE="${2:-beta}"
        FORCE="${3:-}"
        ;;
esac

case "$BUILD_TYPE" in
    beta)
        BRANCH="origin/development"
        UPLOAD_PATH="beta/"
        ;;
    release)
        BRANCH="origin/master"
        UPLOAD_PATH="release/"
        ;;
    *)
        echo "Unknown build type: $BUILD_TYPE (use beta or release)"
        exit 1
        ;;
esac

# Prevent duplicate builds for the same arch/type combination
LOCKFILE="/tmp/build_${MACH}_${BUILD_TYPE}.lock"
if [ -f "$LOCKFILE" ]; then
    LOCK_PID=$(cat "$LOCKFILE" 2>/dev/null)
    if [ -n "$LOCK_PID" ] && kill -0 "$LOCK_PID" 2>/dev/null; then
        echo "[$(date)] : Build for $MACH $BUILD_TYPE is already running with PID $LOCK_PID"
        exit 1
    fi
fi
echo $$ > "$LOCKFILE"
trap 'rm -f "$LOCKFILE"' EXIT

# Change to the directory where this script lives (the repo root)
BUILD_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$BUILD_DIR"

if [ "$BUILD_ONLY" = false ]; then
    if [ -f ~/.domoticz_upload_creds ]; then
        . ~/.domoticz_upload_creds
    else
        echo "Error: credentials file ~/.domoticz_upload_creds not found!"
        exit 1
    fi

    # Make sure we are on latest commit
    echo "Updating to server revision..."
    git fetch --all --recurse-submodules=no

    git reset --hard ${BRANCH}
    git submodule update --init --remote --force

    if [ "$FORCE" != "force" ]; then
        LAST_SUCCESS_FILE="/tmp/build_${MACH}_${BUILD_TYPE}.last_success"
        CURRENT_REV="$(git rev-parse HEAD)"
        if [ -f "$LAST_SUCCESS_FILE" ] && [ "$(cat "$LAST_SUCCESS_FILE" 2>/dev/null)" = "$CURRENT_REV" ]; then
                echo "No Changes..."
                exit 0
        fi
    fi
fi

cmake -DCMAKE_BUILD_TYPE=Release .
if [ $? -ne 0 ]
then
	echo "CMake failed!";
	exit 1
fi

make -j$(nproc)
if [ $? -ne 0 ]
then
	echo "Compilation failed!...";
	exit 1
fi
echo "Success, building ${BUILD_TYPE}...";

if [ "$BUILD_ONLY" = true ]; then
    echo "Done (build only)!...";
    printf '\033[1;32m'; cat appversion.h; printf '\033[0m'
    exit 0
fi

OS=$(uname -s | tr 'A-Z' 'a-z')

archive_file="domoticz_${OS}_${MACH}.tgz"
version_file="version_${OS}_${MACH}.h"
history_file="history_${OS}_${MACH}.txt"

cp -f appversion.h ${version_file}
cp -f History.txt ${history_file}

# Generate the archive
echo "Generating Archive: ${archive_file}..."

rm -f ${archive_file} ${archive_file}.sha256sum

if [ -f bin/domoticz ]; then
    mv bin/domoticz .
elif [ ! -f domoticz ]; then
    echo "Error: domoticz binary not found!"
    exit 1
fi
tar -zcf ${archive_file} domoticz History.txt License.txt domoticz.sh server_cert.pem updatebeta updaterelease www/ scripts/ Config/ plugins/ dzVents/
if [ $? -ne 0 ]
then
        echo "Error creating archive!...";
        exit 1
fi
echo "Creating checksum file...";
hash="$(sha256sum ${archive_file} | sed -e 's/\s.*$//')  update.tgz";
echo $hash > ${archive_file}.sha256sum
if [ ! -f ${archive_file}.sha256sum ];
then
        echo "Error creating archive checksum file!...";
        exit 1
fi

#################################
echo "Uploading to Cloud...";

curl -k -T "{${archive_file},${archive_file}.sha256sum,${version_file},${history_file},History.txt}" -u ${UPLOAD_USER}:${UPLOAD_PASSWORD} sftp://${UPLOAD_SERVER}:${UPLOAD_PORT}/${UPLOAD_PATH}
if [ $? -ne 0 ]
then
        echo "Error uploading to Cloud!...";
        exit 1
fi
#################################

# Mark this commit as successfully built and uploaded
echo "$(git rev-parse HEAD)" > "/tmp/build_${MACH}_${BUILD_TYPE}.last_success"

# Cleaning up
rm -f ${version_file}
rm -f ${history_file}

echo "Done!...";
printf '\033[1;32m'; cat appversion.h; printf '\033[0m'
exit 0;
