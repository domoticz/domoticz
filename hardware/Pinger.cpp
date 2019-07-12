#include "stdafx.h"
#include "Pinger.h"
#include "../main/Helper.h"
#include "../main/HTMLSanitizer.h"
#include "../main/Logger.h"
#include "../main/SQLHelper.h"
#include "../main/RFXtrx.h"
#include "../main/localtime_r.h"
#include "../main/Noncopyable.h"
#include "../main/WebServer.h"
#include "../main/mainworker.h"
#include "../webserver/cWebem.h"
#include "../json/json.h"

#include <boost/asio.hpp>

#include "pinger/icmp_header.h"
#include "pinger/ipv4_header.h"

#include <iostream>

#if BOOST_VERSION >= 107000
#define GET_IO_SERVICE(s) ((boost::asio::io_context&)(s).get_executor().context())
#else
#define GET_IO_SERVICE(s) ((s).get_io_service())
#endif

class pinger
	: private domoticz::noncopyable
{
public:
	pinger(boost::asio::io_service& io_service, const char* destination, const int iPingTimeoutms)
		: resolver_(io_service), socket_(io_service, boost::asio::ip::icmp::v4()),
		timer_(io_service), sequence_number_(0), m_PingState(false), num_replies_(0)
	{
		boost::asio::ip::icmp::resolver::query query(boost::asio::ip::icmp::v4(), destination, "");
		destination_ = *resolver_.resolve(query);

		num_tries_ = 1;
		PingTimeoutms_ = iPingTimeoutms;
		start_send();
		start_receive();
	}
	int num_replies_;
	int num_tries_;
	int PingTimeoutms_;
	bool m_PingState;
private:
	void start_send()
	{
		std::string body("Domoticz");

		// Create an ICMP header for an echo request.
		icmp_header echo_request;
		echo_request.type(icmp_header::echo_request);
		echo_request.code(0);
		echo_request.identifier(get_identifier());
		echo_request.sequence_number(++sequence_number_);
		compute_checksum(echo_request, body.begin(), body.end());

		// Encode the request packet.
		boost::asio::streambuf request_buffer;
		std::ostream os(&request_buffer);
		os << echo_request << body;

		// Send the request.
		time_sent_ = boost::posix_time::microsec_clock::universal_time();
		socket_.send_to(request_buffer.data(), destination_);

		num_replies_ = 0;
		timer_.expires_at(time_sent_ + boost::posix_time::milliseconds(PingTimeoutms_));
		timer_.async_wait(boost::bind(&pinger::handle_timeout, this, boost::asio::placeholders::error));
	}

	void handle_timeout(const boost::system::error_code& error)
	{
		if (error != boost::asio::error::operation_aborted)
		{
			if (num_replies_ == 0)
			{
				m_PingState = false;
				timer_.expires_at(time_sent_ + boost::posix_time::milliseconds(PingTimeoutms_));
				num_tries_++;
				if (num_tries_ > 4)
				{
					GET_IO_SERVICE(resolver_).stop();
				}
				else
				{
					timer_.async_wait(boost::bind(&pinger::start_send, this));
				}
			}
		}
	}

	void start_receive()
	{
		// Discard any data already in the buffer.
		reply_buffer_.consume(reply_buffer_.size());

		// Wait for a reply. We prepare the buffer to receive up to 64KB.
		socket_.async_receive(reply_buffer_.prepare(65536),
			boost::bind(&pinger::handle_receive, this, _2));
	}

	void handle_receive(std::size_t length)
	{
		// The actual number of bytes received is committed to the buffer so that we
		// can extract it using a std::istream object.
		reply_buffer_.commit(length);

		// Decode the reply packet.
		std::istream is(&reply_buffer_);
		ipv4_header ipv4_hdr;
		icmp_header icmp_hdr;
		is >> ipv4_hdr >> icmp_hdr;

		// We can receive all ICMP packets received by the host, so we need to
		// filter out only the echo replies that match the our identifier and
		// expected sequence number.
		if (is && icmp_hdr.type() == icmp_header::echo_reply
			&& icmp_hdr.identifier() == get_identifier()
			&& icmp_hdr.sequence_number() == sequence_number_)
		{
			if (num_replies_++ == 0)
				timer_.cancel();
			m_PingState = true;
			GET_IO_SERVICE(resolver_).stop();
		}
		else
		{
			// DD 2 possible 'invalid' replies that will be discarded are:
			// Type 8: Echo request, happens when we ping ourselves (localhost)
			// Type 3: Destination host unreachable.
			start_receive();
		}
	}

	static unsigned short get_identifier()
	{
#if defined(BOOST_WINDOWS)
		return static_cast<unsigned short>(::GetCurrentProcessId());
#else
		return static_cast<unsigned short>(::getpid());
#endif
	}
	boost::asio::ip::icmp::resolver resolver_;
	boost::asio::ip::icmp::endpoint destination_;
	boost::asio::ip::icmp::socket socket_;
	boost::asio::deadline_timer timer_;
	unsigned short sequence_number_;
	boost::posix_time::ptime time_sent_;
	boost::asio::streambuf reply_buffer_;
};

