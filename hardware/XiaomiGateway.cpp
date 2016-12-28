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

/*
Xiaomi (Aqara) makes a smart home gateway/hub that has support
for a variety of Xiaomi sensors.
They can be purchased on AliExpress or other stores at very
competitive prices.
Protocol is Zigbee and WiFi, and the gateway and
Domoticz need to be in the same network/subnet
*/

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
	_tGeneralSwitch *xcmd = (_tGeneralSwitch*)pdata;
	char szTmp[50];
	sprintf(szTmp, "%08X", (unsigned int)xcmd->id);
	std::string ID = szTmp;
	_log.Log(LOG_STATUS, ID.c_str());
	std::stringstream s_strid;
	s_strid << std::hex << ID;

	std::string sid = s_strid.str();
	std::transform(sid.begin(), sid.end(), sid.begin(), ::tolower);
	//append 158d00 to the front
	sid.insert(0, "158d00");

	std::string command = "on";
	switch (xcmd->cmnd) {
	case gswitch_sOff:
		command = "off";
		break;
	case gswitch_sOn:
		command = "on";
		break;
	default:
		_log.Log(LOG_STATUS, "Unknown command %d", xcmd->cmnd);
		break;
	}
	std::string gatewaykey = GetGatewayKey();
	std::string message = "{\"cmd\":\"write\",\"model\":\"plug\",\"sid\":\"" + sid + "\",\"short_id\":9844,\"data\":\"{\\\"channel_0\\\":\\\"" + command + "\\\",\\\"key\\\":\\\"" + gatewaykey + "\\\"}\" }";

	boost::asio::io_service io_service;
	boost::asio::ip::udp::socket socket_(io_service, boost::asio::ip::udp::endpoint(boost::asio::ip::udp::v4(), 0));
	boost::shared_ptr<std::string> message1(new std::string(message));
	boost::asio::ip::udp::endpoint remote_endpoint_;
	remote_endpoint_ = boost::asio::ip::udp::endpoint(boost::asio::ip::address::from_string(m_GatewayIp), 9898);
	socket_.send_to(boost::asio::buffer(*message1), remote_endpoint_);
	sleep_milliseconds(150);
	boost::array<char, 512> recv_buffer_;
	memset(&recv_buffer_[0], 0, sizeof(recv_buffer_));
	while (socket_.available() > 0) {
		socket_.receive_from(boost::asio::buffer(recv_buffer_), remote_endpoint_);
		std::string receivedString(recv_buffer_.data());
		_log.Log(LOG_STATUS, "XiaomiGateway: response %s", receivedString.c_str());
	}
	return true;
}

void XiaomiGateway::InsertUpdateTemperature(const std::string &nodeid, const std::string &Name, const float Temperature)
{
	// Make sure the ID supplied fits with what is expected ie 158d0000fd32c2
	if (nodeid.length() < 14) {
		_log.Log(LOG_ERROR, "XiaomiGateway: Node ID %s is too short", nodeid.c_str());
		return;
	}
	std::string str = nodeid.substr(6, 8);
	_log.Log(LOG_STATUS, "XiaomiGateway: Temperature - nodeid: %s", nodeid.c_str());
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
	_log.Log(LOG_STATUS, "XiaomiGateway: Humidity - nodeid: %s", nodeid.c_str());
	unsigned int sID;
	std::stringstream ss;
	ss << std::hex << str.c_str();
	ss >> sID;

	SendHumiditySensor(sID, 255, Humidity, Name.c_str());
}

void XiaomiGateway::InsertUpdateSwitch(const std::string &nodeid, const std::string &Name, bool bIsOn, _eSwitchType subtype)
{
	// Make sure the ID supplied fits with what is expected ie 158d0000fd32c2
	if (nodeid.length() < 14) {
		_log.Log(LOG_ERROR, "XiaomiGateway: Node ID %s is too short", nodeid.c_str());
		return;
	}
	std::string str = nodeid.substr(6, 8);

	//_log.Log(LOG_STATUS, "nodeid: %s", nodeid.c_str());
	unsigned int sID;
	std::stringstream ss;
	ss << std::hex << str.c_str();
	ss >> sID;
	//_log.Log(LOG_STATUS, "sID: %d", sID);
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
	xcmd.subtype = sSwitchCustomSwitch;

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
		if (subtype == STYPE_OnOff) {
			customimage = 1;
		}
		m_sql.safe_query("UPDATE DeviceStatus SET Name='%q', SwitchType=%d, CustomImage=%i WHERE(HardwareID == %d) AND (DeviceID == '%q')", Name.c_str(), (subtype), customimage, m_HwdID, ID.c_str());
	}
	else {
		//already in the database
		if (subtype == STYPE_PushOn) {
			//just toggle the last state for a wireless switch.
			int nvalue = atoi(result[0][0].c_str());
			bIsOn = (nvalue == 0);
			if (bIsOn) {
				xcmd.cmnd = gswitch_sOn;
			}
			else {
				xcmd.cmnd = gswitch_sOff;
			}
		}
		m_mainworker.PushAndWaitRxMessage(this, (const unsigned char *)&xcmd, NULL, -1);
	}
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

	EVP_cleanup();
	ERR_free_strings();
	return gatewaykey;
}

XiaomiGateway::xiaomi_udp_server::xiaomi_udp_server(boost::asio::io_service& io_service, int m_HwdID, XiaomiGateway *parent)
	: socket_(io_service, boost::asio::ip::udp::endpoint(boost::asio::ip::udp::v4(), 9898))
{
	m_gatewayip = "127.0.0.1";

	boost::shared_ptr<std::string> message(new std::string("{\"cmd\":\"whois\"}"));
	boost::asio::ip::udp::endpoint remote_endpoint;
	remote_endpoint = boost::asio::ip::udp::endpoint(boost::asio::ip::address::from_string("224.0.0.50"), 4321);
	socket_.send_to(boost::asio::buffer(*message), remote_endpoint);

	socket_.set_option(boost::asio::ip::udp::socket::reuse_address(true));
	socket_.set_option(boost::asio::ip::multicast::join_group(boost::asio::ip::address::from_string("224.0.0.50")));

	m_HardwareID = m_HwdID;
	m_XiaomiGateway = parent;

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
	//_log.Log(LOG_STATUS, "handle_receive starting:");
	if (!error || error == boost::asio::error::message_size)
	{
		//_log.Log(LOG_STATUS, data_);
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
						type = STYPE_PushOn;
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
					if (type != STYPE_END) {
						std::string status = root2["status"].asString();
						bool on = false;
						if ((status == "motion") || (status == "open") || (status == "no_close") || (status == "on")) {
							on = true;
						}
						else if ((status == "no_motion") || (status == "close") || (status == "off")) {
							on = false;
						}
						std::string battery = root2["battery"].asString();
						if (battery != "") {
							m_XiaomiGateway->InsertUpdateVoltage(sid.c_str(), name, atoi(battery.c_str()));
						}
						else {
							m_XiaomiGateway->InsertUpdateSwitch(sid.c_str(), name, on, type);
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
						else if (humidity != "") {
							int hum = atoi(humidity.c_str());
							hum = hum / 100;
							m_XiaomiGateway->InsertUpdateHumidity(sid.c_str(), "Xiaomi Humidity", hum);
						}
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
					for (int i = 0; i < root2.size(); i++) {
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
				m_XiaomiGateway->UpdateToken(root["token"].asString());
				m_gatewayip = root["ip"].asString();
				showmessage = false;
			}
			else {
				//unknown
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
