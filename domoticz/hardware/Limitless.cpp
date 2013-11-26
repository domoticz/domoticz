#include "stdafx.h"
#include "Limitless.h"
#include "../main/Helper.h"
#include "../main/Logger.h"
#include "../main/mainworker.h"
#include "hardwaretypes.h"

#define round(a) ( int ) ( a + .5 )

//This hardware goes under a few different names, i was talled the original name was AppLamp

// Commands
// White LEDs
const unsigned char AllOn[3] = { 0x35, 0x0, 0x55 };
const unsigned char AllOff[3] = { 0x39, 0x0, 0x55 };
const unsigned char AllNight[3] = { 0xB9, 0x0, 0x55 };
const unsigned char AllFull [3] = { 0xB5, 0x0, 0x55 };

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

const unsigned char RGBSpeedUp[3] = { 0x25, 0x0, 0x55 };
const unsigned char RGBSpeedDown[3] = { 0x26, 0x0, 0x55 };

const unsigned char RGBDiscoSpeedSlower[3] = { 0x26, 0x0, 0x55 };
const unsigned char RGBDiscoSpeedFaster[3] = { 0x25, 0x0, 0x55 };

const unsigned char RGBDiscoNext[3] = { 0x27, 0x0, 0x55 };
const unsigned char RGBDiscoLast[3] = { 0x28, 0x0, 0x55 };

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

const unsigned char RGBWSetColorToWhiteAll[3]={0xC2,0x0,0x55};	//SET COLOR TO WHITE (GROUP ALL) 0x42 100ms followed by: 0xC2
const unsigned char RGBWSetColorToWhiteGroup1[3]={0xC5,0x0,0x55};	//SET COLOR TO WHITE (GROUP 1) 0x45 100ms followed by: 0xC5
const unsigned char RGBWSetColorToWhiteGroup2[3]={0xC7,0x0,0x55};	//SET COLOR TO WHITE (GROUP 2) 0x47 100ms followed by: 0xC7
const unsigned char RGBWSetColorToWhiteGroup3[3]={0xC9,0x0,0x55};	//SET COLOR TO WHITE (GROUP 3) 0x49 100ms followed by: 0xC9
const unsigned char RGBWSetColorToWhiteGroup4[3]={0xCB,0x0,0x55};	//SET COLOR TO WHITE (GROUP 4) 0x4B 100ms followed by: 0xCB

unsigned char RGBWSetBrightnessLevel[3]={0x4E,0,0x55};
//LIMITLESSLED RGBW DIRECT BRIGHTNESS SETTING is by a 3BYTE COMMAND: (First send the Group ON for the group you want to set the brightness for. You send the group ON command 100ms before sending the 4E 00 55)
//Byte1: 0x4E (decimal: 78)
//Byte2: 0×00 to 0xFF (full brightness 0x3B)
//Byte3: Always 0×55 (decimal: 85)

unsigned char RGBWSetColor[3]={0x40,0,0x55};
//LIMITLESSLED RGBW COLOR SETTING is by a 3BYTE COMMAND: (First send the Group ON for the group you want to set the color for. You send the group ON command 100ms before sending the 40)
//Byte1: 0×40 (decimal: 64)
//Byte2: 0×00 to 0xFF (255 colors) Color Matrix Chart [COMING SOON]
//Byte3: Always 0×55 (decimal: 85)

CLimitLess::CLimitLess(const int ID, const int LedType, const std::string IPAddress, const unsigned short usIPPort)
{
	m_HwdID=ID;
	m_szIPAddress=IPAddress;
	m_usIPPort=usIPPort;
	m_stoprequested=false;
	m_RemoteSocket=INVALID_SOCKET;
	m_LEDType=LedType;
#if defined WIN32
	//Init winsock
	WSADATA data;
	WORD version; 

	version = (MAKEWORD(2, 2)); 
	int ret = WSAStartup(version, &data); 
	if (ret != 0) 
	{  
		ret = WSAGetLastError(); 

		if (ret == WSANOTINITIALISED) 
		{  
			MessageBox(0,"Winsock not initialized","Error:",MB_OK); 
		}
	}		
#endif
	Init();
}

CLimitLess::~CLimitLess(void)
{
#if defined WIN32
	WSACleanup();
#endif
}

void CLimitLess::Init()
{
}

