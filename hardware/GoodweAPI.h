#pragma once

#include "DomoticzHardware.h"

namespace Json
{
	class Value;
};

class GoodweAPI : public CDomoticzHardwareBase
{
public:
	GoodweAPI(const int ID, const std::string &userName, const int ServerLocation);
	~GoodweAPI(void);
	bool WriteToHardware(const char *pdata, const unsigned char length) override;
private:
	void Init();
	bool StartHardware() override;
	bool StopHardware() override;
	uint32_t hash(const std::string &str);
	int getSunRiseSunSetMinutes(const bool bGetSunRise);
	bool GoodweServerClient(const std::string &sPATH, std::string &sResult);
	float getPowerWatt(const std::string &str);
	float getEnergyWh(const std::string &str);
	void Do_Work();
	void GetMeterDetails();
	void ParseStation(const std::string &sStationId, const std::string &sStationName);
	void ParseDeviceList(const std::string &sStationId, const std::string &sStationName);
	void ParseDevice(const Json::Value &device, const std::string &sStationId, const std::string &sStationName);
private:
	std::string m_UserName;
	std::string m_Host;
	volatile bool m_stoprequested;
	std::shared_ptr<std::thread> m_thread;
};

