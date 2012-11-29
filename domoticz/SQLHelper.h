#pragma once

#include <vector>
#include <string>
#include "UrlEncode.h"
#include "RFXNames.h"

struct sqlite3;

struct _tNotification
{
	unsigned long long ID;
	std::string Params;
	time_t LastSend;
};

class CSQLHelper
{
public:
	CSQLHelper(void);
	~CSQLHelper(void);

	void UpdateValue(const int HardwareID, const char* ID, unsigned char unit, unsigned char devType, unsigned char subType, unsigned char signallevel, unsigned char batterylevel, int nValue);
	void UpdateValue(const int HardwareID, const char* ID, unsigned char unit, unsigned char devType, unsigned char subType, unsigned char signallevel, unsigned char batterylevel, const char* sValue);
	void UpdateValue(const int HardwareID, const char* ID, unsigned char unit, unsigned char devType, unsigned char subType, unsigned char signallevel, unsigned char batterylevel, int nValue, const char* sValue);

	void UpdateTempVar(const char *Key, const char* sValue);
	bool GetTempVar(const char *Key, int &nValue, std::string &sValue);

	void UpdatePreferencesVar(const char *Key, const char* sValue);
	void UpdatePreferencesVar(const char *Key, int nValue);
	void UpdatePreferencesVar(const char *Key, int nValue, const char* sValue);
	bool GetPreferencesVar(const char *Key, int &nValue, std::string &sValue);

	//notification functions
	bool AddNotification(const std::string DevIdx, const std::string Param);
	bool UpdateNotification(const std::string ID, const std::string Param);
	bool RemoveDeviceNotifications(const std::string DevIdx);
	bool RemoveNotification(const std::string ID);
	std::vector<_tNotification> GetNotifications(const unsigned long long DevIdx);
	std::vector<_tNotification> GetNotifications(const std::string DevIdx);
	void TouchNotification(unsigned long long ID);
	bool HasNotifications(const unsigned long long DevIdx);
	bool HasNotifications(const std::string DevIdx);

	bool CheckAndHandleTempHumidityNotification(
		const int HardwareID, 
		const std::string ID, 
		const unsigned char unit, 
		const unsigned char devType, 
		const unsigned char subType, 
		const float temp, 
		const int humidity, 
		const bool bHaveTemp, 
		const bool bHaveHumidity);
	bool CheckAndHandleNotification(
		const int HardwareID, 
		const std::string ID, 
		const unsigned char unit, 
		const unsigned char devType, 
		const unsigned char subType, 
		const _eNotificationTypes ntype, 
		const float mvalue);
	bool CheckAndHandleRainNotification(
		const int HardwareID, 
		const std::string ID, 
		const unsigned char unit, 
		const unsigned char devType, 
		const unsigned char subType, 
		const _eNotificationTypes ntype, 
		const float mvalue);

	bool HasTimers(const unsigned long long Idx);
	bool HasTimers(const std::string Idx);

	bool SendNotification(const std::string EventID, const std::string Message);

	void Schedule5Minute();
	void ScheduleDay();

	void DeleteHardware(const std::string idx);

	std::vector<std::vector<std::string> > query(const std::string szQuery);

	std::string m_LastSwitchID;	//for learning command
	unsigned long long m_LastSwitchRowID;
private:
	CURLEncode m_urlencoder;
	sqlite3 *m_dbase;
	void UpdateTemperatureLog();
	void UpdateRainLog();
	void UpdateWindLog();
	void UpdateUVLog();
	void AddCalendarTemperature();
	void AddCalendarUpdateRain();
	void AddCalendarUpdateWind();
	void AddCalendarUpdateUV();
};

