#include "stdafx.h"
#include "HardwareMonitor.h"
#include "../main/Helper.h"
#include "../main/Logger.h"
#include "hardwaretypes.h"
#include "../main/mainworker.h"
#include "../main/SQLHelper.h"
#include <wchar.h>

#define __STDC_FORMAT_MACROS
#include <inttypes.h>

//Note, for Windows we use Libre Hardware Monitor or Open Hardware Monitor
//https://github.com/LibreHardwareMonitor/LibreHardwareMonitor
//http://openhardwaremonitor.org/

#ifdef WIN32
#include <comdef.h>
#elif defined(__linux__) || defined(__CYGWIN32__) || defined(__FreeBSD__) || defined (__OpenBSD__)
#if !defined ( __FreeBSD__) && !defined(__OpenBSD__)
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

#endif // defined(__linux__) || defined(__CYGWIN32__) || defined(__FreeBSD__) || defined (__OpenBSD__)

#define POLL_INTERVAL_CPU	30
#define POLL_INTERVAL_TEMP	70
#define POLL_INTERVAL_MEM	80
#define POLL_INTERVAL_DISK	170

CHardwareMonitor::CHardwareMonitor(const int ID)
{
	m_HwdID = ID;
	m_lastquerytime = 0;
	m_totcpu = 0;
	m_lastloadcpu = 0;
#ifdef WIN32
	m_pLocator = nullptr;
	m_pServicesHM = nullptr;
	m_pServicesSystem = nullptr;
#endif
}

CHardwareMonitor::~CHardwareMonitor()
{
	StopHardware();
}

bool CHardwareMonitor::StartHardware()
{
	StopHardware();

	m_bHasInternalTemperature = false;
	m_bHasInternalVoltage = false;
	m_bHasInternalCurrent = false;

	m_szInternalTemperatureCommand = "";
	m_szInternalVoltageCommand = "";
	m_szInternalCurrentCommand = "";

	if (!GetOSType(m_OStype))
	{
		Log(LOG_STATUS, "Hardware Monitor was not able to detect an (known) OS type!");
	}
	else if (m_OStype == OStype_Apple)
	{
		Log(LOG_ERROR, "Hardware Monitor does not (yet) support Apple hardware!");
		return false;
	}

	CheckForOnboardSensors();

	RequestStart();

#ifdef WIN32
	InitWMI();
#endif

	m_lastquerytime = 0;
	m_thread = std::make_shared<std::thread>([this] { Do_Work(); });
	SetThreadNameInt(m_thread->native_handle());
	m_bIsStarted = true;
	sOnConnected(this);

	return true;
}

bool CHardwareMonitor::StopHardware()
{
	if (m_thread)
	{
		RequestStop();
		m_thread->join();
		m_thread.reset();
	}
#ifdef WIN32
	ExitWMI();
#endif
	m_bIsStarted = false;
	return true;
}

void CHardwareMonitor::Do_Work()
{
	Log(LOG_STATUS, "Hardware Monitor: Started (OStype %s)", TranslateOSTypeToString(m_OStype).c_str());

	int msec_counter = 0;
	int64_t sec_counter = 140 - 2;	// Start at a moment that is close to most devicecheck intervals
	while (!IsStopRequested(500))
	{
		msec_counter++;
		if (msec_counter == 2)
		{
			msec_counter = 0;
			sec_counter++;
			if (sec_counter % 12 == 0)
				m_LastHeartbeat = mytime(nullptr);

			if (sec_counter % POLL_INTERVAL_TEMP == 0)
			{
				try
				{
					FetchData();
				}
				catch (...)
				{
					Log(LOG_ERROR, "Hardware Monitor: Error occurred while Fetching motherboard sensors!...");
				}
			}

			if (sec_counter % POLL_INTERVAL_CPU == 0)
			{
				try
				{
					FetchCPU();
				}
				catch (...)
				{
					Log(LOG_ERROR, "Hardware Monitor: Error occurred while Fetching CPU data!...");
				}
			}
			if (sec_counter % POLL_INTERVAL_MEM == 0)
			{
				try
				{
					FetchMemory();
				}
				catch (...)
				{
					Log(LOG_ERROR, "Hardware Monitor: Error occurred while Fetching memory data!...");
				}
			}

			if (sec_counter % POLL_INTERVAL_DISK == 0)
			{
				try
				{
					FetchDisk();
				}
				catch (...)
				{
					Log(LOG_ERROR, "Hardware Monitor: Error occurred while Fetching disk data!...");
				}
			}
		}
	}
	Log(LOG_STATUS, "Hardware Monitor: Stopped...");
}

