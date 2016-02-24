#pragma once

#include "DomoticzHardware.h"
#include "csocket.h"

using namespace std;
class bt_openwebnet;

class COpenWebNet : public CDomoticzHardwareBase
{
public:
	COpenWebNet(const int ID, const std::string &IPAddress, const unsigned short usIPPort);
	~COpenWebNet(void);

	enum _eWho {
		WHO_SCENARIO = 0,
		WHO_LIGHTING=1,
		WHO_AUTOMATION=2,
		WHO_LOAD_CONTROL=3,
		WHO_TEMPERATURE_CONTROL = 4,
		WHO_BURGLAR_ALARM = 5,
		WHO_DOOR_ENTRY_SYSTEM = 6,
		WHO_MULTIMEDIA = 7,
		WHO_AUXILIARY = 9,
		WHO_GATEWAY_INTERFACES_MANAGEMENT = 13,
		WHO_LIGHT_SHUTTER_ACTUATOR_LOCK = 14,
		WHO_SCENARIO_SCHEDULER_SWITCH = 15,
		WHO_AUDIO = 16,
		WHO_SCENARIO_PROGRAMMING = 17,
		WHO_ENERGY_MANAGEMENT = 18,
		WHO_LIHGTING_MANAGEMENT = 24,
		WHO_SCENARIO_SCHEDULER_BUTTONS = 25,
		WHO_DIAGNOSTIC = 1000,
		WHO_AUTOMATIC_DIAGNOSTIC = 1001,
		WHO_THERMOREGULATION_DIAGNOSTIC_FAILURES = 1004,
		WHO_DEVICE_DIAGNOSTIC = 1013
	};

	enum _eAutomationWhat {
		AUTOMATION_WHAT_STOP = 0,
		AUTOMATION_WHAT_UP = 1,
		AUTOMATION_WHAT_DOWN = 2
	};

	bool isStatusSocketConnected();
	bool WriteToHardware(const char *pdata, const unsigned char length);

	// signals
	boost::signals2::signal<void()>	sDisconnected;

protected:
	bool StartHardware();
	bool StopHardware(); 
	
	std::string m_szIPAddress;
	unsigned short m_usIPPort;

	void Do_Work();
	void MonitorFrames();
	boost::shared_ptr<boost::thread> m_monitorThread;
	boost::shared_ptr<boost::thread> m_heartbeatThread;
	volatile bool m_stoprequested;
	
	bool connectStatusSocket();
	int m_heartbeatcntr;
	csocket* m_pStatusSocket;

	bool write(const char *pdata, size_t size);
	bool sendCommand(bt_openwebnet& command, vector<bt_openwebnet>& response, int waitForResponse = 0, bool silent=false);
	bool ParseData(char* data, int length, vector<bt_openwebnet>& messages);
	bool FindDevice(int who, int where, int *used);
	bool AddDeviceIfNotExits(string who, string where);
	string frameToString(bt_openwebnet& frame);
	string getWhoDescription(string who);
	string getWhatDescription(string who, string what);
};
