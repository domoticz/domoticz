#pragma once

#include <iosfwd>
#include "DomoticzHardware.h"
#if defined WIN32
#include "ws2tcpip.h"
#endif
#ifndef NOCLOUD
#include "../webserver/proxyclient.h"
#endif

class DomoticzTCP: public CDomoticzHardwareBase
{
public:
	DomoticzTCP(const int ID, const std::string &IPAddress, const unsigned short usIPPort, const std::string &username, const std::string &password);
	~DomoticzTCP(void);

	void write(const char *data, size_t size);
	bool isConnected();
	bool WriteToHardware(const char *pdata, const unsigned char length);
#ifndef NOCLOUD
	void SetConnected(bool connected);
	bool CompareToken(const std::string &aToken);
	bool CompareId(const std::string &instanceid);
	void FromProxy(const unsigned char *data, size_t datalen);
	std::string GetToken();
	void Authenticated(const std::string &aToken, bool authenticated);
	bool StartHardwareProxy();
	bool ConnectInternalProxy();
	void DisconnectProxy();
#endif
public:
	// signals
	boost::signals2::signal<void()>	sDisconnected;
private:
	int m_retrycntr;
	bool StartHardware();
	bool StopHardware();
	void writeTCP(const char *data, size_t size);
#ifndef NOCLOUD
	void writeProxy(const char *data, size_t size);
	bool isConnectedProxy();
	void disconnectProxy();
	bool IsValidAPIKey(const std::string &IPAddress);
	std::string token;
	bool b_ProxyConnected;
#endif
	bool isConnectedTCP();
	void disconnectTCP();
protected:
	bool StartHardwareTCP();
	bool StopHardwareTCP();
#ifndef NOCLOUD
	bool StopHardwareProxy();
	bool b_useProxy;
#endif
	std::string m_szIPAddress;
	unsigned short m_usIPPort;
	std::string m_username;
	std::string m_password;

	void Do_Work();
	bool ConnectInternal();
	void disconnect();
	boost::shared_ptr<boost::thread> m_thread;
	volatile bool m_stoprequested;
	sockaddr_in6 m_addr;
	struct addrinfo *info;
	int m_socket;
	unsigned char mBuffer[512];
};
