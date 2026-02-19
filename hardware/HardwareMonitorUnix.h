#pragma once

#include "HardwareMonitor.h"

#if defined(__linux__) || defined(__CYGWIN32__) || defined(__FreeBSD__) || defined(__OpenBSD__)

class CHardwareMonitorUnix : public CHardwareMonitorBase
{
public:
	explicit CHardwareMonitorUnix(int ID);
	~CHardwareMonitorUnix() override = default;

protected:
	void FetchData() override;
	void FetchCPU() override;
	void FetchMemory() override;
	void FetchDisk() override;
	void CheckForOnboardSensors() override;
	bool IsWSL() override;

private:
	void FetchUnixCPU();
	void FetchUnixMemory();
	void FetchUnixDisk();
	double time_so_far();

	void GetInternalTemperature();
	void GetInternalVoltage();
	void GetInternalCurrent();

#if defined(__linux__)
	float GetProcessMemUsage();
#endif
#if defined(__linux__) || defined(__FreeBSD__)
	float GetMemUsageLinux();
#endif
#if defined(__FreeBSD__) || defined(__OpenBSD__)
	float GetMemUsageOpenBSD();
#endif

	std::string m_dfcommand;

	bool m_bHasInternalTemperature = false;
	std::string m_szInternalTemperatureCommand;

	bool m_bHasInternalVoltage = false;
	std::string m_szInternalVoltageCommand;

	bool m_bHasInternalCurrent = false;
	std::string m_szInternalCurrentCommand;
};

#endif // defined(__linux__) || defined(__CYGWIN32__) || defined(__FreeBSD__) || defined(__OpenBSD__)
