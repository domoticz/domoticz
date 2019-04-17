#include "stdafx.h"
#include "../hardware/hardwaretypes.h"
#include "../main/localtime_r.h"
#include "../main/Logger.h"
#include "../main/mainworker.h"
#include "../main/Helper.h"
#include "../main/SQLHelper.h"
#include "../main/WebServer.h"
#include "../webserver/cWebem.h"
#include "../json/json.h"
#include "XiaomiGateway.h"
#include <openssl/aes.h>
#include <boost/asio.hpp>
#include <boost/bind.hpp>

#ifndef WIN32
#include <ifaddrs.h>
#endif

/*
Xiaomi (Aqara) makes a smart home gateway/hub that has support
for a variety of Xiaomi sensors.
They can be purchased on AliExpress or other stores at very
competitive prices.
Protocol is Zigbee and WiFi, and the gateway and
Domoticz need to be in the same network/subnet with multicast working
*/

#define round(a) ( int ) ( a + .5 )
// Removing this vector and use unitcode to tell what kind of device each is
//std::vector<std::string> arrAqara_Wired_ID;

std::list<XiaomiGateway*> gatewaylist;
std::mutex gatewaylist_mutex;

XiaomiGateway * XiaomiGateway::GatewayByIp(std::string ip)
{
	XiaomiGateway * ret = NULL;
	{
		std::unique_lock<std::mutex> lock(gatewaylist_mutex);
		std::list<XiaomiGateway*>::iterator    it = gatewaylist.begin();
		for (; it != gatewaylist.end(); it++)
		{
			if ((*it)->GetGatewayIp() == ip)
			{
				ret = (*it);
				break;
			};
		};
	}
	return ret;
}

void XiaomiGateway::AddGatewayToList()
{
	XiaomiGateway * maingw = NULL;
	{
		std::unique_lock<std::mutex> lock(gatewaylist_mutex);
		std::list<XiaomiGateway*>::iterator    it = gatewaylist.begin();
		for (; it != gatewaylist.end(); it++)
		{
			if ((*it)->IsMainGateway())
			{
				maingw = (*it);
				break;
			};
		};

		if (!maingw)
		{
			SetAsMainGateway();
		}
		else
		{
			maingw->UnSetMainGateway();
		}

		gatewaylist.push_back(this);
	}

	if (maingw)
	{
		maingw->Restart();
	}
}

void XiaomiGateway::RemoveFromGatewayList()
{
	XiaomiGateway * maingw = NULL;
	{
		std::unique_lock<std::mutex> lock(gatewaylist_mutex);
		gatewaylist.remove(this);
		if (IsMainGateway())
		{
			UnSetMainGateway();

			if (gatewaylist.begin() != gatewaylist.end())
			{
				std::list<XiaomiGateway*>::iterator    it = gatewaylist.begin();
				maingw = (*it);
			}
		}
	}

	if (maingw)
	{
		maingw->Restart();
	}
}

// Use this function to get local ip addresses via getifaddrs when Boost.Asio approach fails
// Adds the addresses found to the supplied vector and returns the count
// Code from Stack Overflow - https://stackoverflow.com/questions/2146191
int XiaomiGateway::get_local_ipaddr(std::vector<std::string>& ip_addrs)
{
#ifdef WIN32
	return 0;
#else
	struct ifaddrs *myaddrs, *ifa;
	void *in_addr;
	char buf[64];
	int count = 0;

	if (getifaddrs(&myaddrs) != 0)
	{
		_log.Log(LOG_ERROR, "getifaddrs failed! (when trying to determine local ip address)");
		perror("getifaddrs");
		return 0;
	}

	for (ifa = myaddrs; ifa != NULL; ifa = ifa->ifa_next)
	{
		if (ifa->ifa_addr == NULL)
			continue;
		if (!(ifa->ifa_flags & IFF_UP))
			continue;

		switch (ifa->ifa_addr->sa_family)
		{
		case AF_INET:
		{
			struct sockaddr_in *s4 = (struct sockaddr_in *)ifa->ifa_addr;
			in_addr = &s4->sin_addr;
			break;
		}

		case AF_INET6:
		{
			struct sockaddr_in6 *s6 = (struct sockaddr_in6 *)ifa->ifa_addr;
			in_addr = &s6->sin6_addr;
			break;
		}

		default:
			continue;
		}

		if (!inet_ntop(ifa->ifa_addr->sa_family, in_addr, buf, sizeof(buf)))
		{
			_log.Log(LOG_ERROR, "Could not convert to IP address, inet_ntop failed for interface %s", ifa->ifa_name);
		}
		else
		{
			ip_addrs.push_back(buf);
			count++;
		}
	}

	freeifaddrs(myaddrs);
	return count;
#endif
}

XiaomiGateway::XiaomiGateway(const int ID)
{
	m_HwdID = ID;
	m_bDoRestart = false;
	m_ListenPort9898 = false;
}

XiaomiGateway::~XiaomiGateway(void)
{
}

