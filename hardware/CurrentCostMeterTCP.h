#pragma once

#include "CurrentCostMeterBase.h"

class CurrentCostMeterTCP : public CurrentCostMeterBase
{
      public:
	CurrentCostMeterTCP(int ID, const std::string &IPAddress, unsigned short usIPPort);
	~CurrentCostMeterTCP() override = default;

	bool WriteToHardware(const char *pdata, unsigned char length) override;

      protected:
	bool StartHardware() override;
	bool StopHardware() override;

      private:
	void write(const char *data, size_t size);
	bool isConnected();
	void disconnect();
	void Do_Work();
	bool ConnectInternal();

	int m_retrycntr;
	std::string m_szIPAddress;
	unsigned short m_usIPPort;
	std::shared_ptr<std::thread> m_thread;
	sockaddr_in m_addr;
	SOCKET m_socket;
};
