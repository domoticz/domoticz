#pragma once

#include "DomoticzHardware.h"

class TiXmlDocument;

// Enigma2 satellite receiver (OpenWebif / WebInterface over HTTP).
// Unit 1: Status (Media Player), Unit 2: Source (selector Off|Standby|Reboot|RestartE2|On).
// Devices are found by unit, so the hardware keeps working with devices it did not create.
class CEnigma2 : public CDomoticzHardwareBase
{
      public:
	CEnigma2(int ID, const std::string &IPAddress, unsigned short usIPPort, const std::string &username, const std::string &password, int pollInterval, int deepStandby,
		 int channelName);
	~CEnigma2() override = default;

	bool WriteToHardware(const char *pdata, unsigned char length) override;

      private:
	bool StartHardware() override;
	bool StopHardware() override;
	void Do_Work();
	void CreateDevices();
	void LogBoxInfo();
	void Poll();
	std::string GetChannelName();
	bool GetXML(const std::string &endpoint, long timeoutSec, TiXmlDocument &doc);
	bool SendPowerState(int newState);
	bool IsPinging();
	void UpdateUnit(int unit, int nValue, const std::string &sValue);

      private:
	std::string m_szIPAddress;
	unsigned short m_usIPPort;
	std::vector<std::string> m_ExtraHeaders;
	int m_iPollInterval;
	bool m_bDeepStandby;
	bool m_bChannelName;
	bool m_bInfoLogged = false;
	bool m_bWebIfDown = false;
	std::string m_szLastChannel;
	std::atomic<int> m_iSecondsToPoll{ 2 };
	std::shared_ptr<std::thread> m_thread;
};
