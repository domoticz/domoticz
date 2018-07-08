#pragma once

#include "DomoticzHardware.h"

class CWunderground : public CDomoticzHardwareBase
{
public:
	CWunderground(const int ID, const std::string &APIKey, const std::string &Location);
	~CWunderground(void);
	bool WriteToHardware(const char *pdata, const unsigned char length) override;
	std::string GetForecastURL();
private:
	void Init();
	bool StartHardware() override;
	bool StopHardware() override;
	void Do_Work();
	void GetMeterDetails();
private:
	bool m_bForceSingleStation;
	bool m_bFirstTime;
	std::string m_APIKey;
	std::string m_Location;
	volatile bool m_stoprequested;
	std::shared_ptr<std::thread> m_thread;
};

