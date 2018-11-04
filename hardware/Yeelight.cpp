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
#include "../json/json.h"

/*
Yeelight (Mi Light) is a company that created White and RGBW lights
You can buy them on AliExpress or Ebay or other storers
Price is around 20 euro
Protocol if WiFi, the light needs to connect to your wireless network
Domoticz and the lights need to be in the same network/subnet
Protocol specification: http://www.yeelight.com/download/Yeelight_Inter-Operation_Spec.pdf
*/

#ifdef _DEBUG
//#define DEBUG_YeeLightR
//#define DEBUG_YeeLightW
#endif

#ifdef DEBUG_YeeLightW
void SaveString2Disk(std::string str, std::string filename)
{
	FILE *fOut = fopen(filename.c_str(), "wb+");
	if (fOut)
	{
		fwrite(str.c_str(), 1, str.size(), fOut);
		fclose(fOut);
	}
}
#endif
#ifdef DEBUG_YeeLightR
std::string ReadFile(std::string filename)
{
	std::string ret;
	std::ifstream file(filename.c_str(), std::ios::in | std::ios::binary);
	if (file)
	{
		ret.append((std::istreambuf_iterator<char>(file)),
			(std::istreambuf_iterator<char>()));
	}
	return ret;
}
#endif

class udp_server;

Yeelight::Yeelight(const int ID)
{
	m_HwdID = ID;
	m_bDoRestart = false;
}

Yeelight::~Yeelight(void)
{
}

bool Yeelight::StartHardware()
{
	RequestStart();

	m_bDoRestart = false;

	//force connect the next first time
	m_bIsStarted = true;

	//Start worker thread
	m_thread = std::make_shared<std::thread>(&Yeelight::Do_Work, this);
	SetThreadNameInt(m_thread->native_handle());

	return (m_thread != nullptr);
}

bool Yeelight::StopHardware()
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

#define YEELIGHT_POLL_INTERVAL 60

void Yeelight::Do_Work()
{
	_log.Log(LOG_STATUS, "YeeLight Worker started...");

	try
	{
		boost::asio::io_service io_service;
		udp_server server(io_service, m_HwdID);
		int sec_counter = YEELIGHT_POLL_INTERVAL - 5;
		while (!IsStopRequested(1000))
		{
			sec_counter++;
			if (sec_counter % 12 == 0) {
				m_LastHeartbeat = mytime(NULL);
			}
			if (sec_counter % 60 == 0) //poll YeeLights every minute
			{
				server.start_send();
				io_service.run();
			}
		}
	}
	catch (const std::exception &e)
	{
		_log.Log(LOG_ERROR, "YeeLight: Exception: %s", e.what());
	}

	_log.Log(LOG_STATUS, "YeeLight stopped");
}


