#include "stdafx.h"
#include "WebServer.h"
#include <boost/bind.hpp>
#include "webserver/cWebem.h"
#include <iostream>
#include "mainworker.h"
#include "RFXNames.h"
#include "RFXtrx.h"
#include "Helper.h"

namespace http {
	namespace server {

CWebServer::CWebServer(void)
{
	m_pWebEm=NULL;
}


CWebServer::~CWebServer(void)
{
	StopServer();
	if (m_pWebEm!=NULL)
		delete m_pWebEm;
	m_pWebEm=NULL;
	SaveUsers();
}

void CWebServer::Do_Work()
{
	if (m_pWebEm)
		m_pWebEm->Run();

	std::cout << "WebServer stopped...\n";
}



bool CWebServer::StartServer(MainWorker *pMain, std::string listenaddress, std::string listenport, std::string serverpath)
{
	m_pMain=pMain;
	StopServer();

	LoadUsers();

	if (m_pWebEm!=NULL)
		delete m_pWebEm;

	m_pWebEm= new http::server::cWebem(
		listenaddress.c_str(),						// address
		listenport.c_str(),							// port
		serverpath.c_str());
	if (m_pWebEm==NULL)
		return false;

	m_pWebEm->SetDigistRealm("DVBControl.com");

	std::vector<_tWebUserPassword>::const_iterator itt;
	for (itt=m_users.begin(); itt!=m_users.end(); ++itt)
	{
		m_pWebEm->AddUserPassword(itt->Username, itt->Password);
	}

	//register callbacks
	m_pWebEm->RegisterIncludeCode( "versionstr",
		boost::bind(
		&CWebServer::DisplayVersion,	// member function
		this ) );			// instance of class

	m_pWebEm->RegisterIncludeCode( "switchtypes",
		boost::bind(
		&CWebServer::DisplaySwitchTypesCombo,	// member function
		this ) );			// instance of class

	m_pWebEm->RegisterIncludeCode( "metertypes",
		boost::bind(
		&CWebServer::DisplayMeterTypesCombo,
		this ) );

	m_pWebEm->RegisterIncludeCode( "hardwaretypes",
		boost::bind(
		&CWebServer::DisplayHardwareTypesCombo,
		this ) );

	m_pWebEm->RegisterIncludeCode( "combohardware",
		boost::bind(
		&CWebServer::DisplayHardwareCombo,
		this ) );

	m_pWebEm->RegisterIncludeCode( "serialdevices",
		boost::bind(
		&CWebServer::DisplaySerialDevicesCombo,
		this ) );

	m_pWebEm->RegisterIncludeCode( "timertypes",
		boost::bind(
		&CWebServer::DisplayTimerTypesCombo,	// member function
		this ) );			// instance of class

	m_pWebEm->RegisterPageCode( "/json.htm",
		boost::bind(
		&CWebServer::GetJSonPage,	// member function
		this ) );			// instance of class

	m_pWebEm->RegisterActionCode( "storesettings",boost::bind(&CWebServer::PostSettings,this));

	//Start worker thread
	m_thread = boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&CWebServer::Do_Work, this)));

	return (m_thread!=NULL);
}

void CWebServer::StopServer()
{
	if (m_pWebEm==NULL)
		return;
	m_pWebEm->Stop();
}

char * CWebServer::DisplayVersion()
{
	m_retstr=VERSION_STRING;
	return (char*)m_retstr.c_str();
}

char * CWebServer::DisplaySwitchTypesCombo()
{
	m_retstr="";
	char szTmp[200];
	for (int ii=0; ii<STYPE_END; ii++)
	{
		sprintf(szTmp,"<option value=\"%d\">%s</option>\n",ii,Switch_Type_Desc((_eSwitchType)ii));
		m_retstr+=szTmp;
	}
	return (char*)m_retstr.c_str();
}

char * CWebServer::DisplayMeterTypesCombo()
{
	m_retstr="";
	char szTmp[200];
	for (int ii=0; ii<MTYPE_END; ii++)
	{
		sprintf(szTmp,"<option value=\"%d\">%s</option>\n",ii,Meter_Type_Desc((_eMeterType)ii));
		m_retstr+=szTmp;
	}
	return (char*)m_retstr.c_str();
}


char * CWebServer::DisplayHardwareCombo()
{
	m_retstr="";
	char szTmp[200];

	std::vector<std::vector<std::string> > result;
	std::stringstream szQuery;
	szQuery << "SELECT ID, Name, Type FROM Hardware ORDER BY ID ASC";
	result=m_pMain->m_sql.query(szQuery.str());
	if (result.size()>0)
	{
		std::vector<std::vector<std::string> >::const_iterator itt;
		for (itt=result.begin(); itt!=result.end(); ++itt)
		{
			std::vector<std::string> sd=*itt;

			int ID=atoi(sd[0].c_str());
			std::string Name=sd[1];
			_eHardwareTypes Type=(_eHardwareTypes)atoi(sd[2].c_str());
			switch (Type)
			{
			case HTYPE_RFXLAN:
			case HTYPE_RFXtrx315:
			case HTYPE_RFXtrx433:
				sprintf(szTmp,"<option value=\"%d\">%s</option>\n",ID,Name.c_str());
				m_retstr+=szTmp;
				break;
			}
		}
	}
	return (char*)m_retstr.c_str();
}

char * CWebServer::DisplayHardwareTypesCombo()
{
	m_retstr="";
	char szTmp[200];
	for (int ii=0; ii<HTYPE_END; ii++)
	{
		sprintf(szTmp,"<option value=\"%d\">%s</option>\n",ii,Hardware_Type_Desc(ii));
		m_retstr+=szTmp;
	}
	return (char*)m_retstr.c_str();
}

char * CWebServer::DisplaySerialDevicesCombo()
{
	m_retstr="";
	char szTmp[200];
	std::vector<std::string> serialports=GetSerialPorts();
	std::vector<std::string>::iterator itt;
	for (itt=serialports.begin(); itt!=serialports.end(); ++itt)
	{
		std::string serialname=*itt;
		int pos=serialname.find_first_of("01234567890");
		if (pos!=std::string::npos) {
			int snumber=atoi(serialname.substr(pos).c_str());
			sprintf(szTmp,"<option value=\"%d\">%s</option>\n",snumber,serialname.c_str());
		}
		m_retstr+=szTmp;
	}
	return (char*)m_retstr.c_str();
}


char * CWebServer::DisplayTimerTypesCombo()
{
	m_retstr="";
	char szTmp[200];
	for (int ii=0; ii<TTYPE_END; ii++)
	{
		sprintf(szTmp,"<option value=\"%d\">%s</option>\n",ii,Timer_Type_Desc(ii));
		m_retstr+=szTmp;
	}
	return (char*)m_retstr.c_str();
}

void CWebServer::LoadUsers()
{
	m_users.clear();

}

void CWebServer::SaveUsers()
{
}

int CWebServer::FindUser(const char* szUserName)
{
	int iUser=0;
	std::vector<_tWebUserPassword>::const_iterator itt;
	for (itt=m_users.begin(); itt!=m_users.end(); ++itt)
	{
		if (itt->Username==szUserName)
			return iUser;
		iUser++;
	}
	return -1;
}


char * CWebServer::PostSettings()
{
	m_retstr="/index.html";
	std::vector<std::vector<std::string> > result;
	std::stringstream szQuery;

	std::string Latitude=m_pWebEm->FindValue("Latitude");
	std::string Longitude=m_pWebEm->FindValue("Longitude");
	if ( (Latitude!="")&&(Longitude!="") )
	{
		std::string LatLong=Latitude+";"+Longitude;
		m_pMain->m_sql.UpdatePreferencesVar("Location",LatLong.c_str());
		m_pMain->GetSunSettings();
	}
	std::string ProwlAPI=m_pWebEm->FindValue("ProwlAPIKey");
	m_pMain->m_sql.UpdatePreferencesVar("ProwlAPI",ProwlAPI.c_str());
	std::string NMAAPI=m_pWebEm->FindValue("NMAAPIKey");
	m_pMain->m_sql.UpdatePreferencesVar("NMAAPI",NMAAPI.c_str());
	std::string DashboardType=m_pWebEm->FindValue("DashboardType");
	m_pMain->m_sql.UpdatePreferencesVar("DashboardType",atoi(DashboardType.c_str()));

	std::string LightHistoryDays=m_pWebEm->FindValue("LightHistoryDays");
	m_pMain->m_sql.UpdatePreferencesVar("LightHistoryDays",atoi(LightHistoryDays.c_str()));
	

	return (char*)m_retstr.c_str();
}

