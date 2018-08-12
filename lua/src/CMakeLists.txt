cmake_minimum_required(VERSION 2.6)

SET(CORE lapi.c lcode.c lctype.c ldebug.c ldo.c ldump.c lfunc.c lgc.c llex.c
	lmem.c lobject.c lopcodes.c lparser.c lstate.c lstring.c ltable.c
	ltm.c lundump.c lvm.c lzio.c)

SET (LIB lauxlib.c lbaselib.c lbitlib.c lcorolib.c ldblib.c liolib.c
	lmathlib.c loslib.c lstrlib.c ltablib.c loadlib.c linit.c)

IF(${CMAKE_SYSTEM_NAME} MATCHES "Linux|FreeBSD")
	SET(OperatingSystem "Linux")
	add_definitions(-DLUA_USE_LINUX)
ENDIF(${CMAKE_SYSTEM_NAME} MATCHES "Linux|FreeBSD")

ADD_LIBRARY(lua STATIC ${LIB} ${CORE})
#ADD_EXECUTABLE(luaexec lua.c)
#TARGET_LINK_LIBRARIES(luaexec lua m) 

#ADD_EXECUTABLE(luac luac.c)
#TARGET_LINK_LIBRARIES(luac lua m) 
