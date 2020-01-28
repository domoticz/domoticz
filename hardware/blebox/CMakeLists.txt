# BleBox devices integration

# {{{ -------------------core library --------------------------------

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

# stuff common with domoticz and unit tests
set( blebox_SRCS
  db/annotate.cpp
  db/device_status.cpp
  db/hardware.cpp
  errors.cpp
  box.cpp
  rfx.cpp
  version.cpp
  products.cpp
  widgets.cpp
  widgets/widget.cpp
  )

add_library(blebox OBJECT ${blebox_SRCS})
add_definitions(-DWEBSERVER_DONT_USE_ZIP)

# for cereal
target_include_directories(blebox PRIVATE ${CMAKE_CURRENT_LIST_DIR}/../..)

# TODO: remove this workaround (for unzip.h header path)
option(USE_BUILTIN_MINIZIP "Use builtin minizip library" YES)
if(USE_BUILTIN_MINIZIP)
  set(MINIZIP_INCLUDE_DIRS "${CMAKE_SOURCE_DIR}/zip")
else(USE_BUILTIN_MINIZIP)
  find_package(PkgConfig)
  pkg_check_modules(MINIZIP minizip)
  if(MINIZIP_FOUND)
    MESSAGE(STATUS "MINIZIP library found at: ${MINIZIP_LIBRARIES}")
    MESSAGE(STATUS "MINIZIP includes found at: ${MINIZIP_INCLUDE_DIRS}")
  else()
    MESSAGE(FATAL_ERROR "MINIZIP not found on your system, try to get this installed. (for example 'sudo apt-get install libminizip-dev')")
  endif(MINIZIP_FOUND)
endif(USE_BUILTIN_MINIZIP)

target_include_directories(blebox PRIVATE ${MINIZIP_INCLUDE_DIRS})

if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  target_compile_options(blebox PRIVATE
    -Weverything
    -Werror
    -Wno-system-headers
    -Wno-c++98-compat
    -Wno-c++98-compat-pedantic
    -Wno-padded
    -Wno-documentation-unknown-command
    -Wno-global-constructors

    # warnings from Domoticz sources:
    -Wno-zero-as-null-pointer-constant
    -Wno-extra-semi
    -Wno-extra-semi-stmt
    -Wno-reserved-id-macro
    -Wno-weak-vtables
    -Wno-double-promotion
    -Wno-float-conversion
    -Wno-exit-time-destructors

    # cereal
    -Wno-sign-conversion
    )
endif()

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  target_compile_options(blebox PRIVATE
    -Wall
    -Wextra
    -Wno-unused-parameter
    -Wno-missing-field-initializers
    #-Wno-parentheses
    )
endif()

# ok, since even gcc-6 supports half of cxx17 features
target_compile_features(blebox PRIVATE cxx_std_17)

if(USE_PRECOMPILED_HEADER)
  target_include_directories(blebox PRIVATE ${CMAKE_CURRENT_LIST_DIR})
  target_precompile_headers(blebox
    PRIVATE
    "$<$<COMPILE_LANGUAGE:C>:precompiled.h>"
    "$<$<COMPILE_LANGUAGE:CXX>:precompiled.hxx>"
    )
endif()

# }}}
# {{{ -----------implentation to meet domoticz requirements------------

# stuff requiring domoticz to link
set( blebox_hardware_SRCS session.cpp web.cpp ../BleBox.cpp)

add_library(blebox_hardware OBJECT ${blebox_hardware_SRCS})

target_include_directories(blebox_hardware PRIVATE ${MINIZIP_INCLUDE_DIRS})

# for cereal
target_include_directories(blebox_hardware PRIVATE ${CMAKE_CURRENT_LIST_DIR}/../..)

if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  target_compile_options(blebox_hardware PRIVATE
    -Weverything
    -Wno-system-headers
    -Wno-c++98-compat
    -Wno-c++98-compat-pedantic
    -Wno-padded
    -Wno-documentation-unknown-command
    -Wno-global-constructors
    -Wno-zero-as-null-pointer-constant
    -Wno-extra-semi
    -Wno-extra-semi-stmt
    -Wno-reserved-id-macro
    -Wno-weak-vtables
    -Wno-double-promotion
    -Wno-float-conversion
    -Wno-exit-time-destructors

    # domoticz sources
    -Wno-old-style-cast
    -Wno-non-virtual-dtor
    -Wno-inconsistent-missing-destructor-override
    )
endif()

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  target_compile_options(blebox_hardware PRIVATE
    -Wall
    -Wextra
    -Wno-unused-parameter
    -Wno-missing-field-initializers
    #-Wno-parentheses
    )
endif()

if(USE_PRECOMPILED_HEADER)
  target_include_directories(blebox_hardware PRIVATE ${CMAKE_CURRENT_LIST_DIR})
  target_precompile_headers(blebox_hardware
    PRIVATE
    "$<$<COMPILE_LANGUAGE:C>:precompiled.h>"
    "$<$<COMPILE_LANGUAGE:CXX>:precompiled.hxx>"
    )
endif()

# ok, since even gcc-6 supports half of cxx17 features
target_compile_features(blebox_hardware PRIVATE cxx_std_17)
# }}}
