#pragma once

#include <deque>
#include <iostream>
#include "DomoticzHardware.h"

class RFXComTCP: public CDomoticzHardwareBase
{
public:
	RFXComTCP(const int ID, const std::string IPAddress, const unsigned short usIPPort);
	~RFXComTCP(void);

	void write(const char *data, size_t size);
	bool isConnected(){ return m_socket!= INVALID_SOCKET; };
	void WriteToHardware(const char *pdata, const unsigned char length);
public:
	// signals
	boost::signals2::signal<void()>	sDisconnected;
	static const int readBufferSize=512;
private:
	void disconnect();
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
	unsigned char mBuffer[readBufferSize];
};

