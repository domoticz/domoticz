SET( SRCS
	getopt_long.c
)

SET( HDRS
	getopt.h
)

INCLUDE_DIRECTORIES( ${CMAKE_CURRENT_SOURCE_DIR} )

ADD_LIBRARY(openbsd-getopt STATIC
	${SRCS}
	${HDRS}
)
