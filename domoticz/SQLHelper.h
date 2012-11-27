#pragma once

#include <vector>
#include <string>
#include "UrlEncode.h"

struct sqlite3;

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

	bool AddEditNotification(const std::string Idx, const std::string Param);
	bool RemoveNotification(const std::string Idx);
	bool GetNotification(const unsigned long long Idx, std::string &Param);
	bool GetNotification(const std::string Idx, std::string &Param);

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
	void TouchNotification(unsigned long long ID);
};

