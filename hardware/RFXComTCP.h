#pragma once

#include "ASyncTCP.h"
#include "RFXBase.h"
#include <boost/asio/steady_timer.hpp>

class RFXComTCP : public CRFXBase, ASyncTCP
{
public:
	RFXComTCP(const int ID, const std::string &IPAddress, const unsigned short usIPPort);
	~RFXComTCP(void);
	bool WriteToHardware(const char *pdata, const unsigned char length) override;
private:
	bool StartHardware() override;
	bool StopHardware() override;
	void Do_Work(const boost::system::error_code& error);

	void OnConnect() override;
	void OnDisconnect() override;
	void OnData(const unsigned char *pData, size_t length) override;
	void OnError(const std::exception e) override;
	void OnError(const boost::system::error_code& error) override;
private:
	std::string m_szIPAddress;
	unsigned short m_usIPPort;
	boost::asio::steady_timer m_heartbeat_timer;
};

