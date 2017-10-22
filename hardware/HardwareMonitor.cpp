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
	#include <sys/time.h>
	#include <unistd.h>
	#include <vector>
	#include <map>

	struct _tDUsageStruct
	{
		std::string MountPoint;
		long long TotalBlocks;
		long long UsedBlocks;
		long long AvailBlocks;
	};

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

#endif

#define POLL_INTERVAL_CPU	30
#define POLL_INTERVAL_TEMP	70
#define POLL_INTERVAL_MEM	80
#define POLL_INTERVAL_DISK	170

extern bool bHasInternalTemperature;
extern std::string szInternalTemperatureCommand;

extern bool bHasInternalVoltage;
extern std::string szInternalVoltageCommand;

extern bool bHasInternalCurrent;
extern std::string szInternalCurrentCommand;


#define round(a) ( int ) ( a + .5 )

CHardwareMonitor::CHardwareMonitor(const int ID)
{
	m_HwdID = ID;
	m_stoprequested=false;
	m_bOutputLog = false;
#ifdef WIN32
	m_pLocator = NULL;
	m_pServicesOHM = NULL;
	m_pServicesSystem = NULL;
	m_lastquerytime = 0;
	//	CoInitializeEx(0, COINIT_MULTITHREADED);
//	CoInitializeSecurity(NULL, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_DEFAULT, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE, NULL);
#endif
}


CHardwareMonitor::~CHardwareMonitor(void)
{
	StopHardware();
#ifdef WIN32
//	CoUninitialize();
#endif
}

bool CHardwareMonitor::StartHardware()
{
#ifdef __APPLE__
	//sorry apple not supported for now
	return false;
#endif
	StopHardware();

#ifdef WIN32
	InitWMI();
#endif
	m_stoprequested = false;
	m_lastquerytime = 0;
	m_thread = boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&CHardwareMonitor::Do_Work, this)));
	m_bIsStarted = true;
	sOnConnected(this);
#if defined(__linux__) || defined(__CYGWIN32__) || defined(__FreeBSD__) || defined(__OpenBSD__)
	// Busybox df doesn't support -x parameter
	int returncode = 0;
	std::vector<std::string> ret = ExecuteCommandAndReturn("df -x nfs -x tmpfs -x devtmpfs 2> /dev/null", returncode);
	returncode == 0 ?
		m_dfcommand = "df -x nfs -x tmpfs -x devtmpfs" :
		m_dfcommand = "df";
#endif
	return true;
}

