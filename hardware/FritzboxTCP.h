#pragma once

#include "ASyncTCP.h"
#include "DomoticzHardware.h"

class FritzboxTCP : public CDomoticzHardwareBase, ASyncTCP
{
public:
	FritzboxTCP(const int ID, const std::string &IPAddress, const unsigned short usIPPort);
	~FritzboxTCP(void);
	bool WriteToHardware(const char *pdata, const unsigned char length) override;
	boost::signals2::signal<void()>	sDisconnected;
private:
	bool isConnected() { return mIsConnected; };
	bool StartHardware() override;
	bool StopHardware() override;
	void UpdateSwitch(const unsigned char Idx, const uint8_t SubUnit, const bool bOn, const double Level, const std::string &defaultname);
	void WriteInt(const std::string &sendStr);
	void Do_Work();
	void OnConnect();
	void OnDisconnect();
	void OnData(const unsigned char *pData, size_t length);
	void OnErrorStd(const std::exception e);
	void OnErrorBoost(const boost::system::error_code& error);
	void ParseData(const unsigned char *pData, int Len);
	void ParseLine();
private:
	int m_retrycntr;
	std::string m_szIPAddress;
	unsigned short m_usIPPort;
	bool m_bDoRestart;
	int m_bufferpos;
	std::shared_ptr<std::thread> m_thread;
};

