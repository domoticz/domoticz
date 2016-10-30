#include "stdafx.h"
#include "Yeelight.h"
#include "../main/Logger.h"
#include "../main/Helper.h"
#include "../main/SQLHelper.h"
#include "../main/localtime_r.h"
#include "../hardware/hardwaretypes.h"
#include "../main/mainworker.h"
#include "../main/WebServer.h"
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

void Yeelight::Do_Work()
{
	boost::asio::io_service io_service;
	udp_server server(io_service, m_HwdID);
	//_log.Log(LOG_STATUS, "Starting Yeelight");
	int sec_counter = 0;
	while (!m_stoprequested)
	{
		sleep_seconds(1);
		sec_counter++;
		if (sec_counter % 12 == 0) {
			m_LastHeartbeat = mytime(NULL);
		}
		if (sec_counter % 60 == 0) //poll yeelights every minute
		{
			server.start_send();
			io_service.run();
		}
	}
	//_log.Log(LOG_STATUS, "Yeelight stopped");
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

			_tYeelight ycmd;
			ycmd.len = sizeof(_tYeelight) - 1;
			ycmd.type = dType;
			ycmd.subtype = YeeType;
			ycmd.id = ID1;
			ycmd.dunit = unit;
			ycmd.value = value;
			ycmd.command = cmd;
			m_mainworker.PushAndWaitRxMessage(this, (const unsigned char *)&ycmd, NULL, -1);
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

	boost::asio::io_service io_service;
	boost::asio::ip::tcp::socket sendSocket(io_service);
	boost::asio::ip::tcp::resolver resolver(io_service);
	boost::asio::ip::tcp::resolver::query query(boost::asio::ip::tcp::v4(), ipAddress.c_str(), "55443");
	boost::asio::ip::tcp::resolver::iterator iterator = resolver.resolve(query);
	boost::asio::connect(sendSocket, iterator);

	std::string message = "{\"id\":1,\"method\":\"toggle\",\"params\":[]}\r\n";

	char request[1024];
	size_t request_length;

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
		strcpy(request, message.c_str());
		request_length = strlen(request);
		boost::asio::write(sendSocket, boost::asio::buffer(request, request_length));
		sleep_milliseconds(200);
		message = "{\"id\":1,\"method\":\"set_rgb\",\"params\":[16777215, \"smooth\", 500]}\r\n";
		break;
	case Yeelight_SetBrightnessLevel: {
		int value = pLed->value;
		message = "{\"id\":1,\"method\":\"set_power\",\"params\":[\"on\", \"smooth\", 500]}\r\n";
		strcpy(request, message.c_str());
		request_length = strlen(request);
		boost::asio::write(sendSocket, boost::asio::buffer(request, request_length));
		sleep_milliseconds(200);
		std::stringstream ss;
		ss << "{\"id\":1,\"method\":\"set_bright\",\"params\":[" << value << ", \"smooth\", 500]}\r\n";
		message = ss.str();
	}
									  break;
	case Yeelight_SetRGBColour: {
		message = "{\"id\":1,\"method\":\"set_power\",\"params\":[\"on\", \"smooth\", 500]}\r\n";
		strcpy(request, message.c_str());
		request_length = strlen(request);
		boost::asio::write(sendSocket, boost::asio::buffer(request, request_length));
		sleep_milliseconds(200);
		float cHue = (359.0f / 255.0f)*float(pLed->value);//hue given was in range of 0-255
														  //message = "{\"id\":1,\"method\":\"set_hsv\",\"params\":[" + std::to_string(cHue) + ", 100, \"smooth\", 2000]}\r\n";
		std::stringstream ss;
		ss << "{\"id\":1,\"method\":\"set_hsv\",\"params\":[" << cHue << ", 100, \"smooth\", 2000]}\r\n";
		message = ss.str();
	}
								break;
	default:
		_log.Log(LOG_STATUS, "Yeelight: Unhandled WriteToHardware command: %d", command);
		break;
	}


	strcpy(request, message.c_str());
	request_length = strlen(request);
	boost::asio::write(sendSocket, boost::asio::buffer(request, request_length));

	//_log.Log(LOG_STATUS, "message is sent..................");
	sleep_milliseconds(200);

	return true;
}


boost::array<char, 4096> recv_buffer_;
int hardwareId;

Yeelight::udp_server::udp_server(boost::asio::io_service& io_service, int m_HwdID)
	: socket_(io_service, boost::asio::ip::udp::endpoint(boost::asio::ip::udp::v4(), 1982))
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
	if (socket_.available() > 0) {
		socket_.receive_from(boost::asio::buffer(recv_buffer_), remote_endpoint_);
		//_log.Log(LOG_STATUS, "data received......");

		std::string startString = "Location: yeelight://";
		std::string endString = ":55443\r\n";

		std::string yeelightLocation;
		std::string yeelightId;
		std::string yeelightModel;
		std::string yeelightStatus;
		std::string yeelightBright;
		std::string yeelightHue;
		std::string receivedString = recv_buffer_.data();
		//_log.Log(LOG_STATUS, receivedString.c_str());

		std::size_t pos = receivedString.find(startString);
		if (pos > 0) {
			pos = pos + startString.length();
		}

		std::size_t pos1 = receivedString.substr(pos).find(endString);
		std::string dataString = receivedString.substr(pos, pos1);
		yeelightLocation = dataString.c_str();

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
		//_log.Log(LOG_STATUS, "DiscoverLights: Location: %s %s", yeelightLocation.c_str(), yeelightStatus.c_str());

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
		int sType = sTypeYeelightWhite;

		std::string yeelightName = "";
		if (yeelightModel == "model: mono") {
			yeelightName = "Yeelight LED (Mono)";
		}
		else if (yeelightModel == "model: color") {
			yeelightName = "Yeelight LED (Color)";
			sType = sTypeYeelightColor;
		}
		Yeelight yeelight(hardwareId);
		yeelight.InsertUpdateSwitch(yeelightId, yeelightName, sType, yeelightLocation, bIsOn, yeelightBright, yeelightHue);
		start_receive();
	}
	else {
		//_log.Log(LOG_STATUS, "no more data......");
	}
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