bool XiaomiGateway::WriteToHardware(const char * pdata, const unsigned char length)
{
	const tRBUF *pCmd = reinterpret_cast<const tRBUF *>(pdata);
	unsigned char packettype = pCmd->ICMND.packettype;
	unsigned char subtype = pCmd->ICMND.subtype;
	bool result = true;
	std::string message = "";
	//bool isctrl = false;

	if (m_GatewaySID == "") {
		m_GatewaySID = XiaomiGatewayTokenManager::GetInstance().GetSID(m_GatewayIp);
	}

	if (packettype == pTypeGeneralSwitch) {
		_tGeneralSwitch *xcmd = (_tGeneralSwitch*)pdata;

		char szTmp[50];
		sprintf(szTmp, "%08X", (unsigned int)xcmd->id);
		std::string ID = szTmp;
		std::stringstream s_strid2;
		s_strid2 << std::hex << ID;
		std::string sid = s_strid2.str();
		std::transform(sid.begin(), sid.end(), sid.begin(), ::tolower);
		std::string cmdchannel = "";
		std::string cmdcommand = "";
		std::string cmddevice = "";
		//bool isctrl2 = false;
		std::string sidtemp = sid;
		sidtemp.insert(0, "158d00");
		//for (unsigned i = 0; i < arrAqara_Wired_ID.size(); i++) {
		//	if (arrAqara_Wired_ID[i] == sidtemp) {
		//		//this device is ctrl2..
		//		isctrl2 = true;
		//		isctrl = true;
		//		if (xcmd->unitcode == 1) {
		//			cmdchannel = "\\\"channel_0\\\":";
		//		}
		//		else if (xcmd->unitcode == 2) {
		//			cmdchannel = "\\\"channel_1\\\":";
		//		}
		//	}
		//}
		//if (isctrl2) {
		//	cmddevice = "ctrl_neutral2";
		//}
		//else {
		//	cmddevice = "ctrl_neutral1";
		//}


		if (xcmd->unitcode == 8) {
			cmdchannel = "\\\"channel_0\\\":";
			cmddevice = "ctrl_neutral1";
		}
		else if (xcmd->unitcode == 9) {
			cmdchannel = "\\\"channel_0\\\":";
			cmddevice = "ctrl_neutral2";
		}
		else if (xcmd->unitcode == 10) {
			cmdchannel = "\\\"channel_1\\\":";
			cmddevice = "ctrl_neutral2";
		}
		if (xcmd->cmnd == 0) {
			cmdcommand = "\\\"off";
		}
		else if (xcmd->cmnd == 1) {
			cmdcommand = "\\\"on";
		}

		if (xcmd->unitcode == 8 || xcmd->unitcode == 9 || xcmd->unitcode == 10) {
			message = "{\"cmd\":\"write\",\"model\":\"" + cmddevice + "\",\"sid\":\"158d00" + sid + "\",\"short_id\":0,\"data\":\"{" + cmdchannel + cmdcommand + "\\\",\\\"key\\\":\\\"@gatewaykey\\\"}\" }";
		}
		else if ((xcmd->subtype == sSwitchGeneralSwitch) && (xcmd->unitcode == 1)) {
			std::string command = "on";
			switch (xcmd->cmnd) {
			case gswitch_sOff:
				command = "off";
				break;
			case gswitch_sOn:
				command = "on";
				break;
			default:
				_log.Log(LOG_ERROR, "XiaomiGateway: Unknown command %d", xcmd->cmnd);
				break;
			}
			message = "{\"cmd\":\"write\",\"model\":\"plug\",\"sid\":\"158d00" + sid + "\",\"short_id\":9844,\"data\":\"{\\\"channel_0\\\":\\\"" + command + "\\\",\\\"key\\\":\\\"@gatewaykey\\\"}\" }";
		}
		else if ((xcmd->subtype == sSwitchTypeSelector) && (xcmd->unitcode >= 3 && xcmd->unitcode <= 5) || (xcmd->subtype == sSwitchGeneralSwitch) && (xcmd->unitcode == 6)) {
			std::stringstream ss;
			if (xcmd->unitcode == 6) {
				if (xcmd->cmnd == 1) {
					std::vector<std::vector<std::string> > result;
					result = m_sql.safe_query("SELECT Value FROM UserVariables WHERE (Name == 'XiaomiMP3')");
					ss << result[0][0];
				}
				else {
					ss << "10000";
				}
			}
			else {
				int level = xcmd->level;
				if (level == 0) { level = 10000; }
				else {
					if (xcmd->unitcode == 3) {
						//Alarm Ringtone
						if (level > 0) { level = (level / 10) - 1; }
					}
					else if (xcmd->unitcode == 4) {
						//Alarm Clock
						if (level > 0) { level = (level / 10) + 19; }
					}
					else if (xcmd->unitcode == 5) {
						//Doorbell
						if (level > 0) { level = (level / 10) + 9; }
					}
				}
				ss << level;
			}
			m_GatewayMusicId = ss.str();
			//sid.insert(0, m_GatewayPrefix);
			message = "{\"cmd\":\"write\",\"model\":\"gateway\",\"sid\":\"" + m_GatewaySID + "\",\"short_id\":0,\"data\":\"{\\\"mid\\\":" + m_GatewayMusicId.c_str() + ",\\\"vol\\\":" + m_GatewayVolume.c_str() + ",\\\"key\\\":\\\"@gatewaykey\\\"}\" }";
		}
		else if (xcmd->subtype == sSwitchGeneralSwitch && xcmd->unitcode == 7) {
			//Xiaomi Gateway volume control
			m_GatewayVolume = std::to_string(xcmd->level);
			//sid.insert(0, m_GatewayPrefix);
			message = "{\"cmd\":\"write\",\"model\":\"gateway\",\"sid\":\"" + m_GatewaySID + "\",\"short_id\":0,\"data\":\"{\\\"mid\\\":" + m_GatewayMusicId.c_str() + ",\\\"vol\\\":" + m_GatewayVolume.c_str() + ",\\\"key\\\":\\\"@gatewaykey\\\"}\" }";
		}
		else if (xcmd->subtype == sSwitchBlindsT2) {
			int level = xcmd->level;
			if (xcmd->cmnd == 1) {
				level = 100;
			}
			message = "{\"cmd\":\"write\",\"model\":\"curtain\",\"sid\":\"158d00" + sid + "\",\"short_id\":9844,\"data\":\"{\\\"curtain_level\\\":\\\"" + std::to_string(level) + "\\\",\\\"key\\\":\\\"@gatewaykey\\\"}\" }";
		}
	}
	else if (packettype == pTypeColorSwitch) {
		//Gateway RGB Controller
		const _tColorSwitch *xcmd = reinterpret_cast<const _tColorSwitch*>(pdata);

		if (xcmd->command == Color_LedOn) {
			m_GatewayBrightnessInt = 100;
			message = "{\"cmd\":\"write\",\"model\":\"gateway\",\"sid\":\"" + m_GatewaySID + "\",\"short_id\":0,\"data\":\"{\\\"rgb\\\":4294967295,\\\"key\\\":\\\"@gatewaykey\\\"}\" }";
		}
		else if (xcmd->command == Color_LedOff) {
			m_GatewayBrightnessInt = 0;
			message = "{\"cmd\":\"write\",\"model\":\"gateway\",\"sid\":\"" + m_GatewaySID + "\",\"short_id\":0,\"data\":\"{\\\"rgb\\\":0,\\\"key\\\":\\\"@gatewaykey\\\"}\" }";
		}
		else if (xcmd->command == Color_SetColor) {
			if (xcmd->color.mode == ColorModeRGB)
			{
				m_GatewayRgbR = xcmd->color.r;
				m_GatewayRgbG = xcmd->color.g;
				m_GatewayRgbB = xcmd->color.b;
				m_GatewayBrightnessInt = xcmd->value; //TODO: What is the valid range for XiaomiGateway, 0..100 or 0..255?

				uint32_t value = (m_GatewayBrightnessInt << 24) | (m_GatewayRgbR << 16) | (m_GatewayRgbG << 8) | (m_GatewayRgbB);

				std::stringstream ss;
				ss << "{\"cmd\":\"write\",\"model\":\"gateway\",\"sid\":\"" << m_GatewaySID << "\",\"short_id\":0,\"data\":\"{\\\"rgb\\\":" << value << ",\\\"key\\\":\\\"@gatewaykey\\\"}\" }";
				message = ss.str();
			}
			else
			{
				_log.Log(LOG_STATUS, "XiaomiGateway: SetRGBColour - Color mode %d is unhandled, if you have a suggestion for what it should do, please post on the Domoticz forum", xcmd->color.mode);
			}
		}
		else if ((xcmd->command == Color_SetBrightnessLevel) || (xcmd->command == Color_SetBrightUp) || (xcmd->command == Color_SetBrightDown)) {
			//add the brightness
			if (xcmd->command == Color_SetBrightUp) {
				//m_GatewayBrightnessInt = std::min(m_GatewayBrightnessInt + 10, 100);
			}
			else if (xcmd->command == Color_SetBrightDown) {
				//m_GatewayBrightnessInt = std::max(m_GatewayBrightnessInt - 10, 0);
			}
			else {
				m_GatewayBrightnessInt = (int)xcmd->value; //TODO: What is the valid range for XiaomiGateway, 0..100 or 0..255?
			}

			uint32_t value = (m_GatewayBrightnessInt << 24) | (m_GatewayRgbR << 16) | (m_GatewayRgbG << 8) | (m_GatewayRgbB);

			std::stringstream ss;
			ss << "{\"cmd\":\"write\",\"model\":\"gateway\",\"sid\":\"" << m_GatewaySID << "\",\"short_id\":0,\"data\":\"{\\\"rgb\\\":" << value << ",\\\"key\\\":\\\"@gatewaykey\\\"}\" }";
			message = ss.str();
		}
		else if (xcmd->command == Color_SetColorToWhite) {
			//ignore Color_SetColorToWhite
		}
		else {
			_log.Log(LOG_ERROR, "XiaomiGateway: Unknown command %d", xcmd->command);
		}
	}
	if (!message.empty()) {
		_log.Debug(DEBUG_HARDWARE, "XiaomiGateway: message: '%s'", message.c_str());
		result = SendMessageToGateway(message);
		if (result == false) {
			//send the message again
			_log.Log(LOG_STATUS, "XiaomiGateway: SendMessageToGateway failed on first attempt, will try again");
			sleep_milliseconds(100);
			result = SendMessageToGateway(message);
		}
	}
	return result;
}

