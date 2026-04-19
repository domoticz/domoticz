# set to minimum version that supports clean build
cmake_minimum_required(VERSION 3.16.0)

project(domoticz)

include(CheckIncludeFile)
include(CheckIncludeFiles)
include(CheckFunctionExists)
include(InstallRequiredSystemLibraries)
include(TestBigEndian)
include(FindPackageHandleStandardArgs)

find_package(Threads REQUIRED)
find_package(PkgConfig)

### DEPENDENCY VERSIONS
set(DOMO_MIN_LIBBOOST_VERSION 1.69.0)

### USER-SETTABLE OPTIONS

# Bundled libraries
option(USE_BUILTIN_JSONCPP  "Use builtin JSonCPP"          YES)
option(USE_BUILTIN_MINIZIP  "Use builtin Minizip"           YES)
option(USE_BUILTIN_SQLITE   "Use builtin sqlite library"    NO)
option(USE_BUILTIN_JWTCPP   "Use builtin JWT-CPP"           YES)

# Optional dependencies
option(USE_PYTHON            "Use Python for Plugins and Event-Scripts" YES)
option(INCLUDE_LINUX_I2C    "Include I2C support"           YES)
option(INCLUDE_SPI          "Include SPI support"           YES)
option(WITH_LIBUSB          "Enable libusb support"         YES)
option(WITH_TELLDUSCORE     "Enable Telldus support"        NO)
option(FORCE_WITH_GPIO      "Force GPIO support even without /sys/class/gpio" NO)

# Link static or shared external dependencies
option(USE_LUA_STATIC        "Link LUA static"                       YES)
option(USE_OPENSSL_STATIC    "Link OpenSSL static"                   NO)
option(USE_STATIC_BOOST      "Build with static BOOST libraries"     YES)
option(USE_STATIC_OPENZWAVE  "Build with static OpenZwave libraries" YES)
option(USE_STATIC_LIBSTDCXX  "Build with static libgcc/libstdc++ libraries" YES)

# Disable updater functionality
option(DISABLE_UPDATER "Disable updater functionality" NO)

# Developer-oriented options
option(USE_PRECOMPILED_HEADER "Use precompiled header feature to speed up build time" YES)
option(GIT_SUBMODULE          "Check submodules during build" YES)
option(ENABLE_ASAN            "Build with address sanitizer"  NO)

### VERSIONING

find_package(Git QUIET)

macro(History_GET_REVISION variable)
  if(EXISTS ${CMAKE_SOURCE_DIR}/History.txt)
    message(STATUS "Read ProjectRevision from History.txt")
    cmake_policy(SET CMP0007 NEW)
    file(STRINGS ${CMAKE_SOURCE_DIR}/History.txt AppVersionContent)
    list(GET AppVersionContent 0 AppVersionContent)
    string(REPLACE " " ";" AppVersionContent ${AppVersionContent})
    list(GET AppVersionContent 1 AppVersionContent)
    string(REPLACE "." ";" AppVersionContent ${AppVersionContent})
    list(GET AppVersionContent 1 ${variable})
  else()
    message(STATUS "Failed to get ProjectRevision from History.txt, set it to 0")
    set(${variable} 0)
  endif()
endmacro()

macro(Gitversion_GET_REVISION dir variable)
  execute_process(
    COMMAND ${GIT_EXECUTABLE} --git-dir ./.git rev-list HEAD --count
    WORKING_DIRECTORY ${dir}
    OUTPUT_VARIABLE ${variable}
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )
endmacro()

Gitversion_GET_REVISION("${CMAKE_SOURCE_DIR}" ProjectRevision)
if(NOT ProjectRevision)
  message(STATUS "Failed to get ProjectRevision from git")
  History_GET_REVISION(ProjectRevision)
else()
  math(EXPR ProjectRevision "${ProjectRevision}+2107")
endif()

### SUBMODULE / BUNDLED SOFTWARE

if(GIT_FOUND AND EXISTS "${PROJECT_SOURCE_DIR}/.git")
  if(GIT_SUBMODULE)
    message(STATUS "Submodule update")
    execute_process(
      COMMAND ${GIT_EXECUTABLE} submodule update --init --recursive
      WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
      RESULT_VARIABLE GIT_SUBMOD_RESULT
    )
    if(NOT GIT_SUBMOD_RESULT EQUAL "0")
      message(FATAL_ERROR "git submodule update --init failed with ${GIT_SUBMOD_RESULT}, please checkout submodules")
    endif()
  endif()
endif()

foreach(_submodule
    "USE_BUILTIN_JSONCPP;extern/jsoncpp/CMakeLists.txt"
    "USE_BUILTIN_MINIZIP;extern/minizip/CMakeLists.txt"
    "USE_BUILTIN_SQLITE;extern/sqlite-amalgamation/CMakeLists.txt"
    "USE_BUILTIN_JWTCPP;extern/jwtcpp/CMakeLists.txt"
)
  list(GET _submodule 0 _opt)
  list(GET _submodule 1 _path)
  if(${_opt} AND NOT EXISTS "${PROJECT_SOURCE_DIR}/${_path}")
    message(FATAL_ERROR "Submodule '${_path}' was not downloaded! GIT_SUBMODULE was turned off or failed. Run: git submodule update --init --recursive")
  endif()
