#pragma once

#include <iosfwd>
#include "DomoticzHardware.h"

class SolarMaxTCP : public CDomoticzHardwareBase
{
public:
	SolarMaxTCP(const int ID, const std::string &IPAddress, const unsigned short usIPPort);
	~SolarMaxTCP(void);
	bool WriteToHardware(const char *pdata, const unsigned char length);
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
	bool m_bDoRestart;

	void write(const char *data, size_t size);
	bool ConnectInternal();
	void disconnect();
	bool isConnected(){ return m_socket != INVALID_SOCKET; };

	std::string MakeRequestString();

	void Do_Work();

	void ParseData(const unsigned char *pData, int Len);
	void ParseLine();

	int m_bufferpos;
	boost::shared_ptr<boost::thread> m_thread;
	volatile bool m_stoprequested;
	sockaddr_in m_addr;
	int m_socket;
	std::string m_endpoint;
	unsigned char m_buffer[512];
};

