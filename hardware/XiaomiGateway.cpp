#include "stdafx.h"
#include "../hardware/hardwaretypes.h"
#include "../main/localtime_r.h"
#include "../main/Logger.h"
#include "../main/mainworker.h"
#include "../main/Helper.h"
#include "../main/SQLHelper.h"
#include "../main/WebServer.h"
#include "../webserver/cWebem.h"
#include "../main/json_helper.h"
#include "XiaomiGateway.h"
#include <openssl/aes.h>
#include <boost/asio.hpp>
#include <boost/bind/bind.hpp>

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

// Unitcode ('Unit' in 'Devices' overview)
typedef enum
{
	ACT_ONOFF_PLUG = 1,		    //  1, TODO: also used for channel 0?
	ACT_ONOFF_WIRED_WALL,		    //  2, TODO: what is this? single or dual channel wired wall switch?
	GATEWAY_SOUND_ALARM_RINGTONE,	    //  3, Xiaomi Gateway Alarm Ringtone
	GATEWAY_SOUND_ALARM_CLOCK,	    //  4, Xiaomi Gateway Alarm Clock
	GATEWAY_SOUND_DOORBELL,		    //  5, Xiaomi Gateway Doorbell
	GATEWAY_SOUND_MP3,		    //  6, Xiaomi Gateway MP3
	GATEWAY_SOUND_VOLUME_CONTROL,	    //  7, Xiaomi Gateway Volume
	SELECTOR_WIRED_WALL_SINGLE,	    //  8, Xiaomi Wired Single Wall Switch
	SELECTOR_WIRED_WALL_DUAL_CHANNEL_0, //  9, Xiaomi Wired Dual Wall Switch Channel 0
	SELECTOR_WIRED_WALL_DUAL_CHANNEL_1  // 10, Xiaomi Wired Dual Wall Switch Channel 1
} XiaomiUnitCode;

/****************************************************************************
 ********************************* SWITCHES *********************************
 ****************************************************************************/

// Unknown Xiaomi device
#define NAME_UNKNOWN_XIAOMI "Unknown Xiaomi device"

// Xiaomi Wireless Switch
#define MODEL_SELECTOR_WIRELESS_SINGLE_1 "switch"
#define MODEL_SELECTOR_WIRELESS_SINGLE_2 "remote.b1acn01"
#define NAME_SELECTOR_WIRELESS_SINGLE "Xiaomi Wireless Switch"

// Xiaomi Square Wireless Switch
#define MODEL_SELECTOR_WIRELESS_SINGLE_SQUARE "sensor_switch.aq2"
#define NAME_SELECTOR_WIRELESS_SINGLE_SQUARE "Xiaomi Square Wireless Switch"

// Xiaomi Wireless Single Wall Switch | WXKG03LM
#define MODEL_SELECTOR_WIRELESS_WALL_SINGLE_1 "86sw1"
#define MODEL_SELECTOR_WIRELESS_WALL_SINGLE_2 "remote.b186acn01"
#define NAME_SELECTOR_WIRELESS_WALL_SINGLE "Xiaomi Wireless Single Wall Switch"

// Xiaomi Wireless Dual Wall Switch | WXKG02LM
#define MODEL_SELECTOR_WIRELESS_WALL_DUAL_1 "86sw2"
#define MODEL_SELECTOR_WIRELESS_WALL_DUAL_2 "remote.b286acn01"
#define NAME_SELECTOR_WIRELESS_WALL_DUAL "Xiaomi Wireless Dual Wall Switch"

// Xiaomi Wired Single Wall Switch
#define MODEL_SELECTOR_WIRED_WALL_SINGLE_1 "ctrl_neutral1"
#define MODEL_SELECTOR_WIRED_WALL_SINGLE_2 "ctrl_ln1"
#define MODEL_SELECTOR_WIRED_WALL_SINGLE_3 "ctrl_ln1.aq1"
#define NAME_SELECTOR_WIRED_WALL_SINGLE "Xiaomi Wired Single Wall Switch"

// Xiaomi Wired Dual Wall Switch | QBKG12LM (QBKG26LM)
#define MODEL_SELECTOR_WIRED_WALL_DUAL_1 "ctrl_neutral2"
#define MODEL_SELECTOR_WIRED_WALL_DUAL_2 "ctrl_ln2"
#define MODEL_SELECTOR_WIRED_WALL_DUAL_3 "ctrl_ln2.aq1"
#define MODEL_SELECTOR_WIRED_WALL_DUAL_4 "switch_b2lacn02"
#define NAME_SELECTOR_WIRED_WALL_DUAL "Xiaomi Wired Dual Wall Switch"
#define NAME_SELECTOR_WIRED_WALL_DUAL_CHANNEL_0 "Xiaomi Wired Dual Wall Switch Channel 0"
#define NAME_SELECTOR_WIRED_WALL_DUAL_CHANNEL_1 "Xiaomi Wired Dual Wall Switch Channel 1"

// Xiaomi Smart Push Button
#define MODEL_SELECTOR_WIRELESS_SINGLE_SMART_PUSH "sensor_switch.aq3"
#define NAME_SELECTOR_WIRELESS_SINGLE_SMART_PUSH "Xiaomi Smart Push Button"

// Xiaomi Cube
#define MODEL_SELECTOR_CUBE_V1 "cube"
#define NAME_SELECTOR_CUBE_V1 "Xiaomi Cube"
#define MODEL_SELECTOR_CUBE_AQARA "sensor_cube.aqgl01"
#define NAME_SELECTOR_CUBE_AQARA "Aqara Cube"

/****************************************************************************
 ********************************* SENSORS **********************************
 ****************************************************************************/

// Motion Sensor Xiaomi
#define MODEL_SENSOR_MOTION_XIAOMI "motion"
#define NAME_SENSOR_MOTION_XIAOMI "Xiaomi Motion Sensor"

// Motion Sensor Aqara | RTCGQ11LM
#define MODEL_SENSOR_MOTION_AQARA "sensor_motion.aq2"
#define NAME_SENSOR_MOTION_AQARA "Aqara Motion Sensor"

// Xiaomi Door and Window Sensor | MCCGQ11LM
#define MODEL_SENSOR_DOOR "magnet"
#define MODEL_SENSOR_DOOR_AQARA "sensor_magnet.aq2"
#define NAME_SENSOR_DOOR "Xiaomi Door Sensor"

// Xiaomi Temperature/Humidity | WSDCGQ01LM
#define MODEL_SENSOR_TEMP_HUM_V1 "sensor_ht"
#define NAME_SENSOR_TEMP_HUM_V1 "Xiaomi Temperature/Humidity"