endforeach()

### SOURCE FILES

set(domoticz_SRCS
  # --- hardware (core) ---
  hardware/1Wire.cpp
  hardware/AccuWeather.cpp
  hardware/AirconWithMe.cpp
  hardware/AlfenEve.cpp
  hardware/AnnaThermostat.cpp
  hardware/Arilux.cpp
  hardware/ASyncSerial.cpp
  hardware/ASyncTCP.cpp
  hardware/ASyncWebsocket.cpp
  hardware/AtagOne.cpp
  hardware/BleBox.cpp
  hardware/Buienradar.cpp
  hardware/ColorSwitch.cpp
  hardware/Comm5Serial.cpp
  hardware/Comm5SMTCP.cpp
  hardware/Comm5TCP.cpp
  hardware/CounterHelper.cpp
  hardware/csocket.cpp
  hardware/CurrentCostMeterBase.cpp
  hardware/CurrentCostMeterSerial.cpp
  hardware/CurrentCostMeterTCP.cpp
  hardware/Daikin.cpp
  hardware/DaikinModbus.cpp
  hardware/DarkSky.cpp
  hardware/DavisLoggerSerial.cpp
  hardware/DenkoviDevices.cpp
  hardware/DenkoviTCPDevices.cpp
  hardware/DenkoviUSBDevices.cpp
  hardware/DomoticzHardware.cpp
  hardware/DomoticzInternal.cpp
  hardware/DomoticzTCP.cpp
  hardware/Dummy.cpp
  hardware/Ec3kMeterTCP.cpp
  hardware/EcoCompteur.cpp
  hardware/EcoDevices.cpp
  hardware/eHouseTCP.cpp
  hardware/Enever.cpp
  hardware/EnOceanEEP.cpp
  hardware/EnOceanESP2.cpp
  hardware/EnOceanESP3.cpp
  hardware/EnOceanRawValue.cpp
  hardware/EnphaseAPI.cpp
  hardware/ETH8020.cpp
  hardware/EvohomeBase.cpp
  hardware/EvohomeRadio.cpp
  hardware/EvohomeScript.cpp
  hardware/EvohomeSerial.cpp
  hardware/EvohomeTCP.cpp
  hardware/EvohomeWeb.cpp
  hardware/FritzboxTCP.cpp
  hardware/Gpio.cpp
  hardware/GpioPin.cpp
  hardware/HardwareMonitor.cpp
  hardware/HardwareMonitorLHM.cpp
  hardware/HardwareMonitorUnix.cpp
  hardware/HarmonyHub.cpp
  hardware/HEOS.cpp
  hardware/Honeywell.cpp
  hardware/HttpPoller.cpp
  hardware/I2C.cpp
  hardware/ICYThermostat.cpp
  hardware/InComfort.cpp
  hardware/KMTronic433.cpp
  hardware/KMTronicBase.cpp
  hardware/KMTronicSerial.cpp
  hardware/KMTronicTCP.cpp
  hardware/KMTronicUDP.cpp
  hardware/Kodi.cpp
  hardware/Limitless.cpp
  hardware/LogitechMediaServer.cpp
  hardware/Matter.cpp
  hardware/Meteorologisk.cpp
  hardware/Meteostick.cpp
  hardware/MitsubishiWF.cpp
  hardware/MochadTCP.cpp
  hardware/MQTT.cpp
  hardware/MQTTAutoDiscover.cpp
  hardware/MultiFun.cpp
  hardware/MySensorsBase.cpp
  hardware/MySensorsMQTT.cpp
  hardware/MySensorsSerial.cpp
  hardware/MySensorsTCP.cpp
  hardware/NefitEasy.cpp
  hardware/Nest.cpp
  hardware/NestOAuthAPI.cpp
  hardware/Netatmo.cpp
  hardware/OctoPrintMQTT.cpp
  hardware/OnkyoAVTCP.cpp
  hardware/OpenMeteo.cpp
  hardware/OpenWeatherMap.cpp
  hardware/OpenWebNetTCP.cpp
  hardware/OpenWebNetUSB.cpp
  hardware/OpenZWave.cpp
  hardware/OTGWBase.cpp
  hardware/OTGWSerial.cpp
  hardware/OTGWTCP.cpp
  hardware/P1MeterBase.cpp
  hardware/P1MeterSerial.cpp
  hardware/P1MeterTCP.cpp
  hardware/PanasonicTV.cpp
  hardware/PiFace.cpp
  hardware/Pinger.cpp
  hardware/PVOutput_Input.cpp
  hardware/RAVEn.cpp
  hardware/Rego6XXSerial.cpp
  hardware/RelayNet.cpp
  hardware/RFLinkBase.cpp
  hardware/RFLinkMQTT.cpp
  hardware/RFLinkSerial.cpp
  hardware/RFLinkTCP.cpp
  hardware/RFXBase.cpp
  hardware/RFXComSerial.cpp
  hardware/RFXComTCP.cpp
  hardware/Rtl433.cpp
  hardware/S0MeterBase.cpp
  hardware/S0MeterSerial.cpp
  hardware/S0MeterTCP.cpp
  hardware/SatelIntegra.cpp
  hardware/SBFSpot.cpp
  hardware/SolarEdgeAPI.cpp
  hardware/SolarMaxTCP.cpp
  hardware/Sterbox.cpp
  hardware/SysfsGpio.cpp
  hardware/Tado.cpp
  hardware/TE923.cpp
  hardware/TE923Tool.cpp
  hardware/TeleinfoBase.cpp
  hardware/TeleinfoSerial.cpp
  hardware/TeleinfoTCP.cpp
  hardware/Tellstick.cpp
  hardware/TellstickFactory.cpp
  hardware/ToonThermostat.cpp
  hardware/TTNMQTT.cpp
  hardware/USBtin.cpp
  hardware/USBtin_MultiblocV8.cpp
  hardware/VisualCrossing.cpp
  hardware/VolcraftCO20.cpp
  hardware/Winddelen.cpp
  hardware/WOL.cpp
  hardware/Wunderground.cpp
  hardware/XiaomiDeviceSupport.cpp
  hardware/XiaomiGateway.cpp
  hardware/Yeelight.cpp
  hardware/YouLess.cpp
  hardware/ZiBlueBase.cpp
  hardware/ZiBlueSerial.cpp
  hardware/ZiBlueTCP.cpp
  hardware/ZWaveBase.cpp

  # --- hardware/1Wire ---
  hardware/1Wire/1WireByKernel.cpp
  hardware/1Wire/1WireByOWFS.cpp
  hardware/1Wire/1WireCommon.cpp
  hardware/1Wire/1WireForWindows.cpp

  # --- hardware/cayenne_lpp ---
  hardware/cayenne_lpp/CayenneLPP_Dec.cpp

  # --- hardware/eHouse ---
  hardware/eHouse/EhouseEvents.cpp
  hardware/eHouse/EhouseTcpClient.cpp
  hardware/eHouse/EhouseUdpListener.cpp

  # --- hardware/eVehicles ---
  hardware/eVehicles/MercApi.cpp
  hardware/eVehicles/TeslaApi.cpp
  hardware/eVehicles/TestcarApi.cpp
  hardware/eVehicles/eVehicle.cpp

  # --- hardware/openwebnet ---
  hardware/openwebnet/bt_openwebnet.cpp

  # --- hardware/openzwave ---
  hardware/openzwave/control_panel/ozwcp.cpp
  hardware/openzwave/control_panel/zwavelib.cpp

  # --- hardware/PhilipsHue ---
  hardware/PhilipsHue/PhilipsHue.cpp
  hardware/PhilipsHue/PhilipsHueHelper.cpp
  hardware/PhilipsHue/PhilipsHueSSE.cpp
  hardware/PhilipsHue/PhilipsHueSensors.cpp
  hardware/PhilipsHue/PhilipsHueV2Sensors.cpp

  # --- hardware/plugins ---
  hardware/plugins/DelayedLink.cpp
  hardware/plugins/PluginManager.cpp
  hardware/plugins/PluginProtocols.cpp
  hardware/plugins/PluginTransports.cpp
  hardware/plugins/Plugins.cpp
  hardware/plugins/PythonObjectEx.cpp
  hardware/plugins/PythonObjects.cpp

  # --- hardware/serial ---
  hardware/serial/impl/unix.cpp
  hardware/serial/serial.cpp

  # --- httpclient ---
  httpclient/HTTPClient.cpp
  httpclient/UrlEncode.cpp

  # --- iamserver ---
  iamserver/IamService.cpp

  # --- main ---
  main/Alexa.cpp
  main/BaroForecastCalculator.cpp
  main/Camera.cpp
  main/CmdLine.cpp
  main/domoticz.cpp
  main/DomoticzWebsocketHandler.cpp
  main/dzVents.cpp
  main/EventsPythonDevice.cpp
  main/EventsPythonModule.cpp
  main/EventSystem.cpp
  main/Helper.cpp
  main/HTMLSanitizer.cpp
  main/IFTTT.cpp
  main/json_helper.cpp
  main/KWHStats.cpp
  main/localtime_r.cpp
  main/Logger.cpp
  main/lsignal.cpp
  main/LuaCommon.cpp
  main/LuaHandler.cpp
  main/LuaTable.cpp
  main/mainworker.cpp
  main/mqtt_helper.cpp
  main/NotificationObserver.cpp
  main/NotificationSystem.cpp
  main/RFXNames.cpp
  main/Scheduler.cpp
  main/SignalHandler.cpp
  main/SQLHelper.cpp
  main/stdafx.cpp
  main/StoppableTask.cpp
  main/SunRiseSet.cpp
  main/TrendCalculator.cpp
  main/WebServer.cpp
  main/WebServerCmds.cpp
  main/WebServerCommands.cpp
  main/WebServerHandleGraph.cpp
  main/WebServerHandleGraphCustom.cpp
  main/WebServerHandleGraphDay.cpp
  main/WebServerHandleGraphHour.cpp
  main/WebServerHandleGraphMonthYear.cpp
  main/WebServerHandleGraphUtils.cpp
  main/WebServerHandleGraphWeek.cpp
  main/WebServerHelper.cpp
  main/WindCalculation.cpp

  # --- mcpserver ---
  mcpserver/McpService.cpp
  mcpserver/McpSessionRegistry.cpp
  mcpserver/McpSseSession.cpp

  # --- mdns ---
  mdns/mdns.cpp

  # --- notifications ---
  notifications/NotificationBase.cpp
  notifications/NotificationBrowser.cpp
  notifications/NotificationEmail.cpp
  notifications/NotificationFCM.cpp
  notifications/NotificationHelper.cpp
  notifications/NotificationHTTP.cpp
  notifications/NotificationKodi.cpp
  notifications/NotificationLogitechMediaServer.cpp
  notifications/NotificationProwl.cpp
  notifications/NotificationPushalot.cpp
  notifications/NotificationPushbullet.cpp
  notifications/NotificationPushover.cpp
  notifications/NotificationPushsafer.cpp
  notifications/NotificationSMS.cpp
  notifications/NotificationTelegram.cpp

  # --- push ---
  push/BasePush.cpp
  push/FibaroPush.cpp
  push/GooglePubSubPush.cpp
  push/HttpPush.cpp
  push/InfluxPush.cpp
  push/McpPush.cpp
  push/MQTTPush.cpp
  push/WebsocketPush.cpp

  # --- smtpclient ---
  smtpclient/SMTPClient.cpp

  # --- tcpserver ---
  tcpserver/TCPClient.cpp
  tcpserver/TCPServer.cpp

  # --- tinyxpath ---
  tinyxpath/action_store.cpp
  tinyxpath/htmlutil.cpp
  tinyxpath/lex_util.cpp
  tinyxpath/node_set.cpp
  tinyxpath/tinystr.cpp
  tinyxpath/tinyxml.cpp
  tinyxpath/tinyxmlerror.cpp
  tinyxpath/tinyxmlparser.cpp
  tinyxpath/tokenlist.cpp
  tinyxpath/xml_util.cpp
  tinyxpath/xpath_expression.cpp
  tinyxpath/xpath_processor.cpp
  tinyxpath/xpath_stack.cpp
  tinyxpath/xpath_static.cpp
  tinyxpath/xpath_stream.cpp
  tinyxpath/xpath_syntax.cpp
)