void CWebServer::GetJSonDevices(Json::Value &root, std::string rused, std::string rfilter, std::string order)
{
	std::vector<std::vector<std::string> > result;
	std::stringstream szQuery;

	//Get All Hardware ID's/Names, need them later
	std::map<int,std::string> _hardwareNames;
	szQuery.clear();
	szQuery.str("");
	szQuery << "SELECT ID, Name FROM Hardware";
	result=m_pMain->m_sql.query(szQuery.str());
	if (result.size()>0)
	{
		std::vector<std::vector<std::string> >::const_iterator itt;
		int ii=0;
		for (itt=result.begin(); itt!=result.end(); ++itt)
		{
			std::vector<std::string> sd=*itt;
			int ID=atoi(sd[0].c_str());
			std::string Name=sd[1];
			_hardwareNames[ID]=Name;
		}
	}

	int nValue=0;
	m_pMain->m_sql.GetPreferencesVar("DashboardType",nValue);
	root["DashboardType"]=nValue;

	char szData[100];
	char szTmp[300];

	char szOrderBy[50];
	if (order=="")
		strcpy(szOrderBy,"LastUpdate DESC");
	else
	{
		sprintf(szOrderBy,"[Order],%s ASC",order.c_str());
	}

	szQuery.clear();
	szQuery.str("");
	szQuery << "SELECT ID, DeviceID, Unit, Name, Used, Type, SubType, SignalLevel, BatteryLevel, nValue, sValue, LastUpdate, Favorite, SwitchType, HardwareID FROM DeviceStatus ORDER BY " << szOrderBy;
	result=m_pMain->m_sql.query(szQuery.str());
	if (result.size()>0)
	{
		std::vector<std::vector<std::string> >::const_iterator itt;
		int ii=0;
		for (itt=result.begin(); itt!=result.end(); ++itt)
		{
			std::vector<std::string> sd=*itt;

			unsigned char dType=atoi(sd[5].c_str());
			unsigned char dSubType=atoi(sd[6].c_str());
			unsigned char used = atoi(sd[4].c_str());
			unsigned char nValue = atoi(sd[9].c_str());
			std::string sValue=sd[10];
			unsigned char favorite = atoi(sd[12].c_str());
			_eSwitchType switchtype=(_eSwitchType) atoi(sd[13].c_str());
			_eMeterType metertype=(_eMeterType)switchtype;
			int hardwareID= atoi(sd[14].c_str());

			if (
				(rused=="true")&&
				(!used)
				)
				continue;
			if (
				(rused=="false")&&
				(used)
				)
				continue;
			if (rfilter!="")
			{
				if (rfilter=="light")
				{
					if (
						(dType!=pTypeLighting1)&&
						(dType!=pTypeLighting2)&&
						(dType!=pTypeLighting3)&&
						(dType!=pTypeLighting4)&&
						(dType!=pTypeLighting5)&&
						(dType!=pTypeLighting6)&&
						(dType!=pTypeSecurity1)
						)
						continue;
				}
				else if (rfilter=="temp")
				{
					if (
						(dType!=pTypeTEMP)&&
						(dType!=pTypeHUM)&&
						(dType!=pTypeTEMP_HUM)&&
						(dType!=pTypeTEMP_HUM_BARO)&&
						(!((dType==pTypeWIND)&&(dSubType==sTypeWIND4)))&&
						(dType!=pTypeUV)&&
						(dType!=pTypeThermostat1)&&
						(!((dType==pTypeRFXSensor)&&(dSubType==sTypeRFXSensorTemp)))
						)
						continue;
				}
				else if (rfilter=="weather")
				{
					if (
						(dType!=pTypeWIND)&&
						(dType!=pTypeRAIN)&&
						(dType!=pTypeTEMP_HUM_BARO)&&
						(dType!=pTypeUV)
						)
						continue;
				}
				else if (rfilter=="utility")
				{
					if (
						(dType!=pTypeRFXMeter)&&
						(!((dType==pTypeRFXSensor)&&(dSubType==sTypeRFXSensorAD)))&&
						(!((dType==pTypeRFXSensor)&&(dSubType==sTypeRFXSensorVolt)))
						)
						continue;
				}
				else if (rfilter=="wind")
				{
					if (
						(dType!=pTypeWIND)
						)
						continue;
				}
				else if (rfilter=="rain")
				{
					if (
						(dType!=pTypeRAIN)
						)
						continue;
				}
				else if (rfilter=="uv")
				{
					if (
						(dType!=pTypeUV)
						)
						continue;
				}
				else if (rfilter=="baro")
				{
					if (
						(dType!=pTypeTEMP_HUM_BARO)
						)
						continue;
				}
			}

			root["result"][ii]["HardwareID"]=hardwareID;
			if (_hardwareNames.find(hardwareID)==_hardwareNames.end())
				root["result"][ii]["HardwareName"]="Unknown?";
			else
				root["result"][ii]["HardwareName"]=_hardwareNames[hardwareID];
			root["result"][ii]["idx"]=sd[0];
			root["result"][ii]["ID"]=sd[1];
			root["result"][ii]["Unit"]=atoi(sd[2].c_str());
			root["result"][ii]["Type"]=RFX_Type_Desc(dType,1);
			root["result"][ii]["SubType"]=RFX_Type_SubType_Desc(dType,dSubType);
			root["result"][ii]["TypeImg"]=RFX_Type_Desc(dType,2);
			root["result"][ii]["Name"]=(used!=0)?sd[3]:"not used";
			root["result"][ii]["Used"]=used;
			root["result"][ii]["Favorite"]=favorite;
			root["result"][ii]["SignalLevel"]=atoi(sd[7].c_str());
			root["result"][ii]["BatteryLevel"]=atoi(sd[8].c_str());
			root["result"][ii]["LastUpdate"]=sd[11].c_str();

			sprintf(szData,"%d, %s", nValue,sValue.c_str());
			root["result"][ii]["Data"]=szData;

			root["result"][ii]["Notifications"]=(m_pMain->m_sql.HasNotifications(sd[0])==true)?"true":"false";
			root["result"][ii]["Timers"]=(m_pMain->m_sql.HasTimers(sd[0])==true)?"true":"false";

			if (
				(dType==pTypeLighting1)||
				(dType==pTypeLighting2)||
				(dType==pTypeLighting3)||
				(dType==pTypeLighting4)||
				(dType==pTypeLighting5)||
				(dType==pTypeLighting6)
				)
			{
				//add light details
				std::string lstatus="";
				int llevel=0;
				bool bHaveDimmer=false;
				bool bHaveGroupCmd=false;

				GetLightStatus(dType,dSubType,nValue,sValue,lstatus,llevel,bHaveDimmer,bHaveGroupCmd);

				root["result"][ii]["Status"]=lstatus;
				root["result"][ii]["Level"]=llevel;
				root["result"][ii]["HaveDimmer"]=bHaveDimmer;
				root["result"][ii]["HaveGroupCmd"]=bHaveGroupCmd;
				root["result"][ii]["SwitchType"]=Switch_Type_Desc(switchtype);
				root["result"][ii]["SwitchTypeVal"]=switchtype;

				if (switchtype==STYPE_Doorbell)
					root["result"][ii]["TypeImg"]="door";
				else if (switchtype==STYPE_X10Siren)
					root["result"][ii]["TypeImg"]="siren";
				else if (switchtype==STYPE_Contact)
					root["result"][ii]["TypeImg"]="contact";
				else if (switchtype==STYPE_Blinds)
				{
					root["result"][ii]["TypeImg"]="blinds";
					if (lstatus=="On") {
						lstatus="Closed";
					} else {
						lstatus="Open";
					}
					root["result"][ii]["Status"]=lstatus;
				}

				if (llevel!=0)
					sprintf(szData,"%s, Level: %d %%", lstatus.c_str(), llevel);
				else
					sprintf(szData,"%s", lstatus.c_str());
				root["result"][ii]["Data"]=szData;
			}
			else if (dType==pTypeSecurity1)
			{
				std::string lstatus="";
				int llevel=0;
				bool bHaveDimmer=false;
				bool bHaveGroupCmd=false;

				GetLightStatus(dType,dSubType,nValue,sValue,lstatus,llevel,bHaveDimmer,bHaveGroupCmd);

				root["result"][ii]["Status"]=lstatus;
				root["result"][ii]["HaveDimmer"]=bHaveDimmer;
				root["result"][ii]["HaveGroupCmd"]=bHaveGroupCmd;
				root["result"][ii]["SwitchType"]="Security";
				root["result"][ii]["SwitchTypeVal"]=0;

				root["result"][ii]["TypeImg"]="security";

				sprintf(szData,"%s", lstatus.c_str());
				root["result"][ii]["Data"]=szData;
			}
			else if (dType == pTypeTEMP)
			{
				root["result"][ii]["Temp"]=atof(sValue.c_str());
				sprintf(szData,"%.1f C", atof(sValue.c_str()));
				root["result"][ii]["Data"]=szData;
			}
			else if (dType == pTypeThermostat1)
			{
				std::vector<std::string> strarray;
				StringSplit(sValue, ";", strarray);
				if (strarray.size()==4)
				{
					root["result"][ii]["Temp"]=atoi(strarray[0].c_str());
				}
			}
			else if ((dType==pTypeRFXSensor)&&(dSubType==sTypeRFXSensorTemp))
			{
				root["result"][ii]["Temp"]=atof(sValue.c_str());
				sprintf(szData,"%.1f C", atof(sValue.c_str()));
				root["result"][ii]["Data"]=szData;
				root["result"][ii]["TypeImg"]="temperature";
			}
			else if (dType == pTypeHUM)
			{
				root["result"][ii]["Humidity"]=nValue;
				root["result"][ii]["HumidityStatus"]=RFX_Humidity_Status_Desc(atoi(sValue.c_str()));
				sprintf(szData,"Humidity %d %%", nValue);
				root["result"][ii]["Data"]=szData;
			}
			else if (dType == pTypeTEMP_HUM)
			{
				std::vector<std::string> strarray;
				StringSplit(sValue, ";", strarray);
				if (strarray.size()==3)
				{
					root["result"][ii]["Temp"]=atof(strarray[0].c_str());
					root["result"][ii]["Humidity"]=atoi(strarray[1].c_str());
					root["result"][ii]["HumidityStatus"]=RFX_Humidity_Status_Desc(atoi(strarray[2].c_str()));
					sprintf(szData,"%.1f C, %d %%", atof(strarray[0].c_str()),atoi(strarray[1].c_str()));
					root["result"][ii]["Data"]=szData;
				}
			}
			else if (dType == pTypeTEMP_HUM_BARO)
			{
				std::vector<std::string> strarray;
				StringSplit(sValue, ";", strarray);
				if (strarray.size()==5)
				{
					root["result"][ii]["Temp"]=atof(strarray[0].c_str());
					root["result"][ii]["Humidity"]=atoi(strarray[1].c_str());
					root["result"][ii]["HumidityStatus"]=RFX_Humidity_Status_Desc(atoi(strarray[2].c_str()));
					root["result"][ii]["Barometer"]=atoi(strarray[3].c_str());
					root["result"][ii]["Forecast"]=atoi(strarray[4].c_str());
					root["result"][ii]["ForecastStr"]=RFX_Forecast_Desc(atoi(strarray[4].c_str()));
					sprintf(szData,"%.1f C, %d %%, %d hPa",
						atof(strarray[0].c_str()),
						atoi(strarray[1].c_str()),
						atoi(strarray[3].c_str())
						);
					root["result"][ii]["Data"]=szData;
				}
			}
			else if (dType == pTypeUV)
			{
				std::vector<std::string> strarray;
				StringSplit(sValue, ";", strarray);
				if (strarray.size()==2)
				{
					float UVI=(float)atof(strarray[0].c_str());
					float Temp=(float)atof(strarray[1].c_str());
					root["result"][ii]["UVI"]=strarray[0];
					root["result"][ii]["Temp"]=strarray[1];
					sprintf(szData,"%.1f UVI, %.1f&deg; C",UVI,Temp);
					root["result"][ii]["Data"]=szData;
				}
			}
			else if (dType == pTypeWIND)
			{
				std::vector<std::string> strarray;
				StringSplit(sValue, ";", strarray);
				if (strarray.size()==6)
				{
					root["result"][ii]["Direction"]=atof(strarray[0].c_str());
					root["result"][ii]["DirectionStr"]=strarray[1];
					int intSpeed=atoi(strarray[2].c_str());
					sprintf(szTmp,"%.1f",float(intSpeed) / 10.0f);
					root["result"][ii]["Speedms"]=szTmp;
					sprintf(szTmp,"%.1f",(float(intSpeed) * 0.36f));
					root["result"][ii]["Speedkmhr"]=szTmp;
					sprintf(szTmp,"%.1f",((float(intSpeed) * 0.223693629f) / 10.0f));
					root["result"][ii]["Speedmph"]=szTmp;
					int intGust=atoi(strarray[3].c_str());
					sprintf(szTmp,"%.1f",float(intGust) / 10.0f);
					root["result"][ii]["Gustms"]=szTmp;
					sprintf(szTmp,"%.1f",(float(intGust )* 0.36f));
					root["result"][ii]["Gustkmhr"]=szTmp;
					sprintf(szTmp,"%.1f",(float(intGust) * 0.223693629f) / 10.0f);
					root["result"][ii]["Gustmph"]=szTmp;

					if ((dType==pTypeWIND)&&(dSubType==sTypeWIND4))
					{
						root["result"][ii]["Temp"]=atof(strarray[4].c_str());
						root["result"][ii]["Chill"]=atof(strarray[5].c_str());
					}
				}
			}
			else if (dType == pTypeRAIN)
			{
				std::vector<std::string> strarray;
				StringSplit(sValue, ";", strarray);
				if (strarray.size()==2)
				{
					//get lowest value of today, and max rate
					time_t now = time(NULL);
					struct tm* tm1 = localtime(&now);

					struct tm ltime;
					ltime.tm_isdst=tm1->tm_isdst;
					ltime.tm_hour=0;
					ltime.tm_min=0;
					ltime.tm_sec=0;
					ltime.tm_year=tm1->tm_year;
					ltime.tm_mon=tm1->tm_mon;
					ltime.tm_mday=tm1->tm_mday;

					char szDate[40];
					sprintf(szDate,"%04d-%02d-%02d",ltime.tm_year+1900,ltime.tm_mon+1,ltime.tm_mday);

					std::vector<std::vector<std::string> > result2;

					szQuery.clear();
					szQuery.str("");
					szQuery << "SELECT MIN(Total), MAX(Total), MAX(Rate) FROM Rain WHERE (DeviceRowID=" << sd[0] << " AND Date>='" << szDate << "')";
					result2=m_pMain->m_sql.query(szQuery.str());
					if (result2.size()>0)
					{
						std::vector<std::string> sd2=result2[0];
						float total_min=(float)atof(sd2[0].c_str());
						float total_max=(float)atof(sd2[1].c_str());
						int rate=atoi(sd2[2].c_str());
						float total_real=total_max-total_min;
						sprintf(szTmp,"%.1f",total_real);
						root["result"][ii]["Rain"]=szTmp;
						//if ((dSubType==sTypeRAIN1)||(dSubType==sTypeRAIN2))
						root["result"][ii]["RainRate"]=rate;
					}
				}
			}
			else if (dType == pTypeRFXMeter)
			{
				//get lowest value of today
				time_t now = time(NULL);
				struct tm* tm1 = localtime(&now);

				struct tm ltime;
				ltime.tm_isdst=tm1->tm_isdst;
				ltime.tm_hour=0;
				ltime.tm_min=0;
				ltime.tm_sec=0;
				ltime.tm_year=tm1->tm_year;
				ltime.tm_mon=tm1->tm_mon;
				ltime.tm_mday=tm1->tm_mday;

				char szDate[40];
				sprintf(szDate,"%04d-%02d-%02d",ltime.tm_year+1900,ltime.tm_mon+1,ltime.tm_mday);

				std::vector<std::vector<std::string> > result2;

				szQuery.clear();
				szQuery.str("");
				szQuery << "SELECT MIN(Value), MAX(Value) FROM Meter WHERE (DeviceRowID=" << sd[0] << " AND Date>='" << szDate << "')";
				result2=m_pMain->m_sql.query(szQuery.str());
				if (result2.size()>0)
				{
					std::vector<std::string> sd2=result2[0];

					unsigned long long total_min,total_max,total_real;

					std::stringstream s_str1( sd2[0] );
					s_str1 >> total_min;
					std::stringstream s_str2( sd2[1] );
					s_str2 >> total_max;
					total_real=total_max-total_min;
					sprintf(szTmp,"%llu",total_real);

					float musage=0;
					switch (metertype)
					{
					case MTYPE_ENERGY:
						musage=float(total_real)/1000.0f;
						sprintf(szTmp,"%.03f kWh",musage);
						break;
					case MTYPE_GAS:
						musage=float(total_real)/100.0f;
						sprintf(szTmp,"%.02f m3",musage);
						break;
					}
					root["result"][ii]["Counter"]=sValue;
					root["result"][ii]["CounterToday"]=szTmp;
					root["result"][ii]["SwitchTypeVal"]=metertype;
				}
			}
			else if (dType == pTypeCURRENT)
			{
				std::vector<std::string> strarray;
				StringSplit(sValue, ";", strarray);
				if (strarray.size()==3)
				{
					sprintf(szData,"%.1f A, %.1f A, %.1f A",atof(strarray[0].c_str()),atof(strarray[1].c_str()),atof(strarray[2].c_str()));
					root["result"][ii]["Data"]=szData;
				}
			}
			else if (dType == pTypeENERGY)
			{
				std::vector<std::string> strarray;
				StringSplit(sValue, ";", strarray);
				if (strarray.size()==2)
				{
					sprintf(szData,"%ld Watt, %.2f Wh",atol(strarray[0].c_str()),atof(strarray[1].c_str()));
					root["result"][ii]["Data"]=szData;
				}
			}
			else if (dType == pTypeRFXMeter)
			{
				root["result"][ii]["Data"]=sValue;
			}
			else if (dType == pTypeRFXSensor)
			{
				switch (dSubType)
				{
				case sTypeRFXSensorAD:
					sprintf(szData,"%d mV",atoi(sValue.c_str()));
					root["result"][ii]["TypeImg"]="current";
					break;
				case sTypeRFXSensorVolt:
					sprintf(szData,"%d mV",atoi(sValue.c_str()));
					root["result"][ii]["TypeImg"]="current";
					break;
				}
				root["result"][ii]["Data"]=szData;
			}
			else if (dType == pTypeSecurity1)
			{
				sprintf(szData,"%s",Security_Status_Desc(nValue));
				root["result"][ii]["Data"]=szData;
			}


			ii++;
		}
	}
}

