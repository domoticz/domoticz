#set to minimum version that supports clean build
cmake_minimum_required(VERSION 3.16.0)

project(domoticz)

INCLUDE(CheckIncludeFile)
INCLUDE(CheckIncludeFiles)
INCLUDE(InstallRequiredSystemLibraries)
INCLUDE(TestBigEndian)

### DEPENDENCY VERSIONS
#
#
## required min. libBoost version
SET(DOMO_MIN_LIBBOOST_VERSION 1.66.0)
##

### USER-SETTABLE OPTIONS
#
#

# Bundled libraries
option(USE_BUILTIN_JSONCPP "Use builtin JSonCPP" YES)
option(USE_BUILTIN_MINIZIP "Use builtin Minizip" YES)
option(USE_BUILTIN_MQTT "Use builtin Mosquitto library" YES)
option(USE_BUILTIN_SQLITE "Use builtin sqlite library" YES)

# Optional dependencies
option(USE_PYTHON "Use Python for Plugins and Event-Scripts" YES)
option(INCLUDE_LINUX_I2C "Include I2C support" YES)
option(INCLUDE_SPI "Include SPI support" YES)
option(WITH_LIBUSB "Enable libusb support" YES)

# Link static or shared, external dependencies
option(USE_LUA_STATIC "Link LUA static" YES)
option(USE_OPENSSL_STATIC "Link OpenSSL static" NO)
option(USE_STATIC_BOOST "Build with static BOOST libraries" YES)
option(USE_STATIC_OPENZWAVE "Build with static OpenZwave libraries" YES)

# Developer-oriented options
option(USE_PRECOMPILED_HEADER "Use precompiled header feature to speed up build time " YES)
option(GIT_SUBMODULE "Check submodules during build" ON)


### COMPILER SETTINGS
#
#
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

