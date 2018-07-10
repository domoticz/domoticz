#pragma once

#include "P1MeterBase.h"

class P1MeterTCP: public P1MeterBase
{
public:
	P1MeterTCP(const int ID, const std::string &IPAddress, const unsigned short usIPPort, const bool disable_crc, const int ratelimit);
	~P1MeterTCP(void);
	bool WriteToHardware(const char *pdata, const unsigned char length) override;
	boost::signals2::signal<void()>	sDisconnected;
private:
	void write(const char *data, size_t size);
	bool isConnected() { return m_socket != INVALID_SOCKET; };
	bool StartHardware() override;
	bool StopHardware() override;
	void disconnect();
	void Do_Work();
	bool ConnectInternal();
private:
	int m_retrycntr;
	int m_ratelimit;
	std::string m_szIPAddress;
	unsigned short m_usIPPort;

	std::shared_ptr<std::thread> m_thread;
	volatile bool m_stoprequested;
	sockaddr_in m_addr;
	int m_socket;
};
