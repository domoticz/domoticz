#include "stdafx.h"
#include "Comm5TCP.h"
#include "../main/Logger.h"
#include "../main/Helper.h"
#include "../main/localtime_r.h"
#include "../main/mainworker.h"

#include <iostream>

#include <boost/lexical_cast.hpp>

#define RETRY_DELAY 30
#define Max_Comm5_MA_Relais 16

static inline std::string &rtrim(std::string &s) {
	s.erase(std::find_if(s.rbegin(), s.rend(), std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());
	return s;
}

static inline std::vector<std::string> tokenize(const std::string &s) {
	std::vector<std::string> tokens;
	std::istringstream iss(s);
	std::copy(std::istream_iterator<std::string>(iss),
		std::istream_iterator<std::string>(),
		std::back_inserter(tokens));
	return tokens;
}

static inline bool startsWith(const std::string &haystack, const std::string &needle) {
	return needle.length() <= haystack.length()
		&& std::equal(needle.begin(), needle.end(), haystack.begin());
}

Comm5TCP::Comm5TCP(const int ID, const std::string &IPAddress, const unsigned short usIPPort) :
m_szIPAddress(IPAddress)
{
	m_HwdID=ID;
	m_stoprequested=false;
	m_usIPPort=usIPPort;
	lastKnownSensorState = 0;
	initSensorData = true;
	notificationEnabled = false;
	m_bReceiverStarted = false;
}

bool Comm5TCP::StartHardware()
{
	m_stoprequested = false;
	m_bReceiverStarted = false;

	//force connect the next first time
	m_bIsStarted = true;
	m_rxbufferpos = 0;

	//Start worker thread
	m_thread = boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&Comm5TCP::Do_Work, this)));

	_log.Log(LOG_STATUS, "Comm5 MA-5XXX: Started");

	return (m_thread != NULL);
}

bool Comm5TCP::StopHardware()
{
	if (m_thread != NULL)
	{
		assert(m_thread);
		m_stoprequested = true;
		m_thread->join();
	}
	m_bIsStarted = false;
	return true;
}

void Comm5TCP::OnConnect()
{
	_log.Log(LOG_STATUS, "Comm5 MA-5XXX: connected to: %s:%ld", m_szIPAddress.c_str(), m_usIPPort);
	m_bIsStarted = true;
	notificationEnabled = false;

	sOnConnected(this);
	queryRelayState();
	querySensorState();
	enableNotifications();
}

void Comm5TCP::OnDisconnect()
{
	_log.Log(LOG_ERROR, "Comm5 MA-5XXX: disconected");
}

void Comm5TCP::Do_Work()
{
	bool bFirstTime = true;
	int count = 0;
	while (!m_stoprequested)
	{
		m_LastHeartbeat = mytime(NULL);
		if (bFirstTime)
		{
			bFirstTime = false;
			if (!mIsConnected)
			{
				m_rxbufferpos = 0;
				connect(m_szIPAddress, m_usIPPort);
			}
		}
		else
		{
			sleep_milliseconds(40);
			update();
			if (count++ >= 100) {
				count = 0;
				querySensorState();
			}
		}
	}
	_log.Log(LOG_STATUS, "Comm5 MA-5XXX: TCP/IP Worker stopped...");
}

void Comm5TCP::processSensorData(const std::string& line) 
{
	std::vector<std::string> tokens = tokenize(line);
	if (tokens.size() < 2)
		return;

	unsigned int sensorbitfield = ::strtol(tokens[1].c_str(), 0, 16);
	for (int i = 0; i < 16; ++i) {
		bool on = (sensorbitfield & (1 << i)) != 0 ? true : false;
		if (((lastKnownSensorState & (1 << i)) ^ (sensorbitfield & (1 << i))) || initSensorData) {
			SendSwitch((i + 1) << 8, 1, 255, on, 0, "Sensor " + boost::lexical_cast<std::string>(i + 1));
		}
	}
	lastKnownSensorState = sensorbitfield;
	initSensorData = false;
}

void Comm5TCP::ParseData(const unsigned char* data, const size_t len)
{
	buffer.append((const char*)data, len);

	std::stringstream stream(buffer);
	std::string line;

	while (std::getline(stream, line, '\n')) {
		line = rtrim(line);
		if (startsWith(line, "211")) {
			std::vector<std::string> tokens = tokenize(line);
			if (tokens.size() < 2)
				break;

			unsigned int relaybitfield = ::strtol(tokens[1].c_str(), 0, 16);
			for (int i = 0; i < 16; ++i) {
				bool on = (relaybitfield & (1 << i)) != 0 ? true : false;
				SendSwitch(i + 1, 1, 255, on, 0, "Relay " + boost::lexical_cast<std::string>(i + 1));
			}
		}
		else if (startsWith(line, "210") && (!startsWith(line, "210 OK"))) {
			processSensorData(line);
		} 
	}

	// Trim consumed bytes.
	buffer.erase(0, buffer.length() - static_cast<unsigned int>(stream.rdbuf()->in_avail()));
}

void Comm5TCP::queryRelayState()
{
	write("OUTPUTS\n");
}

void Comm5TCP::querySensorState()
{
	write("QUERY\n");
}

void Comm5TCP::enableNotifications()
{
	write("NOTIFY ON\n");
	notificationEnabled = true;
}

bool Comm5TCP::WriteToHardware(const char *pdata, const unsigned char length)
{
	const tRBUF *pSen = reinterpret_cast<const tRBUF*>(pdata);

	unsigned char packettype = pSen->ICMND.packettype;
	//unsigned char subtype = pSen->ICMND.subtype;

	if (!mIsConnected)
		return false;

	if (packettype == pTypeLighting2 && pSen->LIGHTING2.id3 == 0)
	{
		//light command

		int Relay = pSen->LIGHTING2.id4;
		if (Relay > Max_Comm5_MA_Relais)
			return false;

		if (pSen->LIGHTING2.cmnd == light2_sOff)
			write("RESET " + boost::lexical_cast<std::string>(Relay) + "\n");
		else
			write("SET " + boost::lexical_cast<std::string>(Relay) + "\n");

		return true;
	}
	return false;
}

void Comm5TCP::OnData(const unsigned char *pData, size_t length)
{
	boost::lock_guard<boost::mutex> l(readQueueMutex);
	ParseData(pData, length);
}

void Comm5TCP::OnError(const std::exception e)
{
	_log.Log(LOG_ERROR, "Comm5 MA-5XXX: Error: %s", e.what());
}

void Comm5TCP::OnError(const boost::system::error_code& error)
{
	switch (error.value())
	{
	case boost::asio::error::address_in_use:
	case boost::asio::error::connection_refused:
	case boost::asio::error::access_denied:
	case boost::asio::error::host_unreachable:
	case boost::asio::error::timed_out:
		_log.Log(LOG_ERROR, "Comm5 MA-5XXX: Can not connect to: %s:%ld", m_szIPAddress.c_str(), m_usIPPort);
		break;
	case boost::asio::error::eof:
	case boost::asio::error::connection_reset:
		_log.Log(LOG_ERROR, "Comm5 MA-5XXX: Connection reset!");
		break;
	default:
		_log.Log(LOG_ERROR, "Comm5 MA-5XXX: %s", error.message().c_str());
	}
}
