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
	explicit CHardwareMonitor(const int ID);
	~CHardwareMonitor(void);
	bool WriteToHardware(const char* /*pdata*/, const unsigned char /*length*/) override { return false; };
#if defined (__linux__)
	float GetProcessMemUsage();
#endif
private:
	bool StartHardware() override;
	bool StopHardware() override;
	double m_lastquerytime;
	void Do_Work();
	volatile bool m_stoprequested;
	std::shared_ptr<std::thread> m_thread;
	void FetchData();
	void GetInternalTemperature();
	void GetInternalVoltage();
	void GetInternalCurrent();
	void UpdateSystemSensor(const std::string& qType, const int dindex, const std::string& devName, const std::string& devValue);
	void SendCurrent(const unsigned long Idx, const float Curr, const std::string &defaultname);
	void SendFanSensor(const int Idx, const int FanSpeed, const std::string &defaultname);
#ifdef WIN32
	bool InitWMI();
	void ExitWMI();
	bool IsOHMRunning();
	void RunWMIQuery(const char* qTable, const std::string &qType);
	IWbemLocator *m_pLocator;
	IWbemServices *m_pServicesOHM;
	IWbemServices *m_pServicesSystem;
#elif defined (__linux__) || defined(__CYGWIN32__) || defined(__FreeBSD__) || defined(__OpenBSD__)
	void FetchUnixCPU();
	void FetchUnixMemory();
	void FetchUnixDisk();
	long long m_lastloadcpu;
	int m_totcpu;
	std::string m_dfcommand;
#endif
};

