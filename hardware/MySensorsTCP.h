#pragma once

#include "ASyncTCP.h"
#include "MySensorsBase.h"

class MySensorsTCP : public MySensorsBase, ASyncTCP
{
public:
	MySensorsTCP(const int ID, const std::string &IPAddress, const unsigned short usIPPort);
	~MySensorsTCP(void);
	bool isConnected() { return mIsConnected; };
public:
	// signals
	boost::signals2::signal<void()>	sDisconnected;
private:
	int m_retrycntr;
	bool StartHardware() override;
	bool StopHardware() override;
protected:
	std::string m_szIPAddress;
	unsigned short m_usIPPort;
	bool m_bDoRestart;

	void WriteInt(const std::string &sendStr) override;

	void Do_Work();

	void OnConnect();
	void OnDisconnect();
	void OnData(const unsigned char *pData, size_t length);
	void OnErrorStd(const std::exception e);
	void OnErrorBoost(const boost::system::error_code& error);

	std::shared_ptr<std::thread> m_thread;
};

