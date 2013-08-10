#pragma once

#include <map>
#include <time.h>
#include "DomoticzHardware.h"

namespace Json
{
	class Value;
};

class CRazberry : public CDomoticzHardwareBase
{
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
		std::string valueType;
		float floatValue;
		int intvalue;

		//battery
		int batValue;

		//main_id
		std::string string_id;

		time_t lastreceived;
		unsigned char sequence_number;
	};
public:
	CRazberry(const int ID, const std::string &ipaddress, const int port, const std::string &username, const std::string &password);
	~CRazberry(void);

	bool GetInitialDevices();
	bool GetUpdates();
	bool StartHardware();
	bool StopHardware();
	void WriteToHardware(const char *pdata, const unsigned char length);

private:
	const std::string GetControllerURL();
	const std::string GetRunURL(const std::string &cmd);
	void parseDevices(const Json::Value &devroot);
	void InsertOrUpdateDevice(_tZWaveDevice device, const bool bSend2Domoticz);
	void UpdateDevice(const std::string &path, const Json::Value &obj);
	void Do_Work();
	void SendDevice2Domoticz(const _tZWaveDevice *pDevice);
	_tZWaveDevice* FindDevice(int nodeID, int instanceID, _eZWaveDeviceType devType);
	_tZWaveDevice* FindDevice(int nodeID, int scaleID);
	void UpdateDeviceBatteryStatus(int nodeID, int value);
	void RunCMD(const std::string &cmd);

	bool m_bInitState;
	std::string m_ipaddress;
	int m_port;
	std::string m_username;
	std::string m_password;
	std::map<std::string,_tZWaveDevice> m_devices;
	time_t m_updateTime;
	int m_controllerID;
	boost::shared_ptr<boost::thread> m_thread;
	bool m_stoprequested;

};



