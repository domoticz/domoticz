#pragma once

#include "ASyncTCP.h"
#include "RFXBase.h"

class RFXComTCP : public CRFXBase, ASyncTCP
{
public:
	RFXComTCP(const int ID, const std::string &IPAddress, const unsigned short usIPPort);
	~RFXComTCP(void);
	bool WriteToHardware(const char *pdata, const unsigned char length) override;
private:
	bool StartHardware() override;
	bool StopHardware() override;
	void OnConnect();
	void OnDisconnect();
	void OnData(const unsigned char *pData, size_t length);
	void OnErrorStd(const std::exception e);
	void OnErrorBoost(const boost::system::error_code& error);
	void Do_Work();
private:
	std::string m_szIPAddress;
	unsigned short m_usIPPort;
};