CPinger::CPinger(const int ID, const int PollIntervalsec, const int PingTimeoutms) :
	m_iThreadsRunning(0)
{
	m_HwdID = ID;
	m_bSkipReceiveCheck = true;
	SetSettings(PollIntervalsec, PingTimeoutms);
}

CPinger::~CPinger(void)
{
	m_bIsStarted = false;
}

bool CPinger::StartHardware()
{
	StopHardware();

	RequestStart();

	m_bIsStarted = true;
	sOnConnected(this);
	m_iThreadsRunning = 0;

	StartHeartbeatThread();

	ReloadNodes();

	//Start worker thread
	m_thread = std::make_shared<std::thread>(&CPinger::Do_Work, this);
	SetThreadNameInt(m_thread->native_handle());
	return true;
}

bool CPinger::StopHardware()
{
	StopHeartbeatThread();

	if (m_thread)
	{
		RequestStop();
		m_thread->join();
		m_thread.reset();
	}
	m_bIsStarted = false;
	return true;
}

bool CPinger::WriteToHardware(const char *pdata, const unsigned char length)
{
	_log.Log(LOG_ERROR, "Pinger: This is a read-only sensor!");
	return false;
}

void CPinger::AddNode(const std::string &Name, const std::string &IPAddress, const int Timeout)
{
	std::lock_guard<std::mutex> l(m_mutex);

	std::vector<std::vector<std::string> > result;

	//Check if exists
	result = m_sql.safe_query("SELECT ID FROM WOLNodes WHERE (HardwareID==%d) AND (Name=='%q') AND (MacAddress=='%q')",
		m_HwdID, Name.c_str(), IPAddress.c_str());
	if (!result.empty())
		return; //Already exists
	m_sql.safe_query("INSERT INTO WOLNodes (HardwareID, Name, MacAddress, Timeout) VALUES (%d,'%q','%q',%d)",
		m_HwdID, Name.c_str(), IPAddress.c_str(), Timeout);

	result = m_sql.safe_query("SELECT ID FROM WOLNodes WHERE (HardwareID==%d) AND (Name=='%q') AND (MacAddress=='%q')",
		m_HwdID, Name.c_str(), IPAddress.c_str());
	if (result.empty())
		return;

	int ID = atoi(result[0][0].c_str());

	char szID[40];
	sprintf(szID, "%X%02X%02X%02X", 0, 0, (ID & 0xFF00) >> 8, ID & 0xFF);

	SendSwitch(ID, 1, 255, false, 0, Name);
	ReloadNodes();
}

bool CPinger::UpdateNode(const int ID, const std::string &Name, const std::string &IPAddress, const int Timeout)
{
	std::lock_guard<std::mutex> l(m_mutex);

	std::vector<std::vector<std::string> > result;

	//Check if exists
	result = m_sql.safe_query("SELECT ID FROM WOLNodes WHERE (HardwareID==%d) AND (ID==%d)",
		m_HwdID, ID);
	if (result.empty())
		return false; //Not Found!?

	m_sql.safe_query("UPDATE WOLNodes SET Name='%q', MacAddress='%q', Timeout=%d WHERE (HardwareID==%d) AND (ID==%d)",
		Name.c_str(), IPAddress.c_str(), Timeout, m_HwdID, ID);

	char szID[40];
	sprintf(szID, "%X%02X%02X%02X", 0, 0, (ID & 0xFF00) >> 8, ID & 0xFF);

	//Also update Light/Switch
	m_sql.safe_query(
		"UPDATE DeviceStatus SET Name='%q' WHERE (HardwareID==%d) AND (DeviceID=='%q')",
		Name.c_str(), m_HwdID, szID);
	ReloadNodes();
	return true;
}