bool XiaomiGateway::SendMessageToGateway(const std::string &controlmessage) {
	std::string message = controlmessage;
	bool result = true;
	boost::asio::io_service io_service;
	boost::asio::ip::udp::socket socket_(io_service, boost::asio::ip::udp::endpoint(boost::asio::ip::udp::v4(), 0));
	stdreplace(message, "@gatewaykey", GetGatewayKey());
	std::shared_ptr<std::string> message1(new std::string(message));
	boost::asio::ip::udp::endpoint remote_endpoint_;
	remote_endpoint_ = boost::asio::ip::udp::endpoint(boost::asio::ip::address::from_string(m_GatewayIp), 9898);
	socket_.send_to(boost::asio::buffer(*message1), remote_endpoint_);
	sleep_milliseconds(150);
	boost::array<char, 512> recv_buffer_;
	memset(&recv_buffer_[0], 0, sizeof(recv_buffer_));
#ifdef _DEBUG
	_log.Log(LOG_STATUS, "XiaomiGateway: request to %s - %s", m_GatewayIp.c_str(), message.c_str());
#endif
	while (socket_.available() > 0) {
		socket_.receive_from(boost::asio::buffer(recv_buffer_), remote_endpoint_);
		std::string receivedString(recv_buffer_.data());

		Json::Value root;
		Json::Reader jReader;
		bool ret = jReader.parse(receivedString, root);
		if ((ret) && (root.isObject()))
		{
			std::string data = root["data"].asString();
			Json::Value root2;
			ret = jReader.parse(data.c_str(), root2);
			if ((ret) && (root2.isObject()))
			{
				std::string error = root2["error"].asString();
				if (error != "") {
					_log.Log(LOG_ERROR, "XiaomiGateway: unable to write command - %s", error.c_str());
					result = false;
				}
			}
		}

#ifdef _DEBUG
		_log.Log(LOG_STATUS, "XiaomiGateway: response %s", receivedString.c_str());
#endif
	}
	socket_.close();
	return result;
}

void XiaomiGateway::InsertUpdateTemperature(const std::string &nodeid, const std::string &Name, const float Temperature, const int battery)
{
	unsigned int sID = GetShortID(nodeid);
	if (sID > 0) {
		SendTempSensor(sID, battery, Temperature, Name);
	}
}

void XiaomiGateway::InsertUpdateHumidity(const std::string &nodeid, const std::string &Name, const int Humidity, const int battery)
{
	unsigned int sID = GetShortID(nodeid);
	if (sID > 0) {
		SendHumiditySensor(sID, battery, Humidity, Name);
	}
}

void XiaomiGateway::InsertUpdatePressure(const std::string &nodeid, const std::string &Name, const float Pressure, const int battery)
{
	unsigned int sID = GetShortID(nodeid);
	if (sID > 0) {
		SendPressureSensor(sID, 1, battery, Pressure, Name);
	}
}

void XiaomiGateway::InsertUpdateTempHumPressure(const std::string &nodeid, const std::string &Name, const float Temperature, const int Humidity, const float Pressure, const int battery)
{
	unsigned int sID = GetShortID(nodeid);
	int barometric_forcast = baroForecastNoInfo;
	if (Pressure < 1000)
		barometric_forcast = baroForecastRain;
	else if (Pressure < 1020)
		barometric_forcast = baroForecastCloudy;
	else if (Pressure < 1030)
		barometric_forcast = baroForecastPartlyCloudy;
	else
		barometric_forcast = baroForecastSunny;

	if (sID > 0) {
		SendTempHumBaroSensor(sID, battery, Temperature, Humidity, Pressure, barometric_forcast, Name);
	}
}

void XiaomiGateway::InsertUpdateTempHum(const std::string &nodeid, const std::string &Name, const float Temperature, const int Humidity, const int battery)
{
	unsigned int sID = GetShortID(nodeid);
	if (sID > 0) {
		SendTempHumSensor(sID, battery, Temperature, Humidity, Name);
	}
}

void XiaomiGateway::InsertUpdateRGBGateway(const std::string & nodeid, const std::string & Name, const bool bIsOn, const int brightness, const int hue)
{
	if (nodeid.length() < 12) {
		_log.Log(LOG_ERROR, "XiaomiGateway: Node ID %s is too short", nodeid.c_str());
		return;
	}
	std::string str = nodeid.substr(4, 8);
	unsigned int sID;
	std::stringstream ss;
	ss << std::hex << str.c_str();
	ss >> sID;

	char szDeviceID[300];
	if (sID == 1)
		sprintf(szDeviceID, "%d", 1);
	else
		sprintf(szDeviceID, "%08X", (unsigned int)sID);

	int lastLevel = 0;
	int nvalue = 0;
	bool tIsOn = !(bIsOn);
	std::vector<std::vector<std::string> > result;
	result = m_sql.safe_query("SELECT nValue, LastLevel FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='%q') AND (Type==%d) AND (SubType==%d)", m_HwdID, szDeviceID, pTypeColorSwitch, sTypeColor_RGB_W);

	if (result.empty())
	{
		_log.Log(LOG_STATUS, "XiaomiGateway: New Gateway Found (%s/%s)", str.c_str(), Name.c_str());
		//int value = atoi(brightness.c_str());
		//int value = hue; // atoi(hue.c_str());
		int cmd = Color_LedOn;
		if (!bIsOn) {
			cmd = Color_LedOff;
		}
		_tColorSwitch ycmd;
		ycmd.subtype = sTypeColor_RGB_W;
		ycmd.id = sID;
		//ycmd.dunit = 0;
		ycmd.value = brightness;
		ycmd.command = cmd;
		m_mainworker.PushAndWaitRxMessage(this, (const unsigned char *)&ycmd, NULL, -1);
		m_sql.safe_query("UPDATE DeviceStatus SET Name='%q', SwitchType=%d, LastLevel=%d WHERE(HardwareID == %d) AND (DeviceID == '%s') AND (Type == %d)", Name.c_str(), (STYPE_Dimmer), brightness, m_HwdID, szDeviceID, pTypeColorSwitch);
	}
	else {
		nvalue = atoi(result[0][0].c_str());
		tIsOn = (nvalue != 0);
		lastLevel = atoi(result[0][1].c_str());
		//int value = atoi(brightness.c_str());
		if ((bIsOn != tIsOn) || (brightness != lastLevel))
		{
			int cmd = Color_LedOn;
			if (!bIsOn) {
				cmd = Color_LedOff;
			}
			_tColorSwitch ycmd;
			ycmd.subtype = sTypeColor_RGB_W;
			ycmd.id = sID;
			//ycmd.dunit = 0;
			ycmd.value = brightness;
			ycmd.command = cmd;
			m_mainworker.PushAndWaitRxMessage(this, (const unsigned char *)&ycmd, NULL, -1);
		}
	}
}

