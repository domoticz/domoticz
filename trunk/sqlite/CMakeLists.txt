cmake_minimum_required(VERSION 2.6)

project (SQLite C)

SET(SQLite_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}")
SET(SQLite_LIBRARIES sqlite)

SET(LIBRARY_OUTPUT_PATH "" CACHE INTERNAL 
  "Where to put the libraries for SQLite"
  )

SET (LIB sqlite3.c)

IF(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
	SET(OperatingSystem "Linux")
ENDIF(${CMAKE_SYSTEM_NAME} MATCHES "Linux")

ADD_LIBRARY(sqlite ${LIB})