### PRE-EXECUTABLE DEPENDENCY TARGETS
# These must be created before libwebem so it can find them via parent scope.

if(USE_BUILTIN_JSONCPP)
  set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build jsoncpp_lib as a shared library." FORCE)
  set(BUILD_STATIC_LIBS ON CACHE BOOL "Build jsoncpp_lib as a static library." FORCE)
  set(JSONCPP_WITH_TESTS OFF CACHE BOOL "Enable jsoncpp tests" FORCE)
  set(JSONCPP_WITH_POST_BUILD_UNITTEST OFF CACHE BOOL "Enable jsoncpp post build unit tests" FORCE)
  set(JSONCPP_WITH_PKGCONFIG_SUPPORT OFF CACHE BOOL "Enable jsoncpp pkgconfig support" FORCE)
  add_subdirectory(extern/jsoncpp ${CMAKE_CURRENT_BINARY_DIR}/jsoncpp_build EXCLUDE_FROM_ALL)
  if(NOT MSVC)
    target_compile_options(jsoncpp_static PRIVATE -Wno-deprecated-declarations)
  endif()
endif()

if(USE_BUILTIN_MINIZIP)
  add_subdirectory(extern/minizip EXCLUDE_FROM_ALL)
endif()

if(USE_BUILTIN_JWTCPP)
  if(NOT TARGET jwt-cpp::jwt-cpp)
    add_library(jwt-cpp_bundled INTERFACE)
    target_include_directories(jwt-cpp_bundled INTERFACE ${CMAKE_SOURCE_DIR}/extern/jwtcpp/include)
    add_library(jwt-cpp::jwt-cpp ALIAS jwt-cpp_bundled)
  endif()
