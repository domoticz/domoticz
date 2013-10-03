IF(COMMAND cmake_policy)
	CMAKE_POLICY(SET CMP0003 NEW)
ENDIF(COMMAND cmake_policy)

FIND_PACKAGE( SignTool REQUIRED )

SET (telldusd_DESCRIPTION
	"background service for Telldus TellStick, must be running to control TellStick"
)

ADD_DEFINITIONS( -DVERSION="${DISPLAYED_VERSION}" )

######## Non configurable options  ########
SET( telldus-service_SRCS
	ClientCommunicationHandler.cpp
	Controller.cpp
	ControllerManager.cpp
	ControllerMessage.cpp
	Device.cpp
	DeviceManager.cpp
	Log.cpp
	Sensor.cpp
	Settings.cpp
	TelldusMain.cpp
	TellStick.cpp
	Timer.cpp
	EventUpdateManager.cpp
)
SET( telldus-service_protocol_SRCS
	Protocol.h
	Protocol.cpp
	ProtocolBrateck.h
	ProtocolBrateck.cpp
	ProtocolComen.h
	ProtocolComen.cpp
	ProtocolEverflourish.h
	ProtocolEverflourish.cpp
	ProtocolFineoffset.h
	ProtocolFineoffset.cpp
	ProtocolFuhaote.h
	ProtocolFuhaote.cpp
	ProtocolGroup.h
	ProtocolGroup.cpp
	ProtocolHasta.h
	ProtocolHasta.cpp
	ProtocolIkea.h
	ProtocolIkea.cpp
	ProtocolMandolyn.h
	ProtocolMandolyn.cpp
	ProtocolNexa.h
	ProtocolNexa.cpp
	ProtocolOregon.h
	ProtocolOregon.cpp
	ProtocolRisingSun.h
	ProtocolRisingSun.cpp
	ProtocolSartano.h
	ProtocolSartano.cpp
	ProtocolScene.h
	ProtocolScene.cpp
	ProtocolSilvanChip.h
	ProtocolSilvanChip.cpp
	ProtocolUpm.h
	ProtocolUpm.cpp
	ProtocolWaveman.h
	ProtocolWaveman.cpp
	ProtocolX10.h
	ProtocolX10.cpp
	ProtocolYidong.h
	ProtocolYidong.cpp
)
SET( telldus-service_HDRS
	ClientCommunicationHandler.h
	ConnectionListener.h
	Controller.h
	ControllerListener.h
	ControllerManager.h
	ControllerMessage.h
	Device.h
	DeviceManager.h
	EventUpdateManager.h
	Log.h
	Sensor.h
	Settings.h
	TelldusMain.h
	TellStick.h
	Timer.h
)
FIND_PACKAGE(Threads REQUIRED)
LIST(APPEND telldus-service_LIBRARIES ${CMAKE_THREAD_LIBS_INIT})

INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR})

######## Configurable options for the platform  ########

IF (UNIX AND NOT APPLE)
	SET(SCRIPT_PATH "/usr/local/share/telldus/scripts" CACHE PATH "The paths for scripts to execute on sensor events")
	SET(SYSCONF_INSTALL_DIR "/etc" CACHE PATH "The sysconfig install dir (default prefix/etc)" )
	IF (${CMAKE_SYSTEM_NAME} MATCHES "FreeBSD")
		SET(DEFAULT_STATE_INSTALL_DIR "/var/spool")
	ELSE ()
		SET(DEFAULT_STATE_INSTALL_DIR "/var/state")
	ENDIF ()
	SET(STATE_INSTALL_DIR "${DEFAULT_STATE_INSTALL_DIR}" CACHE PATH "The directory to store state information of the devices")

ENDIF()

######## Platforms-specific, non configurable  ########

