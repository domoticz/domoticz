set (blebox_files
  ../../../hardware/blebox/db/annotate.cpp
  ../../../hardware/blebox/db/device_status.cpp
  ../../../hardware/blebox/db/hardware.cpp
  ../../../hardware/blebox/box.cpp
  ../../../hardware/blebox/session.cpp
  ../../../hardware/blebox/errors.cpp
  ../../../hardware/blebox/rfx.cpp
  ../../../hardware/blebox/version.cpp
  ../../../hardware/blebox/widgets.cpp
  ../../../hardware/blebox/widgets/widget.cpp
  ../../../hardware/blebox/products.cpp
  )

set (blebox_tests_files
  mocks.cpp

  box_test.cpp
  version_test.cpp
  db/migration_test.cpp

  BleBox_test.cpp

  products/wLightBoxS_test.cpp
  products/fixtures/wLightBoxS_fixtures.cpp

  products/switchBoxD_test.cpp
  products/fixtures/switchBoxD_fixtures.cpp

  products/shutterBox_test.cpp
  products/fixtures/shutterBox_fixtures.cpp

  products/wLightBox_test.cpp
  products/fixtures/wLightBox_fixtures.cpp

  products/gateController_test.cpp
  products/fixtures/gateController_fixtures.cpp

  products/tempSensor_test.cpp
  products/fixtures/tempSensor_fixtures.cpp

  products/airSensor_test.cpp
  products/fixtures/airSensor_fixtures.cpp

  products/saunaBox_test.cpp
  products/fixtures/saunaBox_fixtures.cpp

  products/switchBox_test.cpp
  products/fixtures/switchBox_fixtures.cpp

  products/dimmerBox_test.cpp
  products/fixtures/dimmerBox_fixtures.cpp

  products/gateBox_test.cpp
  products/fixtures/gateBox_fixtures.cpp

  db/hardware_test.cpp
  )

if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  set(max_warnings
    -Weverything
    -Wno-system-headers
    -Wno-c++98-compat
    -Wno-c++98-compat-pedantic
    -Wno-padded
    -Wno-global-constructors
    -Wno-exit-time-destructors

    # domoticz headers
    -Wno-zero-as-null-pointer-constant
    -Wno-reserved-id-macro
    -Wno-extra-semi
    -Wno-float-conversion
    -Wno-weak-vtables
    -Wno-documentation-unknown-command

    -Werror

    # SQLHelper
    -Wno-sign-conversion

    # cereal
    -Wno-extra-semi-stmt

    # TCPServer.h
    -Wno-shadow-field

    )

  set(max_test_warnings ${max_warnings}
    # Google Mock
    -Wno-gnu-zero-variadic-macro-arguments
    )
endif()

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  set(max_warnings
    -Wall
    -Wextra
    -Wno-unused-parameter
    -Wno-missing-field-initializers
    -Wno-parentheses
    )
  set(max_test_warnings ${max_warnings})
endif()

add_library(blebox_tested OBJECT ${blebox_files})
target_compile_options(blebox_tested PRIVATE ${max_warnings})
target_compile_features(blebox_tested PRIVATE cxx_std_17)
target_add_special_build_options(blebox_tested)
target_include_directories(blebox_tested PRIVATE SYSTEM
  ${CMAKE_CURRENT_LIST_DIR}/../../..
  ${CMAKE_CURRENT_LIST_DIR}/../../../json
  ${CMAKE_CURRENT_LIST_DIR}/../../../tinyxpath
  ${CMAKE_CURRENT_LIST_DIR}/../../../httpclient
  ${CMAKE_CURRENT_LIST_DIR}/../../../zip
)

add_library(blebox_tests OBJECT ${blebox_tests_files})
target_compile_options(blebox_tests PRIVATE ${max_test_warnings})
target_compile_features(blebox_tests PRIVATE cxx_std_17)
target_add_special_build_options(blebox_tests)
target_include_directories(blebox_tests PRIVATE SYSTEM
  ${CMAKE_CURRENT_LIST_DIR}/../../..
  ${CMAKE_CURRENT_LIST_DIR}/../../../json
  ${CMAKE_CURRENT_LIST_DIR}/../../../tinyxpath
  ${CMAKE_CURRENT_LIST_DIR}/../../../httpclient
  ${CMAKE_CURRENT_LIST_DIR}/../../../zip
)
