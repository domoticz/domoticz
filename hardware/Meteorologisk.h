#pragma once

#include "DomoticzHardware.h"

class CMeteorologisk : public CDomoticzHardwareBase
{
public:
	CMeteorologisk(const int ID, const std::string &Location);
	~CMeteorologisk(void);
	bool WriteToHardware(const char *pdata, const unsigned char length) override;
	std::string GetForecastURL();
private:
	void Init();
	bool StartHardware() override;
	bool StopHardware() override;
	void Do_Work();
	void GetMeterDetails();
private:
	std::string m_Location;
	std::shared_ptr<std::thread> m_thread;
};

