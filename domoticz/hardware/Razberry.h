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
	void RunCMD(const std::string &cmd);
	void StopHardwareIntern();

	std::string m_ipaddress;
	int m_port;
	std::string m_username;
	std::string m_password;
	int m_controllerID;
};



