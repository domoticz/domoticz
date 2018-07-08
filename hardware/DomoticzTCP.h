#pragma once

#include "DomoticzHardware.h"
#if defined WIN32
#include "ws2tcpip.h"
#endif

class DomoticzTCP : public CDomoticzHardwareBase
{
public:
	DomoticzTCP(const int ID, const std::string &IPAddress, const unsigned short usIPPort, const std::string &username, const std::string &password);
	~DomoticzTCP(void);
	bool WriteToHardware(const char *pdata, const unsigned char length) override;
	boost::signals2::signal<void()>	sDisconnected;
#ifndef NOCLOUD
	void SetConnected(bool connected);
	bool isConnected();
	void Authenticated(const std::string &aToken, bool authenticated);
	void FromProxy(const unsigned char *data, size_t datalen);
	bool CompareToken(const std::string &aToken);
	bool CompareId(const std::string &instanceid);
	bool ConnectInternalProxy();
	void DisconnectProxy();
#endif
private:
	void write(const char *data, size_t size);
	bool isConnectedTCP();
	void disconnectTCP();
	bool StartHardwareTCP();
	bool StopHardwareTCP();
	bool StartHardware() override;
	bool StopHardware() override;
	void writeTCP(const char *data, size_t size);
	void Do_Work();
	bool ConnectInternal();
#ifndef NOCLOUD
	bool StartHardwareProxy();
	void writeProxy(const char *data, size_t size);
	bool isConnectedProxy();
	bool IsValidAPIKey(const std::string &IPAddress);
	bool StopHardwareProxy();
	std::string GetToken();
#endif
private:
	std::string m_szIPAddress;
	unsigned short m_usIPPort;
	std::string m_username;
	std::string m_password;
	int m_retrycntr;
	std::shared_ptr<std::thread> m_thread;
	volatile bool m_stoprequested;
	sockaddr_in6 m_addr;
	struct addrinfo *info;
	int m_socket;
	unsigned char mBuffer[512];
#ifndef NOCLOUD
	std::string token;
	bool b_ProxyConnected;
	bool b_useProxy;
#endif
};