char * CWebServer::GetJSonPage()
{
	Json::Value root;
	root["status"]="ERR";

	bool bHaveUser=(m_pWebEm->m_actualuser!="");
	int iUser=-1;
	if (bHaveUser)
		iUser=FindUser(m_pWebEm->m_actualuser.c_str());

	m_retstr="";
	if (!m_pWebEm->HasParams())
	{
		m_retstr=root.toStyledString();

		return (char*)m_retstr.c_str();
	}

	std::string rtype=m_pWebEm->FindValue("type");
	std::string rdate=m_pWebEm->FindValue("date");

	unsigned long long idx=0;
	if (m_pWebEm->FindValue("idx")!="")
	{
		std::stringstream s_str( m_pWebEm->FindValue("idx") );
		s_str >> idx;
	}


	int day=1;
	int month=-1;
	int year=-1;

	if (rdate!="")
	{
		std::string datestr=rdate.c_str();
		if (datestr.size()==10)
		{
			if (
				(datestr[2]=='-')&&
				(datestr[5]=='-')
				)
			{
				day=atoi(datestr.substr(0,2).c_str());
				month=atoi(datestr.substr(3,2).c_str());
				year=atoi(datestr.substr(6,4).c_str());
			}
		}
		else if (datestr.size()==7)
		{
			if (
				(datestr[2]=='-')
				)
			{
				day=1;
				month=atoi(datestr.substr(0,2).c_str());
				year=atoi(datestr.substr(3,4).c_str());
			}
		}
	}

	std::vector<std::vector<std::string> > result;
	std::vector<std::vector<std::string> > result2;
	std::stringstream szQuery;
	char szData[100];
	char szTmp[300];

	if (rtype=="hardware")
	{
		root["status"]="OK";
		root["title"]="Hardware";

		szQuery.clear();
		szQuery.str("");
		szQuery << "SELECT ID, Name, Type, Address, Port, Username, Password, Mode1, Mode2, Mode3, Mode4, Mode5 FROM Hardware ORDER BY ID ASC";
		result=m_pMain->m_sql.query(szQuery.str());
		if (result.size()>0)
		{
			std::vector<std::vector<std::string> >::const_iterator itt;
			int ii=0;
			for (itt=result.begin(); itt!=result.end(); ++itt)
			{
				std::vector<std::string> sd=*itt;

				root["result"][ii]["idx"]=sd[0];
				root["result"][ii]["Name"]=sd[1];
				root["result"][ii]["Type"]=atoi(sd[2].c_str());
				root["result"][ii]["Address"]=sd[3];
				root["result"][ii]["Port"]=atoi(sd[4].c_str());
				root["result"][ii]["Username"]=sd[5];
				root["result"][ii]["Password"]=sd[6];
				root["result"][ii]["Mode1"]=(unsigned char)atoi(sd[7].c_str());
				root["result"][ii]["Mode2"]=(unsigned char)atoi(sd[8].c_str());
				root["result"][ii]["Mode3"]=(unsigned char)atoi(sd[9].c_str());
				root["result"][ii]["Mode4"]=(unsigned char)atoi(sd[10].c_str());
				root["result"][ii]["Mode5"]=(unsigned char)atoi(sd[11].c_str());

				szQuery.clear();
				szQuery.str("");
				szQuery << "SELECT Port, Username, Password, Rights FROM HardwareSharing WHERE (HardwareID == " << sd[0] << ")";
				result2=m_pMain->m_sql.query(szQuery.str());
				if (result2.size()>0)
				{
					//shared
					sd=result2[0];
					root["result"][ii]["Shared"]="true";
					root["result"][ii]["SharedPort"]=sd[0];
					root["result"][ii]["SharedUsername"]=sd[1];
					root["result"][ii]["SharedPassword"]=sd[2];
					root["result"][ii]["SharedRights"]=sd[3];
				}
				else
				{
					//not shared
					root["result"][ii]["Shared"]="false";
					root["result"][ii]["SharedPort"]="";
					root["result"][ii]["SharedUsername"]="";
					root["result"][ii]["SharedPassword"]="";
					root["result"][ii]["SharedRights"]="";
				}

				ii++;
			}
		}
	} //if (rtype=="hardware")
	else if (rtype=="devices")
	{
		std::string rfilter=m_pWebEm->FindValue("filter");
		std::string order=m_pWebEm->FindValue("order");
		std::string rused=m_pWebEm->FindValue("used");

		root["status"]="OK";
		root["title"]="Devices";

		GetJSonDevices(root, rused, rfilter,order);

	} //if (rtype=="devices")
	else if (rtype=="status-temp")
	{
		root["status"]="OK";
		root["title"]="StatusTemp";

		Json::Value tempjson;

		GetJSonDevices(tempjson, "", "temp","ID");

		Json::Value::const_iterator itt;
		int ii=0;
		size_t rsize=tempjson["result"].size();
		for ( size_t itt = 0; itt<rsize; itt++)
		{
			root["result"][ii]["idx"]=ii;
			root["result"][ii]["Name"]=tempjson["result"][itt]["Name"];
			root["result"][ii]["Temp"]=tempjson["result"][itt]["Temp"];
			root["result"][ii]["LastUpdate"]=tempjson["result"][itt]["LastUpdate"];
			if (!tempjson["result"][itt]["Humidity"].empty())
				root["result"][ii]["Humidity"]=tempjson["result"][itt]["Humidity"];
			else
				root["result"][ii]["Humidity"]=0;
			if (!tempjson["result"][itt]["Chill"].empty())
				root["result"][ii]["Chill"]=tempjson["result"][itt]["Chill"];
			else
				root["result"][ii]["Chill"]=0;

			ii++;
		}
	} //if (rtype=="status-temp")
	else if (rtype=="status-wind")
	{
		root["status"]="OK";
		root["title"]="StatusWind";

		Json::Value tempjson;

		GetJSonDevices(tempjson, "", "wind","ID");

		Json::Value::const_iterator itt;
		int ii=0;
		size_t rsize=tempjson["result"].size();
		for ( size_t itt = 0; itt<rsize; itt++)
		{
			root["result"][ii]["idx"]=ii;
			root["result"][ii]["Name"]=tempjson["result"][itt]["Name"];
			root["result"][ii]["Direction"]=tempjson["result"][itt]["Direction"];
			root["result"][ii]["DirectionStr"]=tempjson["result"][itt]["DirectionStr"];
			root["result"][ii]["Gustms"]=tempjson["result"][itt]["Gustms"];
			root["result"][ii]["Speedms"]=tempjson["result"][itt]["Speedms"];
			root["result"][ii]["LastUpdate"]=tempjson["result"][itt]["LastUpdate"];

			ii++;
		}
	} //if (rtype=="status-wind")
	else if (rtype=="status-rain")
	{
		root["status"]="OK";
		root["title"]="StatusRain";

		Json::Value tempjson;

		GetJSonDevices(tempjson, "", "rain","ID");

		Json::Value::const_iterator itt;
		int ii=0;
		size_t rsize=tempjson["result"].size();
		for ( size_t itt = 0; itt<rsize; itt++)
		{
			root["result"][ii]["idx"]=ii;
			root["result"][ii]["Name"]=tempjson["result"][itt]["Name"];
			root["result"][ii]["Rain"]=tempjson["result"][itt]["Rain"];
			ii++;
		}
	} //if (rtype=="status-rain")
	else if (rtype=="status-uv")
	{
		root["status"]="OK";
		root["title"]="StatusUV";

		Json::Value tempjson;

		GetJSonDevices(tempjson, "", "uv","ID");

		Json::Value::const_iterator itt;
		int ii=0;
		size_t rsize=tempjson["result"].size();
		for ( size_t itt = 0; itt<rsize; itt++)
		{
			root["result"][ii]["idx"]=ii;
			root["result"][ii]["Name"]=tempjson["result"][itt]["Name"];
			root["result"][ii]["UVI"]=tempjson["result"][itt]["UVI"];
			ii++;
		}
	} //if (rtype=="status-uv")
	else if (rtype=="status-baro")
	{
		root["status"]="OK";
		root["title"]="StatusBaro";

		Json::Value tempjson;

		GetJSonDevices(tempjson, "", "baro","ID");

		Json::Value::const_iterator itt;
		int ii=0;
		size_t rsize=tempjson["result"].size();
		for ( size_t itt = 0; itt<rsize; itt++)
		{
			root["result"][ii]["idx"]=ii;
			root["result"][ii]["Name"]=tempjson["result"][itt]["Name"];
			root["result"][ii]["Barometer"]=tempjson["result"][itt]["Barometer"];
			ii++;
		}
	} //if (rtype=="status-baro")
	else if ((rtype=="lightlog")&&(idx!=0))
	{
		//First get Device Type/SubType
		szQuery.clear();
		szQuery.str("");
		szQuery << "SELECT Type, SubType FROM DeviceStatus WHERE (ID == " << idx << ")";
		result=m_pMain->m_sql.query(szQuery.str());
		if (result.size()<1)
			goto exitjson;

		unsigned char dType=atoi(result[0][0].c_str());
		unsigned char dSubType=atoi(result[0][1].c_str());

		if (
			(dType!=pTypeLighting1)&&
			(dType!=pTypeLighting2)&&
			(dType!=pTypeLighting3)&&
			(dType!=pTypeLighting4)&&
			(dType!=pTypeLighting5)&&
			(dType!=pTypeLighting6)&&
			(dType!=pTypeSecurity1)
			)
			goto exitjson; //no light device! we should not be here!

		root["status"]="OK";
		root["title"]="LightLog";

		szQuery.clear();
		szQuery.str("");
		szQuery << "SELECT ROWID, nValue, sValue, Date FROM LightingLog WHERE (DeviceRowID==" << idx << ") ORDER BY Date DESC";
		result=m_pMain->m_sql.query(szQuery.str());
		if (result.size()>0)
		{
			std::vector<std::vector<std::string> >::const_iterator itt;
			int ii=0;
			for (itt=result.begin(); itt!=result.end(); ++itt)
			{
				std::vector<std::string> sd=*itt;

				unsigned char nValue = atoi(sd[1].c_str());
				std::string sValue=sd[2];

				root["result"][ii]["idx"]=sd[0];

				//add light details
				std::string lstatus="";
				int llevel=0;
				bool bHaveDimmer=false;
				bool bHaveGroupCmd=false;

				GetLightStatus(dType,dSubType,nValue,sValue,lstatus,llevel,bHaveDimmer,bHaveGroupCmd);

				if (ii==0)
				{
					root["HaveDimmer"]=bHaveDimmer;
					root["HaveGroupCmd"]=bHaveGroupCmd;
				}

				root["result"][ii]["Date"]=sd[3];
				root["result"][ii]["Status"]=lstatus;
				root["result"][ii]["Level"]=llevel;

				if (llevel!=0)
					sprintf(szData,"%s, Level: %d %%", lstatus.c_str(), llevel);
				else
					sprintf(szData,"%s", lstatus.c_str());
				root["result"][ii]["Data"]=szData;

				ii++;
			}
		}
	} //(rtype=="lightlog")
	else if ((rtype=="timers")&&(idx!=0))
	{
		root["status"]="OK";
		root["title"]="Timers";

		szQuery.clear();
		szQuery.str("");
		szQuery << "SELECT ID, Active, Time, Type, Cmd, Level, Days FROM Timers WHERE (DeviceRowID==" << idx << ") ORDER BY ID";
		result=m_pMain->m_sql.query(szQuery.str());
		if (result.size()>0)
		{
			std::vector<std::vector<std::string> >::const_iterator itt;
			int ii=0;
			for (itt=result.begin(); itt!=result.end(); ++itt)
			{
				std::vector<std::string> sd=*itt;

				root["result"][ii]["idx"]=sd[0];
				root["result"][ii]["Active"]=(atoi(sd[1].c_str())==0)?"false":"true";
				root["result"][ii]["Time"]=sd[2].substr(0,5);
				root["result"][ii]["Type"]=atoi(sd[3].c_str());
				root["result"][ii]["Cmd"]=atoi(sd[4].c_str());
				root["result"][ii]["Level"]=atoi(sd[5].c_str());
				root["result"][ii]["Days"]=atoi(sd[6].c_str());
				ii++;
			}
		}
	} //(rtype=="timers")
	else if ((rtype=="notifications")&&(idx!=0))
	{
		root["status"]="OK";
		root["title"]="Notifications";

		std::vector<_tNotification> notifications=m_pMain->m_sql.GetNotifications(idx);
		if (notifications.size()>0)
		{
			std::vector<_tNotification>::const_iterator itt;
			int ii=0;
			for (itt=notifications.begin(); itt!=notifications.end(); ++itt)
			{
				root["result"][ii]["idx"]=itt->ID;
				root["result"][ii]["Params"]=itt->Params;
				ii++;
			}
		}
	} //(rtype=="notifications")
	else if (rtype=="schedules")
	{
		root["status"]="OK";
		root["title"]="Schedules";

		std::vector<tScheduleItem> schedules=m_pMain->m_scheduler.GetScheduleItems();
		int ii=0;
		std::vector<tScheduleItem>::iterator itt;
		for (itt=schedules.begin(); itt!=schedules.end(); ++itt)
		{
			root["result"][ii]["DevID"]=itt->DevID;
			root["result"][ii]["TimerType"]=Timer_Type_Desc(itt->timerType);
			root["result"][ii]["TimerCmd"]=Timer_Cmd_Desc(itt->timerCmd);
			char *pDate=asctime(localtime(&itt->startTime));
			if (pDate!=NULL)
			{
				pDate[strlen(pDate)-1]=0;
				root["result"][ii]["ScheduleDate"]=pDate;
				ii++;
			}
		}
	} //(rtype=="schedules")
	else if ((rtype=="graph")&&(idx!=0))
	{
		std::string sensor=m_pWebEm->FindValue("sensor");
		if (sensor=="")
			goto exitjson;
		std::string srange=m_pWebEm->FindValue("range");
		if (srange=="")
			goto exitjson;
		std::string dbasetable="";

		if (srange=="day") {
			if (sensor=="temp")
				dbasetable="Temperature";
			else if (sensor=="rain")
				dbasetable="Rain_Calendar";
			else if (sensor=="counter")
				dbasetable="Meter_Calendar";
			else if ( (sensor=="wind") || (sensor=="winddir") )
				dbasetable="Wind";
			else if (sensor=="uv")
				dbasetable="UV";
			else
				goto exitjson;
		}
		else
		{
			if (sensor=="temp")
				dbasetable="Temperature_Calendar";
			else if (sensor=="rain")
				dbasetable="Rain_Calendar";
			else if (sensor=="counter")
				dbasetable="Meter_Calendar";
			else if ( (sensor=="wind") || (sensor=="winddir") )
				dbasetable="Wind_Calendar";
			else if (sensor=="uv")
				dbasetable="UV_Calendar";
			else
				goto exitjson;
		}

		time_t now = time(NULL);
		struct tm* tm1 = localtime(&now);

		szQuery.clear();
		szQuery.str("");
		szQuery << "SELECT Type, SubType, SwitchType FROM DeviceStatus WHERE (ID == " << idx << ")";
		result=m_pMain->m_sql.query(szQuery.str());
		if (result.size()<1)
			goto exitjson;

		unsigned char dType=atoi(result[0][0].c_str());
		unsigned char dSubType=atoi(result[0][1].c_str());
		_eMeterType metertype = (_eMeterType)atoi(result[0][2].c_str());

		if (srange=="day")
		{
			if (sensor=="temp") {
				root["status"]="OK";
				root["title"]="Graph " + sensor + " " + srange;

				szQuery.clear();
				szQuery.str("");
				szQuery << "SELECT Temperature, Chill, Humidity, Barometer, Date FROM " << dbasetable << " WHERE (DeviceRowID==" << idx << ") ORDER BY Date ASC";
				result=m_pMain->m_sql.query(szQuery.str());
				if (result.size()>0)
				{
					std::vector<std::vector<std::string> >::const_iterator itt;
					int ii=0;
					for (itt=result.begin(); itt!=result.end(); ++itt)
					{
						std::vector<std::string> sd=*itt;

						root["result"][ii]["d"]=sd[4].substr(0,16);
						if (
							(dType==pTypeTEMP)||
							(dType==pTypeTEMP_HUM)||
							(dType==pTypeTEMP_HUM_BARO)||
							((dType==pTypeWIND)&&(dSubType==sTypeWIND4))||
							(dType==pTypeUV)||
							(dType==pTypeThermostat1)||
							((dType==pTypeRFXSensor)&&(dSubType==sTypeRFXSensorTemp))
							)
						{
							root["result"][ii]["te"]=sd[0];
						}
						if ((dType==pTypeWIND)&&(dSubType==sTypeWIND4))
						{
							root["result"][ii]["ch"]=sd[1];
						}
						if ((dType==pTypeHUM)||(dType==pTypeTEMP_HUM)||(dType==pTypeTEMP_HUM_BARO))
						{
							root["result"][ii]["hu"]=sd[2];
						}
						if (dType==pTypeTEMP_HUM_BARO)
						{
							root["result"][ii]["ba"]=sd[3];
						}

						ii++;
					}
				}
			}
			else if (sensor=="uv") {
				root["status"]="OK";
				root["title"]="Graph " + sensor + " " + srange;

				szQuery.clear();
				szQuery.str("");
				szQuery << "SELECT Level, Date FROM " << dbasetable << " WHERE (DeviceRowID==" << idx << ") ORDER BY Date ASC";
				result=m_pMain->m_sql.query(szQuery.str());
				if (result.size()>0)
				{
					std::vector<std::vector<std::string> >::const_iterator itt;
					int ii=0;
					for (itt=result.begin(); itt!=result.end(); ++itt)
					{
						std::vector<std::string> sd=*itt;

						root["result"][ii]["d"]=sd[1].substr(0,16);
						root["result"][ii]["uvi"]=sd[0];
						ii++;
					}
				}
			}
			else if (sensor=="rain") {
				root["status"]="OK";
				root["title"]="Graph " + sensor + " " + srange;

				char szDateStart[40];
				char szDateEnd[40];

				struct tm ltime;
				ltime.tm_isdst=tm1->tm_isdst;
				ltime.tm_hour=0;
				ltime.tm_min=0;
				ltime.tm_sec=0;
				ltime.tm_year=tm1->tm_year;
				ltime.tm_mon=tm1->tm_mon;
				ltime.tm_mday=tm1->tm_mday;

				sprintf(szDateEnd,"%04d-%02d-%02d",ltime.tm_year+1900,ltime.tm_mon+1,ltime.tm_mday);

				//Subtract one week

				ltime.tm_mday -= 7;
				time_t later = mktime(&ltime);
				struct tm* tm2 = localtime(&later);
				sprintf(szDateStart,"%04d-%02d-%02d",tm2->tm_year+1900,tm2->tm_mon+1,tm2->tm_mday);

				szQuery.clear();
				szQuery.str("");
				szQuery << "SELECT Total, Rate, Date FROM " << dbasetable << " WHERE (DeviceRowID==" << idx << " AND Date>='" << szDateStart << "' AND Date<='" << szDateEnd << "') ORDER BY Date ASC";
				result=m_pMain->m_sql.query(szQuery.str());
				int ii=0;
				if (result.size()>0)
				{
					std::vector<std::vector<std::string> >::const_iterator itt;
					for (itt=result.begin(); itt!=result.end(); ++itt)
					{
						std::vector<std::string> sd=*itt;

						root["result"][ii]["d"]=sd[2].substr(0,16);
						root["result"][ii]["mm"]=sd[0];
						ii++;
					}
				}
				//add today (have to calculate it)
				szQuery.clear();
				szQuery.str("");
				szQuery << "SELECT MIN(Total), MAX(Total), MAX(Rate) FROM Rain WHERE (DeviceRowID=" << idx << " AND Date>='" << szDateEnd << "')";
				result=m_pMain->m_sql.query(szQuery.str());
				if (result.size()>0)
				{
					std::vector<std::string> sd=result[0];

					float total_min=(float)atof(sd[0].c_str());
					float total_max=(float)atof(sd[1].c_str());
					int rate=atoi(sd[2].c_str());

					float total_real=total_max-total_min;
					sprintf(szTmp,"%.1f",total_real);
					root["result"][ii]["d"]=szDateEnd;
					root["result"][ii]["mm"]=szTmp;
					ii++;
				}
			}
			else if (sensor=="counter") {
				root["status"]="OK";
				root["title"]="Graph " + sensor + " " + srange;

				char szDateStart[40];
				char szDateEnd[40];

				struct tm ltime;
				ltime.tm_isdst=tm1->tm_isdst;
				ltime.tm_hour=0;
				ltime.tm_min=0;
				ltime.tm_sec=0;
				ltime.tm_year=tm1->tm_year;
				ltime.tm_mon=tm1->tm_mon;
				ltime.tm_mday=tm1->tm_mday;

				sprintf(szDateEnd,"%04d-%02d-%02d",ltime.tm_year+1900,ltime.tm_mon+1,ltime.tm_mday);

				//Subtract one week

				ltime.tm_mday -= 7;
				time_t later = mktime(&ltime);
				struct tm* tm2 = localtime(&later);
				sprintf(szDateStart,"%04d-%02d-%02d",tm2->tm_year+1900,tm2->tm_mon+1,tm2->tm_mday);

				szQuery.clear();
				szQuery.str("");
				szQuery << "SELECT Value, Date FROM " << dbasetable << " WHERE (DeviceRowID==" << idx << " AND Date>='" << szDateStart << "' AND Date<='" << szDateEnd << "') ORDER BY Date ASC";
				result=m_pMain->m_sql.query(szQuery.str());
				int ii=0;
				if (result.size()>0)
				{
					std::vector<std::vector<std::string> >::const_iterator itt;
					for (itt=result.begin(); itt!=result.end(); ++itt)
					{
						std::vector<std::string> sd=*itt;

						root["result"][ii]["d"]=sd[1].substr(0,16);
						std::string szValue=sd[0];
						switch (metertype)
						{
						case MTYPE_ENERGY:
							sprintf(szTmp,"%.3f",atof(szValue.c_str())/1000.0f);
							szValue=szTmp;
							break;
						case MTYPE_GAS:
							sprintf(szTmp,"%.2f",atof(szValue.c_str())/100.0f);
							szValue=szTmp;
							break;
						}
						root["result"][ii]["v"]=szValue;
						ii++;
					}
				}
				//add today (have to calculate it)
				szQuery.clear();
				szQuery.str("");
				szQuery << "SELECT MIN(Value), MAX(Value) FROM Meter WHERE (DeviceRowID=" << idx << " AND Date>='" << szDateEnd << "')";
				result=m_pMain->m_sql.query(szQuery.str());
				if (result.size()>0)
				{
					std::vector<std::string> sd=result[0];

					unsigned long long total_min,total_max,total_real;

					std::stringstream s_str1( sd[0] );
					s_str1 >> total_min;
					std::stringstream s_str2( sd[1] );
					s_str2 >> total_max;
					total_real=total_max-total_min;
					sprintf(szTmp,"%llu",total_real);
					std::string szValue=szTmp;
					switch (metertype)
					{
					case MTYPE_ENERGY:
						sprintf(szTmp,"%.3f",atof(szValue.c_str())/1000.0f);
						szValue=szTmp;
						break;
					case MTYPE_GAS:
						sprintf(szTmp,"%.2f",atof(szValue.c_str())/100.0f);
						szValue=szTmp;
						break;
					}

					root["result"][ii]["d"]=szDateEnd;
					root["result"][ii]["v"]=szValue;
					ii++;
				}
			}
			else if (sensor=="wind") {
				root["status"]="OK";
				root["title"]="Graph " + sensor + " " + srange;

				szQuery.clear();
				szQuery.str("");
				szQuery << "SELECT Direction, Speed, Gust, Date FROM " << dbasetable << " WHERE (DeviceRowID==" << idx << ") ORDER BY Date ASC";
				result=m_pMain->m_sql.query(szQuery.str());
				if (result.size()>0)
				{
					std::vector<std::vector<std::string> >::const_iterator itt;
					int ii=0;
					for (itt=result.begin(); itt!=result.end(); ++itt)
					{
						std::vector<std::string> sd=*itt;

						root["result"][ii]["d"]=sd[3].substr(0,16);
						root["result"][ii]["di"]=sd[0];

						int intSpeed=atoi(sd[1].c_str());
						sprintf(szTmp,"%.1f",float(intSpeed) / 10.0f);
						root["result"][ii]["sp"]=szTmp;
						int intGust=atoi(sd[2].c_str());
						sprintf(szTmp,"%.1f",float(intGust) / 10.0f);
						root["result"][ii]["gu"]=szTmp;
						ii++;
					}
				}
			}
			else if (sensor=="winddir") {
				root["status"]="OK";
				root["title"]="Graph " + sensor + " " + srange;

				szQuery.clear();
				szQuery.str("");
				szQuery << "SELECT Direction FROM " << dbasetable << " WHERE (DeviceRowID==" << idx << ") ORDER BY Date ASC";
				result=m_pMain->m_sql.query(szQuery.str());
				if (result.size()>0)
				{
					std::vector<std::vector<std::string> >::const_iterator itt;
					std::map<int,int> _directions;
					int totalvalues=0;
					//init dir list
					int idir;
					for (idir=0; idir<360+1; idir++)
						_directions[idir]=0;

					for (itt=result.begin(); itt!=result.end(); ++itt)
					{
						std::vector<std::string> sd=*itt;
						int direction=atoi(sd[0].c_str());
						_directions[direction]++;
						totalvalues++;
					}
					int ii=0;
					for (idir=0; idir<360+1; idir++)
					{
						if (_directions[idir]!=0)
						{
							root["result"][ii]["dig"]=idir;
							float percentage=(float(100.0/float(totalvalues))*float(_directions[idir]));
							sprintf(szTmp,"%.2f",percentage);
							root["result"][ii]["div"]=szTmp;
							ii++;
						}
					}
				}
			}

		}//day
		else if ( (srange=="month") || (srange=="year" ) )
		{
			char szDateStart[40];
			char szDateEnd[40];

			struct tm ltime;
			ltime.tm_isdst=tm1->tm_isdst;
			ltime.tm_hour=0;
			ltime.tm_min=0;
			ltime.tm_sec=0;
			ltime.tm_year=tm1->tm_year;
			ltime.tm_mon=tm1->tm_mon;
			ltime.tm_mday=tm1->tm_mday;

			sprintf(szDateEnd,"%04d-%02d-%02d",ltime.tm_year+1900,ltime.tm_mon+1,ltime.tm_mday);

			if (srange=="month")
			{
				//Subtract one month
				ltime.tm_mon -= 1;
			}
			else
			{
				//Subtract one year
				ltime.tm_year -= 1;
			}

			time_t later = mktime(&ltime);
			struct tm* tm2 = localtime(&later);

			sprintf(szDateStart,"%04d-%02d-%02d",tm2->tm_year+1900,tm2->tm_mon+1,tm2->tm_mday);
			if (sensor=="temp") {
				root["status"]="OK";
				root["title"]="Graph " + sensor + " " + srange;

				szQuery.clear();
				szQuery.str("");
				szQuery << "SELECT Temp_Min, Temp_Max, Chill_Min, Chill_Max, Humidity, Barometer, Date FROM " << dbasetable << " WHERE (DeviceRowID==" << idx << " AND Date>='" << szDateStart << "' AND Date<='" << szDateEnd << "') ORDER BY Date ASC";
				result=m_pMain->m_sql.query(szQuery.str());
				int ii=0;
				if (result.size()>0)
				{
					std::vector<std::vector<std::string> >::const_iterator itt;
					for (itt=result.begin(); itt!=result.end(); ++itt)
					{
						std::vector<std::string> sd=*itt;

						root["result"][ii]["d"]=sd[6].substr(0,16);
						if (
							(dType==pTypeTEMP)||(dType==pTypeTEMP_HUM)||(dType==pTypeTEMP_HUM_BARO)||(dType==pTypeWIND)||(dType==pTypeUV)||(dType==pTypeThermostat1)||
							((dType==pTypeRFXSensor)&&(dSubType==sTypeRFXSensorTemp))
							)
						{
							if (!((dType==pTypeWIND)&&(dSubType!=sTypeWIND4)))
							{
								root["result"][ii]["te"]=sd[1];
								root["result"][ii]["tm"]=sd[0];
							}
						}
						if ((dType==pTypeWIND)&&(dSubType==sTypeWIND4))
						{
							root["result"][ii]["ch"]=sd[3];
							root["result"][ii]["cm"]=sd[2];
						}
						if ((dType==pTypeHUM)||(dType==pTypeTEMP_HUM)||(dType==pTypeTEMP_HUM_BARO))
						{
							root["result"][ii]["hu"]=sd[4];
						}
						if (dType==pTypeTEMP_HUM_BARO)
						{
							root["result"][ii]["ba"]=sd[5];
						}

						ii++;
					}
				}
				//add today (have to calculate it)
				szQuery.clear();
				szQuery.str("");
				szQuery << "SELECT MIN(Temperature), MAX(Temperature), MIN(Chill), MAX(Chill), MAX(Humidity), MAX(Barometer) FROM Temperature WHERE (DeviceRowID=" << idx << " AND Date>='" << szDateEnd << "')";
				result=m_pMain->m_sql.query(szQuery.str());
				if (result.size()>0)
				{
					std::vector<std::string> sd=result[0];

					root["result"][ii]["d"]=szDateEnd;
					if ((dType==pTypeTEMP)||(dType==pTypeTEMP_HUM)||(dType==pTypeTEMP_HUM_BARO)||(dType==pTypeWIND)||(dType==pTypeUV)||(dType==pTypeThermostat1))
					{
						if (!((dType==pTypeWIND)&&(dSubType!=sTypeWIND4)))
						{
							root["result"][ii]["te"]=sd[1];
							root["result"][ii]["tm"]=sd[0];
						}
					}
					if ((dType==pTypeWIND)&&(dSubType==sTypeWIND4))
					{
						root["result"][ii]["ch"]=sd[3];
						root["result"][ii]["cm"]=sd[2];
					}
					if ((dType==pTypeHUM)||(dType==pTypeTEMP_HUM)||(dType==pTypeTEMP_HUM_BARO))
					{
						root["result"][ii]["hu"]=sd[4];
					}
					if (dType==pTypeTEMP_HUM_BARO)
					{
						root["result"][ii]["ba"]=sd[5];
					}
					ii++;
				}

			}
			else if (sensor=="uv") {
				root["status"]="OK";
				root["title"]="Graph " + sensor + " " + srange;

				szQuery.clear();
				szQuery.str("");
				szQuery << "SELECT Level, Date FROM " << dbasetable << " WHERE (DeviceRowID==" << idx << " AND Date>='" << szDateStart << "' AND Date<='" << szDateEnd << "') ORDER BY Date ASC";
				result=m_pMain->m_sql.query(szQuery.str());
				int ii=0;
				if (result.size()>0)
				{
					std::vector<std::vector<std::string> >::const_iterator itt;
					for (itt=result.begin(); itt!=result.end(); ++itt)
					{
						std::vector<std::string> sd=*itt;

						root["result"][ii]["d"]=sd[1].substr(0,16);
						root["result"][ii]["uvi"]=sd[0];
						ii++;
					}
				}
				//add today (have to calculate it)
				szQuery.clear();
				szQuery.str("");
				szQuery << "SELECT MAX(Level) FROM UV WHERE (DeviceRowID=" << idx << " AND Date>='" << szDateEnd << "')";
				result=m_pMain->m_sql.query(szQuery.str());
				if (result.size()>0)
				{
					std::vector<std::string> sd=result[0];

					root["result"][ii]["d"]=szDateEnd;
					root["result"][ii]["uvi"]=sd[0];
					ii++;
				}
			}
			else if (sensor=="rain") {
				root["status"]="OK";
				root["title"]="Graph " + sensor + " " + srange;

				szQuery.clear();
				szQuery.str("");
				szQuery << "SELECT Total, Rate, Date FROM " << dbasetable << " WHERE (DeviceRowID==" << idx << " AND Date>='" << szDateStart << "' AND Date<='" << szDateEnd << "') ORDER BY Date ASC";
				result=m_pMain->m_sql.query(szQuery.str());
				int ii=0;
				if (result.size()>0)
				{
					std::vector<std::vector<std::string> >::const_iterator itt;
					for (itt=result.begin(); itt!=result.end(); ++itt)
					{
						std::vector<std::string> sd=*itt;

						root["result"][ii]["d"]=sd[2].substr(0,16);
						root["result"][ii]["mm"]=sd[0];
						ii++;
					}
				}
				//add today (have to calculate it)
				szQuery.clear();
				szQuery.str("");
				szQuery << "SELECT MIN(Total), MAX(Total), MAX(Rate) FROM Rain WHERE (DeviceRowID=" << idx << " AND Date>='" << szDateEnd << "')";
				result=m_pMain->m_sql.query(szQuery.str());
				if (result.size()>0)
				{
					std::vector<std::string> sd=result[0];

					float total_min=(float)atof(sd[0].c_str());
					float total_max=(float)atof(sd[1].c_str());
					int rate=atoi(sd[2].c_str());

					float total_real=total_max-total_min;
					sprintf(szTmp,"%.1f",total_real);
					root["result"][ii]["d"]=szDateEnd;
					root["result"][ii]["mm"]=szTmp;
					ii++;
				}
			}
			else if (sensor=="counter") {
				root["status"]="OK";
				root["title"]="Graph " + sensor + " " + srange;

				szQuery.clear();
				szQuery.str("");
				szQuery << "SELECT Value, Date FROM " << dbasetable << " WHERE (DeviceRowID==" << idx << " AND Date>='" << szDateStart << "' AND Date<='" << szDateEnd << "') ORDER BY Date ASC";
				result=m_pMain->m_sql.query(szQuery.str());
				int ii=0;
				if (result.size()>0)
				{
					std::vector<std::vector<std::string> >::const_iterator itt;
					for (itt=result.begin(); itt!=result.end(); ++itt)
					{
						std::vector<std::string> sd=*itt;

						std::string szValue=sd[0];
						switch (metertype)
						{
						case MTYPE_ENERGY:
							sprintf(szTmp,"%.3f",atof(szValue.c_str())/1000.0f);
							szValue=szTmp;
							break;
						case MTYPE_GAS:
							sprintf(szTmp,"%.2f",atof(szValue.c_str())/100.0f);
							szValue=szTmp;
							break;
						}
						root["result"][ii]["d"]=sd[1].substr(0,16);
						root["result"][ii]["v"]=szValue;
						ii++;
					}
				}
				//add today (have to calculate it)
				szQuery.clear();
				szQuery.str("");
				szQuery << "SELECT MIN(Value), MAX(Value), FROM Meter WHERE (DeviceRowID=" << idx << " AND Date>='" << szDateEnd << "')";
				result=m_pMain->m_sql.query(szQuery.str());
				if (result.size()>0)
				{
					std::vector<std::string> sd=result[0];
					unsigned long long total_min,total_max,total_real;

					std::stringstream s_str1( sd[0] );
					s_str1 >> total_min;
					std::stringstream s_str2( sd[1] );
					s_str2 >> total_max;
					total_real=total_max-total_min;
					sprintf(szTmp,"%llu",total_real);
					
					std::string szValue=szTmp;
					switch (metertype)
					{
					case MTYPE_ENERGY:
						sprintf(szTmp,"%.3f",atof(szValue.c_str())/1000.0f);
						szValue=szTmp;
						break;
					case MTYPE_GAS:
						sprintf(szTmp,"%.2f",atof(szValue.c_str())/100.0f);
						szValue=szTmp;
						break;
					}

					root["result"][ii]["d"]=szDateEnd;
					root["result"][ii]["v"]=szValue;
					ii++;
				}
			}
			else if (sensor=="wind") {
				root["status"]="OK";
				root["title"]="Graph " + sensor + " " + srange;

				int ii=0;

				szQuery.clear();
				szQuery.str("");
				szQuery << "SELECT Direction, Speed_Min, Speed_Max, Gust_Min, Gust_Max, Date FROM " << dbasetable << " WHERE (DeviceRowID==" << idx << " AND Date>='" << szDateStart << "' AND Date<='" << szDateEnd << "') ORDER BY Date ASC";
				result=m_pMain->m_sql.query(szQuery.str());
				if (result.size()>0)
				{
					std::vector<std::vector<std::string> >::const_iterator itt;
					for (itt=result.begin(); itt!=result.end(); ++itt)
					{
						std::vector<std::string> sd=*itt;

						root["result"][ii]["d"]=sd[5].substr(0,16);
						root["result"][ii]["di"]=sd[0];

						int intSpeed=atoi(sd[2].c_str());
						sprintf(szTmp,"%.1f",float(intSpeed) / 10.0f);
						root["result"][ii]["sp"]=szTmp;
						int intGust=atoi(sd[4].c_str());
						sprintf(szTmp,"%.1f",float(intGust) / 10.0f);
						root["result"][ii]["gu"]=szTmp;
						ii++;
					}
				}
				//add today (have to calculate it)
				szQuery.clear();
				szQuery.str("");
				szQuery << "SELECT AVG(Direction), MIN(Speed), MAX(Speed), MIN(Gust), MAX(Gust) FROM Wind WHERE (DeviceRowID==" << idx << " AND Date>='" << szDateEnd << "') ORDER BY Date ASC";
				result=m_pMain->m_sql.query(szQuery.str());
				if (result.size()>0)
				{
					std::vector<std::string> sd=result[0];

					root["result"][ii]["d"]=szDateEnd;
					root["result"][ii]["di"]=sd[0];

					int intSpeed=atoi(sd[2].c_str());
					sprintf(szTmp,"%.1f",float(intSpeed) / 10.0f);
					root["result"][ii]["sp"]=szTmp;
					int intGust=atoi(sd[4].c_str());
					sprintf(szTmp,"%.1f",float(intGust) / 10.0f);
					root["result"][ii]["gu"]=szTmp;
					ii++;
				}
			}
		}//month or year
	}
	else if (rtype=="command")
	{
		std::string cparam=m_pWebEm->FindValue("param");
		if (cparam=="")
			goto exitjson;
		if (cparam=="testswitch")
		{
			std::string hwdid=m_pWebEm->FindValue("hwdid");
			std::string sswitchtype=m_pWebEm->FindValue("switchtype");
			std::string slighttype=m_pWebEm->FindValue("lighttype");
			if (
				(hwdid=="")||
				(sswitchtype=="")||
				(slighttype=="")
				)
				goto exitjson;
			_eSwitchType switchtype=(_eSwitchType)atoi(sswitchtype.c_str());
			int lighttype=atoi(slighttype.c_str());
			int dtype;
			int subtype=0;
			std::string sunitcode;
			std::string devid;

			if (lighttype<10)
			{
				dtype=pTypeLighting1;
				subtype=lighttype;
				std::string shousecode=m_pWebEm->FindValue("housecode");
				sunitcode=m_pWebEm->FindValue("unitcode");
				if (
					(shousecode=="")||
					(sunitcode=="")
					)
					goto exitjson;
				devid=shousecode;
			}
			else
			{
				dtype=pTypeLighting2;
				subtype=lighttype-10;
				std::string id=m_pWebEm->FindValue("id");
				sunitcode=m_pWebEm->FindValue("unitcode");
				if (
					(id=="")||
					(sunitcode=="")
					)
					goto exitjson;
				devid=id;
			}
			root["status"]="OK";
			root["title"]="TestSwitch";
			std::vector<std::string> sd;

			sd.push_back(hwdid);
			sd.push_back(devid);
			sd.push_back(sunitcode);
			sprintf(szTmp,"%d",dtype);
			sd.push_back(szTmp);
			sprintf(szTmp,"%d",subtype);
			sd.push_back(szTmp);
			sprintf(szTmp,"%d",switchtype);
			sd.push_back(szTmp);

			std::string switchcmd="On";
			m_pMain->SwitchLightInt(sd,switchcmd,0,true);
		}
		else if (cparam=="addswitch")
		{
			std::string hwdid=m_pWebEm->FindValue("hwdid");
			std::string name=m_pWebEm->FindValue("name");
			std::string sswitchtype=m_pWebEm->FindValue("switchtype");
			std::string slighttype=m_pWebEm->FindValue("lighttype");
			if (
				(hwdid=="")||
				(sswitchtype=="")||
				(slighttype=="")||
				(name=="")
				)
				goto exitjson;
			_eSwitchType switchtype=(_eSwitchType)atoi(sswitchtype.c_str());
			int lighttype=atoi(slighttype.c_str());
			int dtype;
			int subtype=0;
			std::string sunitcode;
			std::string devid;

			if (lighttype<10)
			{
				dtype=pTypeLighting1;
				subtype=lighttype;
				std::string shousecode=m_pWebEm->FindValue("housecode");
				sunitcode=m_pWebEm->FindValue("unitcode");
				if (
					(shousecode=="")||
					(sunitcode=="")
					)
					goto exitjson;
				devid=shousecode;
			}
			else
			{
				dtype=pTypeLighting2;
				subtype=lighttype-10;
				std::string id=m_pWebEm->FindValue("id");
				sunitcode=m_pWebEm->FindValue("unitcode");
				if (
					(id=="")||
					(sunitcode=="")
					)
					goto exitjson;
				devid=id;
			}

			//check if switch is unique
			std::vector<std::vector<std::string> > result;
			std::stringstream szQuery;
			szQuery << "SELECT Name FROM DeviceStatus WHERE (HardwareID==" << hwdid << " AND DeviceID=='" << devid << "' AND Unit==" << sunitcode << " AND Type==" << dtype << " AND SubType==" << subtype << ")";
			result=m_pMain->m_sql.query(szQuery.str());
			if (result.size()>0)
			{
				root["message"]="Switch already exists!";
				goto exitjson;
			}

			m_pMain->m_sql.UpdateValue(atoi(hwdid.c_str()), devid.c_str(),atoi(sunitcode.c_str()),dtype,subtype,0,-1,0);
			//set name and switchtype
			szQuery.clear();
			szQuery.str("");
			szQuery << "SELECT ID FROM DeviceStatus WHERE (HardwareID==" << hwdid << " AND DeviceID=='" << devid << "' AND Unit==" << sunitcode << " AND Type==" << dtype << " AND SubType==" << subtype << ")";
			result=m_pMain->m_sql.query(szQuery.str());
			if (result.size()<1)
			{
				root["message"]="Error finding switch in Database!?!?";
				goto exitjson;
			}
			std::string ID=result[0][0];

			szQuery.clear();
			szQuery.str("");
			szQuery << "UPDATE DeviceStatus SET Used=1, Name='" << name << "', SwitchType=" << switchtype << " WHERE (ID == " << ID << ")";
			result=m_pMain->m_sql.query(szQuery.str());

			root["status"]="OK";
			root["title"]="AddSwitch";
		}
		else if (cparam=="getnotificationtypes")
		{
			std::string idx=m_pWebEm->FindValue("idx");
			if (idx=="")
				goto exitjson;
			//First get Device Type/SubType
			szQuery.clear();
			szQuery.str("");
			szQuery << "SELECT Type, SubType FROM DeviceStatus WHERE (ID == " << idx << ")";
			result=m_pMain->m_sql.query(szQuery.str());
			if (result.size()<1)
				goto exitjson;

			root["status"]="OK";
			root["title"]="GetNotificationTypes";
			unsigned char dType=atoi(result[0][0].c_str());
			unsigned char dSubType=atoi(result[0][1].c_str());

			int ii=0;
			if (
				(dType==pTypeTEMP)||
				(dType==pTypeTEMP_HUM)||
				(dType==pTypeTEMP_HUM_BARO)||
				(dType==pTypeWIND)||
				(dType==pTypeUV)||
				(dType==pTypeThermostat1)||
				((dType==pTypeRFXSensor)&&(dSubType==sTypeRFXSensorTemp))
				)
			{
				if (!((dType==pTypeWIND)&&(dSubType!=sTypeWIND4)))
				{
					root["result"][ii]["val"]=NTYPE_TEMPERATURE;
					root["result"][ii]["text"]=Notification_Type_Desc(NTYPE_TEMPERATURE,0);
					root["result"][ii]["ptag"]=Notification_Type_Desc(NTYPE_TEMPERATURE,1);
					ii++;
				}
			}
			if (
				(dType==pTypeHUM)||
				(dType==pTypeTEMP_HUM)||
				(dType==pTypeTEMP_HUM_BARO)
				)
			{
				root["result"][ii]["val"]=NTYPE_HUMIDITY;
				root["result"][ii]["text"]=Notification_Type_Desc(NTYPE_HUMIDITY,0);
				root["result"][ii]["ptag"]=Notification_Type_Desc(NTYPE_HUMIDITY,1);
				ii++;
			}
			if (dType==pTypeRAIN)
			{
				root["result"][ii]["val"]=NTYPE_RAIN;
				root["result"][ii]["text"]=Notification_Type_Desc(NTYPE_RAIN,0);
				root["result"][ii]["ptag"]=Notification_Type_Desc(NTYPE_RAIN,1);
				ii++;
			}
			if (dType==pTypeWIND)
			{
				root["result"][ii]["val"]=NTYPE_WIND;
				root["result"][ii]["text"]=Notification_Type_Desc(NTYPE_WIND,0);
				root["result"][ii]["ptag"]=Notification_Type_Desc(NTYPE_WIND,1);
				ii++;
			}
			if (dType==pTypeUV)
			{
				root["result"][ii]["val"]=NTYPE_UV;
				root["result"][ii]["text"]=Notification_Type_Desc(NTYPE_UV,0);
				root["result"][ii]["ptag"]=Notification_Type_Desc(NTYPE_UV,1);
				ii++;
			}
			if (
				(dType==pTypeTEMP_HUM_BARO)||
				(dType==pTypeBARO)
				)
			{
				root["result"][ii]["val"]=NTYPE_BARO;
				root["result"][ii]["text"]=Notification_Type_Desc(NTYPE_BARO,0);
				root["result"][ii]["ptag"]=Notification_Type_Desc(NTYPE_BARO,1);
				ii++;
			}
			if ((dType==pTypeRFXMeter)&&(dSubType==sTypeRFXMeterCount))
			{
				root["result"][ii]["val"]=NTYPE_BARO;
				root["result"][ii]["text"]=Notification_Type_Desc(NTYPE_USAGE,0);
				root["result"][ii]["ptag"]=Notification_Type_Desc(NTYPE_USAGE,1);
				ii++;
			}
		}
		else if (cparam=="addnotification")
		{
			std::string idx=m_pWebEm->FindValue("idx");
			std::string ntype=m_pWebEm->FindValue("ntype");
			if (idx=="")
				goto exitjson;
		
			if (ntype=="light")
			{
				root["status"]="OK";
				root["title"]="AddNotification";
				//Lights can only have one notification, so delete old one first
				m_pMain->m_sql.RemoveDeviceNotifications(idx);
				m_pMain->m_sql.AddNotification(idx,"");
			}
			else
			{
				std::string stype=m_pWebEm->FindValue("ttype");
				std::string swhen=m_pWebEm->FindValue("twhen");
				std::string svalue=m_pWebEm->FindValue("tvalue");
				if ((stype=="")||(swhen=="")||(swhen==""))
					goto exitjson;

				root["status"]="OK";
				root["title"]="AddNotification";

				_eNotificationTypes ntype=(_eNotificationTypes)atoi(stype.c_str());
				std::string ttype=Notification_Type_Desc(ntype,1);
				unsigned char twhen=(swhen=="0")?'>':'<';
				sprintf(szTmp,"%s;%c;%s",ttype.c_str(),twhen,svalue.c_str());
				m_pMain->m_sql.AddNotification(idx,szTmp);
			}
		}
		else if (cparam=="updatenotification")
		{
			std::string idx=m_pWebEm->FindValue("idx");
			if (idx=="")
				goto exitjson;

			std::string stype=m_pWebEm->FindValue("ttype");
			std::string swhen=m_pWebEm->FindValue("twhen");
			std::string svalue=m_pWebEm->FindValue("tvalue");
			if ((stype=="")||(swhen=="")||(swhen==""))
				goto exitjson;
			root["status"]="OK";
			root["title"]="UpdateNotification";
			_eNotificationTypes ntype=(_eNotificationTypes)atoi(stype.c_str());
			std::string ttype=Notification_Type_Desc(ntype,1);
			unsigned char twhen=(swhen=="0")?'>':'<';
			sprintf(szTmp,"%s;%c;%s",ttype.c_str(),twhen,svalue.c_str());
			m_pMain->m_sql.UpdateNotification(idx,szTmp);
		}
		else if (cparam=="deletenotification")
		{
			std::string idx=m_pWebEm->FindValue("idx");
			std::string ntype=m_pWebEm->FindValue("ntype");
			if (idx=="")
				goto exitjson;

			root["status"]="OK";
			root["title"]="DeleteNotification";

			if (ntype=="light")
			{
				m_pMain->m_sql.RemoveDeviceNotifications(idx);
			}
			else
			{
				m_pMain->m_sql.RemoveNotification(idx);
			}
		}
		else if (cparam=="switchdeviceorder")
		{
			std::string idx1=m_pWebEm->FindValue("idx1");
			std::string idx2=m_pWebEm->FindValue("idx2");
			if ((idx1=="")||(idx2==""))
				goto exitjson;

			std::string Order1,Order2;
			//get device order 1
			szQuery.clear();
			szQuery.str("");
			szQuery << "SELECT [Order] FROM DeviceStatus WHERE (ID == " << idx1 << ")";
			result=m_pMain->m_sql.query(szQuery.str());
			if (result.size()<1)
				goto exitjson;
			Order1=result[0][0];

			//get device order 2
			szQuery.clear();
			szQuery.str("");
			szQuery << "SELECT [Order] FROM DeviceStatus WHERE (ID == " << idx2 << ")";
			result=m_pMain->m_sql.query(szQuery.str());
			if (result.size()<1)
				goto exitjson;
			Order2=result[0][0];

			root["status"]="OK";
			root["title"]="SwitchDeviceOrder";

			szQuery.clear();
			szQuery.str("");
			szQuery << "UPDATE DeviceStatus SET [Order] = " << Order2 << " WHERE (ID == " << idx1 << ")";
			m_pMain->m_sql.query(szQuery.str());

			szQuery.clear();
			szQuery.str("");
			szQuery << "UPDATE DeviceStatus SET [Order] = " << Order1 << " WHERE (ID == " << idx2 << ")";
			m_pMain->m_sql.query(szQuery.str());
		}
		else if (cparam=="clearnotifications")
		{
			std::string idx=m_pWebEm->FindValue("idx");
			if (idx=="")
				goto exitjson;

			root["status"]="OK";
			root["title"]="ClearNotification";

			m_pMain->m_sql.RemoveDeviceNotifications(idx);
		}
		else if (cparam=="addhardware")
		{
			std::string name=m_pWebEm->FindValue("name");
			std::string shtype=m_pWebEm->FindValue("htype");
			std::string address=m_pWebEm->FindValue("address");
			std::string sport=m_pWebEm->FindValue("port");
			std::string username=m_pWebEm->FindValue("username");
			std::string password=m_pWebEm->FindValue("password");
			if (
				(name=="")||
				(shtype=="")||
				(sport=="")
				)
				goto exitjson;
			_eHardwareTypes htype=(_eHardwareTypes)atoi(shtype.c_str());
			unsigned char mode1=0;
			unsigned char mode2=0;
			unsigned char mode3=0;
			unsigned char mode4=0;
			unsigned char mode5=0;
			int port=atoi(sport.c_str());
			if ((htype==HTYPE_RFXtrx315)||(htype==HTYPE_RFXtrx433))
			{
				//USB
			}
			else if (htype == HTYPE_RFXLAN) {
				//Lan
				if (address=="")
					goto exitjson;
			}
			else if (htype == HTYPE_Domoticz) {
				//Remote Domoticz
				if (address=="")
					goto exitjson;
			}
			else
				goto exitjson;

			std::string shared=m_pWebEm->FindValue("shared");
			std::string shareport=m_pWebEm->FindValue("shareport");
			std::string shareusername=m_pWebEm->FindValue("shareusername");
			std::string sharepassword=m_pWebEm->FindValue("sharepassword");
			std::string sharerights=m_pWebEm->FindValue("sharerights");
			int nsharerights=atoi(sharerights.c_str());
			int nshareport=atoi(shareport.c_str());

			if ((shared=="true")&&(shareport==""))
				goto exitjson;

			root["status"]="OK";
			root["title"]="AddHardware";
			sprintf(szTmp,
				"INSERT INTO Hardware (Name, Type, Address, Port, Username, Password, Mode1, Mode2, Mode3, Mode4, Mode5) VALUES ('%s',%d,'%s',%d,'%s','%s',%d,%d,%d,%d,%d)",
				name.c_str(),
				htype,
				address.c_str(),
				port,
				username.c_str(),
				password.c_str(),
				mode1,mode2,mode3,mode4,mode5
				);
			result=m_pMain->m_sql.query(szTmp);

			//add the device for real in our system
			strcpy(szTmp,"SELECT MAX(ID) FROM Hardware");
			result=m_pMain->m_sql.query(szTmp);
			if (result.size()>0)
			{
				std::vector<std::string> sd=result[0];
				int ID=atoi(sd[0].c_str());

				if (shared=="true") {
					//add sharing
					sprintf(szTmp,
						"INSERT INTO HardwareSharing (HardwareID, Port, Username, Password, Rights) VALUES (%d,%d,'%s','%s',%d)",
						ID,
						nshareport,
						shareusername.c_str(),
						sharepassword.c_str(),
						nsharerights
						);
					result=m_pMain->m_sql.query(szTmp);
				}

				m_pMain->AddHardwareFromParams(ID,name,htype,address,port,username,password,mode1,mode2,mode3,mode4,mode5);
			}
		}
		else if (cparam=="updatehardware")
		{
			std::string idx=m_pWebEm->FindValue("idx");
			if (idx=="")
				goto exitjson;
			std::string name=m_pWebEm->FindValue("name");
			std::string shtype=m_pWebEm->FindValue("htype");
			std::string address=m_pWebEm->FindValue("address");
			std::string sport=m_pWebEm->FindValue("port");
			std::string username=m_pWebEm->FindValue("username");
			std::string password=m_pWebEm->FindValue("password");
			if (
				(name=="")||
				(shtype=="")||
				(sport=="")
				)
				goto exitjson;

			_eHardwareTypes htype=(_eHardwareTypes)atoi(shtype.c_str());

			int port=atoi(sport.c_str());

			if ((htype==HTYPE_RFXtrx315)||(htype==HTYPE_RFXtrx433))
			{
				//USB
			}
			else if (htype == HTYPE_RFXLAN) {
				//Lan
				if (address=="")
					goto exitjson;
			}
			else if (htype == HTYPE_Domoticz) {
				//Remote Domoticz
				if (address=="")
					goto exitjson;
			}
			else
				goto exitjson;

			std::string shared=m_pWebEm->FindValue("shared");
			std::string shareport=m_pWebEm->FindValue("shareport");
			std::string shareusername=m_pWebEm->FindValue("shareusername");
			std::string sharepassword=m_pWebEm->FindValue("sharepassword");
			std::string sharerights=m_pWebEm->FindValue("sharerights");
			int nsharerights=atoi(sharerights.c_str());
			int nshareport=atoi(shareport.c_str());

			if ((shared=="true")&&(shareport==""))
				goto exitjson;

			unsigned char mode1=0;
			unsigned char mode2=0;
			unsigned char mode3=0;
			unsigned char mode4=0;
			unsigned char mode5=0;
			root["status"]="OK";
			root["title"]="UpdateHardware";

			sprintf(szTmp,
				"UPDATE Hardware SET Name='%s', Type=%d, Address='%s', Port=%d, Username='%s', Password='%s', Mode1=%d, Mode2=%d, Mode3=%d, Mode4=%d, Mode5=%d WHERE (ID == %s)",
				name.c_str(),
				htype,
				address.c_str(),
				port,
				username.c_str(),
				password.c_str(),
				mode1,mode2,mode3,mode4,mode5,
				idx.c_str()
				);
			result=m_pMain->m_sql.query(szTmp);

			sprintf(szTmp,"DELETE FROM HardwareSharing WHERE (HardwareID == %s)",idx.c_str());
			result=m_pMain->m_sql.query(szTmp);
			if (shared=="true") {
				//add sharing
				sprintf(szTmp,
					"INSERT INTO HardwareSharing (HardwareID, Port, Username, Password, Rights) VALUES (%s,%d,'%s','%s',%d)",
					idx.c_str(),
					nshareport,
					shareusername.c_str(),
					sharepassword.c_str(),
					nsharerights
					);
				result=m_pMain->m_sql.query(szTmp);
			}

			//re-add the device in our system
			int ID=atoi(idx.c_str());
			m_pMain->AddHardwareFromParams(ID,name,htype,address,port,username,password,mode1,mode2,mode3,mode4,mode5);
		}
		else if (cparam=="deletehardware")
		{
			std::string idx=m_pWebEm->FindValue("idx");
			if (idx=="")
				goto exitjson;
			root["status"]="OK";
			root["title"]="DeleteHardware";

			m_pMain->m_sql.DeleteHardware(idx);
			m_pMain->RemoveDomoticzHardware(atoi(idx.c_str()));
		}
		else if (cparam=="addtimer")
		{
			std::string idx=m_pWebEm->FindValue("idx");
			std::string active=m_pWebEm->FindValue("active");
			std::string stimertype=m_pWebEm->FindValue("timertype");
			std::string shour=m_pWebEm->FindValue("hour");
			std::string smin=m_pWebEm->FindValue("min");
			std::string scmd=m_pWebEm->FindValue("command");
			std::string sdays=m_pWebEm->FindValue("days");
			if (
				(idx=="")||
				(active=="")||
				(stimertype=="")||
				(shour=="")||
				(smin=="")||
				(scmd=="")||
				(sdays=="")
				)
				goto exitjson;
			unsigned char hour = atoi(shour.c_str());
			unsigned char min = atoi(smin.c_str());
			unsigned char icmd = atoi(scmd.c_str());
			unsigned char iTimerType=atoi(stimertype.c_str());
			int days=atoi(sdays.c_str());
			sprintf(szData,"%02d:%02d",hour,min);
			root["status"]="OK";
			root["title"]="AddTimer";
			sprintf(szTmp,
				"INSERT INTO Timers (Active, DeviceRowID, Time, Type, Cmd, Level, Days) VALUES (%d,%s,'%s',%d,%d,%d,%d)",
				(active=="true")?1:0,
				idx.c_str(),
				szData,
				iTimerType,
				icmd,
				0,
				days
				);
			result=m_pMain->m_sql.query(szTmp);
			m_pMain->m_scheduler.ReloadSchedules();
		}
		else if (cparam=="updatetimer")
		{
			std::string idx=m_pWebEm->FindValue("idx");
			std::string active=m_pWebEm->FindValue("active");
			std::string stimertype=m_pWebEm->FindValue("timertype");
			std::string shour=m_pWebEm->FindValue("hour");
			std::string smin=m_pWebEm->FindValue("min");
			std::string scmd=m_pWebEm->FindValue("command");
			std::string sdays=m_pWebEm->FindValue("days");
			if (
				(idx=="")||
				(active=="")||
				(stimertype=="")||
				(shour=="")||
				(smin=="")||
				(scmd=="")||
				(sdays=="")
				)
				goto exitjson;
			unsigned char hour = atoi(shour.c_str());
			unsigned char min = atoi(smin.c_str());
			unsigned char icmd = atoi(scmd.c_str());
			unsigned char iTimerType=atoi(stimertype.c_str());
			int days=atoi(sdays.c_str());
			sprintf(szData,"%02d:%02d",hour,min);
			root["status"]="OK";
			root["title"]="AddTimer";
			sprintf(szTmp,
				"UPDATE Timers SET Active=%d, Time='%s', Type=%d, Cmd=%d, Level=%d, Days=%d WHERE (ID == %s)",
				(active=="true")?1:0,
				szData,
				iTimerType,
				icmd,
				0,
				days,
				idx.c_str()
				);
			result=m_pMain->m_sql.query(szTmp);
			m_pMain->m_scheduler.ReloadSchedules();
		}
		else if (cparam=="deletetimer")
		{
			std::string idx=m_pWebEm->FindValue("idx");
			if (idx=="")
				goto exitjson;
			sprintf(szTmp,
				"DELETE FROM Timers WHERE (ID == %s)",
				idx.c_str()
				);
			result=m_pMain->m_sql.query(szTmp);
			m_pMain->m_scheduler.ReloadSchedules();
		}
		else if (cparam=="clearlightlog")
		{
			std::string idx=m_pWebEm->FindValue("idx");
			if (idx=="")
				goto exitjson;
			//First get Device Type/SubType
			szQuery.clear();
			szQuery.str("");
			szQuery << "SELECT Type, SubType FROM DeviceStatus WHERE (ID == " << idx << ")";
			result=m_pMain->m_sql.query(szQuery.str());
			if (result.size()<1)
				goto exitjson;

			unsigned char dType=atoi(result[0][0].c_str());
			unsigned char dSubType=atoi(result[0][1].c_str());

			if (
				(dType!=pTypeLighting1)&&
				(dType!=pTypeLighting2)&&
				(dType!=pTypeLighting3)&&
				(dType!=pTypeLighting4)&&
				(dType!=pTypeLighting5)&&
				(dType!=pTypeLighting6)
				)
				goto exitjson; //no light device! we should not be here!

			root["status"]="OK";
			root["title"]="ClearLightLog";

			szQuery.clear();
			szQuery.str("");
			szQuery << "DELETE FROM LightingLog WHERE (DeviceRowID==" << idx << ")";
			result=m_pMain->m_sql.query(szQuery.str());
		}
		else if (cparam=="cleartimers")
		{
			std::string idx=m_pWebEm->FindValue("idx");
			if (idx=="")
				goto exitjson;
			sprintf(szTmp,
				"DELETE FROM Timers WHERE (DeviceRowID == %s)",
				idx.c_str()
				);
			result=m_pMain->m_sql.query(szTmp);
			m_pMain->m_scheduler.ReloadSchedules();
		}
		else if (cparam=="learnsw")
		{
			m_pMain->m_sql.m_LastSwitchID="";
			bool bReceivedSwitch=false;
			unsigned char cntr=0;
			while ((!bReceivedSwitch)&&(cntr<50))	//wait for max. 5 seconds
			{
				if (m_pMain->m_sql.m_LastSwitchID!="")
				{
					bReceivedSwitch=true;
					break;
				}
				else
				{
					//sleep 100ms
					boost::this_thread::sleep(boost::posix_time::milliseconds(100));
					cntr++;
				}
			}
			if (bReceivedSwitch)
			{
				//check if used
				szQuery.clear();
				szQuery.str("");
				szQuery << "SELECT Name, Used FROM DeviceStatus WHERE (ID==" << m_pMain->m_sql.m_LastSwitchRowID << ")";
				result=m_pMain->m_sql.query(szQuery.str());
				if (result.size()>0)
				{
					root["status"]="OK";
					root["title"]="LearnSW";
					root["ID"]=m_pMain->m_sql.m_LastSwitchID;
					root["idx"]=m_pMain->m_sql.m_LastSwitchRowID;
					root["Name"]=result[0][0];
					root["Used"]=atoi(result[0][1].c_str());
				}
			}
		} //learnsw
		else if (cparam == "makefavorite")
		{
			std::string idx=m_pWebEm->FindValue("idx");
			std::string sisfavorite=m_pWebEm->FindValue("isfavorite");
			if ((idx=="")||(sisfavorite==""))
				goto exitjson;
			int isfavorite=atoi(sisfavorite.c_str());
			szQuery.clear();
			szQuery.str("");
			szQuery << "UPDATE DeviceStatus SET Favorite=" << isfavorite << " WHERE (ID == " << idx << ")";
			result=m_pMain->m_sql.query(szQuery.str());
			if (result.size()>0)
			{
				root["status"]="OK";
				root["title"]="MakeFavorite";
			}
		} //makefavorite
		else if (cparam=="switchlight")
		{
			std::string idx=m_pWebEm->FindValue("idx");
			std::string switchcmd=m_pWebEm->FindValue("switchcmd");
			std::string level=m_pWebEm->FindValue("level");
			if ((idx=="")||(switchcmd==""))
				goto exitjson;

			if (m_pMain->SwitchLight(idx,switchcmd,level)==true)
			{
				root["status"]="OK";
				root["title"]="SwitchLight";
			}
		} //(rtype=="switchlight")
		else if (cparam =="getSunRiseSet") {
			int nValue=0;
			std::string sValue;
			if (m_pMain->m_sql.GetTempVar("SunRiseSet",nValue,sValue))
			{
				std::vector<std::string> strarray;
				StringSplit(sValue, ";", strarray);
				if (strarray.size()==2)
				{
					root["status"]="OK";
					root["title"]="getSunRiseSet";
					root["Sunrise"]=strarray[0];
					root["Sunset"]=strarray[1];
				}
			}
		}
	} //(rtype=="command")
	else if (rtype=="setused")
	{
		std::string idx=m_pWebEm->FindValue("idx");
		std::string name=m_pWebEm->FindValue("name");
		std::string sused=m_pWebEm->FindValue("used");
		std::string sswitchtype=m_pWebEm->FindValue("switchtype");
		int switchtype=-1;
		if (sswitchtype!="")
			switchtype=atoi(sswitchtype.c_str());

		if ((idx=="")||(sused==""))
			goto exitjson;
		int used=(sused=="true")?1:0;
		if (!used)
			name="Unknown";

		szQuery.clear();
		szQuery.str("");
		if (switchtype==-1)
			szQuery << "UPDATE DeviceStatus SET Used=" << used << ", Name='" << name << "' WHERE (ID == " << idx << ")";
		else
			szQuery << "UPDATE DeviceStatus SET Used=" << used << ", Name='" << name << "', SwitchType=" << switchtype << " WHERE (ID == " << idx << ")";
		result=m_pMain->m_sql.query(szQuery.str());
		if (result.size()>0)
		{
			root["status"]="OK";
			root["title"]="SetUsed";
		}

	} //(rtype=="setused")
	else if (rtype=="deletedevice")
	{
		std::string idx=m_pWebEm->FindValue("idx");
		if (idx=="")
			goto exitjson;

		root["status"]="OK";
		root["title"]="DeleteDevice";
		m_pMain->m_sql.DeleteDevice(idx);
	} //(rtype=="setused")
	else if (rtype=="settings")
	{
		root["status"]="OK";
		root["title"]="settings";

		szQuery.clear();
		szQuery.str("");
		szQuery << "SELECT Key, nValue, sValue FROM Preferences";
		result=m_pMain->m_sql.query(szQuery.str());
		if (result.size()>0)
		{
			std::vector<std::vector<std::string> >::const_iterator itt;
			for (itt=result.begin(); itt!=result.end(); ++itt)
			{
				std::vector<std::string> sd=*itt;
				std::string Key=sd[0];
				int nValue=atoi(sd[1].c_str());
				std::string sValue=sd[2];

				if (Key=="Location")
				{
					std::vector<std::string> strarray;
					StringSplit(sValue, ";", strarray);

					if (strarray.size()==2)
					{
						root["Location"]["Latitude"]=strarray[0];
						root["Location"]["Longitude"]=strarray[1];
					}
				}
				else if (Key=="ProwlAPI")
				{
					root["ProwlAPI"]=sValue;
				}
				else if (Key=="NMAAPI")
				{
					root["NMAAPI"]=sValue;
				}
				else if (Key=="DashboardType")
				{
					root["DashboardType"]=nValue;
				}
				else if (Key=="LightHistoryDays")
				{
					root["LightHistoryDays"]=nValue;
				}
			}
		}

	} //(rtype=="settings")
exitjson:
	m_retstr=root.toStyledString();
	return (char*)m_retstr.c_str();
}


} //server
}//http


