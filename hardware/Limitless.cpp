#include "stdafx.h"
#include "Limitless.h"
#include "../main/Helper.h"
#include "../main/Logger.h"
#include "../main/SQLHelper.h"
#include "hardwaretypes.h"
#include "../main/localtime_r.h"
#include "../main/mainworker.h"
#include "../main/WebServer.h"
#include "../webserver/cWebem.h"

#define round(a) ( int ) ( a + .5 )

//This hardware goes under a few different names, i was told the original name was AppLamp

// Commands
// V6
#define l_zone 0
#define v6_repeats 3

const uint8_t V6_PreAmble[] = { 0x80, 0x00, 0x00, 0x00, 0x11 };

const uint8_t V6_GetSessionID[] = { 0x20, 0x00, 0x00, 0x00, 0x16, 0x02, 0x62, 0x3A, 0xD5, 0xED, 0xA3, 0x01, 0xAE, 0x08, 0x2D, 0x46, 0x61, 0x41, 0xA7, 0xF6, 0xDC, 0xAF, 0xD3, 0xE6, 0x00, 0x00, 0x1E };

uint8_t V6_BridgeOn[10] = { 0x31, 0x00, 0x00, 0x00, 0x03, 0x03, 0x00, 0x00, 0x00, 0x01 };
uint8_t V6_RGBWW_On[10] = { 0x31, 0x00, 0x00, 0x08, 0x04, 0x01, 0x00, 0x00, 0x00, l_zone };
uint8_t V6_RGBW_On[10] = { 0x31, 0x00, 0x00, 0x07, 0x03, 0x01, 0x00, 0x00, 0x00, l_zone };

uint8_t V6_BridgeOff[10] = { 0x31, 0x00, 0x00, 0x00, 0x03, 0x04, 0x00, 0x00, 0x00, 0x01 };
uint8_t V6_RGBWW_Off[10] = { 0x31, 0x00, 0x00, 0x08, 0x04, 0x02, 0x00, 0x00, 0x00, l_zone };
uint8_t V6_RGBW_Off[10] = { 0x31, 0x00, 0x00, 0x07, 0x03, 0x02, 0x00, 0x00, 0x00, l_zone };

uint8_t V6_Bridge_SetColor[10] = { 0x31, 0x00, 0x00, 0x00, 0x01, 0xBA, 0xBA, 0xBA, 0xBA, 0x01 };
uint8_t V6_RGBWW_SetColor[10] = { 0x31, 0x00, 0x00, 0x08, 0x01, 0xBA, 0xBA, 0xBA, 0xBA, l_zone };
uint8_t V6_RGBW_SetColor[10] = { 0x31, 0x00, 0x00, 0x07, 0x01, 0xBA, 0xBA, 0xBA, 0xBA, l_zone };

uint8_t V6_RGBWW_SetKelvinLevel[10] = { 0x31, 0x00, 0x00, 0x08, 0x05, 0x00, 0x00, 0x00, 0x00, l_zone }; //(6th hex values 0x00 to 0x64 : examples: 00 = 2700K (Warm White), 19 = 3650K, 32 = 4600K, 4B, = 5550K, 64 = 6500K (Cool White))

uint8_t V6_Bridge_SetBrightnessLevel[10] = { 0x31, 0x00, 0x00, 0x00, 0x02, 0xBE, 0x00, 0x00, 0x00, 0x01 };
uint8_t V6_RGBWW_SetBrightnessLevel[10] = { 0x31, 0x00, 0x00, 0x08, 0x03, 0xBE, 0x00, 0x00, 0x00, l_zone };
uint8_t V6_RGBW_SetBrightnessLevel[10] = { 0x31, 0x00, 0x00, 0x07, 0x02, 0xBE, 0x00, 0x00, 0x00, l_zone };

uint8_t V6_Bridge_White_On[10] = { 0x31, 0x00, 0x00, 0x00, 0x03, 0x05, 0x00, 0x00, 0x00, 0x01 };
uint8_t V6_RGBWW_White_On[10] = { 0x31, 0x00, 0x00, 0x08, 0x05, 0x64, 0x00, 0x00, 0x00, l_zone };
uint8_t V6_RGBW_White_On[10] = { 0x31, 0x00, 0x00, 0x07, 0x03, 0x05, 0x00, 0x00, 0x00, l_zone };

uint8_t V6_RGBWW_Night_On[10] = { 0x31, 0x00, 0x00, 0x08, 0x04, 0x05, 0x00, 0x00, 0x00, l_zone };
uint8_t V6_RGBW_Night_On[10] = { 0x31, 0x00, 0x00, 0x07, 0x03, 0x06, 0x00, 0x00, 0x00, l_zone };

uint8_t V6_Bridge_Disco_Mode[10] = { 0x31, 0x00, 0x00, 0x00, 0x04, 0x01, 0x00, 0x00, 0x00, 0x01 }; //(6th hex values 0x01 to 0x09 : examples: 01 = Mode1, 02 = Mode2, 03 = Mode3 .. 09 = Mode9)
uint8_t V6_RGBWW_Disco_Mode[10] = { 0x31, 0x00, 0x00, 0x08, 0x06, 0x01, 0x00, 0x00, 0x00, l_zone };  //(6th hex values 0x01 to 0x09 : examples: 01 = Mode1, 02 = Mode2, 03 = Mode3 .. 09 = Mode9)
uint8_t V6_RGBW_Disco_Mode[10] = { 0x31, 0x00, 0x00, 0x07, 0x04, 0x01, 0x00, 0x00, 0x00, l_zone };   //(6th hex values 0x01 to 0x09 : examples: 01 = Mode1, 02 = Mode2, 03 = Mode3 .. 09 = Mode9)

uint8_t V6_Bridge_Mode_Speed_Up[10] = { 0x31, 0x00, 0x00, 0x00, 0x03, 0x02, 0x00, 0x00, 0x00, 0x01 };
uint8_t V6_RGBWW_Mode_Speed_Up[10] = { 0x31, 0x00, 0x00, 0x08, 0x04, 0x03, 0x00, 0x00, 0x00, l_zone };
uint8_t V6_RGBW_Mode_Speed_Up[10] = { 0x31, 0x00, 0x00, 0x07, 0x03, 0x03, 0x00, 0x00, 0x00, l_zone };

uint8_t V6_Bridge_Mode_Speed_Down[10] = { 0x31, 0x00, 0x00, 0x00, 0x03, 0x01, 0x00, 0x00, 0x00, 0x01 };
uint8_t V6_RGBWW_Mode_Speed_Down[10] = { 0x31, 0x00, 0x00, 0x08, 0x04, 0x04, 0x00, 0x00, 0x00, l_zone };
uint8_t V6_RGBW_Mode_Speed_Down[10] = { 0x31, 0x00, 0x00, 0x07, 0x03, 0x04, 0x00, 0x00, 0x00, l_zone };


// V4/V5
// White LEDs
const unsigned char AllOn[3] = { 0x35, 0x0, 0x55 };
const unsigned char AllOff[3] = { 0x39, 0x0, 0x55 };
const unsigned char AllNight[3] = { 0xB9, 0x0, 0x55 };
const unsigned char AllFull[3] = { 0xB5, 0x0, 0x55 };

const unsigned char BrightnessUp[3] = { 0x3C, 0x0, 0x55 };
const unsigned char BrightnessDown[3] = { 0x34, 0x0, 0x55 };

const unsigned char ColorTempUp[3] = { 0x3E, 0x0, 0x55 };
const unsigned char ColorTempDown[3] = { 0x3F, 0x0, 0x55 };

const unsigned char Group1On[3] = { 0x38, 0x0, 0x55 };
const unsigned char Group1Off[3] = { 0x3B, 0x0, 0x55 };
const unsigned char Group1Night[3] = { 0xBB, 0x0, 0x55 };
const unsigned char Group1Full[3] = { 0xB8, 0x0, 0x55 };

const unsigned char Group2On[3] = { 0x3D, 0x0, 0x55 };
const unsigned char Group2Off[3] = { 0x33, 0x0, 0x55 };
const unsigned char Group2Night[3] = { 0xB3, 0x0, 0x55 };
const unsigned char Group2Full[3] = { 0xBD, 0x0, 0x55 };

const unsigned char Group3On[3] = { 0x37, 0x0, 0x55 };
const unsigned char Group3Off[3] = { 0x3A, 0x0, 0x55 };
const unsigned char Group3Night[3] = { 0xBA, 0x0, 0x55 };
const unsigned char Group3Full[3] = { 0xB7, 0x0, 0x55 };

