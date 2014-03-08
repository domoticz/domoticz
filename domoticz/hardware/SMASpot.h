#pragma once

#include "DomoticzHardware.h"
#include <iostream>

class CSMASpot : public CDomoticzHardwareBase
{
public:
	CSMASpot(const int ID, const std::string SMAConfigFile);
	~CSMASpot(void);
	void WriteToHardware(const char *pdata, const unsigned char length);
private:
	void SendMeter(const unsigned char ID1,const unsigned char ID2, const double musage, const double mtotal, const std::string &defaultname);
	bool GetMeter(const unsigned char ID1,const unsigned char ID2, double &musage, double &mtotal);
	void SendVoltage(const unsigned long Idx, const float Volt, const std::string &defaultname);
	void SendPercentage(const unsigned long Idx, const float Percentage, const std::string &defaultname);

	std::string m_SMAConfigFile;
	std::string m_SMADataPath;
	std::string m_SMAPlantName;
	std::string m_SMADateFormat;
	std::string m_SMATimeFormat;

	std::string m_LastDateTime;
	volatile bool m_stoprequested;
	boost::shared_ptr<boost::thread> m_thread;

	void Init();
	bool StartHardware();
	bool StopHardware();
	void Do_Work();
	void GetMeterDetails();
};

