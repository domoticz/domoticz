#include "stdafx.h"
#include "Yeelight.h"
#include "../main/Logger.h"
#include "../main/Helper.h"
#include "../main/SQLHelper.h"
#include "../main/localtime_r.h"
#include "../hardware/hardwaretypes.h"
#include "../main/mainworker.h"
#include "../main/WebServer.h"
#include "../hardware/ASyncTCP.h"
#include "../json/json.h"


std::vector<boost::shared_ptr<Yeelight::YeelightTCP> > m_pNodes;

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
	std::vector<std::vector<std::string> > result;
	result = m_sql.safe_query("SELECT Options, DeviceID FROM DeviceStatus WHERE (Used=1) AND (Type==%d)", pTypeYeelight);
	if (result.size() < 1) {
		//return false;
	}
	//int rowCnt = 0;
	std::vector<std::vector<std::string> >::const_iterator itt;
	for (itt = result.begin(); itt != result.end(); ++itt)
	{
		std::vector<std::string> sd = *itt;
		std::string ipAddress = sd[0].c_str();
		//_log.Log(LOG_STATUS, "Yeelight: found device in database %s %s", ipAddr.c_str(), sd[1].c_str());
		boost::shared_ptr<Yeelight::YeelightTCP>	pNode = (boost::shared_ptr<Yeelight::YeelightTCP>) new Yeelight::YeelightTCP(sd[1].c_str(), ipAddress.c_str(), m_HwdID);
		m_pNodes.push_back(pNode);
	}

	for (std::vector<boost::shared_ptr<Yeelight::YeelightTCP> >::iterator itt = m_pNodes.begin(); itt != m_pNodes.end(); ++itt)
	{
		//_log.Log(LOG_NORM, "Yeelight: (%s) Starting thread.", (*itt)->m_szDeviceId.c_str());
		(*itt)->Start();
	}

	sleep_seconds(5);
	boost::asio::io_service io_service;
	YeelightUDP yeelightUDP(io_service, m_HwdID);
	yeelightUDP.start_send();
	io_service.run();
	int sec_counter = 0;
	while (!m_stoprequested)
	{
		sleep_seconds(1);
		sec_counter++;
		if (sec_counter % 12 == 0) {
			m_LastHeartbeat = mytime(NULL);
		}
		if (sec_counter % 1800 == 0) //poll for new yeelights every 30 minutes 60 * 30
		{
			yeelightUDP.start_send();
			io_service.run();
		}
	}
	//_log.Log(LOG_STATUS, "Yeelight stopped");
}

void Yeelight::UpdateSwitch(const std::string &nodeID, const bool bIsOn, const std::string &yeelightBright, const std::string &yeelightHue)
{
	unsigned char unitcode = 1;
	int cmd = light1_sOn;
	int level = 100;
	if (!bIsOn) {
		cmd = light1_sOff;
		level = 0;
	}
	int value = atoi(yeelightBright.c_str());

	uint8_t unit = 0;
	uint8_t dType = pTypeYeelight;
	unsigned long ID1;
	std::stringstream s_strid1;
	s_strid1 << std::hex << nodeID.c_str();
	s_strid1 >> ID1;
	_tYeelight ycmd;
	ycmd.len = sizeof(_tYeelight) - 1;
	ycmd.type = dType;
	//ycmd.subtype = YeeType;
	ycmd.id = ID1;
	ycmd.dunit = unit;
	ycmd.value = value;
	ycmd.command = cmd;
	m_mainworker.PushAndWaitRxMessage(this, (const unsigned char *)&ycmd, NULL, -1);
	if (yeelightBright != "") {
		m_sql.safe_query("UPDATE DeviceStatus SET nValue=%d, sValue='', LastLevel=%d WHERE(HardwareID == %d) AND (DeviceID == '%q')", int(cmd), value, m_HwdID, nodeID.c_str());
	}
	else {
		m_sql.safe_query("UPDATE DeviceStatus SET nValue=%d WHERE(HardwareID == %d) AND (DeviceID == '%q')", int(cmd), m_HwdID, nodeID.c_str());
	}

	//m_sql.safe_query("UPDATE DeviceStatus SET SwitchType=%d, nValue=%d, sValue='', LastLevel=%d WHERE(HardwareID == %d) AND (DeviceID == '%q')", int(STYPE_Dimmer), int(cmd), value, m_HwdID, nodeID.c_str());
}

