#pragma once

#include "DomoticzHardware.h"

class CBuienRadar : public CDomoticzHardwareBase
{
public:
	CBuienRadar(const int ID);
	~CBuienRadar(void);
	bool WriteToHardware(const char *pdata, const unsigned char length) override;
	std::string GetForecastURL();
private:
	void Init();
	bool StartHardware() override;
	bool StopHardware() override;
	void Do_Work();
	bool FindNearestStationID();
	void GetMeterDetails();
private:
	int m_iNearestStationID = -1;
	std::shared_ptr<std::thread> m_thread;
};

