# GitHub Copilot Instructions for Domoticz

## Project Overview

Domoticz is a Home Automation System designed to monitor and configure various devices like lights, switches, sensors, and meters. It's a multi-platform application (Linux/Windows/Embedded) with a scalable HTML5 web frontend.

## Technology Stack

- **Primary Language:** C++ (main application logic)
- **Build System:** CMake (minimum version 3.16.0)
- **Scripting:** Python (plugins and event scripts), Lua (dzVents)
- **Frontend:** HTML5, JavaScript
- **Testing:** pytest-3 (Gherkin/BDD), mocha (JavaScript), busted (Lua)
- **Dependencies:** Boost (>=1.69.0), OpenSSL, libcurl, SQLite, Lua 5.3

## Code Style and Formatting

### C++ Code Style
- Use the clang-format configuration defined in `clang-format.txt`
- **Indentation:** Tabs (8-space width)
- **Column Limit:** 200 characters
- **Braces:** Custom style with braces on new lines after classes, functions, control statements
- **Pointer Alignment:** Right (`int *ptr`)
- **No short functions on single line**
- **Include sorting:** Disabled (preserve existing order)

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
```bash
cmake -DCMAKE_BUILD_TYPE=Release CMakeLists.txt
make
```

### Testing
- **Functional Tests:** `pytest-3 -rA --tb=no test/gherkin/` (Gherkin/BDD style)
- **dzVents Tests:** `busted --coverage` in `dzVents/runtime/tests/`
- **WWW Tests:** JavaScript tests in `test/www-test/`

Always run relevant tests before submitting changes.

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

## Build Options (CMakeLists.txt)

Key configuration options:
- `USE_PYTHON` - Enable Python plugins (default: YES)
- `USE_BUILTIN_JSONCPP` - Use bundled JsonCPP (default: YES)
- `USE_STATIC_BOOST` - Static link Boost libraries (default: YES)
- `INCLUDE_LINUX_I2C` - I2C support (default: YES)
- `WITH_LIBUSB` - USB support (default: YES)

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