void Yeelight::InsertUpdateSwitch(const std::string &nodeID, const std::string &lightName, const int &YeeType, const std::string &ipAddress, const bool bIsOn, const std::string &yeelightBright, const std::string &yeelightHue)
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
			m_HwdID, nodeID.c_str(), (int)0, pTypeYeelight, int(STYPE_Dimmer), YeeType, lightName.c_str(), ipAddress.c_str());
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

		//check if the ip address has changed.
		result = m_sql.safe_query("SELECT Options FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='%q') AND (Type==%d) AND (SubType==%d) AND (Options=='%q')", m_HwdID, nodeID.c_str(), pTypeYeelight, YeeType, ipAddress.c_str());
		if (result.size() < 1) {
			_log.Log(LOG_STATUS, "Yeelight: IP Address has changed for %s ", nodeID.c_str());
			m_sql.safe_query("UPDATE DeviceStatus SET Options='%q'  WHERE(HardwareID == %d) AND (DeviceID == '%q')", ipAddress.c_str(), m_HwdID, nodeID.c_str());
			//remove and re-add..
			std::vector<boost::shared_ptr<Yeelight::YeelightTCP> >::iterator itt;
			for (itt = m_pNodes.begin(); itt != m_pNodes.end(); ++itt)
			{
				if ((*itt)->m_szDeviceId == nodeID.c_str())
				{
					(*itt)->Stop();
					m_pNodes.erase(itt);
					break;
				}
			}
			boost::shared_ptr<Yeelight::YeelightTCP> pNode = (boost::shared_ptr<Yeelight::YeelightTCP>) new Yeelight::YeelightTCP(nodeID.c_str(), ipAddress.c_str(), m_HwdID);
			m_pNodes.push_back(pNode);
			pNode->Start();
			_log.Log(LOG_STATUS, "Yeelight: restarted with new IP: %x", ipAddress.c_str());
		}
	}
}

