#pragma once

#include "ASyncTCP.h"
#include "P1MeterBase.h"

class P1MeterTCP : public P1MeterBase, ASyncTCP
{
public:
	P1MeterTCP(const int ID, const std::string &IPAddress, const unsigned short usIPPort, const bool disable_crc, const int ratelimit);
	~P1MeterTCP(void);
	bool WriteToHardware(const char *pdata, const unsigned char length) override;
public:
	// signals
	boost::signals2::signal<void()>	sDisconnected;

private:
	bool StartHardware() override;
	bool StopHardware() override;


protected:
	std::string m_szIPAddress;
	unsigned short m_usIPPort;

	void Do_Work();

	void OnConnect() override;
	void OnDisconnect() override;
	void OnData(const unsigned char *pData, size_t length) override;
	void OnError(const std::exception e) override;
	void OnError(const boost::system::error_code& error) override;

	std::shared_ptr<std::thread> m_thread;
};

