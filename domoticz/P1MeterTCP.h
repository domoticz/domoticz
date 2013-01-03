#pragma once

#include <deque>
#include <iostream>
#include "P1MeterBase.h"

class P1MeterTCP: public P1MeterBase
{
public:
	P1MeterTCP(const int ID, const std::string IPAddress, const unsigned short usIPPort);
	~P1MeterTCP(void);

	virtual bool connectto(const char *serveraddr, unsigned short port);
	virtual void disconnect();
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