void CHardwareMonitor::SendCurrent(const unsigned long Idx, const float Curr, const std::string& defaultname)
{
	_tGeneralDevice gDevice;
	gDevice.subtype = sTypeCurrent;
	gDevice.id = 1;
	gDevice.floatval1 = Curr;
	gDevice.intval1 = static_cast<int>(Idx);
	sDecodeRXMessage(this, (const unsigned char*)&gDevice, defaultname.c_str(), 255, nullptr);
}

void CHardwareMonitor::GetInternalTemperature()
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

void CHardwareMonitor::GetInternalVoltage()
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

void CHardwareMonitor::GetInternalCurrent()
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

void CHardwareMonitor::UpdateSystemSensor(const std::string& qType, const int dindex, const std::string& devName, const std::string& devValue)
{
	if (!m_HwdID) {
		Debug(DEBUG_NORM, "Hardware Monitor: Id not found!");
		return;
	}
	int doffset = 0;
	if (qType == "Temperature")
	{
		doffset = 1000;
		float temp = static_cast<float>(atof(devValue.c_str()));
		SendTempSensor(doffset + dindex, 255, temp, devName);
	}
	else if (qType == "Load")
	{
		doffset = 1100;
		float perc = static_cast<float>(atof(devValue.c_str()));
		SendPercentageSensor(doffset + dindex, 0, 255, perc, devName);
	}
	else if (qType == "Fan")
	{
		doffset = 1200;
		int fanspeed = atoi(devValue.c_str());
		SendFanSensor(doffset + dindex, 255, fanspeed, devName);
	}
	else if (qType == "Voltage")
	{
		doffset = 1300;
		float volt = static_cast<float>(atof(devValue.c_str()));
		SendVoltageSensor(0, (uint32_t)(doffset + dindex), 255, volt, devName);
	}
	else if (qType == "Current")
	{
		doffset = 1400;
		float curr = static_cast<float>(atof(devValue.c_str()));
		SendCurrent(doffset + dindex, curr, devName);
	}
	else if (qType == "Process")
	{
		doffset = 1500;
		float usage = static_cast<float>(atof(devValue.c_str()));
		SendCustomSensor(0, doffset + dindex, 255, usage, devName, "MB");
	}
}

bool CHardwareMonitor::GetOSType(nOSType& OStype)
{
	OStype = OStype_Unknown;

#if defined (__linux__)
	OStype = OStype_Linux;

	if (IsWSL())
		OStype = OStype_WSL;
#endif

#if defined (__FreeBSD__)
	OStype = OStype_FreeBSD;
#endif

#if defined (__OpenBSD__)
	OStype = OStype_OpenBSD;
#endif

#if defined (__CYGWIN32__)
	OStype = OStype_CYGWIN;
#endif

#ifdef WIN32
	OStype = OStype_Windows;
#endif

#ifdef __APPLE__
	OStype = OStype_Apple;
#endif

	if (OStype == OStype_Unknown)
		return false;
	return true;
}

std::string CHardwareMonitor::TranslateOSTypeToString(nOSType OSType)
{
	std::string sOSType;

	switch (OSType)
	{
	case OStype_Linux:
		sOSType = "Linux";
		break;
	case OStype_Rpi:
		sOSType = "Raspberry Pi Linux";
		break;
	case OStype_WSL:
		sOSType = "WSL Linux";
		break;
	case OStype_CYGWIN:
		sOSType = "CYGWIN Linux";
		break;
	case OStype_FreeBSD:
		sOSType = "FreeBSD";
		break;
	case OStype_OpenBSD:
		sOSType = "OpenBSD";
		break;
	case OStype_Windows:
		sOSType = "Windows";
		break;
	case OStype_Apple:
		sOSType = "Apple";
		break;
	default:
		sOSType = "Unknown";
		break;
	}
	return sOSType;
}

void CHardwareMonitor::FetchCPU()
{
#if defined(__linux__) || defined(__CYGWIN32__) || defined(__FreeBSD__) || defined(__OpenBSD__)
	FetchUnixCPU();
#endif
}

void CHardwareMonitor::FetchDisk()
{
#if defined(__linux__) || defined(__CYGWIN32__) || defined(__FreeBSD__) || defined(__OpenBSD__)
	FetchUnixDisk();
#endif
}

void CHardwareMonitor::FetchMemory()
{
#if defined(__linux__) || defined(__CYGWIN32__) || defined(__FreeBSD__) || defined(__OpenBSD__)
	FetchUnixMemory();
#endif
}

void CHardwareMonitor::FetchData()
{
#ifdef WIN32
	if (IsHMRunning()) {
		Debug(DEBUG_NORM, "Fetching Windows sensor data (System sensors)");
		RunWMIQuery("Sensor", "Temperature");
		RunWMIQuery("Sensor", "Load");
		RunWMIQuery("Sensor", "Fan");
		RunWMIQuery("Sensor", "Voltage");
		return;
	}
#elif defined(__linux__) || defined(__CYGWIN32__) || defined(__FreeBSD__) || defined(__OpenBSD__)
	Debug(DEBUG_NORM, "Fetching *NIX sensor data (System sensors)");

	if (m_bHasInternalTemperature)
		GetInternalTemperature();

	if (m_bHasInternalVoltage)
		GetInternalVoltage();

	if (m_bHasInternalCurrent)
		GetInternalCurrent();
#endif
}

