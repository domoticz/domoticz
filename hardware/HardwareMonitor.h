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

	explicit CHardwareMonitor(int ID);
	~CHardwareMonitor() override;
	bool WriteToHardware(const char* /*pdata*/, const unsigned char /*length*/) override
	{
		return false;
	};
	bool GetOSType(nOSType& OStype);
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
	void GetInternalVoltage();
	void GetInternalCurrent();
	void CheckForOnboardSensors();
	void UpdateSystemSensor(const std::string& qType, int dindex, const std::string& devName, const std::string& devValue);
	void SendCurrent(unsigned long Idx, float Curr, const std::string& defaultname);
	bool IsWSL();

	struct _tDUsageStruct
	{
		std::string MountPoint;
		int64_t TotalBlocks;
		int64_t UsedBlocks;
		int64_t AvailBlocks;
	};

	int64_t m_lastloadcpu;
	int m_totcpu;
	std::string m_dfcommand;

	bool m_bHasInternalTemperature;
	std::string m_szInternalTemperatureCommand;

	bool m_bHasInternalVoltage;
	std::string m_szInternalVoltageCommand;

	bool m_bHasInternalCurrent;
	std::string m_szInternalCurrentCommand;

#ifdef WIN32
	bool InitWMI();
	void ExitWMI();
	bool IsHMRunning();
	void RunWMIQuery(const char* qTable, const std::string& qType);
	IWbemLocator* m_pLocator;
	IWbemServices* m_pServicesHM;
	IWbemServices* m_pServicesSystem;
	bool m_bIsLibreHardwareMonitor = false;
#elif defined(__linux__) || defined(__CYGWIN32__) || defined(__FreeBSD__) || defined(__OpenBSD__)
	void FetchUnixCPU();
	void FetchUnixMemory();
	void FetchUnixDisk();
	double time_so_far();
#if defined(__linux__)
	float GetProcessMemUsage();
#endif
#if defined(__linux__) || defined(__FreeBSD__)
	float GetMemUsageLinux();
#endif
#if defined(__FreeBSD__) || defined(__OpenBSD__)
	float GetMemUsageOpenBSD();
#endif
#endif
};