IF(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-switch -Wno-parentheses-equality -Wno-deprecated-declarations -Wno-tautological-compare -Wno-unused-value -Wno-comment -Wno-unsequenced -Wno-logical-op-parentheses -Wno-literal-conversion")
  add_definitions(-DUSE_FILE32API)
ENDIF()
IF(CMAKE_SYSTEM_NAME STREQUAL "OpenBSD")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
ENDIF()
IF(CMAKE_SYSTEM_NAME STREQUAL "NetBSD")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
ENDIF()
IF(CMAKE_COMPILER_IS_GNUCXX)
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-psabi -rdynamic")
  #SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-switch")
  #ADD_DEFINITIONS( -Wall -O0 -ggdb )
  #ADD_DEFINITIONS( -Wfatal-errors -Wformat=2 -Werror=format-security )
ENDIF()

### VERSIONING SUPPORT
#
#

MACRO(History_GET_REVISION variable)
  IF(EXISTS ${CMAKE_SOURCE_DIR}/History.txt)
    MESSAGE(STATUS "Read ProjectRevision from History.txt")
    CMAKE_POLICY(SET CMP0007 NEW)
    FILE(STRINGS ${CMAKE_SOURCE_DIR}/History.txt AppVersionContent)
    LIST(GET AppVersionContent 0 AppVersionContent)
    STRING(REPLACE " " ";" AppVersionContent ${AppVersionContent})
    LIST(GET AppVersionContent 1 AppVersionContent)
    STRING(REPLACE "." ";" AppVersionContent ${AppVersionContent})
    LIST(GET AppVersionContent 1 ${variable})
  ELSE(EXISTS ${CMAKE_SOURCE_DIR}/History.txt)
    MESSAGE(STATUS "Failed to get ProjectRevision from History.txt, set it to 0")
    set (${variable} 0)
  ENDIF(EXISTS ${CMAKE_SOURCE_DIR}/History.txt)
ENDMACRO(History_GET_REVISION)

MACRO(Gitversion_GET_REVISION dir variable)
  EXECUTE_PROCESS(COMMAND ${GIT_EXECUTABLE} --git-dir ./.git rev-list HEAD --count
    WORKING_DIRECTORY ${dir}
    OUTPUT_VARIABLE ${variable}
    OUTPUT_STRIP_TRAILING_WHITESPACE)
ENDMACRO(Gitversion_GET_REVISION)

find_package(Git QUIET)

Gitversion_GET_REVISION("${CMAKE_SOURCE_DIR}" ProjectRevision)
IF(NOT ProjectRevision)
  MESSAGE(STATUS "Failed to get ProjectRevision from git")
  History_GET_REVISION(ProjectRevision)
ELSE(NOT ProjectRevision)
  MATH(EXPR ProjectRevision "${ProjectRevision}+2107")
ENDIF(NOT ProjectRevision)

### SUBMODULE / BUNDLED SOFTWARE
#
#
IF(GIT_FOUND AND EXISTS "${PROJECT_SOURCE_DIR}/.git")
# Update submodules as needed
    IF(GIT_SUBMODULE)
        message(STATUS "Submodule update")
    execute_process(COMMAND ${GIT_EXECUTABLE} submodule update --init --remote --recursive
                        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                        RESULT_VARIABLE GIT_SUBMOD_RESULT)
        IF(NOT GIT_SUBMOD_RESULT EQUAL "0")
            message(FATAL_ERROR "git submodule update --init failed with ${GIT_SUBMOD_RESULT}, please checkout submodules")
        ENDIF()
    ENDIF()
ENDIF()

IF(USE_BUILTIN_JSONCPP AND NOT EXISTS "${PROJECT_SOURCE_DIR}/extern/jsoncpp/CMakeLists.txt")
  message(FATAL_ERROR "The submodules were not downloaded! GIT_SUBMODULE was turned off or failed. Please update submodules and try again.")
ENDIF()
IF(USE_BUILTIN_MINIZIP AND NOT EXISTS "${PROJECT_SOURCE_DIR}/extern/minizip/CMakeLists.txt")
  message(FATAL_ERROR "The submodules were not downloaded! GIT_SUBMODULE was turned off or failed. Please update submodules and try again.")
ENDIF()
IF(USE_BUILTIN_MQTT AND NOT EXISTS "${PROJECT_SOURCE_DIR}/extern/mosquitto/CMakeLists.txt")
  message(FATAL_ERROR "The submodules were not downloaded! GIT_SUBMODULE was turned off or failed. Please update submodules and try again.")
ENDIF()
IF(USE_BUILTIN_SQLITE AND NOT EXISTS "${PROJECT_SOURCE_DIR}/extern/sqlite-amalgamation/CMakeLists.txt")
  message(FATAL_ERROR "The submodules were not downloaded! GIT_SUBMODULE was turned off or failed. Please update submodules and try again.")
ENDIF()

# Target
set(
domoticz_SRCS
main/stdafx.cpp
main/BaroForecastCalculator.cpp
main/CmdLine.cpp
main/Camera.cpp
main/domoticz.cpp
main/dzVents.cpp
main/EventSystem.cpp
main/EventsPythonModule.cpp
main/EventsPythonDevice.cpp
main/Helper.cpp
main/HTMLSanitizer.cpp
main/IFTTT.cpp
main/json_helper.cpp
main/localtime_r.cpp
main/Logger.cpp
main/LuaCommon.cpp
main/LuaHandler.cpp
main/LuaTable.cpp
main/mainworker.cpp
main/mosquitto_helper.cpp
main/NotificationObserver.cpp
main/NotificationSystem.cpp
main/RFXNames.cpp
main/Scheduler.cpp
main/SignalHandler.cpp
main/SQLHelper.cpp
main/SunRiseSet.cpp
main/TrendCalculator.cpp
main/WebServer.cpp
main/WebServerHelper.cpp
main/WindCalculation.cpp
push/BasePush.cpp
push/FibaroPush.cpp
push/GooglePubSubPush.cpp
push/HttpPush.cpp
push/InfluxPush.cpp
push/WebsocketPush.cpp
httpclient/HTTPClient.cpp
httpclient/UrlEncode.cpp
hardware/1Wire.cpp
hardware/1Wire/1WireByOWFS.cpp
hardware/1Wire/1WireByKernel.cpp
hardware/1Wire/1WireCommon.cpp
hardware/1Wire/1WireForWindows.cpp
hardware/AccuWeather.cpp
hardware/AirconWithMe.cpp
hardware/AnnaThermostat.cpp
hardware/Arilux.cpp
hardware/ASyncSerial.cpp
hardware/ASyncTCP.cpp
hardware/AtagOne.cpp
hardware/BleBox.cpp
hardware/Buienradar.cpp
hardware/cayenne_lpp/CayenneLPP_Dec.cpp
hardware/ColorSwitch.cpp
hardware/Comm5Serial.cpp
hardware/Comm5SMTCP.cpp
hardware/Comm5TCP.cpp
hardware/csocket.cpp
hardware/CurrentCostMeterBase.cpp
hardware/CurrentCostMeterSerial.cpp
hardware/CurrentCostMeterTCP.cpp
hardware/Daikin.cpp
hardware/DarkSky.cpp
hardware/DavisLoggerSerial.cpp
hardware/DenkoviDevices.cpp
hardware/DenkoviUSBDevices.cpp
hardware/DenkoviTCPDevices.cpp
hardware/DomoticzHardware.cpp
hardware/DomoticzInternal.cpp
hardware/DomoticzTCP.cpp
hardware/Dummy.cpp
hardware/EcoCompteur.cpp
hardware/EcoDevices.cpp
hardware/eHouseTCP.cpp
hardware/eHouse/EhouseEvents.cpp
hardware/eHouse/EhouseTcpClient.cpp
hardware/eHouse/EhouseUdpListener.cpp
hardware/EnOceanESP2.cpp
hardware/EnOceanESP3.cpp
hardware/EnphaseAPI.cpp
hardware/Ec3kMeterTCP.cpp
hardware/eVehicles/TeslaApi.cpp
hardware/eVehicles/MercApi.cpp
hardware/eVehicles/TestcarApi.cpp
hardware/eVehicles/eVehicle.cpp
hardware/EvohomeBase.cpp
hardware/EvohomeRadio.cpp
hardware/EvohomeScript.cpp
hardware/EvohomeSerial.cpp
hardware/EvohomeTCP.cpp
hardware/EvohomeWeb.cpp
hardware/ETH8020.cpp
hardware/FritzboxTCP.cpp
hardware/GoodweAPI.cpp
hardware/Gpio.cpp
hardware/GpioPin.cpp
hardware/HardwareMonitor.cpp
hardware/HarmonyHub.cpp
hardware/Honeywell.cpp
hardware/HEOS.cpp
hardware/I2C.cpp
hardware/ICYThermostat.cpp
hardware/InComfort.cpp
hardware/KMTronicBase.cpp
hardware/KMTronic433.cpp
hardware/KMTronicSerial.cpp
hardware/KMTronicTCP.cpp
hardware/KMTronicUDP.cpp
hardware/Kodi.cpp
hardware/Limitless.cpp
hardware/LogitechMediaServer.cpp
hardware/Meteorologisk.cpp
hardware/Meteostick.cpp
hardware/MochadTCP.cpp
hardware/MQTT.cpp
hardware/MultiFun.cpp
hardware/MySensorsBase.cpp
hardware/MySensorsSerial.cpp
hardware/MySensorsTCP.cpp
hardware/MySensorsMQTT.cpp
hardware/NefitEasy.cpp
hardware/Nest.cpp
hardware/NestOAuthAPI.cpp
hardware/Netatmo.cpp
hardware/HttpPoller.cpp
hardware/OctoPrintMQTT.cpp
hardware/OnkyoAVTCP.cpp
hardware/OpenWeatherMap.cpp
hardware/OpenWebNetTCP.cpp
hardware/OpenWebNetUSB.cpp
hardware/openwebnet/bt_openwebnet.cpp
hardware/OpenZWave.cpp
hardware/openzwave/control_panel/ozwcp.cpp
hardware/openzwave/control_panel/zwavelib.cpp
hardware/OTGWBase.cpp
hardware/OTGWSerial.cpp
hardware/OTGWTCP.cpp
hardware/PanasonicTV.cpp
hardware/P1MeterBase.cpp
hardware/P1MeterSerial.cpp
hardware/P1MeterTCP.cpp
hardware/PhilipsHue/PhilipsHue.cpp
hardware/PhilipsHue/PhilipsHueHelper.cpp
hardware/PhilipsHue/PhilipsHueSensors.cpp
hardware/PiFace.cpp
hardware/Pinger.cpp
hardware/PVOutput_Input.cpp
hardware/RAVEn.cpp
hardware/Rego6XXSerial.cpp
hardware/RelayNet.cpp
hardware/RFLinkBase.cpp
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
hardware/serial/serial.cpp
hardware/serial/impl/unix.cpp
hardware/SolarEdgeAPI.cpp
hardware/SolarMaxTCP.cpp
hardware/Sterbox.cpp
hardware/SysfsGpio.cpp
hardware/Tado.cpp
hardware/TCPProxy/tcpproxy_server.cpp
hardware/TE923.cpp
hardware/TE923Tool.cpp
hardware/TeleinfoBase.cpp
hardware/TeleinfoSerial.cpp
hardware/Tellstick.cpp
hardware/TellstickFactory.cpp
hardware/Thermosmart.cpp
hardware/ToonThermostat.cpp
hardware/TTNMQTT.cpp
hardware/USBtin.cpp
hardware/USBtin_MultiblocV8.cpp
hardware/VolcraftCO20.cpp
hardware/Winddelen.cpp
hardware/WOL.cpp
hardware/Wunderground.cpp
hardware/XiaomiGateway.cpp
hardware/Yeelight.cpp
hardware/YouLess.cpp
hardware/ZiBlueBase.cpp
hardware/ZiBlueSerial.cpp
hardware/ZiBlueTCP.cpp
hardware/ZWaveBase.cpp
hardware/plugins/DelayedLink.cpp
hardware/plugins/Plugins.cpp
hardware/plugins/PluginManager.cpp
hardware/plugins/PluginProtocols.cpp
hardware/plugins/PluginTransports.cpp
hardware/plugins/PythonObjects.cpp
notifications/NotificationBase.cpp
notifications/NotificationBrowser.cpp
notifications/NotificationEmail.cpp
notifications/NotificationFCM.cpp
notifications/NotificationHelper.cpp
notifications/NotificationHTTP.cpp
notifications/NotificationKodi.cpp
notifications/NotificationLogitechMediaServer.cpp
notifications/NotificationPushbullet.cpp
notifications/NotificationProwl.cpp
notifications/NotificationPushover.cpp
notifications/NotificationPushsafer.cpp
notifications/NotificationPushalot.cpp
notifications/NotificationSMS.cpp
notifications/NotificationTelegram.cpp
smtpclient/SMTPClient.cpp
tcpserver/TCPClient.cpp
tcpserver/TCPServer.cpp
webserver/Base64.cpp
webserver/connection.cpp
webserver/connection_manager.cpp
webserver/cWebem.cpp
webserver/fastcgi.cpp
webserver/mime_types.cpp
webserver/proxycereal.cpp
webserver/proxyclient.cpp
webserver/reply.cpp
webserver/request_handler.cpp
webserver/request_parser.cpp
webserver/server.cpp
webserver/Websockets.cpp
webserver/WebsocketHandler.cpp
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
tinyxpath/xpath_stream.cpp
tinyxpath/xpath_stack.cpp
tinyxpath/xpath_static.cpp
tinyxpath/xpath_syntax.cpp
)

add_executable(domoticz ${domoticz_SRCS})
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR})
#SET(EXECUTABLE_OUTPUT_PATH "" CACHE INTERNAL "Where to put the executables for Domoticz")