void XiaomiGateway::InsertUpdateSwitch(const std::string &nodeid, const std::string &Name, const bool bIsOn, const _eSwitchType switchtype, const int unitcode, const int level, const std::string &messagetype, const std::string &load_power, const std::string &power_consumed, const int battery)
{
	unsigned int sID = GetShortID(nodeid);

	char szTmp[300];
	if (sID == 1)
		sprintf(szTmp, "%d", 1);
	else
		sprintf(szTmp, "%08X", (unsigned int)sID);
	std::string ID = szTmp;

	_tGeneralSwitch xcmd;
	xcmd.len = sizeof(_tGeneralSwitch) - 1;
	xcmd.id = sID;
	xcmd.type = pTypeGeneralSwitch;
	xcmd.subtype = sSwitchGeneralSwitch;
	xcmd.unitcode = unitcode;
	int customimage = 0;

	if ((xcmd.unitcode > 2) && (xcmd.unitcode < 8)) {
		customimage = 8; //speaker
	}

	if (bIsOn) {
		xcmd.cmnd = gswitch_sOn;
	}
	else {
		xcmd.cmnd = gswitch_sOff;
	}
	if (switchtype == STYPE_Selector) {
		xcmd.subtype = sSwitchTypeSelector;
		if (level > 0) {
			xcmd.level = level;
		}
	}
	else if (switchtype == STYPE_SMOKEDETECTOR) {
		xcmd.level = level;
	}
	else if (switchtype == STYPE_BlindsPercentage) {
		xcmd.level = level;
		xcmd.subtype = sSwitchBlindsT2;
		xcmd.cmnd = gswitch_sSetLevel;
	}

	//check if this switch is already in the database
	std::vector<std::vector<std::string> > result;

	// block this device if it is already added for another gateway hardware id
	result = m_sql.safe_query("SELECT nValue FROM DeviceStatus WHERE (HardwareID!=%d) AND (DeviceID=='%q') AND (Type==%d) AND (Unit == '%d')", m_HwdID, ID.c_str(), xcmd.type, xcmd.unitcode);
	if (!result.empty()) {
		return;
	}

	result = m_sql.safe_query("SELECT nValue, BatteryLevel FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='%q') AND (Type==%d) AND (Unit == '%d')", m_HwdID, ID.c_str(), xcmd.type, xcmd.unitcode);
	if (result.empty())
	{
		_log.Log(LOG_STATUS, "XiaomiGateway: New %s Found (%s)", Name.c_str(), nodeid.c_str());
		m_mainworker.PushAndWaitRxMessage(this, (const unsigned char *)&xcmd, NULL, battery);
		if (customimage == 0) {
			if (switchtype == STYPE_OnOff) {
				customimage = 1; //wall socket
			}
			else if (switchtype == STYPE_Selector) {
				customimage = 9;
			}
		}
		/*if (isctlr2 == true) {
			m_sql.safe_query("UPDATE DeviceStatus SET Name='Xiaomi Wired Switch 1', SwitchType=%d, CustomImage=%i, Unit='1' WHERE(HardwareID == %d) AND (DeviceID == '%q') AND (Unit == '1')", (switchtype), customimage, m_HwdID, ID.c_str());
			xcmd.unitcode = 2;
			m_mainworker.PushAndWaitRxMessage(this, (const unsigned char *)&xcmd, NULL, -1);
			m_sql.safe_query("UPDATE DeviceStatus SET Name='Xiaomi Wired Switch 2', SwitchType=%d, CustomImage=%i, Unit='2' WHERE(HardwareID == %d) AND (DeviceID == '%q') AND (Unit == '2')", (switchtype), customimage, m_HwdID, ID.c_str());
		}
		else {*/
		m_sql.safe_query("UPDATE DeviceStatus SET Name='%q', SwitchType=%d, CustomImage=%i WHERE(HardwareID == %d) AND (DeviceID == '%q') AND (Unit == '%d')", Name.c_str(), (switchtype), customimage, m_HwdID, ID.c_str(), xcmd.unitcode);
		//}

		if (switchtype == STYPE_Selector) {
			result = m_sql.safe_query("SELECT ID FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='%q') AND (Type==%d) AND (Unit == '%d')", m_HwdID, ID.c_str(), xcmd.type, xcmd.unitcode);
			if (!result.empty()) {
				std::string Idx = result[0][0];
				if (Name == "Xiaomi Wireless Switch") {
					m_sql.SetDeviceOptions(atoi(Idx.c_str()), m_sql.BuildDeviceOptions("SelectorStyle:0;LevelNames:Off|Click|Double Click|Long Click|Long Click Release", false));
				}
				else if (Name == "Xiaomi Square Wireless Switch") {
					// click/double click
					m_sql.SetDeviceOptions(atoi(Idx.c_str()), m_sql.BuildDeviceOptions("SelectorStyle:0;LevelNames:Off|Click|Double Click", false));
				}
				else if (Name == "Xiaomi Smart Push Button") {
					// click/double click
					m_sql.SetDeviceOptions(atoi(Idx.c_str()), m_sql.BuildDeviceOptions("SelectorStyle:0;LevelNames:Off|Click|Shake", false));
				}
				else if (Name == "Xiaomi Cube") {
					// flip90/flip180/move/tap_twice/shake_air/swing/alert/free_fall
					m_sql.SetDeviceOptions(atoi(Idx.c_str()), m_sql.BuildDeviceOptions("SelectorStyle:0;LevelNames:Off|flip90|flip180|move|tap_twice|shake_air|swing|alert|free_fall|clock_wise|anti_clock_wise", false));
				}
				else if (Name == "Aqara Cube") {
					// flip90/flip180/move/tap_twice/shake_air/swing/alert/free_fall/rotate
					m_sql.SetDeviceOptions(atoi(Idx.c_str()), m_sql.BuildDeviceOptions("SelectorStyle:0;LevelNames:Off|flip90|flip180|move|tap_twice|shake_air|swing|alert|free_fall|rotate", false));
				}
				else if (Name == "Aqara Vibration Sensor") {
					// tilt/vibrate/free fall
					m_sql.SetDeviceOptions(atoi(Idx.c_str()), m_sql.BuildDeviceOptions("SelectorStyle:0;LevelNames:Off|Tilt|Vibrate|Free Fall", false));
				}
				else if (Name == "Xiaomi Wireless Dual Wall Switch") {
					//for Aqara wireless switch, 2 buttons support
					m_sql.SetDeviceOptions(atoi(Idx.c_str()), m_sql.BuildDeviceOptions("SelectorStyle:0;LevelNames:Off|Switch 1|Switch 2|Both Click|Switch 1 Double Click|Switch 2 Double Click|Both Double Click|Switch 1 Long Click|Switch 2 Long Click|Both Long Click", false));
				}
				else if (Name == "Xiaomi Wired Single Wall Switch") {
					//for Aqara wired switch, single button support
					m_sql.SetDeviceOptions(atoi(Idx.c_str()), m_sql.BuildDeviceOptions("SelectorStyle:0;LevelNames:Off|Switch1 On|Switch1 Off", false));
				}
				else if (Name == "Xiaomi Wireless Single Wall Switch") {
					//for Aqara wireless switch, single button support
					m_sql.SetDeviceOptions(atoi(Idx.c_str()), m_sql.BuildDeviceOptions("SelectorStyle:0;LevelNames:Off|Switch 1", false));
				}
				else if (Name == "Xiaomi Gateway Alarm Ringtone") {
					//for the Gateway Audio
					m_sql.SetDeviceOptions(atoi(Idx.c_str()), m_sql.BuildDeviceOptions("SelectorStyle:1;LevelNames:Off|Police siren 1|Police siren 2|Accident tone|Missle countdown|Ghost|Sniper|War|Air Strike|Barking dogs", false));
				}
				else if (Name == "Xiaomi Gateway Alarm Clock") {
					//for the Gateway Audio
					m_sql.SetDeviceOptions(atoi(Idx.c_str()), m_sql.BuildDeviceOptions("SelectorStyle:1;LevelNames:Off|MiMix|Enthusiastic|GuitarClassic|IceWorldPiano|LeisureTime|Childhood|MorningStreamlet|MusicBox|Orange|Thinker", false));
				}
				else if (Name == "Xiaomi Gateway Doorbell") {
					//for the Gateway Audio
					m_sql.SetDeviceOptions(atoi(Idx.c_str()), m_sql.BuildDeviceOptions("SelectorStyle:1;LevelNames:Off|Doorbell ring tone|Knock on door|Hilarious|Alarm clock", false));
				}
			}
		}
		else if (switchtype == STYPE_OnOff && Name == "Xiaomi Gateway MP3") {
			std::string errorMessage;
			m_sql.AddUserVariable("XiaomiMP3", USERVARTYPE_INTEGER, "10001", errorMessage);
		}
	}
	else {
		/*if (is2ndchannel) {
			xcmd.unitcode = 2;
		}*/
		int nvalue = atoi(result[0][0].c_str());
		int BatteryLevel = atoi(result[0][1].c_str());

		if (messagetype == "heartbeat") {
			if (battery != 255) {
				BatteryLevel = battery;
				m_sql.safe_query("UPDATE DeviceStatus SET BatteryLevel=%d WHERE(HardwareID == %d) AND (DeviceID == '%q') AND (Unit == '%d')", BatteryLevel, m_HwdID, ID.c_str(), xcmd.unitcode);
			}
		}
		else {
			if ((bIsOn == false && nvalue >= 1) || (bIsOn == true) || (Name == "Xiaomi Wired Dual Wall Switch") || (Name == "Xiaomi Wired Single Wall Switch") || (Name == "Xiaomi Curtain")) {
				m_mainworker.PushAndWaitRxMessage(this, (const unsigned char *)&xcmd, NULL, BatteryLevel);
			}
		}
		if ((Name == "Xiaomi Smart Plug") || (Name == "Xiaomi Smart Wall Plug")) {
			if (load_power != "" && power_consumed != "") {
				int power = atoi(load_power.c_str());
				int consumed = atoi(power_consumed.c_str()) / 1000;
				SendKwhMeter(sID, 1, 255, power, consumed, "Xiaomi Smart Plug Usage");
			}
		}
	}
}

