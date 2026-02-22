#include "stdafx.h"

#if defined(__linux__) || defined(__CYGWIN32__) || defined(__FreeBSD__) || defined(__OpenBSD__)

#include "HardwareMonitorUnix.h"
#include "../main/Helper.h"
#include "../main/Logger.h"
#include "../main/mainworker.h"
#include "../main/SQLHelper.h"
#include <wchar.h>

#define __STDC_FORMAT_MACROS
#include <inttypes.h>

// Unix-specific includes
#if !defined(__FreeBSD__) && !defined(__OpenBSD__)
#include <sys/sysinfo.h>
#endif
#ifdef __OpenBSD__
#include <sys/sysctl.h>
#include <sys/sched.h>
#include <sys/vmmeter.h>
#endif
#include <iostream>
#include <fstream>
#include <string>
#include <limits>
#include <unistd.h>
#include <fcntl.h>

//USER_HZ detection, from openssl code
#ifndef HZ
# if defined(_SC_CLK_TCK) && (!defined(OPENSSL_SYS_VMS) || __CTRL_VER >= 70000000)
#  define HZ ((double)sysconf(_SC_CLK_TCK))
# else
#  ifndef CLK_TCK
#   ifndef _BSD_CLK_TCK_ /* FreeBSD hack */
#    define HZ  100.0
#   else /* _BSD_CLK_TCK_ */
#    define HZ ((double)_BSD_CLK_TCK_)
#   endif
#  else /* CLK_TCK */
#   define HZ ((double)CLK_TCK)
#  endif
# endif
#endif

CHardwareMonitorUnix::CHardwareMonitorUnix(const int ID)
	: CHardwareMonitorBase(ID)
{
}

void CHardwareMonitorUnix::FetchData()
{
	Debug(DEBUG_NORM, "Fetching *NIX sensor data (System sensors)");

	if (m_bHasInternalTemperature)
		GetInternalTemperature();

	if (m_bHasInternalVoltage)
		GetInternalVoltage();

	if (m_bHasInternalCurrent)
		GetInternalCurrent();
}

void CHardwareMonitorUnix::FetchCPU()
{
	FetchUnixCPU();
}

void CHardwareMonitorUnix::FetchMemory()
{
	FetchUnixMemory();
}

void CHardwareMonitorUnix::FetchDisk()
{
	FetchUnixDisk();
}

void CHardwareMonitorUnix::GetInternalTemperature()
{
	Debug(DEBUG_NORM, "Getting Internal Temperature");
	int returncode = 0;
	std::vector<std::string> ret = ExecuteCommandAndReturn(m_szInternalTemperatureCommand, returncode);
	if (ret.empty())
		return;
	std::string tmpline = ret[0];
	if (tmpline.find("temp=") == std::string::npos)
		return;
	tmpline = tmpline.substr(5);
	size_t pos = tmpline.find('\'');
	if (pos != std::string::npos)
	{
		tmpline = tmpline.substr(0, pos);
	}

	float temperature = static_cast<float>(atof(tmpline.c_str()));
	if (temperature == 0)
		return; //hardly possible for a on board temp sensor, if it is, it is probably not working

	if ((temperature != 85) && (temperature != -127) && (temperature > -273))
	{
		SendTempSensor(1, 255, temperature, "Internal Temperature");
	}
}

void CHardwareMonitorUnix::GetInternalVoltage()
{
	Debug(DEBUG_NORM, "Getting Internal Voltage");
	int returncode = 0;
	std::vector<std::string> ret = ExecuteCommandAndReturn(m_szInternalVoltageCommand, returncode);
	if (ret.empty())
		return;
	std::string tmpline = ret[0];
	if (tmpline.find("volt=") == std::string::npos)
		return;
	tmpline = tmpline.substr(5);
	size_t pos = tmpline.find('\'');
	if (pos != std::string::npos)
	{
		tmpline = tmpline.substr(0, pos);
	}

	float voltage = static_cast<float>(atof(tmpline.c_str()));
	if (voltage == 0)
		return; //hardly possible for a on board temp sensor, if it is, it is probably not working

	SendVoltageSensor(0, 1, 255, voltage, "Internal Voltage");
}

