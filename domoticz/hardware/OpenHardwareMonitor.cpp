#include "stdafx.h"
#include "OpenHardwareMonitor.h"
#include "../main/Helper.h"
#include "../main/Logger.h"
#include "hardwaretypes.h"
#include "../main/localtime_r.h"
#include "../main/mainworker.h"
#include <iostream>
#include "../main/Helper.h"
#include "../main/SQLHelper.h"

#ifdef WIN32
#include <comdef.h>

#define POLL_INTERVAL 30
#define SLEEP_INTERVAL 1000

#define round(a) ( int ) ( a + .5 )

COpenHardwareMonitor::COpenHardwareMonitor()
{
	m_pMain=NULL;
	m_stoprequested=false;
}


COpenHardwareMonitor::~COpenHardwareMonitor(void)
{
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
	szQuery << "SELECT ID FROM Hardware WHERE (Type=='" <<HTYPE_System << "') AND (Name=='Internal sensors') LIMIT 1";
	result=m_pMain->m_sql.query(szQuery.str());
	if (result.size()<1)
	{
		szQuery.clear();
		szQuery.str("");
		szQuery << "INSERT INTO Hardware (Name, Enabled, Type, Address, Port, Username, Password, Mode1, Mode2, Mode3, Mode4, Mode5) VALUES ('Internal sensors',1, '" << HTYPE_System << "','',1,'','',0,0,0,0,0)";
		m_pMain->m_sql.query(szQuery.str());
		szQuery.clear();
		szQuery.str("");
		szQuery << "SELECT MAX(ID) FROM Hardware";
		result=m_pMain->m_sql.query(szQuery.str());
		if (result.size()>0)
		{
			std::vector<std::string> sd=result[0];
			hwId=atoi(sd[0].c_str());
			m_pMain->AddHardwareFromParams(hwId,"Internal sensors",1,HTYPE_System,"",1,"","",0,0,0,0,0);
		}

		m_pMain->m_sql.query(szQuery.str());
		szQuery.clear();
		szQuery.str("");
		szQuery << "SELECT ID FROM Hardware WHERE (Type==" <<HTYPE_System << ") AND (Name=='Internal sensors') LIMIT 1";
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


void COpenHardwareMonitor::StopOpenHardwareMonitor()
{
	if (m_thread!=NULL)
	{
		m_stoprequested = true;
		m_thread->join();
	}
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
			GetWinCpuTemperature();
		}

	}
	_log.Log(LOG_NORM,"OpenHardWareMonitor stopped...");			

}

void COpenHardwareMonitor::GetWinCpuTemperature()
{
	LONG pTemperature = -1;
    HRESULT ci = CoInitialize(NULL); 
    HRESULT hr = CoInitializeSecurity(NULL, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_DEFAULT, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE, NULL);
    if (SUCCEEDED(hr))
    {
		IWbemLocator *pLocator;  
        hr = CoCreateInstance(CLSID_WbemAdministrativeLocator, NULL, CLSCTX_INPROC_SERVER, IID_IWbemLocator, (LPVOID*)&pLocator);
        if (SUCCEEDED(hr))
        {
			IWbemServices *pServices;
         	 if(FAILED(hr = pLocator->ConnectServer(L"root\\OpenHardwareMonitor",NULL, NULL, NULL, 0, NULL, NULL, &pServices)))  
			 {  
				pLocator->Release();  
				//_log.Log(LOG_NORM, "Unable to connect to OpenHardWareMonitor");
				if (ci == S_OK)
				{
					CoUninitialize();
				}
        		return;  
			 } 
			 else 
			 {
				pLocator->Release();
				IEnumWbemClassObject* pEnumerator = NULL;  
				hr = pServices->ExecQuery(L"WQL", L"SELECT * FROM Sensor WHERE SensorType = 'Temperature'", WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, NULL, &pEnumerator);  
                pServices->Release();
                if (SUCCEEDED(hr))
                {
					//_log.Log(LOG_NORM, "Query OK");
					IWbemClassObject *pclsObj = NULL;
					ULONG uReturn = 0;
					while (pEnumerator)
					{
						
						hr = pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn);
						if(0 == uReturn || FAILED(hr))
							break;
						
						VARIANT vRet;  
						VariantInit(&vRet);  
						if(SUCCEEDED(pclsObj->Get(L"Name", 0, &vRet, NULL, NULL)) && vRet.vt == VT_BSTR) //String 
						{  
							std::string itemName = _bstr_t (vRet.bstrVal);
							itemName = stdreplace(itemName, "#", "");
							//_log.Log(LOG_NORM, "Name: %s",itemName.c_str());
							VariantClear(&vRet);  
							if(SUCCEEDED(pclsObj->Get(L"Value", 0, &vRet, NULL, NULL))) //Float 
							{  
								float temperature = float (vRet.fltVal);
								//_log.Log(LOG_NORM, "Temp: %d",temperature);
								VariantClear(&vRet); 
								if(SUCCEEDED(pclsObj->Get(L"InstanceId", 0, &vRet, NULL, NULL))) //Float 
								{ 
									std::string instanceId = _bstr_t (vRet.bstrVal);
									instanceId = "WMI"+instanceId;
									//int instanceId = atoi(instanceIdS.c_str()); 
									VariantClear(&vRet); 
									// convert now to string form
									time_t now = time(0);
									char *szDate = asctime(localtime(&now));
									szDate[strlen(szDate)-1]=0;
									WriteMessageStart();
									std::stringstream sTmp;
									sTmp << szDate << " (" <<itemName << ") ";
									WriteMessage(sTmp.str().c_str(),false);
									WriteMessage("Temperature",true);
									WriteMessageEnd();

									UpdateSystemSensor(instanceId, itemName, temperature);
									
								}
							}
						}
						pclsObj->Release();
						pclsObj=NULL;
					}
		        }
			
				pEnumerator->Release();

            }
            if (ci == S_OK)
            {
                CoUninitialize();
            }
        }
		else {
			if (ci == S_OK)
			{
				CoUninitialize();
			}
		}
    }
	else {
		if (ci == S_OK)
		{
			CoUninitialize();
		}
	}
}

void COpenHardwareMonitor::UpdateSystemSensor(const std::string& wmiId, const std::string& devName, const float& devValue)
{
	if (!hwId) {
#ifdef _DEBUG
		_log.Log(LOG_NORM,"OHWM Id not found!");
#endif		
		return;
	}
	std::stringstream szQuery;
	std::vector<std::vector<std::string> > result;
	szQuery << "SELECT ID FROM DeviceStatus WHERE (DeviceID=='" << wmiId << "') AND (Type==" << pTypeTEMP << ") AND (SubType=='" << sTypeTEMP_SYSTEM << "')";
	result=m_pMain->m_sql.query(szQuery.str());
	if (result.size()<1)
	{
		szQuery.clear();
		szQuery.str("");

		szQuery << 
			"INSERT INTO DeviceStatus (HardwareID, DeviceID, Unit, Type, SubType, SignalLevel, BatteryLevel, Name, nValue, sValue) "
			"VALUES (" << hwId << ",'" << wmiId << "',"<< 0 << "," << pTypeTEMP << "," <<sTypeTEMP_SYSTEM << ",12,255,'" << devName << "'," << devValue << ",'" << devValue << "')";
		m_pMain->m_sql.query(szQuery.str());
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