void Yeelight::InsertUpdateSwitch(const std::string &nodeID, const std::string &lightName, const int &YeeType, const std::string &Location, const bool bIsOn, const std::string &yeelightBright, const std::string &syeelightHue,
                                  const std::string &syeelightSat, const std::string &syeelightRGB, const std::string &syeelightCT, const std::string &syeelightColorMode)
{
	std::vector<std::string> ipaddress;
	StringSplit(Location, ".", ipaddress);
	if (ipaddress.size() != 4)
	{
		_log.Log(LOG_STATUS, "YeeLight: Invalid location received! (No IP Address)");
		return;
	}
	uint32_t sID = (uint32_t)(atoi(ipaddress[0].c_str()) << 24) | (uint32_t)(atoi(ipaddress[1].c_str()) << 16) | (atoi(ipaddress[2].c_str()) << 8) | atoi(ipaddress[3].c_str());
	char szDeviceID[20];
	if (sID == 1)
		sprintf(szDeviceID, "%d", 1);
	else
		sprintf(szDeviceID, "%08X", (unsigned int)sID);

	std::vector<std::vector<std::string> > result;
	result = m_sql.safe_query("SELECT nValue, LastLevel, SubType, ID FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='%q') AND (Type==%d)", m_HwdID, szDeviceID, pTypeColorSwitch);
	int yeelightColorMode = atoi(syeelightColorMode.c_str());
	if (yeelightColorMode > 0) {
		_log.Debug(DEBUG_HARDWARE, "Yeelight::InsertUpdateSwitch colorMode: %u, Bri: %s, Hue: %s, Sat: %s, RGB: %s, CT: %s", yeelightColorMode, yeelightBright.c_str(), syeelightHue.c_str(), syeelightSat.c_str(), syeelightRGB.c_str(), syeelightCT.c_str());
	}
	if (result.empty())
	{
		_log.Log(LOG_STATUS, "YeeLight: New Light Found (%s/%s)", Location.c_str(), lightName.c_str());
		int value = atoi(yeelightBright.c_str());
		int cmd = Color_LedOn;
		int level = 100;
		if (!bIsOn) {
			cmd = Color_LedOff;
			level = 0;
		}
		_tColorSwitch ycmd;
		ycmd.subtype = YeeType;
		ycmd.id = sID;
		ycmd.dunit = 0;
		ycmd.value = value;
		ycmd.command = cmd;
		// TODO: Update color
		m_mainworker.PushAndWaitRxMessage(this, (const unsigned char *)&ycmd, NULL, -1);
		m_sql.safe_query("UPDATE DeviceStatus SET Name='%q', SwitchType=%d, LastLevel=%d WHERE(HardwareID == %d) AND (DeviceID == '%q')", lightName.c_str(), (STYPE_Dimmer), value, m_HwdID, szDeviceID);
	}
	else {
		// Make sure subtype is correct
		unsigned sTypeOld = atoi(result[0][2].c_str());
		std::string sIdx = result[0][3];
		if (sTypeOld != YeeType)
		{
			_log.Log(LOG_STATUS, "YeeLight: Updating SubType of light (%s/%s) from %u to %u", Location.c_str(), lightName.c_str(), sTypeOld, YeeType);
			m_sql.UpdateDeviceValue("SubType", (int)YeeType, sIdx);
		}

		int nvalue = atoi(result[0][0].c_str());
		bool tIsOn = (nvalue != 0);
		int lastLevel = atoi(result[0][1].c_str());
		int value = atoi(yeelightBright.c_str());
		if ((bIsOn != tIsOn) || (value != lastLevel))
		{
			int cmd = Color_LedOn;
			if (!bIsOn) {
				cmd = Color_LedOff;
			}
			if (value != lastLevel) {
				cmd = Color_SetBrightnessLevel;
			}
			_tColorSwitch ycmd;
			ycmd.subtype = YeeType;
			ycmd.id = sID;
			ycmd.dunit = 0;
			ycmd.value = value;
			ycmd.command = cmd;
			// TODO: Update color
			m_mainworker.PushAndWaitRxMessage(this, (const unsigned char *)&ycmd, NULL, -1);
		}
	}
}


