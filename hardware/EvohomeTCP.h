#pragma once

#include "ASyncTCP.h"
#include "EvohomeRadio.h"

class CEvohomeTCP : public ASyncTCP, public CEvohomeRadio
{
      public:
	CEvohomeTCP(int ID, const std::string &IPAddress, unsigned short usIPPort, const std::string &UserContID);

      private:
	bool StopHardware() override;
	void Do_Work() override;
	void Do_Send(std::string str) override;

	void OnConnect() override;
	void OnDisconnect() override;
	void OnData(const unsigned char *pData, size_t length) override;
	void OnError(const boost::system::error_code &error) override;

      private:
	std::string m_szIPAddress;
	unsigned short m_usIPPort;
};
