IF(COMMAND cmake_policy)
	cmake_policy(SET CMP0003 NEW)
ENDIF(COMMAND cmake_policy)

FIND_PACKAGE( SignTool REQUIRED )

######## Non configurable options  ########
SET( telldus-core_SRCS
	CallbackDispatcher.cpp
	CallbackMainDispatcher.cpp
	Client.cpp
	telldus-core.cpp
)

SET( telldus-core_HDRS
	CallbackDispatcher.h
	CallbackMainDispatcher.cpp
	Client.h
)
SET( telldus-core_PUB_HDRS
	telldus-core.h
)

FIND_PACKAGE(Threads)
LIST(APPEND telldus-core_LIBRARIES ${CMAKE_THREAD_LIBS_INIT})

INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR})

######## Configurable options for the platform  ########



######## Platforms-specific, non configurable  ########

IF (APPLE)
	#### Mac OS X ####
	SET( telldus-core_TARGET	TelldusCore )
	ADD_DEFINITIONS(
		-D_MACOSX
	)
	LIST(APPEND telldus-core_LIBRARIES
		TelldusCommon
	)
ELSEIF (WIN32)
	#### Windows ####
	ADD_DEFINITIONS( -DUNICODE )
	ADD_DEFINITIONS( /Zc:wchar_t- ) # Treat wchar_t as Built-in Type' = No
	SET( telldus-core_TARGET	TelldusCore )
	LIST(APPEND telldus-core_LIBRARIES
		TelldusCommon
	)
	CONFIGURE_FILE(
		${CMAKE_CURRENT_SOURCE_DIR}/telldus-core.rc.in
		${CMAKE_CURRENT_BINARY_DIR}/telldus-core.rc
	)
	LIST(APPEND telldus-core_SRCS
			libtelldus-core.def
			${CMAKE_CURRENT_SOURCE_DIR}/telldus-core.rc.in
			${CMAKE_CURRENT_BINARY_DIR}/telldus-core.rc
	)
	ADD_DEFINITIONS(
		-D_WINDOWS
		-DTELLDUSCORE_EXPORTS
	)
	IF (CMAKE_CL_64)
		ADD_DEFINITIONS(-D_CL64)
	ENDIF(CMAKE_CL_64)
ELSE (APPLE)
	#### Linux ####
	SET( telldus-core_TARGET	telldus-core )
	LIST(APPEND telldus-core_LIBRARIES
		TelldusCommon
	)

	ADD_DEFINITIONS(
		-D_LINUX
	)
ENDIF (APPLE)



######## Configuring  ########

ADD_LIBRARY(${telldus-core_TARGET} SHARED
	${telldus-core_SRCS}
	${telldus-core_HDRS}
	${telldus-core_PUB_HDRS}
)

#Copy public headers files on windows
IF (WIN32)
	FOREACH(_FILE ${telldus-core_PUB_HDRS})
		ADD_CUSTOM_COMMAND( TARGET ${telldus-core_TARGET}
			POST_BUILD
			COMMAND ${CMAKE_COMMAND} -E copy \"${CMAKE_CURRENT_SOURCE_DIR}/${_FILE}\" \"${LIBRARY_OUTPUT_PATH}/${CMAKE_CFG_INTDIR}\"
			COMMENT "Copy ${_FILE}"
		)
	ENDFOREACH(_FILE)
ENDIF ()

ADD_DEPENDENCIES(${telldus-core_TARGET} TelldusCommon)

IF (UNIX)
	SET_TARGET_PROPERTIES( ${telldus-core_TARGET} PROPERTIES COMPILE_FLAGS "-fPIC -fvisibility=hidden")
ENDIF (UNIX)

TARGET_LINK_LIBRARIES( ${telldus-core_TARGET}	${telldus-core_LIBRARIES} )

SET(telldus-core_TARGET ${telldus-core_TARGET} PARENT_SCOPE)
SET_TARGET_PROPERTIES(${telldus-core_TARGET} PROPERTIES
	FRAMEWORK TRUE
	INSTALL_NAME_DIR "/Library/Frameworks"
	PUBLIC_HEADER ${telldus-core_PUB_HDRS}
	VERSION ${PACKAGE_VERSION}
	SOVERSION ${PACKAGE_SOVERSION}
)
SIGN(${telldus-core_TARGET})

IF (NOT LIB_INSTALL_DIR)
	SET(LIB_INSTALL_DIR "lib")
ENDIF (NOT LIB_INSTALL_DIR)

IF (UNIX)
	INSTALL(TARGETS ${telldus-core_TARGET}
		LIBRARY         DESTINATION   ${LIB_INSTALL_DIR}
		ARCHIVE         DESTINATION   ${LIB_INSTALL_DIR}
		FRAMEWORK       DESTINATION   "/Library/Frameworks"
		PUBLIC_HEADER   DESTINATION   include
	)
ENDIF (UNIX)

