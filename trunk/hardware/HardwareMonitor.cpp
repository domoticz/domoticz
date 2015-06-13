#include "stdafx.h"
#include "HardwareMonitor.h"
#include "../main/Helper.h"
#include "../main/Logger.h"
#include "hardwaretypes.h"
#include "../main/localtime_r.h"
#include "../main/mainworker.h"
#include "../main/SQLHelper.h"
#include <wchar.h>

//Note, for windows we use OpenHardware Monitor
//http://openhardwaremonitor.org/

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

extern bool bHasInternalTemperature;
extern std::string szInternalTemperatureCommand;

extern bool bHasInternalVoltage;
extern std::string szInternalVoltageCommand;

extern bool bHasInternalCurrent;
extern std::string szInternalCurrentCommand;


#define round(a) ( int ) ( a + .5 )

CHardwareMonitor::CHardwareMonitor()
{
	m_stoprequested=false;
	m_bEnabled=true;
	HwdType = HTYPE_System;
	m_bOutputLog = false;
	Name = "Motherboard sensors";
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
void CHardwareMonitor::StartHardwareMonitor()
{
#ifdef _DEBUG
        _log.Log(LOG_STATUS,"Hardware Monitor: Started");
#endif
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
	// Check if there is already a hardware devices for our class, if no add it.
	m_lastquerytime=0;
	m_HwdID = 0;
	std::stringstream szQuery;
	std::vector<std::vector<std::string> > result;
	szQuery << "SELECT ID,Enabled FROM Hardware WHERE (Type=='" <<HTYPE_System << "') AND (Name=='Motherboard') LIMIT 1";
	result=m_sql.query(szQuery.str());
	if (result.size()<1)
	{
		szQuery.clear();
		szQuery.str("");
		szQuery << "INSERT INTO Hardware (Name, Enabled, Type, Address, Port, Username, Password, Mode1, Mode2, Mode3, Mode4, Mode5, Mode6) VALUES ('Motherboard',1, '" << HTYPE_System << "','',1,'','',0,0,0,0,0,0)";
		m_sql.query(szQuery.str());
		szQuery.clear();
		szQuery.str("");
		szQuery << "SELECT MAX(ID) FROM Hardware";
		result=m_sql.query(szQuery.str());
		if (result.size()>0)
		{
			std::vector<std::string> sd=result[0];
			m_HwdID=atoi(sd[0].c_str());
		}

		m_sql.query(szQuery.str());
		szQuery.clear();
		szQuery.str("");
		szQuery << "SELECT ID,Enabled FROM Hardware WHERE (Type==" << HTYPE_System << ") AND (Name=='Motherboard') LIMIT 1";
		result=m_sql.query(szQuery.str());
	}
	
	if (result.size()>0)
    {
		std::vector<std::string> sd=result[0];
		m_HwdID=atoi(sd[0].c_str());
		m_bEnabled=atoi(sd[1].c_str())!=0;
	}
	sOnConnected(this);
}

void CHardwareMonitor::Do_Work()
{
	m_stoprequested=false;
	int sec_counter = 0;
	while (!m_stoprequested)
	{
		sleep_seconds(1);
		sec_counter++;

		if (sec_counter % 12 == 0) {
			m_LastHeartbeat = mytime(NULL);
		}

		if (sec_counter%POLL_INTERVAL == 0)
		{
			FetchData();
		}
	}
	_log.Log(LOG_STATUS,"Hardware Monitor: Stopped...");			

}

void CHardwareMonitor::SendVoltage(const unsigned long Idx, const float Volt, const std::string &defaultname)
{
	bool bDeviceExits = true;
	std::stringstream szQuery;
	std::vector<std::vector<std::string> > result;

	char szTmp[30];
	sprintf(szTmp, "%08X", (unsigned int)Idx);

	szQuery << "SELECT Name FROM DeviceStatus WHERE (HardwareID==" << m_HwdID << ") AND (DeviceID=='" << szTmp << "') AND (Type==" << int(pTypeGeneral) << ") AND (Subtype==" << int(sTypeVoltage) << ")";
	result = m_sql.query(szQuery.str());
	if (result.size() < 1)
	{
		bDeviceExits = false;
	}

	_tGeneralDevice gDevice;
	gDevice.subtype = sTypeVoltage;
	gDevice.id = 1;
	gDevice.floatval1 = Volt;
	gDevice.intval1 = static_cast<int>(Idx);
	sDecodeRXMessage(this, (const unsigned char *)&gDevice);

	if (!bDeviceExits)
	{
		//Assign default name for device
		szQuery.clear();
		szQuery.str("");
		szQuery << "UPDATE DeviceStatus SET Name='" << defaultname << "' WHERE (HardwareID==" << m_HwdID << ") AND (DeviceID=='" << szTmp << "') AND (Type==" << int(pTypeGeneral) << ") AND (Subtype==" << int(sTypeVoltage) << ")";
		result = m_sql.query(szQuery.str());
	}
}

void CHardwareMonitor::SendCurrent(const unsigned long Idx, const float Curr, const std::string &defaultname)
{
	bool bDeviceExits = true;
	std::stringstream szQuery;
	std::vector<std::vector<std::string> > result;

	char szTmp[30];
	sprintf(szTmp, "%08X", (unsigned int)Idx);

	szQuery << "SELECT Name FROM DeviceStatus WHERE (HardwareID==" << m_HwdID << ") AND (DeviceID=='" << szTmp << "') AND (Type==" << int(pTypeGeneral) << ") AND (Subtype==" << int(sTypeCurrent) << ")";
	result = m_sql.query(szQuery.str());
	if (result.size() < 1)
	{
		bDeviceExits = false;
	}

	_tGeneralDevice gDevice;
	gDevice.subtype = sTypeCurrent;
	gDevice.id = 1;
	gDevice.floatval1 = Curr;
	gDevice.intval1 = static_cast<int>(Idx);
	sDecodeRXMessage(this, (const unsigned char *)&gDevice);

	if (!bDeviceExits)
	{
		//Assign default name for device
		szQuery.clear();
		szQuery.str("");
		szQuery << "UPDATE DeviceStatus SET Name='" << defaultname << "' WHERE (HardwareID==" << m_HwdID << ") AND (DeviceID=='" << szTmp << "') AND (Type==" << int(pTypeGeneral) << ") AND (Subtype==" << int(sTypeCurrent) << ")";
		result = m_sql.query(szQuery.str());
	}
}

void CHardwareMonitor::SendTempSensor(const int Idx, const float Temp, const std::string &defaultname)
{
	bool bDeviceExits = true;
	std::stringstream szQuery;
	std::vector<std::vector<std::string> > result;
	szQuery << "SELECT Name FROM DeviceStatus WHERE (HardwareID==" << m_HwdID << ") AND (DeviceID==" << int(Idx) << ") AND (Type==" << int(pTypeTEMP) << ") AND (Subtype==" << int(sTypeTEMP10) << ")";
	result = m_sql.query(szQuery.str());
	if (result.size() < 1)
	{
		bDeviceExits = false;
	}

	RBUF tsen;
	memset(&tsen, 0, sizeof(RBUF));

	tsen.TEMP.packetlength = sizeof(tsen.TEMP) - 1;
	tsen.TEMP.packettype = pTypeTEMP;
	tsen.TEMP.subtype = sTypeTEMP10;
	tsen.TEMP.battery_level = 9;
	tsen.TEMP.rssi = 12;
	tsen.TEMP.id1 = (unsigned char)(Idx>>8);
	tsen.TEMP.id2 = (unsigned char)Idx&0xFF;

	tsen.TEMP.tempsign = (Temp >= 0) ? 0 : 1;
	int at10 = round(abs(Temp*10.0f));
	tsen.TEMP.temperatureh = (BYTE)(at10 / 256);
	at10 -= (tsen.TEMP.temperatureh * 256);
	tsen.TEMP.temperaturel = (BYTE)(at10);

	sDecodeRXMessage(this, (const unsigned char *)&tsen.TEMP);//decode message

	if (!bDeviceExits)
	{
		//Assign default name for device
		szQuery.clear();
		szQuery.str("");
		szQuery << "UPDATE DeviceStatus SET Name='" << defaultname << "' WHERE (HardwareID==" << m_HwdID << ") AND (DeviceID==" << int(Idx) << ") AND (Type==" << int(pTypeTEMP) << ") AND (Subtype==" << int(sTypeTEMP10) << ")";
		result = m_sql.query(szQuery.str());
	}
}

void CHardwareMonitor::SendPercentage(const unsigned long Idx, const float Percentage, const std::string &defaultname)
{
	bool bDeviceExits = true;
	std::stringstream szQuery;
	std::vector<std::vector<std::string> > result;

	char szTmp[30];
	sprintf(szTmp, "%08X", (unsigned int)Idx);

	szQuery << "SELECT Name FROM DeviceStatus WHERE (HardwareID==" << m_HwdID << ") AND (DeviceID=='" << szTmp << "') AND (Type==" << int(pTypeGeneral) << ") AND (Subtype==" << int(sTypePercentage) << ")";
	result = m_sql.query(szQuery.str());
	if (result.size() < 1)
	{
		bDeviceExits = false;
	}

	_tGeneralDevice gDevice;
	gDevice.subtype = sTypePercentage;
	gDevice.id = 1;
	gDevice.floatval1 = Percentage;
	gDevice.intval1 = static_cast<int>(Idx);
	sDecodeRXMessage(this, (const unsigned char *)&gDevice);

	if (!bDeviceExits)
	{
		//Assign default name for device
		szQuery.clear();
		szQuery.str("");
		szQuery << "UPDATE DeviceStatus SET Name='" << defaultname << "' WHERE (HardwareID==" << m_HwdID << ") AND (DeviceID=='" << szTmp << "') AND (Type==" << int(pTypeGeneral) << ") AND (Subtype==" << int(sTypePercentage) << ")";
		result = m_sql.query(szQuery.str());

	}
}

void CHardwareMonitor::SendFanSensor(const int Idx, const int FanSpeed, const std::string &defaultname)
{
	bool bDeviceExits = true;
	std::stringstream szQuery;
	std::vector<std::vector<std::string> > result;

	char szTmp[30];
	sprintf(szTmp, "%08X", (unsigned int)Idx);

	szQuery << "SELECT Name FROM DeviceStatus WHERE (HardwareID==" << m_HwdID << ") AND (DeviceID=='" << szTmp << "') AND (Type==" << int(pTypeGeneral) << ") AND (Subtype==" << int(sTypeFan) << ")";
	result = m_sql.query(szQuery.str());
	if (result.size() < 1)
	{
		bDeviceExits = false;
	}

	_tGeneralDevice gDevice;
	gDevice.subtype = sTypeFan;
	gDevice.id = 1;
	gDevice.intval1 = static_cast<int>(Idx);
	gDevice.intval2 = FanSpeed;
	sDecodeRXMessage(this, (const unsigned char *)&gDevice);

	if (!bDeviceExits)
	{
		//Assign default name for device
		szQuery.clear();
		szQuery.str("");
		szQuery << "UPDATE DeviceStatus SET Name='" << defaultname << "' WHERE (HardwareID==" << m_HwdID << ") AND (DeviceID=='" << szTmp << "') AND (Type==" << int(pTypeGeneral) << ") AND (Subtype==" << int(sTypeFan) << ")";
		result = m_sql.query(szQuery.str());

	}
}

void CHardwareMonitor::GetInternalTemperature()
{
	std::vector<std::string> ret = ExecuteCommandAndReturn(szInternalTemperatureCommand.c_str());
	if (ret.size() < 1)
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

	if ((temperature != 85) && (temperature > -273))
	{
		SendTempSensor(1, temperature, "Internal Temperature");
	}
}

void CHardwareMonitor::GetInternalVoltage()
{
	std::vector<std::string> ret = ExecuteCommandAndReturn(szInternalVoltageCommand.c_str());
	if (ret.size() < 1)
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

	SendVoltage(1, voltage, "Internal Voltage");
}

void CHardwareMonitor::GetInternalCurrent()
{
	std::vector<std::string> ret = ExecuteCommandAndReturn(szInternalCurrentCommand.c_str());
	if (ret.size() < 1)
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
	}
#elif defined __linux__
	_log.Log(LOG_NORM,"Hardware Monitor: Fetching data (System sensors)");
	FetchUnixData();
	if (bHasInternalTemperature)
	{
		GetInternalTemperature();
	}
	if (bHasInternalVoltage)
	{
		GetInternalVoltage();
	}
	if (bHasInternalCurrent)
	{
		GetInternalCurrent();
	}
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
		SendTempSensor(doffset + dindex, temp, devName);
	}
	else if (qType == "Load")
	{
		doffset = 1100;
		float perc = static_cast<float>(atof(devValue.c_str()));
		SendPercentage(doffset + dindex, perc, devName);
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
		SendVoltage(doffset + dindex, volt, devName);
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
		int procId = static_cast<int>(vtProp.iVal);
		if (procId) return true;
	}
	return false;
}

