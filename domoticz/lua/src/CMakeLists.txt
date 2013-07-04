cmake_minimum_required(VERSION 2.6)

SET(CORE lapi.c lcode.c lctype.c ldebug.c ldo.c ldump.c lfunc.c lgc.c llex.c
	lmem.c lobject.c lopcodes.c lparser.c lstate.c lstring.c ltable.c
	ltm.c lundump.c lvm.c lzio.c)

SET (LIB lauxlib.c lbaselib.c lbitlib.c lcorolib.c ldblib.c liolib.c
	lmathlib.c loslib.c lstrlib.c ltablib.c loadlib.c linit.c)

ADD_LIBRARY(lua ${LIB} ${CORE})
ADD_EXECUTABLE(luaexec lua.c)
TARGET_LINK_LIBRARIES(luaexec lua) 

set(CMAKE_EXE_LINKER_FLAGS "-lm")

ADD_EXECUTABLE(luac luac.c)
TARGET_LINK_LIBRARIES(luac lua) 