// Xiaomi Aqara Weather | WSDCGQ11LM
#define MODEL_SENSOR_TEMP_HUM_AQARA "weather.v1"
#define NAME_SENSOR_TEMP_HUM_AQARA "Xiaomi Aqara Weather"

// Aqara Vibration Sensor | DJT11LM
#define MODEL_SENSOR_VIBRATION "vibration"
#define NAME_SENSOR_VIBRATION "Aqara Vibration Sensor"

// Smoke Detector (sensor_smoke.v1)
#define MODEL_SENSOR_SMOKE "smoke"
#define NAME_SENSOR_SMOKE "Xiaomi Smoke Detector"

// Xiaomi Gas Detector
#define MODEL_SENSOR_GAS "natgas"
#define NAME_SENSOR_GAS "Xiaomi Gas Detector"

// Xiaomi Water Leak Detector | SJCGQ11LM
#define MODEL_SENSOR_WATER "sensor_wleak.aq1"
#define NAME_SENSOR_WATER "Xiaomi Water Leak Detector"

/****************************************************************************
 ********************************* ACTUATORS ********************************
 ****************************************************************************/

// Xiaomi Smart Plug (plug.v1) | ZNCZ02LM
#define MODEL_ACT_ONOFF_PLUG "plug"
#define NAME_ACT_ONOFF_PLUG "Xiaomi Smart Plug"

// Xiaomi Smart Wall Plug
#define MODEL_ACT_ONOFF_PLUG_WALL_1 "86plug"
#define MODEL_ACT_ONOFF_PLUG_WALL_2 "ctrl_86plug.aq1"
#define NAME_ACT_ONOFF_PLUG_WALL "Xiaomi Smart Wall Plug"

// Xiaomi Curtain
#define MODEL_ACT_BLINDS_CURTAIN "curtain"
#define NAME_ACT_BLINDS_CURTAIN "Xiaomi Curtain"

// [NEW] Aqara LED Light Bulb (Tunable White) // TODO: not implemented yet
#define MODEL_ACT_LIGHT "light.aqcb02"
#define NAME_ACT_LIGHT "Light bulb"

// [NEW] Aqara Aqara Wireless Relay Controller (2 Channels) | LLKZMK11LM // TODO: not implemented yet
#define MODEL_ACT_RELAIS "relay.c2acn01"
#define NAME_ACT_RELAIS "Aqara Wireless Relay Controller"

/****************************************************************************
 ********************************* GATEWAY **********************************
 ****************************************************************************/

// Xiaomi Gateway
#define MODEL_GATEWAY_1 "gateway"
#define MODEL_GATEWAY_2 "gateway.v3" // Mi Control Hub Gateway
#define MODEL_GATEWAY_3 "acpartner.v3"
#define NAME_GATEWAY "Xiaomi RGB Gateway"
#define NAME_GATEWAY_LUX "Xiaomi Gateway Lux"
#define NAME_GATEWAY_SOUND_MP3 "Xiaomi Gateway MP3"
#define NAME_GATEWAY_SOUND_DOORBELL "Xiaomi Gateway Doorbell"
#define NAME_GATEWAY_SOUND_ALARM_CLOCK "Xiaomi Gateway Alarm Clock"
#define NAME_GATEWAY_SOUND_ALARM_RINGTONE "Xiaomi Gateway Alarm Ringtone"
#define NAME_GATEWAY_SOUND_VOLUME_CONTROL "Xiaomi Gateway Volume"

/****************************************************************************
 ********************************* STATES ***********************************
 ****************************************************************************/

#define NAME_CHANNEL_0 "channel_0"
#define NAME_CHANNEL_1 "channel_1"

#define STATE_ON "on"
#define STATE_OFF "off"

#define STATE_OPEN "open"
#define STATE_CLOSE "close"

#define STATE_WATER_LEAK_YES "leak"
#define STATE_WATER_LEAK_NO "no_leak"

#define STATE_MOTION_YES "motion"
#define STATE_MOTION_NO "no_motion"

#define COMMAND_REPORT "report"
#define COMMAND_READ_ACK "read_ack"
#define COMMAND_HEARTBEAT "heartbeat"


std::list<XiaomiGateway*> gatewaylist;
std::mutex gatewaylist_mutex;

XiaomiGateway *XiaomiGateway::GatewayByIp(const std::string &ip)
{
	std::unique_lock<std::mutex> lock(gatewaylist_mutex);
	for (const auto &gateway : gatewaylist)
		if (gateway->GetGatewayIp() == ip)
			return gateway;

	return nullptr;
}

void XiaomiGateway::AddGatewayToList()
{
	XiaomiGateway *maingw = nullptr;
	{
		std::unique_lock<std::mutex> lock(gatewaylist_mutex);
		for (const auto &gateway : gatewaylist)
		{
			if (gateway->IsMainGateway())
			{
				maingw = gateway;
				break;
			};
		};

		if (!maingw)
			SetAsMainGateway();
		else
			maingw->UnSetMainGateway();

		gatewaylist.push_back(this);
	}

	if (maingw)
		maingw->Restart();
}

