#pragma once

#include "DomoticzHardware.h"

class CETH8020 : public CDomoticzHardwareBase
{
public:
	CETH8020(const int ID, const std::string &IPAddress, const unsigned short usIPPort, const std::string &username, const std::string &password);
	~CETH8020(void);
	bool WriteToHardware(const char *pdata, const unsigned char length) override;
private:
	void Init();
	bool StartHardware() override;
	bool StopHardware() override;
	void Do_Work();
	void GetMeterDetails();
	void UpdateSwitch(const unsigned char Idx, const uint8_t SubUnit, const bool bOn, const double Level, const std::string &defaultname);
private:
	std::string m_szIPAddress;
	unsigned short m_usIPPort;
	std::string m_Username;
	std::string m_Password;
	volatile bool m_stoprequested;
	std::shared_ptr<std::thread> m_thread;
};

