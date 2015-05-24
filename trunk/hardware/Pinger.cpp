#include "stdafx.h"
#include "Pinger.h"
#include "../main/Helper.h"
#include "../main/Logger.h"
#include "../main/SQLHelper.h"
#include "../main/RFXtrx.h"
#include "../main/localtime_r.h"

#include <boost/asio.hpp>

#include "pinger/icmp_header.h"
#include "pinger/ipv4_header.h"

#define PINGER_POLL_INTERVAL 30

class pinger
{
public:
	pinger(boost::asio::io_service& io_service, const char* destination)
		: resolver_(io_service), socket_(io_service, boost::asio::ip::icmp::v4()),
		timer_(io_service), sequence_number_(0), num_replies_(0)
	{
		m_PingState = false;
		boost::asio::ip::icmp::resolver::query query(boost::asio::ip::icmp::v4(), destination, "");
		destination_ = *resolver_.resolve(query);

		start_send();
		start_receive();
	}
	bool m_PingState;

private:
	void start_send()
	{
		std::string body("\"Hello!\" from Domoticz.");

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

		// Wait up to five seconds for a reply.
		num_replies_ = 0;
		timer_.expires_at(time_sent_ + boost::posix_time::seconds(5));
		timer_.async_wait(boost::bind(&pinger::handle_timeout, this));
	}

	void handle_timeout()
	{
		if (num_replies_ == 0)
		{
			std::cout << "Request timed out" << std::endl;
			m_PingState = false;
		}
		resolver_.get_io_service().stop();
		return;
		//		// Requests must be sent no less than one second apart.
		timer_.expires_at(time_sent_ + boost::posix_time::seconds(1));
		timer_.async_wait(boost::bind(&pinger::start_send, this));
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
			// If this is the first reply, interrupt the five second timeout.
			if (num_replies_++ == 0)
				timer_.cancel();
			m_PingState = true;
			/*
			// Print out some information about the reply packet.
			posix_time::ptime now = posix_time::microsec_clock::universal_time();
			std::cout << length - ipv4_hdr.header_length()
			<< " bytes from " << ipv4_hdr.source_address()
			<< ": icmp_seq=" << icmp_hdr.sequence_number()
			<< ", ttl=" << ipv4_hdr.time_to_live()
			<< ", time=" << (now - time_sent_).total_milliseconds() << " ms"
			<< std::endl;
			*/
		}
		resolver_.get_io_service().stop();
		//start_receive();
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
	std::size_t num_replies_;
};

CPinger::CPinger(const int ID)
{
	m_HwdID=ID;
	m_bSkipReceiveCheck = true;
#ifdef WIN32
	// Initialize Winsock
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		_log.Log(LOG_ERROR,"Pinger: Error initializing Winsock!");
	}
#endif
}

CPinger::~CPinger(void)
{
	m_bIsStarted=false;
#ifdef WIN32
	WSACleanup();
#endif
}

bool CPinger::StartHardware()
{
	StopHardware();
	m_bIsStarted=true;
	sOnConnected(this);

	StartHeartbeatThread();

	//Start worker thread
	m_stoprequested = false;
	m_thread = boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&CPinger::Do_Work, this)));
	_log.Log(LOG_STATUS,"Pinger: Started");

	return true;
}

bool CPinger::StopHardware()
{
	StopHeartbeatThread();

	try {
		if (m_thread)
		{
			m_stoprequested = true;
			m_thread->join();
			m_thread.reset();
		}
	}
	catch (...)
	{
		//Don't throw from a Stop command
	}
	m_bIsStarted=false;
    return true;
}

bool CPinger::WriteToHardware(const char *pdata, const unsigned char length)
{
	_log.Log(LOG_ERROR, "Pinger: This is a read-only sensor!");
	return false;
}

void CPinger::AddNode(const std::string &Name, const std::string &IPAddress)
{
	boost::lock_guard<boost::mutex> l(m_mutex);

	std::stringstream szQuery;
	std::vector<std::vector<std::string> > result;

	//Check if exists
	szQuery << "SELECT ID FROM WOLNodes WHERE (HardwareID==" << m_HwdID << ") AND (Name=='" << Name << "') AND (MacAddress=='" << IPAddress << "')";
	result=m_sql.query(szQuery.str());
	if (result.size()>0)
		return; //Already exists
	szQuery.clear();
	szQuery.str("");
	szQuery << "INSERT INTO WOLNodes (HardwareID, Name, MacAddress) VALUES (" << m_HwdID << ",'" << Name << "','" << IPAddress << "')";
	m_sql.query(szQuery.str());

	szQuery.clear();
	szQuery.str("");
	szQuery << "SELECT ID FROM WOLNodes WHERE (HardwareID==" << m_HwdID << ") AND (Name=='" << Name << "') AND (MacAddress=='" << IPAddress << "')";
	result=m_sql.query(szQuery.str());
	if (result.size()<1)
		return;

	int ID=atoi(result[0][0].c_str());

	char szID[40];
	sprintf(szID,"%X%02X%02X%02X", 0, 0, (ID&0xFF00)>>8, ID&0xFF);

	SendSwitch(ID, 1, 255, false, 0, Name);
}

