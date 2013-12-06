#pragma once

#include "DomoticzHardware.h"
#include "../main/RFXtrx.h" //for ptypetemp, needs to go
#include <string>
#include <vector>
#if defined WIN32 
// for windows system info
#include <wbemidl.h>
#pragma comment(lib, "wbemuuid.lib")
#endif

class MainWorker;

class COpenHardwareMonitor
{
public:
	COpenHardwareMonitor(void);
	~COpenHardwareMonitor(void);
	void fetchOpenHardwareMonitorData();
	void Do_Work();	
	void StartOpenHardwareMonitor(MainWorker *pMainWorker);
	void StopOpenHardwareMonitor();
private:
	MainWorker *m_pMain;
	volatile bool m_stoprequested;
	boost::shared_ptr<boost::thread> m_thread;
	void Init();
	int hwId;
	void GetWinCpuTemperature();
	void UpdateSystemSensor(const std::string& wmiId, const std::string& devName, const float& devValue);
	void WriteMessageStart();
	void WriteMessage(const char *szMessage);
	void WriteMessage(const char *szMessage, bool linefeed);
	void WriteMessageEnd();
};

