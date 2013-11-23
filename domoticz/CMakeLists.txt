cmake_minimum_required(VERSION 2.6)

add_subdirectory (lua)
get_directory_property (CMAKE_LUA_INCLUDES DIRECTORY lua
    DEFINITION LUA_INCLUDE_DIRS)
get_directory_property (CMAKE_LUA_LIBRARIES DIRECTORY lua
    DEFINITION LUA_LIBRARIES)

project(domoticz)

SET(EXECUTABLE_OUTPUT_PATH "" CACHE INTERNAL 
  "Where to put the executables for Domoticz"
  )

#set(CMAKE_EXE_LINKER_FLAGS "-static")

# Macro for setting up precompiled headers. Usage:
#
#   add_precompiled_header(target header.h [FORCEINCLUDE])
#
# MSVC: A source file with the same name as the header must exist and
# be included in the target (E.g. header.cpp).
#
# MSVC: Add FORCEINCLUDE to automatically include the precompiled
# header file from every source file.
#
# GCC: The precompiled header is always automatically included from
# every header file.
MACRO(ADD_PRECOMPILED_HEADER _targetName _input)
  GET_FILENAME_COMPONENT(_inputWe ${_input} NAME_WE)
  SET(pch_source ${_inputWe}.cpp)
  FOREACH(arg ${ARGN})
    IF(arg STREQUAL FORCEINCLUDE)
      SET(FORCEINCLUDE ON)
    ELSE(arg STREQUAL FORCEINCLUDE)
      SET(FORCEINCLUDE OFF)
    ENDIF(arg STREQUAL FORCEINCLUDE)
  ENDFOREACH(arg)

  IF(MSVC)
    GET_TARGET_PROPERTY(sources ${_targetName} SOURCES)
    SET(_sourceFound FALSE)
    FOREACH(_source ${sources})
      SET(PCH_COMPILE_FLAGS "")
      IF(_source MATCHES \\.\(cc|cxx|cpp\)$)
        GET_FILENAME_COMPONENT(_sourceWe ${_source} NAME_WE)
        IF(_sourceWe STREQUAL ${_inputWe})
          SET(PCH_COMPILE_FLAGS "${PCH_COMPILE_FLAGS} /Yc${_input}")
          SET(_sourceFound TRUE)
        ELSE(_sourceWe STREQUAL ${_inputWe})
          SET(PCH_COMPILE_FLAGS "${PCH_COMPILE_FLAGS} /Yu${_input}")
          IF(FORCEINCLUDE)
            SET(PCH_COMPILE_FLAGS "${PCH_COMPILE_FLAGS} /FI${_input}")
          ENDIF(FORCEINCLUDE)
        ENDIF(_sourceWe STREQUAL ${_inputWe})
        SET_SOURCE_FILES_PROPERTIES(${_source} PROPERTIES COMPILE_FLAGS "${PCH_COMPILE_FLAGS}")
      ENDIF(_source MATCHES \\.\(cc|cxx|cpp\)$)
    ENDFOREACH()
    IF(NOT _sourceFound)
      MESSAGE(FATAL_ERROR "A source file for ${_input} was not found. Required for MSVC builds.")
    ENDIF(NOT _sourceFound)
  ENDIF(MSVC)

  IF(CMAKE_COMPILER_IS_GNUCXX)
	#ADD_DEFINITIONS( -Wall -O0 -ggdb )
	#ADD_DEFINITIONS( -Wfatal-errors -Wformat=2 -Werror=format-security )
    GET_FILENAME_COMPONENT(_name ${_input} NAME)
    SET(_source "${CMAKE_CURRENT_SOURCE_DIR}/${_input}")
    SET(_outdir "${CMAKE_CURRENT_BINARY_DIR}/${_name}.gch")
    MAKE_DIRECTORY(${_outdir})
    SET(_output "${_outdir}/.c++")

    STRING(TOUPPER "CMAKE_CXX_FLAGS_${CMAKE_BUILD_TYPE}" _flags_var_name)
    SET(_compiler_FLAGS ${${_flags_var_name}})

    GET_DIRECTORY_PROPERTY(_directory_flags INCLUDE_DIRECTORIES)
    FOREACH(item ${_directory_flags})
      LIST(APPEND _compiler_FLAGS "-I${item}")
    ENDFOREACH(item)

    GET_DIRECTORY_PROPERTY(_directory_flags DEFINITIONS)
    LIST(APPEND _compiler_FLAGS ${_directory_flags})

    SEPARATE_ARGUMENTS(_compiler_FLAGS)
    MESSAGE("${CMAKE_CXX_COMPILER} -DPCHCOMPILE ${_compiler_FLAGS} -x c++-header -o {_output} ${_source}")
    ADD_CUSTOM_COMMAND(
      OUTPUT ${_output}
      COMMAND ${CMAKE_CXX_COMPILER} ${_compiler_FLAGS} -x c++-header -o ${_output} ${_source}
      DEPENDS ${_source} )
    ADD_CUSTOM_TARGET(${_targetName}_gch DEPENDS ${_output})
    ADD_DEPENDENCIES(${_targetName} ${_targetName}_gch)
    SET_TARGET_PROPERTIES(${_targetName} PROPERTIES COMPILE_FLAGS "-include ${_name} -Winvalid-pch")
  ENDIF(CMAKE_COMPILER_IS_GNUCXX)
