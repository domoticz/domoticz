#pragma once

#include "DomoticzHardware.h"

class CSBFSpot : public CDomoticzHardwareBase
{
public:
	CSBFSpot(const int ID, const std::string &SBFConfigFile);
	~CSBFSpot(void);
	bool WriteToHardware(const char *pdata, const unsigned char length) override;
	void ImportOldMonthData();
private:
	void SendMeter(const unsigned char ID1,const unsigned char ID2, const double musage, const double mtotal, const std::string &defaultname);
	bool GetMeter(const unsigned char ID1,const unsigned char ID2, double &musage, double &mtotal);
	void Init();
	bool StartHardware() override;
	bool StopHardware() override;
	void Do_Work();
	void GetMeterDetails();
	int getSunRiseSunSetMinutes(const bool bGetSunRise);

	void ImportOldMonthData(const uint64_t DevID, const int Year, const int Month);
private:
	std::string m_SBFConfigFile;
	std::string m_SBFInverter;
	std::string m_SBFDataPath;
	std::string m_SBFPlantName;
	std::string m_SBFDateFormat;
	std::string m_SBFTimeFormat;

	std::string m_LastDateTime;
	volatile bool m_stoprequested;
	std::shared_ptr<std::thread> m_thread;
};