void CHardwareMonitorUnix::GetInternalCurrent()
{
	Debug(DEBUG_NORM, "Getting Internal Current");
	int returncode = 0;
	std::vector<std::string> ret = ExecuteCommandAndReturn(m_szInternalCurrentCommand, returncode);
	if (ret.empty())
		return;
	std::string tmpline = ret[0];
	if (tmpline.find("curr=") == std::string::npos)
		return;
	tmpline = tmpline.substr(5);
	size_t pos = tmpline.find('\'');
	if (pos != std::string::npos)
	{
		tmpline = tmpline.substr(0, pos);
	}

	float current = static_cast<float>(atof(tmpline.c_str()));
	if (current == 0)
		return; //hardly possible for a on board temp sensor, if it is, it is probably not working

	SendCurrent(1, current, "Internal Current");
}

double CHardwareMonitorUnix::time_so_far()
{
	struct timeval tp;
	if (gettimeofday(&tp, (struct timezone*)nullptr) == -1)
		return 0;
	return ((double)(tp.tv_sec)) +
		(((double)tp.tv_usec) * 0.000001);
}

#if defined(__linux__)
float CHardwareMonitorUnix::GetProcessMemUsage()
{
	pid_t pid = getpid();
	std::stringstream ssPidfile;
	ssPidfile << "/proc/" << pid << "/status";
	std::ifstream mfile(ssPidfile.str().c_str());
	if (!mfile.is_open())
		return -1;
	uint32_t VmRSS = -1;
	uint32_t VmSwap = -1;
	std::string token;
	while (mfile >> token)
	{
		if (token == "VmRSS:")
			mfile >> VmRSS;
		else if (token == "VmSwap:")
			mfile >> VmSwap;

		// ignore rest of the line
		mfile.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
	}
	return (VmRSS + VmSwap) / 1000.F;
}
#endif

float CHardwareMonitorUnix::GetMemUsageLinux()
{
#if defined(__FreeBSD__)
	std::ifstream mfile("/compat/linux/proc/meminfo");
#else	// Linux
	std::ifstream mfile("/proc/meminfo");
#endif
	if (!mfile.is_open())
		return -1;
	unsigned long MemTotal = -1;
	unsigned long MemFree = -1;
	unsigned long MemBuffers = -1;
	unsigned long  MemCached = -1;
	std::string token;
	while (mfile >> token) {
		if (token == "MemTotal:") {
			mfile >> MemTotal;
		}
		else if (token == "MemFree:") {
			mfile >> MemFree;
		}
		else if (token == "Buffers:") {
			mfile >> MemBuffers;
		}
		else if (token == "Cached:") {
			mfile >> MemCached;
		}
		// ignore rest of the line
		mfile.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
	}
	unsigned long MemUsed = MemTotal - MemFree - MemBuffers - MemCached;
	float memusedpercentage = (100.0F / float(MemTotal)) * MemUsed;
	return memusedpercentage;
}

#ifdef __OpenBSD__
float CHardwareMonitorUnix::GetMemUsageOpenBSD()
{
	int mibTotalMem[2] = {
		CTL_HW,
		HW_PHYSMEM64
	};
	int mibPageSize[2] = {
		CTL_HW,
		HW_PAGESIZE
	};
	int mibMemStats[2] = {
		CTL_VM,
		VM_METER
	};
	int pageSize;
	int64_t totalMemBytes, usedMem;
	size_t len = sizeof(totalMemBytes);
	float percent;
	struct vmtotal memStats;
	if (sysctl(mibTotalMem, 2, &totalMemBytes, &len, nullptr, 0) == -1)
	{
		return -1;
	}
	len = sizeof(pageSize);
	if (sysctl(mibPageSize, 2, &pageSize, &len, nullptr, 0) == -1)
	{
		return -1;
	}
	len = sizeof(memStats);
	if (sysctl(mibMemStats, 2, &memStats, &len, nullptr, 0) == -1)
	{
		return -1;
	}
	usedMem = memStats.t_arm * pageSize;//active real memory
	percent = (100.0f / float(totalMemBytes)) * usedMem;
	return percent;
}
#endif

