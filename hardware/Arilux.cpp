#include "stdafx.h"
#include "Arilux.h"
#include "../main/Logger.h"
#include "../main/Helper.h"
#include "../main/SQLHelper.h"
#include "../main/localtime_r.h"
#include "../hardware/hardwaretypes.h"
#include "../main/mainworker.h"
#include "../main/WebServer.h"
#include "../webserver/cWebem.h"
#include "../json/json.h"

#include <numeric>
#include <boost/array.hpp>
#include <boost/asio.hpp>
#include <boost/bind.hpp>



/*
Arilux AL-C0x is a Wifi LED Controller based on ESP8266.
Also compatible with Flux LED or any other LED strip controlled by Smart Home App
Multiple version, RGB,RGBWW, with or witout remote controll
Price is from 5 to 20 euro
Protocol if WiFi and TCP, the rgb controller needs to be connected to your wireless network
Domoticz and the lights need to be in the same network/subnet
Based on christianTF boblightwifi plugin for Arilux and Belville Flux Led python plugin
*/


Arilux::Arilux(const int ID)
{
	m_HwdID = ID;
	m_bDoRestart = false;
	m_stoprequested = false;
	cHue = 0.0;
	brightness = 255;
	isWhite = true;

}

Arilux::~Arilux(void)
{
}

bool Arilux::StartHardware()
{
	m_stoprequested = false;
	m_bDoRestart = false;

	//force connect the next first time
	m_bIsStarted = true;

	//Start worker thread
	m_thread = boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&Arilux::Do_Work, this)));

	return (m_thread != NULL);
}

bool Arilux::StopHardware()
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

#define Arilux_POLL_INTERVAL 60

void Arilux::Do_Work()
{
	_log.Log(LOG_STATUS, "Arilux Worker started...");

	
	int sec_counter = Arilux_POLL_INTERVAL - 5;
	while (!m_stoprequested)
	{
		sleep_seconds(1);
		sec_counter++;
		if (sec_counter % 12 == 0) {
			m_LastHeartbeat = mytime(NULL);
		}		
	}
	_log.Log(LOG_STATUS, "Arilux stopped");
}


void Arilux::InsertUpdateSwitch(const std::string &nodeID, const std::string &lightName, const int &YeeType, const std::string &Location, const bool bIsOn, const std::string &ariluxBright, const std::string &ariluxHue)
{
	std::vector<std::string> ipaddress;
	StringSplit(Location, ".", ipaddress);
	if (ipaddress.size() != 4)
	{
		_log.Log(LOG_STATUS, "Arilux: Invalid location received! (No IP Address)");
		return;
	}
	uint32_t sID = (uint32_t)(atoi(ipaddress[0].c_str()) << 24) | (uint32_t)(atoi(ipaddress[1].c_str()) << 16) | (atoi(ipaddress[2].c_str()) << 8) | atoi(ipaddress[3].c_str());
	char szDeviceID[20];
	if (sID == 1)
		sprintf(szDeviceID, "%d", 1);
	else
		sprintf(szDeviceID, "%08X", (unsigned int)sID);

	bool tIsOn = !(bIsOn);
	std::vector<std::vector<std::string> > result;
	result = m_sql.safe_query("SELECT nValue, LastLevel FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='%q') AND (Type==%d) AND (SubType==%d)", m_HwdID, szDeviceID, pTypeLimitlessLights, YeeType);
	if (result.size() < 1)
	{
		_log.Log(LOG_STATUS, "Arilux: New controller added (%s/%s)", Location.c_str(), lightName.c_str());
		int value = atoi(ariluxBright.c_str());
		int cmd = light1_sOn;
		//int level = 100;
		if (!bIsOn) {
			cmd = light1_sOff;
			//level = 0;
		}
		_tLimitlessLights ycmd;
		ycmd.len = sizeof(_tLimitlessLights) - 1;
		ycmd.type = pTypeLimitlessLights;
		ycmd.subtype = YeeType;
		ycmd.id = sID;
		ycmd.dunit = 0;
		ycmd.value = value;
		ycmd.command = cmd;
		m_mainworker.PushAndWaitRxMessage(this, (const unsigned char *)&ycmd, NULL, -1);
		m_sql.safe_query("UPDATE DeviceStatus SET Name='%q', SwitchType=%d, LastLevel=%d WHERE(HardwareID == %d) AND (DeviceID == '%q')", lightName.c_str(), (STYPE_Dimmer), value, m_HwdID, szDeviceID);
	}
	else {

		int nvalue = atoi(result[0][0].c_str());
		tIsOn = (nvalue != 0);
		int lastLevel = atoi(result[0][1].c_str());
		int value = atoi(ariluxBright.c_str());
		if ((bIsOn != tIsOn) || (value != lastLevel))
		{
			int cmd = Limitless_LedOn;
			if (!bIsOn) {
				cmd = Limitless_LedOff;
			}
			if (value != lastLevel) {
				cmd = Limitless_SetBrightnessLevel;
			}
			_tLimitlessLights ycmd;
			ycmd.len = sizeof(_tLimitlessLights) - 1;
			ycmd.type = pTypeLimitlessLights;
			ycmd.subtype = YeeType;
			ycmd.id = sID;
			ycmd.dunit = 0;
			ycmd.value = value;
			ycmd.command = cmd;
			m_mainworker.PushAndWaitRxMessage(this, (const unsigned char *)&ycmd, NULL, -1);
		}
	}
}

