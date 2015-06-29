#pragma once

#include "RFXtrx.h"
#include "../hardware/DomoticzHardware.h"
#include "Scheduler.h"
#include "EventSystem.h"
#include "../hardware/HardwareMonitor.h"
#include "Camera.h"
#include <map>
#include <deque>
#include "WindCalculation.h"
#include "../tcpserver/TCPServer.h"
#include "DataPush.h"
#include "HttpPush.h"

enum eVerboseLevel
{
	EVBL_None,
	EVBL_ALL,
};

class MainWorker
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
	CDomoticzHardwareBase* GetHardware(int HwdId);
	
	void HeartbeatUpdate(const std::string component);
	void HeartbeatRemove(const std::string component);
	void HeartbeatCheck();

	void SetVerboseLevel(eVerboseLevel Level);
	void SetWebserverPort(const std::string &Port);
	std::string GetWebserverPort();
	void SetSecureWebserverPort(const std::string &Port);
	std::string GetSecureWebserverPort();
	void SetSecureWebserverCert(const std::string &CertFile);
	void SetSecureWebserverPass(const std::string &passphrase);

	void DecodeRXMessage(const CDomoticzHardwareBase *pHardware, const unsigned char *pRXCommand);

	bool SwitchLight(const std::string &idx, const std::string &switchcmd,const std::string &level, const std::string &hue, const std::string &ooc, const int ExtraDelay);
	bool SwitchLight(const unsigned long long idx, const std::string &switchcmd, const int level, const int hue, const bool ooc, const int ExtraDelay);
	bool SwitchLightInt(const std::vector<std::string> &sd, std::string switchcmd, int level, int hue, const bool IsTesting);

	bool SwitchScene(const std::string &idx, const std::string &switchcmd);
	bool SwitchScene(const unsigned long long idx, const std::string &switchcmd);
	void CheckSceneCode(const int HardwareID, const char* ID, const unsigned char unit, const unsigned char devType, const unsigned char subType, const int nValue, const char* sValue);
	bool DoesDeviceActiveAScene(const int HwdId, const std::string &idx, const int unit, const int devType, const int subType);

	bool SetSetPoint(const std::string &idx, const float TempValue);
	bool SetSetPoint(const std::string &idx, const float TempValue, const int newMode, const std::string &until);
	bool SetThermostatState(const std::string &idx, const int newState);
	bool SetSetPointInt(const std::vector<std::string> &sd, const float TempValue);
	bool SetClock(const std::string &idx, const std::string &clockstr);
	bool SetClockInt(const std::vector<std::string> &sd, const std::string &clockstr);
	bool SetZWaveThermostatMode(const std::string &idx, const int tMode);
	bool SetZWaveThermostatFanMode(const std::string &idx, const int fMode);
	bool SetZWaveThermostatModeInt(const std::vector<std::string> &sd, const int tMode);
	bool SetZWaveThermostatFanModeInt(const std::vector<std::string> &sd, const int fMode);

	bool SetRFXCOMHardwaremodes(const int HardwareID, const unsigned char Mode1, const unsigned char Mode2, const unsigned char Mode3, const unsigned char Mode4, const unsigned char Mode5, const unsigned char Mode6);
	
	bool SwitchModal(const std::string &idx, const std::string &status, const std::string &action, const std::string &ooc, const std::string &until);

	bool GetSunSettings();
	void LoadSharedUsers();

	bool RestartHardware(const std::string &idx);

	bool AddHardwareFromParams(
				const int ID,
				const std::string &Name,
				const bool Enabled,
				const _eHardwareTypes Type,
				const std::string &Address, const unsigned short Port, const std::string &SerialPort,
				const std::string &Username, const std::string &Password, 
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

	bool UpdateDevice(const int HardwareID, const std::string &DeviceID, const int unit, const int devType, const int subType, const int nValue, const std::string &sValue, const int signallevel, const int batterylevel);

	boost::signals2::signal<void(const int m_HwdID, const unsigned long long DeviceRowIdx, const std::string &DeviceName, const unsigned char *pRXCommand)> sOnDeviceReceived;

	CScheduler m_scheduler;
	CEventSystem m_eventsystem;
	CDataPush m_datapush;
	CHardwareMonitor m_hardwaremonitor;
	CCameraHandler m_cameras;
	CHttpPush m_httppush;

	bool m_bIgnoreUsernamePassword;

	void GetDomoticzUpdate(const std::string &UpdateURL, const std::string &ChecksumURL);
	bool m_bHaveDownloadedDomoticzUpdate;
	bool m_bHaveDownloadedDomoticzUpdateSuccessFull;

	void GetAvailableWebThemes();

	tcp::server::CTCPServer m_sharedserver;
	std::string m_LastSunriseSet;
	std::vector<std::string> m_webthemes;
	std::map<unsigned short, _tWindCalculationStruct> m_wind_calculator;

private:
	void HandleAutomaticBackups();
	unsigned long long PerformRealActionFromDomoticzClient(const unsigned char *pRXCommand, CDomoticzHardwareBase **pOriginalHardware);
	std::string m_LastDeviceName;
	std::map<std::string, time_t > m_componentheartbeats;
	boost::mutex m_heartbeatmutex;

	std::vector<int> m_devicestorestart;

	int m_SecCountdown;
	int m_SecStatus;

	int m_ScheduleLastMinute;
	int m_ScheduleLastHour;
	//fix for hardware clock that sets time back/ford
	time_t m_ScheduleLastMinuteTime;
	time_t m_ScheduleLastHourTime;
	time_t m_ScheduleLastDayTime;


	boost::mutex m_devicemutex;
	boost::mutex decodeRXMessageMutex;

	std::string m_szDomoticzUpdateURL;
	std::string m_szDomoticzUpdateChecksumURL;
	bool m_bDoDownloadDomoticzUpdate;
	bool m_bStartHardware;
	unsigned char m_hardwareStartCounter;

	std::vector<CDomoticzHardwareBase*> m_hardwaredevices;
	eVerboseLevel m_verboselevel;
	std::string m_webserverport;
	std::string m_secure_webserverport;
	std::string m_secure_web_cert_file;
	std::string m_secure_web_passphrase;

	volatile bool m_stoprequested;
	boost::shared_ptr<boost::thread> m_thread;
	boost::mutex m_mutex;

	bool StartThread();
	void Do_Work();
	void Heartbeat();
	void ParseRFXLogFile();
	void SendResetCommand(CDomoticzHardwareBase *pHardware);
	void SendCommand(const int HwdID, unsigned char Cmd, const char *szMessage=NULL);
	bool WriteToHardware(const int HwdID, const char *pdata, const unsigned char length);
	
	void OnHardwareConnected(CDomoticzHardwareBase *pHardware);

	void WriteMessageStart();
	void WriteMessage(const char *szMessage);
	void WriteMessage(const char *szMessage, bool linefeed);
	void WriteMessageEnd();

	//message decoders
	void decode_BateryLevel(bool bIsInPercentage, unsigned char level);
	unsigned char get_BateryLevel(const CDomoticzHardwareBase *pHardware, bool bIsInPercentage, unsigned char level);

	//(RFX) Message decoders
	unsigned long long decode_InterfaceMessage(const CDomoticzHardwareBase *pHardware, const int HwdID, const tRBUF *pResponse);
	unsigned long long decode_InterfaceControl(const CDomoticzHardwareBase *pHardware, const int HwdID, const tRBUF *pResponse);
	unsigned long long decode_UNDECODED(const CDomoticzHardwareBase *pHardware, const int HwdID, const tRBUF *pResponse);
	unsigned long long decode_RecXmitMessage(const CDomoticzHardwareBase *pHardware, const int HwdID, const tRBUF *pResponse);
	unsigned long long decode_Rain(const CDomoticzHardwareBase *pHardware, const int HwdID, const tRBUF *pResponse);
	unsigned long long decode_Wind(const CDomoticzHardwareBase *pHardware, const int HwdID, const tRBUF *pResponse);
	unsigned long long decode_Temp(const CDomoticzHardwareBase *pHardware, const int HwdID, const tRBUF *pResponse);
	unsigned long long decode_Hum(const CDomoticzHardwareBase *pHardware, const int HwdID, const tRBUF *pResponse);
	unsigned long long decode_TempHum(const CDomoticzHardwareBase *pHardware, const int HwdID, const tRBUF *pResponse);
	unsigned long long decode_TempRain(const CDomoticzHardwareBase *pHardware, const int HwdID, const tRBUF *pResponse);
	unsigned long long decode_UV(const CDomoticzHardwareBase *pHardware, const int HwdID, const tRBUF *pResponse);
	unsigned long long decode_Lighting1(const CDomoticzHardwareBase *pHardware, const int HwdID, const tRBUF *pResponse);
	unsigned long long decode_Lighting2(const CDomoticzHardwareBase *pHardware, const int HwdID, const tRBUF *pResponse);
	unsigned long long decode_Lighting3(const CDomoticzHardwareBase *pHardware, const int HwdID, const tRBUF *pResponse);
	unsigned long long decode_Lighting4(const CDomoticzHardwareBase *pHardware, const int HwdID, const tRBUF *pResponse);
	unsigned long long decode_Lighting5(const CDomoticzHardwareBase *pHardware, const int HwdID, const tRBUF *pResponse);
	unsigned long long decode_Lighting6(const CDomoticzHardwareBase *pHardware, const int HwdID, const tRBUF *pResponse);
	unsigned long long decode_Curtain(const CDomoticzHardwareBase *pHardware, const int HwdID, const tRBUF *pResponse);
	unsigned long long decode_BLINDS1(const CDomoticzHardwareBase *pHardware, const int HwdID, const tRBUF *pResponse);
	unsigned long long decode_RFY(const CDomoticzHardwareBase *pHardware, const int HwdID, const tRBUF *pResponse);
	unsigned long long decode_Security1(const CDomoticzHardwareBase *pHardware, const int HwdID, const tRBUF *pResponse);
	unsigned long long decode_Security2(const CDomoticzHardwareBase *pHardware, const int HwdID, const tRBUF *pResponse);
	unsigned long long decode_Camera1(const CDomoticzHardwareBase *pHardware, const int HwdID, const tRBUF *pResponse);
	unsigned long long decode_Remote(const CDomoticzHardwareBase *pHardware, const int HwdID, const tRBUF *pResponse);
	unsigned long long decode_Thermostat1(const CDomoticzHardwareBase *pHardware, const int HwdID, const tRBUF *pResponse);
	unsigned long long decode_Thermostat2(const CDomoticzHardwareBase *pHardware, const int HwdID, const tRBUF *pResponse);
	unsigned long long decode_Thermostat3(const CDomoticzHardwareBase *pHardware, const int HwdID, const tRBUF *pResponse);
	unsigned long long decode_Radiator1(const CDomoticzHardwareBase *pHardware, const int HwdID, const tRBUF *pResponse);
	unsigned long long decode_Baro(const CDomoticzHardwareBase *pHardware, const int HwdID, const tRBUF *pResponse);
	unsigned long long decode_TempHumBaro(const CDomoticzHardwareBase *pHardware, const int HwdID, const tRBUF *pResponse);
	unsigned long long decode_TempBaro(const CDomoticzHardwareBase *pHardware, const int HwdID, const tRBUF *pResponse);
	unsigned long long decode_DateTime(const CDomoticzHardwareBase *pHardware, const int HwdID, const tRBUF *pResponse);
	unsigned long long decode_Current(const CDomoticzHardwareBase *pHardware, const int HwdID, const tRBUF *pResponse);
	unsigned long long decode_Energy(const CDomoticzHardwareBase *pHardware, const int HwdID, const tRBUF *pResponse);
	unsigned long long decode_Current_Energy(const CDomoticzHardwareBase *pHardware, const int HwdID, const tRBUF *pResponse);
	unsigned long long decode_Gas(const CDomoticzHardwareBase *pHardware, const int HwdID, const tRBUF *pResponse);
	unsigned long long decode_Water(const CDomoticzHardwareBase *pHardware, const int HwdID, const tRBUF *pResponse);
	unsigned long long decode_Weight(const CDomoticzHardwareBase *pHardware, const int HwdID, const tRBUF *pResponse);
	unsigned long long decode_RFXSensor(const CDomoticzHardwareBase *pHardware, const int HwdID, const tRBUF *pResponse);
	unsigned long long decode_RFXMeter(const CDomoticzHardwareBase *pHardware, const int HwdID, const tRBUF *pResponse);
	unsigned long long decode_P1MeterPower(const CDomoticzHardwareBase *pHardware, const int HwdID, const tRBUF *pResponse);
	unsigned long long decode_P1MeterGas(const CDomoticzHardwareBase *pHardware, const int HwdID, const tRBUF *pResponse);
	unsigned long long decode_YouLessMeter(const CDomoticzHardwareBase *pHardware, const int HwdID, const tRBUF *pResponse);
	unsigned long long decode_AirQuality(const CDomoticzHardwareBase *pHardware, const int HwdID, const tRBUF *pResponse);
	unsigned long long decode_FS20(const CDomoticzHardwareBase *pHardware, const int HwdID, const tRBUF *pResponse);
	unsigned long long decode_Rego6XXTemp(const CDomoticzHardwareBase *pHardware, const int HwdID, const tRBUF *pResponse);
	unsigned long long decode_Rego6XXValue(const CDomoticzHardwareBase *pHardware, const int HwdID, const tRBUF *pResponse);
	unsigned long long decode_Usage(const CDomoticzHardwareBase *pHardware, const int HwdID, const tRBUF *pResponse);
	unsigned long long decode_Lux(const CDomoticzHardwareBase *pHardware, const int HwdID, const tRBUF *pResponse);
	unsigned long long decode_General(const CDomoticzHardwareBase *pHardware, const int HwdID, const tRBUF *pResponse);
	unsigned long long decode_GeneralSwitch(const CDomoticzHardwareBase *pHardware, const int HwdID, const tRBUF *pResponse);
	unsigned long long decode_Thermostat(const CDomoticzHardwareBase *pHardware, const int HwdID, const tRBUF *pResponse);
	unsigned long long decode_Chime(const CDomoticzHardwareBase *pHardware, const int HwdID, const tRBUF *pResponse);
	unsigned long long decode_BBQ(const CDomoticzHardwareBase *pHardware, const int HwdID, const tRBUF *pResponse);
	unsigned long long decode_Power(const CDomoticzHardwareBase *pHardware, const int HwdID, const tRBUF *pResponse);
	unsigned long long decode_LimitlessLights(const CDomoticzHardwareBase *pHardware, const int HwdID, const tRBUF *pResponse);
	unsigned long long decode_evohome1(const CDomoticzHardwareBase *pHardware, const int HwdID, const tRBUF *pResponse);
	unsigned long long decode_evohome2(const CDomoticzHardwareBase *pHardware, const int HwdID, const tRBUF *pResponse);
	unsigned long long decode_evohome3(const CDomoticzHardwareBase *pHardware, const int HwdID, const tRBUF *pResponse);
};

extern MainWorker m_mainworker;
