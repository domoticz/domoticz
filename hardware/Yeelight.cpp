#include "stdafx.h"
#include "Yeelight.h"
#include "../main/Logger.h"
#include "../main/Helper.h"
#include "../main/SQLHelper.h"
#include "../main/localtime_r.h"
#include "../hardware/hardwaretypes.h"
#include "../main/mainworker.h"
#include "../main/WebServer.h"
#include "../webserver/cWebem.h"
#include <sstream>

Yeelight::Yeelight(const int ID, const std::string &IPAddress, const unsigned short usIPPort) :
	m_szIPAddress(IPAddress)
{
	m_HwdID = ID;
	m_bDoRestart = false;
	m_stoprequested = false;
	m_usIPPort = usIPPort;
	m_szIPAddress = IPAddress;
}

Yeelight::~Yeelight(void)
{
}

bool Yeelight::StartHardware()
{
	m_stoprequested = false;
	m_bDoRestart = false;

	//force connect the next first time
	m_bIsStarted = true;

	//Start worker thread
	m_thread = boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&Yeelight::Do_Work, this)));
	return (m_thread != NULL);
}

bool Yeelight::StopHardware()
{
	m_stoprequested = true;
	try {
		if (m_thread)
		{
			m_thread->join();
		}
	}
	catch (...)
	{
		//Don't throw from a Stop command
	}
	m_bIsStarted = false;
	return true;
}

void Yeelight::Do_Work()
{
	_log.Log(LOG_STATUS, "Starting Yeelight Controller");
	int sec_counter = 0;
	while (!m_stoprequested)
	{
		sleep_seconds(1);
		sec_counter++;
		if (sec_counter % 12 == 0) {
			m_LastHeartbeat = mytime(NULL);
		}
		if (sec_counter % 2) //60 == 0) //wait 1 minute
		{
			DiscoverLights();
			//ListenLights(); not working, it might need the discover message sent
		}
	}
	_log.Log(LOG_STATUS, "Yeelight Controller stopped...");
}