#ifdef WIN32
bool CHardwareMonitor::InitWMI()
{
	HRESULT hr;
	if (m_pLocator)
		return true; //already initialized
	hr = CoCreateInstance(CLSID_WbemAdministrativeLocator, nullptr, CLSCTX_INPROC_SERVER, IID_IWbemLocator, (LPVOID*)&m_pLocator);
	if (FAILED(hr))
		return false;

	//First try Libre Hardware Monitor
	hr = m_pLocator->ConnectServer(L"root\\LibreHardwareMonitor", nullptr, nullptr, nullptr, 0, nullptr, nullptr, &m_pServicesHM);
	if (SUCCEEDED(hr))
	{
		hr = m_pLocator->ConnectServer(L"root\\CIMV2", nullptr, nullptr, nullptr, 0, nullptr, nullptr, &m_pServicesSystem);
		if (SUCCEEDED(hr))
		{
			m_bIsLibreHardwareMonitor = true;
			if (IsHMRunning())
			{
				return true;
			}
			m_bIsLibreHardwareMonitor = false;
			m_pServicesSystem->Release();
			m_pServicesSystem = nullptr;
		}
		m_pServicesHM->Release();
		m_pServicesHM = nullptr;
	}

	//Try Open Hardware Monitor
	hr = m_pLocator->ConnectServer(L"root\\OpenHardwareMonitor", nullptr, nullptr, nullptr, 0, nullptr, nullptr, &m_pServicesHM);
	if (SUCCEEDED(hr))
	{
		hr = m_pLocator->ConnectServer(L"root\\CIMV2", nullptr, nullptr, nullptr, 0, nullptr, nullptr, &m_pServicesSystem);
		if (SUCCEEDED(hr))
		{
			if (IsHMRunning())
			{
				return true;
			}
			m_pServicesSystem->Release();
			m_pServicesSystem = nullptr;
		}
		m_pServicesHM->Release();
		m_pServicesHM = nullptr;
	}
	Log(LOG_STATUS, "Hardware Monitor: Warning, neither Libre Hardware Monitor nor Open Hardware Monitor are installed on this system. (https://github.com/LibreHardwareMonitor/LibreHardwareMonitor , http://openhardwaremonitor.org)");
	return false;
}

void CHardwareMonitor::ExitWMI()
{
	if (m_pServicesSystem != nullptr)
		m_pServicesSystem->Release();
	m_pServicesSystem = nullptr;
	if (m_pServicesHM != nullptr)
		m_pServicesHM->Release();
	m_pServicesHM = nullptr;
	if (m_pLocator != nullptr)
		m_pLocator->Release();
	m_pLocator = nullptr;
}

bool CHardwareMonitor::IsHMRunning()
{
	if ((m_pServicesHM == nullptr) || (m_pServicesSystem == nullptr))
		return false;
	bool bHMRunning = false;
	IEnumWbemClassObject* pEnumerator = nullptr;
	HRESULT hr;
	if (m_bIsLibreHardwareMonitor)
	{
		hr = m_pServicesSystem->ExecQuery(L"WQL", L"Select * from win32_Process WHERE Name='LibreHardwareMonitor.exe'",
			WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, nullptr, &pEnumerator);
	}
	else
	{
		hr = m_pServicesSystem->ExecQuery(L"WQL", L"Select * from win32_Process WHERE Name='OpenHardwareMonitor.exe'",
			WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, nullptr, &pEnumerator);
	}
	if (FAILED(hr))
		return false;
	IWbemClassObject* pclsObj = nullptr;
	ULONG uReturn = 0;
	hr = pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn);
	if ((FAILED(hr)) || (0 == uReturn))
	{
		pEnumerator->Release();
		return false;
	}
	VARIANT vtProp;
	VariantInit(&vtProp);
	hr = pclsObj->Get(L"ProcessId", 0, &vtProp, 0, 0);
	if (SUCCEEDED(hr))
	{
		int procId = static_cast<int>(vtProp.iVal);
		if (procId) {
			bHMRunning = true;
		}
		VariantClear(&vtProp);
	}
	pclsObj->Release();
	pEnumerator->Release();

	return bHMRunning;
}