bool CLimitLess::AddSwitchIfNotExits(const unsigned char Unit, const std::string& devname)
{
	if (m_pMainWorker==NULL)
		return false;
	std::stringstream szQuery;
	std::vector<std::vector<std::string> > result;
	szQuery << "SELECT ID FROM DeviceStatus WHERE (HardwareID==" << m_HwdID << ") AND (Unit==" << int(Unit) << ") AND (Type==" << pTypeLimitlessLights << ") AND (SubType==" << int(m_LEDType) << ")";
	result=m_pMainWorker->m_sql.query(szQuery.str());
	if (result.size()<1)
	{
		szQuery.clear();
		szQuery.str("");

		szQuery << 
			"INSERT INTO DeviceStatus (HardwareID, DeviceID, Unit, Type, SubType, SignalLevel, BatteryLevel, Name, nValue, sValue) "
			"VALUES (" << m_HwdID << ",'" << int(1) << "'," << int(Unit) << "," << pTypeLimitlessLights << "," <<int(m_LEDType) << ",12,255,'" << devname << "',0,' ')";
		m_pMainWorker->m_sql.query(szQuery.str());
		return false;
	}
	return true;
}

bool CLimitLess::StartHardware()
{
	Init();
	if (m_RemoteSocket!=INVALID_SOCKET)
	{
		closesocket(m_RemoteSocket);
		m_RemoteSocket=INVALID_SOCKET;
	}


	struct hostent *he;
	if ((he=gethostbyname(m_szIPAddress.c_str())) == NULL) {  // get the host info
		_log.Log(LOG_ERROR,"AppLamp: Error with IP address!...");
		return false;
	}
	m_RemoteSocket = socket( AF_INET, SOCK_DGRAM, 0 );

	// this call is what allows broadcast packets to be sent:
	int broadcast = 1;
	if (setsockopt(m_RemoteSocket, SOL_SOCKET, SO_BROADCAST, (const char*)&broadcast, sizeof(broadcast)) == -1) {
			_log.Log(LOG_ERROR,"AppLamp: Error with IP address (SO_BROADCAST)!...");
			return false;
	}

	memset(&m_stRemoteDestAddr,0,sizeof(m_stRemoteDestAddr));
	m_stRemoteDestAddr.sin_family = AF_UNSPEC;
	m_stRemoteDestAddr.sin_family = PF_INET; 
	m_stRemoteDestAddr.sin_family = AF_INET;     // host byte order
	m_stRemoteDestAddr.sin_port = htons(m_usIPPort); // short, network byte order
	m_stRemoteDestAddr.sin_addr = *((struct in_addr *)he->h_addr);
	memset(m_stRemoteDestAddr.sin_zero, '\0', sizeof m_stRemoteDestAddr.sin_zero);

	//Add the Default switches
	if (!AddSwitchIfNotExits(0,"AppLamp All"))
	{
		if (!AddSwitchIfNotExits(1,"AppLamp Group1"))
		{
			if (!AddSwitchIfNotExits(2,"AppLamp Group2"))
			{
				if (!AddSwitchIfNotExits(3,"AppLamp Group3"))
				{
					AddSwitchIfNotExits(4,"AppLamp Group4");
				}
			}
		}
	}
	sOnConnected(this);
	//Start worker thread
	m_thread = boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&CLimitLess::Do_Work, this)));

	_log.Log(LOG_NORM,"AppLamp Worker Started...");

	//WriteToHardware((const char*)&RGBWOn,sizeof(RGBWOn));
	//Sleep(100);
	//WriteToHardware((const char*)&RGBWSetColorToWhiteAll,sizeof(RGBWSetColorToWhiteAll));
	
	//RGBWSetColor[1]=90;
	//WriteToHardware((const char*)&RGBWGroup1AllOn,sizeof(RGBWGroup1AllOn));
	//Sleep(100);
	//WriteToHardware((const char*)&RGBWSetColor,sizeof(RGBWSetColor));
	return (m_thread!=NULL);
}

bool CLimitLess::StopHardware()
{
	/*
    m_stoprequested=true;
	if (m_thread)
		m_thread->join();
	return true;
    */
	if (m_thread!=NULL)
	{
		assert(m_thread);
		m_stoprequested = true;
		m_thread->join();
	}
    if (m_RemoteSocket!=INVALID_SOCKET)
	{
		closesocket(m_RemoteSocket);
		m_RemoteSocket=INVALID_SOCKET;
	}
    return true;
}

void CLimitLess::Do_Work()
{
	while (!m_stoprequested)
	{
		boost::this_thread::sleep(boost::posix_time::seconds(1));
	}
	_log.Log(LOG_NORM,"AppLamp Worker stopped...");
}

