#!/bin/bash
# Usage: ./domake.sh [beta|release] [force]
#   ./domake.sh           - build beta (default)
#   ./domake.sh release   - build release
#   ./domake.sh beta force - force build, skip 'No Changes' check

set -e

BUILD_TYPE="${1:-beta}"
FORCE="${2:-}"
schroot -c pi32 -- ~/build32/dev-domoticz/build.sh armv7l "$BUILD_TYPE" "$FORCE"
schroot -c pi64 -- ~/build64/dev-domoticz/build.sh aarch64 "$BUILD_TYPE" "$FORCE"
ssh user@remote ~/build64/dev-domoticz/build.sh x86_64 "$BUILD_TYPE" "$FORCE"
