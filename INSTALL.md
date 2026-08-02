# Domoticz

Usage:
```
domoticz [-www <port>]  [-verbose <0|1>]
          -www <port>   Default is: -www 8080
          -verbose <0|1> (0 is none, 1 is debug)   Default is: -verbose 0
```

Examples:
```
domoticz            (this is the same as domoticz -www 8080 -verbose 0)
domoticz -www 81 -verbose 1
```

If Domoticz and the browser are running on the same system you can connect with http://localhost:8080/
To stop the application: press Ctrl-C in the application screen (not in the browser)

Compatible browsers:
* Edge/Chrome/Firefox/Safari...

Be aware that a Raspberry pi receives its time from an online ntp server.
If the pi is not connected to a network, the device time will not be updated, resulting in scheduling issues. 

All ports below 1024 on linux systems can only be started by root.
If you run Domoticz on port 80, make sure to run it as root, e.g. : sudo ./domoticz

## Building from the source tarball

Domoticz uses git submodules for several bundled dependencies (jsoncpp, minizip,
jwt-cpp, ...). GitHub's automatically generated **"Source code (zip/tar.gz)"**
archives on the Releases page do **not** contain submodule contents, so they
cannot be built directly.

Use one of the following instead:

* The `domoticz_src_<version>.tar.gz` archive attached to each release — it
  bundles all submodules at their pinned commits and builds out of the box:
  ```bash
  tar xzf domoticz_src_2026.2.tar.gz
  cd domoticz
  mkdir build && cd build
  cmake ..
  make -j$(nproc)
  ```

* Or clone the repository with its submodules:
  ```bash
  git clone --recurse-submodules https://github.com/domoticz/domoticz.git
  # for an existing clone:
  git submodule update --init
  ```

Maintainers can regenerate the bundled source archive with
`tools/make_source_archive.sh [ref] [output-dir]`.

