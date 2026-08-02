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

## Unit testing

For _dzVents_ quite some unit-tests are available (_code-coverage above 80%_) testing many aspects of 'dzVents' ensuring that functionality does not change or break when changes are made.

A start is made with the 'www' part of Domoticz. The 'www-test' folder contains tests for components in the _www_-folder. As the components in this folder are written in JavaScript, so are the tests. See the [README.md](www-test/README.md) in the _www-test_ folder for details.

## Test automation

For both Unit testing as Functional testing, there is some test automation using `mocha` (javascript), `busted` (Lua) and `pytest-3` (Python and using BDD plugin).