IF (APPLE) #### Mac OS X ####
	SET(DEFAULT_FTDI_ENGINE "ftd2xx")
	SET( telldus-service_TARGET	TelldusService )
	ADD_DEFINITIONS( -D_MACOSX )
	FIND_LIBRARY(COREFOUNDATION_LIBRARY CoreFoundation)
	FIND_LIBRARY(IOKIT_LIBRARY IOKit)

	SET( telldus-service_LIBRARIES
		${telldus-service_LIBRARIES}
		${COREFOUNDATION_LIBRARY}
		${IOKIT_LIBRARY}
		TelldusCommon
	)
	LIST(APPEND telldus-service_SRCS
		main_mac.cpp
		ConnectionListener_unix.cpp
		ControllerListener_mac.cpp
		SettingsCoreFoundationPreferences.cpp
	)

ELSEIF (WIN32) #### Windows ####
	SET(DEFAULT_FTDI_ENGINE "ftd2xx")
	SET( telldus-service_TARGET	TelldusService )
	ADD_DEFINITIONS( -DUNICODE )
	ADD_DEFINITIONS( /Zc:wchar_t- ) # Treat wchar_t as Built-in Type' = No
	SET(CMAKE_EXE_LINKER_FLAGS
		"${CMAKE_EXE_LINKER_FLAGS} /SUBSYSTEM:CONSOLE"
	)
	LIST(APPEND telldus-service_LIBRARIES
		TelldusCommon
	)
	LIST(APPEND telldus-service_SRCS
		ConnectionListener_win.cpp
		main_win.cpp
		SettingsWinRegistry.cpp
		TelldusWinService_win.cpp
		Messages.mc
		${CMAKE_CURRENT_BINARY_DIR}/Messages.rc
		${CMAKE_CURRENT_BINARY_DIR}/service/Messages.h
	)
	LIST(APPEND telldus-service_HDRS
		TelldusWinService_win.h
	)
	ADD_CUSTOM_COMMAND(
		OUTPUT Messages.rc service/Messages.h
		COMMAND mc.exe -u -r \"${CMAKE_CURRENT_BINARY_DIR}\" -h \"${CMAKE_CURRENT_BINARY_DIR}/service\" \"${CMAKE_CURRENT_SOURCE_DIR}/Messages.mc\"
		DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/Messages.mc
		DEPENDS Messages.rc
		COMMENT "Compiling Messages Resource"
	)
	INCLUDE_DIRECTORIES( ${CMAKE_CURRENT_BINARY_DIR} )

ELSE (APPLE) #### Linux ####
	SET(DEFAULT_FTDI_ENGINE "libftdi")
	FIND_LIBRARY(CONFUSE_LIBRARY confuse)
	ADD_DEFINITIONS( -D_CONFUSE )
	ADD_DEFINITIONS( -D_LINUX )

	SET( telldus-service_TARGET	telldusd )
	LIST(APPEND telldus-service_SRCS
		ConnectionListener_unix.cpp
		main_unix.cpp
		SettingsConfuse.cpp
	)

	LIST(APPEND telldus-service_LIBRARIES
		${CONFUSE_LIBRARY}
		TelldusCommon
	)
ENDIF (APPLE)

SET(FTDI_ENGINE ${DEFAULT_FTDI_ENGINE} CACHE STRING "Which FTDI engine to use. This could be either 'libftdi' or 'ftd2xx'")
IF (FTDI_ENGINE STREQUAL "ftd2xx")
	FIND_LIBRARY(FTD2XX_LIBRARY ftd2xx)
	ADD_DEFINITIONS( -DLIBFTD2XX )
	LIST(APPEND telldus-service_SRCS TellStick_ftd2xx.cpp )
	LIST(APPEND telldus-service_LIBRARIES ${FTD2XX_LIBRARY})
ELSE (FTDI_ENGINE STREQUAL "ftd2xx")
	FIND_LIBRARY(FTDI_LIBRARY ftdi)
	INCLUDE(FindPkgConfig)
	PKG_SEARCH_MODULE(FTDI libftdi)
	INCLUDE_DIRECTORIES( ${FTDI_INCLUDEDIR} )
	ADD_DEFINITIONS( -DLIBFTDI )
	LIST(APPEND telldus-service_SRCS TellStick_libftdi.cpp )
	LIST(APPEND telldus-service_LIBRARIES ${FTDI_LIBRARY})
ENDIF (FTDI_ENGINE STREQUAL "ftd2xx")

######## Configuring  ########

SOURCE_GROUP("Protocol Files" FILES ${telldus-service_protocol_SRCS})

CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/config.h.in
	${CMAKE_CURRENT_BINARY_DIR}/service/config.h)
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR})

SET(telldus-service_TARGET ${telldus-service_TARGET} PARENT_SCOPE)
ADD_EXECUTABLE(${telldus-service_TARGET}
	${telldus-service_SRCS}
	${telldus-service_protocol_SRCS}
	${telldus-service_HDRS}
)
ADD_DEPENDENCIES(${telldus-service_TARGET} TelldusCommon)
SET_SOURCE_FILES_PROPERTIES(${telldus-service_RESOURCES} PROPERTIES MACOSX_PACKAGE_LOCATION Resources)

TARGET_LINK_LIBRARIES( ${telldus-service_TARGET}	${telldus-service_LIBRARIES} )

SIGN(${telldus-service_TARGET})

SET_TARGET_PROPERTIES(${telldus-service_TARGET} PROPERTIES
	MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/Info.plist
)
IF (APPLE)
	SET(TELLDUS_SERVICE_TARGET_PATH "/Library/Telldus" CACHE STRING "Path to install TelldusService")
	SET_TARGET_PROPERTIES(${telldus-service_TARGET} PROPERTIES
		RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
	)
	INSTALL(TARGETS ${telldus-service_TARGET}
		RUNTIME DESTINATION ${TELLDUS_SERVICE_TARGET_PATH}
	)
	INSTALL(CODE "
		INCLUDE(GetPrerequisites)
		GET_FILENAME_COMPONENT(DESTDIR \$ENV{DESTDIR} ABSOLUTE)
		GET_PREREQUISITES(\"\${DESTDIR}/${TELLDUS_SERVICE_TARGET_PATH}/${telldus-service_TARGET}\" prereqs 1 0 \"\$\" \"\$\")
		FOREACH(pr \${prereqs})
			GET_FILENAME_COMPONENT(lib \${pr} NAME)
			FILE(INSTALL \${pr} DESTINATION ${TELLDUS_SERVICE_TARGET_PATH})
			EXECUTE_PROCESS(COMMAND install_name_tool
				-change \"\${pr}\" \"${TELLDUS_SERVICE_TARGET_PATH}/\${lib}\" \"\${DESTDIR}/${TELLDUS_SERVICE_TARGET_PATH}/${telldus-service_TARGET}\"
			)
		ENDFOREACH ()
	")
	INSTALL(FILES com.telldus.service.plist DESTINATION /Library/LaunchDaemons)
ENDIF (APPLE)

IF (UNIX)
	IF (GENERATE_MAN)
		ADD_CUSTOM_COMMAND(
			TARGET ${telldus-service_TARGET}
			POST_BUILD
			COMMAND help2man -n ${telldusd_DESCRIPTION} ./telldusd > telldusd.1
			WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
			COMMENT "Generating man file telldusd.1"
		)
		INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/telldusd.1 DESTINATION share/man/man1)
	ENDIF (GENERATE_MAN)
ENDIF (UNIX)

IF (UNIX AND NOT APPLE)
	INSTALL(TARGETS ${telldus-service_TARGET} RUNTIME DESTINATION sbin)
	INSTALL(FILES tellstick.conf
		DESTINATION ${SYSCONF_INSTALL_DIR}
	)
	INSTALL(FILES telldus-core.conf
		DESTINATION ${STATE_INSTALL_DIR}
		PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ GROUP_WRITE WORLD_READ WORLD_WRITE
	)
ENDIF (UNIX AND NOT APPLE)

IF(ENABLE_TESTING)
	# Build it once more, but as static library so it can be linked by the test-tool
	ADD_LIBRARY(TelldusServiceStatic STATIC
		${telldus-service_SRCS}
		${telldus-service_protocol_SRCS}
		${telldus-service_HDRS}
	)
	TARGET_LINK_LIBRARIES( TelldusServiceStatic	${telldus-service_LIBRARIES} )
	IF (UNIX)
		SET_TARGET_PROPERTIES( TelldusServiceStatic PROPERTIES COMPILE_FLAGS "-fPIC")
	ENDIF (UNIX)
ENDIF()