bool Arilux::SendTCPCommand(char ip[50],std::vector<unsigned char> &command)
{
	_log.Log(LOG_STATUS, "Arilux: Sending data to device");

	//Add checksum calculation
	int sum = std::accumulate(command.begin(), command.end(), 0);
	sum = sum & 0xFF;
	command.push_back((unsigned char)sum);

	boost::asio::io_service io_service;
	boost::asio::ip::tcp::socket sendSocket(io_service);
	boost::asio::ip::tcp::resolver resolver(io_service);
	boost::asio::ip::tcp::resolver::query query(boost::asio::ip::tcp::v4(), ip, "5577");
	boost::asio::ip::tcp::resolver::iterator iterator = resolver.resolve(query);


	
	
	try
	{
		//_log.Log(LOG_STATUS, "Arilux: Try connecting device.");
		boost::asio::connect(sendSocket, iterator);
	}
	catch (const std::exception &e)
	{
		_log.Log(LOG_ERROR, "Arilux: Exception: %s", e.what());
		return false;
	}

	//_log.Log(LOG_STATUS, "Arilux: Connection OK");
	sleep_milliseconds(50);
	
	boost::asio::write(sendSocket, boost::asio::buffer(command, command.size()));
	//_log.Log(LOG_STATUS, "Arilux: Command sent");
	sleep_milliseconds(50);

	/*sendSocket.shutdown(boost::asio::ip::tcp::socket::shutdown_both);
	sendSocket.close();
	io_service.stop();*/

	return true;

}