ENDMACRO()

find_path(SQLITE3_INCLUDE_DIR sqlite3.h)
find_library(SQLITE3_LIBRARY sqlite3)
if(NOT SQLITE3_INCLUDE_DIR)
  message(SEND_ERROR "Failed to find sqlite3.h")
endif()
if(NOT SQLITE3_LIBRARY)
  message(SEND_ERROR "Failed to find the sqlite3 library")
endif()

FIND_PROGRAM(SVN_EXECUTABLE svn
  DOC "subversion command line client")

MACRO(Subversion_GET_REVISION dir variable)
  EXECUTE_PROCESS(COMMAND ${SVN_EXECUTABLE} info ${dir}
    OUTPUT_VARIABLE ${variable}
    OUTPUT_STRIP_TRAILING_WHITESPACE)
  STRING(REGEX REPLACE "^(.*\n)?Revision: ([^\n]+).*"
    "\\2" ${variable} "${${variable}}")
ENDMACRO(Subversion_GET_REVISION)

Subversion_GET_REVISION(. ProjectRevision)
MESSAGE(STATUS "Compiling Revision #${ProjectRevision}")

# The version number.
set (domoticz_VERSION_MAJOR 1)
set (domoticz_VERSION_MINOR 0)
set (domoticz_VERSION_PATCH ${ProjectRevision})

include_directories(${CMAKE_SOURCE_DIR}/main ${CMAKE_SOURCE_DIR}/hardware) 

# a custom target that is always built
ADD_CUSTOM_TARGET(revisiontag ALL)

# creates svnversion.h using cmake script
ADD_CUSTOM_COMMAND(TARGET revisiontag COMMAND ${CMAKE_COMMAND}
   -DSOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR} 
   -P ${CMAKE_CURRENT_SOURCE_DIR}/getsvn.cmake)

# This needs to be before the add_executable below
IF(MSVC)
CMAKE_POLICY(SET CMP0015 NEW)
LINK_DIRECTORIES(${Boost_LIBRARY_DIR} )
ENDIF(MSVC)

## Target
set(
domoticz_SRCS
main/stdafx.cpp
main/Logger.cpp
main/domoticz.cpp
main/CmdLine.cpp
main/EventSystem.cpp
main/mainworker.cpp
main/SQLHelper.cpp
main/WebServer.cpp
main/Scheduler.cpp
main/Camera.cpp
main/localtime_r.cpp
main/RFXNames.cpp
main/Helper.cpp
main/SunRiseSet.cpp
main/WindCalculation.cpp
httpclient/UrlEncode.cpp
httpclient/HTTPClient.cpp
smtpclient/SMTPClient.cpp
hardware/DomoticzHardware.cpp
hardware/1Wire.cpp
hardware/ASyncSerial.cpp
hardware/Dummy.cpp
hardware/Limitless.cpp
hardware/RFXComSerial.cpp
hardware/RFXComTCP.cpp
hardware/DomoticzTCP.cpp
hardware/P1MeterBase.cpp
hardware/P1MeterTCP.cpp
hardware/P1MeterSerial.cpp
hardware/YouLess.cpp
hardware/TE923.cpp
hardware/TE923Tool.cpp
hardware/Teleinfo.cpp
hardware/VolcraftCO20.cpp
hardware/VolcraftCO20Tool.cpp
hardware/Razberry.cpp
hardware/Rego6XXSerial.cpp
hardware/DavisLoggerSerial.cpp
hardware/BMP085.cpp
hardware/S0MeterSerial.cpp
hardware/S0MeterBase.cpp
hardware/smbus.cpp
hardware/Wunderground.cpp
hardware/OpenZWave.cpp
hardware/PiFace.cpp
hardware/OTGWBase.cpp
hardware/OTGWSerial.cpp
hardware/OTGWTCP.cpp
hardware/ZWaveBase.cpp
tcpserver/TCPServer.cpp
tcpserver/TCPClient.cpp
webserver/cWebem.cpp
webserver/Base64.cpp
webserver/connection.cpp
webserver/connection_manager.cpp
webserver/mime_types.cpp
webserver/reply.cpp
webserver/request_handler.cpp
webserver/request_parser.cpp
webserver/server.cpp
json/json_writer.cpp
json/json_reader.cpp
json/json_value.cpp
)

