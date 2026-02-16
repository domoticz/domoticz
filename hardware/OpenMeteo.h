#pragma once

#include "DomoticzHardware.h"

class COpenMeteo : public CDomoticzHardwareBase
{
      public:
	COpenMeteo(int ID);
	~COpenMeteo() override = default;
	bool WriteToHardware(const char *pdata, unsigned char length) override;

      private:
	void Init();
	bool StartHardware() override;
	bool StopHardware() override;
	void Do_Work();
	void GetMeterDetails();

	std::string m_URL;
	double m_Lat = 0;
	double m_Lon = 0;
	std::shared_ptr<std::thread> m_thread;
};