bool Arilux::WriteToHardware(const char *pdata, const unsigned char length)
{
	_log.Log(LOG_STATUS, "Arilux: WriteToHardware...............................");
	_tLimitlessLights *pLed = (_tLimitlessLights*)pdata;
	uint8_t command = pLed->command;
	std::vector<std::vector<std::string> > result;

	unsigned long lID;
	char ipAddress[50];
	bool sendOnFirst = false;

	if (pLed->id == 1)
		sprintf(ipAddress, "%d", 1);
	else
		sprintf(ipAddress, "%08X", (unsigned int)pLed->id);
	std::string ID = ipAddress;

	std::stringstream s_strid;
	s_strid << std::hex << ID;
	s_strid >> lID;

	unsigned char ID1 = (unsigned char)((lID & 0xFF000000) >> 24);
	unsigned char ID2 = (unsigned char)((lID & 0x00FF0000) >> 16);
	unsigned char ID3 = (unsigned char)((lID & 0x0000FF00) >> 8);
	unsigned char ID4 = (unsigned char)((lID & 0x000000FF));

	//IP Address
	sprintf(ipAddress, "%d.%d.%d.%d", ID1, ID2, ID3, ID4);


	//Code for On/Off and RGB command
	unsigned char Arilux_On_Command_Tab[] = { 0x71, 0x23, 0x0f };
	unsigned char Arilux_Off_Command_Tab[] = { 0x71, 0x24, 0x0f };
	unsigned char Arilux_RGBCommand_Command_Tab[] = { 0x31, 0x00, 0x00, 0x00, 0x00, 0xf0, 0x0f };

	//C++98 Vector Init
	std::vector<unsigned char> Arilux_On_Command(Arilux_On_Command_Tab, Arilux_On_Command_Tab+sizeof(Arilux_On_Command_Tab)/sizeof(unsigned char));
	std::vector<unsigned char> Arilux_Off_Command(Arilux_Off_Command_Tab, Arilux_Off_Command_Tab + sizeof(Arilux_Off_Command_Tab) / sizeof(unsigned char));
	std::vector<unsigned char> Arilux_RGBCommand_Command(Arilux_RGBCommand_Command_Tab, Arilux_RGBCommand_Command_Tab + sizeof(Arilux_RGBCommand_Command_Tab) / sizeof(unsigned char));
	

	


	std::vector<unsigned char> commandToSend;
	
	switch (pLed->command)
	{
	case Limitless_LedOn:
		commandToSend = Arilux_On_Command;
		
		break;
	case Limitless_LedOff:
		commandToSend = Arilux_Off_Command;		
		break;
	
	case Limitless_SetColorToWhite:
		sendOnFirst = true;
		isWhite = true;
		Arilux_RGBCommand_Command[1] = 0xff;
		Arilux_RGBCommand_Command[2] = 0xff;
		Arilux_RGBCommand_Command[3] = 0xff;
		Arilux_RGBCommand_Command[4] = 0xff;
		commandToSend = Arilux_RGBCommand_Command;
		break;	
	case Limitless_SetRGBColour: {
		isWhite = false;
		cHue = (360.0f / 255.0f)*float(pLed->value);//hue given was in range of 0-255 - Store Hue value to object
		//Sending is done by SetBrightnessLevel
	}
    break;
	case Limitless_SetBrightnessLevel: {

		int red, green, blue;
		int BrightnessBase = 100;
		int dMax_convert = (int)(round((255.0f / 100.0f)*float(BrightnessBase)));
		BrightnessBase = (int)pLed->value;
		int dMax_Send = (int)(round((255.0f / 100.0f)*float(BrightnessBase)));
		hue2rgb(cHue, red, green, blue, dMax_convert);


		if (isWhite)
		{
			

			Arilux_RGBCommand_Command[1] = (unsigned char)0xff;
			Arilux_RGBCommand_Command[2] = (unsigned char)0xff;
			Arilux_RGBCommand_Command[3] = (unsigned char)0xff;
			Arilux_RGBCommand_Command[4] = (unsigned char)dMax_Send;

			brightness = dMax_Send;

			commandToSend = Arilux_RGBCommand_Command;
		}
		else 
		{
			//_log.Log(LOG_NORM, "Red: %03d, Green:%03d, Blue:%03d, Brightness:%03d", red, green, blue, dMax_Send);

			Arilux_RGBCommand_Command[1] = (unsigned char)red;
			Arilux_RGBCommand_Command[2] = (unsigned char)green;
			Arilux_RGBCommand_Command[3] = (unsigned char)blue;
			Arilux_RGBCommand_Command[4] = (unsigned char)dMax_Send;

			brightness = dMax_Send;

			commandToSend = Arilux_RGBCommand_Command;
		}
	}
	break;
	case Limitless_SetBrightUp:	
		_log.Log(LOG_STATUS, "Arilux: SetBrightUp - This command is unhandled, if you have a suggestion for what it should do, please post on the Domoticz forum");
		break;
	case Limitless_SetBrightDown:
		_log.Log(LOG_STATUS, "Arilux: SetBrightDown - This command is unhandled, if you have a suggestion for what it should do, please post on the Domoticz forum");
		break;
	case Limitless_WarmWhiteIncrease:
		_log.Log(LOG_STATUS, "Arilux: WarmWhiteIncrease - This command is unhandled, if you have a suggestion for what it should do, please post on the Domoticz forum");
		break;
	case Limitless_CoolWhiteIncrease:
		_log.Log(LOG_STATUS, "Arilux: CoolWhiteIncrease - This command is unhandled, if you have a suggestion for what it should do, please post on the Domoticz forum");
		break;
	case Limitless_NightMode:
		_log.Log(LOG_STATUS, "Arilux: NightMode - This command is unhandled, if you have a suggestion for what it should do, please post on the Domoticz forum");
		break;
	case Limitless_FullBrightness: 
		_log.Log(LOG_STATUS, "Arilux: FullBrightness - This command is unhandled, if you have a suggestion for what it should do, please post on the Domoticz forum");
		break;
	case Limitless_DiscoMode:
		_log.Log(LOG_STATUS, "Arilux: DiscoMode - This command is unhandled, if you have a suggestion for what it should do, please post on the Domoticz forum");
		break;
	case Limitless_DiscoSpeedFasterLong:
		_log.Log(LOG_STATUS, "Arilux: DiscoSpeedFasterLong - This command is unhandled, if you have a suggestion for what it should do, please post on the Domoticz forum");
		break;
	default:
		
		break;
	}


	if (commandToSend.empty())return false; //No command to send


	

	


	bool returnValue = false;


	if (sendOnFirst) 
	{
		returnValue= SendTCPCommand(ipAddress,Arilux_On_Command);
	}

	returnValue= SendTCPCommand(ipAddress,commandToSend);
	
	return returnValue;
}

//Webserver helpers
namespace http {
	namespace server {
		void CWebServer::Cmd_AddArilux(WebEmSession & session, const request& req, Json::Value &root)
		{
			root["title"] = "AddArilux";

			std::string idx = request::findValue(&req, "idx");
			std::string sname = request::findValue(&req, "name");
			std::string sipaddress = request::findValue(&req, "ipaddress");
			std::string stype = request::findValue(&req, "stype");
			if (
				(idx.empty()) ||
				(sname.empty()) ||
				(sipaddress.empty()) ||
				(stype.empty())
				)
				return;
			root["status"] = "OK";

			int HwdID = atoi(idx.c_str());

			Arilux Arilux(HwdID);
			Arilux.InsertUpdateSwitch("123", sname, (stype == "0") ? sTypeLimitlessRGB : sTypeLimitlessRGBW, sipaddress, false, "0", "0");
		}
	}
}