void CHardwareMonitorUnix::FetchUnixMemory()
{
	//Memory
	char szTmp[300];
	float memusedpercentage = GetMemUsageLinux();
#ifndef __FreeBSD__
	if (memusedpercentage == -1)
	{
#ifdef __OpenBSD__
		memusedpercentage = GetMemUsageOpenBSD();
#else
		//old (wrong) way
		struct sysinfo mySysInfo;
		int ret = sysinfo(&mySysInfo);
		if (ret != 0)
			return;
		unsigned long usedram = mySysInfo.totalram - mySysInfo.freeram;
		memusedpercentage = (100.0F / float(mySysInfo.totalram)) * usedram;
#endif
	}
#endif
	sprintf(szTmp, "%.2f", memusedpercentage);
	UpdateSystemSensor("Load", 0, "Memory Usage", szTmp);
#ifdef __linux__
	float memProcess = GetProcessMemUsage();
	if (memProcess != -1)
	{
		sprintf(szTmp, "%.2f", memProcess);
		UpdateSystemSensor("Process", 0, "Process Usage", szTmp);
	}
#endif
}

void CHardwareMonitorUnix::FetchUnixCPU()
{
	//CPU
	char szTmp[300];
	char cname[50];
	if (m_lastquerytime == 0)
	{
#if defined(__OpenBSD__)
		//Get number of CPUs
		// sysctl hw.ncpu
		int mib[] = { CTL_HW, HW_NCPU };
		int totcpu = -1;
		size_t size = sizeof(totcpu);
		long loads[CPUSTATES];
		if (sysctl(mib, 2, &totcpu, &size, nullptr, 0) < 0)
		{
			Log(LOG_ERROR, "sysctl NCPU failed.");
			return;
		}
		m_lastquerytime = time_so_far();
		// In the emd there will be single value, so using
		// average loads doesn't generate that much error.
		mib[0] = CTL_KERN;
		mib[1] = KERN_CPTIME;
		size = sizeof(loads);
		if (sysctl(mib, 2, loads, &size, nullptr, 0) < 0)
		{
			Log(LOG_ERROR, "sysctl CPTIME failed.");
			return;
		}
		//Interrupts aren't measured.
		m_lastloadcpu = loads[CP_USER] + loads[CP_NICE] + loads[CP_SYS];
		m_totcpu = totcpu;
#else
		//first time
		m_lastquerytime = time_so_far();
		int actload1, actload2, actload3;
		int totcpu = -1;
#if defined(__FreeBSD__)
		FILE* fIn = fopen("/compat/linux/proc/stat", "r");
#else	// Linux
		FILE* fIn = fopen("/proc/stat", "r");
#endif
		if (fIn != nullptr)
		{
			bool bFirstLine = true;
			while (fgets(szTmp, sizeof(szTmp), fIn) != nullptr)
			{
				int ret = sscanf(szTmp, "%s\t%d\t%d\t%d\n", cname, &actload1, &actload2, &actload3);
				if ((bFirstLine) && (ret == 4)) {
					bFirstLine = false;
					m_lastloadcpu = actload1 + actload2 + actload3;
				}
				char* cPos = strstr(cname, "cpu");
				if (cPos == nullptr)
					break;
				totcpu++;
			}
			fclose(fIn);
		}
		if (totcpu < 1)
			m_lastquerytime = 0;
		else
			m_totcpu = totcpu;
#endif // else __OpenBSD__
	}
	else
	{
		double acttime = time_so_far();
#if defined(__OpenBSD__)
		int mib[] = { CTL_KERN, KERN_CPTIME };
		long loads[CPUSTATES];
		size_t size = sizeof(loads);
		if (sysctl(mib, 2, loads, &size, nullptr, 0) < 0)
		{
			Log(LOG_ERROR, "sysctl CPTIME failed.");
			return;
		}
		else
		{
			int64_t t = (loads[CP_USER] + loads[CP_NICE] + loads[CP_SYS]) - m_lastloadcpu;
			double cpuper = ((double(t) / (difftime(acttime, m_lastquerytime) * HZ)) * 100);///double(m_totcpu);
			if (cpuper > 0)
			{
				sprintf(szTmp, "%.2f", cpuper);
				UpdateSystemSensor("Load", 1, "CPU_Usage", szTmp);
			}
			m_lastloadcpu = loads[CP_USER] + loads[CP_NICE] + loads[CP_SYS];
		}
#else
		int actload1, actload2, actload3;
#if defined(__FreeBSD__)
		FILE* fIn = fopen("/compat/linux/proc/stat", "r");
#else	// Linux
		FILE* fIn = fopen("/proc/stat", "r");
#endif
		if (fIn != nullptr)
		{
			int ret = fscanf(fIn, "%s\t%d\t%d\t%d\n", cname, &actload1, &actload2, &actload3);
			fclose(fIn);
			if (ret == 4)
			{
				int64_t t = (actload1 + actload2 + actload3) - m_lastloadcpu;
				double cpuper = ((t / (difftime(acttime, m_lastquerytime) * HZ)) * 100) / double(m_totcpu);
				if (cpuper > 0)
				{
					sprintf(szTmp, "%.2f", cpuper);
					UpdateSystemSensor("Load", 1, "CPU_Usage", szTmp);
				}
				m_lastloadcpu = actload1 + actload2 + actload3;
			}
		}
#endif //else Openbsd
		m_lastquerytime = acttime;
	}
}

