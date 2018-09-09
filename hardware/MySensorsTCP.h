#pragma once

#include "ASyncTCP.h"
#include "MySensorsBase.h"
#include <boost/asio/steady_timer.hpp>

class MySensorsTCP : public MySensorsBase, ASyncTCP
{
public:
	MySensorsTCP(const int ID, const std::string &IPAddress, const unsigned short usIPPort);
	~MySensorsTCP(void);
public:
	// signals
	boost::signals2::signal<void()>	sDisconnected;
private:
	int m_retrycntr;
	bool StartHardware() override;
	bool StopHardware() override;
protected:
	std::string m_szIPAddress;
	unsigned short m_usIPPort;

	void WriteInt(const std::string &sendStr) override;

	void Do_Work(const boost::system::error_code& error);

	void OnConnect() override;
	void OnDisconnect() override;
	void OnData(const unsigned char *pData, size_t length) override;
	void OnError(const std::exception e) override;
	void OnError(const boost::system::error_code& error) override;

	boost::asio::steady_timer m_heartbeat_timer;
};

