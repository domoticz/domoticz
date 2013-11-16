#pragma once

#include "SQLHelper.h"
#include "WebServer.h"
#include "RFXtrx.h"
#include "../hardware/DomoticzHardware.h"
#include "Scheduler.h"
#include "EventSystem.h"
#include "Camera.h"
#include <map>
#include <deque>
#include "WindCalculation.h"
#include "../tcpserver/TCPServer.h"

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

	void StopDomoticzHardware();
	void StartDomoticzHardware();
	void AddDomoticzHardware(CDomoticzHardwareBase *pHardware);
	void RemoveDomoticzHardware(CDomoticzHardwareBase *pHardware);
	void RemoveDomoticzHardware(int HwdId);
	int FindDomoticzHardware(int HwdId);
	void ClearDomoticzHardware();
	CDomoticzHardwareBase* GetHardware(int HwdId);

	void SetVerboseLevel(eVerboseLevel Level);
	void SetWebserverPort(const std::string &Port);
	std::string GetWebserverPort();

	bool SwitchLight(const std::string &idx, const std::string &switchcmd,const std::string &level);
	bool SwitchLight(unsigned long long idx, const std::string &switchcmd, unsigned char level);
	bool SwitchLightInt(const std::vector<std::string> &sd, std::string switchcmd, unsigned char level, const bool IsTesting);

	bool SwitchScene(const std::string &idx, const std::string &switchcmd);
	bool SwitchScene(const unsigned long long idx, const std::string &switchcmd);
	void CheckSceneCode(const int HardwareID, const char* ID, const unsigned char unit, const unsigned char devType, const unsigned char subType, const int nValue, const char* sValue);

	bool SetSetPoint(const std::string &idx, const float TempValue);
	bool SetSetPointInt(const std::vector<std::string> &sd, const float TempValue);

	bool SetRFXCOMHardwaremodes(const int HardwareID, const unsigned char Mode1,const unsigned char Mode2,const unsigned char Mode3,const unsigned char Mode4,const unsigned char Mode5);

	bool GetSunSettings();
	void LoadSharedUsers();

	bool RestartHardware(const std::string &idx);

	bool AddHardwareFromParams(
				int ID,
				std::string Name,
				bool Enabled,
				_eHardwareTypes Type,
				std::string Address, unsigned short Port, 
				std::string Username, std::string Password, 
				int Mode1,
				int Mode2, 
				int Mode3,
				int Mode4,
				int Mode5);

	void UpdateDomoticzSecurityStatus(const int iSecStatus);

	CSQLHelper m_sql;
	CScheduler m_scheduler;
	CEventSystem m_eventsystem;
    CCamScheduler m_cameras;
	bool m_bIgnoreUsernamePassword;

	void GetDomoticzUpdate(const std::string &UpdateURL);
	bool m_bHaveDownloadedDomoticzUpdate;
	bool m_bHaveDownloadedDomoticzUpdateSuccessFull;

	tcp::server::CTCPServer m_sharedserver;
