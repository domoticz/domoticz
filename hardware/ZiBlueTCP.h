#pragma once

#include "ASyncTCP.h"
#include "ZiBlueBase.h"

class CZiBlueTCP : public CZiBlueBase, ASyncTCP
{
      public:
	CZiBlueTCP(int ID, const std::string &IPAddress, unsigned short usIPPort);
	~CZiBlueTCP() override = default;

      public:
	// signals
	boost::signals2::signal<void()> sDisconnected;

      private:
	int m_retrycntr;
	bool StartHardware() override;
	bool StopHardware() override;
	bool WriteInt(const std::string &sendString) override;
	bool WriteInt(const uint8_t *pData, size_t length) override;

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
