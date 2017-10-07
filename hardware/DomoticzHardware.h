#pragma once

#include <boost/signals2.hpp>
#include "../main/RFXNames.h"

//Base class with functions all notification systems should have
#define RX_BUFFER_SIZE 100

class CDomoticzHardwareBase
{
	friend class MainWorker;
public:
	CDomoticzHardwareBase();
	virtual ~CDomoticzHardwareBase();

	bool Start();
	bool Stop();
	virtual bool WriteToHardware(const char *pdata, const unsigned char length)=0;

	void EnableOutputLog(const bool bEnableLog);

	bool IsStarted() { return m_bIsStarted; }

	void SetHeartbeatReceived();

	time_t m_LastHeartbeat;
	time_t m_LastHeartbeatReceive;
	bool m_bSkipReceiveCheck;
	int m_HwdID;
	unsigned long m_DataTimeout;
	std::string Name;
	_eHardwareTypes HwdType;
	unsigned char m_SeqNr;
	unsigned char m_rxbufferpos;
	bool m_bEnableReceive;
	boost::signals2::signal<void(CDomoticzHardwareBase *pHardware, const unsigned char *pRXCommand, const char *defaultName, const int BatteryLevel)> sDecodeRXMessage;
	boost::signals2::signal<void(CDomoticzHardwareBase *pDevice)> sOnConnected;
	void *m_pUserData;
	bool m_bOutputLog;
protected:

	virtual bool StartHardware()=0;
	virtual bool StopHardware()=0;
	bool onRFXMessage(const unsigned char *pBuffer, const size_t Len);

    //Heartbeat thread for classes that can not provide this themselves
	void StartHeartbeatThread();
	void StopHeartbeatThread();
	void HandleHBCounter(const int iInterval);

