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
	void OnConnect() override;
	void OnDisconnect() override;
	void OnData(const unsigned char *pData, size_t length) override;
	void OnError(const std::exception e) override;
	void OnError(const boost::system::error_code& error) override;
	void ParseData(const unsigned char *pData, int Len);
	void ParseLine();
private:
	int m_retrycntr;
	std::string m_szIPAddress;
	unsigned short m_usIPPort;
	bool m_bDoRestart;
	int m_bufferpos;
	std::shared_ptr<std::thread> m_thread;
	volatile bool m_stoprequested;
};

