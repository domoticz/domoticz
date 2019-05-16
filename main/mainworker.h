#pragma once

#include "RFXtrx.h"
#include "../hardware/DomoticzHardware.h"
#include "Scheduler.h"
#include "EventSystem.h"
#include "Camera.h"
#include <deque>
#include "WindCalculation.h"
#include "TrendCalculator.h"
#include "StoppableTask.h"
#include "../tcpserver/TCPServer.h"
#include "concurrent_queue.h"
#include "../webserver/server_settings.hpp"
#ifdef ENABLE_PYTHON
#	include "../hardware/plugins/PluginManager.h"
#endif

class MainWorker : public StoppableTask
{
public:
	MainWorker();
	~MainWorker();

	bool Start();
	bool Stop();

	void AddAllDomoticzHardware();
	void StopDomoticzHardware();
	void StartDomoticzHardware();
	void AddDomoticzHardware(CDomoticzHardwareBase *pHardware);
	void RemoveDomoticzHardware(CDomoticzHardwareBase *pHardware);
	void RemoveDomoticzHardware(int HwdId);
	int FindDomoticzHardware(int HwdId);
	int FindDomoticzHardwareByType(const _eHardwareTypes HWType);
	CDomoticzHardwareBase* GetHardware(int HwdId);
	CDomoticzHardwareBase* GetHardwareByIDType(const std::string &HwdId, const _eHardwareTypes HWType);
	CDomoticzHardwareBase* GetHardwareByType(const _eHardwareTypes HWType);

	void HeartbeatUpdate(const std::string &component, bool critical = true);
	void HeartbeatRemove(const std::string &component);
	void HeartbeatCheck();

	void SetWebserverSettings(const http::server::server_settings & settings);
	std::string GetWebserverAddress();
	std::string GetWebserverPort();
#ifdef WWW_ENABLE_SSL
	void SetSecureWebserverSettings(const http::server::ssl_server_settings & ssl_settings);
	std::string GetSecureWebserverPort();
#endif
	void DecodeRXMessage(const CDomoticzHardwareBase *pHardware, const uint8_t *pRXCommand, const char *defaultName, const int BatteryLevel);
	void PushAndWaitRxMessage(const CDomoticzHardwareBase *pHardware, const uint8_t *pRXCommand, const char *defaultName, const int BatteryLevel);

	bool SwitchLight(const std::string &idx, const std::string &switchcmd, const std::string &level, const std::string &color, const std::string &ooc, const int ExtraDelay);
	bool SwitchLight(const uint64_t idx, const std::string &switchcmd, const int level, const _tColor color, const bool ooc, const int ExtraDelay);
	bool SwitchLightInt(const std::vector<std::string> &sd, std::string switchcmd, int level, _tColor color, const bool IsTesting);

	bool SwitchScene(const std::string &idx, const std::string &switchcmd);
	bool SwitchScene(const uint64_t idx, std::string switchcmd);
	void CheckSceneCode(const uint64_t DevRowIdx, const uint8_t dType, const uint8_t dSubType, const int nValue, const char* sValue);
	bool DoesDeviceActiveAScene(const uint64_t DevRowIdx, const int Cmnd);

	bool SetSetPoint(const std::string &idx, const float TempValue);
	bool SetSetPoint(const std::string &idx, const float TempValue, const std::string &newMode, const std::string &until);
	bool SetSetPointInt(const std::vector<std::string> &sd, const float TempValue);
	bool SetThermostatState(const std::string &idx, const int newState);
	bool SetClock(const std::string &idx, const std::string &clockstr);
	bool SetClockInt(const std::vector<std::string> &sd, const std::string &clockstr);
	bool SetZWaveThermostatMode(const std::string &idx, const int tMode);
	bool SetZWaveThermostatFanMode(const std::string &idx, const int fMode);
	bool SetZWaveThermostatModeInt(const std::vector<std::string> &sd, const int tMode);
	bool SetZWaveThermostatFanModeInt(const std::vector<std::string> &sd, const int fMode);

	bool SwitchModal(const std::string &idx, const std::string &status, const std::string &action, const std::string &ooc, const std::string &until);

	bool GetSunSettings();
	void LoadSharedUsers();

	void ForceLogNotificationCheck();

	bool RestartHardware(const std::string &idx);