void CHardwareMonitorUnix::FetchUnixDisk()
{
	//Disk Usage
	std::map<std::string, _tDUsageStruct> _disks;
	std::map<std::string, std::string> _dmounts_;
	int returncode = 0;
	std::vector<std::string> _rlines = ExecuteCommandAndReturn(m_dfcommand, returncode);
	if (!_rlines.empty())
	{
		for (const auto& ittDF : _rlines)
		{
			char dname[200];
			char suse[30];
			char smountpoint[300];
			int64_t numblock, usedblocks, availblocks;
			int ret = sscanf(ittDF.c_str(), "%s\t%" PRId64 "\t%" PRId64 "\t%" PRId64 "\t%s\t%s\n", dname, &numblock, &usedblocks, &availblocks, suse, smountpoint);
			if (ret == 6)
			{
				auto it = _dmounts_.find(dname);
				if (it != _dmounts_.end())
				{
					if (it->second.length() < strlen(smountpoint))
					{
						continue;
					}
				}
#if defined(__linux__) || defined(__FreeBSD__) || defined (__OpenBSD__)
				if (strstr(dname, "/dev") != nullptr)
#elif defined(__CYGWIN32__)
				if (strstr(smountpoint, "/cygdrive/") != nullptr)
#endif
				{
					_tDUsageStruct dusage;
					dusage.TotalBlocks = numblock;
					dusage.UsedBlocks = usedblocks;
					dusage.AvailBlocks = availblocks;
					dusage.MountPoint = smountpoint;
					_disks[dname] = dusage;
					_dmounts_[dname] = smountpoint;
				}
			}
		}
		int dindex = 0;
		for (const auto& ittDisks : _disks)
		{
			_tDUsageStruct dusage = ittDisks.second;
			if (dusage.TotalBlocks > 0)
			{
				double UsagedPercentage = (100 / double(dusage.TotalBlocks)) * double(dusage.UsedBlocks);
				//std::cout << "Disk: " << ittDisks.first << ", Mount: " << dusage.MountPoint << ", Used: " << UsagedPercentage << std::endl;
				char szTmp[300];
				sprintf(szTmp, "%.2f", UsagedPercentage);
				std::string hddname = "HDD " + dusage.MountPoint;
				dindex=0;
				std::vector<std::vector<std::string> > listOfHdd;
				listOfHdd = m_sql.safe_query("SELECT ID, DeviceID, Name, Options FROM DeviceStatus WHERE (HardwareID=%d AND DeviceID>'0000044D' AND DeviceID<'000004B0')", m_HwdID);
				if (!listOfHdd.empty())
				{
					for (const auto& sd : listOfHdd)
					{
						std::string szIdx = sd[0];
						std::string szDeviceId = sd[1];
						std::string Name = sd[2];
						int deviceId;
						sscanf(szDeviceId.c_str(), "%x", &deviceId);
						std::string sOptions = sd[3];
						std::map<std::string, std::string> optionsMap = m_sql.BuildDeviceOptions(sOptions);
						if (!optionsMap.empty() and !strcmp(optionsMap["hdd"].c_str(),hddname.c_str()))
						{
							dindex=deviceId;
							break;
						}
						else
						{
							if (!strcmp(Name.c_str(),hddname.c_str()))
							{
								dindex=deviceId;
								uint64_t idx = std::stoull(szIdx);
								optionsMap["hdd"] = hddname;
								m_sql.SetDeviceOptions(idx, optionsMap);
								break;
							}
						}
					}
				}
				if (dindex==0)
				{
					// new HDD
					std::vector<std::vector<std::string> > listOfHdd;
					listOfHdd = m_sql.safe_query("SELECT ID, DeviceID, Name FROM DeviceStatus WHERE (HardwareID=%d AND DeviceID>'0000044D' AND DeviceID<'000004B0')", m_HwdID);
					int firstFreedeviceId=1102;
					if (!listOfHdd.empty())
					{
						for (int i=0; i<listOfHdd.size() ; i++)
						{
							for (const auto& sd : listOfHdd)
							{
								std::string szDeviceId = sd[1];
								int deviceId;
								sscanf(szDeviceId.c_str(), "%x", &deviceId);
								if (firstFreedeviceId==deviceId)
								{
									firstFreedeviceId++;
									break;
								}
							}
						}
					}
					dindex=firstFreedeviceId;
				}
				dindex-=1102;

				UpdateSystemSensor("Load", 2 + dindex, hddname, szTmp);
			}
		}
	}
}

