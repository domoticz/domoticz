#pragma once

#include <deque>
#include <iostream>
#include "ASyncTCP.h"
#include "OTGWBase.h"

class OTGWTCP: public OTGWBase, ASyncTCP
{
public:
	OTGWTCP(const int ID, const std::string IPAddress, const unsigned short usIPPort, const int Mode1, const int Mode2, const int Mode3, const int Mode4, const int Mode5);
	~OTGWTCP(void);
	bool isConnected(){ return mIsConnected; };
	void WriteToHardware(const char *pdata, const unsigned char length);
public:
	// signals
	boost::signals2::signal<void()>	sDisconnected;
private:
	int m_retrycntr;
	bool StartHardware();
	bool StopHardware();
	void GetGatewayDetails();
	void SendOutsideTemperature();
protected:
	std::string m_szIPAddress;
	unsigned short m_usIPPort;

	void Do_Work();
	void OnConnect();
	void OnDisconnect();
	void OnData(const unsigned char *pData, size_t length);
	void OnError(const std::exception e);
	void OnError(const boost::system::error_code& error);

	boost::shared_ptr<boost::thread> m_thread;
	volatile bool m_stoprequested;
};

