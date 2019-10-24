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
		ZDTYPE_SENSOR_THERMOSTAT_OPERATING_STATE,

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
		uint8_t nodeID;
		uint8_t instanceID;
		uint16_t indexID;
		uint8_t orgInstanceID;
		uint16_t orgIndexID;
		uint8_t commandClassID;

		_eZWaveDeviceType devType;
		float scaleMultiply;
		int basicType;
		bool isListening;
		bool hasWakeup;

		uint16_t Manufacturer_id;
		uint16_t Product_id;
		uint16_t Product_type;

		//values
		bool bValidValue;

		float floatValue;
		int intvalue;

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
			label("Unknown"),
			lastreceived(0)
		{
			sequence_number=1;
			nodeID=-1;
			scaleMultiply=1.0f;
			isListening=false;
			hasWakeup=false;
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
			Manufacturer_id = -1;
			Product_id = -1;
			Product_type = -1;
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
	uint8_t m_LastRemovedNode;
	std::mutex m_NotificationMutex;
private:
	void Do_Work();
	void SendDevice2Domoticz(const _tZWaveDevice *pDevice);
	void SendSwitchIfNotExists(const _tZWaveDevice *pDevice);

	_tZWaveDevice* FindDevice(const uint8_t nodeID, const int instanceID, const int indexID);
	_tZWaveDevice* FindDevice(const uint8_t nodeID, const int instanceID, const int indexID, const _eZWaveDeviceType devType);
	_tZWaveDevice* FindDevice(const uint8_t nodeID, const int instanceID, const int indexID, const int CommandClassID, const _eZWaveDeviceType devType);
	_tZWaveDevice* FindDeviceEx(const uint8_t nodeID, const int instanceID, const _eZWaveDeviceType devType);

	std::string GenerateDeviceStringID(const _tZWaveDevice *pDevice);
	void InsertDevice(_tZWaveDevice device);
	unsigned char Convert_Battery_To_PercInt(const unsigned char level);
	virtual bool SwitchLight(_tZWaveDevice* pDevice, const int instanceID, const int value)=0;
	virtual bool SwitchColor(const uint8_t nodeID, const uint8_t instanceID, const std::string &ColorStr) = 0;
	virtual void SetThermostatSetPoint(const uint8_t nodeID, const uint8_t instanceID, const int commandClass, const float value)=0;
	virtual void SetClock(const uint8_t nodeID, const uint8_t instanceID, const int commandClass, const int day, const int hour, const int minute)=0;
	virtual void SetThermostatMode(const uint8_t nodeID, const uint8_t instanceID, const int commandClass, const int tMode) = 0;
	virtual void SetThermostatFanMode(const uint8_t nodeID, const uint8_t instanceID, const int commandClass, const int fMode) = 0;
	virtual std::string GetSupportedThermostatFanModes(const unsigned long ID) = 0;
	virtual void StopHardwareIntern() = 0;
	virtual bool IncludeDevice(const bool bSecure) = 0;
	virtual bool ExcludeDevice(const uint8_t nodeID)=0;
	virtual bool RemoveFailedDevice(const uint8_t nodeID)=0;
	virtual bool CancelControllerCommand(const bool bForce = false)=0;
	virtual bool HasNodeFailed(const uint8_t nodeID) = 0;
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



