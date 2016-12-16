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
	//no need to send
	return true;
}

void XiaomiGateway::InsertUpdateSwitch(const std::string &nodeid, const std::string &Name, bool bIsOn, _eSwitchType type)
{
	// Make sure the ID supplied fits with what is expected ie 158d0000fd32c2
	if (nodeid.length() < 14) {
		return;
	}
	std::string str = nodeid.substr(8, 6);
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
	xcmd.subtype = sSwitchGeneralSwitch;
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
		m_sql.safe_query("UPDATE DeviceStatus SET Name='%q', SwitchType=%d WHERE(HardwareID == %d) AND (DeviceID == '%q')", Name.c_str(), (type), m_HwdID, ID.c_str());
	}
	else {
		//already in the database
		if (type == STYPE_PushOn) {
			//just toggle the last state.
			int nvalue = atoi(result[0][0].c_str());
			//_log.Log(LOG_STATUS, "XiaomiGateway: nvalue (%d)", nvalue);
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

bool XiaomiGateway::StartHardware()
{
	m_stoprequested = false;
	m_bDoRestart = false;

	//force connect the next first time
	m_bIsStarted = true;

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
	_log.Log(LOG_STATUS, "XiaomiGateway Worker started...");
	boost::asio::io_service io_service;
	XiaomiGateway::xiaomi_udp_server udp_server(io_service, m_HwdID);
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
	_log.Log(LOG_STATUS, "XiaomiGateway stopped");
}

XiaomiGateway::xiaomi_udp_server::xiaomi_udp_server(boost::asio::io_service& io_service, int m_HwdID)
	: socket_(io_service, boost::asio::ip::udp::endpoint(boost::asio::ip::udp::v4(), 9898))
{
	socket_.set_option(boost::asio::ip::udp::socket::reuse_address(true));
	socket_.set_option(boost::asio::ip::multicast::join_group(boost::asio::ip::address::from_string("224.0.0.50")));
	m_HardwareID = m_HwdID;
	start_receive();
}

void XiaomiGateway::xiaomi_udp_server::start_receive()
{
	//_log.Log(LOG_STATUS, "start_receive");
	socket_.async_receive_from(boost::asio::buffer(data_, max_length), remote_endpoint_, boost::bind(&xiaomi_udp_server::handle_receive, this, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
}

void XiaomiGateway::xiaomi_udp_server::handle_receive(const boost::system::error_code & error, std::size_t bytes_recvd)
{
	//_log.Log(LOG_STATUS, "handle_receive starting");
	if (!error || error == boost::asio::error::message_size)
	{
		Json::Value root;
		Json::Reader jReader;

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
			if (cmd == "report") {
				//_log.Log(LOG_STATUS, "XiaomiGateway report for %s", model.c_str());
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
						type = STYPE_Contact; //need smart plug type
						name = "Xiaomi Smart Plug";
					}
					if (type != STYPE_END) {
						std::string status = root2["status"].asString();
						bool on = false;
						if ((status == "motion") || (status == "open")) {
							on = true;
						}
						else if ((status == "no_motion") || (status == "close")) {
							on = false;
						}
						//else if ((status == "click")) { // double_click long_click_press long_click_release
							//on = true;
						//}
						XiaomiGateway xiaomiGateway(m_HardwareID);
						xiaomiGateway.InsertUpdateSwitch(sid.c_str(), name, on, type);
					}
					else {
						_log.Log(LOG_STATUS, "XiaomiGateway unhandled model: %s", model.c_str());
					}
				}
			}
		}
		start_receive();
	}
	else {
		_log.Log(LOG_STATUS, "error in handle_receive");
	}
}