#
# LUA
#
IF(USE_LUA_STATIC)
  find_library(LUA_LIBRARIES NAMES liblua5.3.a)
ELSE(USE_LUA_STATIC)
  find_library(LUA_LIBRARIES NAMES liblua5.3.so)
ENDIF(USE_LUA_STATIC)
find_path(LUA_INCLUDE_DIRS NAMES lua5.3/lua.h)
IF(LUA_LIBRARIES AND LUA_INCLUDE_DIRS)
  MESSAGE(STATUS "LUA library found at: ${LUA_LIBRARIES}")
  INCLUDE_DIRECTORIES(${LUA_INCLUDE_DIRS}/lua5.3)
ELSE(LUA_LIBRARIES AND LUA_INCLUDE_DIRS)
  # try using find_package()
  find_package(Lua "5.3" REQUIRED)
  IF(LUA_FOUND)
    MESSAGE(STATUS "LUA library found at  : ${LUA_LIBRARIES}")
    MESSAGE(STATUS "LUA includes found at : ${LUA_INCLUDE_DIR}")
    INCLUDE_DIRECTORIES(${LUA_INCLUDE_DIR}) 
  ELSE(LUA_FOUND)
    MESSAGE(FATAL_ERROR "LUA 5.3 not found! use sudo apt-get install liblua5.3-dev")
  ENDIF(LUA_FOUND)