	bool AddHardwareFromParams(
		const int ID,
		const std::string &Name,
		const bool Enabled,
		const _eHardwareTypes Type,
		const std::string &Address, const uint16_t Port, const std::string &SerialPort,
		const std::string &Username, const std::string &Password,
		const std::string &Extra,
		const int Mode1,
		const int Mode2,
		const int Mode3,
		const int Mode4,
		const int Mode5,
		const int Mode6,
		const int DataTimeout,
		const bool bDoStart
	);

	void UpdateDomoticzSecurityStatus(const int iSecStatus);
	void SetInternalSecStatus();
	bool GetSensorData(const uint64_t idx, int &nValue, std::string &sValue);

	bool UpdateDevice(const int HardwareID, const std::string &DeviceID, const int unit, const int devType, const int subType, int nValue, std::string &sValue, const int signallevel, const int batterylevel, const bool parseTrigger = true);

	boost::signals2::signal<void(const int m_HwdID, const uint64_t DeviceRowIdx, const std::string &DeviceName, const uint8_t *pRXCommand)> sOnDeviceReceived;
	boost::signals2::signal<void(const uint64_t SceneIdx, const std::string &SceneName)> sOnSwitchScene;

	CScheduler m_scheduler;
	CEventSystem m_eventsystem;
#ifdef ENABLE_PYTHON
	Plugins::CPluginSystem m_pluginsystem;
#endif
	CCameraHandler m_cameras;
	bool m_bIgnoreUsernamePassword;
	bool m_bHaveUpdate;
	int m_iRevision;

	std::string m_szSystemName;
	std::string m_szDomoticzUpdateURL;

	bool IsUpdateAvailable(const bool bIsForced = false);
	bool StartDownloadUpdate();
	bool m_bHaveDownloadedDomoticzUpdate;
	bool m_bHaveDownloadedDomoticzUpdateSuccessFull;
	std::string m_UpdateStatusMessage;

	void GetAvailableWebThemes();

	tcp::server::CTCPServer m_sharedserver;
	std::string m_LastSunriseSet;
	std::vector<int> m_SunRiseSetMins;
	std::string m_DayLength;
	std::vector<std::string> m_webthemes;
	std::map<uint16_t, _tWindCalculator> m_wind_calculator;
	std::map<uint64_t, _tTrendCalculator> m_trend_calculator;

	time_t m_LastHeartbeat = 0;
private:
	void HandleAutomaticBackups();
	uint64_t PerformRealActionFromDomoticzClient(const uint8_t *pRXCommand, CDomoticzHardwareBase **pOriginalHardware);
	void HandleLogNotifications();
	std::map<std::string, std::pair<time_t, bool> > m_componentheartbeats;
	std::mutex m_heartbeatmutex;

	std::mutex m_decodeRXMessageMutex;

	std::vector<int> m_devicestorestart;

	bool m_bForceLogNotificationCheck;

	int m_SecCountdown;
	int m_SecStatus;

	int m_ScheduleLastMinute;
	int m_ScheduleLastHour;
	//fix for hardware clock that sets time back/ford
	time_t m_ScheduleLastMinuteTime;
	time_t m_ScheduleLastHourTime;
	time_t m_ScheduleLastDayTime;


	std::mutex m_devicemutex;

	std::string m_szDomoticzUpdateChecksumURL;
	bool m_bDoDownloadDomoticzUpdate;
	bool m_bStartHardware;
	uint8_t m_hardwareStartCounter;

	std::vector<CDomoticzHardwareBase*> m_hardwaredevices;
	http::server::server_settings m_webserver_settings;
#ifdef WWW_ENABLE_SSL
	http::server::ssl_server_settings m_secure_webserver_settings;
#endif
	std::shared_ptr<std::thread> m_thread;
	std::mutex m_mutex;

	time_t m_LastUpdateCheck;

	void Do_Work();
	void Heartbeat();
	void ParseRFXLogFile();
	bool WriteToHardware(const int HwdID, const char *pdata, const uint8_t length);

	void OnHardwareConnected(CDomoticzHardwareBase *pHardware);

	void WriteMessageStart();
	void WriteMessage(const char *szMessage);
	void WriteMessage(const char *szMessage, bool linefeed);
	void WriteMessageEnd();