	//Sensor Helpers
	void SendTempSensor(const int NodeID, const int BatteryLevel, const float temperature, const std::string &defaultname, const int RssiLevel = 12);
	void SendHumiditySensor(const int NodeID, const int BatteryLevel, const int humidity, const std::string &defaultname, const int RssiLevel = 12);
	void SendBaroSensor(const int NodeID, const int ChildID, const int BatteryLevel, const float pressure, const int forecast, const std::string &defaultname);
	void SendTempHumSensor(const int NodeID, const int BatteryLevel, const float temperature, const int humidity, const std::string &defaultname, const int RssiLevel = 12);
	void SendTempHumBaroSensor(const int NodeID, const int BatteryLevel, const float temperature, const int humidity, const float pressure, int forecast, const std::string &defaultname, const int RssiLevel = 12);
	void SendTempHumBaroSensorFloat(const int NodeID, const int BatteryLevel, const float temperature, const int humidity, const float pressure, int forecast, const std::string &defaultname, const int RssiLevel = 12);
	void SendTempBaroSensor(const int NodeID, const int BatteryLevel, const float temperature, const float pressure, const std::string &defaultname);
	void SendSetPointSensor(const int NodeID, const int ChildID, const unsigned char SensorID, const float Temp, const std::string &defaultname);
	void SendKwhMeterOldWay(const int NodeID, const int ChildID, const int BatteryLevel, const double musage, const double mtotal, const std::string &defaultname);
	void SendKwhMeter(const int NodeID, const int ChildID, const int BatteryLevel, const double musage, const double mtotal, const std::string &defaultname);
	void SendWattMeter(const int NodeID, const int ChildID, const int BatteryLevel, const float musage, const std::string &defaultname);
	double GetKwhMeter(const int NodeID, const int ChildID, bool &bExists);
	void SendLuxSensor(const int NodeID, const int ChildID, const int BatteryLevel, const float Lux, const std::string &defaultname);
	void SendAirQualitySensor(const int NodeID, const int ChildID, const int BatteryLevel, const int AirQuality, const std::string &defaultname);
	void SendUsageSensor(const int NodeID, const int ChildID, const int BatteryLevel, const float Usage, const std::string &defaultname);
	void SendSwitch(const int NodeID, const int ChildID, const int BatteryLevel, const bool bOn, const double Level, const std::string &defaultname, const int RssiLevel = 12);
	void SendSwitchIfNotExists(const int NodeID, const int ChildID, const int BatteryLevel, const bool bOn, const double Level, const std::string &defaultname);
	void SendRGBWSwitch(const int NodeID, const int ChildID, const int BatteryLevel, const double Level, const bool bIsRGBW, const std::string &defaultname);
	void SendVoltageSensor(const int NodeID, const int ChildID, const int BatteryLevel, const float Volt, const std::string &defaultname);
	void SendCurrentSensor(const int NodeID, const int BatteryLevel, const float Current1, const float Current2, const float Current3, const std::string &defaultname, const int RssiLevel = 12);
	void SendPercentageSensor(const int NodeID, const int ChildID, const int BatteryLevel, const float Percentage, const std::string &defaultname);
	void SendWaterflowSensor(const int NodeID, const int ChildID, const int BatteryLevel, const float LPM, const std::string &defaultname);
	void SendRainSensor(const int NodeID, const int BatteryLevel, const float RainCounter, const std::string &defaultname, const int RssiLevel = 12);
	float GetRainSensorValue(const int NodeID, bool &bExists);
	bool GetWindSensorValue(const int NodeID, int &WindDir, float &WindSpeed, float &WindGust, float &twindtemp, float &windchill, bool bHaveWindTemp, bool &bExists);
	void SendWind(const int NodeID, const int BatteryLevel, const int WindDir, const float WindSpeed, const float WindGust, const float WindTemp, const float WindChill, const bool bHaveWindTemp, const std::string &defaultname, const int RssiLevel = 12);
	void SendPressureSensor(const int NodeID, const int ChildID, const int BatteryLevel, const float pressure, const std::string &defaultname);
	void SendSolarRadiationSensor(const unsigned char NodeID, const int BatteryLevel, const float radiation, const std::string &defaultname);
	void SendDistanceSensor(const int NodeID, const int ChildID, const int BatteryLevel, const float distance, const std::string &defaultname);
	void SendMeterSensor(const int NodeID, const int ChildID, const int BatteryLevel, const float metervalue, const std::string &defaultname, const int RssiLevel = 12);
	void SendUVSensor(const int NodeID, const int ChildID, const int BatteryLevel, const float UVI, const std::string &defaultname, const int RssiLevel = 12);
	void SendBlindSensor(const int NodeID, const int ChildID, const int BatteryLevel, const int Command, const std::string &defaultname, const int RssiLevel = 12);
	void SendSoundSensor(const int NodeID, const int BatteryLevel, const int sLevel, const std::string &defaultname);
	void SendAlertSensor(const int NodeID, const int BatteryLevel, const int alertLevel, const std::string &message, const std::string &defaultname);
	void SendGeneralSwitchSensor(const int NodeID, const int BatteryLevel, const int switchState, const char* defaultname, const int unitCode);
	void SendMoistureSensor(const int NodeID, const int BatteryLevel, const int mLevel, const std::string &defaultname);
	void SendTextSensor(const int NodeID, const int ChildID, const int BatteryLevel, const std::string &textMessage, const std::string &defaultname);
	std::string GetTextSensorText(const int NodeID, const int ChildID, bool &bExists);
	bool CheckPercentageSensorExists(const int NodeID, const int ChildID);
	void SendCustomSensor(const int NodeID, const int ChildID, const int BatteryLevel, const float Dust, const std::string &defaultname, const std::string &defaultLabel);
	void SendZWaveAlarmSensor(const int NodeID, const int InstanceID, const int BatteryLevel, const int aType, const int aValue, const std::string &defaultname);

	int m_iHBCounter;
	boost::mutex readQueueMutex;
	unsigned char m_rxbuffer[RX_BUFFER_SIZE];

	//Barometric calculation (only for 1 sensor per hardware device!)
	int CalculateBaroForecast(const double pressure);
    
    bool m_bIsStarted;
    
private:
    void Do_Heartbeat_Work();

    volatile bool m_stopHeartbeatrequested;
    boost::shared_ptr<boost::thread> m_Heartbeatthread;

    int m_baro_minuteCount;
    double m_pressureSamples[9][6];
    double m_pressureAvg[9];
    double m_dP_dt;
    int m_last_forecast;
    time_t m_BaroCalcLastTime;

};

