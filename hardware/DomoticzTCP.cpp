#include "stdafx.h"
#include "DomoticzTCP.h"
#include "../main/Logger.h"
#include "../main/Helper.h"
#include "../main/localtime_r.h"
#include "../main/mainworker.h"
#include "../main/WebServerHelper.h"
#include "../webserver/proxyclient.h"
#include "csocket.h"

#define RETRY_DELAY 30

extern http::server::CWebServerHelper m_webservers;

DomoticzTCP::DomoticzTCP(const int ID, const std::string &IPAddress, const unsigned short usIPPort, const std::string &username, const std::string &password) :
	m_username(username),
	m_password(password),
	m_szIPAddress(IPAddress),
	m_usIPPort(usIPPort),
	m_retrycntr(RETRY_DELAY),
	m_connection(nullptr)
{
	m_HwdID = ID;
	m_bIsStarted = false;
#ifndef NOCLOUD
	b_useProxy = IsValidAPIKey(m_szIPAddress);
	b_ProxyConnected = false;
#endif
}

DomoticzTCP::~DomoticzTCP(void)
{
}

#ifndef NOCLOUD
bool DomoticzTCP::IsValidAPIKey(const std::string &IPAddress)
{
	if (IPAddress.find(".") != std::string::npos) {
		// we assume an IPv4 address or host name
		return false;
	}
	if (IPAddress.find(":") != std::string::npos) {
		// we assume an IPv6 address
		return false;
	}
	// just a simple check
	return IPAddress.length() == 15;
}
#endif

bool DomoticzTCP::StartHardware()
{
	RequestStart();

#ifndef NOCLOUD
	b_useProxy = IsValidAPIKey(m_szIPAddress);
	if (b_useProxy) {
		return StartHardwareProxy();
	}
#endif
	m_bIsStarted = true;
	m_retrycntr = RETRY_DELAY; //will force reconnect first thing
	ConnectInternal();
	//Start worker thread
	m_thread = std::make_shared<std::thread>(&DomoticzTCP::Do_Work, this);
	SetThreadName(m_thread->native_handle(), "DomoticzTCP");

	return (m_thread != nullptr);
}

bool DomoticzTCP::StopHardware()
{
#ifndef NOCLOUD
	if (b_useProxy) {
		return StopHardwareProxy();
	}
#endif

	if (m_thread)
	{
		RequestStop();
		m_thread->join();
		m_thread.reset();
	}
	m_bIsStarted = false;
	return true;
}

