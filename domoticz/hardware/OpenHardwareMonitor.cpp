#include "stdafx.h"
#include "OpenHardwareMonitor.h"
#include "../main/Helper.h"
#include "../main/Logger.h"
#include "hardwaretypes.h"
#include "../main/localtime_r.h"
#include "../main/mainworker.h"
#include <wchar.h>

#ifdef WIN32
#include <comdef.h>

#define POLL_INTERVAL 30
#define SLEEP_INTERVAL 1000

COpenHardwareMonitor::COpenHardwareMonitor()
{
	m_pMain=NULL;
	m_pLocator=NULL; 
	m_pServicesOHM=NULL;
	m_pServicesSystem=NULL;
	m_stoprequested=false;
	InitWMI();
}


COpenHardwareMonitor::~COpenHardwareMonitor(void)
{
	if (m_pServicesOHM!=NULL)
		m_pServicesOHM->Release();
	m_pServicesOHM=NULL;

	if (m_pServicesSystem!=NULL)
		m_pServicesSystem->Release();
	m_pServicesSystem=NULL;

	if (m_pLocator!=NULL)
		m_pLocator->Release();
	m_pLocator=NULL;

	CoUninitialize();
	StopOpenHardwareMonitor();
}
void COpenHardwareMonitor::StartOpenHardwareMonitor(MainWorker *pMainWorker)
{

#ifdef _DEBUG
        _log.Log(LOG_NORM,"OpenHardwareMonitor started");
#endif
	m_pMain=pMainWorker;
	Init();
	
	m_thread = boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&COpenHardwareMonitor::Do_Work, this)));
}

void COpenHardwareMonitor::Init()
{
	// Check if there is already hardware running for ohwm, if no start it.
	
	hwId = 0;
	std::stringstream szQuery;
	std::vector<std::vector<std::string> > result;
	szQuery << "SELECT ID FROM Hardware WHERE (Type=='" <<HTYPE_System << "') AND (Name=='Motherboard') LIMIT 1";
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
		szQuery << "SELECT ID FROM Hardware WHERE (Type==" <<HTYPE_System << ") AND (Name=='Motherboard') LIMIT 1";
		result=m_pMain->m_sql.query(szQuery.str());
	}
	
	if (result.size()>0)
    {
		std::vector<std::string> sd=result[0];
		hwId=atoi(sd[0].c_str());
	}
#ifdef _DEBUG
    _log.Log(LOG_NORM,"OHWM Id set to: %d",hwId);
#endif	
}

void COpenHardwareMonitor::InitWMI()
{
	hr = CoInitializeEx(0, COINIT_MULTITHREADED);
    if (SUCCEEDED(hr))
	{
		hr = CoInitializeSecurity(NULL, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_DEFAULT, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE, NULL);
		if (SUCCEEDED(hr)) 
		{
			hr = CoCreateInstance(CLSID_WbemAdministrativeLocator, NULL, CLSCTX_INPROC_SERVER, IID_IWbemLocator, (LPVOID*)&m_pLocator);
			if (SUCCEEDED(hr))
			{
				hr = m_pLocator->ConnectServer(L"root\\OpenHardwareMonitor",NULL, NULL, NULL, 0, NULL, NULL, &m_pServicesOHM);
				hr = m_pLocator->ConnectServer(L"root\\CIMV2",NULL, NULL, NULL, 0, NULL, NULL, &m_pServicesSystem);
			}
		}
	}
}


void COpenHardwareMonitor::StopOpenHardwareMonitor()
{
	if (m_thread!=NULL)
	{
		m_stoprequested = true;
		m_thread->join();
	}
	if (m_pLocator!=NULL)
		m_pLocator->Release(); 
	m_pLocator=NULL;

	if (m_pServicesOHM!=NULL)
		m_pServicesOHM->Release();
	m_pServicesOHM=NULL;

	if (m_pServicesSystem!=NULL)
		m_pServicesSystem->Release();
	m_pServicesSystem=NULL;
}


void COpenHardwareMonitor::Do_Work()
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
	_log.Log(LOG_NORM,"OpenHardWareMonitor stopped...");			

}


void COpenHardwareMonitor::FetchData()
{
	if (IsOHMRunning()) 
	{
		RunWMIQuery("Sensor","Temperature");
		RunWMIQuery("Sensor","Load");
		RunWMIQuery("Sensor","Fan");
	}
}

