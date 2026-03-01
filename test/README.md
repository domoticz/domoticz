# Domoticz Test Suite

This directory contains automated tests for the Domoticz home automation system.

## Test Structure

```
test/
├── gherkin/                    # BDD functional tests (Python/pytest)
│   ├── conftest.py             # Shared fixtures and step definitions
│   ├── *.feature               # Gherkin feature files
│   ├── test_*.py               # Test implementations
│   └── resources/              # Test data (static web content)
├── www/                        # Frontend unit tests (Karma/Jasmine)
│   ├── karma.conf.js           # Karma configuration
│   ├── test-main.js            # RequireJS bootstrap for tests
│   └── unit/                   # Unit test specs
├── lighttpd.conf               # Lighttpd reverse-proxy config for testing
├── lighttpd_proxy.conf         # Lighttpd proxy-only config
├── lighttpd_gzippedjs.lua      # Content-Type handler for .js.gz files
├── lighttpd_gzippedjson.lua    # Content-Type handler for .json.gz files
└── runtests.sh                 # Script to run all functional tests
```

Additional test suites exist elsewhere in the repository:

- **dzVents unit tests** — `dzVents/runtime/tests/` (Lua, run with [busted](https://olivinelabs.com/busted/))
- **dzVents integration tests** — `dzVents/runtime/integration-tests/` (Lua)
- **C++ unit tests** — built as `domoticztester` via CMake
- **Frontend unit tests** — `test/www/` (Karma/Jasmine, see [www test README](www/README.md))

## Functional Tests (BDD/Gherkin)

Uses Python 3 with [pytest](https://docs.pytest.org/) and [pytest-bdd](https://pytest-bdd.readthedocs.io/) to validate webserver behavior using [Gherkin syntax](https://cucumber.io/docs/gherkin/) (Given/When/Then).

**What's tested:**
- Webserver HTTP responses and content encoding (gzip compression)
- OAuth2 authentication flow
- Session management

### Prerequisites

```bash
sudo apt install python3-pytest python3-pytest-bdd
```

### Running

Domoticz must be running first:

```bash
./domoticz -www 8080 -sslwww 0
```

Create a symlink for test web content:

```bash
ln -s ../test/gherkin/resources/testwebcontent www/test
```

Run the tests:

```bash
pytest-3 -rA --tb=no test/gherkin/
```

Or use the all-in-one script (starts Domoticz, runs tests, stops Domoticz):

```bash
./test/runtests.sh
```

Clean up afterwards:

```bash
rm www/test
```

## Lighttpd Test Configuration

The `lighttpd*.conf` and `lighttpd*.lua` files provide a reverse-proxy setup for testing. This serves the Domoticz UI as static content via lighttpd while proxying only `/json.htm` API calls to the Domoticz backend on port 8080. Useful for testing frontend changes without rebuilding.

## dzVents Unit Tests

Located in `dzVents/runtime/tests/`. See the [dzVents test README](../dzVents/runtime/tests/README.md) for setup and usage.

## C++ Unit Tests

Build and run the C++ test executable:

```bash
cd build
cmake ..
make -j$(nproc)
./domoticztester
```
