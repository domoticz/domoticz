#include "stdafx.h"
#include "HardwareMonitor.h"
#include "../main/Helper.h"
#include "../main/Logger.h"
#include "hardwaretypes.h"
#include "../main/localtime_r.h"
#include "../main/mainworker.h"
#include "../main/SQLHelper.h"
#include <wchar.h>

//Note, for Windows we use OpenHardware Monitor
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

#define VCGENCMDTEMPCOMMAND "vcgencmd measure_temp"
#define VCGENCMDARMSPEEDCOMMAND "vcgencmd measure_clock arm"
#define VCGENCMDV3DSPEEDCOMMAND "vcgencmd measure_clock v3d"
#define VCGENCMDCORESPEEDCOMMAND "vcgencmd measure_clock core"

#define round(a) ( int ) ( a + .5 )

CHardwareMonitor::CHardwareMonitor(const int ID)
{
	m_HwdID = ID;
	m_lastquerytime = 0;
	m_totcpu = 0;
	m_lastloadcpu = 0;
#ifdef WIN32
	m_pLocator = nullptr;
	m_pServicesOHM = nullptr;
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

	bHasInternalTemperature=false;
	bHasInternalClockSpeeds=false;
	bHasInternalVoltage=false;
	bHasInternalCurrent=false;

	szInternalTemperatureCommand = "";
	szInternalARMSpeedCommand = "";
	szInternalV3DSpeedCommand = "";
	szInternalCoreSpeedCommand = "";
	szInternalVoltageCommand = "";
	szInternalCurrentCommand = "";

	if(!GetOSType(m_OStype))
	{
		Log(LOG_STATUS,"Hardware Monitor was not able to detect an (known) OS type!");
	}
	else if (m_OStype == OStype_Apple)
	{
		Log(LOG_ERROR,"Hardware Monitor does not (yet) support Apple hardware!");
		return false;
	}

	CheckForOnboardSensors();

	RequestStart();

#ifdef WIN32
	InitWMI();
#endif

	m_lastquerytime = 0;
	m_thread = std::make_shared<std::thread>(&CHardwareMonitor::Do_Work, this);
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
					FetchClockSpeeds();
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
	Log(LOG_STATUS,"Hardware Monitor: Stopped...");
}

void CHardwareMonitor::SendCurrent(const unsigned long Idx, const float Curr, const std::string &defaultname)
{
	_tGeneralDevice gDevice;
	gDevice.subtype = sTypeCurrent;
	gDevice.id = 1;
	gDevice.floatval1 = Curr;
	gDevice.intval1 = static_cast<int>(Idx);
	sDecodeRXMessage(this, (const unsigned char *)&gDevice, defaultname.c_str(), 255, nullptr);
}

