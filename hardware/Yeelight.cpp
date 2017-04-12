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
	m_stoprequested = false;
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

#define Limitless_POLL_INTERVAL 60

void Yeelight::Do_Work()
{
	_log.Log(LOG_STATUS, "YeeLight Worker started...");

	boost::asio::io_service io_service;
	udp_server server(io_service, m_HwdID);
	int sec_counter = Limitless_POLL_INTERVAL - 5;
	while (!m_stoprequested)
	{
		sleep_seconds(1);
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
	_log.Log(LOG_STATUS, "YeeLight stopped");
}


void Yeelight::InsertUpdateSwitch(const std::string &nodeID, const std::string &lightName, const int &YeeType, const std::string &Location, const bool bIsOn, const std::string &yeelightBright, const std::string &yeelightHue)
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

	int lastLevel = 0;
	int nvalue = 0;
	bool tIsOn = !(bIsOn);
	std::vector<std::vector<std::string> > result;
	result = m_sql.safe_query("SELECT nValue, LastLevel FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='%q') AND (Type==%d) AND (SubType==%d)", m_HwdID, szDeviceID, pTypeLimitlessLights, YeeType);
	if (result.size() < 1)
	{
		_log.Log(LOG_STATUS, "YeeLight: New Light Found (%s/%s)", Location.c_str(), lightName.c_str());
		int value = atoi(yeelightBright.c_str());
		int cmd = light1_sOn;
		int level = 100;
		if (!bIsOn) {
			cmd = light1_sOff;
			level = 0;
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

		nvalue = atoi(result[0][0].c_str());
		tIsOn = (nvalue != 0);
		lastLevel = atoi(result[0][1].c_str());
		int value = atoi(yeelightBright.c_str());
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


bool Yeelight::WriteToHardware(const char *pdata, const unsigned char length)
{
	//_log.Log(LOG_STATUS, "YeeLight: WriteToHardware...............................");
	_tLimitlessLights *pLed = (_tLimitlessLights*)pdata;
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

	boost::asio::io_service io_service;
	boost::asio::ip::tcp::socket sendSocket(io_service);
	boost::asio::ip::tcp::resolver resolver(io_service);
	boost::asio::ip::tcp::resolver::query query(boost::asio::ip::tcp::v4(), szTmp, "55443");
	boost::asio::ip::tcp::resolver::iterator iterator = resolver.resolve(query);
	try
	{
		boost::asio::connect(sendSocket, iterator);
	}
	catch (const std::exception &e)
	{
		_log.Log(LOG_ERROR, "YeeLight: Exception: %s", e.what());
		return false;
	}

	std::string message = "";
	char request[1024];
	size_t request_length;
	std::stringstream ss;

	switch (pLed->command)
	{
	case Limitless_LedOn:
		message = "{\"id\":1,\"method\":\"set_power\",\"params\":[\"on\", \"smooth\", 500]}\r\n";
		break;
	case Limitless_LedOff:
		message = "{\"id\":1,\"method\":\"set_power\",\"params\":[\"off\", \"smooth\", 500]}\r\n";
		break;
	case Limitless_LedNight:
		if (pLed->subtype == sTypeLimitlessRGBW) {
			message = "{\"id\":1,\"method\":\"set_scene\", \"params\": [\"color\", 16750848, 1]}\r\n";
		}
		else {
			message = "{\"id\":1,\"method\":\"set_bright\",\"params\":[1, \"smooth\", 500]}\r\n";
		}
		break;
	case Limitless_LedFull:
		message = "{\"id\":1,\"method\":\"set_bright\",\"params\":[100, \"smooth\", 500]}\r\n";
		break;
	case Limitless_BrightnessUp:
		message = "{\"id\":1,\"method\":\"set_adjust\",\"params\":[\"increase\", \"bright\"]}\r\n";
		break;
	case Limitless_BrightnessDown:
		message = "{\"id\":1,\"method\":\"set_adjust\",\"params\":[\"decrease\", \"bright\"]}\r\n";
		break;
	case Limitless_ColorTempUp:
		message = "{\"id\":1,\"method\":\"set_adjust\",\"params\":[\"increase\", \"ct\"]}\r\n";
		break;
	case Limitless_ColorTempDown:
		message = "{\"id\":1,\"method\":\"set_adjust\",\"params\":[\"decrease\", \"ct\"]}\r\n";
		break;
	case Limitless_SetColorToWhite:
		sendOnFirst = true;
		if (pLed->subtype == sTypeLimitlessRGBW) {
			message = "{\"id\":1,\"method\":\"set_rgb\",\"params\":[16777215, \"smooth\", 500]}\r\n";
		}
		else {
			message = "{\"id\":1,\"method\":\"set_bright\",\"params\":[100, \"smooth\", 500]}\r\n";
		}
		break;
	case Limitless_SetBrightnessLevel:
		sendOnFirst = true;
		ss << "{\"id\":1,\"method\":\"set_bright\",\"params\":[" << int(pLed->value) << ", \"smooth\", 500]}\r\n";
		message = ss.str();
		break;
	case Limitless_SetRGBColour: {
			sendOnFirst = true;
			float cHue = (359.0f / 255.0f)*float(pLed->value); // hue given was in range of 0-255
			ss << "{\"id\":1,\"method\":\"set_hsv\",\"params\":[" << cHue << ", 100, \"smooth\", 2000]}\r\n";
			message = ss.str();
		}
		break;
	case Limitless_SetBrightUp:
		message = "{\"id\":1,\"method\":\"set_adjust\",\"params\":[\"increase\", \"bright\"]}\r\n";
		break;
	case Limitless_SetBrightDown:
		message = "{\"id\":1,\"method\":\"set_adjust\",\"params\":[\"decrease\", \"bright\"]}\r\n";
		break;
	case Limitless_WarmWhiteIncrease:
		//message = "{\"id\":1,\"method\":\"set_adjust\",\"params\":[\"increase\", \"bright\"]}\r\n";
		message = "{\"id\":1,\"method\":\"set_ct_abx\",\"params\":[3500, \"smooth\", 500]}\r\n";
		break;
	case Limitless_CoolWhiteIncrease:
		//message = "{\"id\":1,\"method\":\"set_adjust\",\"params\":[\"decrease\", \"bright\"]}\r\n";
		message = "{\"id\":1,\"method\":\"set_ct_abx\",\"params\":[6000, \"smooth\", 500]}\r\n";
		break;
	case Limitless_NightMode:
		if (pLed->subtype == sTypeLimitlessRGBW) {
			message = "{\"id\":1,\"method\":\"set_scene\", \"params\": [\"color\", 16750848, 1]}\r\n";
		}
		else {
			message = "{\"id\":1,\"method\":\"set_bright\",\"params\":[1, \"smooth\", 500]}\r\n";
		}
		break;
	case Limitless_FullBrightness: {
			sendOnFirst = true;
			int value = pLed->value;
			ss << "{\"id\":1,\"method\":\"set_bright\",\"params\":[100, \"smooth\", 500]}\r\n";
			message = ss.str();
		}
		break;
	case Limitless_DiscoMode:
		sendOnFirst = true;
		// simulate strobe effect - at time of writing, minimum timing allowed by Yeelight is 50ms
		_log.Log(LOG_STATUS, "Yeelight: Disco Mode - simulate strobe effect, if you have a suggestion for what it should do, please post on the Domoticz forum");
		message = "{\"id\":1,\"method\":\"start_cf\",\"params\":[ 50, 0, \"";
		message += "50, 2, 5000, 100, ";
		message += "50, 2, 5000, 1\"]}\r\n";
		break;
	case Limitless_DiscoSpeedFasterLong:
		_log.Log(LOG_STATUS, "Yeelight: Exclude Lamp - This command is unhandled, if you have a suggestion for what it should do, please post on the Domoticz forum");
		break;
	default:
		_log.Log(LOG_STATUS, "YeeLight: Unhandled WriteToHardware command: %d - if you have a suggestion for what it should do, please post on the Domoticz forum", command);
		break;
	}

	if (message == "") {
		return false;
	}

	if (sendOnFirst) {
		strcpy(request, "{\"id\":1,\"method\":\"set_power\",\"params\":[\"on\", \"smooth\", 500]}\r\n");
		request_length = strlen(request);
		boost::asio::write(sendSocket, boost::asio::buffer(request, request_length));
		sleep_milliseconds(50);
	}

	strcpy(request, message.c_str());
	request_length = strlen(request);
	boost::asio::write(sendSocket, boost::asio::buffer(request, request_length));
	sleep_milliseconds(50);
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
	std::string testMessage = "M-SEARCH * HTTP/1.1\r\nHOST: 239.255.255.250:1982\r\nMAN: \"ssdp:discover\"\r\nST: wifi_bulb";
	//_log.Log(LOG_STATUS, "start_send..................");
	boost::shared_ptr<std::string> message(
		new std::string(testMessage));
	remote_endpoint_ = boost::asio::ip::udp::endpoint(boost::asio::ip::address::from_string("239.255.255.250"), 1982);
	socket_.send_to(boost::asio::buffer(*message), remote_endpoint_);
	sleep_milliseconds(150);
	start_receive();
}

void Yeelight::udp_server::start_receive()
{
#ifdef DEBUG_YeeLightR
	std::string szData = ReadFile("E:\\YeeLight_receive.txt");
	HandleIncoming(szData);
#endif
	// only allow one response from each ip per run
	std::vector<std::string> receivedip;

	while (socket_.available() > 0) {
		socket_.receive_from(boost::asio::buffer(recv_buffer_), remote_endpoint_);
		HandleIncoming(recv_buffer_.data(), receivedip);
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

	std::string yeelightStatus;
	if (!YeeLightGetTag(szData, "power: ", yeelightStatus))
		return false;

	std::string yeelightBright;
	if (!YeeLightGetTag(szData, "bright: ", yeelightBright))
		return false;

	std::string yeelightHue;
	if (!YeeLightGetTag(szData, "hue: ", yeelightHue))
		return false;

	bool bIsOn = false;
	if (yeelightStatus == "on") {
		bIsOn = true;
	}
	int sType = sTypeLimitlessWhite;

	std::string yeelightName = "";
	if (yeelightModel == "mono") {
		yeelightName = "YeeLight LED (Mono)";
	}
	else if ((yeelightModel == "color") || (yeelightModel == "stripe")) {
		yeelightName = "YeeLight LED (Color)";
		sType = sTypeLimitlessRGBW;
	}
	Yeelight yeelight(hardwareId);
	yeelight.InsertUpdateSwitch(yeelightId, yeelightName, sType, yeelightLocation, bIsOn, yeelightBright, yeelightHue);
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
			yeelight.InsertUpdateSwitch("123", sname, (stype == "0") ? sTypeLimitlessWhite : sTypeLimitlessRGBW, sipaddress, false, "0", "0");
		}
	}
}
