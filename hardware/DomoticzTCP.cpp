#include "stdafx.h"
#include "DomoticzTCP.h"
#include "../main/Logger.h"
#include "../main/Helper.h"
#include "../main/mainworker.h"
#include "../main/WebServerHelper.h"

#define RETRY_DELAY 30

extern http::server::CWebServerHelper m_webservers;

DomoticzTCP::DomoticzTCP(const int ID, const std::string &IPAddress, const unsigned short usIPPort, const std::string &username, const std::string &password)
	: m_szIPAddress(IPAddress)
	, m_username(username)
	, m_password(password)
{
	m_HwdID = ID;
	m_usIPPort = usIPPort;
	m_bIsStarted = false;
}

bool DomoticzTCP::StartHardware()
{
	RequestStart();

	//Start worker thread
	m_thread = std::make_shared<std::thread>([this] { Do_Work(); });
	SetThreadNameInt(m_thread->native_handle());

	return (m_thread != nullptr);
}

bool DomoticzTCP::StopHardware()
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

void DomoticzTCP::OnConnect()
{
	Log(LOG_STATUS, "connected to: %s:%d", m_szIPAddress.c_str(), m_usIPPort);
	if (!m_username.empty())
	{
		std::string sAuth = std_format("AUTH;%s;%s", m_username.c_str(), m_password.c_str());
		WriteToHardware(sAuth.c_str(), (unsigned char)sAuth.size());
	}
	sOnConnected(this);
}

void DomoticzTCP::OnDisconnect()
{
	Log(LOG_STATUS, "disconnected from: %s:%d", m_szIPAddress.c_str(), m_usIPPort);
}

void DomoticzTCP::OnData(const unsigned char *pData, size_t length)
{
	if (length == 6 && strstr(reinterpret_cast<const char *>(pData), "NOAUTH") != nullptr)
	{
		Log(LOG_ERROR, "Authentication failed for user %s on %s:%d", m_username.c_str(), m_szIPAddress.c_str(), m_usIPPort);
		return;
	}
	std::lock_guard<std::mutex> l(readQueueMutex);
	onInternalMessage((const unsigned char *)pData, length, false); // Do not check validity, this might be non RFX-message
}

void DomoticzTCP::OnError(const boost::system::error_code& error)
{
	if (
		(error == boost::asio::error::address_in_use) ||
		(error == boost::asio::error::connection_refused) ||
		(error == boost::asio::error::access_denied) ||
		(error == boost::asio::error::host_unreachable) ||
		(error == boost::asio::error::timed_out) ||
		(error == boost::asio::error::host_not_found)
		)
	{
		Log(LOG_ERROR, "Can not connect to: %s:%d (%s)", m_szIPAddress.c_str(), m_usIPPort, error.message().c_str());
	}
	else if (error != boost::asio::error::eof)
	{
		Log(LOG_ERROR, "%s", error.message().c_str());
	}
}

void DomoticzTCP::Do_Work()
{
	connect(m_szIPAddress, m_usIPPort);
	int sec_counter = 0;
	while (!IsStopRequested(1000))
	{
		sec_counter++;
		if (sec_counter % 12 == 0)
			mytime(&m_LastHeartbeat);
	}
	terminate();

	Log(LOG_STATUS, "Worker stopped...");
}

bool DomoticzTCP::WriteToHardware(const char *pdata, const unsigned char length)
{
	if (ASyncTCP::isConnected())
	{
		write(std::string((const char*)pdata, length));
		return true;
	}
	return false;
}