ENDIF(LUA_LIBRARIES AND LUA_INCLUDE_DIRS)

IF(USE_PYTHON)
  find_package(PythonInterp 3.4)
  find_package(PythonLibs 3.4)
  IF(PYTHONLIBS_FOUND)
    MESSAGE(STATUS "Python3 includes found at: ${PYTHON_INCLUDE_PATH}")
    INCLUDE_DIRECTORIES(${PYTHON_INCLUDE_PATH})
    add_definitions(-DENABLE_PYTHON)
  ELSE(PYTHONLIBS_FOUND)
    MESSAGE(FATAL_ERROR "Python3 not found on your system, use USE_PYTHON=NO or sudo apt-get install python3-dev)")
  ENDIF(PYTHONLIBS_FOUND)
ENDIF(USE_PYTHON)

CHECK_INCLUDE_FILE (execinfo.h HAVE_EXECINFO_H)

IF(HAVE_EXECINFO_H)
  # FreeBSD has to include libexecinfo
  IF(CMAKE_SYSTEM_NAME STREQUAL "FreeBSD")
    find_library(EXECINFO_LIBRARIES NAMES libexecinfo.so)
    IF(EXECINFO_LIBRARIES)
      ADD_DEFINITIONS(-DHAVE_EXECINFO_H)
    ENDIF()
  ELSEIF(CMAKE_SYSTEM_NAME STREQUAL "NetBSD")
    find_library(EXECINFO_LIBRARIES NAMES libexecinfo.so)
    IF(EXECINFO_LIBRARIES)
      ADD_DEFINITIONS(-DHAVE_EXECINFO_H)
    ENDIF()
  ELSE()
    ADD_DEFINITIONS(-DHAVE_EXECINFO_H)
  ENDIF()
ENDIF(HAVE_EXECINFO_H)

IF(INCLUDE_LINUX_I2C)
  CHECK_INCLUDE_FILES ("sys/types.h;linux/i2c-dev.h;linux/i2c.h" HAVE_LINUX_I2C_H)
  IF(HAVE_LINUX_I2C_H)
    message(STATUS "Building with I2C support")
    add_definitions(-DHAVE_LINUX_I2C)
  ELSE()
    message(WARNING "I2C support disabled: headers not found!")
  ENDIF(HAVE_LINUX_I2C_H)
ENDIF(INCLUDE_LINUX_I2C)

IF(INCLUDE_SPI)
  CHECK_INCLUDE_FILES ("sys/types.h;linux/spi/spidev.h" HAVE_LINUX_SPI_H)
  IF(HAVE_LINUX_SPI_H)
    message(STATUS "Building with SPI support")
    add_definitions(-DHAVE_LINUX_SPI)
  ELSE()
    message(WARNING "SPI support disabled: headers not found!")
  ENDIF(HAVE_LINUX_SPI_H)
ENDIF(INCLUDE_SPI)