bool Yeelight::WriteToHardware(const char *pdata, const unsigned char length)
{
	//_log.Log(LOG_STATUS, "YeeLight: WriteToHardware...............................");
	const _tColorSwitch *pLed = reinterpret_cast<const _tColorSwitch*>(pdata);
	uint8_t command = pLed->command;
	std::vector<std::vector<std::string> > result;
	unsigned long lID;
	char szTmp[50];
	bool sendOnFirst = false;

	if (pLed->id == 1)
		sprintf(szTmp, "%d", 1);
	else
		sprintf(szTmp, "%08X", (unsigned int)pLed->id);
	std::string ID = szTmp;

	std::stringstream s_strid;
	s_strid << std::hex << ID;
	s_strid >> lID;

	unsigned char ID1 = (unsigned char)((lID & 0xFF000000) >> 24);
	unsigned char ID2 = (unsigned char)((lID & 0x00FF0000) >> 16);
	unsigned char ID3 = (unsigned char)((lID & 0x0000FF00) >> 8);
	unsigned char ID4 = (unsigned char)((lID & 0x000000FF));

	//IP Address
	sprintf(szTmp, "%d.%d.%d.%d", ID1, ID2, ID3, ID4);

	try
	{
		boost::asio::io_service io_service;
		boost::asio::ip::tcp::socket sendSocket(io_service);
		boost::asio::ip::tcp::resolver resolver(io_service);
		boost::asio::ip::tcp::resolver::query query(boost::asio::ip::tcp::v4(), szTmp, "55443");
		boost::asio::ip::tcp::resolver::iterator iterator = resolver.resolve(query);
		boost::asio::connect(sendSocket, iterator);

		std::string message = "";
		std::string message2 = "";
		char request[1024];
		size_t request_length;
		std::stringstream ss;

		switch (pLed->command)
		{
		case Color_LedOn:
			message = "{\"id\":1,\"method\":\"set_power\",\"params\":[\"on\", \"smooth\", 500]}\r\n";
			break;
		case Color_LedOff:
			message = "{\"id\":1,\"method\":\"set_power\",\"params\":[\"off\", \"smooth\", 500]}\r\n";
			break;
		case Color_LedNight:
			if (pLed->subtype == sTypeColor_RGB_W) {
				message = "{\"id\":1,\"method\":\"set_scene\", \"params\": [\"color\", 16750848, 1]}\r\n";
			}
			else {
				message = "{\"id\":1,\"method\":\"set_bright\",\"params\":[1, \"smooth\", 500]}\r\n";
			}
			break;
		case Color_LedFull:
			message = "{\"id\":1,\"method\":\"set_bright\",\"params\":[100, \"smooth\", 500]}\r\n";
			break;
		case Color_BrightnessUp:
			message = "{\"id\":1,\"method\":\"set_adjust\",\"params\":[\"increase\", \"bright\"]}\r\n";
			break;
		case Color_BrightnessDown:
			message = "{\"id\":1,\"method\":\"set_adjust\",\"params\":[\"decrease\", \"bright\"]}\r\n";
			break;
		case Color_ColorTempUp:
			message = "{\"id\":1,\"method\":\"set_adjust\",\"params\":[\"increase\", \"ct\"]}\r\n";
			break;
		case Color_ColorTempDown:
			message = "{\"id\":1,\"method\":\"set_adjust\",\"params\":[\"decrease\", \"ct\"]}\r\n";
			break;
		case Color_SetColorToWhite:
			sendOnFirst = true;
			if (pLed->subtype == sTypeColor_RGB_W) {
				message = "{\"id\":1,\"method\":\"set_rgb\",\"params\":[16777215, \"smooth\", 500]}\r\n";
			}
			else {
				message = "{\"id\":1,\"method\":\"set_bright\",\"params\":[100, \"smooth\", 500]}\r\n";
			}
			break;
		case Color_SetBrightnessLevel:
			sendOnFirst = true;
			ss << "{\"id\":1,\"method\":\"set_bright\",\"params\":[" << int(pLed->value) << ", \"smooth\", 500]}\r\n";
			message = ss.str();
			break;
		case Color_SetColor: {
			sendOnFirst = true;
			if (pLed->color.mode == ColorModeWhite)
			{
				if (pLed->subtype == sTypeColor_RGB || pLed->subtype == sTypeColor_RGB_W || pLed->subtype == sTypeColor_RGB_CW_WW) {
					int w = 255; // Full white, scaled by separate brightness command
					int rgb = (w << 16) + (w << 8) + pLed->color.b;
					ss << "{\"id\":1,\"method\":\"set_rgb\",\"params\":[" << rgb << ", \"smooth\", 500]}\r\n";
					message = ss.str();
				}
				// For other bulb type, just send brightness
			}
			else if (pLed->color.mode == ColorModeTemp)
			{
				// Convert temperature to Kelvin 1700..6500
				int kelvin = (int(float((255 - pLed->color.t))*(6500.0f - 1700.0f) / 255.0f)) + 1700;
				ss << "{\"id\":1,\"method\":\"set_ct_abx\",\"params\":[" << kelvin << ", \"smooth\", 2000]}\r\n";
				message = ss.str();
			}
			else if (pLed->color.mode == ColorModeRGB)
			{
				int rgb = ((pLed->color.r) << 16) + ((pLed->color.g) << 8) + pLed->color.b;
				ss << "{\"id\":1,\"method\":\"set_rgb\",\"params\":[" << rgb << ", \"smooth\", 2000]}\r\n";
				message = ss.str();
			}
			else
			{
				_log.Log(LOG_STATUS, "YeeLight: SetRGBColour - Color mode %d is unhandled, if you have a suggestion for what it should do, please post on the Domoticz forum", pLed->color.mode);
			}
			// Send brigthness command
			ss.str("");
			ss << "{\"id\":1,\"method\":\"set_bright\",\"params\":[" << pLed->value << ", \"smooth\", 500]}\r\n";
			message2 = ss.str();
		}
							 break;
		case Color_SetBrightUp:
			message = "{\"id\":1,\"method\":\"set_adjust\",\"params\":[\"increase\", \"bright\"]}\r\n";
			break;
		case Color_SetBrightDown:
			message = "{\"id\":1,\"method\":\"set_adjust\",\"params\":[\"decrease\", \"bright\"]}\r\n";
			break;
		case Color_WarmWhiteIncrease:
			//message = "{\"id\":1,\"method\":\"set_adjust\",\"params\":[\"increase\", \"bright\"]}\r\n";
			message = "{\"id\":1,\"method\":\"set_ct_abx\",\"params\":[3500, \"smooth\", 500]}\r\n";
			break;
		case Color_CoolWhiteIncrease:
			//message = "{\"id\":1,\"method\":\"set_adjust\",\"params\":[\"decrease\", \"bright\"]}\r\n";
			message = "{\"id\":1,\"method\":\"set_ct_abx\",\"params\":[6000, \"smooth\", 500]}\r\n";
			break;
		case Color_NightMode:
			if (pLed->subtype == sTypeColor_RGB_W) {
				message = "{\"id\":1,\"method\":\"set_scene\", \"params\": [\"color\", 16750848, 1]}\r\n";
			}
			else {
				message = "{\"id\":1,\"method\":\"set_bright\",\"params\":[1, \"smooth\", 500]}\r\n";
			}
			break;
		case Color_FullBrightness: {
			sendOnFirst = true;
			ss << "{\"id\":1,\"method\":\"set_bright\",\"params\":[100, \"smooth\", 500]}\r\n";
			message = ss.str();
		}
								   break;
		case Color_DiscoMode:
			sendOnFirst = true;
			// simulate strobe effect - at time of writing, minimum timing allowed by Yeelight is 50ms
			_log.Log(LOG_STATUS, "Yeelight: Disco Mode - simulate strobe effect, if you have a suggestion for what it should do, please post on the Domoticz forum");
			message = "{\"id\":1,\"method\":\"start_cf\",\"params\":[ 50, 0, \"";
			message += "50, 2, 5000, 100, ";
			message += "50, 2, 5000, 1\"]}\r\n";
			break;
		case Color_DiscoSpeedFasterLong:
			_log.Log(LOG_STATUS, "Yeelight: Exclude Lamp - This command is unhandled, if you have a suggestion for what it should do, please post on the Domoticz forum");
			break;
		default:
			_log.Log(LOG_STATUS, "YeeLight: Unhandled WriteToHardware command: %d - if you have a suggestion for what it should do, please post on the Domoticz forum", command);
			break;
		}

		if (message.empty()) {
			return false;
		}

		if (sendOnFirst) {
			strcpy(request, "{\"id\":1,\"method\":\"set_power\",\"params\":[\"on\", \"smooth\", 500]}\r\n");
			request_length = strlen(request);
			_log.Debug(DEBUG_HARDWARE, "Yeelight: sending request '%s'", request);
			boost::asio::write(sendSocket, boost::asio::buffer(request, request_length));
			sleep_milliseconds(50);
		}

		strcpy(request, message.c_str());
		request_length = strlen(request);
		_log.Debug(DEBUG_HARDWARE, "Yeelight: sending request '%s'", request);
		boost::asio::write(sendSocket, boost::asio::buffer(request, request_length));
		sleep_milliseconds(50);

		if (!message2.empty())
		{
			strcpy(request, message2.c_str());
			request_length = strlen(request);
			_log.Debug(DEBUG_HARDWARE, "Yeelight: sending request '%s'", request);
			boost::asio::write(sendSocket, boost::asio::buffer(request, request_length));
			sleep_milliseconds(50);
		}

	}
	catch (const std::exception &e)
	{
		_log.Log(LOG_ERROR, "YeeLight: Exception: %s", e.what());
		return false;
	}

	return true;
}