void CPinger::RemoveNode(const int ID)
{
	std::lock_guard<std::mutex> l(m_mutex);

	m_sql.safe_query("DELETE FROM WOLNodes WHERE (HardwareID==%d) AND (ID==%d)",
		m_HwdID, ID);

	//Also delete the switch
	char szID[40];
	sprintf(szID, "%X%02X%02X%02X", 0, 0, (ID & 0xFF00) >> 8, ID & 0xFF);

	m_sql.safe_query("DELETE FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='%q')",
		m_HwdID, szID);
	ReloadNodes();
}

void CPinger::RemoveAllNodes()
{
	std::lock_guard<std::mutex> l(m_mutex);

	m_sql.safe_query("DELETE FROM WOLNodes WHERE (HardwareID==%d)", m_HwdID);

	//Also delete the all switches
	m_sql.safe_query("DELETE FROM DeviceStatus WHERE (HardwareID==%d)", m_HwdID);
	ReloadNodes();
}

void CPinger::ReloadNodes()
{
	m_nodes.clear();
	std::vector<std::vector<std::string> > result;
	result = m_sql.safe_query("SELECT ID,Name,MacAddress,Timeout FROM WOLNodes WHERE (HardwareID==%d)",
		m_HwdID);
	if (!result.empty())
	{
		std::vector<std::vector<std::string> >::const_iterator itt;
		for (itt = result.begin(); itt != result.end(); ++itt)
		{
			std::vector<std::string> sd = *itt;

			PingNode pnode;
			pnode.ID = atoi(sd[0].c_str());
			pnode.Name = sd[1];
			pnode.IP = sd[2];
			pnode.LastOK = mytime(NULL);

			int SensorTimeoutSec = atoi(sd[3].c_str());
			pnode.SensorTimeoutSec = (SensorTimeoutSec > 0) ? SensorTimeoutSec : 5;
			m_nodes.push_back(pnode);
		}
	}
}

void CPinger::Do_Ping_Worker(const PingNode &Node)
{
	bool bPingOK = false;
	boost::asio::io_service io_service;
	try
	{
		pinger p(io_service, Node.IP.c_str(), m_iPingTimeoutms);
		io_service.run();
		if (p.m_PingState == true)
		{
			bPingOK = true;
		}
	}
	catch (std::exception& e)
	{
		(void)e;
		bPingOK = false;
	}
	catch (...)
	{
		bPingOK = false;
	}
	UpdateNodeStatus(Node, bPingOK);
	if (m_iThreadsRunning > 0) m_iThreadsRunning--;
}

void CPinger::UpdateNodeStatus(const PingNode &Node, const bool bPingOK)
{
	//_log.Log(LOG_STATUS, "Pinger: %s = %s", Node.Name.c_str(), (bPingOK == true) ? "OK" : "Error");
	if (!bPingOK)
	{
		//_log.Log(LOG_STATUS, "Pinger: Could not ping host: %s", Node.Name.c_str());
	}

	//Find out node, and update it's status
	std::vector<PingNode>::iterator itt;
	for (itt = m_nodes.begin(); itt != m_nodes.end(); ++itt)
	{
		if (itt->ID == Node.ID)
		{
			//Found it
			time_t atime = mytime(NULL);
			if (bPingOK)
			{
				itt->LastOK = atime;
				SendSwitch(Node.ID, 1, 255, bPingOK, 0, Node.Name);
			}
			else
			{
				if (difftime(atime, itt->LastOK) >= Node.SensorTimeoutSec)
				{
					itt->LastOK = atime;
					SendSwitch(Node.ID, 1, 255, bPingOK, 0, Node.Name);
				}
			}
			break;
		}
	}
}

void CPinger::DoPingHosts()
{
	std::lock_guard<std::mutex> l(m_mutex);
	std::vector<PingNode>::const_iterator itt;
	for (itt = m_nodes.begin(); itt != m_nodes.end(); ++itt)
	{
		if (IsStopRequested(0))
			return;
		if (m_iThreadsRunning < 1000)
		{
			//m_iThreadsRunning++;
			boost::thread t(boost::bind(&CPinger::Do_Ping_Worker, this, *itt));
			SetThreadName(t.native_handle(), "PingerWorker");
			t.join();
		}
	}
}

