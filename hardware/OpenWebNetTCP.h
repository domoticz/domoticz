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
	bool SetSetpoint(const int idx, const float temp); 

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
	void UpdateSetPoint(const int who, const int where, float fval, const char *devname);
    void UpdateDeviceValue(vector<bt_openwebnet>::iterator iter);
    void scan_automation_lighting(const int cen_area);
    void scan_temperature_control();
    void scan_device();
    void requestTime();
    void setTime();
    void requestBurglarAlarmStatus();
	void requestDryContactIRDetectionStatus();
};
