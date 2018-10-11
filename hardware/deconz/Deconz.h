#pragma once

#include "../DomoticzHardware.h"
#include "DeconzLight.h"
#include "DeconzSensor.h"

namespace Json {
	class Value;
}

#define DECONZ "deCONZ"
#define DECONZ_DEFAULT_POLL_INTERVAL 15

class Deconz : public CDomoticzHardwareBase
{
public:
	Deconz(const int id, const std::string &ipAddress, const unsigned short port, const std::string &username, const int poll, const int options);
	~Deconz(void);
	bool WriteToHardware(const char *pdata, const unsigned char length) override;
	static std::string RegisterUser(const std::string &IPAddress, const unsigned short Port, const std::string &username);
private:
	void Init();
	bool StartHardware() override;
	bool StopHardware() override;
	void Do_Work();
	bool GetStates();
	bool GetLights(const Json::Value &root);	
	bool GetSensors(const Json::Value &root);
	bool SwitchLight(const int nodeID, const std::string &LCmd, const int svalue, const int svalue2 = 0, const int svalue3 = 0);
	void InsertUpdateSwitch(int NodeID, _eSwitchType SType, bool status, const std::string& Name, uint8_t BatteryLevel) const;
	void InsertUpdateSwitch(const int NodeID, const DeconzLight &light, const std::string& Options, const bool AddMissingDevice);

	int poll_interval;	
	std::string ipAddress;
	unsigned short port;
	std::string username;
	std::shared_ptr<std::thread> thread;
	std::map<int, DeconzLight> lights;	
	std::map<int, DeconzSensor> sensors;
};


