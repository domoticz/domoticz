#pragma once
#include "../main/RFXtrx.h" 
#if defined WIN32 
	// for windows system info
	#include <wbemidl.h>
	#pragma comment(lib, "wbemuuid.lib")
#endif

class MainWorker;

class CHardwareMonitor
{
public:
	CHardwareMonitor(void);
	~CHardwareMonitor(void);
	void StartHardwareMonitor(MainWorker *pMainWorker);
	void StopHardwareMonitor();
private:
	bool m_bEnabled;
	MainWorker *m_pMain;
	int hwId;
	double m_lastquerytime;
	void Do_Work();	
	volatile bool m_stoprequested;
	boost::shared_ptr<boost::thread> m_thread;
	void Init();
	void FetchData();
	void UpdateSystemSensor(const std::string& qType, const std::string& wmiId, const std::string& devName, const std::string& devValue);
#ifdef WIN32
	void InitWMI();
	void ExitWMI();
	bool IsOHMRunning();
	void RunWMIQuery(const char* qTable, const char* qType);
	IWbemLocator *pLocator; 
	IWbemServices *pServicesOHM;
	IWbemServices *pServicesSystem;
#else
	void FetchUnixData();
	long long m_lastloadcpu;
	int m_totcpu;
#endif
};

