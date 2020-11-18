#pragma once

#include "DomoticzHardware.h"

class CSBFSpot : public CDomoticzHardwareBase
{
      public:
	CSBFSpot(int ID, const std::string &SBFConfigFile);
	~CSBFSpot() override = default;
	bool WriteToHardware(const char *pdata, unsigned char length) override;
	void ImportOldMonthData();

      private:
	void SendMeter(unsigned char ID1, unsigned char ID2, double musage, double mtotal, const std::string &defaultname);
	bool GetMeter(unsigned char ID1, unsigned char ID2, double &musage, double &mtotal);
	void Init();
	bool StartHardware() override;
	bool StopHardware() override;
	void Do_Work();
	void GetMeterDetails();
	int getSunRiseSunSetMinutes(bool bGetSunRise);

	void ImportOldMonthData(uint64_t DevID, int Year, int Month);

      private:
	std::string m_SBFConfigFile;
	std::string m_SBFInverter;
	std::string m_SBFDataPath;
	std::string m_SBFPlantName;
	std::string m_SBFDateFormat;
	std::string m_SBFTimeFormat;

	std::string m_LastDateTime;
	std::shared_ptr<std::thread> m_thread;
};
