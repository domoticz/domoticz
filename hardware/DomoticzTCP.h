#pragma once

#include "DomoticzHardware.h"
#include "RFXBase.h"
#if defined WIN32
#include "ws2tcpip.h"
#endif

class csocket;
class DomoticzTCP : public CRFXBase
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
	bool StartHardware() override;
	bool StopHardware() override;
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
	csocket *m_connection;

#ifndef NOCLOUD
	std::string token;
	bool b_ProxyConnected;
	bool b_useProxy;
#endif
};
