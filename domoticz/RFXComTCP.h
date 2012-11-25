#pragma once

#include <deque>
#include <iostream>
#include "DomoticzDevice.h"

class RFXComTCP: public CDomoticzDeviceBase
{
public:
	RFXComTCP(const int ID, const std::string IPAddress, unsigned short usIPPort);
	~RFXComTCP(void);

	bool Start();
	bool Stop();

	virtual bool connectto(const char *serveraddr, unsigned short port);
	virtual void disconnect();
	void write(const char *data, size_t size);
	bool isConnected(){ return m_socket!= INVALID_SOCKET; };
	void WriteToDevice(const char *pdata, const unsigned char length);
public:
	// signals
	boost::signals2::signal<void()>	sDisconnected;
	static const int readBufferSize=512;
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
	boost::mutex readQueueMutex;
};