void CHardwareMonitor::RunWMIQuery(const char* qTable, const std::string& qType)
{
	if ((m_pServicesHM == nullptr) || (m_pServicesSystem == nullptr))
		return;
	HRESULT hr;
	std::string query = "SELECT * FROM ";
	query.append(qTable);
	query.append(" WHERE SensorType = '");
	query.append(qType);
	query.append("'");
	IEnumWbemClassObject* pEnumerator = nullptr;
	hr = m_pServicesHM->ExecQuery(L"WQL", bstr_t(query.c_str()), WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, nullptr,
		&pEnumerator);
	if (!FAILED(hr))
	{
		int dindex = 0;

		// Get the data from the query
		IWbemClassObject* pclsObj = nullptr;
		while (pEnumerator)
		{
			ULONG uReturn = 0;
			hr = pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn);
			if (FAILED(hr) || (0 == uReturn))
			{
				break;
			}

			VARIANT vtProp;

			hr = pclsObj->Get(L"Name", 0, &vtProp, 0, 0);
			if (SUCCEEDED(hr))
			{
				std::string itemName = _bstr_t(vtProp.bstrVal);
				stdreplace(itemName, "#", "");
				VariantClear(&vtProp);

				hr = pclsObj->Get(L"Value", 0, &vtProp, 0, 0);
				if (SUCCEEDED(hr))
				{
					float fItemValue = float(vtProp.fltVal);
					std::ostringstream itemValue;
					if ((qType == "Load") || (qType == "Temperature")) {
						itemValue.precision(3);
					}
					itemValue << fItemValue;
					VariantClear(&vtProp);

					//hr = pclsObj->Get(L"InstanceId", 0, &vtProp, 0, 0);
					hr = pclsObj->Get(L"Identifier", 0, &vtProp, 0, 0); // instance id seems to drift
					if (SUCCEEDED(hr))
					{
						std::string itemId = _bstr_t(vtProp.bstrVal);
						if (itemId.find("/hdd") != std::string::npos)
						{
							itemName = itemId + " " + itemName;
						}
						itemId = "WMI" + itemId;
						Debug(DEBUG_NORM, "Hardware Monitor: %s, %s, %s", itemId.c_str(), itemName.c_str(), itemValue.str().c_str());
						UpdateSystemSensor(qType, dindex, itemName, itemValue.str());
						VariantClear(&vtProp);
						dindex++;
					}
				}
			}
			pclsObj->Release();
		}
		pEnumerator->Release();
	}
}
#elif defined(__linux__) || defined(__CYGWIN32__) || defined(__FreeBSD__) || defined(__OpenBSD__)
double CHardwareMonitor::time_so_far()
{
	struct timeval tp;
	if (gettimeofday(&tp, (struct timezone*)nullptr) == -1)
		return 0;
	return ((double)(tp.tv_sec)) +
		(((double)tp.tv_usec) * 0.000001);
}

#if defined(__linux__)
float CHardwareMonitor::GetProcessMemUsage()
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

float CHardwareMonitor::GetMemUsageLinux()
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
float CHardwareMonitor::GetMemUsageOpenBSD()
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

void CHardwareMonitor::FetchUnixMemory()
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

void CHardwareMonitor::FetchUnixCPU()
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

void CHardwareMonitor::FetchUnixDisk()
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
				listOfHdd = m_sql.safe_query("SELECT ID, DeviceID, Name FROM DeviceStatus WHERE (HardwareID=%d AND DeviceID>'0000044D' AND DeviceID<'000004B0')", m_HwdID);
				if (!listOfHdd.empty())
				{
					for (const auto& sd : listOfHdd)
					{
						std::string idx = sd[0];
						std::string szDeviceId = sd[1];
						std::string Name = sd[2];
						int deviceId;
						sscanf(szDeviceId.c_str(), "%x", &deviceId);
						if (!strcmp(Name.c_str(),hddname.c_str()))
						{	
							dindex=deviceId;
							break;
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
#endif //WIN32/#elif defined(__linux__) || defined(__CYGWIN32__) || defined(__FreeBSD__)

bool CHardwareMonitor::IsWSL()
{
	// Detect WSL according to https://github.com/Microsoft/WSL/issues/423#issuecomment-221627364
	bool is_wsl = false;

#if defined(__linux__)
	char buf[1024];

	int status_fd = open("/proc/sys/kernel/osrelease", O_RDONLY);
	if (status_fd == -1)
		return is_wsl;

	ssize_t num_read = read(status_fd, buf, sizeof(buf) - 1);

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

	if (num_read > 0)
	{
		buf[num_read] = 0;
		is_wsl |= (strstr(buf, "Microsoft") != nullptr);
		is_wsl |= (strstr(buf, "WSL") != nullptr);
	}
#endif

	return is_wsl;
}

void CHardwareMonitor::CheckForOnboardSensors()
{
	Debug(DEBUG_NORM, "Checking for onboard sensors");

#ifdef WIN32
	Debug(DEBUG_NORM, "Detecting onboard sensors on Windows not supported this way! (But through Libre Hardware Monitor or Open Hardware Monitor and WMI)");
	return;
#endif

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
