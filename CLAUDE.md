# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

Domoticz is a multi-platform home automation system (Linux, Windows, macOS, BSD, embedded devices). It consists of a C++ backend daemon and an AngularJS web frontend.

## Architecture

```
┌─────────────────────────────────────────────────┐
│       Web Frontend (AngularJS 1.x)              │
│  www/                                           │
│  - RequireJS (AMD) module loading               │
│  - Lazy-loaded controllers via AngularAMD       │
│  - PWA with service worker                      │
└────────────────┬────────────────────────────────┘
                 │ JSON API (/json.htm) + WebSocket
┌────────────────▼────────────────────────────────┐
│       Backend Daemon (C++17)                    │
│  - Hardware drivers (hardware/*.cpp)            │
│  - Event/automation engine (dzVents, Lua)       │
│  - Python plugin system                         │
│  - SQLite database (domoticz.db)                │
│  - Notification system                          │
└─────────────────────────────────────────────────┘
```

### Backend Structure
- `main/` - Core application (WebServer, SQLHelper, EventSystem, Scheduler)
- `hardware/` - 150+ device drivers (Z-Wave, Zigbee, MQTT, Philips Hue, 1-Wire, etc.)
- `notifications/` - Push notification handlers (FCM, Telegram, Pushover, etc.)
- `webserver/` - HTTP/WebSocket server implementation
- `push/` - Data push services (MQTT, HTTP, InfluxDB)
- `dzVents/` - Lua-based event scripting system
- `plugins/` - Python plugin system

### Frontend Structure (www/)
- `app/` - AngularJS controllers and services
  - `app/dashboardDynamic/` - Dashboard Dynamic module (widgets, grid, layout persistence)
- `views/` - HTML templates
  - `views/dashboardDynamic/` - Dashboard Dynamic HTML templates
- `js/` - External libraries (jQuery, Highcharts, ACE editor, Blockly)
- `i18n/` - Translations (20+ languages, gzipped JSON)

## Build Commands

### Linux/macOS (CMake)
```bash
# Standard build
mkdir build && cd build
cmake ..
make -j$(nproc)

# Common CMake options
cmake -DUSE_PYTHON=YES ..           # Enable Python plugins
cmake -DUSE_BUILTIN_MQTT=YES ..     # Use bundled Mosquitto
cmake -DUSE_PRECOMPILED_HEADER=YES ..  # Faster builds
```

### Docker Build
```bash
cd build/
./build cmake      # Generate Makefiles
./build compile    # Compile
./build run        # Run at http://127.0.0.1:8080
./build clean      # Clean build
./build shell      # Interactive container shell
```

### Windows (Visual Studio + vcpkg)
```powershell
# Install dependencies via vcpkg
vcpkg install "@msbuild/vcpkg-packages.txt" --triplet x86-windows
vcpkg integrate install
```

1. Open `msbuild/domoticz.sln`
2. Build Win32 configuration (Debug or Release)
3. Set Working Directory to `$(ProjectDir)/..` in project properties

**vcpkg packages:** boost, curl, jsoncpp, lua, minizip, mosquitto, openssl, pthreads, sqlite3, zlib

### Running Domoticz
```bash
./domoticz                    # Default: http://localhost:8080
./domoticz -www 81 -verbose 1 # Custom port with debug output
```

## Key Dependencies

**Required:**
- CMake 3.16+
- C++17 compiler
- Boost 1.69+ (thread)
- OpenSSL
- ZLIB
- CURL
- Lua 5.3
- SQLite3

**Optional:**
- Python 3.4+ (for plugins)
- OpenZWave (Z-Wave support)
- libusb (TE923/Voltcraft support)

**Bundled (via git submodules in extern/):**
- jsoncpp, minizip, mosquitto, sqlite, jwt-cpp

### Git Workflow
- **Commit Messages:**: Follow conventional commits format without Co-Authored-By lines or mentions of Claude
- **Pull Requests:**: Do not include "Generated with Claude Code" or similar AI attribution lines in PR descriptions or mentions of Claude

### Agents
When spawning multiple agents to do jobs, limit the amount to 2 agents
Use the coder-agent for any code related tasks
Use the code-reviewer for any review tasks

## Testing

```bash
# Build and run test executable
./domoticztester
```

## API

All frontend-backend communication uses `/json.htm` endpoint with query parameters:
- `type` - Command type (command, devices, hardware, etc.)
- Additional parameters vary by command

Notable API commands:
- `param=getconfig` — returns global config including `EnableTabDashboardDynamic` (bit 7 of `TabsEnabled`)
- `param=savedashboardlayout` / `param=getdashboardlayout` — per-user Dashboard Dynamic layout persistence
- `param=fetchurl&url=<encoded>` — server-side URL fetch proxy (used by Calendar/RSS widgets to bypass CORS)

WebSocket support via `/livesocket.js` for real-time device updates.

## Contributing

- Base all changes against the `development` branch
- Discuss new features on the forum first: https://forum.domoticz.com/
- Hardware plugin development: http://wiki.domoticz.com/Developing_a_hardware_plugin
- Default credentials: admin/domoticz (change in production)

## Resources

- Website: http://www.domoticz.com
- Forum: https://forum.domoticz.com/
- Wiki: https://wiki.domoticz.com/
- Build from source: https://wiki.domoticz.com/Build_Domoticz_from_source
