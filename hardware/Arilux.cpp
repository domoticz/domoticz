#include "stdafx.h"
#include "Arilux.h"
#include "../hardware/hardwaretypes.h"
#include "../main/Helper.h"
#include "../main/HTMLSanitizer.h"
#include "../main/localtime_r.h"
#include "../main/Logger.h"
#include "../main/mainworker.h"
#include "../main/SQLHelper.h"
#include "../main/WebServer.h"
#include "../webserver/cWebem.h"
#include <json/json.h>

#include <numeric>

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
  m_color.r = 0xff;
  m_color.g = 0xff;
  m_color.b = 0xff;
  m_color.ww = 0xff;
}

bool Arilux::StartHardware()
{
	RequestStart();

	//force connect the next first time
	m_bIsStarted = true;

	//Start worker thread
	m_thread = std::make_shared<std::thread>(&Arilux::Do_Work, this);
	SetThreadNameInt(m_thread->native_handle());

	return (m_thread != nullptr);
}

bool Arilux::StopHardware()
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

#define Arilux_POLL_INTERVAL 60

void Arilux::Do_Work()
{
	Log(LOG_STATUS, "Worker started...");

	int sec_counter = Arilux_POLL_INTERVAL - 5;
	while (!IsStopRequested(1000))
	{
		sec_counter++;
		if (sec_counter % 12 == 0) {
			m_LastHeartbeat = mytime(nullptr);
		}
	}
	Log(LOG_STATUS, "Worker stopped...");
}

void Arilux::InsertUpdateSwitch(const std::string &lightName, const int subType, const std::string &location)
{
	uint32_t sID;
	try {
		sID = boost::asio::ip::address_v4::from_string(location).to_ulong();
	} catch (const std::exception &e) {
		Log(LOG_ERROR, "Bad IP address: %s (%s)", location.c_str(), e.what());
		return;
	}
	char szDeviceID[20];
	sprintf(szDeviceID, "%08X", sID);

	std::vector<std::vector<std::string>> result;
	result = m_sql.safe_query("SELECT nValue, LastLevel FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='%q') AND (Type==%d) AND (SubType==%d)", m_HwdID, szDeviceID, pTypeColorSwitch, subType);
	if (result.empty())
	{
		Log(LOG_STATUS, "New controller added (%s/%s)", location.c_str(), lightName.c_str());
		_tColorSwitch ycmd;
		ycmd.subtype = (uint8_t)subType;
		ycmd.id = sID;
		ycmd.dunit = 0;
		ycmd.value = 0;
		ycmd.command = Color_LedOff;
		m_mainworker.PushAndWaitRxMessage(this, (const unsigned char *)&ycmd, nullptr, -1, m_Name.c_str());
		m_sql.safe_query("UPDATE DeviceStatus SET Name='%q', switchType=%d WHERE(HardwareID == %d) AND (DeviceID == '%q')", lightName.c_str(), STYPE_Dimmer, m_HwdID, szDeviceID);
	}
}

bool Arilux::SendTCPCommand(uint32_t ip,std::vector<unsigned char> &command)
{
	Log(LOG_STATUS, "Sending data to device");

	//Add checksum calculation
	int sum = std::accumulate(command.begin(), command.end(), 0);
	sum = sum & 0xFF;
	command.push_back((unsigned char)sum);

	boost::asio::io_service io_service;
	boost::asio::ip::tcp::socket sendSocket(io_service);
	boost::asio::ip::address_v4 address(ip);
	boost::asio::ip::tcp::endpoint endpoint(address, 5577);
	try
	{
		sendSocket.connect(endpoint);
	}
	catch (const std::exception &e)
	{
		Log(LOG_ERROR, "Exception: %s", e.what());
		return false;
	}

	//Log(LOG_STATUS, "Connection OK");
	sleep_milliseconds(50);

	boost::asio::write(sendSocket, boost::asio::buffer(command, command.size()));
	//Log(LOG_STATUS, "Command sent");
	sleep_milliseconds(50);

	return true;

}


bool Arilux::WriteToHardware(const char *pdata, const unsigned char /*length*/)
{
	Debug(DEBUG_HARDWARE, "WriteToHardware...............................");
	const _tColorSwitch *pLed = reinterpret_cast<const _tColorSwitch*>(pdata);

	//Code for On/Off and RGB command
	auto Arilux_On_Command = std::vector<unsigned char>{ 0x71, 0x23, 0x0f };
	auto Arilux_Off_Command = std::vector<unsigned char>{ 0x71, 0x24, 0x0f };
	auto Arilux_RGBCommand_Command = std::vector<unsigned char>{ 0x31, 0x00, 0x00, 0x00, 0x00, 0xf0, 0x0f };

	switch (pLed->command)
	{
	case Color_LedOn:
		return SendTCPCommand(pLed->id, Arilux_On_Command);
	case Color_LedOff:
		return SendTCPCommand(pLed->id, Arilux_Off_Command);
	case Color_SetColorToWhite:
		Arilux_RGBCommand_Command[1] = m_color.r = 0xff;
		Arilux_RGBCommand_Command[2] = m_color.g = 0xff;
		Arilux_RGBCommand_Command[3] = m_color.b = 0xff;
		Arilux_RGBCommand_Command[4] = m_color.ww = 0xff;
		return SendTCPCommand(pLed->id, Arilux_RGBCommand_Command);
	case Color_SetColor:
		if (pLed->color.mode == ColorModeWhite)
		{
			m_color.r = 0xff;
			m_color.g = 0xff;
			m_color.b = 0xff;
      m_color.ww = 0xff;
		}
		else if (pLed->color.mode == ColorModeRGB)
		{
			m_color.r = pLed->color.r;
			m_color.g = pLed->color.g;
			m_color.b = pLed->color.b;
      m_color.ww = 0;
		}
		else if (pLed->color.mode == ColorModeCustom)
		{
			m_color.r = pLed->color.r;
			m_color.g = pLed->color.g;
			m_color.b = pLed->color.b;
      m_color.ww = pLed->color.ww;
		}
		else {
			Log(LOG_ERROR, "SetRGBColour - Color mode %d is unhandled, if you have a suggestion for what it should do, please post on the Domoticz forum", pLed->color.mode);
      return false;
		}
		// No break, fall through to send combined color + brightness command
	case Color_SetBrightnessLevel:
		Arilux_RGBCommand_Command[1] = static_cast<uint8_t>(m_color.r * pLed->value / 100);
		Arilux_RGBCommand_Command[2] = static_cast<uint8_t>(m_color.g * pLed->value / 100);
		Arilux_RGBCommand_Command[3] = static_cast<uint8_t>(m_color.b * pLed->value / 100);
		Arilux_RGBCommand_Command[4] = static_cast<uint8_t>(m_color.ww * pLed->value / 100);
		return SendTCPCommand(pLed->id, Arilux_RGBCommand_Command);
	}
	Log(LOG_STATUS, "This command is unhandled, if you have a suggestion for what it should do, please post on the Domoticz forum");
	return false;
}

//Webserver helpers
namespace http {
	namespace server {
		void CWebServer::Cmd_AddArilux(WebEmSession & /*session*/, const request& req, Json::Value &root)
		{
			root["title"] = "AddArilux";

			std::string idx = request::findValue(&req, "idx");
			std::string sname = HTMLSanitizer::Sanitize(request::findValue(&req, "name"));
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
			Arilux.InsertUpdateSwitch(sname, (stype == "0") ? sTypeColor_RGB : sTypeColor_RGB_W_Z, sipaddress);
		}
	} // namespace server
} // namespace http