	//message decoders
	void decode_BateryLevel(bool bIsInPercentage, uint8_t level);
	uint8_t get_BateryLevel(const _eHardwareTypes HwdType, bool bIsInPercentage, uint8_t level);

	// RxMessage queue resources
	volatile unsigned long m_rxMessageIdx;
	std::shared_ptr<std::thread> m_rxMessageThread;
	StoppableTask m_TaskRXMessage;
	void Do_Work_On_Rx_Messages();
	struct _tRxQueueItem {
		std::string Name;
		int BatteryLevel;
		unsigned long rxMessageIdx;
		int hardwareId;
		std::vector<uint8_t> vrxCommand;
		boost::uint16_t crc;
		queue_element_trigger* trigger;
	};
	concurrent_queue<_tRxQueueItem> m_rxMessageQueue;
	void UnlockRxMessageQueue();
	void PushRxMessage(const CDomoticzHardwareBase *pHardware, const uint8_t *pRXCommand, const char *defaultName, const int BatteryLevel);
	void CheckAndPushRxMessage(const CDomoticzHardwareBase *pHardware, const uint8_t *pRXCommand, const char *defaultName, const int BatteryLevel, const bool wait);
	void ProcessRXMessage(const CDomoticzHardwareBase *pHardware, const uint8_t *pRXCommand, const char *defaultName, const int BatteryLevel); //battery level: 0-100, 255=no battery, -1 = don't set

	struct _tRxMessageProcessingResult {
		std::string DeviceName;
		uint64_t DeviceRowIdx;
		bool bProcessBatteryValue;
	};

