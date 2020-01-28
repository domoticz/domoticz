cmake_minimum_required(VERSION 3.16.0)
project(domoticz_tests)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

# {{{ GoogleTest + GoogleMock stuff
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/extern/googletest")

mark_as_advanced(
  BUILD_GMOCK BUILD_GTEST BUILD_SHARED_LIBS
  gmock_build_tests gtest_build_samples gtest_build_tests
  gtest_disable_pthreads gtest_force_shared_crt gtest_hide_internal_symbols
  )

set_target_properties(gtest PROPERTIES FOLDER extern)
set_target_properties(gtest_main PROPERTIES FOLDER extern)
set_target_properties(gmock PROPERTIES FOLDER extern)
set_target_properties(gmock_main PROPERTIES FOLDER extern)

# TODO: CMake seems to silently drop the -isystem flags otherwise
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -isystem ${CMAKE_CURRENT_LIST_DIR}/extern/googletest/googlemock/include/")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -isystem ${CMAKE_CURRENT_LIST_DIR}/extern/googletest/googletest/include/")
# }}}
# {{{ test-specific project defines
add_definitions(-DUNIT_TESTING)
add_definitions(-DNOCLOUD)
add_definitions(-DWEBSERVER_DONT_USE_ZIP)
# }}}
# {{{ special build options
option(ADDRESS_SANITIZER "Turn on Clang AddressSanitizer" OFF)
if(ADDRESS_SANITIZER)
  message(STATUS "Clang AddressSanitizer enabled")
endif()

option(UNDEFINED_SANITIZER "Turn on Clang UndefinedBehaviorSanitizer" OFF)
if(UNDEFINED_SANITIZER)
  message(STATUS "Clang UndefinedSanitizer enabled")
endif()

option(MEMORY_SANITIZER "Turn on Clang MemorySanitizer" OFF)
if(UNDEFINED_SANITIZER)
  message(STATUS "Clang UndefinedSanitizer enabled")
endif()

option(CLANG_CODE_COVERAGE "Enable code coverage metrics in Clang" OFF)
if (CLANG_CODE_COVERAGE)
  message(STATUS "Code coverage enabled")
endif()

macro( target_add_special_build_options TESTNAME)
  if(USE_PRECOMPILED_HEADER)
    target_include_directories(${TESTNAME} PUBLIC ${CMAKE_CURRENT_LIST_DIR})
    target_precompile_headers(${TESTNAME}
      PRIVATE
      "$<$<COMPILE_LANGUAGE:C>:precompiled.h>"
      "$<$<COMPILE_LANGUAGE:CXX>:precompiled.hxx>"
    )
  endif()

  if(ADDRESS_SANITIZER)
    target_compile_options(${TESTNAME} PRIVATE -O1 -fno-omit-frame-pointer -fsanitize=address)
    target_link_options(${TESTNAME} PRIVATE -fsanitize=address)
  endif()

  if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    if(MEMORY_SANITIZER)
      target_compile_options(${TESTNAME} PRIVATE -O1 -fno-omit-frame-pointer -fsanitize=memory -fno-optimize-sibling-calls -fsanitize-memory-track-origins=2)
      target_link_options(${TESTNAME} PRIVATE -fsanitize=memory)
    endif()
  endif()

  if(UNDEFINED_SANITIZER)
    target_compile_options(${TESTNAME} PRIVATE -fsanitize=undefined -fsanitize=integer)
    target_link_options(${TESTNAME} PRIVATE -fsanitize=undefined -fsanitize=integer)
  endif()

endmacro()

if (CLANG_CODE_COVERAGE)
  if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -fprofile-instr-generate -fcoverage-mapping")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fprofile-instr-generate -fcoverage-mapping")
  endif()
endif()

