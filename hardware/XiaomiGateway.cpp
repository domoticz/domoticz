#include "stdafx.h"
#include "../main/Logger.h"
#include "../main/Helper.h"
#include "../main/SQLHelper.h"
#include "../main/localtime_r.h"
#include "../hardware/hardwaretypes.h"
#include "../main/mainworker.h"
#include "../main/WebServer.h"
#include "../webserver/cWebem.h"
#include "../json/json.h"
#include "XiaomiGateway.h"
#include <openssl/aes.h>
#include <boost/asio.hpp>
#include <boost/bind.hpp>

/*
Xiaomi (Aqara) makes a smart home gateway/hub that has support
for a variety of Xiaomi sensors.
They can be purchased on AliExpress or other stores at very
competitive prices.
Protocol is Zigbee and WiFi, and the gateway and
Domoticz need to be in the same network/subnet
*/

#define round(a) ( int ) ( a + .5 )
std::vector<std::string> arrAqara_Wired_ID;

XiaomiGateway::XiaomiGateway(const int ID)
{
	m_HwdID = ID;
	m_bDoRestart = false;
	m_stoprequested = false;
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
	bool isctrl = false;

	if (packettype == pTypeGeneralSwitch) {
		_tGeneralSwitch *xcmd = (_tGeneralSwitch*)pdata;

			char szTmp[50];
			sprintf(szTmp, "%08X", (unsigned int)xcmd->id);
			std::string ID = szTmp;
			std::stringstream s_strid2;
			s_strid2 << std::hex << ID;
			std::string sid = s_strid2.str();
			std::transform(sid.begin(), sid.end(), sid.begin(), ::tolower);
			//append 158d00 to the front
			//158d0001190076
			sid.insert(0, "158d00");
			//check that switch is STYPE_OnOff
				std::string cmdchannel = "";
				std::string cmdcommand = "";
				std::string cmddevice = "";
				bool isctrl2 = false;
				for (unsigned i = 0; i < arrAqara_Wired_ID.size(); i++) {
					if (arrAqara_Wired_ID[i] == sid) {
						//this device is ctrl2..
						isctrl2 = true;
						isctrl = true;
						if (xcmd->unitcode == 1) {
							cmdchannel = "\\\"channel_0\\\":";
						}
						else if (xcmd->unitcode == 2) {
							cmdchannel = "\\\"channel_1\\\":";
						}
					}
				}
				if (isctrl2) {
					cmddevice = "ctrl_neutral2";
				}
				else {
					cmddevice = "ctrl_neutral1";
				}
				if (xcmd->cmnd == 0) {
					cmdcommand = "\\\"off";
				}
				else if (xcmd->cmnd == 1) {
					cmdcommand = "\\\"on";
				}
				message = "{\"cmd\":\"write\",\"model\":\"" + cmddevice + "\",\"sid\":\"" + sid + "\",\"short_id\":0,\"data\":\"{" + cmdchannel + cmdcommand + "\\\",\\\"key\\\":\\\"@gatewaykey\\\"}\" }";
			//"{\"cmd\":\"write\",\"model\":\"ctrl_neutral2\",\"sid\":\"" + sid + "\",\"short_id\":0,\"data\":\"{" + cmdchannel + cmdcommand + "\\\",\\\"key\\\":\\\"@gatewaykey\\\"}\" }"
				//message = "{\"cmd\":\"write\",\"model\":\"ctrl_neutral2\",\"sid\":\"" + sid + "\",\"short_id\":0,\"data\":\"{" + cmdchannel + "\\\",\\\"key\\\":\\\"@gatewaykey\\\"}\" }";


		/*if (xcmd->subtype == sSwitchTypeSelector) {
			std::string cmdchannel = "";
			if (xcmd->level == 10) {
				cmdchannel = "\\\"channel_0\\\":\\\"on";
			}
			else if (xcmd->level == 20) {
				cmdchannel = "\\\"channel_0\\\":\\\"off";
			}
			else if (xcmd->level == 30) {
				cmdchannel = "\\\"channel_1\\\":\\\"on";
			}
			else if (xcmd->level == 40) {
				cmdchannel = "\\\"channel_1\\\":\\\"off";
			}
			message = "{\"cmd\":\"write\",\"model\":\"ctrl_neutral2\",\"sid\":\"" + sid + "\",\"short_id\":0,\"data\":\"{" + cmdchannel + "\\\",\\\"key\\\":\\\"@gatewaykey\\\"}\" }";
		}*/
		if ((xcmd->subtype == sSwitchGeneralSwitch) && (!isctrl)) { // added bool to avoid sending command if ID belong to ctrl_neutrals devices
			std::string command = "on";
			switch (xcmd->cmnd) {
			case gswitch_sOff:
				command = "off";
				break;
			case gswitch_sOn:
				command = "on";
				break;
			default:
				_log.Log(LOG_STATUS, "XiaomiGateway: Unknown command %d", xcmd->cmnd);
				break;
			}
			message = "{\"cmd\":\"write\",\"model\":\"plug\",\"sid\":\"" + sid + "\",\"short_id\":9844,\"data\":\"{\\\"channel_0\\\":\\\"" + command + "\\\",\\\"key\\\":\\\"@gatewaykey\\\"}\" }";
		}
	}
	else if (packettype == pTypeLimitlessLights) {
		//Gateway RGB Controller
		_tLimitlessLights *xcmd = (_tLimitlessLights*)pdata;
		char szTmp[50];
		sprintf(szTmp, "%08X", (unsigned int)xcmd->id);
		std::string ID = szTmp;
		std::stringstream s_strid;
		s_strid << std::hex << ID;

		std::string sid = s_strid.str();
		std::transform(sid.begin(), sid.end(), sid.begin(), ::tolower);
		//append f0b4 to the front
		//sid.insert(0, "f0b4");
		sid.insert(0, m_GatewayPrefix);

		if (xcmd->command == Limitless_LedOn) {
			m_GatewayBrightnessInt = 100;
			message = "{\"cmd\":\"write\",\"model\":\"gateway\",\"sid\":\"" + sid + "\",\"short_id\":0,\"data\":\"{\\\"rgb\\\":4294967295,\\\"key\\\":\\\"@gatewaykey\\\"}\" }";
		}
		else if (xcmd->command == Limitless_LedOff) {
			m_GatewayBrightnessInt = 0;
			message = "{\"cmd\":\"write\",\"model\":\"gateway\",\"sid\":\"" + sid + "\",\"short_id\":0,\"data\":\"{\\\"rgb\\\":0,\\\"key\\\":\\\"@gatewaykey\\\"}\" }";
		}
		else if (xcmd->command == Limitless_SetRGBColour) {
			int red, green, blue;
			float cHue = (360.0f / 255.0f)*float(xcmd->value);//hue given was in range of 0-255
			hue2rgb(cHue, red, green, blue, 255);
			std::stringstream sstr;
			sstr << std::setw(2) << std::uppercase << std::hex << std::setfill('0') << std::hex << red
				<< std::setw(2) << std::uppercase << std::hex << std::setfill('0') << std::hex << green
				<< std::setw(2) << std::uppercase << std::hex << std::setfill('0') << std::hex << blue;

			std::string hexstring(sstr.str());
			m_GatewayRgbHex = hexstring;
		}
		else if ((xcmd->command == Limitless_SetBrightnessLevel) || (xcmd->command == Limitless_SetBrightUp) || (xcmd->command == Limitless_SetBrightDown)) {
			std::string brightnessAndRgbHex = m_GatewayRgbHex;
			//add the brightness
			if (xcmd->command == Limitless_SetBrightUp) {
				//m_GatewayBrightnessInt = std::min(m_GatewayBrightnessInt + 10, 100);
			}
			else if (xcmd->command == Limitless_SetBrightDown) {
				//m_GatewayBrightnessInt = std::max(m_GatewayBrightnessInt - 10, 0);
			}
			else {
				m_GatewayBrightnessInt = (int)xcmd->value;
			}
			std::stringstream stream;
			stream << std::hex << m_GatewayBrightnessInt;
			std::string brightnessHex = stream.str();
			brightnessAndRgbHex.insert(0, brightnessHex.c_str());
			//_log.Log(LOG_STATUS, "XiaomiGateway: brightness and rgb hex %s", brightnessAndRgbHex.c_str());
			unsigned long hexvalue = std::strtoul(brightnessAndRgbHex.c_str(), 0, 16);
			std::string rgbvalue;
			std::stringstream strstream;
			strstream << hexvalue;
			strstream >> rgbvalue;
			message = "{\"cmd\":\"write\",\"model\":\"gateway\",\"sid\":\"" + sid + "\",\"short_id\":0,\"data\":\"{\\\"rgb\\\":" + rgbvalue + ",\\\"key\\\":\\\"@gatewaykey\\\"}\" }";
		}
		else if (xcmd->command == Limitless_SetColorToWhite) {
			//ignore Limitless_SetColorToWhite
		}
		else {
			_log.Log(LOG_STATUS, "XiaomiGateway: Unknown command %d", xcmd->command);
		}
	}
	if (message != "") {
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
	boost::shared_ptr<std::string> message1(new std::string(message));
	boost::asio::ip::udp::endpoint remote_endpoint_;
	remote_endpoint_ = boost::asio::ip::udp::endpoint(boost::asio::ip::address::from_string(m_GatewayIp), 9898);
	socket_.send_to(boost::asio::buffer(*message1), remote_endpoint_);
	sleep_milliseconds(150);
	boost::array<char, 512> recv_buffer_;
	memset(&recv_buffer_[0], 0, sizeof(recv_buffer_));
#ifdef _DEBUG
	_log.Log(LOG_STATUS, "XiaomiGateway: request %s", message.c_str());
#endif
	while (socket_.available() > 0) {
		socket_.receive_from(boost::asio::buffer(recv_buffer_), remote_endpoint_);
		std::string receivedString(recv_buffer_.data());
		std::size_t found = receivedString.find("Invalid key");
		if (found != std::string::npos) {
			_log.Log(LOG_ERROR, "XiaomiGateway: unable to write command - Invalid Key");
			result = false;
		}
#ifdef _DEBUG
		_log.Log(LOG_STATUS, "XiaomiGateway: response %s", receivedString.c_str());
#endif
	}
	socket_.close();
	return result;
}

void XiaomiGateway::InsertUpdateTemperature(const std::string &nodeid, const std::string &Name, const float Temperature)
{
	// Make sure the ID supplied fits with what is expected ie 158d0000fd32c2
	if (nodeid.length() < 14) {
		_log.Log(LOG_ERROR, "XiaomiGateway: Node ID %s is too short", nodeid.c_str());
		return;
	}
	std::string str = nodeid.substr(6, 8);
#ifdef _DEBUG
	_log.Log(LOG_STATUS, "XiaomiGateway: Temperature - nodeid: %s", nodeid.c_str());
#endif
	unsigned int sID;
	std::stringstream ss;
	ss << std::hex << str.c_str();
	ss >> sID;

	SendTempSensor(sID, 255, Temperature, Name.c_str());
}

void XiaomiGateway::InsertUpdateHumidity(const std::string &nodeid, const std::string &Name, const int Humidity)
{
	// Make sure the ID supplied fits with what is expected ie 158d0000fd32c2
	if (nodeid.length() < 14) {
		_log.Log(LOG_ERROR, "XiaomiGateway: Node ID %s is too short", nodeid.c_str());
		return;
	}
	std::string str = nodeid.substr(6, 8);
#ifdef _DEBUG
	_log.Log(LOG_STATUS, "XiaomiGateway: Humidity - nodeid: %s", nodeid.c_str());
#endif
	unsigned int sID;
	std::stringstream ss;
	ss << std::hex << str.c_str();
	ss >> sID;

	SendHumiditySensor(sID, 255, Humidity, Name.c_str());
}

void XiaomiGateway::InsertUpdateRGBGateway(const std::string & nodeid, const std::string & Name, const bool bIsOn, const int brightness, const int hue)
{
	if (nodeid.length() < 12) {
		_log.Log(LOG_ERROR, "XiaomiGateway: Node ID %s is too short", nodeid.c_str());
		return;
	}
	m_GatewayPrefix = nodeid.substr(0, 4);
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
	result = m_sql.safe_query("SELECT nValue, LastLevel FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='%q') AND (Type==%d) AND (SubType==%d)", m_HwdID, szDeviceID, pTypeLimitlessLights, sTypeLimitlessRGBW);

	if (result.size() < 1)
	{
		_log.Log(LOG_STATUS, "XiaomiGateway: New Gateway Found (%s/%s)", str.c_str(), Name.c_str());
		//int value = atoi(brightness.c_str());
		//int value = hue; // atoi(hue.c_str());
		int cmd = light1_sOn;
		if (!bIsOn) {
			cmd = light1_sOff;
		}
		_tLimitlessLights ycmd;
		ycmd.len = sizeof(_tLimitlessLights) - 1;
		ycmd.type = pTypeLimitlessLights;
		ycmd.subtype = sTypeLimitlessRGBW;
		ycmd.id = sID;
		//ycmd.dunit = 0;
		ycmd.value = brightness;
		ycmd.command = cmd;
		m_mainworker.PushAndWaitRxMessage(this, (const unsigned char *)&ycmd, NULL, -1);
		m_sql.safe_query("UPDATE DeviceStatus SET Name='%q', SwitchType=%d, LastLevel=%d WHERE(HardwareID == %d) AND (DeviceID == '%s')", Name.c_str(), (STYPE_Dimmer), brightness, m_HwdID, szDeviceID);
	}
	else {
		nvalue = atoi(result[0][0].c_str());
		tIsOn = (nvalue != 0);
		lastLevel = atoi(result[0][1].c_str());
		//int value = atoi(brightness.c_str());
		if ((bIsOn != tIsOn) || (brightness != lastLevel))
		{
			int cmd = Limitless_LedOn;
			if (!bIsOn) {
				cmd = Limitless_LedOff;
			}
			_tLimitlessLights ycmd;
			ycmd.len = sizeof(_tLimitlessLights) - 1;
			ycmd.type = pTypeLimitlessLights;
			ycmd.subtype = sTypeLimitlessRGBW;
			ycmd.id = sID;
			//ycmd.dunit = 0;
			ycmd.value = brightness;
			ycmd.command = cmd;
			m_mainworker.PushAndWaitRxMessage(this, (const unsigned char *)&ycmd, NULL, -1);
		}
	}
}

void XiaomiGateway::InsertUpdateSwitch(const std::string &nodeid, const std::string &Name, const bool bIsOn, const _eSwitchType switchtype, const int level, const bool isctlr2, const bool is2ndchannel)
{
	// Make sure the ID supplied fits with what is expected ie 158d0000fd32c2
	if (nodeid.length() < 14) {
		_log.Log(LOG_ERROR, "XiaomiGateway: Node ID %s is too short", nodeid.c_str());
		return;
	}
	std::string str = nodeid.substr(6, 8);

	unsigned int sID;
	std::stringstream ss;
	ss << std::hex << str.c_str();
	ss >> sID;

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

	if (switchtype == STYPE_Selector) {
		xcmd.subtype = sSwitchTypeSelector;
		if (level > 0) {
			xcmd.level = level;
		}
	}

	if (bIsOn) {
		xcmd.cmnd = gswitch_sOn;
	}
	else {
		xcmd.cmnd = gswitch_sOff;
	}

	//check if this switch is already in the database
	std::vector<std::vector<std::string> > result;
	result = m_sql.safe_query("SELECT nValue FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='%q') AND (Type==%d) ", m_HwdID, ID.c_str(), pTypeGeneralSwitch);
	if (result.size() < 1)
	{
		_log.Log(LOG_STATUS, "XiaomiGateway: New Device Found (%s)", str.c_str());
		m_mainworker.PushAndWaitRxMessage(this, (const unsigned char *)&xcmd, NULL, -1);
		int customimage = 0;
		if (switchtype == STYPE_OnOff) {
			customimage = 1;
		}
		else if (switchtype == STYPE_Selector) {
			customimage = 9;
		}
		if (isctlr2 == true) {
			m_sql.safe_query("UPDATE DeviceStatus SET Name='Xiaomi Wired Switch 1', SwitchType=%d, CustomImage=%i, Unit='1' WHERE(HardwareID == %d) AND (DeviceID == '%q') AND (Unit == '1')", (switchtype), customimage, m_HwdID, ID.c_str());
			xcmd.unitcode = 2;
			m_mainworker.PushAndWaitRxMessage(this, (const unsigned char *)&xcmd, NULL, -1);
			m_sql.safe_query("UPDATE DeviceStatus SET Name='Xiaomi Wired Switch 2', SwitchType=%d, CustomImage=%i, Unit='2' WHERE(HardwareID == %d) AND (DeviceID == '%q') AND (Unit == '2')", (switchtype), customimage, m_HwdID, ID.c_str());
		}
		else {
			m_sql.safe_query("UPDATE DeviceStatus SET Name='%q', SwitchType=%d, CustomImage=%i WHERE(HardwareID == %d) AND (DeviceID == '%q')", Name.c_str(), (switchtype), customimage, m_HwdID, ID.c_str());
		}
		if (switchtype == STYPE_Selector) {
			result = m_sql.safe_query("SELECT ID FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='%q') AND (Type==%d) ", m_HwdID, ID.c_str(), pTypeGeneralSwitch);
			if (result.size() > 0) {
				std::string Idx = result[0][0];
				if (Name == "Xiaomi Wireless Switch") {
					m_sql.SetDeviceOptions(atoi(Idx.c_str()), m_sql.BuildDeviceOptions("SelectorStyle:0;LevelNames:Off|Click|Long Click|Long Click Release|Double Click", false));
				}
				else if (Name == "Xiaomi Cube") {
					// flip90/flip180/move/tap_twice/shake_air/swing/alert/free_fall
					m_sql.SetDeviceOptions(atoi(Idx.c_str()), m_sql.BuildDeviceOptions("SelectorStyle:0;LevelNames:Off|flip90|flip180|move|tap_twice|shake_air|swing|alert|free_fall|clock_wise|anti_clock_wise", false));
				}
				else if (Name == "Xiaomi Wireless Dual Wall Switch") {
					//for Aqara wireless switch, 2 buttons support
					m_sql.SetDeviceOptions(atoi(Idx.c_str()), m_sql.BuildDeviceOptions("SelectorStyle:0;LevelNames:Both Click|Switch 1|Switch 2", false));
				}
				else if (Name == "Xiaomi Wired Single Wall Switch") {
					//for Aqara wired switch, single button support
					m_sql.SetDeviceOptions(atoi(Idx.c_str()), m_sql.BuildDeviceOptions("SelectorStyle:0;LevelNames:Off|Switch1 On|Switch1 Off", false));
				}
				else if (Name == "Xiaomi Wireless Single Wall Switch") {
					//for Aqara wireless switch, single button support
					m_sql.SetDeviceOptions(atoi(Idx.c_str()), m_sql.BuildDeviceOptions("SelectorStyle:0;LevelNames:Off|Switch 1", false));
				}
			}
		}
	}
	else {
		if (is2ndchannel) {
			xcmd.unitcode = 2;
		}
		m_mainworker.PushAndWaitRxMessage(this, (const unsigned char *)&xcmd, NULL, -1);
	}
}

void XiaomiGateway::InsertUpdateCubeText(const std::string & nodeid, const std::string & Name, const std::string &degrees)
{
	// Make sure the ID supplied fits with what is expected ie 158d0000fd32c2
	if (nodeid.length() < 14) {
		_log.Log(LOG_ERROR, "XiaomiGateway: Node ID %s is too short", nodeid.c_str());
		return;
	}
	std::string str = nodeid.substr(6, 8);
#ifdef _DEBUG
	_log.Log(LOG_STATUS, "XiaomiGateway: cube nodeid: %s", nodeid.c_str());
#endif
	unsigned int sID;
	std::stringstream ss;
	ss << std::hex << str.c_str();
	ss >> sID;

	char szTmp[300];
	if (sID == 1)
		sprintf(szTmp, "%d", 1);
	else
		sprintf(szTmp, "%08X", (unsigned int)sID);
	std::string ID = szTmp;

	SendTextSensor(sID, sID, 255, degrees.c_str(), Name.c_str());

}

void XiaomiGateway::InsertUpdateVoltage(const std::string & nodeid, const std::string & Name, const int BatteryLevel)
{
	if (nodeid.length() < 14) {
		_log.Log(LOG_ERROR, "XiaomiGateway: Node ID %s is too short", nodeid.c_str());
		return;
	}
	std::string str = nodeid.substr(6, 8);
	unsigned int sID;
	std::stringstream ss;
	ss << std::hex << str.c_str();
	ss >> sID;

	char szTmp[300];
	if (sID == 1)
		sprintf(szTmp, "%d", 1);
	else
		sprintf(szTmp, "%08X", (unsigned int)sID);

	SendVoltageSensor(sID, sID, BatteryLevel, 3, "Xiaomi Voltage");
}

void XiaomiGateway::UpdateToken(const std::string & value)
{
	boost::lock_guard<boost::mutex> lock(m_mutex);
	m_token = value;
#ifdef _DEBUG
	_log.Log(LOG_STATUS, "XiaomiGateway: Token Set - %s", m_token.c_str());
#endif
}

bool XiaomiGateway::StartHardware()
{
	m_stoprequested = false;
	m_bDoRestart = false;

	//force connect the next first time
	m_bIsStarted = true;

	//check there is only one instance of the Xiaomi Gateway
	std::vector<std::vector<std::string> > result;
	result = m_sql.safe_query("SELECT Password, Address FROM Hardware WHERE Type=%d", HTYPE_XiaomiGateway);
	if (result.size() > 1)
	{
		_log.Log(LOG_ERROR, "XiaomiGateway: Only one Xiaomi Gateway is supported, please remove any duplicates from Hardware");
		return false;
	}
	else {
		//retrieve the gateway key
		m_GatewayPassword = result[0][0].c_str();
		m_GatewayIp = result[0][1].c_str();
		m_GatewayRgbHex = "FFFFFF";
		m_GatewayBrightnessInt = 100;
		m_GatewayPrefix = "f0b4";
	}

	//Start worker thread
	m_thread = boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&XiaomiGateway::Do_Work, this)));

	return (m_thread != NULL);
}

