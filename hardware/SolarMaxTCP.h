#pragma once

#include "DomoticzHardware.h"

class SolarMaxTCP : public CDomoticzHardwareBase
{
      public:
	SolarMaxTCP(int ID, const std::string &IPAddress, unsigned short usIPPort);
	~SolarMaxTCP() override = default;
	bool WriteToHardware(const char *pdata, unsigned char length) override;
	boost::signals2::signal<void()> sDisconnected;

      private:
	bool StartHardware() override;
	bool StopHardware() override;

      protected:
	void write(const char *data, size_t size);
	bool ConnectInternal();
	void disconnect();
	bool isConnected()
	{
		return m_socket != INVALID_SOCKET;
	};
	std::string MakeRequestString();
	void Do_Work();
	void ParseData(const unsigned char *pData, int Len);
	void ParseLine();

	int m_retrycntr;
	std::string m_szIPAddress;
	unsigned short m_usIPPort;
	bool m_bDoRestart;

	int m_bufferpos;
	std::shared_ptr<std::thread> m_thread;
	sockaddr_in m_addr;
	SOCKET m_socket;
	std::string m_endpoint;
	unsigned char m_buffer[512];
};