private:

	void PrintDeviceName(const std::string &devname);
	void GetRaspberryPiTemperature();
	unsigned long long PerformRealActionFromDomoticzClient(const unsigned char *pRXCommand);
	struct _tStartScene
	{
		unsigned long long SceneRowID;
		std::string switchcmd;
	};
	unsigned char m_ScheduleLastMinute;
	unsigned char m_ScheduleLastHour;

	boost::mutex m_devicemutex;
	boost::mutex decodeRXMessageMutex;

	std::string m_szDomoticzUpdateURL;
	bool m_bDoDownloadDomoticzUpdate;
	bool m_bStartHardware;
	unsigned char m_hardwareStartCounter;

	std::vector<CDomoticzHardwareBase*> m_hardwaredevices;
	eVerboseLevel m_verboselevel;
	http::server::CWebServer m_webserver;
	std::string m_webserverport;

	volatile bool m_stoprequested;
	boost::shared_ptr<boost::thread> m_thread;
	boost::mutex m_mutex;

	boost::mutex m_startscene_mutex;
	std::vector<_tStartScene> m_scenes_to_start;

	std::map<unsigned short,_tWindCalculationStruct> m_wind_calculator;

	bool StartThread();
	void Do_Work();
	void SendResetCommand(CDomoticzHardwareBase *pHardware);
	void SendCommand(const int HwdID, unsigned char Cmd, const char *szMessage=NULL);
	void WriteToHardware(const int HwdID, const char *pdata, const unsigned char length);
	void DecodeRXMessage(const CDomoticzHardwareBase *pHardware, const unsigned char *pRXCommand);
	void OnHardwareConnected(CDomoticzHardwareBase *pHardware);

	void WriteMessageStart();
	void WriteMessage(const char *szMessage);
	void WriteMessage(const char *szMessage, bool linefeed);
	void WriteMessageEnd();

	//message decoders
	void decode_BateryLevel(bool bIsInPercentage, unsigned char level);
	unsigned char get_BateryLevel(bool bIsInPercentage, unsigned char level);

	//(RFX) Message decoders
	unsigned long long decode_InterfaceMessage(const int HwdID, const tRBUF *pResponse);
	unsigned long long decode_UNDECODED(const int HwdID, const tRBUF *pResponse);
	unsigned long long decode_RecXmitMessage(const int HwdID, const tRBUF *pResponse);
	unsigned long long decode_Rain(const int HwdID, const tRBUF *pResponse);
	unsigned long long decode_Wind(const int HwdID, const tRBUF *pResponse);
	unsigned long long decode_Temp(const int HwdID, const tRBUF *pResponse);
	unsigned long long decode_Hum(const int HwdID, const tRBUF *pResponse);
	unsigned long long decode_TempHum(const int HwdID, const tRBUF *pResponse);
	unsigned long long decode_TempRain(const int HwdID, const tRBUF *pResponse);
	unsigned long long decode_UV(const int HwdID, const tRBUF *pResponse);
	unsigned long long decode_Lighting1(const int HwdID, const tRBUF *pResponse);
	unsigned long long decode_Lighting2(const int HwdID, const tRBUF *pResponse);
	unsigned long long decode_Lighting3(const int HwdID, const tRBUF *pResponse);
	unsigned long long decode_Lighting4(const int HwdID, const tRBUF *pResponse);
	unsigned long long decode_Lighting5(const int HwdID, const tRBUF *pResponse);
	unsigned long long decode_Lighting6(const int HwdID, const tRBUF *pResponse);
	unsigned long long decode_BLINDS1(const int HwdID, const tRBUF *pResponse);
	unsigned long long decode_Security1(const int HwdID, const tRBUF *pResponse);
	unsigned long long decode_Camera1(const int HwdID, const tRBUF *pResponse);
	unsigned long long decode_Remote(const int HwdID, const tRBUF *pResponse);
	unsigned long long decode_Thermostat1(const int HwdID, const tRBUF *pResponse);
	unsigned long long decode_Thermostat2(const int HwdID, const tRBUF *pResponse);
	unsigned long long decode_Thermostat3(const int HwdID, const tRBUF *pResponse);
	unsigned long long decode_Baro(const int HwdID, const tRBUF *pResponse);
	unsigned long long decode_TempHumBaro(const int HwdID, const tRBUF *pResponse);
	unsigned long long decode_TempBaro(const int HwdID, const tRBUF *pResponse);
	unsigned long long decode_DateTime(const int HwdID, const tRBUF *pResponse);
	unsigned long long decode_Current(const int HwdID, const tRBUF *pResponse);
	unsigned long long decode_Energy(const int HwdID, const tRBUF *pResponse);
	unsigned long long decode_Current_Energy(const int HwdID, const tRBUF *pResponse);
	unsigned long long decode_Gas(const int HwdID, const tRBUF *pResponse);
	unsigned long long decode_Water(const int HwdID, const tRBUF *pResponse);
	unsigned long long decode_Weight(const int HwdID, const tRBUF *pResponse);
	unsigned long long decode_RFXSensor(const int HwdID, const tRBUF *pResponse);
	unsigned long long decode_RFXMeter(const int HwdID, const tRBUF *pResponse);
	unsigned long long decode_P1MeterPower(const int HwdID, const tRBUF *pResponse);
	unsigned long long decode_P1MeterGas(const int HwdID, const tRBUF *pResponse);
	unsigned long long decode_YouLessMeter(const int HwdID, const tRBUF *pResponse);
	unsigned long long decode_AirQuality(const int HwdID, const tRBUF *pResponse);
	unsigned long long decode_FS20(const int HwdID, const tRBUF *pResponse);
	unsigned long long decode_Rego6XXTemp(const int HwdID, const tRBUF *pResponse);
	unsigned long long decode_Rego6XXValue(const int HwdID, const tRBUF *pResponse);
	unsigned long long decode_Usage(const int HwdID, const tRBUF *pResponse);
	unsigned long long decode_Lux(const int HwdID, const tRBUF *pResponse);
	unsigned long long decode_General(const int HwdID, const tRBUF *pResponse);
	unsigned long long decode_Thermostat(const int HwdID, const tRBUF *pResponse);
	unsigned long long decode_Chime(const int HwdID, const tRBUF *pResponse);
	unsigned long long decode_BBQ(const int HwdID, const tRBUF *pResponse);
	unsigned long long decode_Power(const int HwdID, const tRBUF *pResponse);
	unsigned long long decode_LimitlessLights(const int HwdID, const tRBUF *pResponse);
};