FIND_PROGRAM(GIT_EXECUTABLE git
  DOC "git command line client")

include_directories(${CMAKE_SOURCE_DIR}/main)

# a custom target that is always built
ADD_CUSTOM_TARGET(revisiontag ALL)

# creates appversion.h using cmake script
ADD_CUSTOM_COMMAND(TARGET revisiontag COMMAND ${CMAKE_COMMAND}
   -DSOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}
   -P ${CMAKE_CURRENT_SOURCE_DIR}/getgit.cmake)

MESSAGE(STATUS "###########################")
MESSAGE(STATUS "Compiling Revision #${ProjectRevision}")
MESSAGE(STATUS "###########################")

# The version number.
set (domoticz_VERSION_MAJOR 2020)
set (domoticz_VERSION_MINOR 1)
set (domoticz_VERSION_PATCH ${ProjectRevision})

# explicitly say that the executable depends on the revisiontag
add_dependencies(domoticz revisiontag)

TEST_BIG_ENDIAN(BIGENDIAN)
IF(${BIGENDIAN})
     ADD_DEFINITIONS(-DIS_BIG_ENDIAN)
ENDIF(${BIGENDIAN})

## Link libraries

# jsoncpp
IF(USE_BUILTIN_JSONCPP)
  MESSAGE(STATUS "Using builtin jsoncpp library")
#  set(JSONCPP_LIB_BUILD_SHARED OFF CACHE BOOL "Enable jsoncpp shared library" FORCE)
  set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build jsoncpp_lib as a shared library." FORCE)
  set(BUILD_STATIC_LIBS ON CACHE BOOL "Build jsoncpp_lib as a static library." FORCE)
  set(JSONCPP_WITH_TESTS OFF CACHE BOOL "Enable jsoncpp tests" FORCE)
  set(JSONCPP_WITH_POST_BUILD_UNITTEST OFF CACHE BOOL "Enable jsoncpp post build unit tests" FORCE)
  set(JSONCPP_WITH_PKGCONFIG_SUPPORT OFF CACHE BOOL "Enable jsoncpp pkgconfig support" FORCE)
  add_subdirectory (extern/jsoncpp EXCLUDE_FROM_ALL)
  target_link_libraries(domoticz jsoncpp_static)
  INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/extern/jsoncpp/include)
ELSE(USE_BUILTIN_JSONCPP)
  find_package(PkgConfig)
  pkg_check_modules(JSONCPP REQUIRED jsoncpp)
  IF(JSONCPP_FOUND)
    MESSAGE(STATUS "JSONCPP includes found at: ${JSONCPP_INCLUDE_DIRS}")
    target_include_directories(domoticz PRIVATE ${JSONCPP_INCLUDE_DIRS})
    target_link_directories(domoticz PRIVATE ${JSONCPP_LIBRARY_DIRS})
    target_link_libraries(domoticz ${JSONCPP_LIBRARIES})
  ELSE(JSONCPP_FOUND)
    MESSAGE(FATAL_ERROR "JSONCPP not found on your system! try 'sudo apt-get install jsoncpp-dev'")
  ENDIF(JSONCPP_FOUND)
ENDIF(USE_BUILTIN_JSONCPP)

# mosquitto
IF(USE_BUILTIN_MQTT)
  MESSAGE(STATUS "Using builtin Mosquitto library")
  option(WITH_STATIC_LIBRARIES "Build static versions of the libmosquitto/pp libraries?" ON)
  option(DOCUMENTATION "Build documentation?" OFF)
  add_subdirectory (extern/mosquitto EXCLUDE_FROM_ALL)
  target_link_libraries(domoticz libmosquitto_static)
  INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/extern/mosquitto/include)
  INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/extern/mosquitto/lib)
ELSE(USE_BUILTIN_MQTT)
  find_library(MQTT_LIBRARIES NAMES libmosquitto.so)
  find_path(MQTT_INCLUDE_DIRS NAMES mosquitto.h)
  IF(MQTT_LIBRARIES AND MQTT_INCLUDE_DIRS)
    MESSAGE(STATUS "MQTT includes found at: ${MQTT_INCLUDE_DIRS}")
    INCLUDE_DIRECTORIES(${MQTT_INCLUDE_DIRS})
  ELSE(MQTT_LIBRARIES AND MQTT_INCLUDE_DIRS)
    MESSAGE(FATAL_ERROR "Mosquitto library not found on your system, see install.txt how to get them installed. (for example 'sudo apt-get install libmosquitto-dev')")
  ENDIF(MQTT_LIBRARIES AND MQTT_INCLUDE_DIRS)
ENDIF(USE_BUILTIN_MQTT)

# sqlite3
IF(USE_BUILTIN_SQLITE)
  MESSAGE(STATUS "Using builtin SQLite library")
  add_subdirectory (extern/sqlite-amalgamation EXCLUDE_FROM_ALL)
  target_link_libraries(domoticz SQLite3)
  INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/extern/sqlite-amalgamation)
