#pragma once

#include "DomoticzHardware.h"
#include <iosfwd>
#include "../json/json.h"

class GoodweAPI : public CDomoticzHardwareBase
{
public:
	GoodweAPI(const int ID, const std::string &userName);
	~GoodweAPI(void);
	bool WriteToHardware(const char *pdata, const unsigned char length);
private:
	std::string m_UserName;
	volatile bool m_stoprequested;
	boost::shared_ptr<boost::thread> m_thread;

	void Init();
	bool StartHardware();
	bool StopHardware();
	uint32_t hash(const std::string &str);
	int getSunRiseSunSetMinutes(const bool bGetSunRise);
	float getPowerWatt(const std::string &str);
	float getEnergyWh(const std::string &str);
	void Do_Work();
	void GetMeterDetails();
	void ParseStation(const std::string &sStationId, const std::string &sStationName);
	void ParseDeviceList(const std::string &sStationId, const std::string &sStationName);
	void ParseDevice(Json::Value device, const std::string &sStationId, const std::string &sStationName);

};

