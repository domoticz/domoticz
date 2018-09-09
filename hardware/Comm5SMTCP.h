#pragma once

#include <deque>
#include <iostream>
#include "ASyncTCP.h"
#include "DomoticzHardware.h"
#include <boost/asio/steady_timer.hpp>

class Comm5SMTCP : public CDomoticzHardwareBase, ASyncTCP
{
public:
	Comm5SMTCP(const int ID, const std::string &IPAddress, const unsigned short usIPPort);
	bool WriteToHardware(const char *pdata, const unsigned char length) override;
	boost::signals2::signal<void()>	sDisconnected;
private:
	bool StartHardware() override;
	bool StopHardware() override;
protected:
	void OnConnect() override;
	void OnDisconnect() override;
	void OnData(const unsigned char *pData, size_t length) override;
	void OnError(const std::exception e) override;
	void OnError(const boost::system::error_code& error) override;

	void Do_Work(const boost::system::error_code& error);
	void ParseData(const unsigned char *data, const size_t len);
	void querySensorState();
private:
	std::string m_szIPAddress;
	unsigned short m_usIPPort;
	std::string buffer;
	bool initSensorData;
	bool m_bReceiverStarted;
	boost::asio::steady_timer m_heartbeat_timer;
	int m_sec_counter;
};

