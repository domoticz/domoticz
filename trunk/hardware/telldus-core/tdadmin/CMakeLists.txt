PROJECT(tdadmin)

cmake_policy(SET CMP0005 NEW)

SET (tdadmin_DESCRIPTION
	"a command line utility to edit devices and controllers for Telldus TellStick"
)

SET(tdadmin_SRCS
	main.cpp
)

ADD_EXECUTABLE(tdadmin
	${tdadmin_SRCS}
)

INCLUDE_DIRECTORIES(
	${CMAKE_CURRENT_BINARY_DIR}
	${CMAKE_SOURCE_DIR}/driver
)

ADD_DEFINITIONS( -DVERSION="${DISPLAYED_VERSION}" )

IF (WIN32)
	FIND_LIBRARY(TELLDUSCORE_LIBRARY TelldusCore)
	TARGET_LINK_LIBRARIES(tdadmin
		${TELLDUSCORE_LIBRARY}
	)
ELSEIF (APPLE)
	TARGET_LINK_LIBRARIES(tdadmin
		TelldusCore
	)
ELSEIF (CMAKE_SYSTEM_NAME MATCHES "FreeBSD")
	# FreeBSD does not have argp in base libc; port devel/argp-standalone is required.
	FIND_LIBRARY(ARGP_LIBRARY argp)
	TARGET_LINK_LIBRARIES(tdadmin
		${CMAKE_BINARY_DIR}/client/libtelldus-core.so
		${ARGP_LIBRARY}
	)
ELSE (WIN32)
	TARGET_LINK_LIBRARIES(tdadmin
		${CMAKE_BINARY_DIR}/client/libtelldus-core.so
	)
ENDIF (WIN32)

IF (UNIX)
	IF (GENERATE_MAN)
		ADD_CUSTOM_COMMAND(
			TARGET tdadmin
			POST_BUILD
			COMMAND help2man -n ${tdadmin_DESCRIPTION} ./tdadmin > tdadmin.1
			WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
			COMMENT "Generating man file tdadmin.1"
		)
		INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/tdadmin.1 DESTINATION share/man/man1)
	ENDIF (GENERATE_MAN)
ENDIF (UNIX)

INSTALL(TARGETS tdadmin RUNTIME DESTINATION sbin)

IF (UNIX AND NOT APPLE)
	SET(UDEV_RULES_DIR	"/etc/udev/rules.d" CACHE PATH "The directory where udev store its rules" )
	CONFIGURE_FILE(
		${CMAKE_CURRENT_SOURCE_DIR}/05-tellstick.rules
		${CMAKE_BINARY_DIR}/parsed/05-tellstick.rules
		@ONLY
	)
	CONFIGURE_FILE(
		${CMAKE_CURRENT_SOURCE_DIR}/udev.sh
		${CMAKE_BINARY_DIR}/parsed/udev.sh
		@ONLY
	)
	INSTALL(FILES ${CMAKE_BINARY_DIR}/parsed/05-tellstick.rules
		DESTINATION ${UDEV_RULES_DIR}
	)
	INSTALL(PROGRAMS ${CMAKE_BINARY_DIR}/parsed/udev.sh
		DESTINATION share/telldus-core/helpers/
	)
ENDIF (UNIX AND NOT APPLE)