const unsigned char Group4On[3] = { 0x32, 0x0, 0x55 };
const unsigned char Group4Off[3] = { 0x36, 0x0, 0x55 };
const unsigned char Group4Night[3] = { 0xB6, 0x0, 0x55 };
const unsigned char Group4Full[3] = { 0xB2, 0x0, 0x55 };

// RGB LEDs
const unsigned char RGBOn[3] = { 0x22, 0x0, 0x55 };
const unsigned char RGBOff[3] = { 0x21, 0x0, 0x55 };

const unsigned char RGBBrightnessUp[3] = { 0x23, 0x0, 0x55 };
const unsigned char RGBBrightnessDown[3] = { 0x24, 0x0, 0x55 };

const unsigned char RGBDiscoSpeedSlower[3] = { 0x26, 0x0, 0x55 };
const unsigned char RGBDiscoSpeedFaster[3] = { 0x25, 0x0, 0x55 };
const unsigned char RGBDiscoSpeedFasterLong[3] = { 0xA5, 0x0, 0x55 };

const unsigned char RGBDiscoNext[3] = { 0x27, 0x0, 0x55 };
const unsigned char RGBDiscoPrevious[3] = { 0x28, 0x0, 0x55 };

unsigned char RGBSetColour[3] = { 0x20, 0x0, 0x55 };


//RGBW LEDs
const unsigned char RGBWOn[3] = { 0x42, 0x0, 0x55 };
const unsigned char RGBWOff[3] = { 0x41, 0x0, 0x55 };

const unsigned char RGBWDiscoSpeedSlower[3] = { 0x43, 0x0, 0x55 };
const unsigned char RGBWDiscoSpeedFaster[3] = { 0x44, 0x0, 0x55 };

const unsigned char RGBWGroup1AllOn[3] = { 0x45, 0x0, 0x55 };	//(SYNC/PAIR RGB+W Bulb within 2 seconds of Wall Switch Power being turned ON)
const unsigned char RGBWGroup1AllOff[3] = { 0x46, 0x0, 0x55 };
const unsigned char RGBWGroup2AllOn[3] = { 0x47, 0x0, 0x55 };	//(SYNC/PAIR RGB+W Bulb within 2 seconds of Wall Switch Power being turned ON)
const unsigned char RGBWGroup2AllOff[3] = { 0x48, 0x0, 0x55 };
const unsigned char RGBWGroup3AllOn[3] = { 0x49, 0x0, 0x55 };	//(SYNC/PAIR RGB+W Bulb within 2 seconds of Wall Switch Power being turned ON)
const unsigned char RGBWGroup3AllOff[3] = { 0x4A, 0x0, 0x55 };
const unsigned char RGBWGroup4AllOn[3] = { 0x4B, 0x0, 0x55 };	//(SYNC/PAIR RGB+W Bulb within 2 seconds of Wall Switch Power being turned ON)
const unsigned char RGBWGroup4AllOff[3] = { 0x4C, 0x0, 0x55 };
const unsigned char RGBWDiscoMode[3] = { 0x4D, 0x0, 0x55 };
const unsigned char RGBWAllNightMode[3] = { 0xC1, 0x0, 0x55 }; // Set All RGBW to Night Mode
const unsigned char RGBWGroup1NightMode[3] = { 0xC6, 0x0 , 0x55 }; // Set Group 1 to Night Mode
const unsigned char RGBWGroup2NightMode[3] = { 0xC8, 0x0 , 0x55 }; // Set Group 2 to Night Mode
const unsigned char RGBWGroup3NightMode[3] = { 0xCA, 0x0 , 0x55 }; // Set Group 3 to Night Mode
const unsigned char RGBWGroup4NightMode[3] = { 0xCC, 0x0 , 0x55 }; // Set Group 4 to Night Mode

const unsigned char RGBWSetColorToWhiteAll[3] = { 0xC2,0x0,0x55 };	//SET COLOR TO WHITE (GROUP ALL) 0x42 100ms followed by: 0xC2
const unsigned char RGBWSetColorToWhiteGroup1[3] = { 0xC5,0x0,0x55 };	//SET COLOR TO WHITE (GROUP 1) 0x45 100ms followed by: 0xC5
const unsigned char RGBWSetColorToWhiteGroup2[3] = { 0xC7,0x0,0x55 };	//SET COLOR TO WHITE (GROUP 2) 0x47 100ms followed by: 0xC7
const unsigned char RGBWSetColorToWhiteGroup3[3] = { 0xC9,0x0,0x55 };	//SET COLOR TO WHITE (GROUP 3) 0x49 100ms followed by: 0xC9
const unsigned char RGBWSetColorToWhiteGroup4[3] = { 0xCB,0x0,0x55 };	//SET COLOR TO WHITE (GROUP 4) 0x4B 100ms followed by: 0xCB

unsigned char RGBWSetBrightnessLevel[3] = { 0x4E,0,0x55 };
//LIMITLESSLED RGBW DIRECT BRIGHTNESS SETTING is by a 3BYTE COMMAND: (First send the Group ON for the group you want to set the brightness for. You send the group ON command 100ms before sending the 4E 00 55)
//Byte1: 0x4E (decimal: 78)
//Byte2: 0x00 to 0xFF (full brightness 0x3B)
//Byte3: Always 0x55 (decimal: 85)

unsigned char RGBWSetColor[3] = { 0x40,0,0x55 };
//LIMITLESSLED RGBW COLOR SETTING is by a 3BYTE COMMAND: (First send the Group ON for the group you want to set the color for. You send the group ON command 100ms before sending the 40)
//Byte1: 0x40 (decimal: 64)
//Byte2: 0x00 to 0xFF (255 colors) Color Matrix Chart [COMING SOON]
//Byte3: Always 0x55 (decimal: 85)

//White LEDs
const unsigned char WhiteBrightnessUp[3] = { 0x3C, 0x0, 0x55 };
const unsigned char WhiteBrightnessDown[3] = { 0x34, 0x0, 0x55 };
const unsigned char WhiteWarmer[3] = { 0x3E, 0x0, 0x55 };
const unsigned char WhiteCooler[3] = { 0x3F, 0x0, 0x55 };


CLimitLess::CLimitLess(const int ID, const int LedType, const int BridgeType, const std::string& IPAddress, const unsigned short usIPPort) :
	m_szIPAddress(IPAddress)
{
	m_HwdID = ID;
	m_usIPPort = usIPPort;
	m_RemoteSocket = INVALID_SOCKET;
	m_bSkipReceiveCheck = true;
	m_BridgeType = (_eLimitlessBridgeType)BridgeType;
	m_LEDType = (unsigned char)LedType;
	m_bIsStarted = false;
	Init();
}

void CLimitLess::Init()
{
	m_CommandCntr = 1;
	m_BridgeID_1 = m_BridgeID_2 = 0;
}

bool CLimitLess::AddSwitchIfNotExits(const unsigned char Unit, const std::string& devname)
{
	std::vector<std::vector<std::string> > result;
	if (Unit != 5) {
		result = m_sql.safe_query("SELECT ID FROM DeviceStatus WHERE (HardwareID==%d) AND (Unit==%d) AND (Type==%d) AND (SubType==%d)", m_HwdID, int(Unit), pTypeColorSwitch, int(m_LEDType));
		if (result.empty())
		{
			m_sql.InsertDevice(m_HwdID, "1", Unit, pTypeColorSwitch, m_LEDType, STYPE_Dimmer, 0, " ", devname);
			return false;
		}
	}
	else {
		result = m_sql.safe_query("SELECT ID FROM DeviceStatus WHERE (HardwareID==%d) AND (Unit==%d) AND (Type==%d) AND (SubType==%d)", m_HwdID, int(Unit), pTypeColorSwitch, sTypeColor_RGB_CW_WW);
		if (result.empty())
		{
			m_sql.InsertDevice(m_HwdID, "1", Unit, pTypeColorSwitch, sTypeColor_RGB_CW_WW, STYPE_Dimmer, 0, " ", devname);
			return false;
		}
	}
	return true;
}

