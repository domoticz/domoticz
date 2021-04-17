#pragma once

#include "DomoticzHardware.h"
#include "hardwaretypes.h"

namespace Json
{
	class Value;
} // namespace Json

class CHoneywell : public CDomoticzHardwareBase
{
      public:
	CHoneywell(int ID, const std::string &Username, const std::string &Password, const std::string &Extra);
	~CHoneywell() override = default;
	bool WriteToHardware(const char *pdata, unsigned char length) override;

      private:
	void SetSetpoint(int idx, float temp, int nodeid);
	void SetPauseStatus(int idx, bool bHeating, int nodeid);
	void SendSetPointSensor(unsigned char Idx, float Temp, const std::string &defaultname);
	bool refreshToken();
	void Init();
	bool StartHardware() override;
	bool StopHardware() override;
	void Do_Work();
	void GetThermostatData();
	bool GetSwitchValue(int NodeID);

      private:
	std::string mApiKey;
	std::string mApiSecret;
	std::string mAccessToken;
	std::string mRefreshToken;
	time_t mTokenExpires = { 0 };
	std::string mThermostatID;
	int mOutsideTemperatureIdx;
	bool mIsStarted;
	std::shared_ptr<std::thread> m_thread;
	std::vector<std::string> mSessionHeaders;
	std::map<int, Json::Value> mDeviceList;
	std::map<int, std::string> mLocationList;
	int mLastMinute;
};
