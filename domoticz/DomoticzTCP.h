#pragma once

#include <deque>
#include <iostream>
#include "DomoticzHardware.h"

class DomoticzTCP: public CDomoticzHardwareBase
{
public:
	DomoticzTCP(const int ID, const std::string IPAddress, const unsigned short usIPPort, const std::string username, const std::string password);
	~DomoticzTCP(void);

	virtual bool connectto(const char *serveraddr, unsigned short port);
	virtual void disconnect();
	void write(const char *data, size_t size);
	bool isConnected(){ return m_socket!= INVALID_SOCKET; };
	void WriteToHardware(const char *pdata, const unsigned char length);
public:
	// signals
	boost::signals2::signal<void()>	sDisconnected;
	static const int readBufferSize=512;
private:
	bool StartHardware();
	bool StopHardware();
protected:
	std::string m_szIPAddress;
	unsigned short m_usIPPort;
	std::string m_username;
	std::string m_password;

	void Do_Work();
	bool ConnectInternal();
	boost::shared_ptr<boost::thread> m_thread;
	volatile bool m_stoprequested;
	sockaddr_in m_addr;
	int m_socket;
	unsigned char mBuffer[readBufferSize];
};