bool CLimitLess::StartHardware()
{
	RequestStart();

	Init();
	if (m_RemoteSocket != INVALID_SOCKET)
	{
		closesocket(m_RemoteSocket);
		m_RemoteSocket = INVALID_SOCKET;
	}


	struct hostent* he;
	if ((he = gethostbyname(m_szIPAddress.c_str())) == nullptr)
	{ // get the host info
		_log.Log(LOG_ERROR, "AppLamp: Error with IP address!...");
		return false;
	}
	m_RemoteSocket = socket(AF_INET, SOCK_DGRAM, 0);

	// this call is what allows broadcast packets to be sent:
	int broadcast = 1;
	if (setsockopt(m_RemoteSocket, SOL_SOCKET, SO_BROADCAST, (const char*)&broadcast, sizeof(broadcast)) == -1) {
		_log.Log(LOG_ERROR, "AppLamp: Error with IP address (SO_BROADCAST)!...");
		return false;
	}

	memset(&m_stRemoteDestAddr, 0, sizeof(m_stRemoteDestAddr));
	//m_stRemoteDestAddr.sin_family = AF_UNSPEC;
	//m_stRemoteDestAddr.sin_family = PF_INET;
	m_stRemoteDestAddr.sin_family = AF_INET;     // host byte order
	m_stRemoteDestAddr.sin_port = htons(m_usIPPort); // short, network byte order
	m_stRemoteDestAddr.sin_addr = *((struct in_addr*)he->h_addr);
	memset(m_stRemoteDestAddr.sin_zero, '\0', sizeof m_stRemoteDestAddr.sin_zero);

	//Add the Default switches
	if (m_LEDType != sTypeColor_RGB) {
		if (!AddSwitchIfNotExits(0, "AppLamp All"))
		{
			if (!AddSwitchIfNotExits(1, "AppLamp Group1"))
			{
				if (!AddSwitchIfNotExits(2, "AppLamp Group2"))
				{
					if (!AddSwitchIfNotExits(3, "AppLamp Group3"))
					{
						AddSwitchIfNotExits(4, "AppLamp Group4");
					}
				}
			}
		}
		if (m_BridgeType == LBTYPE_V6)
		{
			AddSwitchIfNotExits(5, "AppLamp Bridge");
		}
	}
	else {
		//RGB controller is a single controlled device
		AddSwitchIfNotExits(0, "AppLamp RGB");
	}
	m_bIsStarted = true;
	sOnConnected(this);
	//Start worker thread
	m_thread = std::make_shared<std::thread>([this] { Do_Work(); });
	SetThreadNameInt(m_thread->native_handle());
	_log.Log(LOG_STATUS, "AppLamp: Worker Started...");
	return (m_thread != nullptr);
}

bool CLimitLess::IsDataAvailable(const SOCKET /*sock*/)
{
	fd_set fds;
	int n;
	struct timeval tv;

	// Set up the file descriptor set.
	FD_ZERO(&fds);
	FD_SET(m_RemoteSocket, &fds);

	// Set up the struct timeval for the timeout.
	tv.tv_sec = 1;
	tv.tv_usec = 0;

	// Wait until timeout or data received.
	n = select(m_RemoteSocket + 1, &fds, nullptr, nullptr, &tv);
	return (n > 0);
}

bool CLimitLess::GetV6BridgeID()
{
	m_BridgeID_1 = m_BridgeID_2 = 0;
	int totRetries = 0;

	sendto(m_RemoteSocket, (const char*)&V6_GetSessionID, sizeof(V6_GetSessionID), 0, (struct sockaddr*) & m_stRemoteDestAddr, sizeof(sockaddr_in));
	while (totRetries < v6_repeats)
	{
		if (!IsDataAvailable(m_RemoteSocket))
		{
			totRetries++;
			sendto(m_RemoteSocket, (const char*)&V6_GetSessionID, sizeof(V6_GetSessionID), 0, (struct sockaddr*) & m_stRemoteDestAddr, sizeof(sockaddr_in));
			continue;
		}
		uint8_t rbuffer[1024];
		sockaddr_in si_other;
		socklen_t slen = sizeof(sockaddr_in);
		sleep_milliseconds(100);
		int trecv = recvfrom(m_RemoteSocket, (char*)&rbuffer, sizeof(rbuffer), 0, (struct sockaddr*) & si_other, &slen);

		if (trecv < 1)
		{
			return false;
		}
		if (trecv != 0x16)
			while ((rbuffer[0x00] == 0x88) && (rbuffer[0x07] == 0x01))
			{
				totRetries++;
				trecv = recvfrom(m_RemoteSocket, (char*)&rbuffer, sizeof(rbuffer), 0, (struct sockaddr*) & si_other, &slen);
			}
		//return false;
		if ((rbuffer[0x00] != 0x28) || (rbuffer[0x15] != 0x00))
			return false;
/*
		uint8_t mac_1 = rbuffer[0x07];
		uint8_t mac_2 = rbuffer[0x08];
		uint8_t mac_3 = rbuffer[0x09];
		uint8_t mac_4 = rbuffer[0x0A];
		uint8_t mac_5 = rbuffer[0x0B];
		uint8_t mac_6 = rbuffer[0x0C];
*/
		m_BridgeID_1 = rbuffer[0x13];
		m_BridgeID_2 = rbuffer[0x14];
		return true;
	}
	return false;
}

bool CLimitLess::SendV6Command(const uint8_t* pCmd)
{
	if (m_BridgeID_1 == 0)
	{
		//No Bridge ID received yet, try getting it
		if (!GetV6BridgeID())
			return false;
	}
	uint8_t OutBuffer[100];
	uint8_t RBuffer[100];
	int wPointer = 0;
	memcpy(OutBuffer + wPointer, V6_PreAmble, sizeof(V6_PreAmble)); wPointer += sizeof(V6_PreAmble);
	OutBuffer[wPointer++] = m_BridgeID_1;
	OutBuffer[wPointer++] = m_BridgeID_2;
	OutBuffer[wPointer++] = 0x00;
	OutBuffer[wPointer++] = (uint8_t)(m_CommandCntr++);
	OutBuffer[wPointer++] = 0x00;

	int wPointerCmd = wPointer;
	memcpy(OutBuffer + wPointer, pCmd, 10); wPointer += 10;

	OutBuffer[wPointer++] = 0x00;

	//Calculate checksum
	uint8_t crc = 0;
	for (int ii = 0; ii < wPointer - wPointerCmd; ii++)
		crc = (crc + OutBuffer[wPointerCmd + ii]) & 0xFF;
	OutBuffer[wPointer++] = crc;

	sendto(m_RemoteSocket, (const char*)&OutBuffer, wPointer, 0, (struct sockaddr*) & m_stRemoteDestAddr, sizeof(sockaddr_in));
	sleep_milliseconds(100);
	//	return true;
	int totRetries = 0;
	while (totRetries < v6_repeats)
	{
		if (!IsDataAvailable(m_RemoteSocket))
		{
			sendto(m_RemoteSocket, (const char*)&OutBuffer, wPointer, 0, (struct sockaddr*) & m_stRemoteDestAddr, sizeof(sockaddr_in));
			totRetries++;
			continue;
		}
		sockaddr_in si_other;
		socklen_t slen = sizeof(sockaddr_in);
		int trecv = recvfrom(m_RemoteSocket, (char*)&RBuffer, sizeof(RBuffer), 0, (struct sockaddr*) & si_other, &slen);
		//Hack to workaround other communication get from bridge, should solved more clear
		//8000000015ACCF23F581D8050200340000000000000000000034
		while ((RBuffer[0x00] == 0x80) && (RBuffer[0x04] == 0x15)) {
			trecv = recvfrom(m_RemoteSocket, (char*)&RBuffer, sizeof(RBuffer), 0, (struct sockaddr*) & si_other, &slen);
		}
		if (trecv < 1)
			return false;
		if (RBuffer[0x07] != 0x00)
		{
			//Maybe the Bridge was turned off, try getting the ID again
			if (GetV6BridgeID())
			{
				OutBuffer[5] = m_BridgeID_1;
				OutBuffer[6] = m_BridgeID_2;
				sendto(m_RemoteSocket, (const char*)&OutBuffer, wPointer, 0, (struct sockaddr*) & m_stRemoteDestAddr, sizeof(sockaddr_in));
				//Hack to flush buffer and see if command succeeded
				recvfrom(m_RemoteSocket, (char*)&RBuffer, sizeof(RBuffer), 0, (struct sockaddr*) & si_other, &slen);
				if (RBuffer[0x07] != 0x00)
				{
					_log.Log(LOG_ERROR, "AppLamp: Error sending command to Bridge!...");
				}
				else return true;
				//return (IsDataAvailable(m_RemoteSocket));
			}
			else
			{
				_log.Log(LOG_ERROR, "AppLamp: Invalid command send to Bridge!...");
			}
			return false;
		}
		return true;
	}
	_log.Log(LOG_ERROR, "AppLamp: Error sending command to Bridge!...");
	return false;
}

