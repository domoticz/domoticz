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
#else
	#include <sys/sysinfo.h>
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
        _log.Log(LOG_NORM,"System: Started");
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
	// Check if there is already hardware running for System, if no start it.
	
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
#ifdef _DEBUG
    _log.Log(LOG_NORM,"System: Id set to: %d",hwId);
#endif	
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
	_log.Log(LOG_NORM,"System: Stopped...");			

}


void CHardwareMonitor::FetchData()
{
#ifdef WIN32
	if (IsOHMRunning()) {
		_log.Log(LOG_NORM,"System: Fetching data (System sensors)");
		RunWMIQuery("Sensor","Temperature");
		RunWMIQuery("Sensor","Load");
		RunWMIQuery("Sensor","Fan");
		RunWMIQuery("Sensor","Voltage");
	}
#else
	FetchUnixData();
#endif
}

void CHardwareMonitor::UpdateSystemSensor(const std::string& qType, const std::string& wmiId, const std::string& devName, const std::string& devValue)
{
	if (!hwId) {
#ifdef _DEBUG
		_log.Log(LOG_NORM,"System: Id not found!");
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
		IWbemClassObject *pclsObj;  
		ULONG uReturn = 0;  
		hr = pEnumerator->Next(WBEM_INFINITE, 1,  &pclsObj, &uReturn);  
		if(0 == uReturn)  
		{  
			return false;  
		}  
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
			ULONG uReturn = 0;
			while (pEnumerator)
			{
				HRESULT hr = pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn);			
				if(0 == uReturn || FAILED(hr))
				{
					break;
				}

				VARIANT vtProp;

				hr = pclsObj->Get(L"Name", 0, &vtProp, 0, 0);
				std::string itemName = _bstr_t (vtProp.bstrVal);
				itemName = stdreplace(itemName, "#", "");
				VariantClear(&vtProp);
				hr = pclsObj->Get(L"Value", 0, &vtProp, 0, 0);
				float fItemValue = float (vtProp.fltVal);
				std::ostringstream itemValue;
				if ((qType =="Load")||(qType=="Temperature")) {
					itemValue.precision(3);
				}
				itemValue << fItemValue;
				VariantClear(&vtProp);
				//hr = pclsObj->Get(L"InstanceId", 0, &vtProp, 0, 0);
				hr = pclsObj->Get(L"Identifier", 0, &vtProp, 0, 0); // instance id seems to drift
				std::string itemId = _bstr_t (vtProp.bstrVal);
				//itemId = "WMI"+itemId;
				//_log.Log(LOG_NORM, "System: %s, %s, %s",itemId.c_str(), itemName.c_str(),itemValue.str().c_str());
				UpdateSystemSensor(qType, itemId, itemName, itemValue.str());
				VariantClear(&vtProp);
				uReturn = 0;
				pclsObj->Release();
			}
			pEnumerator->Release();
		}
	}
	else {
		//_log.Log(LOG_NORM, "System: pservices null");
	}
}
#else
	void CHardwareMonitor::FetchUnixData()
	{
		struct sysinfo mySysInfo;
		int ret=sysinfo(&mySysInfo);
		if (ret!=0)
			return;
		char szTmp[80];
		unsigned long usedram=mySysInfo.totalram-mySysInfo.freeram;
		float memusedpercentage=(100.0f/float(mySysInfo.totalram))*usedram;
		sprintf(szTmp,"%.2f",memusedpercentage);
		UpdateSystemSensor("Load", 1, "Memory Usage", szTmp);
		float load1=(float)(mySysInfo.loads[0]);
		float rload1=load1/(float)(1 << SI_LOAD_SHIFT);
		sprintf(szTmp,"%.2f",rload1);
		UpdateSystemSensor("Load", 2, "SystemLoad", szTmp);
	}
#endif //WIN32

