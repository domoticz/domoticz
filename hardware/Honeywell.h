#pragma once

#include "DomoticzHardware.h"
#include <iosfwd>
#include "hardwaretypes.h"
#include "../json/json.h"

class CHoneywell : public CDomoticzHardwareBase
{
public:
	CHoneywell(const int ID, const std::string &Username, const std::string &Password, const int Mode1, const int Mode2, const int Mode3, const int Mode4, const int Mode5, const int Mode6);
	~CHoneywell(void);
	bool WriteToHardware(const char *pdata, const unsigned char length);
private:
	void SetSetpoint(const int idx, const float temp, const int nodeid);
	void SetPauseStatus(const int idx, bool bHeating, const int nodeid);
	void SendSetPointSensor(const unsigned char Idx, const float Temp, const std::string &defaultname);
	bool refreshToken();
	std::string mAccessToken;
	std::string mRefreshToken;
	std::string mThermostatID;
	int mOutsideTemperatureIdx;
	volatile bool mStopRequested;
	bool mNeedsTokenRefresh;
	bool mIsStarted;
	boost::shared_ptr<boost::thread> mThread;
	std::vector<std::string> mSessionHeaders;
	std::map<int, Json::Value> mDeviceList;
	std::map<int, std::string> mLocationList;

	int mLastMinute;

	void Init();
	bool StartHardware();
	bool StopHardware();
	void Do_Work();
	void GetMeterDetails();
};