boost::array<char, 1024> recv_buffer_;
int hardwareId;

Yeelight::udp_server::udp_server(boost::asio::io_service& io_service, int m_HwdID)
	: socket_(io_service, boost::asio::ip::udp::endpoint(boost::asio::ip::udp::v4(), 0))
{
	socket_.set_option(boost::asio::ip::udp::socket::reuse_address(true));
	socket_.set_option(boost::asio::socket_base::broadcast(true));
	hardwareId = m_HwdID;
}


void Yeelight::udp_server::start_send()
{
	try
	{
		std::string testMessage = "M-SEARCH * HTTP/1.1\r\nHOST: 239.255.255.250:1982\r\nMAN: \"ssdp:discover\"\r\nST: wifi_bulb";
		//_log.Log(LOG_STATUS, "start_send..................");
		std::shared_ptr<std::string> message(
			new std::string(testMessage));
		remote_endpoint_ = boost::asio::ip::udp::endpoint(boost::asio::ip::address::from_string("239.255.255.250"), 1982);
		socket_.send_to(boost::asio::buffer(*message), remote_endpoint_);
		sleep_milliseconds(150);
		start_receive();
	}
	catch (const std::exception &e)
	{
		_log.Log(LOG_ERROR, "YeeLight: Exception: %s", e.what());
	}
}