void XiaomiGateway::InsertUpdateCubeText(const std::string & nodeid, const std::string & Name, const std::string &degrees)
{
	unsigned int sID = GetShortID(nodeid);
	if (sID > 0) {
		SendTextSensor(sID, sID, 255, degrees.c_str(), Name);
	}
}

void XiaomiGateway::InsertUpdateVoltage(const std::string & nodeid, const std::string & Name, const int VoltageLevel)
{
	if (VoltageLevel < 3600) {
		unsigned int sID = GetShortID(nodeid);
		if (sID > 0) {
			int percent = ((VoltageLevel - 2200) / 10);
			float voltage = (float)VoltageLevel / 1000;
			SendVoltageSensor(sID, sID, percent, voltage, "Xiaomi Voltage");
		}
	}
}

void XiaomiGateway::InsertUpdateLux(const std::string & nodeid, const std::string & Name, const int Illumination, const int battery)
{
	unsigned int sID = GetShortID(nodeid);
	if (sID > 0) {
		float lux = (float)Illumination;
		SendLuxSensor(sID, sID, battery, lux, Name);
	}
}

bool XiaomiGateway::StartHardware()
{
	RequestStart();

	m_bDoRestart = false;

	//force connect the next first time
	m_bIsStarted = true;

	m_GatewayMusicId = "10000";
	m_GatewayVolume = "20";

	std::vector<std::vector<std::string> > result;
	result = m_sql.safe_query("SELECT Password, Address FROM Hardware WHERE Type=%d AND ID=%d AND Enabled=1", HTYPE_XiaomiGateway, m_HwdID);

	if (result.empty())
		return false;

	m_GatewayPassword = result[0][0].c_str();
	m_GatewayIp = result[0][1].c_str();

	m_GatewayRgbR = 255;
	m_GatewayRgbG = 255;
	m_GatewayRgbB = 255;
	m_GatewayBrightnessInt = 100;
	//m_GatewayPrefix = "f0b4";
	//check for presence of Xiaomi user variable to enable message output
	m_OutputMessage = false;
	result = m_sql.safe_query("SELECT Value FROM UserVariables WHERE (Name == 'XiaomiMessage')");
	if (!result.empty()) {
		m_OutputMessage = true;
	}
	//check for presence of Xiaomi user variable to enable additional voltage devices
	m_IncludeVoltage = false;
	result = m_sql.safe_query("SELECT Value FROM UserVariables WHERE (Name == 'XiaomiVoltage')");
	if (!result.empty()) {
		m_IncludeVoltage = true;
	}
	_log.Log(LOG_STATUS, "XiaomiGateway (ID=%d): Delaying worker startup...", m_HwdID);
	sleep_seconds(5);

	XiaomiGatewayTokenManager::GetInstance();

	AddGatewayToList();

	if (m_ListenPort9898)
	{
		_log.Log(LOG_STATUS, "XiaomiGateway (ID=%d): Selected as main Gateway", m_HwdID);
	}

	//Start worker thread
	m_thread = std::shared_ptr<std::thread>(new std::thread(&XiaomiGateway::Do_Work, this));
	SetThreadNameInt(m_thread->native_handle());

	return (m_thread != nullptr);
}

bool XiaomiGateway::StopHardware()
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

void XiaomiGateway::Do_Work()
{
	_log.Log(LOG_STATUS, "XiaomiGateway (ID=%d): Worker started...", m_HwdID);
	boost::asio::io_service io_service;
	//find the local ip address that is similar to the xiaomi gateway
	try {
		boost::asio::ip::udp::resolver resolver(io_service);
		boost::asio::ip::udp::resolver::query query(boost::asio::ip::udp::v4(), m_GatewayIp, "");
		boost::asio::ip::udp::resolver::iterator endpoints = resolver.resolve(query);
		boost::asio::ip::udp::endpoint ep = *endpoints;
		boost::asio::ip::udp::socket socket(io_service);
		socket.connect(ep);
		boost::asio::ip::address addr = socket.local_endpoint().address();
		std::string compareIp = m_GatewayIp.substr(0, (m_GatewayIp.length() - 3));
		std::size_t found = addr.to_string().find(compareIp);
		if (found != std::string::npos) {
			m_LocalIp = addr.to_string();
			_log.Log(LOG_STATUS, "XiaomiGateway (ID=%d): Using %s for local IP address.", m_HwdID, m_LocalIp.c_str());
		}
	}
	catch (std::exception& e) {
		_log.Log(LOG_STATUS, "XiaomiGateway (ID=%d): Could not detect local IP address using Boost.Asio: %s", m_HwdID, e.what());
	}

	// try finding local ip using ifaddrs when Boost.Asio fails
	if (m_LocalIp == "") {
		try {
			// get first 2 octets of Xiaomi gateway ip to search for similar ip address
			std::string compareIp = m_GatewayIp.substr(0, (m_GatewayIp.length() - 3));
			_log.Log(LOG_STATUS, "XiaomiGateway (ID=%d): XiaomiGateway IP address starts with: %s", m_HwdID, compareIp.c_str());

			std::vector<std::string> ip_addrs;
			if (XiaomiGateway::get_local_ipaddr(ip_addrs) > 0)
			{
				for (const std::string &addr : ip_addrs)
				{
					std::size_t found = addr.find(compareIp);
					if (found != std::string::npos)
					{
						m_LocalIp = addr;
						_log.Log(LOG_STATUS, "XiaomiGateway (ID=%d): Using %s for local IP address.", m_HwdID, m_LocalIp.c_str());
						break;
					}
				}
			}
			else
			{
				_log.Log(LOG_STATUS, "XiaomiGateway (ID=%d): Could not find local IP address with ifaddrs", m_HwdID);
			}
		}
		catch (std::exception& e) {
			_log.Log(LOG_STATUS, "XiaomiGateway (ID=%d): Could not find local IP address with ifaddrs: %s", m_HwdID, e.what());
		}
	}

	XiaomiGateway::xiaomi_udp_server udp_server(io_service, m_HwdID, m_GatewayIp, m_LocalIp, m_ListenPort9898, m_OutputMessage, m_IncludeVoltage, this);
	boost::thread bt;
	if (m_ListenPort9898) {
		bt = boost::thread(boost::bind(&boost::asio::io_service::run, &io_service));
		SetThreadName(bt.native_handle(), "XiaomiGatewayIO");
	}

	int sec_counter = 0;
	while (!IsStopRequested(1000))
	{
		sec_counter++;
		if (sec_counter % 12 == 0) {
			m_LastHeartbeat = mytime(NULL);
		}
		if (sec_counter % 60 == 0)
		{
			//_log.Log(LOG_STATUS, "sec_counter %d", sec_counter);
		}
	}
	io_service.stop();
	RemoveFromGatewayList();
	_log.Log(LOG_STATUS, "XiaomiGateway (ID=%d): stopped", m_HwdID);
}

std::string XiaomiGateway::GetGatewayKey()
{
#ifdef WWW_ENABLE_SSL
	const unsigned char *key = (unsigned char *)m_GatewayPassword.c_str();
	unsigned char iv[AES_BLOCK_SIZE] = { 0x17, 0x99, 0x6d, 0x09, 0x3d, 0x28, 0xdd, 0xb3, 0xba, 0x69, 0x5a, 0x2e, 0x6f, 0x58, 0x56, 0x2e };
	std::string token = XiaomiGatewayTokenManager::GetInstance().GetToken(m_GatewayIp);
	unsigned char *plaintext = (unsigned char *)token.c_str();
	unsigned char ciphertext[128];

	AES_KEY encryption_key;
	AES_set_encrypt_key(key, 128, &(encryption_key));
	AES_cbc_encrypt((unsigned char *)plaintext, ciphertext, sizeof(plaintext) * 8, &encryption_key, iv, AES_ENCRYPT);

	char gatewaykey[128];
	for (int i = 0; i < 16; i++)
	{
		sprintf(&gatewaykey[i * 2], "%02X", ciphertext[i]);
	}
#ifdef _DEBUG
	_log.Log(LOG_STATUS, "XiaomiGateway: GetGatewayKey Password - %s", m_GatewayPassword.c_str());
	_log.Log(LOG_STATUS, "XiaomiGateway: GetGatewayKey key - %s", gatewaykey);
#endif
	return gatewaykey;
#else
	_log.Log(LOG_ERROR, "XiaomiGateway: GetGatewayKey NO SSL AVAILABLE");
	return std::string("");
#endif
}

