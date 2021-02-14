#pragma once

#include "DomoticzHardware.h"

namespace Json
{
	class Value;
} // namespace Json

class GoodweAPI : public CDomoticzHardwareBase
{
      public:
	GoodweAPI(int ID, const std::string &userName, int ServerLocation);
	~GoodweAPI() override = default;
	bool WriteToHardware(const char *pdata, unsigned char length) override;

      private:
	void Init();
	void SendCurrentSensor(int NodeID, uint8_t ChildID, int BatteryLevel, float Amp, const std::string &defaultname);
	bool StartHardware() override;
	bool StopHardware() override;
	uint32_t hash(const std::string &str);
	int getSunRiseSunSetMinutes(bool bGetSunRise);
	bool GoodweServerClient(const std::string &sPATH, std::string &sResult);
	bool getValueFromJson(const Json::Value &inputValue, std::string &outputValue, const std::string &errorString);
	bool getValueFromJson(const Json::Value &inputValue, float &outputValue, const std::string &errorString);
	bool getValueFromJson(const Json::Value &inputValue, int &outputValue, const std::string &errorString);
	void Do_Work();
	void GetMeterDetails();
	void ParseDeviceList(const std::string &sStationId, const std::string &sStationName);
	void ParseDevice(const Json::Value &device, const std::string &sStationId, const std::string &sStationName);

      private:
	std::string m_UserName;
	std::string m_Host;
	std::shared_ptr<std::thread> m_thread;
};