bool Yeelight::WriteToHardware(const char *pdata, const unsigned char length)
{
	//_log.Log(LOG_STATUS, "Yeelight: WriteToHardware...............................");
	bool sendOnFirst = false;
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
	case Yeelight_LedNight:
		message = "{\"id\":1,\"method\":\"set_bright\",\"params\":[1, \"smooth\", 500]}\r\n";
		break;
	case Yeelight_LedFull:
		message = "{\"id\":1,\"method\":\"set_bright\",\"params\":[100, \"smooth\", 500]}\r\n";
		break;
	case Yeelight_BrightnessUp:  //unused?
		message = "{\"id\":1,\"method\":\"set_adjust\",\"params\":[\"increase\", \"bright\"]}\r\n";
		break;
	case Yeelight_BrightnessDown: //unused?
		message = "{\"id\":1,\"method\":\"set_adjust\",\"params\":[\"decrease\", \"bright\"]}\r\n";
		break;
	case Yeelight_ColorTempUp:
		message = "{\"id\":1,\"method\":\"set_adjust\",\"params\":[\"increase\", \"ct\"]}\r\n";
		break;
	case Yeelight_ColorTempDown:
		message = "{\"id\":1,\"method\":\"set_adjust\",\"params\":[\"decrease\", \"ct\"]}\r\n";
		break;
		//case Yeelight_RGBDiscoNext:
		//message = "{\"id\":1,\"method\":\"set_adjust\",\"params\":[\"increase\", \"ct\"]}\r\n";
		//break;
		//case Yeelight_RGBDiscoPrevious:
		//message = "{\"id\":1,\"method\":\"set_adjust\",\"params\":[\"increase\", \"ct\"]}\r\n";
		//break;
	case Yeelight_SetRGBColour: {
		sendOnFirst = true;
		float cHue = (359.0f / 255.0f)*float(pLed->value);//hue given was in range of 0-255
		std::stringstream ss;
		ss << "{\"id\":1,\"method\":\"set_hsv\",\"params\":[" << cHue << ", 100, \"smooth\", 2000]}\r\n";
		message = ss.str();
	}
								break;
								//case Yeelight_DiscoSpeedSlower:
								//message = "{\"id\":1,\"method\":\"set_adjust\",\"params\":[\"increase\", \"ct\"]}\r\n";
								//break;
								//case Yeelight_DiscoSpeedFaster:
								//message = "{\"id\":1,\"method\":\"set_adjust\",\"params\":[\"increase\", \"ct\"]}\r\n";
								//break;
	case Yeelight_DiscoMode: {
		sendOnFirst = true;
		//simulate strobe effect - at time of writing, minimum timing allowed by Yeelight is 50ms
		message = "{\"id\":1,\"method\":\"start_cf\",\"params\":[ 50, 0, \"";
		message += "50, 2, 5000, 100, ";
		message += "50, 2, 5000, 1\"]}\r\n";
	}
							 break;
	case Yeelight_SetColorToWhite:
		sendOnFirst = true;
		message = "{\"id\":1,\"method\":\"set_rgb\",\"params\":[16777215, \"smooth\", 500]}\r\n";
		break;
	case Yeelight_SetBrightnessLevel: {
		sendOnFirst = true;
		int value = pLed->value;
		std::stringstream ss;
		ss << "{\"id\":1,\"method\":\"set_bright\",\"params\":[" << value << ", \"smooth\", 500]}\r\n";
		message = ss.str();
	}
									  break;
	case Yeelight_SetBrightUp:
		message = "{\"id\":1,\"method\":\"set_adjust\",\"params\":[\"increase\", \"bright\"]}\r\n";
		break;
	case Yeelight_SetBrightDown:
		message = "{\"id\":1,\"method\":\"set_adjust\",\"params\":[\"decrease\", \"bright\"]}\r\n";
		break;
	case Yeelight_WarmWhiteIncrease:
		message = "{\"id\":1,\"method\":\"set_adjust\",\"params\":[\"increase\", \"bright\"]}\r\n";
		break;
	case Yeelight_CoolWhiteIncrease:
		message = "{\"id\":1,\"method\":\"set_adjust\",\"params\":[\"decrease\", \"bright\"]}\r\n";
		break;
	case Yeelight_NightMode:
		message = "{\"id\":1,\"method\":\"set_bright\",\"params\":[1, \"smooth\", 500]}\r\n";
		break;
	case Yeelight_FullBrightness: {
		sendOnFirst = true;
		int value = pLed->value;
		std::stringstream ss;
		ss << "{\"id\":1,\"method\":\"set_bright\",\"params\":[100, \"smooth\", 500]}\r\n";
		message = ss.str();
	}
								  //case Yeelight_DiscoSpeedFasterLong:
								  //message = "{\"id\":1,\"method\":\"set_bright\",\"params\":[1, \"smooth\", 500]}\r\n";
								  //break;
								  //case Yeelight_SetHEXColour:
								  //message = "{\"id\":1,\"method\":\"set_bright\",\"params\":[1, \"smooth\", 500]}\r\n";
								  //break;
	default:
		_log.Log(LOG_STATUS, "Yeelight: Unhandled WriteToHardware command: %d", command);
		break;
	}

	std::vector<boost::shared_ptr<Yeelight::YeelightTCP> >::iterator itt;
	for (itt = m_pNodes.begin(); itt != m_pNodes.end(); ++itt)
	{
		if ((*itt)->m_szDeviceId == ID)
		{
			if (sendOnFirst) {
				(*itt)->WriteInt("{\"id\":1,\"method\":\"set_power\",\"params\":[\"on\", \"smooth\", 500]}\r\n");
				sleep_milliseconds(100);
			}
			(*itt)->WriteInt(message.c_str());
			return true;
		}
	}
	sleep_milliseconds(100);
	return true;
}

