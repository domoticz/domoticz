#pragma once

#include <vector>
#include <string>
#include "RFXNames.h"
#include "../httpclient/UrlEncode.h"

struct sqlite3;

struct _tNotification
{
	unsigned long long ID;
	std::string Params;
	time_t LastSend;
};

struct _tDeviceStatus
{
	unsigned char _DelayTime;
	int _HardwareID;
	std::string _ID;
	unsigned char _unit;
	unsigned char _devType;
	unsigned char _subType;
	unsigned char _signallevel;
	unsigned char _batterylevel;
	int _nValue;
	std::string _sValue;

	_tDeviceStatus(const unsigned char DelayTime, const int HardwareID, const char* ID, const unsigned char unit, const unsigned char devType, const unsigned char subType, const unsigned char signallevel, const unsigned char batterylevel, const int nValue, const char* sValue)
	{
		_DelayTime=DelayTime;
		_HardwareID=HardwareID;
		_ID=ID;
		_unit=unit;
		_devType=devType;
		_subType=subType;
		_signallevel=signallevel;
		_batterylevel=batterylevel;
		_nValue=nValue;
		_sValue=sValue;
	}
};

class CSQLHelper
{
public:
	CSQLHelper(void);
	~CSQLHelper(void);

	bool OpenDatabase();
	void SetDatabaseName(const std::string DBName);

	void UpdateValue(const int HardwareID, const char* ID, const unsigned char unit, const unsigned char devType, const unsigned char subType, const unsigned char signallevel, const unsigned char batterylevel, const int nValue);
	void UpdateValue(const int HardwareID, const char* ID, const unsigned char unit, const unsigned char devType, const unsigned char subType, const unsigned char signallevel, const unsigned char batterylevel, const char* sValue);
	void UpdateValue(const int HardwareID, const char* ID, const unsigned char unit, const unsigned char devType, const unsigned char subType, const unsigned char signallevel, const unsigned char batterylevel, const int nValue, const char* sValue);

	void UpdateTempVar(const char *Key, const char* sValue);
	bool GetTempVar(const char *Key, int &nValue, std::string &sValue);

	void UpdatePreferencesVar(const char *Key, const char* sValue);
	void UpdatePreferencesVar(const char *Key, const int nValue);
	void UpdatePreferencesVar(const char *Key, const int nValue, const char* sValue);
	bool GetPreferencesVar(const char *Key, int &nValue, std::string &sValue);
	bool GetPreferencesVar(const char *Key, int &nValue);

	//notification functions
	bool AddNotification(const std::string DevIdx, const std::string Param);
	bool UpdateNotification(const std::string ID, const std::string Param);
	bool RemoveDeviceNotifications(const std::string DevIdx);
	bool RemoveNotification(const std::string ID);
	std::vector<_tNotification> GetNotifications(const unsigned long long DevIdx);
	std::vector<_tNotification> GetNotifications(const std::string DevIdx);
	void TouchNotification(const unsigned long long ID);
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
	bool CheckAndHandleTotalNotification(
		const int HardwareID, 
		const std::string ID, 
		const unsigned char unit, 
		const unsigned char devType, 
		const unsigned char subType, 
		const _eNotificationTypes ntype, 
		const float mvalue);
	bool CheckAndHandleUsageNotification(
		const int HardwareID, 
		const std::string ID, 
		const unsigned char unit, 
		const unsigned char devType, 
		const unsigned char subType, 
		const _eNotificationTypes ntype, 
		const float mvalue);
	bool CheckAndHandleAmpere123Notification(
		const int HardwareID, 
		const std::string ID, 
		const unsigned char unit, 
		const unsigned char devType, 
		const unsigned char subType, 
		const float Ampere1,
		const float Ampere2,
		const float Ampere3
		);

	bool HasTimers(const unsigned long long Idx);
	bool HasTimers(const std::string Idx);

	bool SendNotification(const std::string EventID, const std::string Message);

	void Schedule5Minute();
	void ScheduleDay();

	void DeleteHardware(const std::string idx);

	void DeleteDevice(const std::string idx);

	void TransferDevice(const std::string oldidx, const std::string newidx);

	std::vector<std::vector<std::string> > query(const std::string szQuery);

	std::string m_LastSwitchID;	//for learning command
	unsigned long long m_LastSwitchRowID;
private:
	boost::mutex m_sqlQueryMutex;
	CURLEncode m_urlencoder;
	sqlite3 *m_dbase;
	std::string m_dbase_name;

	std::vector<_tDeviceStatus> m_device_status_queue;
	boost::shared_ptr<boost::thread> m_device_status_thread;
	boost::mutex m_device_status_mutex;
	bool m_stoprequested;
	bool StartThread();
	void Do_Work();

	void UpdateValueInt(const int HardwareID, const char* ID, const unsigned char unit, const unsigned char devType, const unsigned char subType, const unsigned char signallevel, const unsigned char batterylevel, const int nValue, const char* sValue);

	void CheckAndUpdateDeviceOrder();

	void CleanupLightLog();

	void UpdateTemperatureLog();
	void UpdateRainLog();
	void UpdateWindLog();
	void UpdateUVLog();
	void UpdateMeter();
	void UpdateMultiMeter();
	void AddCalendarTemperature();
	void AddCalendarUpdateRain();
	void AddCalendarUpdateWind();
	void AddCalendarUpdateUV();
	void AddCalendarUpdateMeter();
	void AddCalendarUpdateMultiMeter();
};

