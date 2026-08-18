# Domoticz (automated) testing

This folder is used for everything around (automated) testing of Domoticz.

## Functional testing

Some functional testing is done by using BDD/Gherkin style tests. See the [README.md](gherkin/README.md) in the _gherkin_ folder for more information.

### Webserver / API sweep

`python/test_api_sweep.py` walks the **entire** registered JSON API surface and
reports anything the HTTP layer refuses to carry. It exists because libwebem
(`extern/libwebem`) sits underneath every endpoint: a change there can break the
API without breaking the build, and the Gherkin suite only covers a handful of
URIs.

```
python test/python/test_api_sweep.py msbuild/x64/Debug/domoticz.exe
```

It starts its own Domoticz on a free port with a throwaway database and user
data folder, so it never touches an existing installation. The endpoint list is
extracted from `main/WebServer.cpp` at run time, so it cannot go stale as
commands are added.

Two things it is careful about, both of which produce false results if ignored:

* A **transport** failure (connection reset, timeout, or 413/414/431/501 to a
  short well-formed GET) is a libwebem regression. An **application** response —
  including a 400 with a JSON body, which is what `logincheck` and
  `setupwizardcreateadmin` correctly return to a bare GET — is not, and is not
  reported as one.
* Domoticz takes a global single-instance mutex, so no other instance may be
  running (or still shutting down) when the sweep starts. The harness waits for
  this; an unexplained `rc=1` with an empty log is what violating it looks like.

It also probes the request-size limits, HTTP keep-alive behaviour, and the
authenticated WebSocket API.

### Device calibration (AddjValue/AddjMulti)

