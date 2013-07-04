cmake_minimum_required(VERSION 2.6)

project (LUA C)

# used bypackages that wish to link against lua
SET(LUA_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/src")
SET(LUA_LIBRARIES lua)

SET(LIBRARY_OUTPUT_PATH "" CACHE INTERNAL 
  "Where to put the libraries for LUA"
  )

ADD_SUBDIRECTORY(src)