bool CHardwareMonitorUnix::IsWSL()
{
	// Detect WSL according to https://github.com/Microsoft/WSL/issues/423#issuecomment-221627364
	bool is_wsl = false;

#if defined(__linux__)
	char buf[1024];

	int status_fd = open("/proc/sys/kernel/osrelease", O_RDONLY);
	if (status_fd == -1)
		return is_wsl;

	ssize_t num_read = read(status_fd, buf, sizeof(buf) - 1);
	close(status_fd);

	if (num_read > 0)
	{
		buf[num_read] = 0;
		is_wsl |= (strstr(buf, "Microsoft") != nullptr);
		is_wsl |= (strstr(buf, "WSL") != nullptr);
	}

	status_fd = open("/proc/version", O_RDONLY);
	if (status_fd == -1)
		return is_wsl;

	num_read = read(status_fd, buf, sizeof(buf) - 1);
	close(status_fd);

	if (num_read > 0)
	{
		buf[num_read] = 0;
		is_wsl |= (strstr(buf, "Microsoft") != nullptr);
		is_wsl |= (strstr(buf, "WSL") != nullptr);
	}
#endif

	return is_wsl;
}

void CHardwareMonitorUnix::CheckForOnboardSensors()
{
	Debug(DEBUG_NORM, "Checking for onboard sensors");

#if defined(__linux__) || defined(__CYGWIN32__) || defined(__FreeBSD__) || defined(__OpenBSD__)
	// Busybox df doesn't support -x parameter
	int returncode = 0;
	std::vector<std::string> ret = ExecuteCommandAndReturn("df -x nfs -x tmpfs -x devtmpfs 2> /dev/null", returncode);
	returncode == 0 ?
		m_dfcommand = "df -x nfs -x tmpfs -x devtmpfs" :
		m_dfcommand = "df";
#endif

#if defined(__linux__) || defined(__CYGWIN32__) || defined(__FreeBSD__)

	if (!m_bHasInternalTemperature)
	{
		if (file_exist("/sys/devices/platform/sunxi-i2c.0/i2c-0/0-0034/temp1_input"))
		{
			Log(LOG_STATUS, "System: Cubieboard/Cubietruck");
			m_szInternalTemperatureCommand = R"(cat /sys/devices/platform/sunxi-i2c.0/i2c-0/0-0034/temp1_input | awk '{ printf ("temp=%0.2f\n",$1/1000); }')";
			m_bHasInternalTemperature = true;
		}
		else if (file_exist("/sys/devices/virtual/thermal/thermal_zone0/temp"))
		{
			Log(LOG_STATUS, "System: ODroid/Raspberry");
			m_szInternalTemperatureCommand = R"(cat /sys/devices/virtual/thermal/thermal_zone0/temp | awk '{ if ($1 < 100) printf("temp=%d\n",$1); else printf ("temp=%0.2f\n",$1/1000); }')";
			m_bHasInternalTemperature = true;
		}
	}
	if (file_exist("/sys/class/power_supply/ac/voltage_now"))
	{
		Debug(DEBUG_NORM, "Internal voltage sensor detected");
		m_szInternalVoltageCommand = R"(cat /sys/class/power_supply/ac/voltage_now | awk '{ printf ("volt=%0.2f\n",$1/1000000); }')";
		m_bHasInternalVoltage = true;
	}
	if (file_exist("/sys/class/power_supply/ac/current_now"))
	{
		Debug(DEBUG_NORM, "Internal current sensor detected");
		m_szInternalCurrentCommand = R"(cat /sys/class/power_supply/ac/current_now | awk '{ printf ("curr=%0.2f\n",$1/1000000); }')";
		m_bHasInternalCurrent = true;
	}
	//New Armbian Kernal 4.14+
	if (file_exist("/sys/class/power_supply/axp20x-ac/voltage_now"))
	{
		Debug(DEBUG_NORM, "Internal voltage sensor detected");
		m_szInternalVoltageCommand = R"(cat /sys/class/power_supply/axp20x-ac/voltage_now | awk '{ printf ("volt=%0.2f\n",$1/1000000); }')";
		m_bHasInternalVoltage = true;
	}
	if (file_exist("/sys/class/power_supply/axp20x-ac/current_now"))
	{
		Debug(DEBUG_NORM, "Internal current sensor detected");
		m_szInternalCurrentCommand = R"(cat /sys/class/power_supply/axp20x-ac/current_now | awk '{ printf ("curr=%0.2f\n",$1/1000000); }')";
		m_bHasInternalCurrent = true;
	}
#endif

#if defined (__OpenBSD__)
	Debug(DEBUG_NORM, "Internal temperature- and voltage sensors detected");
	m_szInternalTemperatureCommand = "sysctl hw.sensors.acpitz0.temp0|sed -e 's/.*temp0/temp/'|cut -d ' ' -f 1";
	m_bHasInternalTemperature = true;
	m_szInternalVoltageCommand = "sysctl hw.sensors.acpibat0.volt1|sed -e 's/.*volt1/volt/'|cut -d ' ' -f 1";
	m_bHasInternalVoltage = true;
#endif

}

#endif // defined(__linux__) || defined(__CYGWIN32__) || defined(__FreeBSD__) || defined(__OpenBSD__)
