cmake_minimum_required(VERSION 2.6)

project (MQTT C)

# used bypackages that wish to link against lua
SET(MQTT_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}")
SET(MQTT_LIBRARIES mqtt)

SET(LIBRARY_OUTPUT_PATH "" CACHE INTERNAL 
  "Where to put the libraries for MQTT"
  )

SET(CORE Clients.c Heap.c LinkedList.c Log.c Messages.c MQTTPacket.c MQTTPacketOut.c MQTTPersistence.c MQTTPersistenceDefault.c MQTTProtocolClient.c MQTTProtocolOut.c MQTTVersion.c
		Socket.c SocketBuffer.c SSLSocket.c StackTrace.c Thread.c Tree.c utf-8.c)

SET (LIB MQTTAsync.c)

IF(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
	SET(OperatingSystem "Linux")
ENDIF(${CMAKE_SYSTEM_NAME} MATCHES "Linux")

ADD_LIBRARY(mqtt ${LIB} ${CORE})