void Yeelight::udp_server::start_receive()
{
#ifdef DEBUG_YeeLightR
	std::string szData = ReadFile("E:\\YeeLight_receive.txt");
	HandleIncoming(szData);
	return;
#endif
	try
	{
		// only allow one response from each ip per run
		std::vector<std::string> receivedip;

		while (socket_.available() > 0) {
			socket_.receive_from(boost::asio::buffer(recv_buffer_), remote_endpoint_);
			HandleIncoming(recv_buffer_.data(), receivedip);
		}
	}
	catch (const std::exception &e)
	{
		_log.Log(LOG_ERROR, "YeeLight: Exception: %s", e.what());
	}
}

bool YeeLightGetTag(const std::string &InputString, const std::string &Tag, std::string &Value)
{
	std::size_t pos = InputString.find(Tag);
	if (pos == std::string::npos)
		return false; //not found
	std::string szValue = InputString.substr(pos + Tag.size());
	pos = szValue.find("\r\n");
	if (pos == std::string::npos)
		return false; //not found
	Value = szValue.substr(0, pos);
	return true;
}


bool Yeelight::udp_server::HandleIncoming(const std::string &szData, std::vector<std::string> &receivedip)
{
	std::string receivedString(szData);
	//_log.Log(LOG_STATUS, receivedString.c_str());
#ifdef DEBUG_YeeLightW
	SaveString2Disk(receivedString, "E:\\YeeLight_receive.txt");
#endif
	std::string startString = "Location: yeelight://";
	std::string endString = ":55443";

	std::size_t pos = receivedString.find(startString);
	if (pos == std::string::npos)
		return false; //not for us

	pos += startString.length();

	std::size_t pos1 = receivedString.substr(pos).find(endString);
	std::string dataString = receivedString.substr(pos, pos1);

	std::string yeelightLocation = dataString.c_str();
	// check if we have received this ip already
	size_t i;
	for (i = 0; i < receivedip.size(); i++) {
		if (std::strcmp(receivedip[i].c_str(), yeelightLocation.c_str()) == 0) {
			//_log.Log(LOG_STATUS, "Already received: %s", yeelightLocation.c_str());
			return false;
		}
	}
	receivedip.push_back(yeelightLocation);
	//_log.Log(LOG_STATUS, "Location: %s", yeelightLocation.c_str());
	std::string yeelightId;
	if (!YeeLightGetTag(szData, "id: ", yeelightId))
		return false;
	if (yeelightId.size() > 10)
		yeelightId = yeelightId.substr(10, yeelightId.length() - 10);

	std::string yeelightModel;
	if (!YeeLightGetTag(szData, "model: ", yeelightModel))
		return false;

	std::string yeelightSupport = "";
	if (!YeeLightGetTag(szData, "support: ", yeelightSupport))
		return false;

	std::string yeelightStatus;
	if (!YeeLightGetTag(szData, "power: ", yeelightStatus))
		return false;

	std::string yeelightBright;
	if (!YeeLightGetTag(szData, "bright: ", yeelightBright))
		return false;

	std::string yeelightColorMode;
	if (!YeeLightGetTag(szData, "color_mode: ", yeelightColorMode))
		return false;

	std::string yeelightHue;
	if (!YeeLightGetTag(szData, "hue: ", yeelightHue))
		return false;

	std::string yeelightSat;
	if (!YeeLightGetTag(szData, "sat: ", yeelightSat))
		return false;

	std::string yeelightRGB;
	if (!YeeLightGetTag(szData, "rgb: ", yeelightRGB))
		return false;

	std::string yeelightCT;
	if (!YeeLightGetTag(szData, "ct: ", yeelightCT))
		return false;

	bool bIsOn = false;
	if (yeelightStatus == "on") {
		bIsOn = true;
	}
	int sType = sTypeColor_White;

	std::vector<std::string> tmp;
	StringSplit(yeelightSupport, " ", tmp);
	std::set<std::string> support(tmp.begin(), tmp.end());

	if (support.find("set_ct_abx") != support.end())
	{
		sType = sTypeColor_CW_WW;
	}

	if (support.find("set_rgb") != support.end())
	{
		sType = sTypeColor_RGB;
	}

	if (support.find("set_rgb") != support.end() && support.find("set_ct_abx") != support.end())
	{
		sType = sTypeColor_RGB_CW_WW;
	}

	std::string yeelightName = "";
	if (yeelightModel == "mono") {
		yeelightName = "YeeLight LED (Mono)";
	}
	else if (yeelightModel == "color") {
		yeelightName = "YeeLight LED (Color)";
	}
	else if (yeelightModel == "stripe") {
		yeelightName = "YeeLight LED (Stripe)";
	}
	else if (yeelightModel == "ceiling") {
		yeelightName = "YeeLight LED (Ceiling)";
	}
	else if (yeelightModel == "bslamp") {
		yeelightName = "YeeLight LED (BSLamp)";
	}
	Yeelight yeelight(hardwareId);

	yeelight.InsertUpdateSwitch(yeelightId, yeelightName, sType, yeelightLocation, bIsOn, yeelightBright, yeelightHue, yeelightSat, yeelightRGB, yeelightCT, yeelightColorMode);
	return true;
}

//Webserver helpers
namespace http {
	namespace server {
		void CWebServer::Cmd_AddYeeLight(WebEmSession & session, const request& req, Json::Value &root)
		{
			root["title"] = "AddYeeLight";

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

			Yeelight yeelight(HwdID);
			//TODO: Add support for other bulb types to WebUI (WW, RGB, RGBWW)
			yeelight.InsertUpdateSwitch("123", sname, (stype == "0") ? sTypeColor_White : sTypeColor_RGB_W, sipaddress, false, "0", "0", "", "", "", "");
		}
	}
}