bool CHardwareMonitor::StopHardware()
{
	if (m_thread!=NULL)
	{
		m_stoprequested = true;
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

	_log.Log(LOG_STATUS, "Hardware Monitor: Started");

	int msec_counter = 0;
	int64_t sec_counter = POLL_INTERVAL_CPU - 5;
	while (!m_stoprequested)
	{
		sleep_milliseconds(500);
		msec_counter++;
		if (msec_counter == 2)
		{
			msec_counter = 0;
			sec_counter++;
			if (sec_counter % 12 == 0)
				m_LastHeartbeat = mytime(NULL);

			if (sec_counter % POLL_INTERVAL_TEMP == 0)
			{
				try
				{
					FetchData();
				}
				catch (...)
				{
					_log.Log(LOG_STATUS, "Hardware Monitor: Error occurred while Fetching motherboard sensors!...");
				}
			}

#if defined(__linux__) || defined(__CYGWIN32__) || defined(__FreeBSD__) || defined(__OpenBSD__)
			if (sec_counter % POLL_INTERVAL_CPU == 0)
			{
				try
				{
					FetchUnixCPU();
				}
				catch (...)
				{
					_log.Log(LOG_STATUS, "Hardware Monitor: Error occurred while Fetching CPU data!...");
				}
			}

			if (sec_counter % POLL_INTERVAL_MEM == 0)
			{
				try
				{
					FetchUnixMemory();
				}
				catch (...)
				{
					_log.Log(LOG_STATUS, "Hardware Monitor: Error occurred while Fetching memory data!...");
				}
			}

			if (sec_counter % POLL_INTERVAL_DISK == 0)
			{
				try
				{
					FetchUnixDisk();
				}
				catch (...)
				{
					_log.Log(LOG_STATUS, "Hardware Monitor: Error occurred while Fetching disk data!...");
				}
			}
#endif
		}
	}
	_log.Log(LOG_STATUS,"Hardware Monitor: Stopped...");
}

void CHardwareMonitor::SendCurrent(const unsigned long Idx, const float Curr, const std::string &defaultname)
{
	_tGeneralDevice gDevice;
	gDevice.subtype = sTypeCurrent;
	gDevice.id = 1;
	gDevice.floatval1 = Curr;
	gDevice.intval1 = static_cast<int>(Idx);
	sDecodeRXMessage(this, (const unsigned char *)&gDevice, defaultname.c_str(), 255);
}

void CHardwareMonitor::SendFanSensor(const int Idx, const int FanSpeed, const std::string &defaultname)
{
	_tGeneralDevice gDevice;
	gDevice.subtype = sTypeFan;
	gDevice.id = 1;
	gDevice.intval1 = static_cast<int>(Idx);
	gDevice.intval2 = FanSpeed;
	sDecodeRXMessage(this, (const unsigned char *)&gDevice, defaultname.c_str(), 255);
}

void CHardwareMonitor::GetInternalTemperature()
{
	int returncode = 0;
	std::vector<std::string> ret = ExecuteCommandAndReturn(szInternalTemperatureCommand, returncode);
	if (ret.empty())
		return;
	std::string tmpline = ret[0];
	if (tmpline.find("temp=") == std::string::npos)
		return;
	tmpline = tmpline.substr(5);
	size_t pos = tmpline.find("'");
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
	int returncode = 0;
	std::vector<std::string> ret = ExecuteCommandAndReturn(szInternalVoltageCommand, returncode);
	if (ret.empty())
		return;
	std::string tmpline = ret[0];
	if (tmpline.find("volt=") == std::string::npos)
		return;
	tmpline = tmpline.substr(5);
	size_t pos = tmpline.find("'");
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
	int returncode = 0;
	std::vector<std::string> ret = ExecuteCommandAndReturn(szInternalCurrentCommand, returncode);
	if (ret.empty())
		return;
	std::string tmpline = ret[0];
	if (tmpline.find("curr=") == std::string::npos)
		return;
	tmpline = tmpline.substr(5);
	size_t pos = tmpline.find("'");
	if (pos != std::string::npos)
	{
		tmpline = tmpline.substr(0, pos);
	}

	float current = static_cast<float>(atof(tmpline.c_str()));
	if (current == 0)
		return; //hardly possible for a on board temp sensor, if it is, it is probably not working

	SendCurrent(1, current, "Internal Current");
}

void CHardwareMonitor::FetchData()
{
#ifdef WIN32
	if (IsOHMRunning()) {
		_log.Log(LOG_NORM,"Hardware Monitor: Fetching data (System sensors)");
		RunWMIQuery("Sensor","Temperature");
		RunWMIQuery("Sensor","Load");
		RunWMIQuery("Sensor","Fan");
		RunWMIQuery("Sensor","Voltage");
		return;
	}
#elif defined(__linux__) || defined(__CYGWIN32__) || defined(__FreeBSD__) || defined(__OpenBSD__)
	if (bHasInternalTemperature)
		GetInternalTemperature();

	if (bHasInternalVoltage)
		GetInternalVoltage();

	if (bHasInternalCurrent)
		GetInternalCurrent();
#endif
}

void CHardwareMonitor::UpdateSystemSensor(const std::string& qType, const int dindex, const std::string& devName, const std::string& devValue)
{
	if (!m_HwdID) {
#ifdef _DEBUG
		_log.Log(LOG_NORM,"Hardware Monitor: Id not found!");
#endif
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
		SendFanSensor(doffset + dindex, fanspeed, devName);
	}
	else if (qType == "Voltage")
	{
		doffset = 1300;
		float volt = static_cast<float>(atof(devValue.c_str()));
		SendVoltageSensor(0, doffset + dindex, 255, volt, devName);
	}
	else if (qType == "Current")
	{
		doffset = 1400;
		float curr = static_cast<float>(atof(devValue.c_str()));
		SendCurrent(doffset + dindex, curr, devName);
	}
	return;
}

#ifdef WIN32
bool CHardwareMonitor::InitWMI()
{
	HRESULT hr;
	if (m_pLocator)
		return true; //already initialized
	hr = CoCreateInstance(CLSID_WbemAdministrativeLocator, NULL, CLSCTX_INPROC_SERVER, IID_IWbemLocator, (LPVOID*)&m_pLocator);
	if (FAILED(hr))
		return false;
	hr = m_pLocator->ConnectServer(L"root\\OpenHardwareMonitor",NULL, NULL, NULL, 0, NULL, NULL, &m_pServicesOHM);
	if (FAILED(hr))
	{
		_log.Log(LOG_STATUS, "Hardware Monitor: Warning, OpenHardware Monitor is not installed on this system. (http://openhardwaremonitor.org)");
		return false;
	}
	hr = m_pLocator->ConnectServer(L"root\\CIMV2", NULL, NULL, NULL, 0, NULL, NULL, &m_pServicesSystem);
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
		_log.Log(LOG_STATUS, "Hardware Monitor: Warning, OpenHardware Monitor is not installed on this system. (http://openhardwaremonitor.org)");
		return false;
	}
	return true;
}

void CHardwareMonitor::ExitWMI()
{
	if (m_pServicesSystem != NULL)
		m_pServicesSystem->Release();
	m_pServicesSystem = NULL;
	if (m_pServicesOHM!=NULL)
		m_pServicesOHM->Release();
	m_pServicesOHM = NULL;
	if (m_pLocator!=NULL)
		m_pLocator->Release();
	m_pLocator = NULL;
}


