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
	void GetRainPrediction();
private:
	int m_iNearestStationID = -1;
	std::string m_szMyLatitude;
	std::string m_szMyLongitude;
	std::shared_ptr<std::thread> m_thread;
};

