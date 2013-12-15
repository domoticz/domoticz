#pragma once
#include "../main/RFXtrx.h" 
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
	void InitWMI();
	int hwId;
	IWbemLocator *m_pLocator; 
	IWbemServices *m_pServicesOHM;
	IWbemServices *m_pServicesSystem;
	HRESULT hr;
	void FetchData();
	bool COpenHardwareMonitor::IsOHMRunning();
	void RunWMIQuery(const char* qTable, const char* qType);
	void UpdateSystemSensor(const std::string& qType, const std::string& wmiId, const std::string& devName, const std::string& devValue);
	void WriteMessageStart();
	void WriteMessage(const char *szMessage);
	void WriteMessage(const char *szMessage, bool linefeed);
	void WriteMessageEnd();
};

