
## Link libraries

#
# Find MD5/RMD160/SHA library
#
option(USE_SSL "Build with SSL SUPPORT" YES)

macro(libssl target)
if(USE_SSL)
  find_package(OpenSSL REQUIRED)
  if(NOT OPENSSL_INCLUDE_DIR)
    message(SEND_ERROR "Failed to find OpenSSL include files (ssl.h), no HTTPS support")
  endif()
  if(NOT OPENSSL_FOUND)
    message(SEND_ERROR "Failed to find the OpenSSL library, no HTTPS support")
    find_library(MD_LIBRARY NAMES md)
    if(MD_LIBRARY)
      target_link_libraries(${target} ${MD_LIBRARY})
    else()
      message(FATAL_ERROR "md5 library not found (provided by e.g. OpenSSL)")
    endif(MD_LIBRARY)
  else()
    message(STATUS "OPENSSL library found at: ${OPENSSL_LIBRARIES}")
    add_definitions(-DWWW_ENABLE_SSL)
    add_definitions(-DWITH_TLS)
    include_directories(${OPENSSL_INCLUDE_DIR})
    target_link_libraries(${target} ${OPENSSL_LIBRARIES})
  endif()
else()
  find_library(MD_LIBRARY NAMES md)
  if(MD_LIBRARY)
    target_link_libraries(${target} ${MD_LIBRARY})
  else()
    # ok tests
    message(STATUS "WARNING: md5 library not found (provided by e.g. OpenSSL)")
  endif(MD_LIBRARY)
endif()
endmacro()

option(USE_OPENSSL_STATIC "Link OpenSSL static" YES)
IF(USE_OPENSSL_STATIC)
	set(OPENSSL_USE_STATIC_LIBS TRUE)
ENDIF(USE_OPENSSL_STATIC)