void Yeelight::ParseData(const std::string & DeviceID, const char *pData, size_t len)
{
	//{"method":"props","params":{"hue":232}}
	//{"method":"props","params":{"bright":42}}
	//{"method":"props","params":{"power":"off"}}
	//_log.Log(LOG_STATUS, "Yeelight::ParseData DeviceID: %s", DeviceID.c_str());
	std::string yeelightStatus = "";
	std::string yeelightHue = "";
	std::string yeelightBright = "";
	std::string startString = "{\"power\":";
	std::string endString = "}}\r\n";
	std::string receivedString;
	receivedString.assign((char *)pData, len);
	_log.Log(LOG_STATUS, receivedString.c_str());
	int pos = receivedString.find(startString);
	int pos1;
	std::string dataString;

	if (pos > 0) {

		_log.Log(LOG_STATUS, "Yeelight: pos is: %i", pos);

		pos = pos + startString.length();
		pos1 = receivedString.substr(pos).find(endString);
		dataString = receivedString.substr(pos, pos1);
		yeelightStatus = dataString.c_str();
	}


	//check for hue
	startString = "{\"hue\":";
	pos = receivedString.find(startString);
	if (pos > 0) {
		_log.Log(LOG_STATUS, "Yeelight: pos is: %i", pos);
		pos = pos + startString.length();
		pos1 = receivedString.substr(pos).find(endString);
		dataString = receivedString.substr(pos, pos1);
		yeelightHue = dataString.c_str();
	}

	startString = "{\"bright\":";
	pos = receivedString.find(startString);
	if (pos > 0) {
		_log.Log(LOG_STATUS, "Yeelight: pos is: %i", pos);
		pos = pos + startString.length();
		pos1 = receivedString.substr(pos).find(endString);
		dataString = receivedString.substr(pos, pos1);
		yeelightHue = dataString.c_str();
	}


	_log.Log(LOG_STATUS, "Yeelight::ParseData yeelightStatus: %s", yeelightStatus.c_str());
	_log.Log(LOG_STATUS, "Yeelight::ParseData yeelightBright: %s", yeelightBright.c_str());
	_log.Log(LOG_STATUS, "Yeelight::ParseData yeelightHue: %s", yeelightHue.c_str());

	//std::string yeelightLocation;
	//std::string yeelightId;
	//std::string yeelightBright;
	//std::string yeelightHue;

	bool bIsOn = true;
	if (yeelightStatus == "\"off\"") {
		bIsOn = false;
	}

	UpdateSwitch(DeviceID, bIsOn, yeelightBright, yeelightHue);
	//_log.Log(LOG_STATUS, receivedString.c_str());
}


boost::array<char, 512> recv_buffer_;

Yeelight::YeelightUDP::YeelightUDP(boost::asio::io_service& io_service, int hardwareId)
	: socket_(io_service, boost::asio::ip::udp::endpoint(boost::asio::ip::udp::v4(), 1982))
{
	socket_.set_option(boost::asio::ip::udp::socket::reuse_address(true));
	socket_.set_option(boost::asio::socket_base::broadcast(true));
	m_HwdID = hardwareId;
}

void Yeelight::YeelightUDP::start_send()
{
	std::string testMessage = "M-SEARCH * HTTP/1.1\r\nHOST: 239.255.255.250:1982\r\nMAN: \"ssdp:discover\"\r\nST: wifi_bulb";
	//_log.Log(LOG_STATUS, "start_send..................");
	boost::shared_ptr<std::string> message(
		new std::string(testMessage));
	remote_endpoint_ = boost::asio::ip::udp::endpoint(boost::asio::ip::address::from_string("239.255.255.250"), 1982);
	socket_.send_to(boost::asio::buffer(*message), remote_endpoint_);
	start_receive();
}

void Yeelight::YeelightUDP::start_receive()
{
	sleep_milliseconds(100);
	if (socket_.available() > 0) {
		socket_.receive_from(boost::asio::buffer(recv_buffer_), remote_endpoint_);
		//_log.Log(LOG_STATUS, "start_receive: data received......");

		std::string startString = "Location: yeelight://";
		std::string endString = ":55443\r\n";

		std::string yeelightLocation;
		std::string yeelightId;
		std::string yeelightModel;
		std::string yeelightStatus;
		std::string yeelightBright;
		std::string yeelightHue;
		std::string receivedString = recv_buffer_.data();
		//_log.Log(LOG_STATUS, "START ---------------------------------");
		//_log.Log(LOG_STATUS, receivedString.c_str());
		//_log.Log(LOG_STATUS, "END -----------------------------------");
		int pos = receivedString.find(startString);
		if (pos > 0) {
			pos = pos + startString.length();
		}

		int pos1 = receivedString.substr(pos).find(endString);
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
		//_log.Log(LOG_STATUS, "start_receive: Location: %s %s", yeelightLocation.c_str(), yeelightStatus.c_str());

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
		Yeelight yeelight(m_HwdID);
		yeelight.InsertUpdateSwitch(yeelightId, yeelightName, sType, yeelightLocation, bIsOn, yeelightBright, yeelightHue);
		start_receive();
	}
	else {
		//_log.Log(LOG_STATUS, "no more data......");
	}
}


Yeelight::YeelightTCP::YeelightTCP(const std::string DeviceID, const std::string IPAddress, int hardwareId)
{
	m_szDeviceId = DeviceID;
	m_HwdID = hardwareId;
	m_szIPAddress = IPAddress;
	m_bDoRestart = false;
	m_stoprequested = false;
	m_usIPPort = 55443;
}

