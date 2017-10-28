#pragma once

#include "DomoticzHardware.h"
#include <iosfwd>

class CSBFSpot : public CDomoticzHardwareBase
{
public:
	CSBFSpot(const int ID, const std::string &SBFConfigFile);
	~CSBFSpot(void);
	bool WriteToHardware(const char *pdata, const unsigned char length);
	void ImportOldMonthData();
private:
	void SendMeter(const unsigned char ID1,const unsigned char ID2, const double musage, const double mtotal, const std::string &defaultname);
	bool GetMeter(const unsigned char ID1,const unsigned char ID2, double &musage, double &mtotal);

	std::string m_SBFConfigFile;
	std::string m_SBFInverter;
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
	int getSunRiseSunSetMinutes(const bool bGetSunRise);

	void ImportOldMonthData(const uint64_t DevID, const int Year, const int Month);
};