endif()

add_subdirectory(extern/libwebem)

### EXECUTABLE

add_executable(domoticz ${domoticz_SRCS})
target_link_libraries(domoticz webem)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR})

### COMPILER & LINKER SETTINGS

set_target_properties(domoticz PROPERTIES
  CXX_STANDARD 17
  CXX_STANDARD_REQUIRED ON
  CXX_EXTENSIONS OFF
)

if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
  target_compile_options(domoticz PRIVATE
    -Wno-switch -Wno-parentheses-equality -Wno-deprecated-declarations
    -Wno-tautological-compare -Wno-unused-value -Wno-comment
    -Wno-unsequenced -Wno-logical-op-parentheses -Wno-literal-conversion
  )
  target_compile_definitions(domoticz PRIVATE USE_FILE32API)
elseif(CMAKE_SYSTEM_NAME MATCHES "^(OpenBSD|NetBSD)$")
  target_compile_options(domoticz PRIVATE -pthread)
  target_link_options(domoticz PRIVATE -pthread)
endif()

if(CMAKE_COMPILER_IS_GNUCXX)
  target_compile_options(domoticz PRIVATE -Wno-psabi)
  target_link_options(domoticz PRIVATE -rdynamic)
  target_compile_definitions(domoticz PRIVATE _FILE_OFFSET_BITS=64)
