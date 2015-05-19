#pragma once
//#include "../main/RFXtrx.h" 
#include "DomoticzHardware.h"
#if defined WIN32 
	// for windows system info
	#include <wbemidl.h>
	#pragma comment(lib, "wbemuuid.lib")
#endif

class CHardwareMonitor : public CDomoticzHardwareBase
{
public:
	CHardwareMonitor(void);
	~CHardwareMonitor(void);
	void StartHardwareMonitor();
	void StopHardwareMonitor();
	bool WriteToHardware(const char *pdata, const unsigned char length) { return false; };
private:
	bool m_bEnabled;
	double m_lastquerytime;
	void Do_Work();	
	volatile bool m_stoprequested;
	boost::shared_ptr<boost::thread> m_thread;
	void Init();
	void FetchData();
	void GetInternalTemperature();
	void GetInternalVoltage();
	void GetInternalCurrent();
	void UpdateSystemSensor(const std::string& qType, const int dindex, const std::string& devName, const std::string& devValue);
	void SendVoltage(const unsigned long Idx, const float Volt, const std::string &defaultname);
	void SendCurrent(const unsigned long Idx, const float Curr, const std::string &defaultname);
	void SendTempSensor(const int Idx, const float Temp, const std::string &defaultname);
	void SendPercentage(const unsigned long Idx, const float Percentage, const std::string &defaultname);
	void SendFanSensor(const int Idx, const int FanSpeed, const std::string &defaultname);
	bool StartHardware() { return true; };
	bool StopHardware() { return true; };
#ifdef WIN32
	void InitWMI();
	void ExitWMI();
	bool IsOHMRunning();
	void RunWMIQuery(const char* qTable, const char* qType);
	IWbemLocator *pLocator; 
	IWbemServices *pServicesOHM;
	IWbemServices *pServicesSystem;
#elif defined __linux__
	void FetchUnixData();
	long long m_lastloadcpu;
	int m_totcpu;
#endif
};

