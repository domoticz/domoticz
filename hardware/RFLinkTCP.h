#pragma once

#include "ASyncTCP.h"
#include "RFLinkBase.h"

class CRFLinkTCP : public CRFLinkBase, ASyncTCP
{
      public:
	CRFLinkTCP(int ID, const std::string &IPAddress, unsigned short usIPPort);
	~CRFLinkTCP() override = default;

      public:
	// signals
	boost::signals2::signal<void()> sDisconnected;

      private:
	bool StartHardware() override;
	bool StopHardware() override;
	bool WriteInt(const std::string &sendString) override;

      protected:
	std::string m_szIPAddress;
	unsigned short m_usIPPort;
	bool m_bDoRestart;

	void Do_Work();

	void OnConnect() override;
	void OnDisconnect() override;
	void OnData(const unsigned char *pData, size_t length) override;
	void OnError(const boost::system::error_code &error) override;

	std::shared_ptr<std::thread> m_thread;
};
