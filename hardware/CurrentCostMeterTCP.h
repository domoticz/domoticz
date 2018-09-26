#pragma once

#include "CurrentCostMeterBase.h"

class CurrentCostMeterTCP: public CurrentCostMeterBase
{
public:
	CurrentCostMeterTCP(const int ID, const std::string &IPAddress, const unsigned short usIPPort);
	virtual ~CurrentCostMeterTCP(void);

	virtual bool WriteToHardware(const char *pdata, const unsigned char length) override;

protected:
	virtual bool StartHardware() override;
	virtual bool StopHardware() override;

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
	int m_socket;
};