void XiaomiGateway::RemoveFromGatewayList()
{
	XiaomiGateway *maingw = nullptr;
	{
		std::unique_lock<std::mutex> lock(gatewaylist_mutex);
		gatewaylist.remove(this);
		if (IsMainGateway())
		{
			UnSetMainGateway();

			if (gatewaylist.begin() != gatewaylist.end())
				maingw = *gatewaylist.begin();
		}
	}

	if (maingw)
		maingw->Restart();
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

	for (ifa = myaddrs; ifa != nullptr; ifa = ifa->ifa_next)
	{
		if (ifa->ifa_addr == nullptr)
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

bool XiaomiGateway::WriteToHardware(const char * pdata, const unsigned char length)
{
	const tRBUF *pCmd = reinterpret_cast<const tRBUF *>(pdata);
	unsigned char packettype = pCmd->ICMND.packettype;
	unsigned char subtype = pCmd->ICMND.subtype;
	bool result = true;
	std::string message;

	if (m_GatewaySID.empty())
	{
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
		std::string cmdchannel;
		std::string cmdcommand;
		std::string cmddevice;
		std::string sidtemp = sid;
		sidtemp.insert(0, "158d00");

		cmdchannel = DetermineChannel(xcmd->unitcode);
		cmddevice = DetermineDevice(xcmd->unitcode);
		cmdcommand = DetermineCommand(xcmd->cmnd);

		if (xcmd->unitcode == XiaomiUnitCode::SELECTOR_WIRED_WALL_SINGLE || xcmd->unitcode == XiaomiUnitCode::SELECTOR_WIRED_WALL_DUAL_CHANNEL_0 || 
			xcmd->unitcode == XiaomiUnitCode::SELECTOR_WIRED_WALL_DUAL_CHANNEL_1 || ((xcmd->subtype == sSwitchGeneralSwitch) && (xcmd->unitcode == XiaomiUnitCode::ACT_ONOFF_PLUG))) {
			message = R"({"cmd":"write","model":")" + cmddevice + R"(","sid":"158d00)" + sid + R"(","short_id":0,"data":"{\")" + cmdchannel + R"(\":\")" + cmdcommand +
				  R"(\",\"key\":\"@gatewaykey\"}" })";
		}
		else if (((xcmd->subtype == sSwitchTypeSelector) && (xcmd->unitcode >= XiaomiUnitCode::GATEWAY_SOUND_ALARM_RINGTONE && xcmd->unitcode <= XiaomiUnitCode::GATEWAY_SOUND_DOORBELL)) ||
			 ((xcmd->subtype == sSwitchGeneralSwitch) && (xcmd->unitcode == XiaomiUnitCode::GATEWAY_SOUND_MP3)))
		{
			std::stringstream ss;
			if (xcmd->unitcode == XiaomiUnitCode::GATEWAY_SOUND_MP3) {
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
					if (xcmd->unitcode == XiaomiUnitCode::GATEWAY_SOUND_ALARM_RINGTONE) {
						if (level > 0) { level = (level / 10) - 1; }
					}
					else if (xcmd->unitcode == XiaomiUnitCode::GATEWAY_SOUND_ALARM_CLOCK) {
						if (level > 0) { level = (level / 10) + 19; }
					}
					else if (xcmd->unitcode == XiaomiUnitCode::GATEWAY_SOUND_DOORBELL) {
						if (level > 0) { level = (level / 10) + 9; }
					}
				}
				ss << level;
			}
			m_GatewayMusicId = ss.str();
			//sid.insert(0, m_GatewayPrefix);
			message = R"({"cmd":"write","model":"gateway","sid":")" + m_GatewaySID + R"(","short_id":0,"data":"{\"mid\":)" + m_GatewayMusicId + R"(,\"vol\":)" + m_GatewayVolume +
				  R"(,\"key\":\"@gatewaykey\"}" })";
		}
		else if (xcmd->subtype == sSwitchGeneralSwitch && xcmd->unitcode == XiaomiUnitCode::GATEWAY_SOUND_VOLUME_CONTROL) {
			m_GatewayVolume = std::to_string(xcmd->level);
			//sid.insert(0, m_GatewayPrefix);
			message = R"({"cmd":"write","model":"gateway","sid":")" + m_GatewaySID + R"(","short_id":0,"data":"{\"mid\":)" + m_GatewayMusicId + R"(,\"vol\":)" + m_GatewayVolume +
				  R"(,\"key\":\"@gatewaykey\"}" })";
		}
		else if (xcmd->subtype == sSwitchBlindsT2) {
			int level = xcmd->level;
			if (xcmd->cmnd == 1) {
				level = 100;
			}
			message = R"({"cmd":"write","model":"curtain","sid":"158d00)" + sid + R"(","short_id":9844,"data":"{\"curtain_level\":\")" + std::to_string(level) +
				  R"(\",\"key\":\"@gatewaykey\"}" })";
		}
	}
	else if (packettype == pTypeColorSwitch) {
		// Gateway RGB Controller
		const _tColorSwitch *xcmd = reinterpret_cast<const _tColorSwitch*>(pdata);

		if (xcmd->command == Color_LedOn) {
			m_GatewayBrightnessInt = 100;
			message = R"({"cmd":"write","model":"gateway","sid":")" + m_GatewaySID + R"(","short_id":0,"data":"{\"rgb\":4294967295,\"key\":\"@gatewaykey\"}" })";
		}
		else if (xcmd->command == Color_LedOff) {
			m_GatewayBrightnessInt = 0;
			message = R"({"cmd":"write","model":"gateway","sid":")" + m_GatewaySID + R"(","short_id":0,"data":"{\"rgb\":0,\"key\":\"@gatewaykey\"}" })";
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
				ss << R"({"cmd":"write","model":"gateway","sid":")" << m_GatewaySID << R"(","short_id":0,"data":"{\"rgb\":)" << value << R"(,\"key\":\"@gatewaykey\"}" })";
				message = ss.str();
			}
			else
			{
				_log.Log(LOG_STATUS, "XiaomiGateway: SetRGBColour - Color mode '%d' is unhandled, if you have a suggestion for what it should do, please post on the Domoticz forum", xcmd->color.mode);
			}
		}
		else if ((xcmd->command == Color_SetBrightnessLevel) || (xcmd->command == Color_SetBrightUp) || (xcmd->command == Color_SetBrightDown)) {
			// Add the brightness
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
			ss << R"({"cmd":"write","model":"gateway","sid":")" << m_GatewaySID << R"(","short_id":0,"data":"{\"rgb\":)" << value << R"(,\"key\":\"@gatewaykey\"}" })";
			message = ss.str();
		}
		else if (xcmd->command == Color_SetColorToWhite) {
			// Ignore Color_SetColorToWhite
		}
		else {
			_log.Log(LOG_ERROR, "XiaomiGateway: Unknown command %d", xcmd->command);
		}
	}
	if (!message.empty()) {
		_log.Debug(DEBUG_HARDWARE, "XiaomiGateway: message: '%s'", message.c_str());
		result = SendMessageToGateway(message);
		if (result == false) {
			// Retry, send the message again
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
		bool ret = ParseJSon(receivedString, root);
		if ((ret) && (root.isObject()))
		{
			std::string data = root["data"].asString();
			Json::Value root2;
			ret = ParseJSon(data, root2);
			if ((ret) && (root2.isObject()))
			{
				std::string error = root2["error"].asString();
				if (!error.empty())
				{
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
		m_mainworker.PushAndWaitRxMessage(this, (const unsigned char *)&ycmd, nullptr, -1, m_Name.c_str());
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
			m_mainworker.PushAndWaitRxMessage(this, (const unsigned char *)&ycmd, nullptr, -1, m_Name.c_str());
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

	if ((xcmd.unitcode >= XiaomiUnitCode::GATEWAY_SOUND_ALARM_RINGTONE) && (xcmd.unitcode <= XiaomiUnitCode::GATEWAY_SOUND_VOLUME_CONTROL)) {
		customimage = 8; // Speaker
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

	// Check if this switch is already in the database
	std::vector<std::vector<std::string> > result;

	// Block this device if it is already added for another gateway hardware id
	result = m_sql.safe_query("SELECT nValue FROM DeviceStatus WHERE (HardwareID!=%d) AND (DeviceID=='%q') AND (Type==%d) AND (Unit == '%d')", m_HwdID, ID.c_str(), xcmd.type, xcmd.unitcode);
	if (!result.empty()) {
		return;
	}

	result = m_sql.safe_query("SELECT nValue, BatteryLevel FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='%q') AND (Type==%d) AND (Unit == '%d')", m_HwdID, ID.c_str(), xcmd.type, xcmd.unitcode);
	if (result.empty())
	{
		_log.Log(LOG_STATUS, "XiaomiGateway: New %s Found (%s)", Name.c_str(), nodeid.c_str());
		m_mainworker.PushAndWaitRxMessage(this, (const unsigned char *)&xcmd, nullptr, battery, m_Name.c_str());
		if (customimage == 0) {
			if (switchtype == STYPE_OnOff) {
				customimage = 1; // Wall socket
			}
			else if (switchtype == STYPE_Selector) {
				customimage = 9;
			}
		}

		m_sql.safe_query("UPDATE DeviceStatus SET Name='%q', SwitchType=%d, CustomImage=%i WHERE(HardwareID == %d) AND (DeviceID == '%q') AND (Unit == '%d')", Name.c_str(), (switchtype), customimage, m_HwdID, ID.c_str(), xcmd.unitcode);

		if (switchtype == STYPE_Selector) {
			result = m_sql.safe_query("SELECT ID FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='%q') AND (Type==%d) AND (Unit == '%d')", m_HwdID, ID.c_str(), xcmd.type, xcmd.unitcode);
			if (!result.empty()) {
				std::string Idx = result[0][0];
				if (Name == NAME_SELECTOR_WIRELESS_SINGLE) {
					m_sql.SetDeviceOptions(atoi(Idx.c_str()), m_sql.BuildDeviceOptions("SelectorStyle:0;LevelNames:Off|Click|Double Click|Long Click|Long Click Release", false));
				}
				else if (Name == NAME_SELECTOR_WIRELESS_SINGLE_SQUARE) {
					m_sql.SetDeviceOptions(atoi(Idx.c_str()), m_sql.BuildDeviceOptions("SelectorStyle:0;LevelNames:Off|Click|Double Click", false));
				}
				else if (Name == NAME_SELECTOR_WIRELESS_SINGLE_SMART_PUSH) {
					m_sql.SetDeviceOptions(atoi(Idx.c_str()), m_sql.BuildDeviceOptions("SelectorStyle:0;LevelNames:Off|Click|Shake", false));
				}
				else if (Name == NAME_SELECTOR_CUBE_V1) {
					m_sql.SetDeviceOptions(atoi(Idx.c_str()), m_sql.BuildDeviceOptions("SelectorStyle:0;LevelNames:Off|flip90|flip180|move|tap_twice|shake_air|swing|alert|free_fall|clock_wise|anti_clock_wise", false));
				}
				else if (Name == NAME_SELECTOR_CUBE_AQARA) {
					m_sql.SetDeviceOptions(atoi(Idx.c_str()), m_sql.BuildDeviceOptions("SelectorStyle:0;LevelNames:Off|flip90|flip180|move|tap_twice|shake_air|swing|alert|free_fall|rotate", false));
				}
				else if (Name == NAME_SENSOR_VIBRATION) {
					m_sql.SetDeviceOptions(atoi(Idx.c_str()), m_sql.BuildDeviceOptions("SelectorStyle:0;LevelNames:Off|Tilt|Vibrate|Free Fall", false));
				}
				else if (Name == NAME_SELECTOR_WIRELESS_WALL_DUAL) {
					m_sql.SetDeviceOptions(atoi(Idx.c_str()), m_sql.BuildDeviceOptions("SelectorStyle:0;LevelNames:Off|Switch 1|Switch 2|Both Click|Switch 1 Double Click|Switch 2 Double Click|Both Double Click|Switch 1 Long Click|Switch 2 Long Click|Both Long Click", false));
				}
				else if (Name == NAME_SELECTOR_WIRED_WALL_SINGLE) {
					m_sql.SetDeviceOptions(atoi(Idx.c_str()), m_sql.BuildDeviceOptions("SelectorStyle:0;LevelNames:Off|Switch1 On|Switch1 Off", false));
				}
				else if (Name == NAME_SELECTOR_WIRELESS_WALL_SINGLE) {
					m_sql.SetDeviceOptions(atoi(Idx.c_str()), m_sql.BuildDeviceOptions("SelectorStyle:0;LevelNames:Off|Click|Double Click|Long Click", false));
				}
				else if (Name == NAME_GATEWAY_SOUND_ALARM_RINGTONE) {
					m_sql.SetDeviceOptions(atoi(Idx.c_str()), m_sql.BuildDeviceOptions("SelectorStyle:1;LevelNames:Off|Police siren 1|Police siren 2|Accident tone|Missle countdown|Ghost|Sniper|War|Air Strike|Barking dogs", false));
				}
				else if (Name == NAME_GATEWAY_SOUND_ALARM_CLOCK) {
					m_sql.SetDeviceOptions(atoi(Idx.c_str()), m_sql.BuildDeviceOptions("SelectorStyle:1;LevelNames:Off|MiMix|Enthusiastic|GuitarClassic|IceWorldPiano|LeisureTime|Childhood|MorningStreamlet|MusicBox|Orange|Thinker", false));
				}
				else if (Name == NAME_GATEWAY_SOUND_DOORBELL) {
					m_sql.SetDeviceOptions(atoi(Idx.c_str()), m_sql.BuildDeviceOptions("SelectorStyle:1;LevelNames:Off|Doorbell ring tone|Knock on door|Hilarious|Alarm clock", false));
				}
			}
		}
		else if (switchtype == STYPE_OnOff && Name == NAME_GATEWAY_SOUND_MP3) {
			std::string errorMessage;
			m_sql.AddUserVariable("XiaomiMP3", USERVARTYPE_INTEGER, "10001", errorMessage);
		}
	}
	else {
		int nvalue = atoi(result[0][0].c_str());
		int BatteryLevel = atoi(result[0][1].c_str());

		if (messagetype == "heartbeat") {
			if (battery != 255) {
				BatteryLevel = battery;
				m_sql.safe_query("UPDATE DeviceStatus SET BatteryLevel=%d WHERE(HardwareID == %d) AND (DeviceID == '%q') AND (Unit == '%d')", BatteryLevel, m_HwdID, ID.c_str(), xcmd.unitcode);
			}
		}
		else {
			if ((bIsOn == false && nvalue >= 1) || (bIsOn == true) || (Name == NAME_SELECTOR_WIRED_WALL_DUAL) || (Name == NAME_SELECTOR_WIRED_WALL_SINGLE) || (Name == NAME_ACT_BLINDS_CURTAIN)) {
				m_mainworker.PushAndWaitRxMessage(this, (const unsigned char *)&xcmd, nullptr, BatteryLevel, m_Name.c_str());
			}
		}
		if ((Name == NAME_ACT_ONOFF_PLUG) || (Name == NAME_ACT_ONOFF_PLUG_WALL)) {
			if (!load_power.empty() && !power_consumed.empty())
			{
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
		SendTextSensor(sID, sID, 255, degrees, Name);
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

	// Force connect the next first time
	m_bIsStarted = true;

	m_GatewayMusicId = "10000";
	m_GatewayVolume = "20";

	std::vector<std::vector<std::string> > result;
	result = m_sql.safe_query("SELECT Password, Address FROM Hardware WHERE Type=%d AND ID=%d AND Enabled=1", HTYPE_XiaomiGateway, m_HwdID);

	if (result.empty())
		return false;

	m_GatewayPassword = result[0][0];
	m_GatewayIp = result[0][1];

	m_GatewayRgbR = 255;
	m_GatewayRgbG = 255;
	m_GatewayRgbB = 255;
	m_GatewayBrightnessInt = 100;
	// Check for presence of Xiaomi user variable to enable message output
	m_OutputMessage = false;
	result = m_sql.safe_query("SELECT Value FROM UserVariables WHERE (Name == 'XiaomiMessage')");
	if (!result.empty()) {
		m_OutputMessage = true;
	}
	// Check for presence of Xiaomi user variable to enable additional voltage devices
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

	// Start worker thread
	m_thread = std::make_shared<std::thread>(&XiaomiGateway::Do_Work, this);
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
	// Find the local ip address that is similar to the xiaomi gateway
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

	// Try finding local ip using ifaddrs when Boost.Asio fails
	if (m_LocalIp.empty())
	{
		try {
			// Get first 2 octets of Xiaomi gateway ip to search for similar ip address
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
			m_LastHeartbeat = mytime(nullptr);
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
		// Gateway
		str = nodeid.substr(4, 8);
	}
	else {
		// Device
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
			if (!m_localip.empty())
			{
				boost::system::error_code ec;
				boost::asio::ip::address listen_addr = boost::asio::ip::address::from_string(m_localip, ec);
				boost::asio::ip::address mcast_addr = boost::asio::ip::address::from_string("224.0.0.50", ec);
				boost::asio::ip::udp::endpoint listen_endpoint(mcast_addr, 9898);

				socket_.bind(boost::asio::ip::udp::endpoint(boost::asio::ip::udp::v4(), 9898));
				std::shared_ptr<std::string> message(new std::string(R"({"cmd":"whois"})"));
				boost::asio::ip::udp::endpoint remote_endpoint;
				remote_endpoint = boost::asio::ip::udp::endpoint(mcast_addr, 4321);
				socket_.send_to(boost::asio::buffer(*message), remote_endpoint);
				socket_.set_option(boost::asio::ip::multicast::join_group(mcast_addr.to_v4(), listen_addr.to_v4()), ec);
				socket_.bind(listen_endpoint, ec);
			}
			else {
				socket_.bind(boost::asio::ip::udp::endpoint(boost::asio::ip::udp::v4(), 9898));
				std::shared_ptr<std::string> message(new std::string(R"({"cmd":"whois"})"));
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
		bool showmessage = true;
		bool ret = ParseJSon(data_, root);
		if ((!ret) || (!root.isObject()))
		{
			_log.Log(LOG_ERROR, "XiaomiGateway: invalid data received!");
			start_receive();
			return;
		}
		std::string cmd = root["cmd"].asString();
		std::string model = root["model"].asString();
		std::string sid = root["sid"].asString();
		std::string data = root["data"].asString();
		int unitcode = 1;
		if ((cmd == COMMAND_REPORT) || (cmd == COMMAND_READ_ACK) || (cmd == COMMAND_HEARTBEAT))
		{
			Json::Value root2;
			ret = ParseJSon(data, root2);
			if ((ret) || (!root2.isObject()))
			{
				_eSwitchType type = STYPE_END;
				std::string name = NAME_UNKNOWN_XIAOMI;
				if (model == MODEL_SENSOR_MOTION_XIAOMI)
				{
					type = STYPE_Motion;
					name = NAME_SENSOR_MOTION_XIAOMI;
				}
				else if (model == MODEL_SENSOR_MOTION_AQARA)
				{
					type = STYPE_Motion;
					name = NAME_SENSOR_MOTION_AQARA;
				}
				else if ((model == MODEL_SELECTOR_WIRELESS_SINGLE_1) || (model == MODEL_SELECTOR_WIRELESS_SINGLE_2))
				{
					type = STYPE_Selector;
					name = NAME_SELECTOR_WIRELESS_SINGLE;
				}
				else if (model == MODEL_SELECTOR_WIRELESS_SINGLE_SQUARE)
				{
					type = STYPE_Selector;
					name = NAME_SELECTOR_WIRELESS_SINGLE_SQUARE;
				}
				else if (model == MODEL_SELECTOR_WIRELESS_SINGLE_SMART_PUSH)
				{
					type = STYPE_Selector;
					name = NAME_SELECTOR_WIRELESS_SINGLE_SMART_PUSH;
				}
				else if ((model == MODEL_SENSOR_DOOR) || (model == MODEL_SENSOR_DOOR_AQARA))
				{
					type = STYPE_Contact;
					name = NAME_SENSOR_DOOR;
				}
				else if (model == MODEL_ACT_ONOFF_PLUG)
				{
					type = STYPE_OnOff;
					name = NAME_ACT_ONOFF_PLUG;
				}
				else if (model == MODEL_ACT_ONOFF_PLUG_WALL_1 || model == MODEL_ACT_ONOFF_PLUG_WALL_2)
				{
					type = STYPE_OnOff;
					name = NAME_ACT_ONOFF_PLUG_WALL;
				}
				else if (model == MODEL_SENSOR_TEMP_HUM_V1)
				{
					name = NAME_SENSOR_TEMP_HUM_V1;
				}
				else if (model == MODEL_SENSOR_TEMP_HUM_AQARA)
				{
					name = NAME_SENSOR_TEMP_HUM_AQARA;
				}
				else if (model == MODEL_SELECTOR_CUBE_V1)
				{
					type = STYPE_Selector;
					name = NAME_SELECTOR_CUBE_V1;
				}
				else if (model == MODEL_SELECTOR_CUBE_AQARA)
				{
					type = STYPE_Selector;
					name = NAME_SELECTOR_CUBE_AQARA;
				}
				else if (model == MODEL_SENSOR_VIBRATION)
				{
					type = STYPE_Selector;
					name = NAME_SENSOR_VIBRATION;
				}
				else if (model == MODEL_GATEWAY_1 || model == MODEL_GATEWAY_2 || model == MODEL_GATEWAY_3)
				{
					name = NAME_GATEWAY;
				}
				else if (model == MODEL_SELECTOR_WIRED_WALL_SINGLE_1 || model == MODEL_SELECTOR_WIRED_WALL_SINGLE_2 || model == MODEL_SELECTOR_WIRED_WALL_SINGLE_3)
				{
					type = STYPE_END; // type = STYPE_OnOff; // TODO: fix this hack
					name = NAME_SELECTOR_WIRED_WALL_SINGLE;
				}
				else if (model == MODEL_SELECTOR_WIRED_WALL_DUAL_1 || model == MODEL_SELECTOR_WIRED_WALL_DUAL_2 || model == MODEL_SELECTOR_WIRED_WALL_DUAL_3 || model == MODEL_SELECTOR_WIRED_WALL_DUAL_4)
				{
					type = STYPE_END; // type = STYPE_OnOff; // TODO: fix this hack
					name = NAME_SELECTOR_WIRED_WALL_DUAL;
				}
				else if (model == MODEL_SELECTOR_WIRELESS_WALL_SINGLE_1 || model == MODEL_SELECTOR_WIRELESS_WALL_SINGLE_2)
				{
					type = STYPE_Selector;
					name = NAME_SELECTOR_WIRELESS_WALL_SINGLE;
				}
				else if (model == MODEL_SELECTOR_WIRELESS_WALL_DUAL_1 || model == MODEL_SELECTOR_WIRELESS_WALL_DUAL_2)
				{
					type = STYPE_Selector;
					name = NAME_SELECTOR_WIRELESS_WALL_DUAL;
				}
				else if (model == MODEL_SENSOR_SMOKE)
				{
					type = STYPE_SMOKEDETECTOR;
					name = NAME_SENSOR_SMOKE;
				}
				else if (model == MODEL_SENSOR_GAS)
				{
					type = STYPE_SMOKEDETECTOR;
					name = NAME_SENSOR_GAS;
				}
				else if (model == MODEL_SENSOR_WATER)
				{
					type = STYPE_SMOKEDETECTOR;
					name = NAME_SENSOR_WATER;
				}
				else if (model == MODEL_ACT_BLINDS_CURTAIN)
				{
					type = STYPE_BlindsPercentage;
					name = NAME_ACT_BLINDS_CURTAIN;
				}

				std::string voltage = root2["voltage"].asString();
				int battery = 255;
				if (!voltage.empty() && voltage != "3600")
				{
					battery = ((atoi(voltage.c_str()) - 2200) / 10);
				}
				if (type != STYPE_END)
				{
					std::string status = root2["status"].asString();
					std::string no_close = root2["no_close"].asString();
					std::string no_motion = root2["no_motion"].asString();
					// Aqara's Wireless switch reports per channel
					std::string aqara_wireless1 = root2[NAME_CHANNEL_0].asString();
					std::string aqara_wireless2 = root2[NAME_CHANNEL_1].asString();
					std::string aqara_wireless3 = root2["dual_channel"].asString();
					// Smart plug usage
					std::string load_power = root2["load_power"].asString();
					std::string power_consumed = root2["power_consumed"].asString();
					// Smoke or Gas Detector
					std::string density = root2["density"].asString();
					std::string alarm = root2["alarm"].asString();
					// Aqara motion sensor
					std::string lux = root2["lux"].asString();
					// Curtain
					std::string curtain = root2["curtain_level"].asString();
					bool on = false;
					int level = -1;
					if (model == MODEL_SELECTOR_WIRELESS_SINGLE_1)
					{
						level = 0;
					}
					else if (model == MODEL_SENSOR_SMOKE || model == MODEL_SENSOR_GAS || model == MODEL_SENSOR_WATER || model == MODEL_SELECTOR_CUBE_AQARA)
					{
						if (battery != 255 && (model == MODEL_SENSOR_WATER || model == MODEL_SELECTOR_CUBE_AQARA))
						{
							level = 0;
						}
						if ((alarm == "1") || (alarm == "2") || (status == STATE_WATER_LEAK_YES))
						{
							level = 0;
							on = true;
						}
						else if ((alarm == "0") || (status == STATE_WATER_LEAK_NO) || (status == "iam"))
						{
							level = 0;
						}
						if (!density.empty())
							level = atoi(density.c_str());
					}
					if ((status == STATE_MOTION_YES) || (status == STATE_OPEN) || (status == "no_close") || (status == STATE_ON) || (!no_close.empty()))
					{
						level = 0;
						on = true;
					}
					else if ((status == STATE_MOTION_NO) || (status == STATE_CLOSE) || (status == STATE_OFF) || (!no_motion.empty()))
					{
						level = 0;
						on = false;
					}
					else if ((status == "click") || (status == "flip90") || (aqara_wireless1 == "click") || (status == "tilt"))
					{
						level = 10;
						on = true;
					}
					else if ((status == "double_click") || (status == "flip180") || (aqara_wireless2 == "click") || (status == "shake") || (status == "vibrate") ||
						 (name == "Xiaomi Wireless Single Wall Switch" && aqara_wireless1 == "double_click"))
					{
						level = 20;
						on = true;
					}
					else if ((status == "long_click_press") || (status == "move") || (aqara_wireless3 == "both_click") ||
						 (name == "Xiaomi Wireless Single Wall Switch" && aqara_wireless1 == "long_click"))
					{
						level = 30;
						on = true;
					}
					else if ((status == "tap_twice") || (status == "long_click_release") || (name == "Xiaomi Wireless Dual Wall Switch" && aqara_wireless1 == "double_click"))
					{
						level = 40;
						on = true;
					}
					else if ((status == "shake_air") || (aqara_wireless2 == "double_click"))
					{
						level = 50;
						on = true;
					}
					else if ((status == "swing") || (aqara_wireless3 == "double_both_click"))
					{
						level = 60;
						on = true;
					}
					else if ((status == "alert") || (name == "Xiaomi Wireless Dual Wall Switch" && aqara_wireless1 == "long_click"))
					{
						level = 70;
						on = true;
					}
					else if ((status == "free_fall") || (aqara_wireless2 == "long_click"))
					{
						level = 80;
						on = true;
					}
					else if (aqara_wireless3 == "long_both_click")
					{
						level = 90;
						on = true;
					}
					std::string rotate = root2["rotate"].asString();
					if (!rotate.empty())
					{
						int amount = atoi(rotate.c_str());
						if (amount > 0)
						{
							level = 90;
						}
						else
						{
							level = 100;
						}
						on = true;
						TrueGateway->InsertUpdateCubeText(sid, name, rotate);
						TrueGateway->InsertUpdateSwitch(sid, name, on, type, unitcode, level, cmd, "", "", battery);
					}
					else
					{
						if (model == MODEL_ACT_ONOFF_PLUG || model == MODEL_ACT_ONOFF_PLUG_WALL_1 || model == MODEL_ACT_ONOFF_PLUG_WALL_2)
						{
							sleep_milliseconds(100); // Need to sleep here as the gateway will send 2 update messages, and need time for the database to update the state so
										 // that the event is not triggered twice
							TrueGateway->InsertUpdateSwitch(sid, name, on, type, unitcode, level, cmd, load_power, power_consumed, battery);
						}
						else if ((model == MODEL_ACT_BLINDS_CURTAIN) && (!curtain.empty()))
						{
							level = atoi(curtain.c_str());
							TrueGateway->InsertUpdateSwitch(sid, name, on, type, unitcode, level, cmd, "", "", battery);
						}
						else {
							if (level > -1)
							{ // This should stop false updates when empty 'data' is received
								TrueGateway->InsertUpdateSwitch(sid, name, on, type, unitcode, level, cmd, "", "", battery);
							}
							if (!lux.empty())
							{
								TrueGateway->InsertUpdateLux(sid, name, atoi(lux.c_str()), battery);
							}
							if (!voltage.empty() && m_IncludeVoltage)
							{
								TrueGateway->InsertUpdateVoltage(sid, name, atoi(voltage.c_str()));
							}
						}
					}
				}
				else if ((name == NAME_SELECTOR_WIRED_WALL_SINGLE) || (name == NAME_SELECTOR_WIRED_WALL_DUAL))
				{
					// Aqara wired dual switch, bidirectional communication support
					type = STYPE_OnOff; // TODO: Needs to be set above but need different way of executing this code without hack
					std::string aqara_wired1 = root2[NAME_CHANNEL_0].asString();
					std::string aqara_wired2 = root2[NAME_CHANNEL_1].asString();
					bool state = (aqara_wired1 == STATE_ON) || (aqara_wired2 == STATE_ON);

					unitcode = XiaomiUnitCode::SELECTOR_WIRED_WALL_SINGLE;
					if (name == NAME_SELECTOR_WIRED_WALL_SINGLE)
					{
						unitcode = XiaomiUnitCode::SELECTOR_WIRED_WALL_SINGLE;
					}
					else
					{
						unitcode = XiaomiUnitCode::SELECTOR_WIRED_WALL_DUAL_CHANNEL_0;
						name = NAME_SELECTOR_WIRED_WALL_DUAL_CHANNEL_0;
					}
					if (!aqara_wired1.empty())
					{
						TrueGateway->InsertUpdateSwitch(sid, name, state, type, unitcode, 0, cmd, "", "", battery);
					}
					else if (!aqara_wired2.empty())
					{
						unitcode = XiaomiUnitCode::SELECTOR_WIRED_WALL_DUAL_CHANNEL_1;
						name = NAME_SELECTOR_WIRED_WALL_DUAL_CHANNEL_1;
						TrueGateway->InsertUpdateSwitch(sid, name, state, type, unitcode, 0, cmd, "", "", battery);
					}
				}
				else if ((name == NAME_SENSOR_TEMP_HUM_V1) || (name == NAME_SENSOR_TEMP_HUM_AQARA))
				{
					std::string temperature = root2["temperature"].asString();
					std::string humidity = root2["humidity"].asString();
					float pressure = 0;

					if (name == NAME_SENSOR_TEMP_HUM_AQARA)
					{
						std::string szPressure = root2["pressure"].asString();
						pressure = static_cast<float>(atof(szPressure.c_str())) / 100.0F;
					}

					if ((!temperature.empty()) && (!humidity.empty()) && (pressure != 0))
					{
						// Temp+Hum+Baro
						float temp = std::stof(temperature) / 100.0F;
						int hum = static_cast<int>((std::stof(humidity) / 100));
						TrueGateway->InsertUpdateTempHumPressure(sid, "Xiaomi TempHumBaro", temp, hum, pressure, battery);
					}
					else if ((!temperature.empty()) && (!humidity.empty()))
					{
						// Temp+Hum
						float temp = std::stof(temperature) / 100.0F;
						int hum = static_cast<int>((std::stof(humidity) / 100));
						TrueGateway->InsertUpdateTempHum(sid, "Xiaomi TempHum", temp, hum, battery);
					}
					else if (!temperature.empty())
					{
						float temp = std::stof(temperature) / 100.0F;
						if (temp < 99)
						{
							TrueGateway->InsertUpdateTemperature(sid, "Xiaomi Temperature", temp, battery);
						}
					}
					else if (!humidity.empty())
					{
						int hum = static_cast<int>((std::stof(humidity) / 100));
						if (hum > 1)
						{
							TrueGateway->InsertUpdateHumidity(sid, "Xiaomi Humidity", hum, battery);
						}
					}
				}
				else if (name == NAME_GATEWAY)
				{
					std::string rgb = root2["rgb"].asString();
					std::string illumination = root2["illumination"].asString();
					if (!rgb.empty())
					{
						// Only add in the gateway that matches the SID for this hardware.
						if (TrueGateway->GetGatewaySid() == sid)
						{
							std::stringstream ss;
							ss << std::hex << atoi(rgb.c_str());
							std::string hexstring(ss.str());
							if (hexstring.length() == 7)
							{
								hexstring.insert(0, "0");
							}
							std::string bright_hex = hexstring.substr(0, 2);
							std::stringstream ss2;
							ss2 << std::hex << bright_hex.c_str();
							int brightness = strtoul(bright_hex.c_str(), nullptr, 16);
							bool on = false;
							if (rgb != "0")
							{
								on = true;
							}
							TrueGateway->InsertUpdateRGBGateway(sid, name + " (" + TrueGateway->GetGatewayIp() + ")", on, brightness, 0);
							TrueGateway->InsertUpdateLux(sid, NAME_GATEWAY_LUX, atoi(illumination.c_str()), 255);
							TrueGateway->InsertUpdateSwitch(sid, NAME_GATEWAY_SOUND_ALARM_RINGTONE, false, STYPE_Selector, 3, 0, cmd, "", "", 255);
							TrueGateway->InsertUpdateSwitch(sid, NAME_GATEWAY_SOUND_ALARM_CLOCK, false, STYPE_Selector, 4, 0, cmd, "", "", 255);
							TrueGateway->InsertUpdateSwitch(sid, NAME_GATEWAY_SOUND_DOORBELL, false, STYPE_Selector, 5, 0, cmd, "", "", 255);
							TrueGateway->InsertUpdateSwitch(sid, NAME_GATEWAY_SOUND_MP3, false, STYPE_OnOff, 6, 0, cmd, "", "", 255);
							TrueGateway->InsertUpdateSwitch(sid, NAME_GATEWAY_SOUND_VOLUME_CONTROL, false, STYPE_Dimmer, 7, 0, cmd, "", "", 255);
						}
					}
					else
					{
						// Check for token
						std::string token = root["token"].asString();
						std::string ip = root2["ip"].asString();

						if ((!token.empty()) && (!ip.empty()))
						{
							XiaomiGatewayTokenManager::GetInstance().UpdateTokenSID(ip, token, sid);
							showmessage = false;
						}
					}
				}
				else
				{
					_log.Log(LOG_STATUS, "XiaomiGateway (ID=%d): unhandled model: '%s', name: '%s'", TrueGateway->GetGatewayHardwareID(), model.c_str(), name.c_str());
				}
			}
		}
		else if (cmd == "get_id_list_ack")
		{
			Json::Value root2;
			ret = ParseJSon(data, root2);
			if ((ret) || (!root2.isObject()))
			{
				for (const auto &r : root2)
				{
					std::string message = R"({"cmd" : "read","sid":")";
					message.append(r.asString());
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
			if (model == MODEL_GATEWAY_1 || model == MODEL_GATEWAY_2 || model == MODEL_GATEWAY_3)
			{
				std::string ip = root["ip"].asString();
				// Only add in the gateway that matches the IP address for this hardware.
				if (ip == TrueGateway->GetGatewayIp())
				{
					_log.Log(LOG_STATUS, "XiaomiGateway: RGB Gateway Detected");
					TrueGateway->InsertUpdateRGBGateway(sid, "Xiaomi RGB Gateway (" + ip + ")", false, 0, 100);
					TrueGateway->InsertUpdateSwitch(sid, NAME_GATEWAY_SOUND_ALARM_RINGTONE, false, STYPE_Selector, 3, 0, cmd, "", "", 255);
					TrueGateway->InsertUpdateSwitch(sid, NAME_GATEWAY_SOUND_ALARM_CLOCK, false, STYPE_Selector, 4, 0, cmd, "", "", 255);
					TrueGateway->InsertUpdateSwitch(sid, NAME_GATEWAY_SOUND_DOORBELL, false, STYPE_Selector, 5, 0, cmd, "", "", 255);
					TrueGateway->InsertUpdateSwitch(sid, NAME_GATEWAY_SOUND_MP3, false, STYPE_OnOff, 6, 0, cmd, "", "", 255);
					TrueGateway->InsertUpdateSwitch(sid, NAME_GATEWAY_SOUND_VOLUME_CONTROL, false, STYPE_Dimmer, 7, 0, cmd, "", "", 255);

					// Query for list of devices
					std::string message = R"({"cmd" : "get_id_list"})";
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
			_log.Log(LOG_STATUS, "XiaomiGateway (ID=%d): unknown cmd received: '%s', model: '%s'", TrueGateway->GetGatewayHardwareID(), cmd.c_str(), model.c_str());
		}

		if (showmessage && m_OutputMessage) {
			_log.Log(LOG_STATUS, "%s", data_);
		}
		start_receive();
	}
	else {
		_log.Log(LOG_ERROR, "XiaomiGateway: error in handle_receive '%s'", error.message().c_str());
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
	for (auto &m_GatewayToken : m_GatewayTokens)
	{
		if (boost::get<0>(m_GatewayToken) == ip)
		{
			boost::get<1>(m_GatewayToken) = token;
			boost::get<2>(m_GatewayToken) = sid;
			found = true;
		}
	}
	if (!found) {
		m_GatewayTokens.push_back(boost::make_tuple(ip, token, sid));
	}

}

std::string XiaomiGateway::XiaomiGatewayTokenManager::GetToken(const std::string & ip)
{
	std::string token;
	bool found = false;
	std::unique_lock<std::mutex> lock(m_mutex);
	for (auto &m_GatewayToken : m_GatewayTokens)
		if (boost::get<0>(m_GatewayToken) == ip)
			token = boost::get<1>(m_GatewayToken);

	return token;
}

std::string XiaomiGateway::XiaomiGatewayTokenManager::GetSID(const std::string & ip)
{
	std::string sid;
	bool found = false;
	std::unique_lock<std::mutex> lock(m_mutex);
	for (auto &m_GatewayToken : m_GatewayTokens)
		if (boost::get<0>(m_GatewayToken) == ip)
			sid = boost::get<2>(m_GatewayToken);

	return sid;
}

std::string XiaomiGateway::DetermineChannel(int32_t unitcode)
{
	std::string cmdchannel;
	if (unitcode == XiaomiUnitCode::SELECTOR_WIRED_WALL_SINGLE || unitcode == XiaomiUnitCode::ACT_ONOFF_PLUG ||
		unitcode == XiaomiUnitCode::SELECTOR_WIRED_WALL_DUAL_CHANNEL_0) {
		cmdchannel = NAME_CHANNEL_0;
	}
	else if (unitcode == XiaomiUnitCode::SELECTOR_WIRED_WALL_DUAL_CHANNEL_1) {
		cmdchannel = NAME_CHANNEL_1;
	}
	return cmdchannel;
}

std::string XiaomiGateway::DetermineDevice(int32_t unitcode)
{
	std::string cmddevice;
	if (unitcode == XiaomiUnitCode::SELECTOR_WIRED_WALL_SINGLE) {
		cmddevice = "ctrl_neutral1";
	}
	else if (unitcode == XiaomiUnitCode::ACT_ONOFF_PLUG) {
		cmddevice = "plug";
	}
	else if (unitcode == XiaomiUnitCode::SELECTOR_WIRED_WALL_DUAL_CHANNEL_0 || unitcode == XiaomiUnitCode::SELECTOR_WIRED_WALL_DUAL_CHANNEL_1) {
		cmddevice = "ctrl_neutral2";
	}
	return cmddevice;
}

std::string XiaomiGateway::DetermineCommand(uint8_t commandcode)
{
	std::string command;
	switch (commandcode) {
		case gswitch_sOff:
			command = STATE_OFF;
			break;
		case gswitch_sOn:
			command = STATE_ON;
			break;
		default:
			command = "unknown command";
			break;
	}
	return command;
}
