#pragma once

#include "DomoticzHardware.h"
#include "RFXBase.h"
#if defined WIN32
#include "ws2tcpip.h"
#endif
#include "ASyncTCP.h"

class DomoticzTCP : public CRFXBase, ASyncTCP
{
public:
	DomoticzTCP(int ID, const std::string &IPAddress, unsigned short usIPPort, const std::string &username, const std::string &password);
	~DomoticzTCP() override = default;
	bool WriteToHardware(const char *pdata, unsigned char length) override;
	boost::signals2::signal<void()> sDisconnected;
private:
	bool StartHardware() override;
	bool StopHardware() override;
	void Do_Work();

	std::string m_szIPAddress;
	unsigned short m_usIPPort;
	std::string m_username;
	std::string m_password;
	std::shared_ptr<std::thread> m_thread;
protected:
	void OnConnect() override;
	void OnDisconnect() override;
	void OnData(const unsigned char *pData, size_t length) override;
	void OnError(const boost::system::error_code &error) override;
};
