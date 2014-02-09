#pragma once

#include "SolarEdgeBase.h"

namespace tcp_proxy
{
	class acceptor;
}

class SolarEdgeTCP: public SolarEdgeBase
{
public:
	SolarEdgeTCP(const int ID, const unsigned short usListenPort, const std::string &DestHost, const unsigned short usDestPort);
	~SolarEdgeTCP(void);

	void write(const char *data, size_t size);
	void WriteToHardware(const char *pdata, const unsigned char length);
public:
	// signals
	boost::signals2::signal<void()>	sDisconnected;
private:
	int m_retrycntr;
	bool StartHardware();
	bool StopHardware();
	void disconnect();
	void OnUpstreamData(const unsigned char *pData, const size_t Len);
	void OnDownstreamData(const unsigned char *pData, const size_t Len);

protected:
	std::string m_szSourceHost;
	unsigned short m_usSourcePort;
	std::string m_szDestHost;
	std::string m_szDestPort;
	tcp_proxy::acceptor *m_pTCPProxy;

	void Do_Work();
	bool ConnectInternal();
	boost::shared_ptr<boost::thread> m_thread;
	volatile bool m_stoprequested;
};

