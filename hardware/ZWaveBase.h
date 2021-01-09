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
		ZDTYPE_SENSOR_KVAH,
		ZDTYPE_SENSOR_KVAR,
		ZDTYPE_SENSOR_KVARH,
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
		ZDTYPE_SENSOR_LOUDNESS,

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

		// values
		bool bValidValue;

		float floatValue;
		int intvalue;

		time_t lastSendValue = 0;
		float prevFloatValue = 0;
		int prevIntValue = 0;

		// battery
		int batValue;

		// main_id
		std::string string_id;

		// label
		std::string label;
		std::string custom_label;

		time_t lastreceived{ 0 };
		unsigned char sequence_number;

		int Alarm_Type;

		_tZWaveDevice()
			: label("Unknown")
		{
			sequence_number = 1;
			nodeID = (uint8_t)-1;
			scaleMultiply = 1.0f;
			isListening = false;
			hasWakeup = false;
			batValue = 255;
			floatValue = 0;
			intvalue = 0;
			bValidValue = true;
			commandClassID = 0;
			instanceID = 0;
			indexID = 0;
			orgInstanceID = 0;
			orgIndexID = 0;
			devType = ZDTYPE_SWITCH_NORMAL;
			basicType = 0;
			Manufacturer_id = (uint16_t)-1;
			Product_id = (uint16_t)-1;
			Product_type = (uint16_t)-1;
			Alarm_Type = -1;
		}
	};

      public:
	ZWaveBase();
	~ZWaveBase() override = default;

	virtual bool GetInitialDevices() = 0;
	virtual bool GetUpdates() = 0;
	bool StartHardware() override;
	bool StopHardware() override;
	bool WriteToHardware(const char *pdata, unsigned char length) override;

      public:
	bool m_bNodeReplaced;
	uint8_t m_NodeToBeReplaced;

	bool m_bHasNodeFailedDone;
	std::string m_sHasNodeFailedResult;
	uint8_t m_HasNodeFailedIdx;

	int m_LastIncludedNode;
	std::string m_LastIncludedNodeType;
	bool m_bHaveLastIncludedNodeInfo;
	int m_LastRemovedNode;
	std::mutex m_NotificationMutex;

      private:
	void Do_Work();
	void SendDevice2Domoticz(_tZWaveDevice *pDevice);
	void SendSwitchIfNotExists(const _tZWaveDevice *pDevice);

	_tZWaveDevice *FindDevice(uint8_t nodeID, int instanceID, _eZWaveDeviceType devType);
	_tZWaveDevice *FindDevice(uint8_t nodeID, int instanceID, uint8_t CommandClassID, _eZWaveDeviceType devType);

	std::string GenerateDeviceStringID(const _tZWaveDevice *pDevice);
	void InsertDevice(_tZWaveDevice device);
	unsigned char Convert_Battery_To_PercInt(unsigned char level);
	virtual bool SwitchLight(_tZWaveDevice *pDevice, int instanceID, int value) = 0;
	virtual bool SwitchColor(uint8_t nodeID, uint8_t instanceID, const std::string &ColorStr) = 0;
	virtual void SetThermostatSetPoint(uint8_t nodeID, uint8_t instanceID, uint8_t commandClass, float value) = 0;
	virtual void SetClock(uint8_t nodeID, uint8_t instanceID, uint8_t commandClass, uint8_t day, uint8_t hour, uint8_t minute) = 0;
	virtual void SetThermostatMode(uint8_t nodeID, uint8_t instanceID, uint8_t commandClass, int tMode) = 0;
	virtual void SetThermostatFanMode(uint8_t nodeID, uint8_t instanceID, uint8_t commandClass, int fMode) = 0;
	virtual std::string GetSupportedThermostatFanModes(unsigned long ID) = 0;
	virtual void StopHardwareIntern() = 0;
	virtual bool IncludeDevice(bool bSecure) = 0;
	virtual bool ExcludeDevice(uint8_t nodeID) = 0;
	virtual bool RemoveFailedDevice(uint8_t nodeID) = 0;
	virtual bool CancelControllerCommand(bool bForce = false) = 0;
	virtual bool HasNodeFailed(uint8_t nodeID) = 0;
	virtual bool IsNodeIncluded() = 0;
	virtual bool IsNodeExcluded() = 0;

	bool m_bControllerCommandInProgress;
	bool m_bControllerCommandCanceled;
	time_t m_ControllerCommandStartTime{ 0 };
	time_t m_updateTime{ 0 };
	bool m_bInitState;
	std::map<std::string, _tZWaveDevice> m_devices;
	std::shared_ptr<std::thread> m_thread;
};
