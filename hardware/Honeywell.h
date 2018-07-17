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
	CHoneywell(const int ID, const std::string &Username, const std::string &Password);
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
	void GetMeterDetails();
private:
	std::string mAccessToken;
	std::string mRefreshToken;
	std::string mThermostatID;
	int mOutsideTemperatureIdx;
	volatile bool mStopRequested;
	bool mNeedsTokenRefresh;
	bool mIsStarted;
	std::shared_ptr<std::thread> m_thread;
	std::vector<std::string> mSessionHeaders;
	std::map<int, Json::Value> mDeviceList;
	std::map<int, std::string> mLocationList;
	int mLastMinute;
};