bool Yeelight::ListenLights() {

	_log.Log(LOG_STATUS, "Yeelight ListenLights.");

	int iResult = 0;

	WSADATA wsaData;

	SOCKET RecvSocket;
	sockaddr_in RecvAddr;

	unsigned short Port = 1982;

	char RecvBuf[1024];
	int BufLen = 1024;

	sockaddr_in SenderAddr;
	int SenderAddrSize = sizeof(SenderAddr);

	//-----------------------------------------------
	// Initialize Winsock
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != NO_ERROR) {
		_log.Log(LOG_STATUS, "WSAStartup() failed with error: %d\n", iResult);
		//wprintf(L"WSAStartup failed with error %d\n", iResult);
		return 1;
	}
	//-----------------------------------------------
	// Create a receiver socket to receive datagrams
	RecvSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (RecvSocket == INVALID_SOCKET) {
		_log.Log(LOG_STATUS, "socket failed with error %d\n", WSAGetLastError());
		//wprintf(L"socket failed with error %d\n", WSAGetLastError());
		return 1;
	}
	//-----------------------------------------------
	// Bind the socket to any address and the specified port.
	RecvAddr.sin_family = AF_INET;
	RecvAddr.sin_port = htons(Port);
	//RecvAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	RecvAddr.sin_addr.s_addr = inet_addr(m_szIPAddress.c_str());

	iResult = bind(RecvSocket, (SOCKADDR *)& RecvAddr, sizeof(RecvAddr));
	if (iResult != 0) {
		_log.Log(LOG_STATUS, "bind failed with error %d\n", WSAGetLastError());
		//wprintf(L"bind failed with error %d\n", WSAGetLastError());
		return 1;
	}
	//-----------------------------------------------
	// Call the recvfrom function to receive datagrams
	// on the bound socket.
	//wprintf(L"Receiving datagrams...\n");
	_log.Log(LOG_STATUS, "Receiving datagrams...\n");
	iResult = recvfrom(RecvSocket,
		RecvBuf, BufLen, 0, (SOCKADDR *)& SenderAddr, &SenderAddrSize);
	if (iResult == SOCKET_ERROR) {
		_log.Log(LOG_STATUS, "recvfrom failed with error %d\n", WSAGetLastError());
		//wprintf(L"recvfrom failed with error %d\n", WSAGetLastError());
	}
	_log.Log(LOG_STATUS, "recv: %s\n", RecvBuf);
	//-----------------------------------------------
	// Close the socket when finished receiving datagrams
	//wprintf(L"Finished receiving. Closing socket.\n");
	iResult = closesocket(RecvSocket);
	if (iResult == SOCKET_ERROR) {
		_log.Log(LOG_STATUS, "closesocket failed with error %d\n", WSAGetLastError());
		//wprintf(L"closesocket failed with error %d\n", WSAGetLastError());
		return false;
	}

	//-----------------------------------------------
	// Clean up and exit.
	_log.Log(LOG_STATUS, "Exiting.\n");
	//wprintf(L"Exiting.\n");
	WSACleanup();

	/*
	//----------------------
	// Initialize Winsock
	WSADATA wsaData;
	int iResult = 0;

	SOCKET ListenSocket = INVALID_SOCKET;
	sockaddr_in service;

	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != NO_ERROR) {
	_log.Log(LOG_STATUS, "WSAStartup() failed with error: %d\n", iResult);
	//wprintf(L"WSAStartup() failed with error: %d\n", iResult);
	return 1;
	}
	//udpSocket = socket(AF_INET, SOCK_DGRAM, 0);
	//----------------------
	// Create a SOCKET for listening for incoming connection requests.
	ListenSocket = socket(AF_INET, SOCK_STREAM, 0); // IPPROTO_UDP);
	if (ListenSocket == INVALID_SOCKET) {
	_log.Log(LOG_STATUS, "socket function failed with error: %ld\n", WSAGetLastError());
	//wprintf(L"socket function failed with error: %ld\n", WSAGetLastError());
	WSACleanup();
	return 1;
	}
	//----------------------
	// The sockaddr_in structure specifies the address family,
	// IP address, and port for the socket that is being bound.
	service.sin_family = AF_INET;
	service.sin_addr.s_addr = inet_addr(m_szIPAddress.c_str()); //inet_addr("127.0.0.1");
	service.sin_port = htons(1982);

	iResult = bind(ListenSocket, (SOCKADDR *)& service, sizeof(service));
	if (iResult == SOCKET_ERROR) {
	//wprintf(L"bind function failed with error %d\n", WSAGetLastError());
	_log.Log(LOG_STATUS, "bind function failed with error %d\n", WSAGetLastError());
	iResult = closesocket(ListenSocket);
	if (iResult == SOCKET_ERROR)
	_log.Log(LOG_STATUS, "closesocket function failed with error %d\n", WSAGetLastError());
	//wprintf(L"closesocket function failed with error %d\n", WSAGetLastError());
	WSACleanup();
	return 1;
	}
	//----------------------
	// Listen for incoming connection requests
	// on the created socket
	if (listen(ListenSocket, SOMAXCONN) == SOCKET_ERROR)
	_log.Log(LOG_STATUS, "listen function failed with error: %d\n", WSAGetLastError());

	//wprintf(L"listen function failed with error: %d\n", WSAGetLastError());

	//wprintf(L"Listening on socket...\n");

	iResult = closesocket(ListenSocket);
	if (iResult == SOCKET_ERROR) {
	//wprintf(L"closesocket function failed with error %d\n", WSAGetLastError());
	_log.Log(LOG_STATUS, "closesocket function failed with error %d\n", WSAGetLastError());
	WSACleanup();
	return 1;
	}

	WSACleanup();

	*/



	return true;
}