bool DomoticzTCP::ConnectInternal()
{
	if (m_connection)
		delete m_connection;
	m_connection = new csocket();
	if (m_connection->connect(m_szIPAddress.c_str(), m_usIPPort))
	{
		_log.Log(LOG_ERROR, "Domoticz: TCP could not connect to: %s:%d", m_szIPAddress.c_str(), m_usIPPort);
		return false;
	}
#ifndef WIN32
	// Send keepalive after 600 sec inactivity, closes when no response after 12 * 10 sec.
	int keepaliveEnable = 1;
	int keepaliveTime = 600;
	int keepaliveCount = 12;
	int keepaliveInterval = 10;
	if (
		m_connection->setSockOpt(IPPROTO_TCP, TCP_KEEPIDLE, &keepaliveTime, sizeof(keepaliveTime)) ||
		m_connection->setSockOpt(IPPROTO_TCP, TCP_KEEPCNT, &keepaliveCount, sizeof(keepaliveCount)) ||
		m_connection->setSockOpt(IPPROTO_TCP, TCP_KEEPINTVL, &keepaliveInterval, sizeof(keepaliveInterval)) ||
#else
	char keepaliveEnable = 1;
	if (
#endif
		m_connection->setSockOpt(SOL_SOCKET, SO_KEEPALIVE, &keepaliveEnable, sizeof(keepaliveEnable)))
	{
		_log.Log(LOG_ERROR, "Domoticz: TCP could not set keepalive on socket");
	}
	_log.Log(LOG_STATUS, "Domoticz: TCP connected to: %s:%d", m_szIPAddress.c_str(), m_usIPPort);

	if (m_username != "")
	{
		char szAuth[300];
		snprintf(szAuth, sizeof(szAuth), "AUTH;%s;%s", m_username.c_str(), m_password.c_str());
		WriteToHardware((const char*)&szAuth, (const unsigned char)strlen(szAuth));
	}

	sOnConnected(this);
	return true;
}

void DomoticzTCP::Do_Work()
{
	char buf[100];
	int sec_counter = 0;
	while (!IsStopRequested(100))
	{
		if (m_connection->getState() != csocket::CONNECTED)
		{
			if (!IsStopRequested(900)) //+100 = 1 second
			{
			}
			sleep_seconds(1);
			sec_counter++;

			if (sec_counter % 12 == 0) {
				mytime(&m_LastHeartbeat);
			}

			m_retrycntr++;
			if (m_retrycntr >= RETRY_DELAY)
			{
				m_retrycntr = 0;
				if (!ConnectInternal())
				{
					_log.Log(LOG_STATUS, "Domoticz: retrying in %d seconds...", RETRY_DELAY);
				}
			}
		}
		else
		{
			sec_counter++;
			if (sec_counter % 12 == 0) {
				mytime(&m_LastHeartbeat);
			}
			bool bIsDataReadable = true;
			if (m_connection->canRead(&bIsDataReadable, 1) < 0)
			{
				_log.Log(LOG_ERROR, "Domoticz: TCP/IP connection closed!, retrying in %d seconds...", RETRY_DELAY);
				m_retrycntr = 0;
				continue;
			}
			if (bIsDataReadable)
			{
				int bytesReceived = m_connection->read(buf, sizeof(buf), false);
				if (bytesReceived > 0)
				{
					std::lock_guard<std::mutex> l(readQueueMutex);
					onInternalMessage((const unsigned char *)&buf, bytesReceived, false); // Do not check validity, this might be non RFX-message
				}
			}
			if (IsStopRequested(10))
				break;
		}
	}
	m_connection->close();
	_log.Log(LOG_STATUS, "Domoticz: TCP/IP Worker stopped...");
}

bool DomoticzTCP::WriteToHardware(const char *pdata, const unsigned char length)
{
#ifndef NOCLOUD
	if (b_useProxy) {
		if (isConnectedProxy()) {
			writeProxy(pdata, length);
			return true;
		}
	}
	else {
		if (m_connection->getState() == csocket::CONNECTED)
		{
			m_connection->write(pdata, length);
			return true;
		}
	}
#else
	if (m_connection->getState() == csocket::CONNECTED)
	{
		m_connection->write(pdata, length);
		return true;
	}
#endif
	return false;
}

bool DomoticzTCP::isConnected()
{
#ifndef NOCLOUD
	if (b_useProxy) {
		return isConnectedProxy();
	}
	else {
		return (m_connection->getState() == csocket::CONNECTED);
	}
#else
	return (m_connection->getState() == csocket::CONNECTED);
#endif
}

#ifndef NOCLOUD
bool DomoticzTCP::CompareToken(const std::string &aToken)
{
	return (aToken == token);
}

bool DomoticzTCP::CompareId(const std::string &instanceid)
{
	return (m_szIPAddress == instanceid);
}

bool DomoticzTCP::StartHardwareProxy()
{
	if (m_bIsStarted) {
		return false; // dont start twice
	}
	m_bIsStarted = true;
	return ConnectInternalProxy();
}

bool DomoticzTCP::ConnectInternalProxy()
{
	std::shared_ptr<http::server::CProxyClient> proxy;
	const int version = 1;
	// we temporarily use the instance id as an identifier for this connection, meanwhile we get a token from the proxy
	// this means that we connect connect twice to the same server
	token = m_szIPAddress;
	proxy = m_webservers.GetProxyForMaster(this);
	if (proxy) {
		proxy->ConnectToDomoticz(m_szIPAddress, m_username, m_password, this, version);
		sOnConnected(this); // we do need this?
	}
	else {
		_log.Log(LOG_STATUS, "Delaying Domoticz master login");
	}
	return true;
}

bool DomoticzTCP::StopHardwareProxy()
{
	DisconnectProxy();
	m_bIsStarted = false;
	// Avoid dangling pointer if this hardware is removed.
	m_webservers.RemoveMaster(this);
	return true;
}

void DomoticzTCP::DisconnectProxy()
{
	std::shared_ptr<http::server::CProxyClient> proxy;

	proxy = m_webservers.GetProxyForMaster(this);
	if (proxy) {
		proxy->DisconnectFromDomoticz(token, this);
	}
	b_ProxyConnected = false;
}

bool DomoticzTCP::isConnectedProxy()
{
	return b_ProxyConnected;
}

void DomoticzTCP::writeProxy(const char *data, size_t size)
{
	/* send data to slave */
	if (isConnectedProxy()) {
		std::shared_ptr<http::server::CProxyClient> proxy = m_webservers.GetProxyForMaster(this);
		if (proxy) {
			proxy->WriteMasterData(token, data, size);
		}
	}
}

void DomoticzTCP::FromProxy(const unsigned char *data, size_t datalen)
{
	/* data received from slave */
	std::lock_guard<std::mutex> l(readQueueMutex);
	onInternalMessage(data, datalen);
}

std::string DomoticzTCP::GetToken()
{
	return token;
}

void DomoticzTCP::Authenticated(const std::string &aToken, bool authenticated)
{
	b_ProxyConnected = authenticated;
	token = aToken;
	if (authenticated) {
		_log.Log(LOG_STATUS, "Domoticz TCP connected via Proxy.");
	}
}

void DomoticzTCP::SetConnected(bool connected)
{
	if (connected) {
		ConnectInternalProxy();
	}
	else {
		b_ProxyConnected = false;
	}
}
#endif

