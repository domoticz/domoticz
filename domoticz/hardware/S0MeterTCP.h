#pragma once

#include <deque>
#include <iostream>
#include "S0MeterBase.h"

class S0MeterTCP: public S0MeterBase
{
public:
	S0MeterTCP(const int ID, const std::string IPAddress, const unsigned short usIPPort, const int M1Type, const int M1PPH, const int M2Type, const int M2PPH);
	~S0MeterTCP(void);

	void write(const char *data, size_t size);
	bool isConnected(){ return m_socket!= INVALID_SOCKET; };
	void WriteToHardware(const char *pdata, const unsigned char length);
public:
	// signals
	boost::signals2::signal<void()>	sDisconnected;
private:
	int m_retrycntr;
	bool StartHardware();
	bool StopHardware();
	void disconnect();
protected:
	std::string m_szIPAddress;
	unsigned short m_usIPPort;

	void Do_Work();
	bool ConnectInternal();
	boost::shared_ptr<boost::thread> m_thread;
	volatile bool m_stoprequested;
	sockaddr_in m_addr;
	int m_socket;
};

