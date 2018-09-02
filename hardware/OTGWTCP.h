#pragma once

#include "ASyncTCP.h"
#include "OTGWBase.h"

class OTGWTCP: public OTGWBase, ASyncTCP
{
public:
	OTGWTCP(const int ID, const std::string &IPAddress, const unsigned short usIPPort, const int Mode1, const int Mode2, const int Mode3, const int Mode4, const int Mode5, const int Mode6);
	~OTGWTCP(void);
	bool isConnected(){ return mIsConnected; };
public:
	// signals
	boost::signals2::signal<void()>	sDisconnected;
private:
	int m_retrycntr;
	bool StartHardware() override;
	bool StopHardware() override;
	bool WriteInt(const unsigned char *pData, const unsigned char Len) override;
protected:
	std::string m_szIPAddress;
	unsigned short m_usIPPort;
	bool m_bDoRestart;

	void Do_Work();

	void OnConnect();
	void OnDisconnect();
	void OnData(const unsigned char *pData, size_t length);
	void OnErrorStd(const std::exception e);
	void OnErrorBoost(const boost::system::error_code& error);

	std::shared_ptr<std::thread> m_thread;
};