bool CPinger::UpdateNode(const int ID, const std::string &Name, const std::string &IPAddress)
{
	boost::lock_guard<boost::mutex> l(m_mutex);

	std::stringstream szQuery;
	std::vector<std::vector<std::string> > result;

	//Check if exists
	szQuery << "SELECT ID FROM WOLNodes WHERE (HardwareID==" << m_HwdID << ") AND (ID==" << ID << ")";
	result=m_sql.query(szQuery.str());
	if (result.size()<1)
		return false; //Not Found!?

	szQuery.clear();
	szQuery.str("");

	szQuery << "UPDATE WOLNodes SET Name='" << Name << "', MacAddress='" << IPAddress << "' WHERE (HardwareID==" << m_HwdID << ") AND (ID==" << ID << ")";
	m_sql.query(szQuery.str());

	char szID[40];
	sprintf(szID,"%X%02X%02X%02X", 0, 0, (ID&0xFF00)>>8, ID&0xFF);

	//Also update Light/Switch
	szQuery.clear();
	szQuery.str("");
	szQuery <<
		"UPDATE DeviceStatus SET Name='" << Name << "' WHERE (HardwareID==" << m_HwdID << ") AND (DeviceID=='" << szID << "')";
	m_sql.query(szQuery.str());

	return true;
}

void CPinger::RemoveNode(const int ID)
{
	boost::lock_guard<boost::mutex> l(m_mutex);

	std::stringstream szQuery;
	szQuery << "DELETE FROM WOLNodes WHERE (HardwareID==" << m_HwdID << ") AND (ID==" << ID << ")";
	m_sql.query(szQuery.str());

	//Also delete the switch
	szQuery.clear();
	szQuery.str("");

	char szID[40];
	sprintf(szID,"%X%02X%02X%02X", 0, 0, (ID&0xFF00)>>8, ID&0xFF);

	szQuery << "DELETE FROM DeviceStatus WHERE (HardwareID==" << m_HwdID << ") AND (DeviceID=='" << szID << "')";
	m_sql.query(szQuery.str());
}

void CPinger::RemoveAllNodes()
{
	boost::lock_guard<boost::mutex> l(m_mutex);

	std::stringstream szQuery;
	szQuery << "DELETE FROM WOLNodes WHERE (HardwareID==" << m_HwdID << ")";
	m_sql.query(szQuery.str());

	//Also delete the all switches
	szQuery.clear();
	szQuery.str("");
	szQuery << "DELETE FROM DeviceStatus WHERE (HardwareID==" << m_HwdID << ")";
	m_sql.query(szQuery.str());
}

void CPinger::DoPingHosts()
{
	std::stringstream szQuery;
	std::vector<std::vector<std::string> > result;
	szQuery << "SELECT ID,Name,MacAddress FROM WOLNodes WHERE (HardwareID==" << m_HwdID << ")";
	result = m_sql.query(szQuery.str());
	if (result.size() > 0)
	{
		std::vector<std::vector<std::string> >::const_iterator itt;
		for (itt = result.begin(); itt != result.end(); ++itt)
		{
			boost::lock_guard<boost::mutex> l(m_mutex);

			std::vector<std::string> sd = *itt;

			std::string idx = sd[0];
			std::string Name = sd[1];
			std::string IP = sd[2];

			int ID = atoi(idx.c_str());

			bool bPingOK = false;
			boost::asio::io_service io_service;
			try
			{
				pinger p(io_service, IP.c_str());
				io_service.run();

				if (p.m_PingState == true)
				{
					//Could ping device
					bPingOK = true;
				}
			}
			catch (std::exception& e)
			{
				//Could not ping device
				bPingOK = false;
				_log.Log(LOG_STATUS, "Pinger: Could not ping host: %s",Name.c_str());
			}
			catch (...)
			{
				//Could not ping device
				bPingOK = false;
				_log.Log(LOG_STATUS, "Pinger: Could not ping host: %s", Name.c_str());
			}
			SendSwitch(ID, 1, 255, bPingOK, (bPingOK==true)? 100 : 0, Name);

		}
	}
}

void CPinger::Do_Work()
{
	int scounter = 0;
	bool bFirstTime = true;
	while (!m_stoprequested)
	{
		sleep_milliseconds(500);
		scounter++;
		if (scounter >= 2)
		{
			scounter = 0;
			time_t atime = mytime(NULL);
			if ((atime % PINGER_POLL_INTERVAL == 0)||(bFirstTime))
			{
				bFirstTime = false;
				DoPingHosts();
			}
		}
	}
	_log.Log(LOG_STATUS, "Pinger: Worker stopped...");
}
