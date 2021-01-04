#pragma once

#include "ASyncTCP.h"
#include "OTGWBase.h"

class OTGWTCP : public OTGWBase, ASyncTCP
{
      public:
	OTGWTCP(int ID, const std::string &IPAddress, unsigned short usIPPort, int Mode1, int Mode2, int Mode3, int Mode4, int Mode5, int Mode6);
	~OTGWTCP() override = default;

      public:
	// signals
	boost::signals2::signal<void()> sDisconnected;

      private:
	int m_retrycntr;
	bool StartHardware() override;
	bool StopHardware() override;
	bool WriteInt(const unsigned char *pData, unsigned char Len) override;

      protected:
	std::string m_szIPAddress;
	unsigned short m_usIPPort;

	void Do_Work();

	void OnConnect() override;
	void OnDisconnect() override;
	void OnData(const unsigned char *pData, size_t length) override;
	void OnError(const boost::system::error_code &error) override;

	std::shared_ptr<std::thread> m_thread;
};