void CHardwareMonitor::RunWMIQuery(const char* qTable,const char* qType)
{
	if (pServicesOHM && pServicesSystem)
	{
		HRESULT hr;
		int dindex = 0;
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
		UpdateSystemSensor("Load", 0, "Memory Usage", szTmp);

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
						UpdateSystemSensor("Load", 1, "CPU_Usage", szTmp);
					}
					m_lastloadcpu=actload1+actload2+actload3;
				}
			}
			m_lastquerytime=acttime;
		}

		//Disk Usage
		std::map<std::string, _tDUsageStruct> _disks;
		std::map<std::string, std::string> _dmounts_;
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
				std::map<std::string, std::string>::iterator it = _dmounts_.find(dname);
				if (it != _dmounts_.end())
				{
					if (it->second.length() < strlen(smountpoint))
					{
						continue;
					}
				}
				if (strstr(dname,"/dev")!=NULL)
				{
					_tDUsageStruct dusage;
					dusage.TotalBlocks=numblock;
					dusage.UsedBlocks=usedblocks;
					dusage.AvailBlocks=availblocks;
					dusage.MountPoint=smountpoint;
					_disks[dname]=dusage;
					_dmounts_[dname]=smountpoint;
				}
			}
		}
		int dindex=0;
		std::map<std::string, _tDUsageStruct>::const_iterator ittDisks;
		for (ittDisks=_disks.begin(); ittDisks!=_disks.end(); ++ittDisks)
		{
			_tDUsageStruct dusage=(*ittDisks).second;
			if (dusage.TotalBlocks>0)
			{
				double UsagedPercentage=(100 / double(dusage.TotalBlocks))*double(dusage.UsedBlocks);
				//std::cout << "Disk: " << (*ittDisks).first << ", Mount: " << dusage.MountPoint << ", Used: " << UsagedPercentage << std::endl;
				sprintf(szTmp,"%.2f", UsagedPercentage);
				std::string hddname = "HDD " + dusage.MountPoint;
				UpdateSystemSensor("Load", 2+dindex, hddname, szTmp);
				dindex++;
			}
		}
	}
#endif //WIN32/#elif defined __linux__

