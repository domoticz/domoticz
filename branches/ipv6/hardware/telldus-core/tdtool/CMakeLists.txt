PROJECT(tdtool)

cmake_policy(SET CMP0005 NEW)

SET (tdtool_DESCRIPTION
	"a command line utility to send commands to a Telldus TellStick"
)

SET(tdtool_SRCS
	main.cpp
)

ADD_EXECUTABLE(tdtool
	${tdtool_SRCS}
)
SIGN(tdtool)

INCLUDE_DIRECTORIES(
	${CMAKE_CURRENT_BINARY_DIR}
	${CMAKE_SOURCE_DIR}/driver
)

ADD_DEFINITIONS( -DVERSION="${DISPLAYED_VERSION}" )

IF (WIN32)
	TARGET_LINK_LIBRARIES(tdtool
		TelldusCore
		openbsd-getopt
	)
	INCLUDE_DIRECTORIES(
		${CMAKE_SOURCE_DIR}/3rdparty/openbsd-getopt
	)
ELSEIF (APPLE)
	TARGET_LINK_LIBRARIES(tdtool
		TelldusCore
	)
ELSE (WIN32)
	TARGET_LINK_LIBRARIES(tdtool
		${CMAKE_BINARY_DIR}/client/libtelldus-core.so
	)
ENDIF (WIN32)

IF (UNIX)
	IF (GENERATE_MAN)
		ADD_CUSTOM_COMMAND(
			TARGET tdtool
			POST_BUILD
			COMMAND help2man -n ${tdtool_DESCRIPTION} ./tdtool > tdtool.1
			WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
			COMMENT "Generating man file tdtool.1"
		)
		INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/tdtool.1 DESTINATION share/man/man1)
	ENDIF (GENERATE_MAN)
ENDIF (UNIX)

INSTALL(TARGETS tdtool RUNTIME DESTINATION bin)