Yeelight::YeelightTCP::~YeelightTCP(void) {

}

bool Yeelight::YeelightTCP::Start()
{
	m_stoprequested = false;
	m_bDoRestart = false;
	//Start worker thread
	m_thread = boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&YeelightTCP::Do_Work, this)));
	return (m_thread != NULL);
}

bool Yeelight::YeelightTCP::Stop()
{
	m_stoprequested = true;
	if (isConnected())
	{
		try {
			disconnect();
			close();
		}
		catch (...)
		{
			//Don't throw from a Stop command
		}
	}
	try {
		if (m_thread)
		{
			m_thread->join();
			m_thread.reset();
		}
	}
	catch (...)
	{
		//Don't throw from a Stop command
	}

	//m_bIsStarted = false;
	return true;
}

bool Yeelight::YeelightTCP::WriteInt(const std::string & sendString)
{
	//_log.Log(LOG_STATUS, "YeelightTCP: WriteInt %s", sendString.c_str());
	if (!mIsConnected)
	{
		return false;
	}
	write(sendString);
	return true;
}

bool Yeelight::YeelightTCP::WriteInt(const uint8_t * pData, const size_t length)
{
	if (!mIsConnected)
	{
		return false;
	}
	write(pData, length);
	return true;
}

void Yeelight::YeelightTCP::Do_Work()
{
	bool bFirstTime = true;
	int sec_counter = 0;
	while (!m_stoprequested)
	{
		sleep_seconds(1);
		sec_counter++;
		if (bFirstTime)
		{
			bFirstTime = false;
			if (mIsConnected)
			{
				try {
					disconnect();
					close();
				}
				catch (...)
				{
					//Don't throw from a Stop command
				}
			}
			_log.Log(LOG_STATUS, "YeelightTCP: trying to connect to %s:%d", m_szIPAddress.c_str(), m_usIPPort);
			connect(m_szIPAddress, m_usIPPort);
		}
		else
		{
			if ((m_bDoRestart) && (sec_counter % 30 == 0))
			{
				_log.Log(LOG_STATUS, "YeelightTCP: trying to connect to %s:%d", m_szIPAddress.c_str(), m_usIPPort);
				try {
					disconnect();
					close();
				}
				catch (...)
				{
					//Don't throw from a Stop command
				}
				connect(m_szIPAddress, m_usIPPort);
			}
			update();
		}
	}
	_log.Log(LOG_STATUS, "YeelightTCP: TCP/IP Worker stopped...");
}

void Yeelight::YeelightTCP::OnConnect()
{
	_log.Log(LOG_STATUS, "YeelightTCP: connected to: %s:%ld", m_szIPAddress.c_str(), m_usIPPort);
	m_bDoRestart = false;
}

void Yeelight::YeelightTCP::OnDisconnect()
{
	_log.Log(LOG_STATUS, "YeelightTCP: disconnected");
	m_bDoRestart = true;
}

void Yeelight::YeelightTCP::OnData(const unsigned char * pData, size_t length)
{
	//boost::lock_guard<boost::mutex> l(readQueueMutex);
	Yeelight yeelight(m_HwdID);
	yeelight.ParseData(m_szDeviceId, (const char*)pData, length);
}

void Yeelight::YeelightTCP::OnError(const std::exception e)
{
	_log.Log(LOG_ERROR, "YeelightTCP: Error: %s", e.what());
}

void Yeelight::YeelightTCP::OnError(const boost::system::error_code & error)
{
	if (
		(error == boost::asio::error::address_in_use) ||
		(error == boost::asio::error::connection_refused) ||
		(error == boost::asio::error::access_denied) ||
		(error == boost::asio::error::host_unreachable) ||
		(error == boost::asio::error::timed_out)
		)
	{
		_log.Log(LOG_ERROR, "YeelightTCP: Cannot connect to: %s:%ld", m_szIPAddress.c_str(), m_usIPPort);
	}
	else if (
		(error == boost::asio::error::eof) ||
		(error == boost::asio::error::connection_reset)
		)
	{
		_log.Log(LOG_STATUS, "YeelightTCP: Connection reset!");
	}
	else
		_log.Log(LOG_ERROR, "YeelightTCP: %s", error.message().c_str());
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


