#pragma once

#include <map>
#include <time.h>
#include "DomoticzHardware.h"

class ZWaveBase : public CDomoticzHardwareBase
{
	friend class CRazberry;
	friend class COpenZWave;

	enum _eZWaveDeviceType
	{
		ZDTYPE_SWITCHNORMAL = 0,
		ZDTYPE_SWITCHDIMMER,
		ZDTYPE_SENSOR_POWER,
		ZDTYPE_SENSOR_TEMPERATURE,
		ZDTYPE_SENSOR_HUMIDITY,
		ZDTYPE_SENSOR_LIGHT,
		ZDTYPE_SENSOR_SETPOINT,
		ZDTYPE_SENSOR_POWERENERGYMETER,
	};
	struct _tZWaveDevice
	{
		int nodeID;
		int instanceID;
		int commandClassID;
		_eZWaveDeviceType devType;
		int scaleID;
		int scaleMultiply;
		int basicType;
		int genericType;
		int specificType;
		bool isListening;
		bool sensor250;
		bool sensor1000;
		bool isFLiRS;
		bool hasWakeup;
		bool hasBattery;

		//values
		float floatValue;
		int intvalue;
		bool bValidValue;

		//battery
		int batValue;

		//main_id
		std::string string_id;

		time_t lastreceived;
		unsigned char sequence_number;

		_tZWaveDevice()
		{
			nodeID=-1;
			scaleID=1;
			scaleMultiply=1;
			isListening=false;
			sensor250=false;
			sensor1000=false;
			isFLiRS=false;
			hasWakeup=false;
			hasBattery=false;
			floatValue=0;
			intvalue=0;
			bValidValue=true;
		}
	};
public:
	ZWaveBase();
	~ZWaveBase(void);

	virtual bool GetInitialDevices()=0;
	virtual bool GetUpdates()=0;
	bool StartHardware();
	bool StopHardware();
	void WriteToHardware(const char *pdata, const unsigned char length);
private:
	void Do_Work();
	void SendDevice2Domoticz(const _tZWaveDevice *pDevice);
	void SendSwitchIfNotExists(const _tZWaveDevice *pDevice);
	_tZWaveDevice* FindDevice(int nodeID, int instanceID, _eZWaveDeviceType devType);
	_tZWaveDevice* FindDevice(int nodeID, int instanceID, int CommandClassID, _eZWaveDeviceType devType);
	_tZWaveDevice* FindDevice(int nodeID, int scaleID);
	_tZWaveDevice* FindDeviceInstance(int nodeID, int instanceID);
	std::string GenerateDeviceStringID(const _tZWaveDevice *pDevice);
	void InsertDevice(_tZWaveDevice device);
	void UpdateDeviceBatteryStatus(const int nodeID, const int value);
	unsigned char Convert_Battery_To_PercInt(const unsigned char level);
	virtual void SwitchLight(const int nodeID, const int instanceID, const int commandClass, const int value)=0;
	virtual void SetThermostatSetPoint(const int nodeID, const int instanceID, const int commandClass, const float value)=0;
	virtual void StopHardwareIntern()=0;
	virtual bool IncludeDevice()=0;
	virtual bool ExcludeDevice(const int nodeID)=0;
	virtual bool RemoveFailedDevice(const int nodeID)=0;
	virtual bool CancelControllerCommand()=0;

	bool m_bControllerCommandInProgress;
	time_t m_ControllerCommandStartTime;
	int m_LastIncludedNode;
	time_t m_updateTime;
	bool m_bInitState;
	std::map<std::string,_tZWaveDevice> m_devices;
	boost::shared_ptr<boost::thread> m_thread;
	bool m_stoprequested;
};