unsigned int XiaomiGateway::GetShortID(const std::string & nodeid)
{
	if (nodeid.length() < 12) {
		_log.Log(LOG_ERROR, "XiaomiGateway: Node ID %s is too short", nodeid.c_str());
		return -1;
	}
	std::string str;
	if (nodeid.length() < 14) {
		//gateway
		str = nodeid.substr(4, 8);
	}
	else {
		//device
		str = nodeid.substr(6, 8);
	}
	unsigned int sID;
	std::stringstream ss;
	ss << std::hex << str.c_str();
	ss >> sID;
	return sID;
}

XiaomiGateway::xiaomi_udp_server::xiaomi_udp_server(boost::asio::io_service& io_service, int m_HwdID, const std::string &gatewayIp, const std::string &localIp, const bool listenPort9898, const bool outputMessage, const bool includeVoltage, XiaomiGateway *parent)
	: socket_(io_service, boost::asio::ip::udp::v4())
{
	m_HardwareID = m_HwdID;
	m_XiaomiGateway = parent;
	m_gatewayip = gatewayIp;
	m_localip = localIp;
	m_OutputMessage = outputMessage;
	m_IncludeVoltage = includeVoltage;
	if (listenPort9898) {
		try {
			socket_.set_option(boost::asio::ip::udp::socket::reuse_address(true));
			if (m_localip != "") {
				boost::system::error_code ec;
				boost::asio::ip::address listen_addr = boost::asio::ip::address::from_string(m_localip, ec);
				boost::asio::ip::address mcast_addr = boost::asio::ip::address::from_string("224.0.0.50", ec);
				boost::asio::ip::udp::endpoint listen_endpoint(mcast_addr, 9898);

				socket_.bind(boost::asio::ip::udp::endpoint(boost::asio::ip::udp::v4(), 9898));
				std::shared_ptr<std::string> message(new std::string("{\"cmd\":\"whois\"}"));
				boost::asio::ip::udp::endpoint remote_endpoint;
				remote_endpoint = boost::asio::ip::udp::endpoint(mcast_addr, 4321);
				socket_.send_to(boost::asio::buffer(*message), remote_endpoint);
				socket_.set_option(boost::asio::ip::multicast::join_group(mcast_addr.to_v4(), listen_addr.to_v4()), ec);
				socket_.bind(listen_endpoint, ec);
			}
			else {
				socket_.bind(boost::asio::ip::udp::endpoint(boost::asio::ip::udp::v4(), 9898));
				std::shared_ptr<std::string> message(new std::string("{\"cmd\":\"whois\"}"));
				boost::asio::ip::udp::endpoint remote_endpoint;
				remote_endpoint = boost::asio::ip::udp::endpoint(boost::asio::ip::address::from_string("224.0.0.50"), 4321);
				socket_.send_to(boost::asio::buffer(*message), remote_endpoint);
				socket_.set_option(boost::asio::ip::multicast::join_group(boost::asio::ip::address::from_string("224.0.0.50")));
			}
		}
		catch (const boost::system::system_error& ex) {
			_log.Log(LOG_ERROR, "XiaomiGateway: %s", ex.code().category().name());
			m_XiaomiGateway->StopHardware();
			return;
		}
		start_receive();
	}
	else {
	}
}

XiaomiGateway::xiaomi_udp_server::~xiaomi_udp_server()
{
}

