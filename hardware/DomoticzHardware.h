#pragma once

#include <boost/signals2.hpp>
#include "../main/RFXNames.h"
#include "../main/StoppableTask.h"
// type support
#include "../cereal/types/string.hpp"
#include "../cereal/types/memory.hpp"
// the archiver
#include "../cereal/archives/portable_binary.hpp"

enum _eLogLevel : uint32_t;
enum _eDebugLevel : uint32_t;

//Base class with functions all notification systems should have
class CDomoticzHardwareBase : public StoppableTask
{
	friend class MainWorker;
public:
	CDomoticzHardwareBase();
	virtual ~CDomoticzHardwareBase();

	bool Start();
	bool Stop();
	bool Restart();
	bool RestartWithDelay(const long seconds);
	virtual bool WriteToHardware(const char *pdata, const unsigned char length) = 0;
	virtual bool CustomCommand(const uint64_t idx, const std::string &sCommand);

	void EnableOutputLog(const bool bEnableLog);

	bool IsStarted() { return m_bIsStarted; }

	void SetHeartbeatReceived();

	time_t m_LastHeartbeat = { 0 };
	time_t m_LastHeartbeatReceive = { 0 };

	int m_HwdID = { 0 }; //must be uniquely assigned
	bool m_bSkipReceiveCheck = { false };
	unsigned long m_DataTimeout = { 0 };
	std::string m_Name;
	std::string m_ShortName;
	_eHardwareTypes HwdType;
	unsigned char m_SeqNr = { 0 };
	bool m_bEnableReceive = { false };
	boost::signals2::signal<void(CDomoticzHardwareBase *pHardware, const unsigned char *pRXCommand, const char *defaultName, const int BatteryLevel)> sDecodeRXMessage;
	boost::signals2::signal<void(CDomoticzHardwareBase *pDevice)> sOnConnected;
	void *m_pUserData = { NULL };
	bool m_bOutputLog = { true };

	int SetThreadNameInt(const std::thread::native_handle_type &thread);

	//Log Helper functions
	void Log(const _eLogLevel level, const std::string& sLogline);
	void Log(const _eLogLevel level, const char* logline, ...)
#ifdef __GNUC__
		__attribute__((format(printf, 3, 4)))
#endif
		;
	void Debug(const _eDebugLevel level, const std::string& sLogline);
	void Debug(const _eDebugLevel level, const char* logline, ...)
#ifdef __GNUC__
		__attribute__((format(printf, 3, 4)))
#endif
		;

protected:
	virtual bool StartHardware()=0;
	virtual bool StopHardware()=0;

    //Heartbeat thread for classes that can not provide this themselves
	void StartHeartbeatThread();
	void StartHeartbeatThread(const char* ThreadName);
	void StopHeartbeatThread();
	void HandleHBCounter(const int iInterval);