`python/test_calibration.py` proves whether calibration offsets (the device
edit dialog's "Calibration" tab) are actually applied, and applied exactly
once, across the different ways a device's value can be updated. It exists
because calibration is historically re-implemented ad hoc at each ingest
point (the JSON `udevice` API, `hardware/MQTTAutoDiscover.cpp`) instead of
living in one place, so some paths applied it and others silently did not;
the Python plugin ingest path (`hardware/plugins/PythonObjects.cpp`) was one
that did not.

```
python test/python/test_calibration.py msbuild/x64/Debug/domoticz.exe
```

Like the API sweep, it starts its own Domoticz on a free port with a
throwaway database and userdata folder, so it never touches an existing
installation, and it is subject to the same single-instance-mutex caveat
described above.

It exercises the udevice and plugin ingest paths against pTypeTEMP,
pTypeTEMP_HUM, pTypeTEMP_HUM_BARO (both the integer- and float-barometer
sValue formats, covering temperature via AddjValue *and* barometer via
AddjValue2), a bare barometer, and UV:

* The JSON `udevice` API, which already calibrates temperature and the
  paired barometer correctly today. This is the regression guard.
* A small example plugin, `plugins/examples/CalibrationTest/`, copied by the
  test into its own throwaway `userdata/plugins/CalibrationTest/` folder for
  the run (Domoticz scans `<userdata>/plugins/<Name>/plugin.py` once at
  boot, so the real checkout's `plugins/` directory is never touched). A
  "Push Raw Values" switch device lets the test set calibration first, then
  deterministically trigger the plugin to push known raw values, rather
  than waiting on a heartbeat.

* MQTT Auto Discovery (`hardware/MQTTAutoDiscover.cpp`), against a small
  in-process MQTT 3.1.1 broker, `python/mini_mqtt_broker.py`, since there is
  no broker installed on this machine and none bundled in this repository.
  The test starts the broker on a free loopback port, adds an "MQTT Auto
  Discovery Client Gateway" hardware instance pointed at it, waits for
  Domoticz to actually connect (it watches for the `<prefix>/status` =
  `online` message Domoticz publishes once connected and subscribed, rather
  than assuming), then uses a real `paho-mqtt` client to replay the Home
  Assistant discovery handshake for two standalone sensors: a temperature
  sensor (retained config on `<prefix>/sensor/tempnode/temperature/config`,
  `AddjValue`) and a bare atmospheric-pressure sensor in hPa (retained config
  on `<prefix>/sensor/baronode/pressure/config`, `AddjValue2` -- this is the
  case `MQTTAutoDiscover.cpp` never calibrated even before the refactor,
  since it only ever hand-applied calibration for temperature). Each sensor
  gets an initial uncalibrated push so it becomes visible via `getdevices`
  (same empty-`sValue` caveat as the plugin path, see below), calibration is
  set, then a second value is published and read back. If `paho-mqtt` is not
  installed, or Domoticz never connects to the mini broker, or the
  discovered devices never appear, this path is skipped with a clear `SKIP`
  line rather than failing.

  `mini_mqtt_broker.py` is also runnable standalone for debugging
  (`python test/python/mini_mqtt_broker.py [port]`), printing every publish
  it sees. It supports CONNECT/CONNACK, SUBSCRIBE/SUBACK,
  UNSUBSCRIBE/UNSUBACK, PUBLISH both directions, PUBACK, PINGREQ/PINGRESP,
  DISCONNECT, QoS 0 and 1 (QoS 1 is acknowledged with PUBACK but always
  forwarded to subscribers at QoS 0 -- "at most once" on the broker side,
  which is enough for one publisher and one subscriber on loopback), and
  retained messages (stored and replayed to subscribers that subscribe
  later, which the discovery flow depends on). It does not implement
  sessions, Will messages, QoS 2, or TLS, and a malformed or unexpected
  packet is logged and the connection dropped rather than taking the whole
  broker thread down.

Pass `--record baseline.json` to additionally dump every case's raw value,
calibration, expected value, observed value, and PASS/FAIL/SKIP status to a
JSON file, so a run against a pre-fix binary and a run against a post-fix
binary can be diffed directly. The printed table and exit code always
reflect PASS/FAIL/SKIP regardless of `--record`.

Set `KEEP_TESTDATA=1` to leave the throwaway database, log and plugins
folder behind after the run instead of deleting them, which is what you want
when a case fails and you need to inspect the instance's `DeviceStatus`
table or log.

One thing worth knowing when reading a failure: a device whose `sValue` is
still empty is not returned by `getdevices` at all, and a bare barometer has
no value until something updates it. The plugin cases therefore fire one
uncalibrated push purely to make every device visible, then set calibration
and push a second time. Skipping that first push makes the barometer, the
device this test cares most about, permanently invisible.

### Plugin bar ranges (DeviceStatus.Color)

`python/test_plugin_color_ranges.py` proves that a Python plugin can store bar
range definitions in a device's `Color` field, and that doing so cannot reach
any other device's row.

```
python test/python/test_plugin_color_ranges.py msbuild/x64/Debug/domoticz.exe
```

`Color` holds two unrelated things: RGB/WW color state for color switches,
and the bar ranges the utility, temperature and weather cards render
(`www/app/widgets/dzBar.js`). The plugin framework used to push every write
through `_tColor`, which blanks anything without a color mode, so a range
payload never survived. Relaxing that had a catch worth guarding: it made
plugin-supplied strings the first arbitrary text to reach the `UPDATE` built
in `CSQLHelper::UpdateDeviceValue`, whose value was interpolated with `%s`.

Like the tests above it starts its own Domoticz on a free port with a
throwaway database and userdata folder, uses an example plugin
(`plugins/examples/ColorRangeTest/`) copied into that folder for the run, and
is subject to the same single-instance-mutex caveat. An "Apply Color Payloads"
switch triggers the plugin to assign one fixed payload per device.

The five payloads cover both halves:

* A bare range array (utility card shape) and a keyed range object
  (temperature and weather card shape) must reach the database byte for byte.
* A range payload whose color string contains a single quote and SQL
  fragments must be stored verbatim **and** leave every other device alone.
  The test creates a witness device on separate dummy hardware and checks
  its `Name` and `Color` afterwards. Note that JSON validity is no defence
  here: a single quote is legal inside a JSON string, so this case fails
  against a build that passes ranges through without fixing the escaping.
* Genuine RGB color state must still be normalized through `_tColor`, and a
  payload that is not JSON at all must still be rejected. These two are the
  regression guards for color switches.

Against an unfixed binary the first three fail and the last two pass; against
a binary that passes ranges through but does not escape the SQL value, the
witness checks fail too.

## Unit testing

For _dzVents_ quite some unit-tests are available (_code-coverage above 80%_) testing many aspects of 'dzVents' ensuring that functionality does not change or break when changes are made.

A start is made with the 'www' part of Domoticz. The 'www-test' folder contains tests for components in the _www_-folder. As the components in this folder are written in JavaScript, so are the tests. See the [README.md](www-test/README.md) in the _www-test_ folder for details.

## Test automation

For both Unit testing as Functional testing, there is some test automation using `mocha` (javascript), `busted` (Lua) and `pytest-3` (Python and using BDD plugin).
