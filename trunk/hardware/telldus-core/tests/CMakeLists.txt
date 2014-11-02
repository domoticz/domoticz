SET(ENABLE_TESTING	FALSE	CACHE BOOL "Enable unit tests")

#We have disabled some of the default tests in the google style guidelines
# whitespace/tab
#  We are using tabs for indentation, not spaces, in our code
#
# whitespace/parens
#  We believe grouping of parameters with spaces in some functions could ease the readability
#
# whitespace/line_length
#  Although you should try to keep the lines short it should not be a requirement (at least for now)
#
# whitespace/labels
#  Since we use tabs instead of spaces for indentation, this test makes no sense
#
# runtime/rtti
#  We are using dynamic_cast for the events. We use this to be able to send arbitrary data
#  trought the events.
#
SET(cpplint_filters
	+whitespace/use_tab_for_indentation,-whitespace/tab,-whitespace/parens,-whitespace/line_length,-whitespace/labels,-runtime/rtti
)

FUNCTION(ADD_SOURCES TARGET PATH)
	GET_TARGET_PROPERTY(SOURCES ${TARGET} SOURCES)
	FOREACH(SOURCE ${SOURCES})
		LIST(APPEND L ${PATH}/${SOURCE})
	ENDFOREACH()
	ADD_TEST(StyleGuidelines-${TARGET} ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/cpplint.py --filter=${cpplint_filters} ${L})
ENDFUNCTION()

IF(ENABLE_TESTING)
	INCLUDE(FindPythonInterp)
	FIND_LIBRARY(CPPUNIT cppunit)
	ADD_SUBDIRECTORY(common)
	ADD_SUBDIRECTORY(service)

	ADD_EXECUTABLE(TestRunner cppunit.cpp)
	TARGET_LINK_LIBRARIES(TestRunner cppunit TelldusCommonTests TelldusServiceTests)
	ADD_DEPENDENCIES(TestRunner TelldusCommonTests TelldusServiceTests)

	ADD_SOURCES(TelldusCommon ${CMAKE_SOURCE_DIR}/common)
	ADD_SOURCES(${telldus-core_TARGET} ${CMAKE_SOURCE_DIR}/client)
	ADD_SOURCES(${telldus-service_TARGET} ${CMAKE_SOURCE_DIR}/service)

	ADD_TEST(cppunit ${CMAKE_CURRENT_BINARY_DIR}/TestRunner)
	IF (UNIX)
		ADD_TEST(cppcheck cppcheck --quiet --error-exitcode=2 ${CMAKE_SOURCE_DIR})
	ENDIF()
ENDIF()

