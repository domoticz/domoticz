#pragma once

#include "DomoticzHardware.h"

class CMeteorologisk : public CDomoticzHardwareBase
{
      public:
	CMeteorologisk(int ID, const std::string &Location);
	~CMeteorologisk() override = default;
	bool WriteToHardware(const char *pdata, unsigned char length) override;
	std::string GetForecastURL();

      private:
	void Init();
	bool StartHardware() override;
	bool StopHardware() override;
	void Do_Work();
	void GetMeterDetails();

	std::string m_Location;
	std::string m_URL;
	std::string m_ForecastURL;
	double m_Lat = 0;
	double m_Lon = 0;
	std::shared_ptr<std::thread> m_thread;
};
