#pragma once

#include "DomoticzHardware.h"

class COpenMeteo : public CDomoticzHardwareBase
{
      public:
	COpenMeteo(int ID);
	~COpenMeteo() override = default;
	bool WriteToHardware(const char *pdata, unsigned char length) override;
	std::string GetForecastURL();

      private:
	void Init();
	bool StartHardware() override;
	bool StopHardware() override;
	void Do_Work();
	void GetMeterDetails();

	std::string m_URL;
	std::string m_ForecastURL;
	double m_Lat = 0;
	double m_Lon = 0;
	bool m_bDevicesUsed = false;
	std::shared_ptr<std::thread> m_thread;
};