endif()

if(ENABLE_ASAN)
  if(NOT MSVC)
    target_compile_options(domoticz PRIVATE -fsanitize=address)
    target_link_options(domoticz PRIVATE -fsanitize=address)
  else()
    message(WARNING "ASAN is not supported with MSVC compiler")
  endif()
endif()

if(DISABLE_UPDATER)
  target_compile_definitions(domoticz PRIVATE DISABLE_UPDATER)
endif()

if(CMAKE_COMPILER_IS_GNUCXX AND USE_STATIC_LIBSTDCXX)
  message(STATUS "Using static libgcc/libstdc++")
  target_link_options(domoticz PRIVATE -static-libgcc -static-libstdc++)
else()
  message(STATUS "Using dynamic libgcc_s/libstdc++")
endif()

# Bundled library include dirs (must come after add_executable)
target_include_directories(domoticz PRIVATE
  ${CMAKE_SOURCE_DIR}/main
)
if(USE_BUILTIN_JSONCPP)
  target_include_directories(domoticz PRIVATE ${CMAKE_SOURCE_DIR}/extern/jsoncpp/include)
endif()
if(USE_BUILTIN_MINIZIP)
  target_include_directories(domoticz PRIVATE ${CMAKE_SOURCE_DIR}/extern/minizip)
endif()

### DEPENDENCIES

#
# Lua
#
if(USE_LUA_STATIC)
  set(LUA_USE_STATIC_LIBS ON)
endif()
find_library(LUA_LIBRARIES NAMES liblua5.3.a liblua5.3.so lua5.3)
find_path(LUA_INCLUDE_DIRS NAMES lua5.3/lua.h)
if(LUA_LIBRARIES AND LUA_INCLUDE_DIRS)
  message(STATUS "LUA library found at: ${LUA_LIBRARIES}")
  target_include_directories(domoticz PRIVATE ${LUA_INCLUDE_DIRS}/lua5.3)
else()
  find_package(Lua "5.3" REQUIRED)
  message(STATUS "LUA library found at: ${LUA_LIBRARIES}")
  message(STATUS "LUA includes found at: ${LUA_INCLUDE_DIR}")
  target_include_directories(domoticz PRIVATE ${LUA_INCLUDE_DIR})
endif()
target_link_libraries(domoticz ${LUA_LIBRARIES})

#
# Python3
#
if(USE_PYTHON)
  find_package(Python3 3.4 COMPONENTS Development)
  if(Python3_Development_FOUND)
    message(STATUS "Python3 version ${Python3_VERSION} includes found at: ${Python3_INCLUDE_DIRS}")
    target_include_directories(domoticz PRIVATE ${Python3_INCLUDE_DIRS})
    target_compile_definitions(domoticz PRIVATE ENABLE_PYTHON)
    # Python library intentionally NOT linked — the plugin system loads it at runtime via dlopen()
  else()
    message(FATAL_ERROR "Python3 not found on your system. Use USE_PYTHON=OFF or: sudo apt-get install python3-dev")
  endif()
endif()

#
# Platform feature checks
#
check_include_file(execinfo.h HAVE_EXECINFO_H)
if(HAVE_EXECINFO_H)
  if(CMAKE_SYSTEM_NAME MATCHES "^(FreeBSD|NetBSD)$")
    find_library(EXECINFO_LIBRARIES NAMES libexecinfo.so)
    if(EXECINFO_LIBRARIES)
      target_compile_definitions(domoticz PRIVATE HAVE_EXECINFO_H)
      target_link_libraries(domoticz ${EXECINFO_LIBRARIES})
    endif()
  else()
    target_compile_definitions(domoticz PRIVATE HAVE_EXECINFO_H)
  endif()
endif()