if(WIN32)
set(
domoticz_SRCS
${domoticz_SRCS}
main/WindowsHelper.cpp
)
endif(WIN32)

set(
domoticz_HEADERS
main/appversion.h
main/Camera.h
main/CmdLine.h
main/dirent_windows.h
main/EventSystem.h
main/Helper.h
main/localtime_r.h
main/Logger.h
main/mainstructs.h
main/mainworker.h
main/resource.h
main/RFXNames.h
main/RFXtrx.h
main/Scheduler.h
main/SQLHelper.h
main/stdafx.h
main/SunRiseSet.h
main/targetver.h
main/WebServer.h
main/WindCalculation.h
main/WindowsHelper.h
hardware/1Wire.h
hardware/ASyncSerial.h
hardware/BMP085.h
hardware/DavisLoggerSerial.h
hardware/DomoticzHardware.h
hardware/DomoticzTCP.h
hardware/Dummy.h
hardware/hardwaretypes.h
hardware/hidapi.h
hardware/libusbwinusbbridge.h
hardware/Limitless.h
hardware/MCP23x17.h
hardware/OpenZWave.h
hardware/OTGWBase.h
hardware/OTGWSerial.h
hardware/OTGWTCP.h
hardware/P1MeterBase.h
hardware/P1MeterSerial.h
hardware/P1MeterTCP.h
hardware/PiFace.h
hardware/Razberry.h
hardware/Rego6XXSerial.h
hardware/RFXComSerial.h
hardware/RFXComTCP.h
hardware/S0MeterBase.h
hardware/S0MeterSerial.h
hardware/S0MeterTCP.h
hardware/smbus.h
hardware/TE923.h
hardware/TE923Tool.h
hardware/Teleinfo.h
hardware/VolcraftCO20.h
hardware/VolcraftCO20Tool.h
hardware/Wunderground.h
hardware/YouLess.h
hardware/ZWaveBase.h
hardware/ZWaveCommands.h
)
IF(MSVC)
add_executable(domoticz WIN32 ${domoticz_SRCS} ${domoticz_HEADERS})
else()
add_executable(domoticz ${domoticz_SRCS} ${domoticz_HEADERS})
endif(MSVC)

# explicitly say that the executable depends on the revisiontag
add_dependencies(domoticz revisiontag)

IF(MSVC)
    SET_TARGET_PROPERTIES(lua PROPERTIES
            COMPILE_DEFINITIONS "_CRT_SECURE_NO_WARNINGS")
    SET_TARGET_PROPERTIES(lua PROPERTIES
            COMPILE_FLAGS "/wd4996 /nologo")

    SET_TARGET_PROPERTIES(domoticz PROPERTIES
            COMPILE_DEFINITIONS "_CRT_SECURE_NO_WARNINGS")
    SET_TARGET_PROPERTIES(domoticz PROPERTIES
            COMPILE_FLAGS "/wd4996 /nologo")
ENDIF(MSVC)

## Link libraries
#set(BOOST_USE_STATIC_LIBS ON)
set(BOOST_LIBS thread date_time system)
find_package(Boost COMPONENTS ${BOOST_LIBS} REQUIRED)
IF (Boost_FOUND)
    INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIR})
    ADD_DEFINITIONS( "-DHAS_BOOST" )
ENDIF()

FIND_PACKAGE(CURL)
IF(CURL_FOUND)
  MESSAGE(STATUS "Curl libraries found at: ${CURL_LIBRARIES}")
  MESSAGE(STATUS "Curl includes found at: ${CURL_INCLUDE_DIRS}")
  INCLUDE_DIRECTORIES(${CURL_INCLUDE_DIRS})
else()
  MESSAGE(FATAL_ERROR "Could not find cURL on your system, see install.txt how to get them installed. (for example 'sudo apt-get install curl libcurl4-openssl-dev')")
ENDIF(CURL_FOUND)