bool CLimitLess::StopHardware()
{
	if (m_thread)
	{
		RequestStop();
		m_thread->join();
		m_thread.reset();
	}
	m_bIsStarted = false;
	return true;
}

void CLimitLess::Do_Work()
{
	int sec_counter = 0;
	bool bDoInit = true;

	while (!IsStopRequested(1000))
	{
		sec_counter++;

		if (bDoInit)
		{
			bDoInit = false;
			if (m_BridgeType == LBTYPE_V6)
			{
				if (!GetV6BridgeID())
				{
					_log.Log(LOG_ERROR, "AppLamp: Bridge not found, check IP Address/Port!...");
					_log.Log(LOG_ERROR, "AppLamp: Worker stopped!...");
					return;
				}
				_log.Log(LOG_STATUS, "AppLamp: Bridge found!...");
			}
		}

		if (sec_counter % 12 == 0) {
			m_LastHeartbeat = mytime(nullptr);
		}
	}
	if (m_RemoteSocket != INVALID_SOCKET)
	{
		closesocket(m_RemoteSocket);
		m_RemoteSocket = INVALID_SOCKET;
	}

	_log.Log(LOG_STATUS, "AppLamp: Worker stopped...");
}

void CLimitLess::Send_V6_RGBWW_On(const uint8_t dunit, const long delay)
{
	unsigned char* pCMD;
	if (dunit == 5)
		pCMD = (unsigned char*)&V6_BridgeOn;
	else {
		pCMD = (unsigned char*)&V6_RGBWW_On;
		pCMD[0x09] = dunit;
	}
	SendV6Command(pCMD);
	sleep_milliseconds(delay);
}

void CLimitLess::Send_V6_RGBW_On(const uint8_t dunit, const long delay)
{
	unsigned char* pCMD;
	if (dunit == 5)
		pCMD = (unsigned char*)&V6_BridgeOn;
	else {
		pCMD = (unsigned char*)&V6_RGBW_On;
		pCMD[0x09] = dunit;
	}
	SendV6Command(pCMD);
	sleep_milliseconds(delay);
}

void CLimitLess::Send_V4V5_RGBW_On(const uint8_t dunit, const long delay)
{
	if (dunit == 0)
		sendto(m_RemoteSocket, (const char*)&RGBWOn, 3, 0, (struct sockaddr*) & m_stRemoteDestAddr, sizeof(sockaddr_in));
	else if (dunit == 1)
		sendto(m_RemoteSocket, (const char*)&RGBWGroup1AllOn, 3, 0, (struct sockaddr*) & m_stRemoteDestAddr, sizeof(sockaddr_in));
	else if (dunit == 2)
		sendto(m_RemoteSocket, (const char*)&RGBWGroup2AllOn, 3, 0, (struct sockaddr*) & m_stRemoteDestAddr, sizeof(sockaddr_in));
	else if (dunit == 3)
		sendto(m_RemoteSocket, (const char*)&RGBWGroup3AllOn, 3, 0, (struct sockaddr*) & m_stRemoteDestAddr, sizeof(sockaddr_in));
	else if (dunit == 4)
		sendto(m_RemoteSocket, (const char*)&RGBWGroup4AllOn, 3, 0, (struct sockaddr*) & m_stRemoteDestAddr, sizeof(sockaddr_in));
	sleep_milliseconds(delay);
}