void CPinger::Do_Work()
{
	int mcounter = 0;
	int scounter = 0;
	bool bFirstTime = true;
	_log.Log(LOG_STATUS, "Pinger: Worker started...");
	while (!IsStopRequested(500))
	{
		mcounter++;
		if (mcounter == 2)
		{
			mcounter = 0;
			scounter++;
			if ((scounter >= m_iPollInterval) || (bFirstTime))
			{
				scounter = 0;
				bFirstTime = false;
				DoPingHosts();
			}
		}
	}
	//Make sure all our background workers are stopped
	while (m_iThreadsRunning > 0)
	{
		sleep_milliseconds(150);
	}
	_log.Log(LOG_STATUS, "Pinger: Worker stopped...");
}

void CPinger::SetSettings(const int PollIntervalsec, const int PingTimeoutms)
{
	//Defaults
	m_iPollInterval = 30;
	m_iPingTimeoutms = 1000;

	if (PollIntervalsec > 1)
		m_iPollInterval = PollIntervalsec;
	if ((PingTimeoutms / 1000 < m_iPollInterval) && (PingTimeoutms != 0))
		m_iPingTimeoutms = PingTimeoutms;
}

//Webserver helpers
namespace http {
	namespace server {
		void CWebServer::Cmd_PingerGetNodes(WebEmSession & session, const request& req, Json::Value &root)
		{
			if (session.rights != 2)
			{
				session.reply_status = reply::forbidden;
				return; //Only admin user allowed
			}
			std::string hwid = request::findValue(&req, "idx");
			if (hwid == "")
				return;
			int iHardwareID = atoi(hwid.c_str());
			CDomoticzHardwareBase *pHardware = m_mainworker.GetHardware(iHardwareID);
			if (pHardware == NULL)
				return;
			if (pHardware->HwdType != HTYPE_Pinger)
				return;

			root["status"] = "OK";
			root["title"] = "PingerGetNodes";

			std::vector<std::vector<std::string> > result;
			result = m_sql.safe_query("SELECT ID,Name,MacAddress,Timeout FROM WOLNodes WHERE (HardwareID==%d)",
				iHardwareID);
			if (!result.empty())
			{
				std::vector<std::vector<std::string> >::const_iterator itt;
				int ii = 0;
				for (itt = result.begin(); itt != result.end(); ++itt)
				{
					std::vector<std::string> sd = *itt;

					root["result"][ii]["idx"] = sd[0];
					root["result"][ii]["Name"] = sd[1];
					root["result"][ii]["IP"] = sd[2];
					root["result"][ii]["Timeout"] = atoi(sd[3].c_str());
					ii++;
				}
			}
		}

		void CWebServer::Cmd_PingerSetMode(WebEmSession & session, const request& req, Json::Value &root)
		{
			if (session.rights != 2)
			{
				session.reply_status = reply::forbidden;
				return; //Only admin user allowed
			}
			std::string hwid = request::findValue(&req, "idx");
			std::string mode1 = request::findValue(&req, "mode1");
			std::string mode2 = request::findValue(&req, "mode2");
			if (
				(hwid == "") ||
				(mode1 == "") ||
				(mode2 == "")
				)
				return;
			int iHardwareID = atoi(hwid.c_str());
			CDomoticzHardwareBase *pBaseHardware = m_mainworker.GetHardware(iHardwareID);
			if (pBaseHardware == NULL)
				return;
			if (pBaseHardware->HwdType != HTYPE_Pinger)
				return;
			CPinger *pHardware = reinterpret_cast<CPinger*>(pBaseHardware);

			root["status"] = "OK";
			root["title"] = "PingerSetMode";

			int iMode1 = atoi(mode1.c_str());
			int iMode2 = atoi(mode2.c_str());

			m_sql.safe_query(
				"UPDATE Hardware SET Mode1=%d, Mode2=%d WHERE (ID == '%q')",
				iMode1,
				iMode2,
				hwid.c_str());
			pHardware->SetSettings(iMode1, iMode2);
			pHardware->Restart();
		}


