#pragma once

#include "DomoticzHardware.h"
#include "csocket.h"

using namespace std;
class bt_openwebnet;

class COpenWebNetTCP : public CDomoticzHardwareBase
{
public:
	COpenWebNetTCP(const int ID, const std::string &IPAddress, const unsigned short usIPPort, const std::string &ownPassword);
	~COpenWebNetTCP(void);

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
		WHO_CEN_PLUS_DRY_CONTACT_IR_DETECTION = 25,
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

	enum _eLightWhat {
		LIGHT_WHAT_OFF = 0,
		LIGHT_WHAT_ON = 1
	};

	enum _eAuxiliaryWhat {
        AUXILIARY_WHAT_OFF = 0,
        AUXILIARY_WHAT_ON = 1
	};

	enum _eDryContactIrDetectionWhat {
		DRY_CONTACT_IR_DETECTION_WHAT_ON = 31,
		DRY_CONTACT_IR_DETECTION_WHAT_OFF = 32
	};

	enum _eArea {
		WHERE_CEN_0 = 0,
		WHERE_AREA_1 = 1,
		WHERE_AREA_2 = 2,
		WHERE_AREA_3 = 3,
		WHERE_AREA_4 = 4,
		WHERE_AREA_5 = 5,
		WHERE_AREA_6 = 6,
		WHERE_AREA_7 = 7,
		WHERE_AREA_8 = 8,
		WHERE_AREA_9 = 9,
		MAX_WHERE_AREA =10
		/*
		TODO: with virtual configuration are present PL [10 - 15]
		      but in this case need to develop all this sending rules:

			  - A = 00;			PL [01 - 15];
			  - A [1 -9];		PL [1 - 9];
			  - A = 10;			PL [01 - 15];
			  - A [01 - 09];	PL [10 - 15];

			  */
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
    std::string m_ownPassword;

	time_t LastScanTime;

	void Do_Work();
	void MonitorFrames();
	boost::shared_ptr<boost::thread> m_monitorThread;
	boost::shared_ptr<boost::thread> m_heartbeatThread;
	volatile bool m_stoprequested;
    volatile uint32_t mask_request_status;
    uint32_t ownCalcPass(string password, string nonce);
    bool nonceHashAuthentication(csocket *connectionSocket);
	csocket* connectGwOwn(const char *connectionMode);
	void disconnect();
	int m_heartbeatcntr;
	csocket* m_pStatusSocket;

	bool write(const char *pdata, size_t size);
	bool sendCommand(bt_openwebnet& command, vector<bt_openwebnet>& response, int waitForResponse = 0, bool silent=false);
	bool ParseData(char* data, int length, vector<bt_openwebnet>& messages);
	bool FindDevice(int who, int where, int iInterface, int *used);
    void UpdateSwitch(const int who, const int where, const int Level, int iInterface, const int BatteryLevel,const char *devname, const int subtype);
    void UpdateBlinds(const int who, const int where, const int Command, int iInterface, const int BatteryLevel, const char *devname);
    void UpdateAlarm(const int who, const int where, const int Command, const char *sCommand, int iInterface, const int BatteryLevel, const char *devname);
		void UpdateSensorAlarm(const int who, const int where, const int Command, const char *sCommand, int iInterface, const int BatteryLevel, const char *devname);
    void UpdateCenPlus(const int who, const int where, const int Command, const int iAppValue, int iInterface, const int BatteryLevel, const char *devname);
    void UpdateTemp(const int who, const int where, float fval, const int BatteryLevel, const char *devname);
    void UpdateDeviceValue(vector<bt_openwebnet>::iterator iter);
    void scan_automation_lighting(const int cen_area);
    void scan_temperature_control();
    void scan_device();
    void requestTime();
    void setTime();
    void requestBurglarAlarmStatus();
	void requestDryContactIRDetectionStatus();
};
