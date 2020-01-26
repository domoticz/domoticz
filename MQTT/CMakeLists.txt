#set to minimum version that supports clean build on cygwin
cmake_minimum_required(VERSION 2.8.4)

project (MQTT C)

SET(MQTT_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}")
SET(MQTT_LIBRARIES mqtt)

SET(LIBRARY_OUTPUT_PATH "" CACHE INTERNAL 
  "Where to put the libraries for MQTT"
)

option(USE_OPENSSL_STATIC "Link OpenSSL static" YES)
IF(USE_OPENSSL_STATIC)
	set(OPENSSL_USE_STATIC_LIBS TRUE)
ENDIF(USE_OPENSSL_STATIC)

find_package(OpenSSL)
if(NOT OPENSSL_INCLUDE_DIR)
  message(STATUS "Failed to find openssl include files (ssl.h), no TLS support")
endif()
if(NOT OPENSSL_FOUND)
  message(STATUS "Failed to find the openssl library, no TLS support")
else()
  add_definitions(-DWITH_TLS)
  include_directories(${OPENSSL_INCLUDE_DIR})
endif()

SET(CORE mosquitto.c
	actions.c alias_mosq.c callbacks.c connect.c handle_auth.c handle_connack.c handle_disconnect.c handle_ping.c handle_pubackcomp.c handle_publish.c
	handle_pubrec.c handle_pubrel.c handle_suback.c handle_unsuback.c helpers.c logging_mosq.c loop.c memory_mosq.c messages_mosq.c net_mosq.c
	net_mosq_ocsp.c options.c packet_datatypes.c packet_mosq.c property_mosq.c read_handle.c send_connect.c send_disconnect.c send_mosq.c send_publish.c
	send_subscribe.c send_unsubscribe.c socks_mosq.c srv_mosq.c thread_mosq.c time_mosq.c tls_mosq.c utf8_mosq.c util_mosq.c util_topic.c will_mosq.c
)

IF(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
	SET(OperatingSystem "Linux")
ENDIF(${CMAKE_SYSTEM_NAME} MATCHES "Linux")

ADD_LIBRARY(mqtt STATIC ${LIB} ${CORE})

include_directories("${CMAKE_CURRENT_SOURCE_DIR}")
link_directories("${CMAKE_CURRENT_SOURCE_DIR}")

target_link_libraries(mqtt OpenSSL::Crypto OpenSSL::SSL)
