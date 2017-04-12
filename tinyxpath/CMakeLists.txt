#set to minimum version that supports clean build on cygwin
cmake_minimum_required(VERSION 2.8.4)

project (TINYXPATH CPP)

SET(TINYXPATH_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}")
SET(TINYXPATH_LIBRARIES tinyxpath)

SET(LIBRARY_OUTPUT_PATH "" CACHE INTERNAL 
  "Where to put the libraries for TINYXPATH"
)

SET(CORE
	action_store.cpp lex_util.cpp node_set.cpp tinyxml.cpp tinyxmlparser.cpp xml_util.cpp xpath_processor.cpp xpath_static.cpp xpath_syntax.cpp
	htmlutil.cpp tinystr.cpp tinyxmlerror.cpp tokenlist.cpp xpath_expression.cpp xpath_stack.cpp xpath_stream.cpp
)

IF(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
	SET(OperatingSystem "Linux")
ENDIF(${CMAKE_SYSTEM_NAME} MATCHES "Linux")

ADD_LIBRARY(tinyxpath ${LIB} ${CORE})
