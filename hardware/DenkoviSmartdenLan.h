#pragma once

#include "DomoticzHardware.h"

class CDenkoviSmartdenLan : public CDomoticzHardwareBase
{
public:
	CDenkoviSmartdenLan(const int ID, const std::string &IPAddress, const unsigned short usIPPort, const std::string &password, const int pollInterval);
	~CDenkoviSmartdenLan(void);
	bool WriteToHardware(const char *pdata, const unsigned char length) override;
private:
	void Init();
	bool StartHardware() override;
	bool StopHardware() override;
	void Do_Work();
	void GetMeterDetails();
	void UpdateSwitch(const unsigned char Idx, const int SubUnit, const bool bOn, const double Level, const std::string &defaultname);
private:
	std::string m_szIPAddress;
	unsigned short m_usIPPort;
	std::string m_Password;
	int m_pollInterval;
	volatile bool m_stoprequested;
	std::shared_ptr<std::thread> m_thread;
};

