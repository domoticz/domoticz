#pragma once

#include <deque>
#include <iosfwd>
#include "ASyncTCP.h"
#include "RFLinkBase.h"

class CRFLinkTCP: public CRFLinkBase, ASyncTCP
{
public:
	CRFLinkTCP(const int ID, const std::string &IPAddress, const unsigned short usIPPort);
	~CRFLinkTCP(void);
	bool isConnected(){ return mIsConnected; };
public:
	// signals
	boost::signals2::signal<void()>	sDisconnected;
private:
	int m_retrycntr;
	bool StartHardware();
	bool StopHardware();
	bool WriteInt(const std::string &sendString);
protected:
	std::string m_szIPAddress;
	unsigned short m_usIPPort;
	bool m_bDoRestart;

	void Do_Work();
	void OnConnect();
	void OnDisconnect();
	void OnData(const unsigned char *pData, size_t length);
	void OnError(const std::exception e);
	void OnError(const boost::system::error_code& error);

	boost::shared_ptr<boost::thread> m_thread;
	volatile bool m_stoprequested;
};

