#pragma once

#include <time.h>
#include "DomoticzHardware.h"

class ZWaveBase : public CDomoticzHardwareBase
{
	friend class CRazberry;
	friend class COpenZWave;

	enum _eZWaveDeviceType
	{
		ZDTYPE_SWITCH_NORMAL = 0,
		ZDTYPE_SWITCH_DIMMER,
		ZDTYPE_SWITCH_RGBW,
		ZDTYPE_SWITCH_COLOR,

		ZDTYPE_SENSOR_TEMPERATURE,
		ZDTYPE_SENSOR_SETPOINT,
		ZDTYPE_SENSOR_HUMIDITY,
		ZDTYPE_SENSOR_LIGHT,

		ZDTYPE_SENSOR_PERCENTAGE,

		ZDTYPE_SENSOR_POWER,
		ZDTYPE_SENSOR_POWERENERGYMETER,
		ZDTYPE_SENSOR_GAS,
		ZDTYPE_SENSOR_VOLTAGE,
		ZDTYPE_SENSOR_AMPERE,

		ZDTYPE_SENSOR_THERMOSTAT_CLOCK,
		ZDTYPE_SENSOR_THERMOSTAT_FAN_MODE,
		ZDTYPE_SENSOR_THERMOSTAT_MODE,

		ZDTYPE_SENSOR_VELOCITY,
		ZDTYPE_SENSOR_BAROMETER,
		ZDTYPE_SENSOR_DEWPOINT,
		ZDTYPE_SENSOR_CO2,
		ZDTYPE_SENSOR_UV,
		ZDTYPE_SENSOR_WATER,
		ZDTYPE_SENSOR_MOISTURE,
		ZDTYPE_SENSOR_TANK_CAPACITY,

		ZDTYPE_ALARM,
		ZDTYPE_CENTRAL_SCENE,

		ZDTYPE_SENSOR_CUSTOM,
	};
	struct _tZWaveDevice
	{
		int nodeID;
		int instanceID;
		int indexID;
		int orgInstanceID;
		int orgIndexID;
		int commandClassID;
		_eZWaveDeviceType devType;
		float scaleMultiply;
		int basicType;
		int genericType;
		int specificType;
		bool isListening;
		bool sensor250;
		bool sensor1000;
		bool isFLiRS;
		bool hasWakeup;
		bool hasBattery;

		int Manufacturer_id;
		int Product_id;
		int Product_type;

		//values
		float floatValue;
		int intvalue;
		bool bValidValue;

		//battery
		int batValue;

		//main_id
		std::string string_id;

		//label
		std::string label;
		std::string custom_label;

		time_t lastreceived;
		unsigned char sequence_number;

		int Alarm_Type;

		_tZWaveDevice() :
			label("Unknown")
		{
			sequence_number=1;
			nodeID=-1;
			scaleMultiply=1.0f;
			isListening=false;
			sensor250=false;
			sensor1000=false;
			isFLiRS=false;
			hasWakeup=false;
			hasBattery=false;
			batValue = 255;
			floatValue=0;
			intvalue=0;
			bValidValue=true;
			commandClassID=0;
			instanceID=0;
			indexID=0;
			orgInstanceID=0;
			orgIndexID=0;
			devType = ZDTYPE_SWITCH_NORMAL;
			basicType=0;
			genericType=0;
			specificType=0;
			Manufacturer_id = -1;
			Product_id = -1;
			Product_type = -1;
			lastreceived = 0;
			Alarm_Type = -1;
		}
	};
public:
	ZWaveBase();
	~ZWaveBase(void);

	virtual bool GetInitialDevices()=0;
	virtual bool GetUpdates()=0;
	bool StartHardware() override;
	bool StopHardware() override;
	bool WriteToHardware(const char *pdata, const unsigned char length) override;
public:
	int m_LastIncludedNode;
	std::string m_LastIncludedNodeType;
	bool m_bHaveLastIncludedNodeInfo;
	int m_LastRemovedNode;
	std::mutex m_NotificationMutex;
private:
	void Do_Work();
	void SendDevice2Domoticz(const _tZWaveDevice *pDevice);
	void SendSwitchIfNotExists(const _tZWaveDevice *pDevice);

	_tZWaveDevice* FindDevice(const int nodeID, const int instanceID, const int indexID);
	_tZWaveDevice* FindDevice(const int nodeID, const int instanceID, const int indexID, const _eZWaveDeviceType devType);
	_tZWaveDevice* FindDevice(const int nodeID, const int instanceID, const int indexID, const int CommandClassID, const _eZWaveDeviceType devType);
	_tZWaveDevice* FindDeviceEx(const int nodeID, const int instanceID, const _eZWaveDeviceType devType);

	void ForceUpdateForNodeDevices(const unsigned int homeID, const int nodeID);
	bool IsNodeRGBW(const unsigned int homeID, const int nodeID);

	std::string GenerateDeviceStringID(const _tZWaveDevice *pDevice);
	void InsertDevice(_tZWaveDevice device);
	void UpdateDeviceBatteryStatus(const int nodeID, const int value);
	unsigned char Convert_Battery_To_PercInt(const unsigned char level);
	virtual bool SwitchLight(const int nodeID, const int instanceID, const int commandClass, const int value)=0;
	virtual bool SwitchColor(const int nodeID, const int instanceID, const int commandClass, const std::string &ColorStr) = 0;
	virtual void SetThermostatSetPoint(const int nodeID, const int instanceID, const int commandClass, const float value)=0;
	virtual void SetClock(const int nodeID, const int instanceID, const int commandClass, const int day, const int hour, const int minute)=0;
	virtual void SetThermostatMode(const int nodeID, const int instanceID, const int commandClass, const int tMode) = 0;
	virtual void SetThermostatFanMode(const int nodeID, const int instanceID, const int commandClass, const int fMode) = 0;
	virtual std::string GetSupportedThermostatFanModes(const unsigned long ID) = 0;
	virtual void StopHardwareIntern() = 0;
	virtual bool IncludeDevice(const bool bSecure) = 0;
	virtual bool ExcludeDevice(const int nodeID)=0;
	virtual bool RemoveFailedDevice(const int nodeID)=0;
	virtual bool CancelControllerCommand(const bool bForce = false)=0;
	virtual bool HasNodeFailed(const int nodeID) = 0;
	virtual bool IsNodeIncluded() = 0;
	virtual bool IsNodeExcluded() = 0;

	bool m_bControllerCommandInProgress;
	bool m_bControllerCommandCanceled;
	time_t m_ControllerCommandStartTime;
	time_t m_updateTime;
	bool m_bInitState;
	std::map<std::string,_tZWaveDevice> m_devices;
	std::shared_ptr<std::thread> m_thread;
};



