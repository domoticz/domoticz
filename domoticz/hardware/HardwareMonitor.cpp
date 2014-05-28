#include "stdafx.h"
#include "HardwareMonitor.h"
#include "../main/Helper.h"
#include "../main/Logger.h"
#include "hardwaretypes.h"
#include "../main/localtime_r.h"
#include "../main/mainworker.h"
#include <wchar.h>

#ifdef WIN32
	#include <comdef.h>
#elif defined __linux__
	#include <sys/sysinfo.h>
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

#endif

#define POLL_INTERVAL 30
#define SLEEP_INTERVAL 1000

CHardwareMonitor::CHardwareMonitor()
{
	m_pMain=NULL;
	m_stoprequested=false;
	m_bEnabled=true;
#ifdef WIN32
	InitWMI();
#endif
}


CHardwareMonitor::~CHardwareMonitor(void)
{
#ifdef WIN32
	ExitWMI();
#endif
	StopHardwareMonitor();
}
void CHardwareMonitor::StartHardwareMonitor(MainWorker *pMainWorker)
{
#ifdef _DEBUG
        _log.Log(LOG_STATUS,"Hardware Monitor: Started");
#endif
	m_pMain=pMainWorker;
	Init();

	if (m_bEnabled)
	{
		m_thread = boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&CHardwareMonitor::Do_Work, this)));
	}
}

void CHardwareMonitor::StopHardwareMonitor()
{
	if (!m_bEnabled)
		return;

	if (m_thread!=NULL)
	{
		m_stoprequested = true;
		m_thread->join();
		//if (pLocator!=NULL)
		//pLocator->Release(); 
		//if (pServicesOHM!=NULL)
		//pServicesOHM->Release();
		//if (pServicesSystem!=NULL)
		//pServicesSystem->Release();
	}
}

void CHardwareMonitor::Init()
{
#ifdef __APPLE__
	//sorry apple not supported for now
	m_bEnabled=false;
	return;
#endif
	// Check if there is already hardware running for System, if no start it.
	m_lastquerytime=0;
	hwId = 0;
	std::stringstream szQuery;
	std::vector<std::vector<std::string> > result;
	szQuery << "SELECT ID,Enabled FROM Hardware WHERE (Type=='" <<HTYPE_System << "') AND (Name=='Motherboard') LIMIT 1";
	result=m_pMain->m_sql.query(szQuery.str());
	if (result.size()<1)
	{
		szQuery.clear();
		szQuery.str("");
		szQuery << "INSERT INTO Hardware (Name, Enabled, Type, Address, Port, Username, Password, Mode1, Mode2, Mode3, Mode4, Mode5) VALUES ('Motherboard',1, '" << HTYPE_System << "','',1,'','',0,0,0,0,0)";
		m_pMain->m_sql.query(szQuery.str());
		szQuery.clear();
		szQuery.str("");
		szQuery << "SELECT MAX(ID) FROM Hardware";
		result=m_pMain->m_sql.query(szQuery.str());
		if (result.size()>0)
		{
			std::vector<std::string> sd=result[0];
			hwId=atoi(sd[0].c_str());
			m_pMain->AddHardwareFromParams(hwId,"Motherboard",1,HTYPE_System,"",1,"","",0,0,0,0,0);
		}

		m_pMain->m_sql.query(szQuery.str());
		szQuery.clear();
		szQuery.str("");
		szQuery << "SELECT ID,Enabled FROM Hardware WHERE (Type==" <<HTYPE_System << ") AND (Name=='Motherboard') LIMIT 1";
		result=m_pMain->m_sql.query(szQuery.str());
	}
	
	if (result.size()>0)
    {
		std::vector<std::string> sd=result[0];
		hwId=atoi(sd[0].c_str());
		m_bEnabled=atoi(sd[1].c_str())!=0;
	}
}

void CHardwareMonitor::Do_Work()
{

	m_stoprequested=false;

	while (!m_stoprequested)
	{
		sleep_milliseconds(SLEEP_INTERVAL);
		time_t atime=mytime(NULL);
		struct tm ltime;
		localtime_r(&atime,&ltime);

		if (ltime.tm_sec%POLL_INTERVAL==0)
		{
			FetchData();
		}

	}
	_log.Log(LOG_STATUS,"Hardware Monitor: Stopped...");			

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
	}
#elif defined __linux__
	_log.Log(LOG_NORM,"Hardware Monitor: Fetching data (System sensors)");
	FetchUnixData();
#endif
}