void CLimitLess::WriteToHardware(const char *pdata, const unsigned char length)
{
	_tLimitlessLights *pLed=(_tLimitlessLights*)pdata;

	unsigned char *pCMD=NULL;

	if (m_LEDType==sTypeLimitlessRGBW)
	{
		switch (pLed->command)
		{
		case Limitless_LedOn:
			if (pLed->dunit==0)
				pCMD=(unsigned char*)&RGBWOn;
			else if (pLed->dunit==1)
				pCMD=(unsigned char*)&RGBWGroup1AllOn;
			else if (pLed->dunit==2)
				pCMD=(unsigned char*)&RGBWGroup2AllOn;
			else if (pLed->dunit==3)
				pCMD=(unsigned char*)&RGBWGroup3AllOn;
			else if (pLed->dunit==4)
				pCMD=(unsigned char*)&RGBWGroup4AllOn;
			break;
		case Limitless_LedOff:
			if (pLed->dunit==0)
				pCMD=(unsigned char*)&RGBWOff;
			else if (pLed->dunit==1)
				pCMD=(unsigned char*)&RGBWGroup1AllOff;
			else if (pLed->dunit==2)
				pCMD=(unsigned char*)&RGBWGroup2AllOff;
			else if (pLed->dunit==3)
				pCMD=(unsigned char*)&RGBWGroup3AllOff;
			else if (pLed->dunit==4)
				pCMD=(unsigned char*)&RGBWGroup4AllOff;
			break;
		case Limitless_SetRGBColour:
			{
				//First send ON , sleep 100ms, then the command
				if (pLed->dunit==0) {
					sendto(m_RemoteSocket,(const char*)&RGBWOn,3,0,(struct sockaddr*)&m_stRemoteDestAddr, sizeof(sockaddr_in));
					boost::this_thread::sleep(boost::posix_time::milliseconds(100));
				}
				else if (pLed->dunit==1) {
					sendto(m_RemoteSocket,(const char*)&RGBWGroup1AllOn,3,0,(struct sockaddr*)&m_stRemoteDestAddr, sizeof(sockaddr_in));
					boost::this_thread::sleep(boost::posix_time::milliseconds(100));
				}
				else if (pLed->dunit==2) {
					sendto(m_RemoteSocket,(const char*)&RGBWGroup2AllOn,3,0,(struct sockaddr*)&m_stRemoteDestAddr, sizeof(sockaddr_in));
					boost::this_thread::sleep(boost::posix_time::milliseconds(100));
				}
				else if (pLed->dunit==3) {
					sendto(m_RemoteSocket,(const char*)&RGBWGroup3AllOn,3,0,(struct sockaddr*)&m_stRemoteDestAddr, sizeof(sockaddr_in));
					boost::this_thread::sleep(boost::posix_time::milliseconds(100));
				}
				else if (pLed->dunit==4) {
					sendto(m_RemoteSocket,(const char*)&RGBWGroup1AllOn,4,0,(struct sockaddr*)&m_stRemoteDestAddr, sizeof(sockaddr_in));
					boost::this_thread::sleep(boost::posix_time::milliseconds(100));
				}
				//The Hue is inverted/swifted 90 degrees
				int iHue=((255-pLed->value)+192)&0xFF;
				RGBWSetColor[1]=(unsigned char)iHue;
				pCMD=(unsigned char*)&RGBWSetColor;
			}
			break;
		case Limitless_DiscoSpeedSlower:
			pCMD=(unsigned char*)&RGBWDiscoSpeedSlower;
			break;
		case Limitless_DiscoSpeedFaster:
			pCMD=(unsigned char*)&RGBWDiscoSpeedFaster;
			break;
		case Limitless_DiscoMode:
			pCMD=(unsigned char*)&RGBWDiscoMode;
			break;
		case Limitless_SetColorToWhite:
			//First send ON , sleep 100ms, then the command
			if (pLed->dunit==0) {
				sendto(m_RemoteSocket,(const char*)&RGBWOn,3,0,(struct sockaddr*)&m_stRemoteDestAddr, sizeof(sockaddr_in));
				boost::this_thread::sleep(boost::posix_time::milliseconds(100));
				pCMD=(unsigned char*)&RGBWSetColorToWhiteAll;
			}
			else if (pLed->dunit==1) {
				sendto(m_RemoteSocket,(const char*)&RGBWGroup1AllOn,3,0,(struct sockaddr*)&m_stRemoteDestAddr, sizeof(sockaddr_in));
				boost::this_thread::sleep(boost::posix_time::milliseconds(100));
				pCMD=(unsigned char*)&RGBWSetColorToWhiteGroup1;
			}
			else if (pLed->dunit==2) {
				sendto(m_RemoteSocket,(const char*)&RGBWGroup2AllOn,3,0,(struct sockaddr*)&m_stRemoteDestAddr, sizeof(sockaddr_in));
				boost::this_thread::sleep(boost::posix_time::milliseconds(100));
				pCMD=(unsigned char*)&RGBWSetColorToWhiteGroup2;
			}
			else if (pLed->dunit==3) {
				sendto(m_RemoteSocket,(const char*)&RGBWGroup3AllOn,3,0,(struct sockaddr*)&m_stRemoteDestAddr, sizeof(sockaddr_in));
				boost::this_thread::sleep(boost::posix_time::milliseconds(100));
				pCMD=(unsigned char*)&RGBWSetColorToWhiteGroup3;
			}
			else if (pLed->dunit==4) {
				sendto(m_RemoteSocket,(const char*)&RGBWGroup1AllOn,4,0,(struct sockaddr*)&m_stRemoteDestAddr, sizeof(sockaddr_in));
				boost::this_thread::sleep(boost::posix_time::milliseconds(100));
				pCMD=(unsigned char*)&RGBWSetColorToWhiteGroup4;
			}
			pCMD=(unsigned char*)&RGBWSetColorToWhiteAll;
			break;
		case Limitless_SetBrightnessLevel:
			{
				//First send ON , sleep 100ms, then the command
				if (pLed->dunit==0) {
					sendto(m_RemoteSocket,(const char*)&RGBWOn,3,0,(struct sockaddr*)&m_stRemoteDestAddr, sizeof(sockaddr_in));
					boost::this_thread::sleep(boost::posix_time::milliseconds(100));
				}
				else if (pLed->dunit==1) {
					sendto(m_RemoteSocket,(const char*)&RGBWGroup1AllOn,3,0,(struct sockaddr*)&m_stRemoteDestAddr, sizeof(sockaddr_in));
					boost::this_thread::sleep(boost::posix_time::milliseconds(100));
				}
				else if (pLed->dunit==2) {
					sendto(m_RemoteSocket,(const char*)&RGBWGroup2AllOn,3,0,(struct sockaddr*)&m_stRemoteDestAddr, sizeof(sockaddr_in));
					boost::this_thread::sleep(boost::posix_time::milliseconds(100));
				}
				else if (pLed->dunit==3) {
					sendto(m_RemoteSocket,(const char*)&RGBWGroup3AllOn,3,0,(struct sockaddr*)&m_stRemoteDestAddr, sizeof(sockaddr_in));
					boost::this_thread::sleep(boost::posix_time::milliseconds(100));
				}
				else if (pLed->dunit==4) {
					sendto(m_RemoteSocket,(const char*)&RGBWGroup1AllOn,4,0,(struct sockaddr*)&m_stRemoteDestAddr, sizeof(sockaddr_in));
					boost::this_thread::sleep(boost::posix_time::milliseconds(100));
				}
				//convert brightness (0-100) to (0-50) to 0-59
				double dval=(59.0/100.0)*float(pLed->value/2);
				int ival=round(dval);
				if (ival<2)
					ival=2;
				if (ival>27)
					ival=27;
				RGBWSetBrightnessLevel[1]=(unsigned char)ival;
				pCMD=(unsigned char*)&RGBWSetBrightnessLevel;
			}
			break;
		}
	}
	else if (m_LEDType==sTypeLimitlessRGB)
	{
		switch (pLed->command)
		{
		case Limitless_LedOn:
			if (pLed->dunit==0)
				pCMD=(unsigned char*)&RGBOn;
			else if (pLed->dunit==1)
				pCMD=(unsigned char*)&Group1On;
			else if (pLed->dunit==2)
				pCMD=(unsigned char*)&Group2On;
			else if (pLed->dunit==3)
				pCMD=(unsigned char*)&Group3On;
			else if (pLed->dunit==4)
				pCMD=(unsigned char*)&Group4On;
			break;
		case Limitless_LedOff:
			if (pLed->dunit==0)
				pCMD=(unsigned char*)&RGBOff;
			else if (pLed->dunit==1)
				pCMD=(unsigned char*)&Group1Off;
			else if (pLed->dunit==2)
				pCMD=(unsigned char*)&Group2Off;
			else if (pLed->dunit==3)
				pCMD=(unsigned char*)&Group3Off;
			else if (pLed->dunit==4)
				pCMD=(unsigned char*)&Group4Off;
			break;
		case Limitless_SetRGBColour:
			{
				//First send ON , sleep 100ms, then the command
				if (pLed->dunit==0) {
					sendto(m_RemoteSocket,(const char*)&RGBOn,3,0,(struct sockaddr*)&m_stRemoteDestAddr, sizeof(sockaddr_in));
					boost::this_thread::sleep(boost::posix_time::milliseconds(100));
				}
				else if (pLed->dunit==1) {
					sendto(m_RemoteSocket,(const char*)&Group1On,3,0,(struct sockaddr*)&m_stRemoteDestAddr, sizeof(sockaddr_in));
					boost::this_thread::sleep(boost::posix_time::milliseconds(100));
				}
				else if (pLed->dunit==2) {
					sendto(m_RemoteSocket,(const char*)&Group2On,3,0,(struct sockaddr*)&m_stRemoteDestAddr, sizeof(sockaddr_in));
					boost::this_thread::sleep(boost::posix_time::milliseconds(100));
				}
				else if (pLed->dunit==3) {
					sendto(m_RemoteSocket,(const char*)&Group3On,3,0,(struct sockaddr*)&m_stRemoteDestAddr, sizeof(sockaddr_in));
					boost::this_thread::sleep(boost::posix_time::milliseconds(100));
				}
				else if (pLed->dunit==4) {
					sendto(m_RemoteSocket,(const char*)&Group4On,4,0,(struct sockaddr*)&m_stRemoteDestAddr, sizeof(sockaddr_in));
					boost::this_thread::sleep(boost::posix_time::milliseconds(100));
				}
				//The Hue is inverted/swifted 90 degrees
				int iHue=((255-pLed->value)+192)&0xFF;
				RGBWSetColor[1]=(unsigned char)iHue;
				pCMD=(unsigned char*)&RGBSetColour;
			}
			break;
		case Limitless_DiscoSpeedSlower:
			pCMD=(unsigned char*)&RGBDiscoSpeedSlower;
			break;
		case Limitless_DiscoSpeedFaster:
			pCMD=(unsigned char*)&RGBDiscoSpeedFaster;
			break;
		case Limitless_RGBDiscoNext:
			pCMD=(unsigned char*)&RGBDiscoNext;
			break;
		case Limitless_RGBDiscoLast:
			pCMD=(unsigned char*)&RGBDiscoLast;
			break;
		case Limitless_SetColorToWhite:
			if (pLed->dunit==0) {
				pCMD=(unsigned char*)&AllOn;
			}
			else if (pLed->dunit==1) {
				pCMD=(unsigned char*)&Group1Full;
			}
			else if (pLed->dunit==2) {
				pCMD=(unsigned char*)&Group2Full;
			}
			else if (pLed->dunit==3) {
				pCMD=(unsigned char*)&Group3Full;
			}
			else if (pLed->dunit==4) {
				pCMD=(unsigned char*)&Group4Full;
			}
			pCMD=(unsigned char*)&RGBWSetColorToWhiteAll;
			break;
		}
	}
	else if (m_LEDType==sTypeLimitlessWhite)
	{
		switch (pLed->command)
		{
		case Limitless_LedOn:
			if (pLed->dunit==0)
				pCMD=(unsigned char*)&AllOn;
			else if (pLed->dunit==1)
				pCMD=(unsigned char*)&Group1On;
			else if (pLed->dunit==2)
				pCMD=(unsigned char*)&Group2On;
			else if (pLed->dunit==3)
				pCMD=(unsigned char*)&Group3On;
			else if (pLed->dunit==4)
				pCMD=(unsigned char*)&Group4On;
			break;
		case Limitless_LedOff:
			if (pLed->dunit==0)
				pCMD=(unsigned char*)&AllOn;
			else if (pLed->dunit==1)
				pCMD=(unsigned char*)&Group1Off;
			else if (pLed->dunit==2)
				pCMD=(unsigned char*)&Group2Off;
			else if (pLed->dunit==3)
				pCMD=(unsigned char*)&Group3Off;
			else if (pLed->dunit==4)
				pCMD=(unsigned char*)&Group4Off;
			break;
		case Limitless_SetColorToWhite:
			if (pLed->dunit==0) {
				pCMD=(unsigned char*)&AllOn;
			}
			else if (pLed->dunit==1) {
				pCMD=(unsigned char*)&Group1Full;
			}
			else if (pLed->dunit==2) {
				pCMD=(unsigned char*)&Group2Full;
			}
			else if (pLed->dunit==3) {
				pCMD=(unsigned char*)&Group3Full;
			}
			else if (pLed->dunit==4) {
				pCMD=(unsigned char*)&Group4Full;
			}
			pCMD=(unsigned char*)&RGBWSetColorToWhiteAll;
			break;
		}
	}

	if (pCMD!=NULL)
		sendto(m_RemoteSocket,(const char*)pCMD,3,0,(struct sockaddr*)&m_stRemoteDestAddr, sizeof(sockaddr_in));
}