if(INCLUDE_LINUX_I2C)
  check_include_files("sys/types.h;linux/i2c-dev.h;linux/i2c.h" HAVE_LINUX_I2C_H)
  if(HAVE_LINUX_I2C_H)
    message(STATUS "Building with I2C support")
    target_compile_definitions(domoticz PRIVATE HAVE_LINUX_I2C)
  else()
    message(WARNING "I2C support disabled: headers not found!")
  endif()
endif()

if(INCLUDE_SPI)
  check_include_files("sys/types.h;linux/spi/spidev.h" HAVE_LINUX_SPI_H)
  if(HAVE_LINUX_SPI_H)
    message(STATUS "Building with SPI support")
    target_compile_definitions(domoticz PRIVATE HAVE_LINUX_SPI)
  else()
    message(WARNING "SPI support disabled: headers not found!")
  endif()
endif()

#
# JSONCPP
#
if(USE_BUILTIN_JSONCPP)
  message(STATUS "Using builtin jsoncpp library")
  target_link_libraries(domoticz jsoncpp_static)
else()
  pkg_check_modules(JSONCPP REQUIRED jsoncpp)
  message(STATUS "JSONCPP includes found at: ${JSONCPP_INCLUDE_DIRS}")
  target_include_directories(domoticz PRIVATE ${JSONCPP_INCLUDE_DIRS})
  target_link_directories(domoticz PRIVATE ${JSONCPP_LIBRARY_DIRS})
  target_link_libraries(domoticz ${JSONCPP_LIBRARIES})
endif()

#
# Mosquitto
#
find_library(MQTT_LIBRARIES NAMES libmosquitto.so mosquitto)
find_path(MQTT_INCLUDE_DIRS NAMES mosquitto.h)
if(MQTT_LIBRARIES AND MQTT_INCLUDE_DIRS)
  message(STATUS "MQTT library found at: ${MQTT_LIBRARIES}")
  message(STATUS "MQTT includes found at: ${MQTT_INCLUDE_DIRS}")
  target_include_directories(domoticz PRIVATE ${MQTT_INCLUDE_DIRS})
  target_link_libraries(domoticz ${MQTT_LIBRARIES})
elseif(PKG_CONFIG_FOUND)
  pkg_check_modules(Mosquitto REQUIRED libmosquitto)
  message(STATUS "Mosquitto ${Mosquitto_VERSION} found via pkg-config")
  target_include_directories(domoticz PRIVATE ${Mosquitto_INCLUDE_DIRS})
  target_link_directories(domoticz PRIVATE ${Mosquitto_LIBRARY_DIRS})
  target_link_libraries(domoticz ${Mosquitto_LIBRARIES})
  set(MQTT_LIBRARIES ${Mosquitto_LIBRARIES})
else()
  message(FATAL_ERROR "Mosquitto library not found. Install with: sudo apt-get install libmosquitto-dev")
endif()

#
# SQLite3
#
if(USE_BUILTIN_SQLITE)
  message(STATUS "Using builtin SQLite library")
  add_subdirectory(extern/sqlite-amalgamation EXCLUDE_FROM_ALL)
  target_link_libraries(domoticz SQLite::SQLite3)
else()
  find_package(SQLite3 REQUIRED)
  message(STATUS "SQLite library found at: ${SQLite3_LIBRARIES}")
  message(STATUS "SQLite includes found at: ${SQLite3_INCLUDE_DIRS}")
  target_link_libraries(domoticz SQLite3::SQLite3)
endif()

#
# Minizip
#
if(USE_BUILTIN_MINIZIP)
  target_link_libraries(domoticz minizip)
else()
  pkg_check_modules(MINIZIP REQUIRED minizip)
  message(STATUS "MINIZIP includes found at: ${MINIZIP_INCLUDE_DIRS}")
  target_include_directories(domoticz PRIVATE ${MINIZIP_INCLUDE_DIRS})
  target_link_directories(domoticz PRIVATE ${MINIZIP_LIBRARY_DIRS})
  target_link_libraries(domoticz ${MINIZIP_LIBRARIES})
endif()

#
# JWT-CPP (target already created above; domoticz also needs includes directly for NotificationFCM.cpp)
#
if(TARGET jwt-cpp::jwt-cpp)
  target_link_libraries(domoticz jwt-cpp::jwt-cpp)
endif()

#
# OpenSSL
#
if(USE_OPENSSL_STATIC)
  set(OPENSSL_USE_STATIC_LIBS TRUE)
endif()
find_package(OpenSSL REQUIRED)
message(STATUS "OpenSSL ${OPENSSL_VERSION} library found at: ${OPENSSL_LIBRARIES}")
target_compile_definitions(domoticz PRIVATE WWW_ENABLE_SSL WITH_TLS)
target_link_libraries(domoticz OpenSSL::SSL OpenSSL::Crypto)