bool XiaomiGateway::StopHardware()
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

void XiaomiGateway::Do_Work()
{
	_log.Log(LOG_STATUS, "XiaomiGateway: Worker started...");
	boost::asio::io_service io_service;
	XiaomiGateway::xiaomi_udp_server udp_server(io_service, m_HwdID, this);
	boost::thread bt(boost::bind(&boost::asio::io_service::run, &io_service));

	int sec_counter = 0;
	while (!m_stoprequested)
	{
		sleep_seconds(1);
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
	_log.Log(LOG_STATUS, "XiaomiGateway: stopped");
}

std::string XiaomiGateway::GetGatewayKey()
{
#ifdef WWW_ENABLE_SSL

	const unsigned char *key = (unsigned char *)m_GatewayPassword.c_str();
	unsigned char iv[AES_BLOCK_SIZE] = { 0x17, 0x99, 0x6d, 0x09, 0x3d, 0x28, 0xdd, 0xb3, 0xba, 0x69, 0x5a, 0x2e, 0x6f, 0x58, 0x56, 0x2e };
	unsigned char *plaintext = (unsigned char *)m_token.c_str();
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
	_log.Log(LOG_STATUS, "XiaomiGateway: GetGatewayKey Token - %s", m_token.c_str());
	_log.Log(LOG_STATUS, "XiaomiGateway: GetGatewayKey key - %s", gatewaykey);
#endif
	return gatewaykey;
#else
	_log.Log(LOG_ERROR, "XiaomiGateway: GetGatewayKey NO SSL AVAILABLE");
	return std::string("");
#endif
}

XiaomiGateway::xiaomi_udp_server::xiaomi_udp_server(boost::asio::io_service& io_service, int m_HwdID, XiaomiGateway *parent)
	: socket_(io_service, boost::asio::ip::udp::v4())
{
	m_HardwareID = m_HwdID;
	m_XiaomiGateway = parent;
	m_gatewayip = "127.0.0.1";

	try {
		socket_.set_option(boost::asio::ip::udp::socket::reuse_address(true));
		socket_.bind(boost::asio::ip::udp::endpoint(boost::asio::ip::udp::v4(), 9898));
		boost::shared_ptr<std::string> message(new std::string("{\"cmd\":\"whois\"}"));
		boost::asio::ip::udp::endpoint remote_endpoint;
		remote_endpoint = boost::asio::ip::udp::endpoint(boost::asio::ip::address::from_string("224.0.0.50"), 4321);
		socket_.send_to(boost::asio::buffer(*message), remote_endpoint);
	}
	catch (const boost::system::system_error& ex) {
		_log.Log(LOG_ERROR, "XiaomiGateway: %s", ex.code().category().name());
		m_XiaomiGateway->StopHardware();
		return;
	}
	socket_.set_option(boost::asio::ip::multicast::join_group(boost::asio::ip::address::from_string("224.0.0.50")));
	start_receive();
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
#ifdef _DEBUG
		_log.Log(LOG_STATUS, data_);
#endif
		Json::Value root;
		Json::Reader jReader;
		bool showmessage = true;
		bool ret = jReader.parse(data_, root);
		if (!ret) {
			_log.Log(LOG_ERROR, "XiaomiGateway: invalid data received!");
			return;
		}
		else {
			std::string cmd = root["cmd"].asString();
			std::string model = root["model"].asString();
			std::string sid = root["sid"].asString();
			std::string data = root["data"].asString();
			if ((cmd == "report") || (cmd == "read_ack")) {

				Json::Value root2;
				ret = jReader.parse(data.c_str(), root2);
				if (ret) {
					_eSwitchType type = STYPE_END;
					std::string name = "Xiaomi Switch";
					if (model == "motion") {
						type = STYPE_Motion;
						name = "Xiaomi Motion Sensor";
					}
					else if (model == "switch") {
						type = STYPE_Selector;
						name = "Xiaomi Wireless Switch";
					}
					else if (model == "magnet") {
						type = STYPE_Contact;
						name = "Xiaomi Door Sensor";
					}
					else if (model == "plug") {
						type = STYPE_OnOff;
						name = "Xiaomi Smart Plug";
					}
					else if (model == "sensor_ht") {
						name = "Xiaomi Temperature/Humidity";
					}
					else if (model == "cube") {
						name = "Xiaomi Cube";
						type = STYPE_Selector;
					}
					else if (model == "86sw2") {
						name = "Xiaomi Wireless Dual Wall Switch";
						type = STYPE_Selector;
					}
					else if (model == "ctrl_neutral2") {
						name = "Xiaomi Wired Dual Wall Switch";
						//type = STYPE_Selector;
					}
					else if (model == "gateway") {
						name = "Xiaomi RGB Gateway";
					}
					else if (model == "ctrl_neutral1") {
						name = "Xiaomi Wired Single Wall Switch";
						//type = STYPE_Selector;
					}
					else if (model == "86sw1") {
						name = "Xiaomi Wireless Single Wall Switch";
						type = STYPE_PushOn;
					}
					if (type != STYPE_END) {
						std::string status = root2["status"].asString();
						//Aqara's Wireless switch reports per channel
						std::string aqara_wireless1 = root2["channel_0"].asString();
						std::string aqara_wireless2 = root2["channel_1"].asString();
						bool on = false;
						int level = 0;
						if ((status == "motion") || (status == "open") || (status == "no_close") || (status == "on")) {
							on = true;
						}
						else if ((status == "no_motion") || (status == "close") || (status == "off")) {
							on = false;
						}
						else if ((status == "click") || (status == "flip90") || (aqara_wireless1 == "click")) {
							level = 10;
							on = true;
						}
						else if ((status == "long_click_press")  || (status == "flip180") || (aqara_wireless2 == "click")) {
							level = 20;
							on = true;
						}
						else if ((status == "long_click_release") || (status == "move")) {
							level = 30;
							on = true;
						}
						else if ((status == "tap_twice") || (status == "double_click")) {
							level = 40;
							on = true;
						}
						else if (status == "shake_air") {
							level = 50;
							on = true;
						}
						else if (status == "swing") {
							level = 60;
							on = true;
						}
						else if (status == "alert") {
							level = 70;
							on = true;
						}
						else if (status == "free_fall") {
							level = 80;
							on = true;
						}
						std::string no_motion = root2["no_motion"].asString();
						if (no_motion != "") {
							on = false;
						}
						std::string no_close = root2["no_close"].asString();
						if (no_close != "") {
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
							m_XiaomiGateway->InsertUpdateCubeText(sid.c_str(), name, rotate.c_str());
							m_XiaomiGateway->InsertUpdateSwitch(sid.c_str(), name, on, type, level);
						}
						else {
							std::string battery = root2["battery"].asString();
							if (battery != "") {
								m_XiaomiGateway->InsertUpdateVoltage(sid.c_str(), name, atoi(battery.c_str()));
							}
							else {
								m_XiaomiGateway->InsertUpdateSwitch(sid.c_str(), name, on, type, level);
							}
						}
					}
					else if ((name == "Xiaomi Wired Dual Wall Switch") || (name == "Xiaomi Wired Single Wall Switch")) {
						//aqara wired dual switch, bidirectional communiction support
						type = STYPE_OnOff;
						std::string aqara_wired1 = root2["channel_0"].asString();
						std::string aqara_wired2 = root2["channel_1"].asString();
						bool state = false;
						bool xctrl = false;
						if ((aqara_wired1 == "on") || (aqara_wired2 =="on")) {
							state = true;
						}
						bool cid = false;
						for (unsigned i = 0; i < arrAqara_Wired_ID.size(); i++) {
							if (arrAqara_Wired_ID[i] == sid) {
								cid = true;
							}
						}
						if ((cid == false) || (arrAqara_Wired_ID.size() < 1)) {
							arrAqara_Wired_ID.push_back(sid);
						}
						if (name == "Xiaomi Wired Dual Wall Switch") {
							xctrl = true;
						}
						if (aqara_wired1 != "") {
							m_XiaomiGateway->InsertUpdateSwitch(sid.c_str(), name, state, type, 0, xctrl, false);
						}
						else if (aqara_wired2 != "") {
							m_XiaomiGateway->InsertUpdateSwitch(sid.c_str(), name, state, type, 0, xctrl, true);
						}
					}
					else if (name == "Xiaomi Temperature/Humidity") {
						std::string temperature = root2["temperature"].asString();
						std::string humidity = root2["humidity"].asString();
						if (temperature != "") {
							float temp = (float)atoi(temperature.c_str());
							temp = temp / 100;
							m_XiaomiGateway->InsertUpdateTemperature(sid.c_str(), "Xiaomi Temperature", temp);
						}
						if (humidity != "") {
							int hum = atoi(humidity.c_str());
							hum = hum / 100;
							m_XiaomiGateway->InsertUpdateHumidity(sid.c_str(), "Xiaomi Humidity", hum);
						}
					}
					else if (name == "Xiaomi RGB Gateway") {
						std::string rgb = root2["rgb"].asString();
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
						m_XiaomiGateway->InsertUpdateRGBGateway(sid.c_str(), name, on, brightness, 0);
					}
					else {
						_log.Log(LOG_STATUS, "XiaomiGateway: unhandled model: %s", model.c_str());
					}
				}
			}
			else if (cmd == "get_id_list_ack") {
				Json::Value root2;
				ret = jReader.parse(data.c_str(), root2);
				if (ret) {
					for (int i = 0; i < (int)root2.size(); i++) {
						std::string message = "{\"cmd\" : \"read\",\"sid\":\"";
						message.append(root2[i].asString().c_str());
						message.append("\"}");
						boost::shared_ptr<std::string> message1(new std::string(message));
						boost::asio::ip::udp::endpoint remote_endpoint;
						remote_endpoint = boost::asio::ip::udp::endpoint(boost::asio::ip::address::from_string(m_gatewayip), 9898);
						socket_.send_to(boost::asio::buffer(*message1), remote_endpoint);
					}
				}
				showmessage = false;
			}
			else if (cmd == "iam") {
				if (model == "gateway") {
					_log.Log(LOG_STATUS, "XiaomiGateway: RGB Gateway Detected");
					m_XiaomiGateway->InsertUpdateRGBGateway(sid.c_str(), "Xiaomi RGB Gateway", false, 0, 100);
					m_gatewayip = root["ip"].asString();
					//query for list of devices
					std::string message = "{\"cmd\" : \"get_id_list\"}";
					boost::shared_ptr<std::string> message2(new std::string(message));
					boost::asio::ip::udp::endpoint remote_endpoint;
					remote_endpoint = boost::asio::ip::udp::endpoint(boost::asio::ip::address::from_string(m_gatewayip.c_str()), 9898);
					socket_.send_to(boost::asio::buffer(*message2), remote_endpoint);
				}
				showmessage = false;
			}
			else if (cmd == "heartbeat") {
				//update the token and gateway ip address.
#ifdef _DEBUG
				_log.Log(LOG_STATUS, "XiaomiGateway: Token Received - %s", root["token"].asString().c_str());
#endif
				m_XiaomiGateway->UpdateToken(root["token"].asString());
				m_gatewayip = root["ip"].asString();
				showmessage = false;
			}
			else {
				_log.Log(LOG_STATUS, "XiaomiGateway: unknown cmd received: %s", cmd.c_str());
			}
		}
		if (showmessage) {
			_log.Log(LOG_STATUS, data_);
		}
		start_receive();
	}
	else {
		_log.Log(LOG_ERROR, "XiaomiGateway: error in handle_receive");
	}

}
