#include "stdafx.h"
#include "Comm5SMTCP.h"
#include "../main/Helper.h"
#include "../main/localtime_r.h"
#include "../main/Logger.h"
#include "../main/mainworker.h"
#include "../httpclient/HTTPClient.h"
#include "../httpclient/UrlEncode.h"

#include <iostream>

/*
	This driver allows Domoticz to read sensor data from SM-XXXX sensors from Comm5 Technology
	The SM-1200 sensor provides: Temperature, Humidity, Barometric and Luminosity data.
	https://www.comm5.com.br/en/sm-1200/
*/

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

Comm5SMTCP::Comm5SMTCP(const int ID, const std::string &IPAddress, const unsigned short usIPPort) :
	m_szIPAddress(IPAddress)
{
	m_HwdID = ID;
	m_usIPPort = usIPPort;
	initSensorData = true;
	m_bReceiverStarted = false;
}

bool Comm5SMTCP::StartHardware()
{
	RequestStart();

	m_bReceiverStarted = false;

	//force connect the next first time
	m_bIsStarted = true;

	//Start worker thread
	m_thread = std::make_shared<std::thread>(&Comm5SMTCP::Do_Work, this);
	SetThreadNameInt(m_thread->native_handle());

	Log(LOG_STATUS, "Started");

	return (m_thread != nullptr);
}

bool Comm5SMTCP::StopHardware()
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

void Comm5SMTCP::OnConnect()
{
	Log(LOG_STATUS, "connected to: %s:%d", m_szIPAddress.c_str(), m_usIPPort);
	m_bIsStarted = true;

	sOnConnected(this);
	querySensorState();
}

void Comm5SMTCP::OnDisconnect()
{
	Log(LOG_ERROR, "disconected");
}

void Comm5SMTCP::Do_Work()
{
	int sec_counter = 0;
	connect(m_szIPAddress, m_usIPPort);
	while (!IsStopRequested(1000))
	{
		sec_counter++;

		if (sec_counter % 12 == 0) {
			m_LastHeartbeat = mytime(NULL);
		}
		if (sec_counter % 4 == 0) {
			querySensorState();
		}
	}
	terminate();

	Log(LOG_STATUS, "TCP/IP Worker stopped...");
}

void Comm5SMTCP::ParseData(const unsigned char* data, const size_t len)
{
	buffer.append((const char*)data, len);

	if (buffer.find('\n') == std::string::npos)
		return; // Incomplete lines

	std::stringstream stream(buffer);
	std::string line;

	while (std::getline(stream, line, '\n')) {
		line = rtrim(line);
		if (startsWith(line, "280")) {
			std::vector<std::string> tokens = tokenize(line);
			if (tokens.size() < 2)
				break;

			float temperature = static_cast<float>(atof(tokens[1].c_str()));
			SendTempSensor(1, 255, temperature, "TEMPERATURE");
		}
		else if (startsWith(line, "281")) {
			std::vector<std::string> tokens = tokenize(line);
			if (tokens.size() < 2)
				break;

			int humidity = atoi(tokens[1].c_str());
			SendHumiditySensor(1, 255, humidity, "HUMIDITY");
		}
		else if (startsWith(line, "282")) {
			std::vector<std::string> tokens = tokenize(line);
			if (tokens.size() < 2)
				break;

			float baro = static_cast<float>(atof(tokens[1].c_str()));
			SendBaroSensor(1, 0, 255, baro, 0, "BAROMETRIC");
		}
	}

	// Trim consumed bytes.
	buffer.erase(0, buffer.length() - static_cast<unsigned int>(stream.rdbuf()->in_avail()));
}

void Comm5SMTCP::querySensorState()
{
	write("TEMPERATURE\n\r");
	write("HUMIDITY\n\r");
	write("PRESSURE\n\r");
}

bool Comm5SMTCP::WriteToHardware(const char* /*pdata*/, const unsigned char /*length*/)
{
	return false;
}

void Comm5SMTCP::OnData(const unsigned char *pData, size_t length)
{
	ParseData(pData, length);
}

void Comm5SMTCP::OnError(const std::exception e)
{
	Log(LOG_ERROR, "Error: %s", e.what());
}

void Comm5SMTCP::OnError(const boost::system::error_code& error)
{
	switch (error.value())
	{
	case boost::asio::error::address_in_use:
	case boost::asio::error::connection_refused:
	case boost::asio::error::access_denied:
	case boost::asio::error::host_unreachable:
	case boost::asio::error::timed_out:
		Log(LOG_ERROR, "Can not connect to: %s:%d", m_szIPAddress.c_str(), m_usIPPort);
		break;
	case boost::asio::error::eof:
	case boost::asio::error::connection_reset:
		Log(LOG_ERROR, "Connection reset!");
		break;
	default:
		Log(LOG_ERROR, "%s", error.message().c_str());
	}
}
