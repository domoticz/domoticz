#pragma once

#define BOOST_ALLOW_DEPRECATED_HEADERS
#include <boost/signals2.hpp>

#include "../main/RFXNames.h"
#include "../main/StoppableTask.h"
// type support
#include <cereal/types/string.hpp>
#include <cereal/types/memory.hpp>
// the archiver
#include <cereal/archives/portable_binary.hpp>

enum _eLogLevel : uint32_t;
enum _eDebugLevel : uint32_t;

// Base class with functions all notification systems should have
class CDomoticzHardwareBase : public StoppableTask
{
	friend class MainWorker;

      public:
	CDomoticzHardwareBase();
	virtual ~CDomoticzHardwareBase() = default;

	bool Start();
	bool Stop();
	bool Restart();
	bool RestartWithDelay(long seconds);
	virtual bool WriteToHardware(const char *pdata, unsigned char length) = 0;
	virtual bool CustomCommand(uint64_t idx, const std::string &sCommand);

	void EnableOutputLog(bool bEnableLog);

	bool IsStarted()
	{
		return m_bIsStarted;
	}

	void SetHeartbeatReceived();

	time_t m_LastHeartbeat = { 0 };
	time_t m_LastHeartbeatReceive = { 0 };

	int m_HwdID = { 0 }; // must be uniquely assigned
	bool m_bSkipReceiveCheck = { false };
	unsigned long m_DataTimeout = { 0 };
	std::string m_Name;
	std::string m_ShortName;
	_eHardwareTypes HwdType;
	unsigned char m_SeqNr = { 0 };
	bool m_bEnableReceive = { false };
	boost::signals2::signal<void(CDomoticzHardwareBase *pHardware, const unsigned char *pRXCommand, const char *defaultName, const int BatteryLevel, const char *userName)> sDecodeRXMessage;
	boost::signals2::signal<void(CDomoticzHardwareBase *pDevice)> sOnConnected;
	void *m_pUserData = { nullptr };
	bool m_bOutputLog = { true };

	int SetThreadNameInt(const std::thread::native_handle_type &thread);

	// Log Helper functions
	void Log(_eLogLevel level, const std::string &sLogline);
	void Log(_eLogLevel level, const char *logline, ...)
#ifdef __GNUC__
		__attribute__((format(printf, 3, 4)))
#endif
		;
	void Debug(_eDebugLevel level, const std::string &sLogline);
	void Debug(_eDebugLevel level, const char *logline, ...)
#ifdef __GNUC__
		__attribute__((format(printf, 3, 4)))
#endif
		;
	uint32_t m_LogLevelEnabled; //bitwise _eLogLevel 7 = LOG_NORM | LOG_STATUS | LOG_ERROR
      protected:
	virtual bool StartHardware() = 0;
	virtual bool StopHardware() = 0;

	// Heartbeat thread for classes that can not provide this themselves
	void StartHeartbeatThread();
	void StartHeartbeatThread(const char *ThreadName);
	void StopHeartbeatThread();
	void HandleHBCounter(int iInterval);

