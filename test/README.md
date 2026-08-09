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

It exercises two ingest paths against pTypeTEMP, pTypeTEMP_HUM,
pTypeTEMP_HUM_BARO (both the integer- and float-barometer sValue formats,
covering temperature via AddjValue *and* barometer via AddjValue2), a bare
barometer, and UV:

* The JSON `udevice` API, which already calibrates temperature and the
  paired barometer correctly today. This is the regression guard.
* A small example plugin, `plugins/examples/CalibrationTest/`, copied by the
  test into its own throwaway `userdata/plugins/CalibrationTest/` folder for
  the run (Domoticz scans `<userdata>/plugins/<Name>/plugin.py` once at
  boot, so the real checkout's `plugins/` directory is never touched). A
  "Push Raw Values" switch device lets the test set calibration first, then
  deterministically trigger the plugin to push known raw values, rather
  than waiting on a heartbeat.

A third path, MQTT Auto Discovery, is optional: if no MQTT broker is
reachable on `127.0.0.1:1883` it is skipped with a clear `SKIP` line rather
than failing.

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

## Unit testing

For _dzVents_ quite some unit-tests are available (_code-coverage above 80%_) testing many aspects of 'dzVents' ensuring that functionality does not change or break when changes are made.

A start is made with the 'www' part of Domoticz. The 'www-test' folder contains tests for components in the _www_-folder. As the components in this folder are written in JavaScript, so are the tests. See the [README.md](www-test/README.md) in the _www-test_ folder for details.

## Test automation

For both Unit testing as Functional testing, there is some test automation using `mocha` (javascript), `busted` (Lua) and `pytest-3` (Python and using BDD plugin).