		void CWebServer::Cmd_PingerAddNode(WebEmSession & session, const request& req, Json::Value &root)
		{
			if (session.rights != 2)
			{
				session.reply_status = reply::forbidden;
				return; //Only admin user allowed
			}

			std::string hwid = request::findValue(&req, "idx");
			std::string name = HTMLSanitizer::Sanitize(request::findValue(&req, "name"));
			std::string ip = HTMLSanitizer::Sanitize(request::findValue(&req, "ip"));
			int Timeout = atoi(request::findValue(&req, "timeout").c_str());
			if (
				(hwid == "") ||
				(name == "") ||
				(ip == "") ||
				(Timeout == 0)
				)
				return;
			int iHardwareID = atoi(hwid.c_str());
			CDomoticzHardwareBase *pBaseHardware = m_mainworker.GetHardware(iHardwareID);
			if (pBaseHardware == NULL)
				return;
			if (pBaseHardware->HwdType != HTYPE_Pinger)
				return;
			CPinger *pHardware = reinterpret_cast<CPinger*>(pBaseHardware);

			root["status"] = "OK";
			root["title"] = "PingerAddNode";
			pHardware->AddNode(name, ip, Timeout);
		}

		void CWebServer::Cmd_PingerUpdateNode(WebEmSession & session, const request& req, Json::Value &root)
		{
			if (session.rights != 2)
			{
				session.reply_status = reply::forbidden;
				return; //Only admin user allowed
			}

			std::string hwid = request::findValue(&req, "idx");
			std::string nodeid = request::findValue(&req, "nodeid");
			std::string name = HTMLSanitizer::Sanitize(request::findValue(&req, "name"));
			std::string ip = HTMLSanitizer::Sanitize(request::findValue(&req, "ip"));
			int Timeout = atoi(request::findValue(&req, "timeout").c_str());
			if (
				(hwid == "") ||
				(nodeid == "") ||
				(name == "") ||
				(ip == "") ||
				(Timeout == 0)
				)
				return;
			int iHardwareID = atoi(hwid.c_str());
			CDomoticzHardwareBase *pBaseHardware = m_mainworker.GetHardware(iHardwareID);
			if (pBaseHardware == NULL)
				return;
			if (pBaseHardware->HwdType != HTYPE_Pinger)
				return;
			CPinger *pHardware = reinterpret_cast<CPinger*>(pBaseHardware);

			int NodeID = atoi(nodeid.c_str());
			root["status"] = "OK";
			root["title"] = "PingerUpdateNode";
			pHardware->UpdateNode(NodeID, name, ip, Timeout);
		}

		void CWebServer::Cmd_PingerRemoveNode(WebEmSession & session, const request& req, Json::Value &root)
		{
			if (session.rights != 2)
			{
				session.reply_status = reply::forbidden;
				return; //Only admin user allowed
			}

			std::string hwid = request::findValue(&req, "idx");
			std::string nodeid = request::findValue(&req, "nodeid");
			if (
				(hwid == "") ||
				(nodeid == "")
				)
				return;
			int iHardwareID = atoi(hwid.c_str());
			CDomoticzHardwareBase *pBaseHardware = m_mainworker.GetHardware(iHardwareID);
			if (pBaseHardware == NULL)
				return;
			if (pBaseHardware->HwdType != HTYPE_Pinger)
				return;
			CPinger *pHardware = reinterpret_cast<CPinger*>(pBaseHardware);

			int NodeID = atoi(nodeid.c_str());
			root["status"] = "OK";
			root["title"] = "PingerRemoveNode";
			pHardware->RemoveNode(NodeID);
		}

		void CWebServer::Cmd_PingerClearNodes(WebEmSession & session, const request& req, Json::Value &root)
		{
			if (session.rights != 2)
			{
				session.reply_status = reply::forbidden;
				return; //Only admin user allowed
			}

			std::string hwid = request::findValue(&req, "idx");
			if (hwid == "")
				return;
			int iHardwareID = atoi(hwid.c_str());
			CDomoticzHardwareBase *pBaseHardware = m_mainworker.GetHardware(iHardwareID);
			if (pBaseHardware == NULL)
				return;
			if (pBaseHardware->HwdType != HTYPE_Pinger)
				return;
			CPinger *pHardware = reinterpret_cast<CPinger*>(pBaseHardware);

			root["status"] = "OK";
			root["title"] = "PingerClearNodes";
			pHardware->RemoveAllNodes();
		}
	}
}