ELSE(USE_BUILTIN_SQLITE)
  find_package(SQLite3)
  IF(SQLite3_FOUND)
    MESSAGE(STATUS "SQLite library found at: ${SQLite3_LIBRARIES}")
    MESSAGE(STATUS "SQLite includes found at: ${SQLite3_INCLUDE_DIRS}")
    INCLUDE_DIRECTORIES(${SQLite3_INCLUDE_DIRS})
    target_link_libraries(domoticz ${SQLite3_LIBRARIES})
  ELSE(SQLite3_FOUND)
    MESSAGE(FATAL_ERROR "SQLite3 not found on your system! try 'sudo apt-get install libsqlite3-dev'")
  ENDIF(SQLite3_FOUND)
ENDIF(USE_BUILTIN_SQLITE)

# minizip
IF(USE_BUILTIN_MINIZIP)
  add_subdirectory (extern/minizip EXCLUDE_FROM_ALL)
  target_link_libraries(domoticz minizip)
  INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/extern/minizip)
ELSE(USE_BUILTIN_MINIZIP)
  find_package(PkgConfig)
  pkg_check_modules(MINIZIP REQUIRED minizip)
  IF(MINIZIP_FOUND)
    MESSAGE(STATUS "MINIZIP includes found at: ${MINIZIP_INCLUDE_DIRS}")
    target_include_directories(domoticz PRIVATE ${MINIZIP_INCLUDE_DIRS})
    target_link_directories(domoticz PRIVATE ${MINIZIP_LIBRARY_DIRS})
    target_link_libraries(domoticz ${MINIZIP_LIBRARIES})
  ELSE(MINIZIP_FOUND)
    MESSAGE(FATAL_ERROR "MINIZIP not found on your system! try 'sudo apt-get install minizip-dev'")
  ENDIF(MINIZIP_FOUND)
ENDIF(USE_BUILTIN_MINIZIP)

#
# Find MD5/RMD160/SHA library
#
IF(USE_OPENSSL_STATIC)
  set(OPENSSL_USE_STATIC_LIBS TRUE)
ENDIF(USE_OPENSSL_STATIC)

find_package(OpenSSL REQUIRED)
IF(NOT OPENSSL_INCLUDE_DIR)
  message(SEND_ERROR "Failed to find OpenSSL include files (ssl.h), no HTTPS support")
ENDIF()
IF(NOT OPENSSL_FOUND)
  message(SEND_ERROR "Failed to find the OpenSSL library, no HTTPS support")
  find_library(MD_LIBRARY NAMES md)
  IF(MD_LIBRARY)
    target_link_libraries(domoticz ${MD_LIBRARY})
  ENDIF(MD_LIBRARY)
ELSE()
  message(STATUS "OPENSSL library found at: ${OPENSSL_LIBRARIES}")
  add_definitions(-DWWW_ENABLE_SSL)
  add_definitions(-DWITH_TLS)
  include_directories(${OPENSSL_INCLUDE_DIR})
  target_link_libraries(domoticz ${OPENSSL_LIBRARIES})
ENDIF()

#
# Boost
#
set(Boost_USE_STATIC_LIBS ${USE_STATIC_BOOST})
set(Boost_USE_MULTITHREADED ON)
unset(Boost_INCLUDE_DIR CACHE)
unset(Boost_LIBRARY_DIRS CACHE)

IF(USE_STATIC_BOOST)
   message(STATUS "Linking against boost static libraries")
ELSE(USE_STATIC_BOOST)
   message(STATUS "Linking against boost dynamic libraries")
ENDIF(USE_STATIC_BOOST)

find_package(Boost REQUIRED COMPONENTS thread system)
IF(Boost_FOUND)
    MESSAGE(STATUS "BOOST includes found at: ${Boost_INCLUDE_DIR}")
ELSE(Boost_FOUND)
    MESSAGE(FATAL_ERROR "Boost thread/system library not found on your system, try to get this installed.")
ENDIF(Boost_FOUND)

target_link_libraries(domoticz Boost::thread Boost::system)

# compare found vs required libBoost version
IF(Boost_VERSION VERSION_LESS DOMO_MIN_LIBBOOST_VERSION)
   message(FATAL_ERROR "Found libBoost version ${Boost_VERSION}, ${DOMO_MIN_LIBBOOST_VERSION} or newer required")
ENDIF(Boost_VERSION VERSION_LESS DOMO_MIN_LIBBOOST_VERSION)

include_directories(${Boost_INCLUDE_DIRS})

#
# ZLIB
#
find_package(ZLIB REQUIRED)
IF(ZLIB_FOUND)
  MESSAGE(STATUS "ZLIB library found at: ${ZLIB_LIBRARIES}")
  MESSAGE(STATUS "ZLIB includes found at: ${ZLIB_INCLUDE_DIRS}")
  include_directories(${ZLIB_INCLUDE_DIRS})
ELSE()
  MESSAGE(FATAL_ERROR "ZLIB not found on your system! try 'sudo apt-get install zlib1g-dev'")
ENDIF(ZLIB_FOUND)

