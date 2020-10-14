#pragma once

#include "DomoticzHardware.h"

#if defined WIN32
	// for windows system info
	#include <wbemidl.h>
	#pragma comment(lib, "wbemuuid.lib")
#endif

class CHardwareMonitor : public CDomoticzHardwareBase
{
public:
	enum nOSType
	{
		OStype_Unknown = 0,
		OStype_Linux = 1,
		OStype_Rpi = 2,
		OStype_WSL = 3,
		OStype_CYGWIN = 4,
		OStype_FreeBSD = 8,
		OStype_OpenBSD = 9,
		OStype_Windows = 14,
		OStype_Apple = 15
	};

	explicit CHardwareMonitor(const int ID);
	~CHardwareMonitor(void);
	bool WriteToHardware(const char* /*pdata*/, const unsigned char /*length*/) override { return false; };
	bool GetOSType(nOSType &OStype);
	std::string TranslateOSTypeToString(nOSType);
private:
	bool StartHardware() override;
	bool StopHardware() override;
	double m_lastquerytime;
	std::shared_ptr<std::thread> m_thread;
	nOSType m_OStype;

	void Do_Work();
	void FetchData();
	void FetchCPU();
	void FetchMemory();
	void FetchDisk();
	void GetInternalTemperature();
	void FetchClockSpeeds();
	void GetInternalARMClockSpeed();
	void GetInternalV3DClockSpeed();
	void GetInternalCoreClockSpeed();
	void GetInternalVoltage();
	void GetInternalCurrent();
	void CheckForOnboardSensors();
	void UpdateSystemSensor(const std::string& qType, const int dindex, const std::string& devName, const std::string& devValue);
	void SendCurrent(const unsigned long Idx, const float Curr, const std::string &defaultname);
	bool IsWSL();

	struct _tDUsageStruct
	{
		std::string MountPoint;
		long long TotalBlocks;
		long long UsedBlocks;
		long long AvailBlocks;
	};

	long long m_lastloadcpu;
	int m_totcpu;
	std::string m_dfcommand;

	bool bHasInternalTemperature;
	std::string szInternalTemperatureCommand;

	bool bHasInternalClockSpeeds;
	std::string szInternalARMSpeedCommand;
	std::string szInternalV3DSpeedCommand;
	std::string szInternalCoreSpeedCommand;

	bool bHasInternalVoltage;
	std::string szInternalVoltageCommand;

	bool bHasInternalCurrent;
	std::string szInternalCurrentCommand;

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
	double time_so_far();
#if defined (__linux__)
	float GetProcessMemUsage();
#endif
#if defined (__linux__) || defined (__FreeBSD__)
	float GetMemUsageLinux();
#endif
#if defined (__FreeBSD__) || defined(__OpenBSD__)
	float GetMemUsageOpenBSD();
#endif
#endif
};
