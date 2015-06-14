cmake_minimum_required(VERSION 2.6)

project (MQTT C)

SET(MQTT_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}")
SET(MQTT_LIBRARIES mqtt)

SET(LIBRARY_OUTPUT_PATH "" CACHE INTERNAL 
  "Where to put the libraries for MQTT"
)

find_library(OPENSSL_LIBRARY ssl)
if(NOT OPENSSL_INCLUDE_DIR)
  message(STATUS "Failed to find openssl include files (ssl.h), no TLS support")
endif()
if(NOT OPENSSL_LIBRARY)
  message(STATUS "Failed to find the openssl library, no TLS support")
else()
  add_definitions(-DNS_ENABLE_SSL)
endif()

SET(CORE
	logging_mosq.c memory_mosq.c messages_mosq.c net_mosq.c read_handle.c read_handle_client.c read_handle_shared.c
	send_client_mosq.c send_mosq.c socks_mosq.c srv_mosq.c thread_mosq.c time_mosq.c tls_mosq.c util_mosq.c will_mosq.c
)

SET (LIB mosquitto.c)

IF(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
	SET(OperatingSystem "Linux")
ENDIF(${CMAKE_SYSTEM_NAME} MATCHES "Linux")

ADD_LIBRARY(mqtt ${LIB} ${CORE})