#
# CURL
#
FIND_PACKAGE(CURL REQUIRED)
IF(CURL_FOUND)
  MESSAGE(STATUS "Curl library found at: ${CURL_LIBRARIES}")
  MESSAGE(STATUS "Curl includes found at: ${CURL_INCLUDE_DIRS}")
  INCLUDE_DIRECTORIES(${CURL_INCLUDE_DIRS})
ELSE()
  MESSAGE(FATAL_ERROR "cURL not found! try 'sudo apt-get install curl libcurl4-gnutls-dev'")
ENDIF(CURL_FOUND)

#
# CEREAL
#
find_path(CEREAL_INCLUDE_DIRS NAMES cereal/cereal.hpp)
IF(CEREAL_INCLUDE_DIRS)
  MESSAGE(STATUS "CEREAL includes found at: ${CEREAL_INCLUDE_DIRS}")
  include_directories(${CEREAL_INCLUDE_DIRS})
ELSE()
  MESSAGE(FATAL_ERROR "CEREAL not found on your system! try 'sudo apt-get install libcereal-dev'")
ENDIF(CEREAL_INCLUDE_DIRS)

#
# USB
#
IF(WITH_LIBUSB)
  find_path(LIBUSB_INCLUDE_DIR usb.h
     HINTS ${PC_LIBUSB_INCLUDEDIR} ${PC_LIBUSB_INCLUDE_DIRS})
  find_library(LIBUSB_LIBRARY NAMES usb
     HINTS ${PC_LIBUSB_LIBDIR} ${PC_LIBUSB_LIBRARY_DIRS})
  set(LIBUSB_LIBRARIES ${LIBUSB_LIBRARY})

  find_package_handle_standard_args(LIBUSB  DEFAULT_MSG  LIBUSB_LIBRARIES LIBUSB_INCLUDE_DIR)
  IF(LIBUSB_FOUND)
    MESSAGE(STATUS "LIBUSB library found at: ${LIBUSB_LIBRARIES}")
    add_definitions(-DWITH_LIBUSB)
    target_link_libraries(domoticz ${LIBUSB_LIBRARIES})
  ELSE()
    MESSAGE(STATUS "==== LibUSB not found, support for TE923/Voltcraft disabled!")
  ENDIF(LIBUSB_FOUND)
ENDIF(WITH_LIBUSB)

#
# OpenZWave
# try to find open-zwave, if found, include support
#
IF(USE_STATIC_OPENZWAVE)
  find_library(OpenZWave NAMES libopenzwave.a HINTS "../open-zwave-read-only" "../open-zwave-read-only/cpp/build")
  get_filename_component(OpenZWave_dir ${OpenZWave} DIRECTORY)
  find_path(OPENZWAVE_INCLUDE_DIRS NAMES OZWException.h HINTS "${OpenZWave_dir}/cpp/src")
  set(OPENZWAVE_LIB ${OpenZWave})
ELSE()
  pkg_check_modules(OPENZWAVE libopenzwave)
  IF(OPENZWAVE_FOUND)
    MESSAGE(STATUS "==== OpenZWave package found!")
    find_library(OpenZWave NAMES libopenzwave.so HINTS ${OPENZWAVE_LIBRARY_DIRS})
    message(STATUS OpenZWave library found at: ${OpenZWave})
  ENDIF()
ENDIF()
IF(OpenZWave)
  message(STATUS "OpenZWave library found at: ${OpenZWave}")
  target_link_libraries(domoticz ${OpenZWave})

  find_path(OPENZWAVE_INCLUDE_DIRS NAMES OZWException.h)
  IF (OPENZWAVE_INCLUDE_DIRS)
    include_directories(${OPENZWAVE_INCLUDE_DIRS})
    message(STATUS "OpenZWave includes found at: ${OPENZWAVE_INCLUDE_DIRS}")
  ELSE()
    message(FATAL_ERROR "OpenZWave includes not found.")
  ENDIF (OPENZWAVE_INCLUDE_DIRS)
  add_definitions(-DWITH_OPENZWAVE)
ELSE()
  MESSAGE(STATUS "==== OpenZWave not found, support disabled!")
ENDIF(OpenZWave)

IF(EXISTS /sys/class/gpio)
  message(STATUS "GPIO is available")
  add_definitions(-DWITH_GPIO)
ELSE()
  message(STATUS "GPIO is not available")
ENDIF()

find_path(TELLDUSCORE_INCLUDE NAMES telldus-core.h)
IF(TELLDUSCORE_INCLUDE)
  message(STATUS "Found telldus-core (telldus-core.h) at : ${TELLDUSCORE_INCLUDE}")
  find_library(TELLDUS_LIBRARIES NAMES libtelldus-core.so)
  IF(TELLDUS_LIBRARIES)
    message(STATUS "Found libtelldus-core at : ${TELLDUS_LIBRARIES}, adding telldus support")
    add_definitions(-DWITH_TELLDUSCORE)
  ENDIF(TELLDUS_LIBRARIES)
ELSE()
  message(STATUS "Not found telldus-core (telldus-core.h), not adding tellstick support")
ENDIF(TELLDUSCORE_INCLUDE)

