#pragma once

#include "ASyncTCP.h"
#include "RFLinkBase.h"
#include <boost/asio/steady_timer.hpp>

class CRFLinkTCP: public CRFLinkBase, ASyncTCP
{
public:
	CRFLinkTCP(const int ID, const std::string &IPAddress, const unsigned short usIPPort);
	~CRFLinkTCP(void);
public:
	// signals
	boost::signals2::signal<void()>	sDisconnected;
private:
	bool StartHardware() override;
	bool StopHardware() override;
	bool WriteInt(const std::string &sendString) override;
protected:
	std::string m_szIPAddress;
	unsigned short m_usIPPort;

	void Do_Work(const boost::system::error_code& error);

	void OnConnect() override;
	void OnDisconnect() override;
	void OnData(const unsigned char *pData, size_t length) override;
	void OnError(const std::exception e) override;
	void OnError(const boost::system::error_code& error) override;
	boost::asio::steady_timer m_heartbeat_timer;
	int m_sec_counter;
};