bool CHardwareMonitor::IsOHMRunning()
{
	if ((m_pServicesOHM == NULL) || (m_pServicesSystem == NULL))
		return false;
	bool bOHMRunning = false;
	IEnumWbemClassObject* pEnumerator = NULL;
	HRESULT hr;
	hr = m_pServicesSystem->ExecQuery(L"WQL", L"Select * from win32_Process WHERE Name='OpenHardwareMonitor.exe'" , WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, NULL, &pEnumerator);
	if (SUCCEEDED(hr))
	{
		IWbemClassObject *pclsObj=NULL;
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
	if ((m_pServicesOHM == NULL) || (m_pServicesSystem == NULL))
		return;
	HRESULT hr;
	int dindex = 0;
	std::string query = "SELECT * FROM ";
	query.append(qTable);
	query.append(" WHERE SensorType = '");
	query.append(qType);
	query.append("'");
	IEnumWbemClassObject* pEnumerator = NULL;
	hr = m_pServicesOHM->ExecQuery(L"WQL", bstr_t(query.c_str()), WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, NULL, &pEnumerator);
	if (!FAILED(hr))
	{
		// Get the data from the query
		IWbemClassObject *pclsObj = NULL;
		while (pEnumerator)
		{
			ULONG uReturn = 0;
			HRESULT hr = pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn);
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
						//itemId = "WMI"+itemId;
						//_log.Log(LOG_NORM, "Hardware Monitor: %s, %s, %s",itemId.c_str(), itemName.c_str(),itemValue.str().c_str());
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
	double time_so_far()
	{
		struct timeval tp;
		if (gettimeofday(&tp, (struct timezone *) NULL) == -1)
			return 0;
		return ((double) (tp.tv_sec)) +
			(((double) tp.tv_usec) * 0.000001 );
	}

	float GetMemUsageLinux()
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
		float memusedpercentage = (100.0f / float(MemTotal))*MemUsed;
		return memusedpercentage;
	}
#ifdef __OpenBSD__
	float GetMemUsageOpenBSD()
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
		if (sysctl(mibTotalMem, 2, &totalMemBytes,
			   &len, NULL, 0) == -1){
			return -1;
		}
		len = sizeof(pageSize);
		if (sysctl(mibPageSize, 2, &pageSize,
			   &len, NULL, 0) == -1){
			return -1;
		}
		len = sizeof(memStats);
		if (sysctl(mibMemStats, 2, &memStats,
			   &len, NULL, 0) == -1){
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
			memusedpercentage = (100.0f / float(mySysInfo.totalram))*usedram;
#endif
		}
#endif
		sprintf(szTmp,"%.2f",memusedpercentage);
		UpdateSystemSensor("Load", 0, "Memory Usage", szTmp);
	}

	void CHardwareMonitor::FetchUnixCPU()
	{
		char szTmp[300];
		//CPU
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
			if (sysctl(mib, 2, &totcpu, &size, NULL, 0) <0){
				_log.Log(LOG_ERROR, "sysctl NCPU failed.");
				return;
			}
			m_lastquerytime = time_so_far();
			// In the emd there will be single value, so using
			// average loads doesn't generate that much error.
			mib[0] = CTL_KERN;
			mib[1] = KERN_CPTIME;
			size = sizeof(loads);
			if (sysctl(mib, 2, loads, &size, NULL, 0) < 0){
				_log.Log(LOG_ERROR, "sysctl CPTIME failed.");
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
			if (fIn!=NULL)
			{
				bool bFirstLine=true;
				while( fgets(szTmp, sizeof(szTmp), fIn) != NULL )
				{
					int ret=sscanf(szTmp, "%s\t%d\t%d\t%d\n", cname, &actload1, &actload2, &actload3);
					if ((bFirstLine)&&(ret==4)) {
						bFirstLine=false;
						m_lastloadcpu=actload1+actload2+actload3;
					}
					char *cPos=strstr(cname,"cpu");
					if (cPos==NULL)
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
			if (sysctl(mib, 2, loads, &size, NULL, 0) < 0){
				_log.Log(LOG_ERROR, "sysctl CPTIME failed.");
				return;
			}else {
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
			if (fIn!=NULL)
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
			std::vector<std::string>::const_iterator ittDF;
			for (ittDF = _rlines.begin(); ittDF != _rlines.end(); ++ittDF)
			{
				char dname[200];
				char suse[30];
				char smountpoint[300];
				long numblock, usedblocks, availblocks;
				int ret = sscanf((*ittDF).c_str(), "%s\t%ld\t%ld\t%ld\t%s\t%s\n", dname, &numblock, &usedblocks, &availblocks, suse, smountpoint);
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
					if (strstr(dname, "/dev") != NULL)
#elif defined(__CYGWIN32__)
					if (strstr(smountpoint, "/cygdrive/") != NULL)
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
			std::map<std::string, _tDUsageStruct>::const_iterator ittDisks;
			for (ittDisks = _disks.begin(); ittDisks != _disks.end(); ++ittDisks)
			{
				_tDUsageStruct dusage = (*ittDisks).second;
				if (dusage.TotalBlocks > 0)
				{
					double UsagedPercentage = (100 / double(dusage.TotalBlocks))*double(dusage.UsedBlocks);
					//std::cout << "Disk: " << (*ittDisks).first << ", Mount: " << dusage.MountPoint << ", Used: " << UsagedPercentage << std::endl;
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