target_link_libraries(domoticz ${Boost_LIBRARIES} ${ZLIB_LIBRARIES} ${MINIZIP_LIBRARIES} ${CURL_LIBRARIES} pthread ${MQTT_LIBRARIES} ${LUA_LIBRARIES} ${CMAKE_DL_LIBS} ${TELLDUS_LIBRARIES})

IF(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
  target_link_libraries(domoticz -lresolv)
ELSEIF(CMAKE_SYSTEM_NAME STREQUAL "OpenBSD")
  target_link_libraries(domoticz ${EXECINFO_LIBRARIES})
ELSEIF(CMAKE_SYSTEM_NAME STREQUAL "FreeBSD")
  target_link_libraries(domoticz ${EXECINFO_LIBRARIES})
ELSE()
  target_link_libraries(domoticz -lrt -lresolv ${EXECINFO_LIBRARIES})
ENDIF()

IF(USE_PRECOMPILED_HEADER)
  message(STATUS "Using precompiled headers")
  target_precompile_headers(domoticz PRIVATE "main/stdafx.h")
ENDIF(USE_PRECOMPILED_HEADER)

IF(CMAKE_COMPILER_IS_GNUCXX)
  option(USE_STATIC_LIBSTDCXX "Build with static libgcc/libstdc++ libraries" YES)
  IF(USE_STATIC_LIBSTDCXX)
    message(STATUS "Using static libgcc/libstdc++")
    SET(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS "${CMAKE_SHARED_LIBRARY_LINK_C_FLAGS} -static-libgcc -static-libstdc++")
    SET(CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS "${CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS} -static-libgcc -static-libstdc++")
    SET(CMAKE_CXX_LINK_FLAGS "${CMAKE_CXX_LINK_FLAGS} -static-libgcc -static-libstdc++")
  ELSE(USE_STATIC_LIBSTDCXX)
    message(STATUS "Using dynamic libgcc_s/libstdc++")
  ENDIF(USE_STATIC_LIBSTDCXX)
ENDIF(CMAKE_COMPILER_IS_GNUCXX)

# build a CPack driven installer package

SET(CPACK_GENERATOR "DEB")
SET(CPACK_PACKAGE_NAME "domoticz")
SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY "${domoticz}-Home Automation System (Domotica).")
SET(CPACK_PACKAGE_VENDOR "Domoticz.com")
SET(CPACK_PACKAGE_CONTACT "info@domoticz.com")
SET(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/License.txt")

SET(CPACK_PACKAGE_VERSION_MAJOR "${domoticz_VERSION_MAJOR}")
SET(CPACK_PACKAGE_VERSION_MINOR "${domoticz_VERSION_MINOR}")
SET(CPACK_PACKAGE_VERSION_PATCH "${domoticz_VERSION_PATCH}")

SET(CPACK_PACKAGE_INSTALL_DIRECTORY "domoticz")
#SET (DOMOTICZ_VERSION_SHORT "${domoticz_VERSION_MAJOR}.${domoticz_VERSION_MINOR}.${domoticz_VERSION_PATCH}")
#SET(CPACK_PACKAGE_FILE_NAME "domoticz-${DOMOTICZ_VERSION_SHORT}-${CMAKE_SYSTEM_NAME}")

SET(CPACK_SOURCE_PACKAGE_FILE_NAME "${domoticz}-dev")
SET(CPACK_PACKAGING_INSTALL_PREFIX "/")
SET(CPACK_PACKAGE_DEFAULT_LOCATION "/opt/${CPACK_PACKAGE_NAME}")
SET(CPACK_PACKAGE_EXECUTABLES "domoticz;Home Automation System (Domotica).")

#set(CPACK_DEB_COMPONENT_INSTALL TRUE)
SET(CPACK_DEBIAN_ARCHITECTURE ${CMAKE_SYSTEM_PROCESSOR})

INCLUDE(CPack)

IF(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
  set(CMAKE_INSTALL_PREFIX "/opt/${CPACK_PACKAGE_NAME}" CACHE PATH "Install path prefix, prepended onto install directories." FORCE)
ENDIF(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)

install(TARGETS domoticz DESTINATION ${CMAKE_INSTALL_PREFIX})
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/www DESTINATION ${CMAKE_INSTALL_PREFIX})
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/License.txt DESTINATION ${CMAKE_INSTALL_PREFIX})
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/History.txt DESTINATION ${CMAKE_INSTALL_PREFIX})
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/server_cert.pem DESTINATION ${CMAKE_INSTALL_PREFIX})
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/scripts DESTINATION ${CMAKE_INSTALL_PREFIX} USE_SOURCE_PERMISSIONS)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/plugins DESTINATION ${CMAKE_INSTALL_PREFIX})
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/Config DESTINATION ${CMAKE_INSTALL_PREFIX})
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/dzVents DESTINATION ${CMAKE_INSTALL_PREFIX})
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/updatedomo DESTINATION ${CMAKE_INSTALL_PREFIX} PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ GROUP_EXECUTE GROUP_READ)
