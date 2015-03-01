#pragma once

#include "DomoticzHardware.h"
#include <iostream>

class CSBFSpot : public CDomoticzHardwareBase
{
public:
	CSBFSpot(const int ID, const std::string SBFConfigFile);
	~CSBFSpot(void);
	bool WriteToHardware(const char *pdata, const unsigned char length);
	void ImportOldMonthData();
private:
	void SendMeter(const unsigned char ID1,const unsigned char ID2, const double musage, const double mtotal, const std::string &defaultname);
	bool GetMeter(const unsigned char ID1,const unsigned char ID2, double &musage, double &mtotal);
	void SendTempSensor(const unsigned char Idx, const float Temp, const std::string &defaultname);
	void SendVoltage(const unsigned long Idx, const float Volt, const std::string &defaultname);
	void SendPercentage(const unsigned long Idx, const float Percentage, const std::string &defaultname);

	std::string m_SBFConfigFile;
	std::string m_SBFDataPath;
	std::string m_SBFPlantName;
	std::string m_SBFDateFormat;
	std::string m_SBFTimeFormat;

	std::string m_LastDateTime;
	volatile bool m_stoprequested;
	boost::shared_ptr<boost::thread> m_thread;

	void Init();
	bool StartHardware();
	bool StopHardware();
	void Do_Work();
	void GetMeterDetails();

	void ImportOldMonthData(const unsigned long long DevID, const int Year, const int Month);
};