void XiaomiGateway::xiaomi_udp_server::start_receive()
{
	//_log.Log(LOG_STATUS, "start_receive");
	memset(&data_[0], 0, sizeof(data_));
	socket_.async_receive_from(boost::asio::buffer(data_, max_length), remote_endpoint_, boost::bind(&xiaomi_udp_server::handle_receive, this, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
}

void XiaomiGateway::xiaomi_udp_server::handle_receive(const boost::system::error_code & error, std::size_t bytes_recvd)
{
	if (!error || error == boost::asio::error::message_size)
	{
		XiaomiGateway * TrueGateway = m_XiaomiGateway->GatewayByIp(remote_endpoint_.address().to_v4().to_string());

		if (!TrueGateway)
		{
			_log.Log(LOG_ERROR, "XiaomiGateway: received data from  unregisted gateway!");
			start_receive();
			return;
		}
#ifdef _DEBUG
		_log.Log(LOG_STATUS, data_);
#endif
		Json::Value root;
		Json::Reader jReader;
		bool showmessage = true;
		bool ret = jReader.parse(data_, root);
		if ((!ret) || (!root.isObject()))
		{
			_log.Log(LOG_ERROR, "XiaomiGateway: invalid data received!");
			start_receive();
			return;
		}
		else {
			std::string cmd = root["cmd"].asString();
			std::string model = root["model"].asString();
			std::string sid = root["sid"].asString();
			std::string data = root["data"].asString();
			int unitcode = 1;
			if ((cmd == "report") || (cmd == "read_ack") || (cmd == "heartbeat"))
			{
				Json::Value root2;
				ret = jReader.parse(data.c_str(), root2);
				if ((ret) || (!root2.isObject()))
				{
					_eSwitchType type = STYPE_END;
					std::string name = "Xiaomi Switch";
					if (model == "motion") {
						type = STYPE_Motion;
						name = "Xiaomi Motion Sensor";
					}
					else if (model == "sensor_motion.aq2") {
						type = STYPE_Motion;
						name = "Aqara Motion Sensor";
					}
					else if ((model == "switch") || (model == "remote.b1acn01")) {
						type = STYPE_Selector;
						name = "Xiaomi Wireless Switch";
					}
					else if (model == "sensor_switch.aq2") {
						type = STYPE_Selector;
						name = "Xiaomi Square Wireless Switch";
					}
					else if (model == "sensor_switch.aq3") {
						type = STYPE_Selector;
						name = "Xiaomi Smart Push Button";
					}
					else if ((model == "magnet") || (model == "sensor_magnet.aq2")) {
						type = STYPE_Contact;
						name = "Xiaomi Door Sensor";
					}
					else if (model == "plug") {
						type = STYPE_OnOff;
						name = "Xiaomi Smart Plug";
					}
					else if (model == "86plug" || model == "ctrl_86plug.aq1") {
						type = STYPE_OnOff;
						name = "Xiaomi Smart Wall Plug";
					}
					else if (model == "sensor_ht") {
						name = "Xiaomi Temperature/Humidity";
					}
					else if (model == "weather.v1") {
						name = "Xiaomi Aqara Weather";
					}
					else if (model == "cube") {
						name = "Xiaomi Cube";
						type = STYPE_Selector;
					}
					else if (model == "sensor_cube.aqgl01") {
						name = "Aqara Cube";
						type = STYPE_Selector;
					}
					else if (model == "86sw2" || model == "remote.b286acn01") {
						name = "Xiaomi Wireless Dual Wall Switch";
						type = STYPE_Selector;
					}
					else if (model == "vibration") {
						name = "Aqara Vibration Sensor";
						type = STYPE_Selector;
					}
					else if (model == "ctrl_neutral2" || model == "ctrl_ln2" || model == "ctrl_ln2.aq1") {
						name = "Xiaomi Wired Dual Wall Switch";
						//type = STYPE_Selector;
					}
					else if (model == "gateway" || model == "gateway.v3" || model == "acpartner.v3") {
						name = "Xiaomi RGB Gateway";
					}
					else if (model == "ctrl_neutral1" || model == "ctrl_ln1" || model == "ctrl_ln1.aq1") {
						name = "Xiaomi Wired Single Wall Switch";
						//type = STYPE_Selector;
					}
					else if (model == "86sw1" || model == "remote.b186acn01") {
						name = "Xiaomi Wireless Single Wall Switch";
						type = STYPE_PushOn;
					}
					else if (model == "smoke") {
						name = "Xiaomi Smoke Detector";
						type = STYPE_SMOKEDETECTOR;
					}
					else if (model == "natgas") {
						name = "Xiaomi Gas Detector";
						type = STYPE_SMOKEDETECTOR;
					}
					else if (model == "sensor_wleak.aq1") {
						name = "Xiaomi Water Leak Detector";
						type = STYPE_SMOKEDETECTOR;
					}
					else if (model == "curtain") {
						name = "Xiaomi Curtain";
						type = STYPE_BlindsPercentage;
					}
					std::string voltage = root2["voltage"].asString();
					int battery = 255;
					if (voltage != "" && voltage != "3600") {
						battery = ((atoi(voltage.c_str()) - 2200) / 10);
					}
					if (type != STYPE_END)
					{
						std::string status = root2["status"].asString();
						std::string no_close = root2["no_close"].asString();
						std::string no_motion = root2["no_motion"].asString();
						//Aqara's Wireless switch reports per channel
						std::string aqara_wireless1 = root2["channel_0"].asString();
						std::string aqara_wireless2 = root2["channel_1"].asString();
						std::string aqara_wireless3 = root2["dual_channel"].asString();
						//Smart plug usage
						std::string load_power = root2["load_power"].asString();
						std::string power_consumed = root2["power_consumed"].asString();
						//Smoke or Gas Detector
						std::string density = root2["density"].asString();
						std::string alarm = root2["alarm"].asString();
						//Aqara motion sensor
						std::string lux = root2["lux"].asString();
						//Curtain
						std::string curtain = root2["curtain_level"].asString();
						bool on = false;
						int level = -1;
						if (model == "switch") {
							level = 0;
						}
						else if (model == "smoke" || model == "natgas" || model == "sensor_wleak.aq1" || model == "sensor_cube.aqgl01") {
							if (battery != 255 && (model == "sensor_wleak.aq1" || model == "sensor_cube.aqgl01")) {
								level = 0;
							}
							if ((alarm == "1") || (alarm == "2") || (status == "leak")) {
								level = 0;
								on = true;
							}
							else if ((alarm == "0") || (status == "no_leak") || (status == "iam")) {
								level = 0;
							}
							if (density != "")
								level = atoi(density.c_str());
						}
						if ((status == "motion") || (status == "open") || (status == "no_close") || (status == "on") || (no_close != "")) {
							level = 0;
							on = true;
						}
						else if ((status == "no_motion") || (status == "close") || (status == "off") || (no_motion != "")) {
							level = 0;
							on = false;
						}
						else if ((status == "click") || (status == "flip90") || (aqara_wireless1 == "click") || (status == "tilt")) {
							level = 10;
							on = true;
						}
						else if ((status == "double_click") || (status == "flip180") || (aqara_wireless2 == "click") || (status == "shake") || (status == "vibrate")) {
							level = 20;
							on = true;
						}
						else if ((status == "long_click_press") || (status == "move") || (aqara_wireless3 == "both_click") || (status == "free_fall")) {
							level = 30;
							on = true;
						}
						else if ((status == "tap_twice") || (status == "long_click_release") || (aqara_wireless1 == "double_click")) {
							level = 40;
							on = true;
						}
						else if ((status == "shake_air") || (aqara_wireless2 == "double_click")) {
							level = 50;
							on = true;
						}
						else if ((status == "swing") || (aqara_wireless3 == "double_both_click")) {
							level = 60;
							on = true;
						}
						else if ((status == "alert") || (aqara_wireless1 == "long_click")) {
							level = 70;
							on = true;
						}
						else if ((status == "free_fall") || (aqara_wireless2 == "long_click")) {
							level = 80;
							on = true;
						}
						else if (aqara_wireless3 == "long_both_click") {
							level = 90;
							on = true;
						}
						std::string rotate = root2["rotate"].asString();
						if (rotate != "") {
							int amount = atoi(rotate.c_str());
							if (amount > 0) {
								level = 90;
							}
							else {
								level = 100;
							}
							on = true;
							TrueGateway->InsertUpdateCubeText(sid.c_str(), name, rotate.c_str());
							TrueGateway->InsertUpdateSwitch(sid.c_str(), name, on, type, unitcode, level, cmd, "", "", battery);
						}
						else {
							if (model == "plug" || model == "86plug" || model == "ctrl_86plug.aq1") {
								sleep_milliseconds(100); //need to sleep here as the gateway will send 2 update messages, and need time for the database to update the state so that the event is not triggered twice
								TrueGateway->InsertUpdateSwitch(sid.c_str(), name, on, type, unitcode, level, cmd, load_power, power_consumed, battery);
							}
							else if ((model == "curtain") && (curtain != "")) {
								level = atoi(curtain.c_str());
								TrueGateway->InsertUpdateSwitch(sid.c_str(), name, on, type, unitcode, level, cmd, "", "", battery);
							}
							else {
								if (level > -1) { //this should stop false updates when empty 'data' is received
									TrueGateway->InsertUpdateSwitch(sid.c_str(), name, on, type, unitcode, level, cmd, "", "", battery);
								}
								if (lux != "") {
									TrueGateway->InsertUpdateLux(sid.c_str(), name, atoi(lux.c_str()), battery);
								}
								if (voltage != "" && m_IncludeVoltage) {
									TrueGateway->InsertUpdateVoltage(sid.c_str(), name, atoi(voltage.c_str()));
								}
							}
						}
					}
					else if ((name == "Xiaomi Wired Dual Wall Switch") || (name == "Xiaomi Wired Single Wall Switch"))
					{
						//aqara wired dual switch, bidirectional communiction support
						type = STYPE_OnOff;
						std::string aqara_wired1 = root2["channel_0"].asString();
						std::string aqara_wired2 = root2["channel_1"].asString();
						bool state = false;
						if ((aqara_wired1 == "on") || (aqara_wired2 == "on")) {
							state = true;
						}
						unitcode = 8;
						if (name == "Xiaomi Wired Single Wall Switch") {
							unitcode = 8;
						}
						else {
							unitcode = 9;
							name = "Xiaomi Wired Dual Wall Switch Channel 0";
						}
						if (aqara_wired1 != "") {
							TrueGateway->InsertUpdateSwitch(sid.c_str(), name, state, type, unitcode, 0, cmd, "", "", battery);
						}
						else if (aqara_wired2 != "") {
							unitcode = 10;
							name = "Xiaomi Wired Dual Wall Switch Channel 1";
							TrueGateway->InsertUpdateSwitch(sid.c_str(), name, state, type, unitcode, 0, cmd, "", "", battery);
						}
					}
					else if ((name == "Xiaomi Temperature/Humidity") || (name == "Xiaomi Aqara Weather"))
					{
						std::string temperature = root2["temperature"].asString();
						std::string humidity = root2["humidity"].asString();
						float pressure = 0;

						if (name == "Xiaomi Aqara Weather") {
							std::string szPressure = root2["pressure"].asString();
							pressure = static_cast<float>(atof(szPressure.c_str())) / 100.0f;
						}

						if ((!temperature.empty()) && (!humidity.empty()) && (pressure != 0))
						{
							//Temp+Hum+Baro
							float temp = (float)atof(temperature.c_str()) / 100.0f;
							int hum = atoi(humidity.c_str()) / 100;
							TrueGateway->InsertUpdateTempHumPressure(sid.c_str(), "Xiaomi TempHumBaro", temp, hum, pressure, battery);
						}
						else if ((!temperature.empty()) && (!humidity.empty()))
						{
							//Temp+Hum
							float temp = (float)atof(temperature.c_str()) / 100.0f;
							int hum = atoi(humidity.c_str()) / 100;
							TrueGateway->InsertUpdateTempHum(sid.c_str(), "Xiaomi TempHum", temp, hum, battery);
						}
						else if (temperature != "") {
							float temp = (float)atof(temperature.c_str()) / 100.0f;
							if (temp < 99) {
								TrueGateway->InsertUpdateTemperature(sid.c_str(), "Xiaomi Temperature", temp, battery);
							}
						}
						else if (humidity != "") {
							int hum = atoi(humidity.c_str()) / 100;
							if (hum > 1) {
								TrueGateway->InsertUpdateHumidity(sid.c_str(), "Xiaomi Humidity", hum, battery);
							}
						}
					}
					else if (name == "Xiaomi RGB Gateway")
					{
						std::string rgb = root2["rgb"].asString();
						std::string illumination = root2["illumination"].asString();
						if (rgb != "") {
							// Only add in the gateway that matches the SID for this hardware.
							//if (m_XiaomiGateway->m_GatewaySID == sid) {
							if (TrueGateway->GetGatewaySid() == sid)
							{
								std::stringstream ss;
								ss << std::hex << atoi(rgb.c_str());
								std::string hexstring(ss.str());
								if (hexstring.length() == 7) {
									hexstring.insert(0, "0");
								}
								std::string bright_hex = hexstring.substr(0, 2);
								std::stringstream ss2;
								ss2 << std::hex << bright_hex.c_str();
								int brightness = strtoul(bright_hex.c_str(), NULL, 16);
								bool on = false;
								if (rgb != "0") {
									on = true;
								}
								TrueGateway->InsertUpdateRGBGateway(sid.c_str(), name + " (" + TrueGateway->GetGatewayIp() + ")", on, brightness, 0);
								TrueGateway->InsertUpdateLux(sid.c_str(), "Xiaomi Gateway Lux", atoi(illumination.c_str()), 255);
								TrueGateway->InsertUpdateSwitch(sid.c_str(), "Xiaomi Gateway Alarm Ringtone", false, STYPE_Selector, 3, 0, cmd, "", "", 255);
								TrueGateway->InsertUpdateSwitch(sid.c_str(), "Xiaomi Gateway Alarm Clock", false, STYPE_Selector, 4, 0, cmd, "", "", 255);
								TrueGateway->InsertUpdateSwitch(sid.c_str(), "Xiaomi Gateway Doorbell", false, STYPE_Selector, 5, 0, cmd, "", "", 255);
								TrueGateway->InsertUpdateSwitch(sid.c_str(), "Xiaomi Gateway MP3", false, STYPE_OnOff, 6, 0, cmd, "", "", 255);
								TrueGateway->InsertUpdateSwitch(sid.c_str(), "Xiaomi Gateway Volume", false, STYPE_Dimmer, 7, 0, cmd, "", "", 255);
							}
						}
						else {
							//check for token
							std::string token = root["token"].asString();
							std::string ip = root2["ip"].asString();

							if ((token != "") && (ip != "")) {
								XiaomiGatewayTokenManager::GetInstance().UpdateTokenSID(ip, token, sid);
								showmessage = false;
							}
						}
					}
					else
					{
						_log.Log(LOG_STATUS, "XiaomiGateway (ID=%d): unhandled model: %s, name: %s", TrueGateway->GetGatewayHardwareID(), model.c_str(), name.c_str());
					}
				}
			}
			else if (cmd == "get_id_list_ack")
			{
				Json::Value root2;
				ret = jReader.parse(data.c_str(), root2);
				if ((ret) || (!root2.isObject()))
				{
					for (int i = 0; i < (int)root2.size(); i++) {
						std::string message = "{\"cmd\" : \"read\",\"sid\":\"";
						message.append(root2[i].asString().c_str());
						message.append("\"}");
						std::shared_ptr<std::string> message1(new std::string(message));
						boost::asio::ip::udp::endpoint remote_endpoint;
						remote_endpoint = boost::asio::ip::udp::endpoint(boost::asio::ip::address::from_string(TrueGateway->GetGatewayIp().c_str()), 9898);
						socket_.send_to(boost::asio::buffer(*message1), remote_endpoint);
					}
				}
				showmessage = false;
			}
			else if (cmd == "iam")
			{
				if (model == "gateway" || model == "gateway.v3" || model == "acpartner.v3")
				{
					std::string ip = root["ip"].asString();
					// Only add in the gateway that matches the IP address for this hardware.
					if (ip == TrueGateway->GetGatewayIp())
					{
						_log.Log(LOG_STATUS, "XiaomiGateway: RGB Gateway Detected");
						TrueGateway->InsertUpdateRGBGateway(sid.c_str(), "Xiaomi RGB Gateway (" + ip + ")", false, 0, 100);
						TrueGateway->InsertUpdateSwitch(sid.c_str(), "Xiaomi Gateway Alarm Ringtone", false, STYPE_Selector, 3, 0, cmd, "", "", 255);
						TrueGateway->InsertUpdateSwitch(sid.c_str(), "Xiaomi Gateway Alarm Clock", false, STYPE_Selector, 4, 0, cmd, "", "", 255);
						TrueGateway->InsertUpdateSwitch(sid.c_str(), "Xiaomi Gateway Doorbell", false, STYPE_Selector, 5, 0, cmd, "", "", 255);
						TrueGateway->InsertUpdateSwitch(sid.c_str(), "Xiaomi Gateway MP3", false, STYPE_OnOff, 6, 0, cmd, "", "", 255);
						TrueGateway->InsertUpdateSwitch(sid.c_str(), "Xiaomi Gateway Volume", false, STYPE_Dimmer, 7, 0, cmd, "", "", 255);

						//query for list of devices
						std::string message = "{\"cmd\" : \"get_id_list\"}";
						std::shared_ptr<std::string> message2(new std::string(message));
						boost::asio::ip::udp::endpoint remote_endpoint;
						remote_endpoint = boost::asio::ip::udp::endpoint(boost::asio::ip::address::from_string(TrueGateway->GetGatewayIp().c_str()), 9898);
						socket_.send_to(boost::asio::buffer(*message2), remote_endpoint);
					}
				}
				showmessage = false;
			}
			else
			{
				_log.Log(LOG_STATUS, "XiaomiGateway (ID=%d): unknown cmd received: %s, model: %s", TrueGateway->GetGatewayHardwareID(), cmd.c_str(), model.c_str());
			}
		}
		if (showmessage && m_OutputMessage) {
			_log.Log(LOG_STATUS, "%s", data_);
		}
		start_receive();
	}
	else {
		_log.Log(LOG_ERROR, "XiaomiGateway: error in handle_receive %s", error.message().c_str());
	}
}


XiaomiGateway::XiaomiGatewayTokenManager& XiaomiGateway::XiaomiGatewayTokenManager::GetInstance()
{
	static XiaomiGateway::XiaomiGatewayTokenManager instance;
	return instance;
}

void XiaomiGateway::XiaomiGatewayTokenManager::UpdateTokenSID(const std::string & ip, const std::string & token, const std::string & sid)
{
	bool found = false;
	std::unique_lock<std::mutex> lock(m_mutex);
	for (unsigned i = 0; i < m_GatewayTokens.size(); i++) {
		if (boost::get<0>(m_GatewayTokens[i]) == ip) {
			boost::get<1>(m_GatewayTokens[i]) = token;
			boost::get<2>(m_GatewayTokens[i]) = sid;
			found = true;
		}
	}
	if (!found) {
		m_GatewayTokens.push_back(boost::make_tuple(ip, token, sid));
	}

}

std::string XiaomiGateway::XiaomiGatewayTokenManager::GetToken(const std::string & ip)
{
	std::string token = "";
	bool found = false;
	std::unique_lock<std::mutex> lock(m_mutex);
	for (unsigned i = 0; i < m_GatewayTokens.size(); i++) {
		if (boost::get<0>(m_GatewayTokens[i]) == ip) {
			token = boost::get<1>(m_GatewayTokens[i]);
		}
	}
	return token;
}

std::string XiaomiGateway::XiaomiGatewayTokenManager::GetSID(const std::string & ip)
{
	std::string sid = "";
	bool found = false;
	std::unique_lock<std::mutex> lock(m_mutex);
	for (unsigned i = 0; i < m_GatewayTokens.size(); i++) {
		if (boost::get<0>(m_GatewayTokens[i]) == ip) {
			sid = boost::get<2>(m_GatewayTokens[i]);
		}
	}
	return sid;
}