bool Yeelight::DiscoverLights()
{
	//_log.Log(LOG_STATUS, "Starting Broadcast...");
	int udpSocket;
	struct sockaddr_in udpClient, udpServer;
	udpSocket = socket(AF_INET, SOCK_DGRAM, 0);
	int broadcast = 1;
	if (setsockopt(udpSocket, SOL_SOCKET, SO_BROADCAST, (const char*)&broadcast, sizeof(broadcast)) == -1)
	{
		return false;
	}
	udpClient.sin_family = AF_INET;
	//udpClient.sin_addr.s_addr = htonl(INADDR_ANY);
	udpClient.sin_addr.s_addr = inet_addr(m_szIPAddress.c_str());
	udpClient.sin_port = 0;
	bind(udpSocket, (struct sockaddr*)&udpClient, sizeof(udpClient));

	udpServer.sin_family = AF_INET;
	udpServer.sin_addr.s_addr = inet_addr("239.255.255.250");
	udpServer.sin_port = htons(1982);
	char *pPacket = "M-SEARCH * HTTP/1.1\r\n\HOST: 239.255.255.250:1982\r\n\MAN: \"ssdp:discover\"\r\n\ST: wifi_bulb";
	/** send the packet **/
	sendto(udpSocket, (const char*)pPacket, 102, 0, (struct sockaddr*)&udpServer, sizeof(udpServer));

	int limit = 100; //this will limit the number of lights discoverable
	int i = 0;
	DWORD timeout = 2 * 1000;

	//_log.Log(LOG_STATUS, "Starting Listen...");
	while (i < limit)
	{
		//_log.Log(LOG_STATUS, "loop...");
		setsockopt(udpSocket, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));

		char buf[10000];
		unsigned slen = sizeof(sockaddr);
		int recv_size = recvfrom(udpSocket, buf, sizeof(buf) - 1, 0, (sockaddr *)&udpServer, (int *)&slen);
		if (recv_size == SOCKET_ERROR) {
			//_log.Log(LOG_STATUS, "SOCKET ERROR");
			closesocket(udpSocket);
		}
		else {

			std::string startString = "Location: yeelight://";
			std::string endString = ":55443\r\n";

			std::string yeelightLocation;
			std::string yeelightId;
			std::string yeelightModel;
			std::string yeelightStatus;
			std::string yeelightBright;
			std::string yeelightHue;

			//_log.Log(LOG_STATUS, "recv: %s\n", buf);
			std::string receivedString = buf;
			endString = "name:";
			//_log.Log(LOG_STATUS, "recv: %s\n", receivedString.c_str());


			endString = ":55443\r\n";
			std::size_t pos = receivedString.find(startString);
			if (pos > 0) {
				pos = pos + startString.length();
			}

			std::size_t pos1 = receivedString.substr(pos).find(endString);
			std::string dataString = receivedString.substr(pos, pos1);

			yeelightLocation = dataString.c_str();
			//_log.Log(LOG_STATUS, "DiscoverLights: Location:  %s", dataString.c_str());

			endString = "\r\n";
			startString = "id: ";
			pos = receivedString.find(startString);
			pos = pos + 4;
			pos1 = receivedString.substr(pos).find(endString);
			dataString = receivedString.substr(pos, pos1);
			yeelightId = dataString.c_str();
			yeelightId = dataString.substr(10, dataString.length() - 10);

			startString = "model: ";
			pos = receivedString.find(startString);
			pos1 = receivedString.substr(pos).find(endString);
			dataString = receivedString.substr(pos, pos1);
			yeelightModel = dataString.c_str();

			startString = "power: ";
			pos = receivedString.find(startString);
			pos1 = receivedString.substr(pos).find(endString);
			dataString = receivedString.substr(pos, pos1);
			yeelightStatus = dataString.c_str();
			//_log.Log(LOG_STATUS, "Found Yeelight  %s %s %s", yeelightId.c_str(), yeelightModel.c_str(), yeelightStatus.c_str());

			/*
			rgb: 2464799
			hue: 229
			sat: 100
			*/

			startString = "bright: ";
			pos = receivedString.find(startString);
			pos = pos + 8;
			pos1 = receivedString.substr(pos).find(endString);
			dataString = receivedString.substr(pos, pos1);
			yeelightBright = dataString.c_str();

			startString = "hue: ";
			pos = receivedString.find(startString);
			pos = pos + 5;
			pos1 = receivedString.substr(pos).find(endString);
			dataString = receivedString.substr(pos, pos1);
			yeelightHue = dataString.c_str();

			bool bIsOn = false;
			if (yeelightStatus == "power: on") {
				bIsOn = true;
			}
			int sType = sTypeYeelightWhite; // sTypeLimitlessWhite;

			std::string yeelightName = "";
			if (yeelightModel == "model: mono") {
				yeelightName = "Yeelight LED (Mono)";
				//InsertUpdateSwitch(yeelightId, yeelightName, sTypeLimitlessWhite, yeelightLocation, bIsOn, yeelightBright);
			}
			else if (yeelightModel == "model: color") {
				yeelightName = "Yeelight LED (Color)";
				sType = sTypeYeelightColor; // sTypeLimitlessRGBW;
				//InsertUpdateSwitch(yeelightId, yeelightName, sTypeLimitlessRGBW, yeelightLocation, bIsOn, yeelightBright);
			}
			InsertUpdateSwitch(yeelightId, yeelightName, sType, yeelightLocation, bIsOn, yeelightBright, yeelightHue);
		}
		i++;
		//sleep_seconds(1);
	}
	closesocket(udpSocket);

	//_log.Log(LOG_STATUS, "Finished Listen...");
	return true;
}


