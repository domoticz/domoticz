#cp ../c/domoticztest/hardware/eHouseTCP.cpp ./hardware/eHouseTCP.cpp
#cp ../c/domoticztest/hardware/eHouseTCP.h  ./hardware/eHouseTCP.h
##cp ../c/domoticztest/n/eHouseTCP.h  ./hardware/eHouseTCP.h
##mkdir ../hardware/eHouse
#cp ../c/domoticztest/hardware/eHouse/status.h ./hardware/eHouse/status.h
#cp ../c/domoticztest/hardware/eHouse/EhouseTcpClient.cpp ./hardware/eHouse/EhouseTcpClient.cpp
#cp ../c/domoticztest/hardware/eHouse/EhouseUdpListener.cpp ./hardware/eHouse/EhouseUdpListener.cpp
#cp ../c/domoticztest/hardware/eHouse/globals.h ./hardware/eHouse/globals.h
#cp ../c/domoticztest/hardware/eHouse/EhouseEvents.cpp ./hardware/eHouse/EhouseEvents.cpp
#cp ../c/domoticztest/hardware/eHouse/EventsCodes.h ./hardware/eHouse/EventsCodes.h

./cmake-clean ./
cmake -USE_STATIC_OPENZWAVE -DCMAKE_BUILD_TYPE=Debug -debug-out CMakeLists.txt
make
./domoticz
