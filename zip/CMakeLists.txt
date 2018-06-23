cmake_minimum_required(VERSION 2.6)

project (MINIZIP C)

# used bypackages that wish to link against minizip
SET(MINIZIP_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}")
SET(MINIZIP_LIBRARIES minizip)

SET(LIBRARY_OUTPUT_PATH "" CACHE INTERNAL 
  "Where to put the libraries for MINIZIP"
  )

SET(minizip_SRCS unzip.c ioapi.c)
ADD_LIBRARY(minizip STATIC ${minizip_SRCS})
