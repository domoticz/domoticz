#pragma once

#include <boost/signals2.hpp>
#include "../main/RFXNames.h"

//Base class with functions all notification systems should have
#define RX_BUFFER_SIZE 40

class CDomoticzHardwareBase
{
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
	boost::signals2::signal<void(CDomoticzHardwareBase *pHardware, const unsigned char *pRXCommand)> sDecodeRXMessage;
	boost::signals2::signal<void(CDomoticzHardwareBase *pDevice)> sOnConnected;
	void *m_pUserData;
	bool m_bOutputLog;
	int m_iLastSendNodeBatteryValue;
private:
	friend class C1Wire;
	friend class CDummy;
	friend class CEvohome;
	friend class DomoticzTCP;
	friend class P1MeterBase;
	friend class P1MeterSerial;
	friend class P1MeterTCP;
	friend class RFXComSerial;
	friend class RFXComTCP;
	friend class CYouLess;
	friend class CRego6XXSerial;
	friend class CDavisLoggerSerial;
	friend class CBMP085;
	friend class CPiFace;
	friend class S0MeterSerial;
	friend class OTGWBase;
	friend class OTGWSerial;
	friend class OTGWTCP;
	friend class Teleinfo;
	friend class CLimitLess;
	friend class ZWaveBase;
	friend class CWunderground;
	friend class CForecastIO;
	friend class CTE923;
	friend class CVolcraftCO20;
	friend class CEnOceanESP2;
	friend class CEnOceanESP3;
	friend class SolarEdgeTCP;
	friend class CSBFSpot;
	friend class CWOL;
	friend class CICYThermostat;
	friend class CPVOutputInput;
	friend class CGpio;
	friend class Meteostick;
	friend class CToonThermostat;
	friend class CEcoDevices;
	friend class CHarmonyHub;
	friend class MochadTCP;
	friend class CPhilipsHue;
	friend class MySensorsBase;
	friend class MySensorsSerial;
	friend class MySensorsTCP;
	friend class MQTT;
	friend class FritzboxTCP;
	friend class CETH8020;
	friend class CRFLink;
	friend class KMTronicBase;
	friend class KMTronicSerial;
	friend class KMTronicTCP;
	friend class KMTronic433;
	friend class SolarMaxTCP;
	friend class CPinger;

	virtual bool StartHardware()=0;
	virtual bool StopHardware()=0;
	bool onRFXMessage(const unsigned char *pBuffer, const size_t Len);
	//Heartbeat thread for classes that can not provide this themselves
	void StartHeartbeatThread();
	void StopHeartbeatThread();
	void Do_Heartbeat_Work();
	void HandleHBCounter(const int iInterval);

	//Sensor Helpers
	void SendTempSensor(const int NodeID, const int BatteryLevel, const float temperature, const std::string &defaultname);
	void SendHumiditySensor(const int NodeID, const int BatteryLevel, const int humidity);
	void SendBaroSensor(const int NodeID, const int ChildID, const int BatteryLevel, const float pressure, const int forecast);
	void SendTempHumSensor(const int NodeID, const int BatteryLevel, const float temperature, const int humidity);
	void SendTempHumBaroSensor(const int NodeID, const int BatteryLevel, const float temperature, const int humidity, const float pressure, int forecast);
	void SendTempHumBaroSensorFloat(const int NodeID, const int BatteryLevel, const float temperature, const int humidity, const float pressure, int forecast);
	void SendKwhMeter(const int NodeID, const int ChildID, const int BatteryLevel, const double musage, const double mtotal, const std::string &defaultname);
	void SendLuxSensor(const int NodeID, const int ChildID, const int BatteryLevel, const float Lux);
	void SendAirQualitySensor(const int NodeID, const int ChildID, const int BatteryLevel, const int AirQuality);
	void SendUsageSensor(const int NodeID, const int ChildID, const int BatteryLevel, const float Usage);
	void SendSwitch(const int NodeID, const int ChildID, const int BatteryLevel, const bool bOn, const double Level, const std::string &defaultname);
	void SendSwitchIfNotExists(const int NodeID, const int ChildID, const int BatteryLevel, const bool bOn, const double Level, const std::string &defaultname);
	void SendRGBWSwitch(const int NodeID, const int ChildID, const int BatteryLevel, const double Level, const bool bIsRGBW, const std::string &defaultname);
	void SendVoltageSensor(const int NodeID, const int ChildID, const int BatteryLevel, const float Volt, const std::string &defaultname);
	void SendCurrentSensor(const int NodeID, const int BatteryLevel, const float Current1, const float Current2, const float Current3, const std::string &defaultname);
	void SendPercentageSensor(const int NodeID, const int ChildID, const int BatteryLevel, const float Percentage, const std::string &defaultname);
	void SendRainSensor(const int NodeID, const int BatteryLevel, const int RainCounter);
	void SendWind(const int NodeID, const int BatteryLevel, const int WindDir, const float WindSpeed, const float WindGust, const float WindTemp, const float WindChill, const bool bHaveWindTemp);
	void SendPressureSensor(const int NodeID, const int ChildID, const int BatteryLevel, const float pressure);
	void SendDistanceSensor(const int NodeID, const int ChildID, const int BatteryLevel, const float distance);
	void SendMeterSensor(const int NodeID, const int ChildID, const int BatteryLevel, const float metervalue);
	void SenUVSensor(const int NodeID, const int ChildID, const int BatteryLevel, const float UVI);

	int m_iHBCounter;
	boost::shared_ptr<boost::thread> m_Heartbeatthread;
	volatile bool m_stopHeartbeatrequested;
	boost::mutex readQueueMutex;
	unsigned char m_rxbuffer[RX_BUFFER_SIZE];
	bool m_bIsStarted;

};