void Yeelight::InsertUpdateSwitch(const std::string &nodeID, const std::string &lightName, const int &YeeType, const std::string &Location, const bool bIsOn, const std::string &yeelightBright, const std::string &yeelightHue)
{
	int lastLevel = 0;
	int nvalue = 0;
	bool tIsOn = !(bIsOn);
	std::vector<std::vector<std::string> > result;
	result = m_sql.safe_query("SELECT nValue, LastLevel FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='%q') AND (Type==%d) AND (SubType==%d)", m_HwdID, nodeID.c_str(), pTypeYeelight, YeeType);
	if (result.size() < 1)
	{
		_log.Log(LOG_STATUS, "Yeelight: New %s Found", lightName.c_str());
		m_sql.safe_query(
			"INSERT INTO DeviceStatus (HardwareID, DeviceID, Unit, Type, SwitchType, SubType, SignalLevel, BatteryLevel, Name, nValue, sValue, Options) "
			"VALUES (%d,'%q', %d, %d, %d, %d, 12,255,'%q',0,' ','%q')",
			m_HwdID, nodeID.c_str(), (int)0, pTypeYeelight, int(STYPE_Dimmer), YeeType, lightName.c_str(), Location.c_str());
	}
	else {

		nvalue = atoi(result[0][0].c_str());
		tIsOn = (nvalue != 0);
		lastLevel = atoi(result[0][1].c_str());
		int value = atoi(yeelightBright.c_str());
		if ((bIsOn != tIsOn) || (value != lastLevel))
		{
			unsigned char unitcode = 1;
			int cmd = light1_sOn;
			int level = 100;
			if (!bIsOn) {
				cmd = light1_sOff;
				level = 0;
			}

			uint8_t unit = 0;
			uint8_t dType = pTypeYeelight;

			unsigned long ID1;
			std::stringstream s_strid1;
			s_strid1 << std::hex << nodeID.c_str();
			s_strid1 >> ID1;

			_tYeelight lcmd;
			lcmd.len = sizeof(_tYeelight) - 1;
			lcmd.type = dType;
			lcmd.subtype = YeeType;
			lcmd.id = ID1;
			lcmd.dunit = unit;
			
			lcmd.value = value;
			lcmd.command = cmd;
			m_mainworker.PushAndWaitRxMessage(this, (const unsigned char *)&lcmd, NULL, -1);
			//add yeelightHue
			m_sql.safe_query("UPDATE DeviceStatus SET SwitchType=%d, nValue=%d, sValue='', LastLevel=%d WHERE(HardwareID == %d) AND (DeviceID == '%q')", int(STYPE_Dimmer), int(cmd), value, m_HwdID, nodeID.c_str());		

		}
	}
}

