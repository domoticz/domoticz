#pragma once

#include <map>
#include <time.h>
#include "../json/json.h"
#include "DomoticzHardware.h"

enum _eZWaveDeviceType
{
	ZDTYPE_SWITCHNORMAL = 0,
	ZDTYPE_SWITCHDIMMER,
	ZDTYPE_SENSOR_POWER,
};

class CRazberry : public CDomoticzHardwareBase
{
	struct _tZWaveDevice
	{
		int nodeID;
		int instanceID;
		int commandClassID;
		_eZWaveDeviceType devType;
		int scaleID;
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

		//main_id
		std::string string_id;

		time_t lastreceived;
		unsigned char sequence_number;
	};
public:
	CRazberry(const int ID, const std::string ipaddress, const int port, const std::string username, const std::string password);
	~CRazberry(void);

	bool GetInitialDevices();
	bool GetUpdates();
	bool StartHardware();
	bool StopHardware();
	void WriteToHardware(const char *pdata, const unsigned char length);

private:
	const std::string GetControllerURL();
	const std::string GetRunURL(const std::string cmd);
	void parseDevices(const Json::Value devroot);
	void InsertOrUpdateDevice(_tZWaveDevice device, const bool bSend2Domoticz);
	void UpdateDevice(const std::string path, const Json::Value obj);
	void Do_Work();
	void SendDevice2Domoticz(const _tZWaveDevice *pDevice);
	const _tZWaveDevice* FindDevice(int nodeID, int instanceID, _eZWaveDeviceType devType);
	void RunCMD(const std::string cmd);

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