#
# Boost
#
set(Boost_USE_STATIC_LIBS ${USE_STATIC_BOOST})
set(Boost_USE_MULTITHREADED ON)
unset(Boost_INCLUDE_DIR CACHE)
unset(Boost_LIBRARY_DIRS CACHE)
if(USE_STATIC_BOOST)
  message(STATUS "Linking against boost static libraries")
else()
  message(STATUS "Linking against boost dynamic libraries")
endif()
find_package(Boost ${DOMO_MIN_LIBBOOST_VERSION} CONFIG REQUIRED COMPONENTS thread)
message(STATUS "Boost ${Boost_VERSION} includes found at: ${Boost_INCLUDE_DIR}")
target_link_libraries(domoticz Boost::thread)

#
# ZLIB
#
find_package(ZLIB REQUIRED)
message(STATUS "ZLIB ${ZLIB_VERSION_STRING} library found at: ${ZLIB_LIBRARIES}")
target_link_libraries(domoticz ZLIB::ZLIB)

#
# CURL
#
find_package(CURL REQUIRED)
message(STATUS "CURL ${CURL_VERSION_STRING} library found at: ${CURL_LIBRARIES}")
target_link_libraries(domoticz CURL::libcurl)

#
# USB (libusb 0.x API — used by TE923Tool.cpp and VolcraftCO20.cpp)
#
if(WITH_LIBUSB)
  find_path(LIBUSB_INCLUDE_DIR usb.h
    HINTS ${PC_LIBUSB_INCLUDEDIR} ${PC_LIBUSB_INCLUDE_DIRS})
  find_library(LIBUSB_LIBRARY NAMES usb
    HINTS ${PC_LIBUSB_LIBDIR} ${PC_LIBUSB_LIBRARY_DIRS})
  set(LIBUSB_LIBRARIES ${LIBUSB_LIBRARY})
  find_package_handle_standard_args(LIBUSB DEFAULT_MSG LIBUSB_LIBRARIES LIBUSB_INCLUDE_DIR)
  if(LIBUSB_FOUND)
    message(STATUS "LIBUSB library found at: ${LIBUSB_LIBRARIES}")
    target_include_directories(domoticz PRIVATE ${LIBUSB_INCLUDE_DIR})
    target_link_libraries(domoticz ${LIBUSB_LIBRARIES})
    target_compile_definitions(domoticz PRIVATE WITH_LIBUSB)
  else()
    message(STATUS "LibUSB not found, support for TE923/Voltcraft disabled!")
  endif()
endif()

#
# OpenZWave
#
set(OPENZWAVE_ROOT_HINT "" CACHE PATH "Path hint to open-zwave root directory")
set(_ozw_hints "../open-zwave-read-only" "../open-zwave-read-only/cpp/build")
if(DEFINED ENV{OPENZWAVE_ROOT})
  list(PREPEND _ozw_hints "$ENV{OPENZWAVE_ROOT}" "$ENV{OPENZWAVE_ROOT}/cpp/build")
endif()
if(OPENZWAVE_ROOT_HINT)
  list(PREPEND _ozw_hints "${OPENZWAVE_ROOT_HINT}" "${OPENZWAVE_ROOT_HINT}/cpp/build")
endif()

if(USE_STATIC_OPENZWAVE)
  find_library(OpenZWave NAMES libopenzwave.a HINTS ${_ozw_hints})
  if(OpenZWave)
    get_filename_component(_ozw_lib_dir ${OpenZWave} DIRECTORY)
    find_path(OPENZWAVE_INCLUDE_DIRS NAMES OZWException.h
      HINTS "${_ozw_lib_dir}/cpp/src" "${_ozw_lib_dir}/../src")
  endif()
else()
  pkg_check_modules(OPENZWAVE libopenzwave)
  if(OPENZWAVE_FOUND)
    message(STATUS "OpenZWave package found!")
    find_library(OpenZWave NAMES libopenzwave.so HINTS ${OPENZWAVE_LIBRARY_DIRS})
    find_path(OPENZWAVE_INCLUDE_DIRS NAMES OZWException.h HINTS ${OPENZWAVE_INCLUDE_DIRS})
  endif()
endif()

if(OpenZWave)
  message(STATUS "OpenZWave library found at: ${OpenZWave}")
  if(NOT OPENZWAVE_INCLUDE_DIRS)
    message(FATAL_ERROR "OpenZWave includes not found.")
  endif()
  message(STATUS "OpenZWave includes found at: ${OPENZWAVE_INCLUDE_DIRS}")
  target_include_directories(domoticz PRIVATE ${OPENZWAVE_INCLUDE_DIRS})
  target_link_libraries(domoticz ${OpenZWave})
  target_compile_definitions(domoticz PRIVATE WITH_OPENZWAVE)
else()
  message(STATUS "OpenZWave not found, support disabled!")
endif()

#
# GPIO
#
if(EXISTS /sys/class/gpio OR FORCE_WITH_GPIO)
  message(STATUS "GPIO is available")
  target_compile_definitions(domoticz PRIVATE WITH_GPIO)
