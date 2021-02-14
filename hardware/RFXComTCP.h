#pragma once

#include "ASyncTCP.h"
#include "RFXBase.h"

class RFXComTCP : public CRFXBase, ASyncTCP
{
      public:
	RFXComTCP(int ID, const std::string &IPAddress, unsigned short usIPPort, _eRFXAsyncType AsyncType);
	~RFXComTCP() override = default;
	bool WriteToHardware(const char *pdata, unsigned char length) override;

      private:
	bool StartHardware() override;
	bool StopHardware() override;
	void Do_Work();

	void OnConnect() override;
	void OnDisconnect() override;
	void OnData(const unsigned char *pData, size_t length) override;
	void OnError(const boost::system::error_code &error) override;

      private:
	std::string m_szIPAddress;
	unsigned short m_usIPPort;
};
