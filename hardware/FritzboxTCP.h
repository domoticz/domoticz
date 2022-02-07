#pragma once

#include "ASyncTCP.h"
#include "DomoticzHardware.h"

class FritzboxTCP : public CDomoticzHardwareBase, ASyncTCP
{
      public:
	FritzboxTCP(int ID, const std::string &IPAddress, unsigned short usIPPort);
	~FritzboxTCP() override = default;
	bool WriteToHardware(const char *pdata, unsigned char length) override;
	boost::signals2::signal<void()> sDisconnected;

      private:
	bool StartHardware() override;
	bool StopHardware() override;
	void UpdateSwitch(unsigned char Idx, uint8_t SubUnit, bool bOn, double Level, const std::string &defaultname);
	void WriteInt(const std::string &sendStr);
	void Do_Work();
	void ParseData(const unsigned char *pData, int Len);
	void ParseLine();

	void OnConnect() override;
	void OnDisconnect() override;
	void OnData(const unsigned char *pData, size_t length) override;
	void OnError(const boost::system::error_code &error) override;

private:
	void GetStatistics();
	bool GetSoapData(const std::string& endpoint, const std::string& urn, const std::string& Action, std::string &Response);
	std::string BuildSoapXML(const std::string& urn, const std::string& Action);
	bool GetValueFromXML(const std::string& xml, const std::string& key, std::string& value);
	int m_retrycntr;
	std::string m_szIPAddress;
	unsigned short m_usIPPort;
	int m_bufferpos;
	std::shared_ptr<std::thread> m_thread;
	unsigned char m_buffer[1024];
};