bool Yeelight::WriteToHardware(const char *pdata, const unsigned char length)
{
	//_log.Log(LOG_STATUS, "Yeelight: WriteToHardware...............................");
	std::string ipAddress = "192.168.0.1";
	_tYeelight *pLed = (_tYeelight*)pdata;
	uint8_t command = pLed->command;
	//_log.Log(LOG_STATUS, "Yeelight: WriteToHardware command: %d", command);	
	std::vector<std::vector<std::string> > result;
	char szTmp[300];
	if (pLed->id == 1)
		sprintf(szTmp, "%d", 1);
	else
		sprintf(szTmp, "%08x", (unsigned int)pLed->id);
	std::string ID = szTmp;
	result = m_sql.safe_query("SELECT Options FROM DeviceStatus WHERE (DeviceID='%q')", ID.c_str());
	if (result.size() > 0) {
		ipAddress = result[0][0].c_str();
	}

	struct sockaddr_in address;
	memset(&address, 0, sizeof(address));
	address.sin_family = AF_INET;
	address.sin_port = htons(55443);
	address.sin_family = PF_INET;
	address.sin_addr.s_addr = inet_addr(ipAddress.c_str());
	int sd = socket(AF_INET, SOCK_STREAM, 0);
	if (::connect(sd, (struct sockaddr*)&address, sizeof(address)) != 0) {
		return false;
	}
	std::string message = "{\"id\":1,\"method\":\"toggle\",\"params\":[]}\r\n";
	switch (pLed->command)
	{
	case Yeelight_LedOn:
		message = "{\"id\":1,\"method\":\"set_power\",\"params\":[\"on\", \"smooth\", 500]}\r\n";
		break;
	case Yeelight_LedOff:
		message = "{\"id\":1,\"method\":\"set_power\",\"params\":[\"off\", \"smooth\", 500]}\r\n";
		break;
	case Yeelight_SetColorToWhite:

		message = "{\"id\":1,\"method\":\"set_power\",\"params\":[\"on\", \"smooth\", 500]}\r\n";
		send(sd, message.c_str(), message.length(), 0);
		sleep_milliseconds(200);
		message = "{\"id\":1,\"method\":\"set_rgb\",\"params\":[16777215, \"smooth\", 500]}\r\n";
		break;
	case Yeelight_SetBrightnessLevel: {
		int value = pLed->value;
		message = "{\"id\":1,\"method\":\"set_power\",\"params\":[\"on\", \"smooth\", 500]}\r\n";
		send(sd, message.c_str(), message.length(), 0);
		sleep_milliseconds(200);
		message = "{\"id\":1,\"method\":\"set_bright\",\"params\":[" + std::to_string(value) + ", \"smooth\", 500]}\r\n";
	}
		break;
	case Yeelight_SetRGBColour: {
		message = "{\"id\":1,\"method\":\"set_power\",\"params\":[\"on\", \"smooth\", 500]}\r\n";
		send(sd, message.c_str(), message.length(), 0);
		sleep_milliseconds(200);
		float cHue = (359.0f / 255.0f)*float(pLed->value);//hue given was in range of 0-255
		message = "{\"id\":1,\"method\":\"set_hsv\",\"params\":[" + std::to_string(cHue) + ", 100, \"smooth\", 2000]}\r\n";
	}
		break;
	default:
		_log.Log(LOG_STATUS, "Yeelight: Unhandled WriteToHardware command: %d", command);
		break;
	}
	//_log.Log(LOG_STATUS, msg1.c_str());
	send(sd, message.c_str(), message.length(), 0);
	sleep_milliseconds(200);
	closesocket(sd);
	//_log.Log(LOG_STATUS, "Yeelight: WriteToHardware FINISHED");
	return true;
}




//Webserver helpers
namespace http {
	namespace server {
		void CWebServer::SetYeelightType(WebEmSession & session, const request& req, std::string & redirect_uri)
		{
			redirect_uri = "/index.html";
			if (session.rights != 2)
			{
				//No admin user, and not allowed to be here
				return;
			}
			std::string idx = request::findValue(&req, "idx");
			if (idx == "") {
				return;
			}

			std::vector<std::vector<std::string> > result;

			result = m_sql.safe_query("SELECT Mode1, Mode2, Mode3, Mode4, Mode5, Mode6 FROM Hardware WHERE (ID='%q')", idx.c_str());
			if (result.size() < 1)
				return;

			int Mode1 = atoi(request::findValue(&req, "YeelightType").c_str());
			int Mode2 = atoi(result[0][1].c_str());
			int Mode3 = atoi(result[0][2].c_str());
			int Mode4 = atoi(result[0][3].c_str());
			m_sql.UpdateRFXCOMHardwareDetails(atoi(idx.c_str()), Mode1, Mode2, Mode3, Mode4, 0, 0);

			m_mainworker.RestartHardware(idx);

			return;
		}
	}
}