#pragma once

#include "DomoticzHardware.h"
#include "hardwaretypes.h"

namespace Json
{
	class Value;
};

class CHoneywell : public CDomoticzHardwareBase
{
public:
	CHoneywell(const int ID, const std::string &Username, const std::string &Password, const std::string &Extra);
	~CHoneywell(void);
	bool WriteToHardware(const char *pdata, const unsigned char length) override;
private:
	void SetSetpoint(const int idx, const float temp, const int nodeid);
	void SetPauseStatus(const int idx, bool bHeating, const int nodeid);
	void SendSetPointSensor(const unsigned char Idx, const float Temp, const std::string &defaultname);
	bool refreshToken();
	void Init();
	bool StartHardware() override;
	bool StopHardware() override;
	void Do_Work();
	void GetThermostatData();
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
