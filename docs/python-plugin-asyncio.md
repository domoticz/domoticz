# asyncio in Python plugins requires Python 3.12+

By default each Python plugin runs in its own sub-interpreter (`Py_NewInterpreter()` in `hardware/plugins/Plugins.cpp`). On **Python 3.11 and earlier**, the `_asyncio` C extension is single-phase init and keeps the running event loop in **process-global C statics** (`cached_running_holder` / `cached_running_holder_tsid` in `Modules/_asynciomodule.c`). That cache is shared across every sub-interpreter, so two asyncio-based plugins running concurrently will see each other's event loops via `asyncio.events._get_running_loop()`, corrupting awaits with errors like:

```
RuntimeError: Cannot run the event loop while another loop is running
RuntimeError: got Future <Future pending> attached to a different loop
```

Python 3.12 fixed this by converting `_asyncio` to multi-phase init with per-interpreter module state.

## Recommendations for plugin authors who use `asyncio`

- **Preferred (the only fully working option on multi-asyncio systems):** require **Python 3.12+** and let the plugin run in its own sub-interpreter (no `shared` attribute, or `shared="false"`).
- **`shared="true"` is a partial workaround only:** it runs the plugin in the main interpreter via `PyGILState_Ensure()` (`Plugins.cpp:1326-1340`), where the global asyncio cache is consistent because there's only one interpreter. Caveats:
  - **`Domoticz` vs `DomoticzEx` collision:** `FindModule()` (`Plugins.cpp:903-914`) rejects any interpreter where both the `Domoticz` and `DomoticzEx` modules are registered. With `shared="true"`, all shared plugins live in the same main interpreter, so mixing one plugin that does `import Domoticz` with another that does `import DomoticzEx` will fail with: `Domoticz and DomoticzEx modules both found in interpreter, use one or the other.` On a system with both module styles in use, **shared mode is not a viable fix** and Python 3.12+ is the only path.
  - Plugin-local sub-module name collisions in `sys.modules` (Domoticz scrubs `sys.modules["plugin"]` on each start, but sub-modules can still collide).
  - Different cleanup path (`PyThreadState_Clear` / `Delete`) that doesn't preserve interpreter state across disable/enable.

Domoticz logs a one-time status message on startup when a sub-interpreter plugin is created on Python < 3.12 to highlight this.