	//Sensor Helpers
	void SendTempSensor(const int NodeID, const int BatteryLevel, const float temperature, const std::string &defaultname, const int RssiLevel = 12);
	void SendHumiditySensor(const int NodeID, const int BatteryLevel, const int humidity, const std::string &defaultname, const int RssiLevel = 12);
	void SendBaroSensor(const int NodeID, const int ChildID, const int BatteryLevel, const float pressure, const int forecast, const std::string &defaultname);
	void SendTempHumSensor(const int NodeID, const int BatteryLevel, const float temperature, const int humidity, const std::string &defaultname, const int RssiLevel = 12);
	void SendTempHumBaroSensor(const int NodeID, const int BatteryLevel, const float temperature, const int humidity, const float pressure, int forecast, const std::string &defaultname, const int RssiLevel = 12);
	void SendTempHumBaroSensorFloat(const int NodeID, const int BatteryLevel, const float temperature, const int humidity, const float pressure, const uint8_t forecast, const std::string &defaultname, const int RssiLevel = 12);
	void SendTempBaroSensor(const uint8_t NodeID, const int BatteryLevel, const float temperature, const float pressure, const std::string &defaultname);
	void SendSetPointSensor(const uint8_t NodeID, const uint8_t ChildID, const unsigned char SensorID, const float Temp, const std::string &defaultname);
	void SendKwhMeterOldWay(const int NodeID, const int ChildID, const int BatteryLevel, const double musage, const double mtotal, const std::string &defaultname);
	void SendKwhMeter(const int NodeID, const int ChildID, const int BatteryLevel, const double musage, const double mtotal, const std::string &defaultname);
	void SendWattMeter(const uint8_t NodeID, const uint8_t ChildID, const int BatteryLevel, const float musage, const std::string &defaultname);
	double GetKwhMeter(const int NodeID, const int ChildID, bool &bExists);
	void SendLuxSensor(const uint8_t NodeID, const uint8_t ChildID, const uint8_t BatteryLevel, const float Lux, const std::string &defaultname);
	void SendAirQualitySensor(const uint8_t NodeID, const uint8_t ChildID, const int BatteryLevel, const int AirQuality, const std::string &defaultname);
	void SendUsageSensor(const uint8_t NodeID, const uint8_t ChildID, const int BatteryLevel, const float Usage, const std::string &defaultname);
	void SendSwitchUnchecked(const int NodeID, const uint8_t ChildID, const int BatteryLevel, const bool bOn, const double Level, const std::string &defaultname, const int RssiLevel = 12);
	void SendSwitch(const int NodeID, const uint8_t ChildID, const int BatteryLevel, const bool bOn, const double Level, const std::string &defaultname, const int RssiLevel = 12);
	void SendSwitchIfNotExists(const int NodeID, const uint8_t ChildID, const int BatteryLevel, const bool bOn, const double Level, const std::string &defaultname);
	void SendRGBWSwitch(const int NodeID, const uint8_t ChildID, const int BatteryLevel, const int Level, const bool bIsRGBW, const std::string &defaultname);
	void SendGeneralSwitch(const int NodeID, const int ChildID, const int BatteryLevel, const uint8_t SwitchState, const uint8_t Level, const std::string &defaultname, const int RssiLevel = 12);
	void SendVoltageSensor(const int NodeID, const uint32_t ChildID, const int BatteryLevel, const float Volt, const std::string &defaultname);
	void SendCurrentSensor(const int NodeID, const int BatteryLevel, const float Current1, const float Current2, const float Current3, const std::string &defaultname, const int RssiLevel = 12);
	void SendPercentageSensor(const int NodeID, const uint8_t ChildID, const int BatteryLevel, const float Percentage, const std::string &defaultname);
	void SendWaterflowSensor(const int NodeID, const uint8_t ChildID, const int BatteryLevel, const float LPM, const std::string &defaultname);
	void SendRainSensor(const int NodeID, const int BatteryLevel, const float RainCounter, const std::string &defaultname, const int RssiLevel = 12);
	void SendRainRateSensor(const int NodeID, const int BatteryLevel, const float RainRate, const std::string &defaultname, const int RssiLevel = 12);
	float GetRainSensorValue(const int NodeID, bool &bExists);
	bool GetWindSensorValue(const int NodeID, int &WindDir, float &WindSpeed, float &WindGust, float &twindtemp, float &windchill, bool bHaveWindTemp, bool &bExists);
	void SendWind(const int NodeID, const int BatteryLevel, const int WindDir, const float WindSpeed, const float WindGust, const float WindTemp, const float WindChill, const bool bHaveWindTemp, const bool bHaveWindChill, const std::string &defaultname, const int RssiLevel = 12);
	void SendPressureSensor(const int NodeID, const int ChildID, const int BatteryLevel, const float pressure, const std::string &defaultname);
	void SendSolarRadiationSensor(const unsigned char NodeID, const int BatteryLevel, const float radiation, const std::string &defaultname);
	void SendDistanceSensor(const int NodeID, const int ChildID, const int BatteryLevel, const float distance, const std::string &defaultname);
	void SendMeterSensor(const int NodeID, const int ChildID, const int BatteryLevel, const float metervalue, const std::string &defaultname, const int RssiLevel = 12);
	void SendUVSensor(const int NodeID, const int ChildID, const int BatteryLevel, const float UVI, const std::string &defaultname, const int RssiLevel = 12);
	void SendBlindSensor(const uint8_t NodeID, const uint8_t ChildID, const int BatteryLevel, const uint8_t Command, const std::string &defaultname, const int RssiLevel = 12);
	void SendSoundSensor(const int NodeID, const int BatteryLevel, const int sLevel, const std::string &defaultname);
	void SendAlertSensor(const int NodeID, const int BatteryLevel, const int alertLevel, const std::string &message, const std::string &defaultname);
	void SendMoistureSensor(const int NodeID, const int BatteryLevel, const int mLevel, const std::string &defaultname);
	void SendTextSensor(const int NodeID, const int ChildID, const int BatteryLevel, const std::string &textMessage, const std::string &defaultname);
	std::string GetTextSensorText(const int NodeID, const int ChildID, bool &bExists);
	bool CheckPercentageSensorExists(const int NodeID, const int ChildID);
	void SendCustomSensor(const int NodeID, const uint8_t ChildID, const int BatteryLevel, const float CustomValue, const std::string &defaultname, const std::string &defaultLabel);
	void SendZWaveAlarmSensor(const int NodeID, const uint8_t InstanceID, const int BatteryLevel, const uint8_t aType, const int aValue, const std::string& alarmLabel, const std::string &defaultname);
	void SendFanSensor(const int Idx, const int BatteryLevel, const int FanSpeed, const std::string &defaultname);

	int m_iHBCounter = { 0 };
	bool m_bIsStarted = { false };
private:
    void Do_Heartbeat_Work();

	volatile bool m_stopHeartbeatrequested = { false };
	std::shared_ptr<std::thread> m_Heartbeatthread = { nullptr };
};