# }}}
# {{{ blebox tests and library (subdir)
add_subdirectory(hardware/blebox)
# }}}
set(
  domoticz_test_sources

  stubs.cpp
  test_helper.cpp
  worker_helper.cpp
  testdb.cpp
  memdb.cpp
  ../hardware/BleBox.cpp

  # minimal set of deps from main app
  ../hardware/ColorSwitch.cpp
  ../json/json_reader.cpp
  ../json/json_value.cpp
  ../json/json_writer.cpp
  ../main/json_helper.cpp
  ../main/Helper.cpp
  ../main/RFXNames.cpp
  ../main/localtime_r.cpp
  ../main/mainworker.cpp
  ../main/SQLHelper.cpp
  ../webserver/Base64.cpp
  ../httpclient/UrlEncode.cpp
  ../main/TrendCalculator.cpp
  ../main/Logger.cpp
)

add_executable(testDomoticz
  ${domoticz_test_sources}
  $<TARGET_OBJECTS:blebox_tested>
  $<TARGET_OBJECTS:blebox_tests>
  )
target_compile_features(testDomoticz PRIVATE cxx_std_17)

if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  target_compile_options(testDomoticz PRIVATE
    -Weverything
    -Wno-system-headers
    -Wno-c++98-compat
    -Wno-c++98-compat-pedantic
    -Wno-padded
    -Werror

    # hide the public/private hack above
    -Wno-keyword-macro

    # domoticz headers and sources:
    -Wno-global-constructors
    -Wno-exit-time-destructors
    -Wno-zero-as-null-pointer-constant
    -Wno-extra-semi
    -Wno-extra-semi-stmt
    -Wno-reserved-id-macro
    -Wno-documentation-unknown-command
    -Wno-weak-vtables
    -Wno-double-promotion
    -Wno-float-conversion
    -Wno-old-style-cast
    -Wno-switch-enum
    -Wno-implicit-fallthrough
    -Wno-implicit-int-conversion
    -Wno-sign-conversion
    -Wno-deprecated
    -Wno-disabled-macro-expansion
    -Wno-missing-variable-declarations
    -Wno-unused-macros
    -Wno-inconsistent-missing-destructor-override
    -Wno-shadow
    -Wno-format-nonliteral
    -Wno-unused
    -Wno-unused-parameter
    -Wno-unreachable-code-break
    -Wno-float-equal
    -Wno-shorten-64-to-32
    -Wno-cast-align
    -Wno-cast-qual
    -Wno-non-virtual-dtor
    -Wno-missing-field-initializers
    -Wno-missing-prototypes
    -Wno-missing-noreturn
    -Wno-covered-switch-default
    -Wno-undef
    -Wno-implicit-float-conversion

    # TCPServer.h
    -Wno-shadow-field

    # stdafx.h
    -Wno-macro-redefined

    # mainworker.cpp
    -Wno-switch
    -Wno-tautological-unsigned-zero-compare

    # HEOS.h
    -Wno-newline-eof

    # SQLHelper
    -Wno-range-loop-analysis
    -Wno-overloaded-virtual

    # ASyncSerial.h
    -Wno-documentation

    #mainworker.cpp
    -Wno-unreachable-code

    #UrlEncode.cpp
    -Wno-unreachable-code-return
    )
endif()

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  target_compile_options(testDomoticz PRIVATE
    -Wall
    -Wextra
    -Wno-unused-parameter
    -Wno-missing-field-initializers
    -Wno-parentheses
    -Wno-implicit-fallthrough
    )
endif()

target_link_libraries(testDomoticz gtest gmock gtest_main)

include(cereal)
libcereal(testDomoticz) # headers only

include(ssl)
libssl(testDomoticz)

include(boost)
libboost(testDomoticz)

include(sqlite)
libsqlite(testDomoticz)

include(lua)
liblua(testDomoticz)

include(minizip)
libminizip(testDomoticz)

include_directories(${CMAKE_CURRENT_LIST_DIR}/../main)
include_directories(${CMAKE_CURRENT_LIST_DIR}/..)

# only headers are needed (due to stubs)
include_directories(${CMAKE_CURRENT_LIST_DIR}/../MQTT)

# TODO: check if this works when precompiled headers are disabled
set_target_properties(testDomoticz PROPERTIES FOLDER tests)

add_test(NAME testDomoticz COMMAND testDomoticz)
target_add_special_build_options(testDomoticz)
# }}} // test target