bool CLimitLess::WriteToHardware(const char* pdata, const unsigned char /*length*/)
{
	const _tColorSwitch* pLed = reinterpret_cast<const _tColorSwitch*>(pdata);
	unsigned char *pCMD = nullptr;

	if (m_BridgeType == LBTYPE_V6)
	{
		if (pLed->subtype == sTypeColor_RGB_CW_WW)
		{
			switch (pLed->command)
			{
			case Color_LedOn:
			{
				//Send ON, sleep 100ms
				Send_V6_RGBWW_On(pLed->dunit, 100);
				break;
			}
			case Color_LedOff:
			{
				if (pLed->dunit == 5)
					pCMD = (unsigned char*)&V6_BridgeOff;
				else {
					pCMD = (unsigned char*)&V6_RGBWW_Off;
					pCMD[0x09] = pLed->dunit;
				}
				break;
			}
			case Color_SetColor:
			{
				//Color temperature is not for bridge
				if (pLed->color.mode == ColorModeTemp && pLed->dunit == 5)
				{
					return false;
				}

				//Send ON, sleep 100ms
				Send_V6_RGBWW_On(pLed->dunit, 100);

				//Send the command, sleep 100ms
				if (pLed->color.mode == ColorModeWhite)
				{
					if (pLed->dunit == 5)
						pCMD = (unsigned char*)&V6_Bridge_White_On;
					else {
						pCMD = (unsigned char*)&V6_RGBWW_White_On;
						pCMD[0x09] = pLed->dunit;
					}
				}
				else if (pLed->color.mode == ColorModeTemp)
				{
					pCMD = (unsigned char*)&V6_RGBWW_SetKelvinLevel;
					pCMD[0x05] = (255 - pLed->color.t) * 100 / 255;
					pCMD[0x09] = pLed->dunit;
				}
				else if (pLed->color.mode == ColorModeRGB)
				{
					if (pLed->dunit == 5)
						pCMD = (unsigned char*)&V6_Bridge_SetColor;
					else
						pCMD = (unsigned char*)&V6_RGBWW_SetColor;
					float hsb[3];
					rgb2hsb(pLed->color.r, pLed->color.g, pLed->color.b, hsb);
					pCMD[0x05] = (unsigned char)(hsb[0] * 255.0F);
					pCMD[0x06] = (unsigned char)(hsb[0] * 255.0F);
					pCMD[0x07] = (unsigned char)(hsb[0] * 255.0F);
					pCMD[0x08] = (unsigned char)(hsb[0] * 255.0F);
					if (pLed->dunit != 5)
						pCMD[0x09] = pLed->dunit;
				}
				else {
					_log.Log(LOG_STATUS, "AppLamp: SetRGBColour - Color mode %d is unhandled, if you have a suggestion for what it should do, please post on the Domoticz forum", pLed->color.mode);
				}
				SendV6Command(pCMD);
				sleep_milliseconds(100);

				//Send brightness
				if (pLed->dunit == 5)
					pCMD = (unsigned char*)&V6_Bridge_SetBrightnessLevel;
				else
					pCMD = (unsigned char*)&V6_RGBWW_SetBrightnessLevel;
				pCMD[0x05] = pLed->value;
				if (pLed->dunit != 5)
					pCMD[0x09] = pLed->dunit;

				break;
			}
			case Color_SetBrightnessLevel:
			{
				//First send ON , sleep 100ms, then the command
				Send_V6_RGBWW_On(pLed->dunit, 100);
				if (pLed->dunit == 5)
					pCMD = (unsigned char*)&V6_Bridge_SetBrightnessLevel;
				else
					pCMD = (unsigned char*)&V6_RGBWW_SetBrightnessLevel;
				pCMD[0x05] = pLed->value;
				if (pLed->dunit != 5)
					pCMD[0x09] = pLed->dunit;
				break;
			}
			case Color_SetColorToWhite:
			{
				//First send ON , sleep 100ms, then the command
				Send_V6_RGBWW_On(pLed->dunit, 100);
				if (pLed->dunit == 5)
					pCMD = (unsigned char*)&V6_Bridge_White_On;
				else {
					pCMD = (unsigned char*)&V6_RGBWW_White_On;
					pCMD[0x09] = pLed->dunit;
				}
				break;
			}
			case Color_NightMode:
			{
				if (pLed->dunit == 5) {
					//First put on
					pCMD = (unsigned char*)&V6_BridgeOn;
					SendV6Command(pCMD);
					sleep_milliseconds(50);
					//Second set to white
					pCMD = (unsigned char*)&V6_Bridge_White_On;
					SendV6Command(pCMD);
					sleep_milliseconds(50);
					//Third set to 2 percent brightness
					pCMD = (unsigned char*)&V6_Bridge_SetBrightnessLevel;
					pCMD[0x05] = 0x01;
				}
				else {
					pCMD = (unsigned char*)&V6_RGBWW_Night_On;
					pCMD[0x09] = pLed->dunit;
				}
				break;
			}
			case Color_DiscoSpeedSlower:
			{
				if (pLed->dunit == 5)
					pCMD = (unsigned char*)&V6_Bridge_Mode_Speed_Down;
				else {
					pCMD = (unsigned char*)&V6_RGBWW_Mode_Speed_Down;
					pCMD[0x09] = pLed->dunit;
				}
				break;
			case Color_DiscoSpeedFaster:
				if (pLed->dunit == 5)
					pCMD = (unsigned char*)&V6_Bridge_Mode_Speed_Up;
				else {
					pCMD = (unsigned char*)&V6_RGBWW_Mode_Speed_Up;
					pCMD[0x09] = pLed->dunit;
				}
				break;
			}
			case Color_DiscoMode:
			{
				//First send ON , sleep 100ms, then the command
				Send_V6_RGBWW_On(pLed->dunit, 100);
				if (pLed->dunit == 5)
					pCMD = (unsigned char*)&V6_Bridge_Disco_Mode;
				else {
					pCMD = (unsigned char*)&V6_RGBWW_Disco_Mode;
					pCMD[0x09] = pLed->dunit;
				}
				break;
			}
			case Color_DiscoMode_1:
			case Color_DiscoMode_2:
			case Color_DiscoMode_3:
			case Color_DiscoMode_4:
			case Color_DiscoMode_5:
			case Color_DiscoMode_6:
			case Color_DiscoMode_7:
			case Color_DiscoMode_8:
			case Color_DiscoMode_9:
			{
				//First send ON , sleep 100ms, then the command
				Send_V6_RGBWW_On(pLed->dunit, 100);
				unsigned char mode = pLed->command - Color_DiscoMode_1 + 1;
				if (pLed->dunit == 5) {
					pCMD = (unsigned char*)&V6_Bridge_Disco_Mode;
					pCMD[0X05] = mode;
				}
				else {
					pCMD = (unsigned char*)&V6_RGBWW_Disco_Mode;
					pCMD[0x09] = pLed->dunit;
					pCMD[0X05] = mode;
				}
				break;
			}
			}
		}
		else if (pLed->subtype == sTypeColor_RGB_W)
		{
			switch (pLed->command)
			{
			case Color_LedOn:
			{
				//Send ON, sleep 100ms
				Send_V6_RGBW_On(pLed->dunit, 100);
				break;
			}
			case Color_LedOff:
			{
				if (pLed->dunit == 5)
					pCMD = (unsigned char*)&V6_BridgeOff;
				else {
					pCMD = (unsigned char*)&V6_RGBW_Off;
					pCMD[0x09] = pLed->dunit;
				}
				break;
			}
			case Color_SetColor:
			{
				//Send ON, sleep 100ms
				Send_V6_RGBW_On(pLed->dunit, 100);

				//Send the command, sleep 100ms
				if (pLed->color.mode == ColorModeWhite)
				{
					if (pLed->dunit == 5)
						pCMD = (unsigned char*)&V6_Bridge_White_On;
					else {
						pCMD = (unsigned char*)&V6_RGBW_White_On;
						pCMD[0x09] = pLed->dunit;
					}
				}
				else if (pLed->color.mode == ColorModeRGB)
				{
					if (pLed->dunit == 5)
						pCMD = (unsigned char*)&V6_Bridge_SetColor;
					else
						pCMD = (unsigned char*)&V6_RGBW_SetColor;
					float hsb[3];
					rgb2hsb(pLed->color.r, pLed->color.g, pLed->color.b, hsb);
					pCMD[0x05] = (unsigned char)(hsb[0] * 255.0F);
					pCMD[0x06] = (unsigned char)(hsb[0] * 255.0F);
					pCMD[0x07] = (unsigned char)(hsb[0] * 255.0F);
					pCMD[0x08] = (unsigned char)(hsb[0] * 255.0F);
					if (pLed->dunit != 5)
						pCMD[0x09] = pLed->dunit;
				}
				else {
					_log.Log(LOG_STATUS, "AppLamp: SetRGBColour - Color mode %d is unhandled, if you have a suggestion for what it should do, please post on the Domoticz forum", pLed->color.mode);
				}
				SendV6Command(pCMD);
				sleep_milliseconds(100);

				//Send brightness
				if (pLed->dunit == 5)
					pCMD = (unsigned char*)&V6_Bridge_SetBrightnessLevel;
				else
					pCMD = (unsigned char*)&V6_RGBW_SetBrightnessLevel;
				pCMD[0x05] = pLed->value;
				if (pLed->dunit != 5)
					pCMD[0x09] = pLed->dunit;

				break;
			}
			case Color_SetBrightnessLevel:
			{
				//First send ON , sleep 100ms, then the command
				Send_V6_RGBW_On(pLed->dunit, 100);
				if (pLed->dunit == 5)
					pCMD = (unsigned char*)&V6_Bridge_SetBrightnessLevel;
				else
					pCMD = (unsigned char*)&V6_RGBW_SetBrightnessLevel;
				pCMD[0x05] = pLed->value;
				if (pLed->dunit != 5)
					pCMD[0x09] = pLed->dunit;
				break;
			}
			case Color_SetColorToWhite:
			{
				//First send ON , sleep 100ms, then the command
				Send_V6_RGBW_On(pLed->dunit, 100);
				if (pLed->dunit == 5)
					pCMD = (unsigned char*)&V6_Bridge_White_On;
				else {
					pCMD = (unsigned char*)&V6_RGBW_White_On;
					pCMD[0x09] = pLed->dunit;
				}
				break;
			}
			case Color_NightMode:
			{
				if (pLed->dunit == 5)
					return false;
				pCMD = (unsigned char*)&V6_RGBW_Night_On;
				pCMD[0x09] = pLed->dunit;
				break;
			}
			case Color_DiscoSpeedSlower:
			{
				if (pLed->dunit == 5)
					return false;
				pCMD = (unsigned char*)&V6_RGBW_Mode_Speed_Down;
				pCMD[0x09] = pLed->dunit;
				break;
			}
			case Color_DiscoSpeedFaster:
			{
				if (pLed->dunit == 5)
					return false;
				pCMD = (unsigned char*)&V6_RGBW_Mode_Speed_Up;
				pCMD[0x09] = pLed->dunit;
				break;
			}
			case Color_DiscoMode:
			{
				//First send ON , sleep 100ms, then the command
				Send_V6_RGBW_On(pLed->dunit, 100);
				if (pLed->dunit == 5)
					return false;
				pCMD = (unsigned char*)&V6_RGBW_Disco_Mode;
				pCMD[0x09] = pLed->dunit;
				break;
			}
			case Color_DiscoMode_1:
			case Color_DiscoMode_2:
			case Color_DiscoMode_3:
			case Color_DiscoMode_4:
			case Color_DiscoMode_5:
			case Color_DiscoMode_6:
			case Color_DiscoMode_7:
			case Color_DiscoMode_8:
			case Color_DiscoMode_9:
			{
				//First send ON , sleep 100ms, then the command
				Send_V6_RGBW_On(pLed->dunit, 100);
				if (pLed->dunit == 5)
					return false;
				unsigned char mode = pLed->command - Color_DiscoMode_1 + 1;
				pCMD = (unsigned char*)&V6_RGBW_Disco_Mode;
				pCMD[0x09] = pLed->dunit;
				pCMD[0x05] = mode;
				break;
			}
			/* test for brighthup
						case Color_SetBrightUp:
						{
							if (pLed->dunit == 5) {
								pCMD = (unsigned char*)&V6_Bridge_SetBrightnessLevel;
								if (pLed->value <= 0x5F) {
									pCMD[0x05] = (pLed->value + 0x05);
								}
								else {
									pCMD[0x05] = 0x64;
								}
							}
							else {
								pCMD = (unsigned char*)&V6_RGBW_SetBrightnessLevel;
								if (pLed->value <= 0x5F) {
									pCMD[0x05] = (pLed->value + 0x05);
								}
								else {
									pCMD[0x05] = 0x64;
								}
								pCMD[0x09] = pLed->dunit;
							}
							break;
						}
			*/
			}
		}
		/*
				else if (m_LEDType == sTypeColor_RGB)
				{
					switch (pLed->command)
					{
					case Color_LedOn:
						pCMD = (unsigned char*)&RGBOn;
						break;
					case Color_LedOff:
						pCMD = (unsigned char*)&RGBOff;
						break;
					case Color_SetColor:
					{
						//First send ON , sleep 100ms, then the command
						sendto(m_RemoteSocket, (const char*)&RGBOn, 3, 0, (struct sockaddr*)&m_stRemoteDestAddr, sizeof(sockaddr_in));
						sleep_milliseconds(100);
						//The Hue is inverted/swifted 90 degrees
						int iHue = ((255 - pLed->value) + 192) & 0xFF;
						RGBSetColour[1] = (unsigned char)iHue;
						pCMD = (unsigned char*)&RGBSetColour;
					}
					break;
					case Color_SetBrightUp:
						//First send ON , sleep 100ms, then the command
						sendto(m_RemoteSocket, (const char*)&RGBOn, 3, 0, (struct sockaddr*)&m_stRemoteDestAddr, sizeof(sockaddr_in));
						sleep_milliseconds(100);
						pCMD = (unsigned char*)&RGBBrightnessUp;
						break;
					case Color_SetBrightDown:
						//First send ON , sleep 100ms, then the command
						sendto(m_RemoteSocket, (const char*)&RGBOn, 3, 0, (struct sockaddr*)&m_stRemoteDestAddr, sizeof(sockaddr_in));
						sleep_milliseconds(100);
						pCMD = (unsigned char*)&RGBBrightnessDown;
						break;
					case Color_DiscoSpeedSlower:
						//First send ON , sleep 100ms, then the command
						sendto(m_RemoteSocket, (const char*)&RGBOn, 3, 0, (struct sockaddr*)&m_stRemoteDestAddr, sizeof(sockaddr_in));
						sleep_milliseconds(100);
						pCMD = (unsigned char*)&RGBDiscoSpeedSlower;
						break;
					case Color_DiscoSpeedFaster:
						//First send ON , sleep 100ms, then the command
						sendto(m_RemoteSocket, (const char*)&RGBOn, 3, 0, (struct sockaddr*)&m_stRemoteDestAddr, sizeof(sockaddr_in));
						sleep_milliseconds(100);
						pCMD = (unsigned char*)&RGBDiscoSpeedFaster;
						break;
					case Color_DiscoSpeedFasterLong:
						//First send ON , sleep 100ms, then the command
						sendto(m_RemoteSocket, (const char*)&RGBOn, 3, 0, (struct sockaddr*)&m_stRemoteDestAddr, sizeof(sockaddr_in));
						sleep_milliseconds(100);
						pCMD = (unsigned char*)&RGBDiscoSpeedFasterLong;
						break;
					case Color_DiscoMode:
						sendto(m_RemoteSocket, (const char*)&RGBOn, 3, 0, (struct sockaddr*)&m_stRemoteDestAddr, sizeof(sockaddr_in));
						sleep_milliseconds(100);
						pCMD = (unsigned char*)&RGBWDiscoMode;
						break;
					case Color_RGBDiscoNext:
						//First send ON , sleep 100ms, then the command
						sendto(m_RemoteSocket, (const char*)&RGBOn, 3, 0, (struct sockaddr*)&m_stRemoteDestAddr, sizeof(sockaddr_in));
						sleep_milliseconds(100);
						pCMD = (unsigned char*)&RGBDiscoNext;
						break;
					case Color_RGBDiscoPrevious:
						//First send ON , sleep 100ms, then the command
						sendto(m_RemoteSocket, (const char*)&RGBOn, 3, 0, (struct sockaddr*)&m_stRemoteDestAddr, sizeof(sockaddr_in));
						sleep_milliseconds(100);
						pCMD = (unsigned char*)&RGBDiscoPrevious;
						break;
					}
				}
				else if (m_LEDType == sTypeColor_White)
				{
					switch (pLed->command)
					{
					case Color_LedOn:
						if (pLed->dunit == 0)
							pCMD = (unsigned char*)&AllOn;
						else if (pLed->dunit == 1)
							pCMD = (unsigned char*)&Group1On;
						else if (pLed->dunit == 2)
							pCMD = (unsigned char*)&Group2On;
						else if (pLed->dunit == 3)
							pCMD = (unsigned char*)&Group3On;
						else if (pLed->dunit == 4)
							pCMD = (unsigned char*)&Group4On;
						break;
					case Color_LedOff:
						if (pLed->dunit == 0)
							pCMD = (unsigned char*)&AllOff;
						else if (pLed->dunit == 1)
							pCMD = (unsigned char*)&Group1Off;
						else if (pLed->dunit == 2)
							pCMD = (unsigned char*)&Group2Off;
						else if (pLed->dunit == 3)
							pCMD = (unsigned char*)&Group3Off;
						else if (pLed->dunit == 4)
							pCMD = (unsigned char*)&Group4Off;
						break;
					case Color_SetBrightUp:
						pCMD = (unsigned char*)&WhiteBrightnessUp;
						break;
					case Color_SetBrightDown:
						pCMD = (unsigned char*)&WhiteBrightnessDown;
						break;
					case Color_WarmWhiteIncrease:
						pCMD = (unsigned char*)&WhiteWarmer;
						break;
					case Color_CoolWhiteIncrease:
						pCMD = (unsigned char*)&WhiteCooler;
						break;
					case Color_SetColorToWhite:
						if (pLed->dunit == 0) {
							pCMD = (unsigned char*)&AllFull;
						}
						else if (pLed->dunit == 1) {
							pCMD = (unsigned char*)&Group1Full;
						}
						else if (pLed->dunit == 2) {
							pCMD = (unsigned char*)&Group2Full;
						}
						else if (pLed->dunit == 3) {
							pCMD = (unsigned char*)&Group3Full;
						}
						else if (pLed->dunit == 4) {
							pCMD = (unsigned char*)&Group4Full;
						}
						break;
					case Color_NightMode:
						if (pLed->dunit == 0) {
							pCMD = (unsigned char*)&AllNight;
						}
						else if (pLed->dunit == 1) {
							pCMD = (unsigned char*)&Group1Night;
						}
						else if (pLed->dunit == 2) {
							pCMD = (unsigned char*)&Group2Night;
						}
						else if (pLed->dunit == 3) {
							pCMD = (unsigned char*)&Group3Night;
						}
						else if (pLed->dunit == 4) {
							pCMD = (unsigned char*)&Group4Night;
						}
						break;
					}
				}
		*/
		if (pCMD != nullptr)
		{
			SendV6Command(pCMD);
			sleep_milliseconds(100);
		}
		return true;
	}

	//V4 / V5
	if (m_LEDType == sTypeColor_RGB_W)
	{
		switch (pLed->command)
		{
		case Color_LedOn:
			//Send ON, sleep 100ms
			Send_V4V5_RGBW_On(pLed->dunit, 100);
			break;
		case Color_LedOff:
			if (pLed->dunit == 0)
				pCMD = (unsigned char*)&RGBWOff;
			else if (pLed->dunit == 1)
				pCMD = (unsigned char*)&RGBWGroup1AllOff;
			else if (pLed->dunit == 2)
				pCMD = (unsigned char*)&RGBWGroup2AllOff;
			else if (pLed->dunit == 3)
				pCMD = (unsigned char*)&RGBWGroup3AllOff;
			else if (pLed->dunit == 4)
				pCMD = (unsigned char*)&RGBWGroup4AllOff;
			break;
		case Color_SetColor:
		{
			//Send ON, sleep 100ms
			Send_V4V5_RGBW_On(pLed->dunit, 100);

			//Send brightness, sleep 100ms
			//convert brightness (0-100) to (0-50) to 0-59
			double dval = (59.0 / 100.0) * float(pLed->value / 2);
			int ival = round(dval);
			if (ival < 2)
				ival = 2;
			if (ival > 27)
				ival = 27;
			RGBWSetBrightnessLevel[1] = (unsigned char)ival;
			pCMD = (unsigned char*)&RGBWSetBrightnessLevel;
			sendto(m_RemoteSocket, (const char*)pCMD, 3, 0, (struct sockaddr*) & m_stRemoteDestAddr, sizeof(sockaddr_in));
			sleep_milliseconds(100);

			//Send the command
			if (pLed->color.mode == ColorModeWhite)
			{
				if (pLed->dunit == 0) {
					pCMD = (unsigned char*)&RGBWSetColorToWhiteAll;
				}
				else if (pLed->dunit == 1) {
					pCMD = (unsigned char*)&RGBWSetColorToWhiteGroup1;
				}
				else if (pLed->dunit == 2) {
					pCMD = (unsigned char*)&RGBWSetColorToWhiteGroup2;
				}
				else if (pLed->dunit == 3) {
					pCMD = (unsigned char*)&RGBWSetColorToWhiteGroup3;
				}
				else if (pLed->dunit == 4) {
					pCMD = (unsigned char*)&RGBWSetColorToWhiteGroup4;
				}
			}
			else if (pLed->color.mode == ColorModeRGB)
			{
				// Convert RGB to HSV
				float hsb[3];
				rgb2hsb(pLed->color.r, pLed->color.g, pLed->color.b, hsb);
				int iHue = (unsigned char)(hsb[0] * 255.0F);
				//The Hue is inverted/swifted 90 degrees
				iHue = ((255 - iHue) + 192) & 0xFF;
				RGBWSetColor[1] = (unsigned char)iHue;
				pCMD = (unsigned char*)&RGBWSetColor;
			}
			else {
				_log.Log(LOG_STATUS, "AppLamp: SetRGBColour - Color mode %d is unhandled, if you have a suggestion for what it should do, please post on the Domoticz forum", pLed->color.mode);
			}
		}
		break;
		case Color_DiscoSpeedSlower:
		{
			//First send ON , sleep 100ms, then the command
			Send_V4V5_RGBW_On(pLed->dunit, 100);

			pCMD = (unsigned char*)&RGBWDiscoSpeedSlower;
			break;
		}
		case Color_DiscoSpeedFaster:
		{
			//First send ON , sleep 100ms, then the command
			Send_V4V5_RGBW_On(pLed->dunit, 100);

			pCMD = (unsigned char*)&RGBWDiscoSpeedFaster;
			break;
		}
		case Color_DiscoMode:
		{
			//First send ON , sleep 100ms, then the command
			Send_V4V5_RGBW_On(pLed->dunit, 100);

			pCMD = (unsigned char*)&RGBWDiscoMode;
			break;
		}
		case Color_DiscoSpeedMinimal:
		{
			//First send ON , sleep 100ms, then the command
			Send_V4V5_RGBW_On(pLed->dunit, 100);

			//It takes at max 8 speed clicks to get the minimal speed
			for (int i = 1; i < 8; i++) {
				sendto(m_RemoteSocket, (const char*)&RGBWDiscoSpeedSlower, 3, 0, (struct sockaddr*) & m_stRemoteDestAddr, sizeof(sockaddr_in));
				sleep_milliseconds(50);
			}

			pCMD = (unsigned char*)&RGBWDiscoSpeedSlower;
			break;
		}
		case Color_DiscoSpeedMaximal:
		{
			//First send ON , sleep 100ms, then the command
			Send_V4V5_RGBW_On(pLed->dunit, 100);

			//It takes at max 8 speed clicks to get the minimal speed
			for (int i = 1; i < 8; i++) {
				sendto(m_RemoteSocket, (const char*)&RGBWDiscoSpeedFaster, 3, 0, (struct sockaddr*) & m_stRemoteDestAddr, sizeof(sockaddr_in));
				sleep_milliseconds(50);
			}

			pCMD = (unsigned char*)&RGBWDiscoSpeedFaster;
			break;
		}
		case Color_DiscoMode_1:
		case Color_DiscoMode_2:
		case Color_DiscoMode_3:
		case Color_DiscoMode_4:
		case Color_DiscoMode_5:
		case Color_DiscoMode_6:
		case Color_DiscoMode_7:
		case Color_DiscoMode_8:
		case Color_DiscoMode_9:
		{
			//First send ON , sleep 100ms, then the command
			Send_V4V5_RGBW_On(pLed->dunit, 100);

			//reset disco mode
			if (pLed->dunit == 0)
				sendto(m_RemoteSocket, (const char*)&RGBWSetColorToWhiteAll, 3, 0, (struct sockaddr*) & m_stRemoteDestAddr, sizeof(sockaddr_in));
			else if (pLed->dunit == 1)
				sendto(m_RemoteSocket, (const char*)&RGBWSetColorToWhiteGroup1, 3, 0, (struct sockaddr*) & m_stRemoteDestAddr, sizeof(sockaddr_in));
			else if (pLed->dunit == 2)
				sendto(m_RemoteSocket, (const char*)&RGBWSetColorToWhiteGroup2, 3, 0, (struct sockaddr*) & m_stRemoteDestAddr, sizeof(sockaddr_in));
			else if (pLed->dunit == 3)
				sendto(m_RemoteSocket, (const char*)&RGBWSetColorToWhiteGroup3, 3, 0, (struct sockaddr*) & m_stRemoteDestAddr, sizeof(sockaddr_in));
			else if (pLed->dunit == 4)
				sendto(m_RemoteSocket, (const char*)&RGBWSetColorToWhiteGroup4, 3, 0, (struct sockaddr*) & m_stRemoteDestAddr, sizeof(sockaddr_in));
			sleep_milliseconds(100);

			unsigned char mode = pLed->command - Color_DiscoMode_1 + 1;
			for (int i = 0; i < mode; i++) {
				sendto(m_RemoteSocket, (const char*)&RGBWDiscoMode, 3, 0, (struct sockaddr*) & m_stRemoteDestAddr, sizeof(sockaddr_in));
				sleep_milliseconds(50);
			}
			break;
		}
		case Color_NightMode:
		{
			//First send Off hack for keeping this stable, sleep 100ms, then the command
			if (pLed->dunit == 0)
				sendto(m_RemoteSocket, (const char*)&RGBWOff, 3, 0, (struct sockaddr*) & m_stRemoteDestAddr, sizeof(sockaddr_in));
			else if (pLed->dunit == 1)
				sendto(m_RemoteSocket, (const char*)&RGBWGroup1AllOff, 3, 0, (struct sockaddr*) & m_stRemoteDestAddr, sizeof(sockaddr_in));
			else if (pLed->dunit == 2)
				sendto(m_RemoteSocket, (const char*)&RGBWGroup2AllOff, 3, 0, (struct sockaddr*) & m_stRemoteDestAddr, sizeof(sockaddr_in));
			else if (pLed->dunit == 3)
				sendto(m_RemoteSocket, (const char*)&RGBWGroup3AllOff, 3, 0, (struct sockaddr*) & m_stRemoteDestAddr, sizeof(sockaddr_in));
			else if (pLed->dunit == 4)
				sendto(m_RemoteSocket, (const char*)&RGBWGroup4AllOff, 3, 0, (struct sockaddr*) & m_stRemoteDestAddr, sizeof(sockaddr_in));
			sleep_milliseconds(100);

			if (pLed->dunit == 0)
				pCMD = (unsigned char*)&RGBWAllNightMode;
			else if (pLed->dunit == 1)
				pCMD = (unsigned char*)&RGBWGroup1NightMode;
			else if (pLed->dunit == 2)
				pCMD = (unsigned char*)&RGBWGroup2NightMode;
			else if (pLed->dunit == 3)
				pCMD = (unsigned char*)&RGBWGroup3NightMode;
			else if (pLed->dunit == 4)
				pCMD = (unsigned char*)&RGBWGroup4NightMode;
			break;
		}
		case Color_SetColorToWhite:
			//First send ON , sleep 100ms, then the command
			Send_V4V5_RGBW_On(pLed->dunit, 100);
			if (pLed->dunit == 0) {
				pCMD = (unsigned char*)&RGBWSetColorToWhiteAll;
			}
			else if (pLed->dunit == 1) {
				pCMD = (unsigned char*)&RGBWSetColorToWhiteGroup1;
			}
			else if (pLed->dunit == 2) {
				pCMD = (unsigned char*)&RGBWSetColorToWhiteGroup2;
			}
			else if (pLed->dunit == 3) {
				pCMD = (unsigned char*)&RGBWSetColorToWhiteGroup3;
			}
			else if (pLed->dunit == 4) {
				pCMD = (unsigned char*)&RGBWSetColorToWhiteGroup4;
			}
			break;
		case Color_SetBrightnessLevel:
		{
			//First send ON , sleep 100ms, then the command
			Send_V4V5_RGBW_On(pLed->dunit, 100);
			//convert brightness (0-100) to (0-50) to 0-59
			double dval = (59.0 / 100.0) * float(pLed->value / 2);
			int ival = round(dval);
			if (ival < 2)
				ival = 2;
			if (ival > 27)
				ival = 27;
			RGBWSetBrightnessLevel[1] = (unsigned char)ival;
			pCMD = (unsigned char*)&RGBWSetBrightnessLevel;
		}
		break;
		}
	}
	else if (m_LEDType == sTypeColor_RGB)
	{
		switch (pLed->command)
		{
		case Color_LedOn:
			pCMD = (unsigned char*)&RGBOn;
			break;
		case Color_LedOff:
			pCMD = (unsigned char*)&RGBOff;
			break;
		case Color_SetColor:
		{
			if (pLed->color.mode == ColorModeRGB)
			{
				//First send ON , sleep 100ms, then the command
				sendto(m_RemoteSocket, (const char*)&RGBOn, 3, 0, (struct sockaddr*) & m_stRemoteDestAddr, sizeof(sockaddr_in));
				sleep_milliseconds(100);
				// Convert RGB to HSV
				float hsb[3];
				rgb2hsb(pLed->color.r, pLed->color.g, pLed->color.b, hsb);
				int iHue = (unsigned char)(hsb[0] * 255.0F);
				//The Hue is inverted/swifted 90 degrees
				iHue = ((255 - iHue) + 192) & 0xFF;
				RGBSetColour[1] = (unsigned char)iHue;
				pCMD = (unsigned char*)&RGBSetColour;
			}
			else {
				_log.Log(LOG_STATUS, "AppLamp: SetRGBColour - Color mode %d is unhandled, if you have a suggestion for what it should do, please post on the Domoticz forum", pLed->color.mode);
			}
		}
		break;
		case Color_SetBrightUp:
			//First send ON , sleep 100ms, then the command
			sendto(m_RemoteSocket, (const char*)&RGBOn, 3, 0, (struct sockaddr*) & m_stRemoteDestAddr, sizeof(sockaddr_in));
			sleep_milliseconds(100);
			pCMD = (unsigned char*)&RGBBrightnessUp;
			break;
		case Color_SetBrightDown:
			//First send ON , sleep 100ms, then the command
			sendto(m_RemoteSocket, (const char*)&RGBOn, 3, 0, (struct sockaddr*) & m_stRemoteDestAddr, sizeof(sockaddr_in));
			sleep_milliseconds(100);
			pCMD = (unsigned char*)&RGBBrightnessDown;
			break;
		case Color_DiscoSpeedSlower:
			//First send ON , sleep 100ms, then the command
			sendto(m_RemoteSocket, (const char*)&RGBOn, 3, 0, (struct sockaddr*) & m_stRemoteDestAddr, sizeof(sockaddr_in));
			sleep_milliseconds(100);
			pCMD = (unsigned char*)&RGBDiscoSpeedSlower;
			break;
		case Color_DiscoSpeedFaster:
			//First send ON , sleep 100ms, then the command
			sendto(m_RemoteSocket, (const char*)&RGBOn, 3, 0, (struct sockaddr*) & m_stRemoteDestAddr, sizeof(sockaddr_in));
			sleep_milliseconds(100);
			pCMD = (unsigned char*)&RGBDiscoSpeedFaster;
			break;
		case Color_DiscoSpeedFasterLong:
			//First send ON , sleep 100ms, then the command
			sendto(m_RemoteSocket, (const char*)&RGBOn, 3, 0, (struct sockaddr*) & m_stRemoteDestAddr, sizeof(sockaddr_in));
			sleep_milliseconds(100);
			pCMD = (unsigned char*)&RGBDiscoSpeedFasterLong;
			break;
		case Color_DiscoMode:
			sendto(m_RemoteSocket, (const char*)&RGBOn, 3, 0, (struct sockaddr*) & m_stRemoteDestAddr, sizeof(sockaddr_in));
			sleep_milliseconds(100);
			pCMD = (unsigned char*)&RGBWDiscoMode;
			break;
		case Color_RGBDiscoNext:
			//First send ON , sleep 100ms, then the command
			sendto(m_RemoteSocket, (const char*)&RGBOn, 3, 0, (struct sockaddr*) & m_stRemoteDestAddr, sizeof(sockaddr_in));
			sleep_milliseconds(100);
			pCMD = (unsigned char*)&RGBDiscoNext;
			break;
		case Color_RGBDiscoPrevious:
			//First send ON , sleep 100ms, then the command
			sendto(m_RemoteSocket, (const char*)&RGBOn, 3, 0, (struct sockaddr*) & m_stRemoteDestAddr, sizeof(sockaddr_in));
			sleep_milliseconds(100);
			pCMD = (unsigned char*)&RGBDiscoPrevious;
			break;
		}
	}
	else if (m_LEDType == sTypeColor_White || m_LEDType == sTypeColor_CW_WW)
	{
		switch (pLed->command)
		{
		case Color_LedOn:
			if (pLed->dunit == 0)
				pCMD = (unsigned char*)&AllOn;
			else if (pLed->dunit == 1)
				pCMD = (unsigned char*)&Group1On;
			else if (pLed->dunit == 2)
				pCMD = (unsigned char*)&Group2On;
			else if (pLed->dunit == 3)
				pCMD = (unsigned char*)&Group3On;
			else if (pLed->dunit == 4)
				pCMD = (unsigned char*)&Group4On;
			break;
		case Color_LedOff:
			if (pLed->dunit == 0)
				pCMD = (unsigned char*)&AllOff;
			else if (pLed->dunit == 1)
				pCMD = (unsigned char*)&Group1Off;
			else if (pLed->dunit == 2)
				pCMD = (unsigned char*)&Group2Off;
			else if (pLed->dunit == 3)
				pCMD = (unsigned char*)&Group3Off;
			else if (pLed->dunit == 4)
				pCMD = (unsigned char*)&Group4Off;
			break;
		case Color_SetBrightUp:
			pCMD = (unsigned char*)&WhiteBrightnessUp;
			break;
		case Color_SetBrightDown:
			pCMD = (unsigned char*)&WhiteBrightnessDown;
			break;
		case Color_WarmWhiteIncrease:
			pCMD = (unsigned char*)&WhiteWarmer;
			break;
		case Color_CoolWhiteIncrease:
			pCMD = (unsigned char*)&WhiteCooler;
			break;
		case Color_SetColorToWhite:
			if (pLed->dunit == 0) {
				pCMD = (unsigned char*)&AllFull;
			}
			else if (pLed->dunit == 1) {
				pCMD = (unsigned char*)&Group1Full;
			}
			else if (pLed->dunit == 2) {
				pCMD = (unsigned char*)&Group2Full;
			}
			else if (pLed->dunit == 3) {
				pCMD = (unsigned char*)&Group3Full;
			}
			else if (pLed->dunit == 4) {
				pCMD = (unsigned char*)&Group4Full;
			}
			break;
		case Color_NightMode:
			if (pLed->dunit == 0) {
				pCMD = (unsigned char*)&AllNight;
			}
			else if (pLed->dunit == 1) {
				pCMD = (unsigned char*)&Group1Night;
			}
			else if (pLed->dunit == 2) {
				pCMD = (unsigned char*)&Group2Night;
			}
			else if (pLed->dunit == 3) {
				pCMD = (unsigned char*)&Group3Night;
			}
			else if (pLed->dunit == 4) {
				pCMD = (unsigned char*)&Group4Night;
			}
			break;
		}
	}

	if (pCMD != nullptr)
	{
		sendto(m_RemoteSocket, (const char*)pCMD, 3, 0, (struct sockaddr*) & m_stRemoteDestAddr, sizeof(sockaddr_in));
		sleep_milliseconds(100);
	}
	return true;
}

//Webserver helpers
namespace http {
	namespace server {
		void CWebServer::SetLimitlessType(WebEmSession& session, const request& req, std::string& redirect_uri)
		{
			redirect_uri = "/index.html";
			if (session.rights != 2)
			{
				session.reply_status = reply::forbidden;
				return; //Only admin user allowed
			}
			std::string idx = request::findValue(&req, "idx");
			if (idx.empty())
			{
				return;
			}

			std::vector<std::vector<std::string> > result;

			result = m_sql.safe_query("SELECT Mode1, Mode2, Mode3, Mode4, Mode5, Mode6 FROM Hardware WHERE (ID='%q')", idx.c_str());
			if (result.empty())
				return;

			int Mode1 = atoi(request::findValue(&req, "LimitlessType").c_str());
			int Mode2 = atoi(request::findValue(&req, "CCBridgeType").c_str());
			int Mode3 = atoi(result[0][2].c_str());
			int Mode4 = atoi(result[0][3].c_str());
			m_sql.UpdateRFXCOMHardwareDetails(atoi(idx.c_str()), Mode1, Mode2, Mode3, Mode4, 0, 0);

			m_mainworker.RestartHardware(idx);
		}
	} // namespace server
} // namespace http