void CHardwareMonitor::GetInternalTemperature()
{
	Debug(DEBUG_NORM,"Getting Internal Temperature");
	int returncode = 0;
	std::vector<std::string> ret = ExecuteCommandAndReturn(szInternalTemperatureCommand, returncode);
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

void CHardwareMonitor::GetInternalARMClockSpeed()
{
	Debug(DEBUG_NORM,"Getting ARM Clock speed");
	float ArmClockSpeed = 0.0;
	int returncode = 0;
	std::vector<std::string> ret = ExecuteCommandAndReturn(szInternalARMSpeedCommand, returncode);
	if (ret.empty())
	{ 
		Debug(DEBUG_NORM,"String empty");
		return;
	}
	std::string tmpline = ret[0];
	if (tmpline.find("frequency") == std::string::npos)
	{
		Debug(DEBUG_NORM,"Frequency not found (%s)",tmpline.c_str());
		return;
	}

	std::vector<std::string> strarray;
	StringSplit(tmpline,"=",strarray);

	if (strarray.size()==2)
	{
		ArmClockSpeed = static_cast<float>(atof(strarray[1].c_str()))/1000000;
	}

	Debug(DEBUG_NORM,"Updating sensor with value %.2f",ArmClockSpeed);

	SendCustomSensor(0, 1, 255, ArmClockSpeed, "Arm Clock Speed","Mhz");
}

void CHardwareMonitor::GetInternalV3DClockSpeed()
{
	Debug(DEBUG_NORM,"Getting V3D Clock speed");
	float V3DClockSpeed = 0.0;
	int returncode = 0;
	std::vector<std::string> ret = ExecuteCommandAndReturn(szInternalV3DSpeedCommand, returncode);
	if (ret.empty())
	{ 
		Debug(DEBUG_NORM,"String empty");
		return;
	}
	std::string tmpline = ret[0];
	if (tmpline.find("frequency") == std::string::npos)
	{
		Debug(DEBUG_NORM,"Frequency not found (%s)",tmpline.c_str());
		return;
	}

	std::vector<std::string> strarray;
	StringSplit(tmpline,"=",strarray);

	if (strarray.size()==2)
	{
		V3DClockSpeed = static_cast<float>(atof(strarray[1].c_str()))/1000000;
	}

	Debug(DEBUG_NORM,"Updating sensor with value %.2f",V3DClockSpeed);

	SendCustomSensor(0, 2, 255, V3DClockSpeed, "V3D Clock Speed","Mhz");
}

void CHardwareMonitor::GetInternalCoreClockSpeed()
{
	Debug(DEBUG_NORM,"Getting Core Clock speed");
	float CoreClockSpeed = 0.0;
	int returncode = 0;
	std::vector<std::string> ret = ExecuteCommandAndReturn(szInternalCoreSpeedCommand, returncode);
	if (ret.empty())
	{ 
		Debug(DEBUG_NORM,"String empty");
		return;
	}
	std::string tmpline = ret[0];
	if (tmpline.find("frequency") == std::string::npos)
	{
		Debug(DEBUG_NORM,"Frequency not found (%s)",tmpline.c_str());
		return;
	}

	std::vector<std::string> strarray;
	StringSplit(tmpline,"=",strarray);

	if (strarray.size()==2)
	{
		CoreClockSpeed = static_cast<float>(atof(strarray[1].c_str()))/1000000;
	}

	Debug(DEBUG_NORM,"Updating sensor with value %.2f",CoreClockSpeed);

	SendCustomSensor(0, 3, 255, CoreClockSpeed, "Core Clock Speed","Mhz");
}

void CHardwareMonitor::GetInternalVoltage()
{
	Debug(DEBUG_NORM,"Getting Internal Voltage");
	int returncode = 0;
	std::vector<std::string> ret = ExecuteCommandAndReturn(szInternalVoltageCommand, returncode);
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
	Debug(DEBUG_NORM,"Getting Internal Current");
	int returncode = 0;
	std::vector<std::string> ret = ExecuteCommandAndReturn(szInternalCurrentCommand, returncode);
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
		Debug(DEBUG_NORM,"Hardware Monitor: Id not found!");
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

bool CHardwareMonitor::GetOSType(nOSType &OStype)
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
	if (IsOHMRunning()) {
		Debug(DEBUG_NORM,"Fetching Windows sensor data (System sensors)");
		RunWMIQuery("Sensor","Temperature");
		RunWMIQuery("Sensor","Load");
		RunWMIQuery("Sensor","Fan");
		RunWMIQuery("Sensor","Voltage");
		return;
	}
#elif defined(__linux__) || defined(__CYGWIN32__) || defined(__FreeBSD__) || defined(__OpenBSD__)
	Debug(DEBUG_NORM,"Fetching *NIX sensor data (System sensors)");

	if (bHasInternalTemperature)
		GetInternalTemperature();

	if (bHasInternalVoltage)
		GetInternalVoltage();

	if (bHasInternalCurrent)
		GetInternalCurrent();
#endif
}

void CHardwareMonitor::FetchClockSpeeds()
{
	if (bHasInternalClockSpeeds) {
		GetInternalARMClockSpeed();
		GetInternalV3DClockSpeed();
		GetInternalCoreClockSpeed();
	}
}

#ifdef WIN32
bool CHardwareMonitor::InitWMI()
{
	HRESULT hr;
	if (m_pLocator)
		return true; //already initialized
	hr = CoCreateInstance(CLSID_WbemAdministrativeLocator, nullptr, CLSCTX_INPROC_SERVER, IID_IWbemLocator, (LPVOID *)&m_pLocator);
	if (FAILED(hr))
		return false;
	hr = m_pLocator->ConnectServer(L"root\\OpenHardwareMonitor", nullptr, nullptr, nullptr, 0, nullptr, nullptr, &m_pServicesOHM);
	if (FAILED(hr))
	{
		Log(LOG_STATUS, "Hardware Monitor: Warning, OpenHardware Monitor is not installed on this system. (http://openhardwaremonitor.org)");
		return false;
	}
	hr = m_pLocator->ConnectServer(L"root\\CIMV2", nullptr, nullptr, nullptr, 0, nullptr, nullptr, &m_pServicesSystem);
	if (FAILED(hr))
		return false;
/*
	// Set security levels on the proxy
	hr = CoSetProxyBlanket(
		m_pServicesSystem,                        // Indicates the proxy to set
		RPC_C_AUTHN_WINNT,           // RPC_C_AUTHN_xxx
		RPC_C_AUTHZ_NONE,            // RPC_C_AUTHZ_xxx
		NULL,                        // Server principal name
		RPC_C_AUTHN_LEVEL_CALL,      // RPC_C_AUTHN_LEVEL_xxx
		RPC_C_IMP_LEVEL_IMPERSONATE, // RPC_C_IMP_LEVEL_xxx
		NULL,                        // client identity
		EOAC_NONE                    // proxy capabilities
		);
*/
	if (!IsOHMRunning())
	{
		Log(LOG_STATUS, "Hardware Monitor: Warning, OpenHardware Monitor is not installed on this system. (http://openhardwaremonitor.org)");
		return false;
	}
	return true;
}

void CHardwareMonitor::ExitWMI()
{
	if (m_pServicesSystem != nullptr)
		m_pServicesSystem->Release();
	m_pServicesSystem = nullptr;
	if (m_pServicesOHM != nullptr)
		m_pServicesOHM->Release();
	m_pServicesOHM = nullptr;
	if (m_pLocator != nullptr)
		m_pLocator->Release();
	m_pLocator = nullptr;
}

bool CHardwareMonitor::IsOHMRunning()
{
	if ((m_pServicesOHM == nullptr) || (m_pServicesSystem == nullptr))
		return false;
	bool bOHMRunning = false;
	IEnumWbemClassObject *pEnumerator = nullptr;
	HRESULT hr;
	hr = m_pServicesSystem->ExecQuery(L"WQL", L"Select * from win32_Process WHERE Name='OpenHardwareMonitor.exe'",
					  WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, nullptr, &pEnumerator);
	if (SUCCEEDED(hr))
	{
		IWbemClassObject *pclsObj = nullptr;
		ULONG uReturn = 0;
		hr = pEnumerator->Next(WBEM_INFINITE, 1,  &pclsObj, &uReturn);
		if ((FAILED(hr)) || (0 == uReturn))
		{
			pEnumerator->Release();
			return false;
		}
		VARIANT vtProp;
		VariantInit(&vtProp);
		hr = pclsObj->Get(L"ProcessId", 0, &vtProp, 0, 0);
		int procId = static_cast<int>(vtProp.iVal);
		if (procId) {
			bOHMRunning = true;
		}
		pclsObj->Release();
		pEnumerator->Release();
	}
	return bOHMRunning;
}

void CHardwareMonitor::RunWMIQuery(const char* qTable, const std::string &qType)
{
	if ((m_pServicesOHM == nullptr) || (m_pServicesSystem == nullptr))
		return;
	HRESULT hr;
	std::string query = "SELECT * FROM ";
	query.append(qTable);
	query.append(" WHERE SensorType = '");
	query.append(qType);
	query.append("'");
	IEnumWbemClassObject *pEnumerator = nullptr;
	hr = m_pServicesOHM->ExecQuery(L"WQL", bstr_t(query.c_str()), WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, nullptr,
				       &pEnumerator);
	if (!FAILED(hr))
	{
		int dindex = 0;

		// Get the data from the query
		IWbemClassObject *pclsObj = nullptr;
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
						itemId = "WMI"+itemId;
						Debug(DEBUG_NORM, "Hardware Monitor: %s, %s, %s",itemId.c_str(), itemName.c_str(),itemValue.str().c_str());
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
		if (gettimeofday(&tp, (struct timezone *)nullptr) == -1)
			return 0;
		return ((double) (tp.tv_sec)) +
			(((double) tp.tv_usec) * 0.000001 );
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
		percent = (100.0f / float(totalMemBytes))*usedMem;
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
	sprintf(szTmp,"%.2f",memusedpercentage);
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
	if (m_lastquerytime==0)
	{
#if defined(__OpenBSD__)
		//Get number of CPUs
		// sysctl hw.ncpu
		int mib[] = {CTL_HW, HW_NCPU};
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
		m_totcpu=totcpu;
#else
		//first time
		m_lastquerytime = time_so_far();
		int actload1,actload2,actload3;
		int totcpu=-1;
#if defined(__FreeBSD__)
		FILE *fIn = fopen("/compat/linux/proc/stat", "r");
#else	// Linux
		FILE *fIn = fopen("/proc/stat", "r");
#endif
		if (fIn != nullptr)
		{
			bool bFirstLine=true;
			while (fgets(szTmp, sizeof(szTmp), fIn) != nullptr)
			{
				int ret=sscanf(szTmp, "%s\t%d\t%d\t%d\n", cname, &actload1, &actload2, &actload3);
				if ((bFirstLine)&&(ret==4)) {
					bFirstLine=false;
					m_lastloadcpu=actload1+actload2+actload3;
				}
				char *cPos=strstr(cname,"cpu");
				if (cPos == nullptr)
					break;
				totcpu++;
			}
			fclose(fIn);
		}
		if (totcpu<1)
			m_lastquerytime=0;
		else
			m_totcpu=totcpu;
#endif // else __OpenBSD__
	}
	else
	{
		double acttime = time_so_far();
#if defined(__OpenBSD__)
		int mib[] = {CTL_KERN, KERN_CPTIME};
		long loads[CPUSTATES];
		size_t size = sizeof(loads);
		if (sysctl(mib, 2, loads, &size, nullptr, 0) < 0)
		{
			Log(LOG_ERROR, "sysctl CPTIME failed.");
			return;
		}
		else
		{
			long long t = (loads[CP_USER] + loads[CP_NICE] + loads[CP_SYS])-m_lastloadcpu;
			double cpuper=((double(t) / (difftime(acttime,m_lastquerytime) * HZ)) * 100);///double(m_totcpu);
			if (cpuper>0)
			{
				sprintf(szTmp,"%.2f", cpuper);
				UpdateSystemSensor("Load", 1, "CPU_Usage", szTmp);
			}
			m_lastloadcpu = loads[CP_USER] + loads[CP_NICE] + loads[CP_SYS];
		}
#else
		int actload1,actload2,actload3;
#if defined(__FreeBSD__)
		FILE *fIn = fopen("/compat/linux/proc/stat", "r");
#else	// Linux
		FILE *fIn = fopen("/proc/stat", "r");
#endif
		if (fIn != nullptr)
		{
			int ret=fscanf(fIn, "%s\t%d\t%d\t%d\n", cname, &actload1, &actload2, &actload3);
			fclose(fIn);
			if (ret==4)
			{
				long long t = (actload1+actload2+actload3)-m_lastloadcpu;
				double cpuper=((t / (difftime(acttime,m_lastquerytime) * HZ)) * 100)/double(m_totcpu);
				if (cpuper>0)
				{
					sprintf(szTmp,"%.2f", cpuper);
					UpdateSystemSensor("Load", 1, "CPU_Usage", szTmp);
				}
				m_lastloadcpu=actload1+actload2+actload3;
			}
		}
#endif //else Openbsd
		m_lastquerytime=acttime;
	}
}

void CHardwareMonitor::FetchUnixDisk()
{
	//Disk Usage
	std::map<std::string, _tDUsageStruct> _disks;
	std::map<std::string, std::string> _dmounts_;
	int returncode = 0;
	std::vector<std::string> _rlines=ExecuteCommandAndReturn(m_dfcommand, returncode);
	if (!_rlines.empty())
	{
		for (const auto & ittDF : _rlines)
		{
			char dname[200];
			char suse[30];
			char smountpoint[300];
			long long numblock, usedblocks, availblocks;
			int ret = sscanf(ittDF.c_str(), "%s\t%lld\t%lld\t%lld\t%s\t%s\n", dname, &numblock, &usedblocks, &availblocks, suse, smountpoint);
			if (ret == 6)
			{
				std::map<std::string, std::string>::iterator it = _dmounts_.find(dname);
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
		for (const auto & ittDisks : _disks)
		{
			_tDUsageStruct dusage = ittDisks.second;
			if (dusage.TotalBlocks > 0)
			{
				double UsagedPercentage = (100 / double(dusage.TotalBlocks))*double(dusage.UsedBlocks);
				//std::cout << "Disk: " << ittDisks.first << ", Mount: " << dusage.MountPoint << ", Used: " << UsagedPercentage << std::endl;
				char szTmp[300];
				sprintf(szTmp, "%.2f", UsagedPercentage);
				std::string hddname = "HDD " + dusage.MountPoint;
				UpdateSystemSensor("Load", 2 + dindex, hddname, szTmp);
				dindex++;
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
	Debug(DEBUG_NORM,"Checking for onboard sensors");

#ifdef WIN32
	Debug(DEBUG_NORM, "Detecting onboard sensors on Windows not supported this way! (But through openhardwaremonitor.org and WMI)");
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

	//Check if we are running on a RaspberryPi
	std::string sLine;
	std::ifstream infile;
	bool bPi = false;

#if defined(__FreeBSD__)
	infile.open("/compat/linux/proc/cpuinfo");
#else
	infile.open("/proc/cpuinfo");
#endif
	if (infile.is_open())
	{
		while (!infile.eof())
		{
			getline(infile, sLine);
			if (
				(sLine.find("BCM2708") != std::string::npos) ||
				(sLine.find("BCM2709") != std::string::npos) ||
				(sLine.find("BCM2711") != std::string::npos) ||
				(sLine.find("BCM2835") != std::string::npos)

				)
			{
				Log(LOG_STATUS, "System: Raspberry Pi");
				bPi = true;
				//Check if we have vcgencmd (are running on a RaspberryPi)
				//
				int returncode = 0;
				std::vector<std::string> ret = ExecuteCommandAndReturn(VCGENCMDTEMPCOMMAND, returncode);

				if (ret.empty()) {
					Log(LOG_STATUS,"It seems vcgencmd is not installed. If you would like use the hardware monitor, consider installing this!");
				}
				else {
					std::string tmpline = ret[0];
					if (tmpline.find("temp=") == std::string::npos) {
						Log(LOG_STATUS, "It seems vcgencmd is not installed. If you would like use the hardware monitor, consider installing this!");
					}
					else {
						//Core temperature of BCM2835 SoC
						szInternalTemperatureCommand = VCGENCMDTEMPCOMMAND;
						bHasInternalTemperature = true;

						//PI Clock speeds	
						szInternalARMSpeedCommand = VCGENCMDARMSPEEDCOMMAND;
						szInternalV3DSpeedCommand = VCGENCMDV3DSPEEDCOMMAND;
						szInternalCoreSpeedCommand = VCGENCMDCORESPEEDCOMMAND;
						bHasInternalClockSpeeds = true;
					}
				}
			}
		}
		infile.close();
	}

	if(!bPi)
	{
		Debug(DEBUG_NORM,"System does not seem to be a Raspberry Pi");
	}
	else
	{
		m_OStype = OStype_Rpi;
	}

	if (!bHasInternalTemperature)
	{
		if (file_exist("/sys/devices/platform/sunxi-i2c.0/i2c-0/0-0034/temp1_input"))
		{
			Log(LOG_STATUS, "System: Cubieboard/Cubietruck");
			szInternalTemperatureCommand = R"(cat /sys/devices/platform/sunxi-i2c.0/i2c-0/0-0034/temp1_input | awk '{ printf ("temp=%0.2f\n",$1/1000); }')";
			bHasInternalTemperature = true;
		}
		else if (file_exist("/sys/devices/virtual/thermal/thermal_zone0/temp"))
		{
			Log(LOG_STATUS,"System: ODroid");
			szInternalTemperatureCommand = R"(cat /sys/devices/virtual/thermal/thermal_zone0/temp | awk '{ if ($1 < 100) printf("temp=%d\n",$1); else printf ("temp=%0.2f\n",$1/1000); }')";
			bHasInternalTemperature = true;
		}
	}
	if (file_exist("/sys/class/power_supply/ac/voltage_now"))
	{
		Debug(DEBUG_NORM, "Internal voltage sensor detected");
		szInternalVoltageCommand = R"(cat /sys/class/power_supply/ac/voltage_now | awk '{ printf ("volt=%0.2f\n",$1/1000000); }')";
		bHasInternalVoltage = true;
	}
	if (file_exist("/sys/class/power_supply/ac/current_now"))
	{
		Debug(DEBUG_NORM, "Internal current sensor detected");
		szInternalCurrentCommand = R"(cat /sys/class/power_supply/ac/current_now | awk '{ printf ("curr=%0.2f\n",$1/1000000); }')";
		bHasInternalCurrent = true;
	}
	//New Armbian Kernal 4.14+
	if (file_exist("/sys/class/power_supply/axp20x-ac/voltage_now"))
	{
		Debug(DEBUG_NORM, "Internal voltage sensor detected");
		szInternalVoltageCommand = R"(cat /sys/class/power_supply/axp20x-ac/voltage_now | awk '{ printf ("volt=%0.2f\n",$1/1000000); }')";
		bHasInternalVoltage = true;
	}
	if (file_exist("/sys/class/power_supply/axp20x-ac/current_now"))
	{
		Debug(DEBUG_NORM, "Internal current sensor detected");
		szInternalCurrentCommand = R"(cat /sys/class/power_supply/axp20x-ac/current_now | awk '{ printf ("curr=%0.2f\n",$1/1000000); }')";
		bHasInternalCurrent = true;
	}
#endif

#if defined (__OpenBSD__)
	Debug(DEBUG_NORM, "Internal temperature- and voltage sensors detected");
	szInternalTemperatureCommand = "sysctl hw.sensors.acpitz0.temp0|sed -e 's/.*temp0/temp/'|cut -d ' ' -f 1";
	bHasInternalTemperature = true;
	szInternalVoltageCommand = "sysctl hw.sensors.acpibat0.volt1|sed -e 's/.*volt1/volt/'|cut -d ' ' -f 1";
	bHasInternalVoltage = true;
#endif

}
