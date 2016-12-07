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
	return false;
}

void XiaomiGateway::InsertUpdateSwitch(const std::string &nodeid, const std::string &Name, bool bIsOn)
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

	_tGeneralSwitch xcmd;
	xcmd.len = sizeof(_tGeneralSwitch) - 1;
	xcmd.id = sID;
	xcmd.type = pTypeGeneralSwitch;
	xcmd.subtype = sSwitchGeneralSwitch; // STYPE_Motion; //0
	if (bIsOn) {
		xcmd.cmnd = gswitch_sOn;
	}
	else {
		xcmd.cmnd = gswitch_sOff;
	}
	m_mainworker.PushAndWaitRxMessage(this, (const unsigned char *)&xcmd, NULL, -1);
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
			//_log.Log(LOG_STATUS, cmd.c_str());

			std::string model = root["model"].asString();
			//_log.Log(LOG_STATUS, model.c_str());

			std::string sid = root["sid"].asString();
			//_log.Log(LOG_STATUS, sid.c_str());

			std::string data = root["data"].asString();
			//_log.Log(LOG_STATUS, data.c_str());

			if (model == "motion") {
				//get the status
				Json::Value root2;
				ret = jReader.parse(data.c_str(), root2);
				if (ret) {
					std::string status = root2["status"].asString();
					//_log.Log(LOG_STATUS, status.c_str());
					if (status == "motion") {
						_log.Log(LOG_STATUS, "Xiaomi motion detected from %s", sid.c_str());
						//update domoticz
						XiaomiGateway xiaomiGateway(m_HardwareID);
						xiaomiGateway.InsertUpdateSwitch(sid.c_str(), "name", true);
					}
					else if (status == "no_motion") {
						_log.Log(LOG_STATUS, "Xiaomi NO motion detected from %s", sid.c_str());
						//update domoticz
						XiaomiGateway xiaomiGateway(m_HardwareID);
						xiaomiGateway.InsertUpdateSwitch(sid.c_str(), "name", false);
					}
					//}
					//else {
					//	_log.Log(LOG_STATUS, data.c_str());
				}
			}
		}
		start_receive();
	}
	else {
		_log.Log(LOG_STATUS, "error in handle_receive");
	}
}