bool COpenHardwareMonitor::IsOHMRunning()
{
	if (m_pServicesSystem==NULL)
		return false;
	IEnumWbemClassObject* pEnumerator = NULL;  
	hr = m_pServicesSystem->ExecQuery(L"WQL", L"Select * from win32_Process WHERE Name='OpenHardwareMonitor.exe'" , WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, NULL, &pEnumerator);  
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

void COpenHardwareMonitor::RunWMIQuery(const char* qTable,const char* qType)
{
	if (m_pServicesOHM && m_pServicesSystem)
	{
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
				hr = pclsObj->Get(L"InstanceId", 0, &vtProp, 0, 0);
				std::string itemId = _bstr_t (vtProp.bstrVal);
				itemId = "WMI"+itemId;
				//_log.Log(LOG_NORM, "%s, %s, %s",itemId.c_str(), itemName.c_str(),itemValue.str().c_str());
				UpdateSystemSensor(qType, itemId, itemName, itemValue.str());
				VariantClear(&vtProp);
				uReturn = 0;
				pclsObj->Release();
			}
			pEnumerator->Release();
		}
	}
	else {
		//_log.Log(LOG_NORM, "pservices null");
	}
}

void COpenHardwareMonitor::UpdateSystemSensor(const std::string& qType, const std::string& wmiId, const std::string& devName, const std::string& devValue)
{
	if (!hwId) {
#ifdef _DEBUG
		_log.Log(LOG_NORM,"OHWM Id not found!");
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
		if (qType=="Temperature") {
			szQuery << 
				"INSERT INTO DeviceStatus (HardwareID, DeviceID, Unit, Type, SubType, SignalLevel, BatteryLevel, Name, nValue, sValue) "
				"VALUES (" << hwId << ",'" << wmiId << "',"<< 0 << "," << pTypeGeneral << "," <<sTypeSystemTemp << ",12,255,'" << devName << "'," << devValue << ",'" << devValue << "')";
			m_pMain->m_sql.query(szQuery.str());
		}
		else if (qType=="Load") {
			szQuery << 
				"INSERT INTO DeviceStatus (HardwareID, DeviceID, Unit, Type, SubType, SignalLevel, BatteryLevel, Name, nValue, sValue) "
				"VALUES (" << hwId << ",'" << wmiId << "',"<< 0 << "," << pTypeGeneral << "," <<sTypeSystemLoad << ",12,255,'" << devName << "'," << devValue << ",'" << devValue << "')";
			m_pMain->m_sql.query(szQuery.str());
		}
		else if (qType=="Fan") {
			szQuery << 
				"INSERT INTO DeviceStatus (HardwareID, DeviceID, Unit, Type, SubType, SignalLevel, BatteryLevel, Name, nValue, sValue) "
				"VALUES (" << hwId << ",'" << wmiId << "',"<< 0 << "," << pTypeGeneral << "," <<sTypeSystemFan << ",12,255,'" << devName << "'," << devValue << ",'" << devValue << "')";
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

		ltime.tm_year+1900,ltime.tm_mon+1, ltime.tm_mday, ltime.tm_hour, ltime.tm_min, ltime.tm_sec,
		
		
		m_pMain->m_sql.query(szQuery.str());

		if (qType == "Load") {
			m_pMain->m_sql.CheckAndHandleNotification(hwId, wmiId, 0, pTypeGeneral, sTypeSystemLoad, NTYPE_PERCENTAGE, (const float)atof(devValue.c_str()));
		}
		else if (qType == "Temperature") {
			m_pMain->m_sql.CheckAndHandleNotification(hwId, wmiId, 0, pTypeGeneral, sTypeSystemTemp, NTYPE_TEMPERATURE, (const float)atof(devValue.c_str()));
		}
		else if (qType == "Fan") {
			m_pMain->m_sql.CheckAndHandleNotification(hwId, wmiId, 0, pTypeGeneral, sTypeSystemFan, NTYPE_RPM, (const float)atof(devValue.c_str()));
		}

	}
	return;
}

void COpenHardwareMonitor::WriteMessageStart()
{
	_log.LogSequenceStart();
}

void COpenHardwareMonitor::WriteMessageEnd()
{
	_log.LogSequenceEnd(LOG_NORM);
}

void COpenHardwareMonitor::WriteMessage(const char *szMessage)
{
	_log.LogSequenceAdd(szMessage);
}

void COpenHardwareMonitor::WriteMessage(const char *szMessage, bool linefeed)
{
	if (linefeed)
		_log.LogSequenceAdd(szMessage);
	else
		_log.LogSequenceAddNoLF(szMessage);
}

#endif //WIN32