else()
  message(STATUS "GPIO is not available")
endif()

#
# Telldus
#
if(WITH_TELLDUSCORE)
  find_path(TELLDUSCORE_INCLUDE NAMES telldus-core.h)
  if(NOT TELLDUSCORE_INCLUDE)
    message(FATAL_ERROR "telldus-core.h not found. Install with: sudo apt-get install libtelldus-core-dev")
  endif()
  message(STATUS "Found telldus-core.h at: ${TELLDUSCORE_INCLUDE}")
  find_library(TELLDUS_LIBRARIES NAMES libtelldus-core.so)
  if(TELLDUS_LIBRARIES)
    message(STATUS "Found libtelldus-core at: ${TELLDUS_LIBRARIES}")
    target_include_directories(domoticz PRIVATE ${TELLDUSCORE_INCLUDE})
    target_link_libraries(domoticz ${TELLDUS_LIBRARIES})
    target_compile_definitions(domoticz PRIVATE WITH_TELLDUSCORE)
  endif()
endif()

#
# Resolver
#
check_function_exists(res_init HAVE_LIBC_RESOLV)
if(NOT HAVE_LIBC_RESOLV)
  target_link_libraries(domoticz resolv)
endif()

#
# librt (Linux real-time extensions)
#
find_library(LIBRT rt)
if(LIBRT)
  target_link_libraries(domoticz ${LIBRT})
endif()

#
# Threading and dynamic loading
#
target_link_libraries(domoticz Threads::Threads ${CMAKE_DL_LIBS})

### REVISION TAG & VERSION NUMBERS

# a custom target that is always built
add_custom_target(revisiontag ALL)

# creates appversion.h using cmake script
add_custom_command(TARGET revisiontag PRE_BUILD COMMAND ${CMAKE_COMMAND}
  -DSOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}
  -P ${CMAKE_CURRENT_SOURCE_DIR}/getgit.cmake)

message(STATUS "###########################")
message(STATUS "Compiling Revision #${ProjectRevision}")
message(STATUS "###########################")

# The version number
set(domoticz_VERSION_MAJOR 2025)
set(domoticz_VERSION_MINOR 2)
set(domoticz_VERSION_PATCH ${ProjectRevision})

# explicitly say that the executable depends on the revisiontag
add_dependencies(domoticz revisiontag)

test_big_endian(BIGENDIAN)
if(BIGENDIAN)
  target_compile_definitions(domoticz PRIVATE IS_BIG_ENDIAN)
endif()

### PRECOMPILED HEADERS

if(USE_PRECOMPILED_HEADER)
  message(STATUS "Using precompiled headers")
  target_precompile_headers(domoticz PRIVATE "main/stdafx.h")
endif()

### CPACK / INSTALLATION

set(CPACK_GENERATOR "DEB")
set(CPACK_PACKAGE_NAME "domoticz")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "${domoticz}-Home Automation System (Domotica).")
set(CPACK_PACKAGE_VENDOR "Domoticz.com")
set(CPACK_PACKAGE_CONTACT "info@domoticz.com")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/License.txt")

set(CPACK_PACKAGE_VERSION_MAJOR "${domoticz_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${domoticz_VERSION_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH "${domoticz_VERSION_PATCH}")

set(CPACK_PACKAGE_INSTALL_DIRECTORY "domoticz")
set(CPACK_SOURCE_PACKAGE_FILE_NAME "${domoticz}-dev")
set(CPACK_PACKAGING_INSTALL_PREFIX "/")
set(CPACK_PACKAGE_DEFAULT_LOCATION "/opt/${CPACK_PACKAGE_NAME}")
set(CPACK_PACKAGE_EXECUTABLES "domoticz;Home Automation System (Domotica).")
set(CPACK_DEBIAN_ARCHITECTURE ${CMAKE_SYSTEM_PROCESSOR})

include(CPack)

if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
  set(CMAKE_INSTALL_PREFIX "/opt/${CPACK_PACKAGE_NAME}" CACHE PATH "Install path prefix, prepended onto install directories." FORCE)
endif()

install(TARGETS domoticz DESTINATION ${CMAKE_INSTALL_PREFIX})
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/www DESTINATION ${CMAKE_INSTALL_PREFIX})
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/License.txt DESTINATION ${CMAKE_INSTALL_PREFIX})
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/History.txt DESTINATION ${CMAKE_INSTALL_PREFIX})
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/server_cert.pem DESTINATION ${CMAKE_INSTALL_PREFIX})
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/scripts DESTINATION ${CMAKE_INSTALL_PREFIX} USE_SOURCE_PERMISSIONS)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/plugins DESTINATION ${CMAKE_INSTALL_PREFIX})
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/dzVents DESTINATION ${CMAKE_INSTALL_PREFIX})
if(OpenZWave)
  install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/Config DESTINATION ${CMAKE_INSTALL_PREFIX})
endif()
