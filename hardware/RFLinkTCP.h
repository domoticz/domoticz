#pragma once

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
	bool StartHardware() override;
	bool StopHardware() override;
	bool WriteInt(const std::string &sendString) override;
protected:
	std::string m_szIPAddress;
	unsigned short m_usIPPort;
	bool m_bDoRestart;

	void Do_Work();
	void OnConnect() override;
	void OnDisconnect() override;
	void OnData(const unsigned char *pData, size_t length) override;
	void OnError(const std::exception e) override;
	void OnError(const boost::system::error_code& error) override;

	std::shared_ptr<std::thread> m_thread;
	volatile bool m_stoprequested;
};