	//(RFX) Message decoders
	void decode_InterfaceMessage(const int HwdID, const _eHardwareTypes HwdType, const tRBUF *pResponse, _tRxMessageProcessingResult & procResult);
	void decode_InterfaceControl(const int HwdID, const _eHardwareTypes HwdType, const tRBUF *pResponse, _tRxMessageProcessingResult & procResult);
	void decode_UNDECODED(const int HwdID, const _eHardwareTypes HwdType, const tRBUF *pResponse, _tRxMessageProcessingResult & procResult);
	void decode_RecXmitMessage(const int HwdID, const _eHardwareTypes HwdType, const tRBUF *pResponse, _tRxMessageProcessingResult & procResult);
	void decode_Rain(const int HwdID, const _eHardwareTypes HwdType, const tRBUF *pResponse, _tRxMessageProcessingResult & procResult);
	void decode_Wind(const int HwdID, const _eHardwareTypes HwdType, const tRBUF *pResponse, _tRxMessageProcessingResult & procResult);
	void decode_Temp(const int HwdID, const _eHardwareTypes HwdType, const tRBUF *pResponse, _tRxMessageProcessingResult & procResult);
	void decode_Hum(const int HwdID, const _eHardwareTypes HwdType, const tRBUF *pResponse, _tRxMessageProcessingResult & procResult);
	void decode_TempHum(const int HwdID, const _eHardwareTypes HwdType, const tRBUF *pResponse, _tRxMessageProcessingResult & procResult);
	void decode_TempRain(const int HwdID, const _eHardwareTypes HwdType, const tRBUF *pResponse, _tRxMessageProcessingResult & procResult);
	void decode_UV(const int HwdID, const _eHardwareTypes HwdType, const tRBUF *pResponse, _tRxMessageProcessingResult & procResult);
	void decode_Lighting1(const int HwdID, const _eHardwareTypes HwdType, const tRBUF *pResponse, _tRxMessageProcessingResult & procResult);
	void decode_Lighting2(const int HwdID, const _eHardwareTypes HwdType, const tRBUF *pResponse, _tRxMessageProcessingResult & procResult);
	void decode_Lighting3(const int HwdID, const _eHardwareTypes HwdType, const tRBUF *pResponse, _tRxMessageProcessingResult & procResult);
	void decode_Lighting4(const int HwdID, const _eHardwareTypes HwdType, const tRBUF *pResponse, _tRxMessageProcessingResult & procResult);
	void decode_Lighting5(const int HwdID, const _eHardwareTypes HwdType, const tRBUF *pResponse, _tRxMessageProcessingResult & procResult);
	void decode_Lighting6(const int HwdID, const _eHardwareTypes HwdType, const tRBUF *pResponse, _tRxMessageProcessingResult & procResult);
	void decode_Fan(const int HwdID, const _eHardwareTypes HwdType, const tRBUF *pResponse, _tRxMessageProcessingResult & procResult);
	void decode_Curtain(const int HwdID, const _eHardwareTypes HwdType, const tRBUF *pResponse, _tRxMessageProcessingResult & procResult);
	void decode_BLINDS1(const int HwdID, const _eHardwareTypes HwdType, const tRBUF *pResponse, _tRxMessageProcessingResult & procResult);
	void decode_RFY(const int HwdID, const _eHardwareTypes HwdType, const tRBUF *pResponse, _tRxMessageProcessingResult & procResult);
	void decode_Security1(const int HwdID, const _eHardwareTypes HwdType, const tRBUF *pResponse, _tRxMessageProcessingResult & procResult);
	void decode_Security2(const int HwdID, const _eHardwareTypes HwdType, const tRBUF *pResponse, _tRxMessageProcessingResult & procResult);
	void decode_Camera1(const int HwdID, const _eHardwareTypes HwdType, const tRBUF *pResponse, _tRxMessageProcessingResult & procResult);
	void decode_Remote(const int HwdID, const _eHardwareTypes HwdType, const tRBUF *pResponse, _tRxMessageProcessingResult & procResult);
	void decode_Thermostat1(const int HwdID, const _eHardwareTypes HwdType, const tRBUF *pResponse, _tRxMessageProcessingResult & procResult);
	void decode_Thermostat2(const int HwdID, const _eHardwareTypes HwdType, const tRBUF *pResponse, _tRxMessageProcessingResult & procResult);
	void decode_Thermostat3(const int HwdID, const _eHardwareTypes HwdType, const tRBUF *pResponse, _tRxMessageProcessingResult & procResult);
	void decode_Thermostat4(const int HwdID, const _eHardwareTypes HwdType, const tRBUF *pResponse, _tRxMessageProcessingResult & procResult);
	void decode_Radiator1(const int HwdID, const _eHardwareTypes HwdType, const tRBUF *pResponse, _tRxMessageProcessingResult & procResult);
	void decode_Baro(const int HwdID, const _eHardwareTypes HwdType, const tRBUF *pResponse, _tRxMessageProcessingResult & procResult);
	void decode_TempHumBaro(const int HwdID, const _eHardwareTypes HwdType, const tRBUF *pResponse, _tRxMessageProcessingResult & procResult);
	void decode_TempBaro(const int HwdID, const _eHardwareTypes HwdType, const tRBUF *pResponse, _tRxMessageProcessingResult & procResult);
	void decode_DateTime(const int HwdID, const _eHardwareTypes HwdType, const tRBUF *pResponse, _tRxMessageProcessingResult & procResult);
	void decode_Current(const int HwdID, const _eHardwareTypes HwdType, const tRBUF *pResponse, _tRxMessageProcessingResult & procResult);
	void decode_Energy(const int HwdID, const _eHardwareTypes HwdType, const tRBUF *pResponse, _tRxMessageProcessingResult & procResult);
	void decode_Current_Energy(const int HwdID, const _eHardwareTypes HwdType, const tRBUF *pResponse, _tRxMessageProcessingResult & procResult);
	void decode_Gas(const int HwdID, const _eHardwareTypes HwdType, const tRBUF *pResponse, _tRxMessageProcessingResult & procResult);
	void decode_Water(const int HwdID, const _eHardwareTypes HwdType, const tRBUF *pResponse, _tRxMessageProcessingResult & procResult);
	void decode_Weight(const int HwdID, const _eHardwareTypes HwdType, const tRBUF *pResponse, _tRxMessageProcessingResult & procResult);
	void decode_RFXSensor(const int HwdID, const _eHardwareTypes HwdType, const tRBUF *pResponse, _tRxMessageProcessingResult & procResult);
	void decode_RFXMeter(const int HwdID, const _eHardwareTypes HwdType, const tRBUF *pResponse, _tRxMessageProcessingResult & procResult);
	void decode_P1MeterPower(const int HwdID, const _eHardwareTypes HwdType, const tRBUF *pResponse, _tRxMessageProcessingResult & procResult);
	void decode_P1MeterGas(const int HwdID, const _eHardwareTypes HwdType, const tRBUF *pResponse, _tRxMessageProcessingResult & procResult);
	void decode_YouLessMeter(const int HwdID, const _eHardwareTypes HwdType, const tRBUF *pResponse, _tRxMessageProcessingResult & procResult);
	void decode_AirQuality(const int HwdID, const _eHardwareTypes HwdType, const tRBUF *pResponse, _tRxMessageProcessingResult & procResult);
	void decode_FS20(const int HwdID, const _eHardwareTypes HwdType, const tRBUF *pResponse, _tRxMessageProcessingResult & procResult);
	void decode_Rego6XXTemp(const int HwdID, const _eHardwareTypes HwdType, const tRBUF *pResponse, _tRxMessageProcessingResult & procResult);
	void decode_Rego6XXValue(const int HwdID, const _eHardwareTypes HwdType, const tRBUF *pResponse, _tRxMessageProcessingResult & procResult);
	void decode_Usage(const int HwdID, const _eHardwareTypes HwdType, const tRBUF *pResponse, _tRxMessageProcessingResult & procResult);
	void decode_Lux(const int HwdID, const _eHardwareTypes HwdType, const tRBUF *pResponse, _tRxMessageProcessingResult & procResult);
	void decode_General(const int HwdID, const _eHardwareTypes HwdType, const tRBUF *pResponse, _tRxMessageProcessingResult & procResult, const uint8_t SignalLevel = 12, const uint8_t BatteryLevel = 255);
	void decode_GeneralSwitch(const int HwdID, const _eHardwareTypes HwdType, const tRBUF *pResponse, _tRxMessageProcessingResult & procResult);
	void decode_HomeConfort(const int HwdID, const _eHardwareTypes HwdType, const tRBUF *pResponse, _tRxMessageProcessingResult & procResult);
	void decode_Thermostat(const int HwdID, const _eHardwareTypes HwdType, const tRBUF *pResponse, _tRxMessageProcessingResult & procResult);
	void decode_Chime(const int HwdID, const _eHardwareTypes HwdType, const tRBUF *pResponse, _tRxMessageProcessingResult & procResult);
	void decode_BBQ(const int HwdID, const _eHardwareTypes HwdType, const tRBUF *pResponse, _tRxMessageProcessingResult & procResult);
	void decode_Power(const int HwdID, const _eHardwareTypes HwdType, const tRBUF *pResponse, _tRxMessageProcessingResult & procResult);
	void decode_ColorSwitch(const int HwdID, const _eHardwareTypes HwdType, const tRBUF *pResponse, _tRxMessageProcessingResult & procResult);
	void decode_evohome1(const int HwdID, const _eHardwareTypes HwdType, const tRBUF *pResponse, _tRxMessageProcessingResult & procResult);
	void decode_evohome2(const int HwdID, const _eHardwareTypes HwdType, const tRBUF *pResponse, _tRxMessageProcessingResult & procResult);
	void decode_evohome3(const int HwdID, const _eHardwareTypes HwdType, const tRBUF *pResponse, _tRxMessageProcessingResult & procResult);
	void decode_Cartelectronic(const int HwdID, const _eHardwareTypes HwdType, const tRBUF *pResponse, _tRxMessageProcessingResult & procResult);
	void decode_CartelectronicTIC(const int HwdID, const tRBUF *pResponse, _tRxMessageProcessingResult & procResult);
	void decode_CartelectronicEncoder(const int HwdID, const tRBUF *pResponse, _tRxMessageProcessingResult & procResult);
	void decode_ASyncPort(const int HwdID, const _eHardwareTypes HwdType, const tRBUF *pResponse, _tRxMessageProcessingResult & procResult);
	void decode_ASyncData(const int HwdID, const _eHardwareTypes HwdType, const tRBUF *pResponse, _tRxMessageProcessingResult & procResult);
	void decode_Weather(const int HwdID, const _eHardwareTypes HwdType, const tRBUF *pResponse, _tRxMessageProcessingResult & procResult);
	void decode_Solar(const int HwdID, const _eHardwareTypes HwdType, const tRBUF *pResponse, _tRxMessageProcessingResult & procResult);
};

extern MainWorker m_mainworker;
