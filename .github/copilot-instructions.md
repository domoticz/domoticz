# GitHub Copilot Instructions for Domoticz

## Project Overview

Domoticz is a Home Automation System designed to monitor and configure various devices like lights, switches, sensors, and meters. It's a multi-platform application (Linux/Windows/Embedded) with a scalable HTML5 web frontend.

## Technology Stack

- **Primary Language:** C++ (C++17 standard)
- **Build System:** CMake (minimum version 3.16.0, CI uses 3.31.3)
- **Scripting:** Python (plugins and event scripts), Lua (dzVents)
- **Frontend:** HTML5, JavaScript
- **Testing:** pytest-3 (Gherkin/BDD), mocha (JavaScript), busted (Lua)
- **Dependencies:** Boost (>=1.69.0, CI uses 1.86.0), OpenSSL, libcurl, SQLite, Lua 5.3, libusb, mosquitto, zlib, uthash

## Code Style and Formatting

### C++ Code Style
- Use the clang-format configuration defined in `clang-format.txt`
- **C++ Standard:** C++17 (required)
- **Indentation:** Tabs (8-space width)
- **Column Limit:** 200 characters
- **Braces:** Custom style with braces on new lines after classes, functions, control statements
- **Pointer Alignment:** Right (`int *ptr`)
- **No short functions on single line**
- **Include sorting:** Disabled (preserve existing order)

### Platform-Specific Compiler Flags
- **Linux (GCC):** `-Wno-psabi -rdynamic`
- **macOS (Darwin):** Warnings disabled for switch, deprecated declarations, etc.
- **OpenBSD/NetBSD:** `-pthread` required

### Key Formatting Rules
```cpp
// Control statements - braces on new line
if (condition)
{
	// code with tab indentation
}

// Functions - braces on new line
void FunctionName()
{
	// code
}

// Classes - braces on new line
class ClassName
{
	// members
};
```

## Project Structure

- **`main/`** - Core application logic and main components
- **`hardware/`** - Hardware plugin implementations for various devices
- **`webserver/`** - Web server implementation
- **`www/`** - Web frontend files (HTML, JavaScript, CSS)
- **`dzVents/`** - Lua-based automation framework
- **`plugins/`** - Python plugins for device integration
- **`notifications/`** - Notification systems
- **`test/`** - Test files (Gherkin BDD tests, www unit tests)

## Development Workflow

### Branching
- Base all changes against the **`development`** branch
- Bug fixes: create feature branch from `development`
- New features: discuss on forum first, then branch from `development`

### Building

#### Local Development Dependencies (Ubuntu/Debian)
```bash
sudo apt-get install make gcc g++ libssl-dev git libcurl4-gnutls-dev \
  libusb-dev libmosquitto-dev python3-dev zlib1g-dev liblua5.3-dev \
  uthash-dev libsqlite3-dev python3-pytest python3-pytest-bdd
```

#### Build Domoticz
```bash
cmake -DCMAKE_BUILD_TYPE=Release CMakeLists.txt
make
```

The build uses C++17 standard with compiler-specific flags (e.g., `-rdynamic` for GCC).

## Security Considerations

- **Authentication:** Default credentials are admin/domoticz - must be changed
- **Security Issues:** Report to security@domoticz.com (not GitHub issues)
- **Only latest Stable and Beta releases** receive security fixes
- Follow guidelines in `SECURITY.md` and `SECURITY_SETUP.md`
- Never commit secrets or credentials to source code

## Hardware Development

When adding new hardware support:
- Consult https://wiki.domoticz.com/Developing_a_hardware_plugin
- Place implementation in `hardware/` directory
- Follow existing hardware plugin patterns

## Contributing Guidelines

- Use descriptive commit messages
- Discuss new features on forum before implementation
- Ensure changes align with project direction
- Update documentation if relevant
- Test thoroughly before submitting
- **Never include the `extern/` folder in pull requests** - it is defined in git submodules and externally managed

## Running Domoticz Locally

Start the application for development and testing:
```bash
# Basic start (port 8080, no SSL)
./domoticz

# Custom port
./domoticz -www 81

# With debug output
./domoticz -verbose 1

# For testing (disable SSL on web port)
./domoticz -sslwww 0 -wwwroot www
```

Access via browser: `http://localhost:8080/`
Stop with: Ctrl-C in the application terminal

**Note:** Ports below 1024 on Linux require root (e.g., `sudo ./domoticz` for port 80)

## CI/CD Workflows

**Pull Request Checks:**
- PRs are checked against `development` and `master` branches
- Build runs on Ubuntu 24.04
- Must pass build and basic validation
- Build artifacts retained for 7 days
- Automated tests currently disabled in CI (but should be run locally)

**Build Process in CI:**
1. Install dependencies
2. Build Boost 1.86.0 from source (static linking)
3. Build Domoticz with CMake + make

**Paths Ignored by CI:**
- `msbuild/**`
- `.github/**`
- `tools/**`
- `**.md` and `**.txt` files

## Build Options (CMakeLists.txt)

Key configuration options:

**Bundled Libraries:**
- `USE_BUILTIN_JSONCPP` - Use bundled JsonCPP (default: YES)
- `USE_BUILTIN_MINIZIP` - Use bundled Minizip (default: YES)
- `USE_BUILTIN_SQLITE` - Use bundled SQLite (default: NO)
- `USE_BUILTIN_JWTCPP` - Use bundled JWT-CPP (default: YES)

**Optional Features:**
- `USE_PYTHON` - Enable Python plugins (default: YES)
- `INCLUDE_LINUX_I2C` - I2C support (default: YES)
- `INCLUDE_SPI` - SPI support (default: YES)
- `WITH_LIBUSB` - USB support (default: YES)
- `WITH_TELLDUSCORE` - Telldus support (default: NO)
- `DISABLE_UPDATER` - Disable updater functionality (default: NO)

**Linking Options:**
- `USE_STATIC_BOOST` - Static link Boost libraries (default: YES)
- `USE_LUA_STATIC` - Static link Lua (default: YES)
- `USE_OPENSSL_STATIC` - Static link OpenSSL (default: NO)
- `USE_STATIC_OPENZWAVE` - Static link OpenZwave (default: YES, **deprecated**)

**Developer Options:**
- `USE_PRECOMPILED_HEADER` - Speed up build time (default: YES)
- `GIT_SUBMODULE` - Check submodules during build (default: ON)

## Common Dependencies

When suggesting dependencies:
- Prefer existing bundled libraries (JsonCPP, Minizip, JWT-CPP)
- Use Boost libraries already in use (chrono, system, thread)
- Consider multi-platform compatibility (Linux, Windows, embedded)

## Code Comments

- Add comments only when necessary to explain complex logic
- Match existing comment style in the file
- Avoid obvious or redundant comments

## Important Notes

- Default user: `admin`, default password: `domoticz` (should be changed)
- Forum: https://forum.domoticz.com/ for support
- Wiki: https://wiki.domoticz.com/ for documentation
- GitHub issues are for bugs/features, not end-user support
- **Do not modify or include `extern/` folder in PRs** - defined in git submodules and externally managed (contains JsonCPP, Minizip, JWT-CPP, SQLite)
