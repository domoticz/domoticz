#pragma once

#include <map>
#include <time.h>
#include "ZWaveBase.h"

namespace Json
{
	class Value;
};

class CRazberry : public ZWaveBase
{
public:
	CRazberry(const int ID, const std::string &ipaddress, const int port, const std::string &username, const std::string &password);
	~CRazberry(void);

	bool GetInitialDevices();
	bool GetUpdates();
private:
	const std::string GetControllerURL();
	const std::string GetRunURL(const std::string &cmd);
	void parseDevices(const Json::Value &devroot);
	void UpdateDevice(const std::string &path, const Json::Value &obj);

	_tZWaveDevice* FindDeviceByScale(const int nodeID, const int scaleID, const int cmdID);
	_tZWaveDevice* FindDeviceInstance(const int nodeID, const int instanceID, const int cmdID);

	bool SwitchLight(const int nodeID, const int instanceID, const int commandClass, const int value);
	bool SwitchColor(const int nodeID, const int instanceID, const int commandClass, const std::string &ColorStr);
	void SetThermostatSetPoint(const int nodeID, const int instanceID, const int commandClass, const float value);
	void SetClock(const int nodeID, const int instanceID, const int commandClass, const int day, const int hour, const int minute);
	void SetThermostatMode(const int nodeID, const int instanceID, const int commandClass, const int tMode);
	void SetThermostatFanMode(const int nodeID, const int instanceID, const int commandClass, const int fMode);
	std::string GetSupportedThermostatModes(const unsigned long ID);
	std::string GetSupportedThermostatFanModes(const unsigned long ID);

	bool HasNodeFailed(const int nodeID);

	void RunCMD(const std::string &cmd);
	void StopHardwareIntern();
	bool IncludeDevice(const bool bSecure);
	bool ExcludeDevice(const int nodeID);
	bool RemoveFailedDevice(const int nodeID);
	bool CancelControllerCommand();
	std::string m_ipaddress;
	int m_port;
	std::string m_username;
	std::string m_password;
	int m_controllerID;
};



