#pragma once

#include <iosfwd>
#include "ASyncTCP.h"
#include "MySensorsBase.h"

class MySensorsTCP : public MySensorsBase, ASyncTCP
{
public:
	MySensorsTCP(const int ID, const std::string &IPAddress, const unsigned short usIPPort);
	~MySensorsTCP(void);
	bool isConnected(){ return mIsConnected; };
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

	void WriteInt(const std::string &sendStr);


	void Do_Work();
	void OnConnect();
	void OnDisconnect();
	void OnData(const unsigned char *pData, size_t length);
	void OnError(const std::exception e);
	void OnError(const boost::system::error_code& error);

	boost::shared_ptr<boost::thread> m_thread;
	volatile bool m_stoprequested;
};

