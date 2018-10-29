#pragma once

#include "ASyncTCP.h"
#include "S0MeterBase.h"

class S0MeterTCP: public S0MeterBase, ASyncTCP
{
public:
	S0MeterTCP(const int ID, const std::string &IPAddress, const unsigned short usIPPort);
	~S0MeterTCP(void);
	bool WriteToHardware(const char *pdata, const unsigned char length) override;
	boost::signals2::signal<void()>	sDisconnected;
private:
	bool StartHardware() override;
	bool StopHardware() override;
	bool WriteInt(const std::string &sendString);
	void Do_Work();

	void OnConnect() override;
	void OnDisconnect() override;
	void OnData(const unsigned char *pData, size_t length) override;
	void OnError(const std::exception e) override;
	void OnError(const boost::system::error_code& error) override;
private:
	int m_retrycntr;
	std::string m_szIPAddress;
	unsigned short m_usIPPort;
	std::shared_ptr<std::thread> m_thread;
};

