#pragma once

#include "ASyncTCP.h"
#include "TeleinfoBase.h"

class CTeleinfoTCP : public CTeleinfoBase, ASyncTCP
{
      public:
	CTeleinfoTCP(int ID, const std::string &IPAddress, unsigned short usIPPort, int datatimeout, bool disable_crc, int ratelimit);
	~CTeleinfoTCP() override = default;
	bool WriteToHardware(const char *pdata, unsigned char length) override;
	boost::signals2::signal<void()> sDisconnected;

      private:
	bool StartHardware() override;
	bool StopHardware() override;
	void Init();

	void Do_Work();

	void OnConnect() override;
	void OnDisconnect() override;
	void OnData(const unsigned char *pData, size_t length) override;
	void OnError(const boost::system::error_code &error) override;

	std::string m_szIPAddress;
	unsigned short m_usIPPort;
	std::shared_ptr<std::thread> m_thread;
};