	// Sensor Helpers
	void SendTempSensor(int NodeID, int BatteryLevel, float temperature, const std::string &defaultname, int RssiLevel = 12);
	void SendHumiditySensor(int NodeID, int BatteryLevel, int humidity, const std::string &defaultname, int RssiLevel = 12);
	void SendBaroSensor(int NodeID, int ChildID, int BatteryLevel, float pressure, int forecast, const std::string &defaultname);
	void SendTempHumSensor(int NodeID, int BatteryLevel, float temperature, int humidity, const std::string &defaultname, int RssiLevel = 12);
	void SendTempHumBaroSensor(int NodeID, int BatteryLevel, float temperature, int humidity, float pressure, int forecast, const std::string &defaultname, int RssiLevel = 12);
	void SendTempHumBaroSensorFloat(int NodeID, int BatteryLevel, float temperature, int humidity, float pressure, uint8_t forecast, const std::string &defaultname, int RssiLevel = 12);
	void SendTempBaroSensor(uint8_t NodeID, int BatteryLevel, float temperature, float pressure, const std::string &defaultname);
	void SendSetPointSensor(uint8_t NodeID, uint8_t ChildID, unsigned char SensorID, float Temp, const std::string &defaultname);
	void SendKwhMeterOldWay(int NodeID, int ChildID, int BatteryLevel, double musage, double mtotal, const std::string &defaultname, int RssiLevel = 12);
	void SendKwhMeter(int NodeID, int ChildID, int BatteryLevel, double musage, double mtotal, const std::string &defaultname, int RssiLevel = 12);
	void SendWattMeter(uint8_t NodeID, uint8_t ChildID, int BatteryLevel, float musage, const std::string &defaultname, int RssiLevel = 12);
	double GetKwhMeter(int NodeID, int ChildID, bool &bExists);
	void SendLuxSensor(uint8_t NodeID, uint8_t ChildID, uint8_t BatteryLevel, float Lux, const std::string &defaultname);
	void SendAirQualitySensor(uint8_t NodeID, uint8_t ChildID, int BatteryLevel, int AirQuality, const std::string &defaultname);
	void SendSwitchUnchecked(int NodeID, uint8_t ChildID, int BatteryLevel, bool bOn, double Level, const std::string &defaultname, const std::string &userName, int RssiLevel = 12);
	void SendSwitch(int NodeID, uint8_t ChildID, int BatteryLevel, bool bOn, double Level, const std::string &defaultname, const std::string &userName, int RssiLevel = 12);
	void SendSwitchIfNotExists(int NodeID, uint8_t ChildID, int BatteryLevel, bool bOn, double Level, const std::string &userName, const std::string &defaultname);
	void SendRGBWSwitch(int NodeID, uint8_t ChildID, int BatteryLevel, int Level, bool bIsRGBW, const std::string &defaultname, const std::string &userName);
	void SendGeneralSwitch(int NodeID, int ChildID, int BatteryLevel, uint8_t SwitchState, uint8_t Level, const std::string &defaultname, const std::string &userName, int RssiLevel = 12);
	void SendVoltageSensor(int NodeID, uint32_t ChildID, int BatteryLevel, float Volt, const std::string &defaultname);
	void SendCurrentSensor(int NodeID, int BatteryLevel, float Current1, float Current2, float Current3, const std::string &defaultname, int RssiLevel = 12);
	void SendPercentageSensor(int NodeID, uint8_t ChildID, int BatteryLevel, float Percentage, const std::string &defaultname);
	void SendWaterflowSensor(int NodeID, uint8_t ChildID, int BatteryLevel, float LPM, const std::string &defaultname);
	void SendRainSensor(int NodeID, int BatteryLevel, float RainCounter, const std::string &defaultname, int RssiLevel = 12);
	void SendRainSensorWU(int NodeID, int BatteryLevel, float RainCounter, float LastHour, const std::string &defaultname, int RssiLevel = 12);
	void SendRainRateSensor(int NodeID, int BatteryLevel, float RainRate, const std::string &defaultname, int RssiLevel = 12);
	float GetRainSensorValue(int NodeID, bool &bExists);
	bool GetWindSensorValue(int NodeID, int &WindDir, float &WindSpeed, float &WindGust, float &twindtemp, float &windchill, bool bHaveWindTemp, bool &bExists);
	void SendWind(int NodeID, int BatteryLevel, int WindDir, float WindSpeed, float WindGust, float WindTemp, float WindChill, bool bHaveWindTemp, bool bHaveWindChill,
		      const std::string &defaultname, int RssiLevel = 12);
	void SendPressureSensor(int NodeID, int ChildID, int BatteryLevel, float pressure, const std::string &defaultname);
	void SendSolarRadiationSensor(unsigned char NodeID, int BatteryLevel, float radiation, const std::string &defaultname);
	void SendDistanceSensor(int NodeID, int ChildID, int BatteryLevel, float distance, const std::string &defaultname, int RssiLevel = 12);
	void SendMeterSensor(int NodeID, int ChildID, int BatteryLevel, float metervalue, const std::string &defaultname, int RssiLevel = 12);
	void SendUVSensor(int NodeID, int ChildID, int BatteryLevel, float UVI, const std::string &defaultname, int RssiLevel = 12);
	void SendVisibilitySensor(int NodeID, int ChildID, int BatteryLevel, float Visibility, const std::string &defaultname);
	void SendBlindSensor(uint8_t NodeID, uint8_t ChildID, int BatteryLevel, uint8_t Command, const std::string &defaultname, const std::string &userName, int RssiLevel = 12);
	void SendSoundSensor(int NodeID, int BatteryLevel, int sLevel, const std::string &defaultname);
	void SendAlertSensor(int NodeID, int BatteryLevel, int alertLevel, const std::string &message, const std::string &defaultname);
	void SendMoistureSensor(int NodeID, int BatteryLevel, int mLevel, const std::string &defaultname, int RssiLevel = 12);
	void SendTextSensor(int NodeID, int ChildID, int BatteryLevel, const std::string &textMessage, const std::string &defaultname);
	std::string GetTextSensorText(int NodeID, int ChildID, bool &bExists);
	bool CheckPercentageSensorExists(int NodeID, int ChildID);
	void SendCustomSensor(int NodeID, uint8_t ChildID, int BatteryLevel, float CustomValue, const std::string &defaultname, const std::string &defaultLabel, int RssiLevel = 12);
	void SendZWaveAlarmSensor(int NodeID, uint8_t InstanceID, int BatteryLevel, uint8_t aType, int aValue, const std::string &alarmLabel, const std::string &defaultname);
	void SendFanSensor(int Idx, int BatteryLevel, int FanSpeed, const std::string &defaultname);
	void SendSecurity1Sensor(int NodeID, int DeviceSubType, int BatteryLevel, int Status, const std::string &defaultname, const std::string &userName, int RssiLevel = 12);
	void SendSelectorSwitch(int NodeID, uint8_t ChildID, const std::string &sValue, const std::string &defaultname, int customImage, bool nDropdown, const std::string &LevelNames,
				const std::string &LevelActions, bool bHideOff, const std::string &userName);
	int MigrateSelectorSwitch(int NodeID, uint8_t ChildID, const std::string &LevelNames, const std::string &LevelActions, bool bMigrate);
	int m_iHBCounter = { 0 };
	bool m_bIsStarted = { false };

      private:
	void Do_Heartbeat_Work();

	volatile bool m_stopHeartbeatrequested = { false };
	std::shared_ptr<std::thread> m_Heartbeatthread = { nullptr };
};