void CHardwareMonitor::UpdateSystemSensor(const std::string& qType, const std::string& wmiId, const std::string& devName, const std::string& devValue)
{
	if (!hwId) {
#ifdef _DEBUG
		_log.Log(LOG_NORM,"Hardware Monitor: Id not found!");
#endif		
		return;
	}
	std::stringstream szQuery;
	std::vector<std::vector<std::string> > result;
	szQuery << "SELECT ID FROM DeviceStatus WHERE (DeviceID=='" << wmiId << "')";
	result=m_pMain->m_sql.query(szQuery.str());
	if (result.size()<1)
	{
		szQuery.clear();
		szQuery.str("");
		int dsubtype=0;
		if (qType=="Temperature")
			dsubtype=sTypeSystemTemp;
		else if (qType=="Load")
			dsubtype=sTypePercentage;
		else if (qType=="Fan")
			dsubtype=sTypeSystemFan;
		else if (qType=="Voltage")
			dsubtype=sTypeVoltage;
		if (dsubtype!=0) {
			szQuery << 
				"INSERT INTO DeviceStatus (HardwareID, DeviceID, Unit, Type, SubType, SignalLevel, BatteryLevel, Name, nValue, sValue) "
				"VALUES (" << hwId << ",'" << wmiId << "',"<< 0 << "," << pTypeGeneral << "," <<dsubtype << ",12,255,'" << devName << "'," << devValue << ",'" << devValue << "')";
			m_pMain->m_sql.query(szQuery.str());
		}
	}
	else 
	{
		szQuery.clear();
		szQuery.str("");

		time_t now = time(0);
		struct tm ltime;
		localtime_r(&now,&ltime);

		char szLastUpdate[40];
		sprintf(szLastUpdate,"%04d-%02d-%02d %02d:%02d:%02d",ltime.tm_year+1900,ltime.tm_mon+1, ltime.tm_mday, ltime.tm_hour, ltime.tm_min, ltime.tm_sec);

		szQuery << "UPDATE DeviceStatus SET HardwareID = " << hwId << ", nValue=" << devValue << ", sValue ='" << devValue << "', LastUpdate='" << szLastUpdate << "' WHERE (DeviceID == '" << wmiId << "')";
		m_pMain->m_sql.query(szQuery.str());

		if (qType == "Load") {
			m_pMain->m_sql.CheckAndHandleNotification(hwId, wmiId, 0, pTypeGeneral, sTypePercentage, NTYPE_PERCENTAGE, (const float)atof(devValue.c_str()));
		}
		else if (qType == "Temperature") {
			m_pMain->m_sql.CheckAndHandleNotification(hwId, wmiId, 0, pTypeGeneral, sTypeSystemTemp, NTYPE_TEMPERATURE, (const float)atof(devValue.c_str()));
		}
		else if (qType == "Fan") {
			m_pMain->m_sql.CheckAndHandleNotification(hwId, wmiId, 0, pTypeGeneral, sTypeSystemFan, NTYPE_RPM, (const float)atof(devValue.c_str()));
		}
		else if (qType=="Voltage") {
			m_pMain->m_sql.CheckAndHandleNotification(hwId, wmiId, 0, pTypeGeneral, sTypeSystemFan, NTYPE_USAGE, (const float)atof(devValue.c_str()));
		}
	}
	return;
}

#ifdef WIN32
void CHardwareMonitor::InitWMI()
{
	pLocator=0; 
	pServicesOHM=0;
	pServicesSystem=0;
	HRESULT hr;
	hr = CoInitializeEx(0, COINIT_MULTITHREADED);
    if (SUCCEEDED(hr))
	{
		hr = CoInitializeSecurity(NULL, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_DEFAULT, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE, NULL);
		if (SUCCEEDED(hr)) 
		{
			hr = CoCreateInstance(CLSID_WbemAdministrativeLocator, NULL, CLSCTX_INPROC_SERVER, IID_IWbemLocator, (LPVOID*)&pLocator);
			if (SUCCEEDED(hr))
			{
				hr = pLocator->ConnectServer(L"root\\OpenHardwareMonitor",NULL, NULL, NULL, 0, NULL, NULL, &pServicesOHM);
				hr = pLocator->ConnectServer(L"root\\CIMV2",NULL, NULL, NULL, 0, NULL, NULL, &pServicesSystem);
			}
		}
	}
}

void CHardwareMonitor::ExitWMI()
{
	if (pServicesOHM!=NULL)
		pServicesOHM->Release();
	if (pServicesSystem!=NULL)
		pServicesSystem->Release();
	if (pLocator!=NULL)
		pLocator->Release();
	CoUninitialize();
}


bool CHardwareMonitor::IsOHMRunning()
{
	IEnumWbemClassObject* pEnumerator = NULL;  
	HRESULT hr;
	hr = pServicesSystem->ExecQuery(L"WQL", L"Select * from win32_Process WHERE Name='OpenHardwareMonitor.exe'" , WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, NULL, &pEnumerator);  
	if (SUCCEEDED(hr))  
	{
		IWbemClassObject *pclsObj=NULL;  
		ULONG uReturn = 0;  
		hr = pEnumerator->Next(WBEM_INFINITE, 1,  &pclsObj, &uReturn);  
		if ( (FAILED(hr)) || (0 == uReturn) )
			return false;
		VARIANT vtProp;  
		VariantInit(&vtProp);  
		hr = pclsObj->Get(L"ProcessId", 0, &vtProp, 0, 0);  
		int procId=(int)vtProp.iVal;
		if (procId) return true;
	}
	return false;
}

