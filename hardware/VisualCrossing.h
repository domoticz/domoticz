#pragma once

#include "DomoticzHardware.h"

class CVisualCrossing : public CDomoticzHardwareBase
{
      public:
	CVisualCrossing(int ID, const std::string &APIKey, const std::string &Location);
	~CVisualCrossing() override = default;
	bool WriteToHardware(const char *pdata, unsigned char length) override;
	std::string GetForecastURL();

      private:
	void Init();
	bool StartHardware() override;
	bool StopHardware() override;
	void Do_Work();
	void GetMeterDetails();

      private:
	std::string m_APIKey;
	std::string m_Location;
	std::shared_ptr<std::thread> m_thread;
};
