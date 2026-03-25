## Purpose

This repository contains build and release automation scripts for **Domoticz** (home automation system). It orchestrates cross-compilation for two ARM architectures using schroot environments, plus a remote x86_64 build host.

## Files

- `domake.sh` — Top-level entry point that triggers builds across all targets (schroot pi32, schroot pi64, and a remote SSH host at 172.16.0.201). Passes the full path to `build.sh` inside each build directory. Stops on first failure (`set -e`).
- `build.sh` — Single parameterized build script. Lives in the Domoticz repository root and automatically changes to its own directory when invoked. Two modes:
  - **CI mode**: `./build.sh <arch> [beta|release] [force]` — full build + package + upload (used by `domake.sh`).
  - **Developer mode**: `./build.sh [beta|release]` — auto-detects architecture, builds only (no git reset, no packaging, no upload).
- `domoticz_upload_creds.example` — Template for the credentials file; must be copied to `~/.domoticz_upload_creds` on the build server.

## Usage

```bash
# CI / release builds (full build + package + upload)
./domake.sh           # build beta (default) for all targets
./domake.sh release   # build release for all targets
./build.sh armv7l     # build beta for 32-bit ARM only
./build.sh aarch64 release  # build release for 64-bit ARM only
./build.sh x86_64           # build beta for Intel x64 only

# Developer builds (compile only, no upload)
./build.sh            # build beta for current architecture
./build.sh release    # build release for current architecture
```

## Build Flow

1. Skips build if no new commits on the target branch
2. `git fetch --all` then `git reset --hard` to the target branch (`origin/development` for beta, `origin/master` for release)
3. `cmake -DCMAKE_BUILD_TYPE=Release .`
4. `make -j4`
5. Package binary + assets into `domoticz_linux_<arch>.tgz` with SHA256 checksum
6. Upload archive, checksum, version header, and history via SFTP (curl)

## Supported Architectures

- `armv7l` — 32-bit ARM, builds in schroot `pi32`, source in `~/build32/dev-domoticz/`
- `aarch64` — 64-bit ARM, builds in schroot `pi64`, source in `~/build64/dev-domoticz/`
- `x86_64` — Intel 64-bit, builds on remote host `172.16.0.201`, source in `~/build64/dev-domoticz/`

## Notes

- `build.sh` lives in the Domoticz repository root. `domake.sh` calls it using absolute paths (e.g., `~/build32/dev-domoticz/build.sh`). The build directory is no longer hardcoded in `build.sh`; it derives its working directory from the script's own location.
- SFTP credentials are stored in `~/.domoticz_upload_creds` (not in the repo). See `domoticz_upload_creds.example` for the required format.
- Concurrent builds are prevented per arch/type combination using lockfiles (`/tmp/build_<arch>_<type>.lock`). Stale lockfiles from killed processes are detected and ignored automatically.
- Scripts use POSIX-compatible tools (`tr`, `kill -0`) to work across distros (Ubuntu, RHEL, etc.).