void CHardwareMonitor::RunWMIQuery(const char* qTable,const char* qType)
{
	if (pServicesOHM && pServicesSystem)
	{
		HRESULT hr;
		std::string query = "SELECT * FROM ";
		query.append(qTable);
		query.append(" WHERE SensorType = '");
		query.append(qType);
		query.append("'");
		IEnumWbemClassObject* pEnumerator = NULL; 
		hr = pServicesOHM->ExecQuery(L"WQL", bstr_t(query.c_str()), WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, NULL, &pEnumerator);
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
					itemName = stdreplace(itemName, "#", "");
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
							//itemId = "WMI"+itemId;
							//_log.Log(LOG_NORM, "Hardware Monitor: %s, %s, %s",itemId.c_str(), itemName.c_str(),itemValue.str().c_str());
							UpdateSystemSensor(qType, itemId, itemName, itemValue.str());
							VariantClear(&vtProp);
						}
					}
				}
				pclsObj->Release();
			}
			pEnumerator->Release();
		}
	}
	else {
		//_log.Log(LOG_NORM, "Hardware Monitor: pservices null");
	}
}
#elif defined __linux__
	double time_so_far()
	{
		struct timeval tp;
		if (gettimeofday(&tp, (struct timezone *) NULL) == -1)
			return 0;
		return ((double) (tp.tv_sec)) +
			(((double) tp.tv_usec) * 0.000001 );
	}

	void CHardwareMonitor::FetchUnixData()
	{
		struct sysinfo mySysInfo;
		int ret=sysinfo(&mySysInfo);
		if (ret!=0)
			return;
		char szTmp[300];
		//Memory
		unsigned long usedram=mySysInfo.totalram-mySysInfo.freeram;
		float memusedpercentage=(100.0f/float(mySysInfo.totalram))*usedram;
		sprintf(szTmp,"%.2f",memusedpercentage);
		UpdateSystemSensor("Load", "Memory Usage", "Memory Usage", szTmp);

		//CPU
		char cname[50];
		if (m_lastquerytime==0)
		{
			//first time
			m_lastquerytime = time_so_far();
			int actload1,actload2,actload3;
			int totcpu=-1;
			FILE *fIn = fopen("/proc/stat", "r");
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
			}
			if (totcpu<1)
				m_lastquerytime=0;
			else
				m_totcpu=totcpu;
		}
		else
		{
			double acttime = time_so_far();
			int actload1,actload2,actload3;
			FILE *fIn = fopen("/proc/stat", "r");
			if (fIn!=NULL)
			{
				int ret=fscanf(fIn, "%s\t%d\t%d\t%d\n", cname, &actload1, &actload2, &actload3);
				fclose(fIn);
				if (ret==4)
				{
					long long t = (actload1+actload2+actload3)-m_lastloadcpu;
					double cpuper=((t / ((acttime-m_lastquerytime) * 100)) * 100)/double(m_totcpu);
					if (cpuper>0)
					{
						sprintf(szTmp,"%.2f", cpuper);
						UpdateSystemSensor("Load", "CPU_Usage", "CPU_Usage", szTmp);
					}
					m_lastloadcpu=actload1+actload2+actload3;
				}
			}
			m_lastquerytime=acttime;
		}

		//Disk Usage
		std::map<std::string, _tDUsageStruct> _disks;
		std::vector<std::string> _rlines=ExecuteCommandAndReturn("df");
		std::vector<std::string>::const_iterator ittDF;
		for (ittDF=_rlines.begin(); ittDF!=_rlines.end(); ++ittDF)
		{
			char dname[200];
			char suse[30];
			char smountpoint[300];
			long numblock, usedblocks, availblocks;
			int ret=sscanf((*ittDF).c_str(), "%s\t%ld\t%ld\t%ld\t%s\t%s\n", dname, &numblock, &usedblocks, &availblocks, suse, smountpoint);
			if (ret==6)
			{
				if (strstr(dname,"/dev")!=NULL)
				{
					_tDUsageStruct dusage;
					dusage.TotalBlocks=numblock;
					dusage.UsedBlocks=usedblocks;
					dusage.AvailBlocks=availblocks;
					dusage.MountPoint=smountpoint;
					_disks[dname]=dusage;
				}
			}
		}
		std::map<std::string, _tDUsageStruct>::const_iterator ittDisks;
		for (ittDisks=_disks.begin(); ittDisks!=_disks.end(); ++ittDisks)
		{
			_tDUsageStruct dusage=(*ittDisks).second;
			if (dusage.TotalBlocks>0)
			{
				double UsagedPercentage=(100 / double(dusage.TotalBlocks))*double(dusage.UsedBlocks);
				//std::cout << "Disk: " << (*ittDisks).first << ", Mount: " << dusage.MountPoint << ", Used: " << UsagedPercentage << std::endl;
				sprintf(szTmp,"%.2f", UsagedPercentage);
				UpdateSystemSensor("Load", (*ittDisks).first, dusage.MountPoint, szTmp);

			}
		}
	}
#endif //WIN32/#elif defined __linux__