find_path(LIBUSB_INCLUDE_DIR usb.h
   HINTS ${PC_LIBUSB_INCLUDEDIR} ${PC_LIBUSB_INCLUDE_DIRS})
find_library(LIBUSB_LIBRARY NAMES usb
   HINTS ${PC_LIBUSB_LIBDIR} ${PC_LIBUSB_LIBRARY_DIRS})
set(LIBUSB_LIBRARIES ${LIBUSB_LIBRARY})

find_package_handle_standard_args(LIBUSB  DEFAULT_MSG  LIBUSB_LIBRARIES LIBUSB_INCLUDE_DIR)
IF(LIBUSB_FOUND)
  MESSAGE(STATUS "LIBUSB found at: ${LIBUSB_LIBRARIES}")
else()
  MESSAGE(FATAL_ERROR "Could not find LIBUSB on your system, see install.txt how to get them installed.")
ENDIF(LIBUSB_FOUND)

find_path(OPENZWAVE_INCLUDE_DIR Manager.h)
find_library(OPENZWAVE_LIBRARY NAMES libopenzwave.a OpenZWave.lib HINTS "../open-zwave-read-only/cpp/lib/linux" "../open-zwave-read-only/cpp/lib/mac")
IF(OPENZWAVE_LIBRARY)
  message(STATUS ${OPENZWAVE_LIBRARY})
  target_link_libraries(domoticz ${OPENZWAVE_LIBRARY})
  INCLUDE_DIRECTORIES(${OPENZWAVE_INCLUDE_DIR})
else()
  MESSAGE(FATAL_ERROR "Could not find OpenZWave on your system, see install.txt how to get it installed.")
ENDIF(OPENZWAVE_LIBRARY)

IF(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
  set(IOKIT_LIBRARY "-framework IOKit -framework CoreFoundation" CACHE FILEPATH "IOKit framework" FORCE)
  target_link_libraries(domoticz ${IOKIT_LIBRARY})
elseif("WIN32")
  # No additional requirements for notification system on Windows
else()
  find_library(UDEV NAMES libudev.a)
  IF(UDEV)
    message(STATUS ${UDEV})
    target_link_libraries(domoticz ${UDEV})
  else()
    find_library(UDEV NAMES libudev.so)
    IF(UDEV)
      message(STATUS ${UDEV})
      target_link_libraries(domoticz ${UDEV})
    else()
      MESSAGE(FATAL_ERROR "Could not find LIB UDEV on your system, see install.txt how to get them installed.\nsudo apt-get install libudev-dev")
    ENDIF(UDEV)
  ENDIF(UDEV)
ENDIF()

TARGET_LINK_LIBRARIES(domoticz ${Boost_LIBRARIES} ${SQLITE3_LIBRARY} ${LIBUSB_LIBRARIES} ${OPENZWAVE_LIBRARY} lua)

IF(MSVC)
TARGET_LINK_LIBRARIES(domoticz ${CURL_LIBRARIES})
ELSE()
TARGET_LINK_LIBRARIES(domoticz pthread curl)
ENDIF(MSVC)

ADD_PRECOMPILED_HEADER(domoticz "stdafx.h")

# If it wasn't overridden from command line / cache.
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
	set(CMAKE_INSTALL_PREFIX "/opt/${CPACK_PACKAGE_NAME}" CACHE PATH "Install path prefix, prepended onto install directories." FORCE)
endif(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)

install(TARGETS domoticz DESTINATION /opt/domoticz)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/www DESTINATION /opt/domoticz USE_SOURCE_PERMISSIONS)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/License.txt DESTINATION /opt/domoticz)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/History.txt DESTINATION /opt/domoticz)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/svnversion.h DESTINATION /opt/domoticz)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/scripts DESTINATION /opt/domoticz USE_SOURCE_PERMISSIONS)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/Config DESTINATION /opt/domoticz USE_SOURCE_PERMISSIONS)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/updatedomo DESTINATION /opt/domoticz PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ GROUP_EXECUTE GROUP_READ)

# build a CPack driven installer package
INCLUDE(InstallRequiredSystemLibraries)

SET(CPACK_GENERATOR "DEB")
SET(CPACK_PACKAGE_NAME "domoticz")
SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY "${domoticz}-A program for home automation (Domotica).")
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
SET(CPACK_PACKAGE_EXECUTABLES "domoticz;A program for home automation (Domotica).")

#set(CPACK_DEB_COMPONENT_INSTALL TRUE)
SET(CPACK_DEBIAN_ARCHITECTURE ${CMAKE_SYSTEM_PROCESSOR})

INCLUDE(CPack)

