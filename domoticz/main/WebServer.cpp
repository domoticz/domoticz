#include "stdafx.h"
#include "WebServer.h"
#include <boost/bind.hpp>
#include <iostream>
#include "mainworker.h"
#include "Helper.h"
#include "localtime_r.h"
#include "EventSystem.h"
#include "../webserver/cWebem.h"
#include "../httpclient/HTTPClient.h"
#include "../hardware/hardwaretypes.h"
#include "../hardware/1Wire.h"
#include "../hardware/OpenZWave.h"
#include "../webserver/Base64.h"
#include "../smtpclient/SMTPClient.h"
#include "../json/config.h"
#include "../json/json.h"
#include "Logger.h"
#ifndef WIN32
	#include <sys/utsname.h>
#else
	#include "WindowsHelper.h"
#endif

#include "mainstructs.h"

#define round(a) ( int ) ( a + .5 )

extern std::string szStartupFolder;
extern bool bIsRaspberryPi;
extern std::string szAppVersion;


struct _tGuiLanguage {
	const char* szShort;
	const char* szLong;
};

static const _tGuiLanguage guiLanguage[]=
{
	{ "en", "English" },
	{ "es", "Spanish" },
	{ "nl", "Dutch" },
	{ "de", "German" },
	{ "fr", "French" },
	{ "it", "Italian" },
	{ "sv", "Swedish" },
	
	{ NULL, NULL}
};

namespace http {
	namespace server {

CWebServer::CWebServer(void)
{
	m_pWebEm=NULL;
	m_LastUpdateCheck=0;
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
	while (1==1)
	{
		try
		{
			if (m_pWebEm)
				m_pWebEm->Run();
		}
		catch(...)
		{
			_log.Log(LOG_ERROR,"WebServer stopped by exception, starting again...");
			m_pWebEm->Stop();
			continue;
		}
		break;
	}

	_log.Log(LOG_NORM,"WebServer stopped...");
}



bool CWebServer::StartServer(MainWorker *pMain, const std::string &listenaddress, const std::string &listenport, const std::string &serverpath, const bool bIgnoreUsernamePassword)
{
	m_pMain=pMain;
	StopServer();

	m_custom_light_icons.clear();
	std::string sLine = "";
	std::ifstream infile;
	std::string switchlightsfile=serverpath+"/switch_icons.txt";
	infile.open(switchlightsfile.c_str());
	if (infile.is_open())
	{
		while (!infile.eof())
		{
			getline(infile, sLine);
			if (sLine.size()!=0)
			{
				std::vector<std::string> results;
				StringSplit(sLine,";",results);
				if (results.size()==3)
				{
					_tCustomIcon cImage;
					cImage.RootFile=results[0];
					cImage.Title=results[1];
					cImage.Description=results[2];
					m_custom_light_icons.push_back(cImage);
				}
			}
		}
	}


	if (m_pWebEm!=NULL)
		delete m_pWebEm;
    try {
        m_pWebEm= new http::server::cWebem(
                                           listenaddress.c_str(),						// address
                                           listenport.c_str(),							// port
                                           serverpath.c_str());
    }
    catch(...) {
        _log.Log(LOG_ERROR,"Failed to start the web server");
        if(atoi(listenport.c_str())<1024)
            _log.Log(LOG_ERROR,"check privileges for opening ports below 1024");
		else
			_log.Log(LOG_ERROR,"check if no other application is using port: %s",listenport.c_str());
        return false;
    }
	m_pWebEm->SetDigistRealm("Domoticz.com");

	if (!bIgnoreUsernamePassword)
	{
		LoadUsers();
		std::string WebLocalNetworks;
		int nValue;
		if (m_pMain->m_sql.GetPreferencesVar("WebLocalNetworks",nValue,WebLocalNetworks))
		{
			std::vector<std::string> strarray;
			StringSplit(WebLocalNetworks, ";", strarray);
			std::vector<std::string>::const_iterator itt;
			for (itt=strarray.begin(); itt!=strarray.end(); ++itt)
			{
				std::string network=*itt;
				m_pWebEm->AddLocalNetworks(network);
			}
			//add local hostname
			m_pWebEm->AddLocalNetworks("");
		}
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

	m_pWebEm->RegisterIncludeCode( "combolanguage",
		boost::bind(
		&CWebServer::DisplayLanguageCombo,
		this ) );

	m_pWebEm->RegisterIncludeCode( "deviceslist",
		boost::bind(
		&CWebServer::DisplayDevicesList,
		this ) );

	m_pWebEm->RegisterPageCode( "/json.htm",
		boost::bind(
		&CWebServer::GetJSonPage,	// member function
		this ) );			// instance of class
	m_pWebEm->RegisterPageCode( "/camsnapshot.jpg", 
		boost::bind( 
		&CWebServer::GetCameraSnapshot,
		this ) );
	m_pWebEm->RegisterPageCode( "/backupdatabase.php", 
		boost::bind( 
		&CWebServer::GetDatabaseBackup,
		this ) );

	m_pWebEm->RegisterPageCode( "/raspberry.cgi",
		boost::bind(
		&CWebServer::GetInternalCameraSnapshot,	// member function
		this ) );			// instance of class
	m_pWebEm->RegisterPageCode( "/uvccapture.cgi",
		boost::bind(
		&CWebServer::GetInternalCameraSnapshot,	// member function
		this ) );			// instance of class
//	m_pWebEm->RegisterPageCode( "/videostream.cgi",
	//	boost::bind(
		//&CWebServer::GetInternalCameraSnapshot,	// member function
		//this ) );			// instance of class
	
	m_pWebEm->RegisterPageCode( "/getlanguage.js",
		boost::bind(
		&CWebServer::GetLanguage,	// member function
		this ) );			// instance of class
	

	m_pWebEm->RegisterActionCode( "storesettings",boost::bind(&CWebServer::PostSettings,this));
	m_pWebEm->RegisterActionCode( "setrfxcommode",boost::bind(&CWebServer::SetRFXCOMMode,this));
	m_pWebEm->RegisterActionCode( "setrego6xxtype",boost::bind(&CWebServer::SetRego6XXType,this));
	m_pWebEm->RegisterActionCode( "sets0metertype",boost::bind(&CWebServer::SetS0MeterType,this));
	m_pWebEm->RegisterActionCode( "setlimitlesstype",boost::bind(&CWebServer::SetLimitlessType,this));

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

void CWebServer::SetAuthenticationMethod(int amethod)
{
	if (m_pWebEm==NULL)
		return;
	m_pWebEm->SetAuthenticationMethod((_eAuthenticationMethod)amethod);
}

char * CWebServer::DisplayVersion()
{
	m_retstr=szAppVersion;
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

char * CWebServer::DisplayLanguageCombo()
{
	m_retstr="";
	char szTmp[200];
	int ii=0;
	while (guiLanguage[ii].szShort!=NULL)
	{
		sprintf(szTmp,"<option value=\"%s\">%s</option>\n",guiLanguage[ii].szShort,guiLanguage[ii].szLong);
		m_retstr+=szTmp;
		ii++;
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
			case HTYPE_Dummy:
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
	std::map<std::string,int> _htypes;
	char szTmp[200];
	for (int ii=0; ii<HTYPE_END; ii++)
	{
		bool bDoAdd=true;
#ifndef _DEBUG
#ifdef WIN32
		if (
			(ii == HTYPE_RaspberryBMP085)
			)
		{
			bDoAdd=false;
		}
		else
		{
#ifndef WITH_LIBUSB
			if (
				(ii == HTYPE_VOLCRAFTCO20)||
				(ii == HTYPE_TE923)
				)
			{
				bDoAdd=false;
			}
#endif

		}
#endif
#endif
#ifndef WITH_OPENZWAVE
		if (ii == HTYPE_OpenZWave)
			bDoAdd=false;
#endif
		if ((ii == HTYPE_1WIRE)&&(!C1Wire::Have1WireSystem()))
			bDoAdd=false;

		if (bDoAdd)
			_htypes[Hardware_Type_Desc(ii)]=ii;
	}
	//return a sorted hardware list
	std::map<std::string,int>::const_iterator itt;
	for (itt=_htypes.begin(); itt!=_htypes.end(); ++itt)
	{
		sprintf(szTmp,"<option value=\"%d\">%s</option>\n",itt->second,itt->first.c_str());
		m_retstr+=szTmp;

	}
	return (char*)m_retstr.c_str();
}

char * CWebServer::DisplaySerialDevicesCombo()
{
	m_retstr="";
	char szTmp[200];
	bool bUseDirectPath=false;
	std::vector<std::string> serialports=GetSerialPorts(bUseDirectPath);
	std::vector<std::string>::iterator itt;
	int iDevice=0;
	for (itt=serialports.begin(); itt!=serialports.end(); ++itt)
	{
		std::string serialname=*itt;
		int snumber=-1;
		if (!bUseDirectPath)
		{
			size_t pos=serialname.find_first_of("01234567890");
			if (pos!=std::string::npos) {
				snumber=atoi(serialname.substr(pos).c_str());
			}
		}
		else
		{
			snumber=iDevice;
		}
		if (iDevice!=-1) 
		{
			sprintf(szTmp,"<option value=\"%d\">%s</option>\n",snumber,serialname.c_str());
			m_retstr+=szTmp;
		}
		iDevice++;
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

char * CWebServer::DisplayDevicesList()
{
	m_retstr="";
	char szTmp[300];

	std::vector<std::vector<std::string> > result;
	result=m_pMain->m_sql.query("SELECT ID, Name FROM DeviceStatus WHERE (Used == 1) ORDER BY Name");
	if (result.size()>0)
	{
		std::vector<std::vector<std::string> >::const_iterator itt;
		for (itt=result.begin(); itt!=result.end(); ++itt)
		{
			std::vector<std::string> sd=*itt;
			sprintf(szTmp,"<option value=\"%s\">%s</option>\n",sd[0].c_str(),sd[1].c_str());
			m_retstr+=szTmp;
		}
	}

	return (char*)m_retstr.c_str();
}

void CWebServer::LoadUsers()
{
	ClearUserPasswords();
	std::string WebUserName,WebPassword;
	int nValue=0;
	if (m_pMain->m_sql.GetPreferencesVar("WebUserName",nValue,WebUserName))
	{
		if (m_pMain->m_sql.GetPreferencesVar("WebPassword",nValue,WebPassword))
		{
			if ((WebUserName!="")&&(WebPassword!="")) 
			{
				WebUserName=base64_decode(WebUserName);
				WebPassword=base64_decode(WebPassword);
				AddUser(10000,WebUserName, WebPassword, URIGHTS_ADMIN);

				std::vector<std::vector<std::string> > result;
				result=m_pMain->m_sql.query("SELECT ID, Active, Username, Password, Rights FROM Users");
				if (result.size()>0)
				{
					std::vector<std::vector<std::string> >::const_iterator itt;
					int ii=0;
					for (itt=result.begin(); itt!=result.end(); ++itt)
					{
						std::vector<std::string> sd=*itt;

						unsigned long ID;
						std::stringstream s_strid;
						s_strid << std::hex << sd[0];
						s_strid >> ID;

						std::string username=base64_decode(sd[2]);
						std::string password=base64_decode(sd[3]);

						_eUserRights rights=(_eUserRights)atoi(sd[4].c_str());

						int bIsActive=(int)atoi(sd[1].c_str());
						if (bIsActive)
						{
							AddUser(ID,username,password,rights);
						}
					}
				}
			}
		}
	}
	m_pMain->LoadSharedUsers();
}

void CWebServer::AddUser(const unsigned long ID, const std::string &username, const std::string &password, const int userrights)
{
	_tWebUserPassword wtmp;
	wtmp.ID=ID;
	wtmp.Username=username;
	wtmp.Password=password;
	wtmp.userrights=(_eUserRights)userrights;
	m_users.push_back(wtmp);

	m_pWebEm->AddUserPassword(ID,username,password,(_eUserRights)userrights);
}

void CWebServer::SaveUsers()
{
}

void CWebServer::ClearUserPasswords()
{
	m_users.clear();
	m_pWebEm->ClearUserPasswords();
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

bool CWebServer::FindAdminUser()
{
	std::vector<_tWebUserPassword>::const_iterator itt;
	for (itt=m_users.begin(); itt!=m_users.end(); ++itt)
	{
		if (itt->userrights== URIGHTS_ADMIN)
			return true;
	}
	return false;
}

char * CWebServer::PostSettings()
{
	m_retstr="/index.html";

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
	std::string MobileType=m_pWebEm->FindValue("MobileType");
	m_pMain->m_sql.UpdatePreferencesVar("MobileType",atoi(MobileType.c_str()));

	int nUnit=atoi(m_pWebEm->FindValue("WindUnit").c_str());
	m_pMain->m_sql.UpdatePreferencesVar("WindUnit",nUnit);
	m_pMain->m_sql.m_windunit=(_eWindUnit)nUnit;
/*
	nUnit=atoi(m_pWebEm->FindValue("TempUnit").c_str());
	m_pMain->m_sql.UpdatePreferencesVar("TempUnit",nUnit);
	m_pMain->m_sql.m_tempunit=(_eTempUnit)nUnit;
*/
	m_pMain->m_sql.SetUnitsAndScale();

	std::string AuthenticationMethod=m_pWebEm->FindValue("AuthenticationMethod");
	_eAuthenticationMethod amethod=(_eAuthenticationMethod)atoi(AuthenticationMethod.c_str());
	m_pMain->m_sql.UpdatePreferencesVar("AuthenticationMethod",(int)amethod);
	m_pWebEm->SetAuthenticationMethod(amethod);
	
	std::string ReleaseChannel=m_pWebEm->FindValue("ReleaseChannel");
	m_pMain->m_sql.UpdatePreferencesVar("ReleaseChannel",atoi(ReleaseChannel.c_str()));

	std::string LightHistoryDays=m_pWebEm->FindValue("LightHistoryDays");
	m_pMain->m_sql.UpdatePreferencesVar("LightHistoryDays",atoi(LightHistoryDays.c_str()));

	std::string s5MinuteHistoryDays=m_pWebEm->FindValue("ShortLogDays");
	m_pMain->m_sql.UpdatePreferencesVar("5MinuteHistoryDays",atoi(s5MinuteHistoryDays.c_str()));

	std::string sElectricVoltage=m_pWebEm->FindValue("ElectricVoltage");
	m_pMain->m_sql.UpdatePreferencesVar("ElectricVoltage",atoi(sElectricVoltage.c_str()));

	std::string sCM113DisplayType=m_pWebEm->FindValue("CM113DisplayType");
	m_pMain->m_sql.UpdatePreferencesVar("CM113DisplayType",atoi(sCM113DisplayType.c_str()));
	
	std::string WebUserName=m_pWebEm->FindValue("WebUserName");
	std::string WebPassword=m_pWebEm->FindValue("WebPassword");
	std::string WebLocalNetworks=m_pWebEm->FindValue("WebLocalNetworks");
	WebUserName=CURLEncode::URLDecode(WebUserName);
	WebPassword=CURLEncode::URLDecode(WebPassword);
	WebLocalNetworks=CURLEncode::URLDecode(WebLocalNetworks);

	if ((WebUserName=="")||(WebPassword==""))
	{
		WebUserName="";
		WebPassword="";
	}
	WebUserName=base64_encode((const unsigned char*)WebUserName.c_str(),WebUserName.size());
	WebPassword=base64_encode((const unsigned char*)WebPassword.c_str(),WebPassword.size());
	m_pMain->m_sql.UpdatePreferencesVar("WebUserName",WebUserName.c_str());
	m_pMain->m_sql.UpdatePreferencesVar("WebPassword",WebPassword.c_str());
	m_pMain->m_sql.UpdatePreferencesVar("WebLocalNetworks",WebLocalNetworks.c_str());

	LoadUsers();
	m_pWebEm->ClearLocalNetworks();
	std::vector<std::string> strarray;
	StringSplit(WebLocalNetworks, ";", strarray);
	std::vector<std::string>::const_iterator itt;
	for (itt=strarray.begin(); itt!=strarray.end(); ++itt)
	{
		std::string network=*itt;
		m_pWebEm->AddLocalNetworks(network);
	}
	//add local hostname
	m_pWebEm->AddLocalNetworks("");

	std::string SecPassword=m_pWebEm->FindValue("SecPassword");
	SecPassword=CURLEncode::URLDecode(SecPassword);
	SecPassword=base64_encode((const unsigned char*)SecPassword.c_str(),SecPassword.size());
	m_pMain->m_sql.UpdatePreferencesVar("SecPassword",SecPassword.c_str());

	int EnergyDivider=atoi(m_pWebEm->FindValue("EnergyDivider").c_str());
	int GasDivider=atoi(m_pWebEm->FindValue("GasDivider").c_str());
	int WaterDivider=atoi(m_pWebEm->FindValue("WaterDivider").c_str());
	if (EnergyDivider<1)
		EnergyDivider=1000;
	if (GasDivider<1)
		GasDivider=100;
	if (WaterDivider<1)
		WaterDivider=100;
	m_pMain->m_sql.UpdatePreferencesVar("MeterDividerEnergy",EnergyDivider);
	m_pMain->m_sql.UpdatePreferencesVar("MeterDividerGas",GasDivider);
	m_pMain->m_sql.UpdatePreferencesVar("MeterDividerWater",WaterDivider);

	std::string scheckforupdates=m_pWebEm->FindValue("checkforupdates");
	m_pMain->m_sql.UpdatePreferencesVar("UseAutoUpdate",(scheckforupdates=="on"?1:0));

	float CostEnergy=(float)atof(m_pWebEm->FindValue("CostEnergy").c_str());
	float CostEnergyT2=(float)atof(m_pWebEm->FindValue("CostEnergyT2").c_str());
	float CostGas=(float)atof(m_pWebEm->FindValue("CostGas").c_str());
	float CostWater=(float)atof(m_pWebEm->FindValue("CostWater").c_str());
	m_pMain->m_sql.UpdatePreferencesVar("CostEnergy",int(CostEnergy*10000.0f));
	m_pMain->m_sql.UpdatePreferencesVar("CostEnergyT2",int(CostEnergyT2*10000.0f));
	m_pMain->m_sql.UpdatePreferencesVar("CostGas",int(CostGas*10000.0f));
	m_pMain->m_sql.UpdatePreferencesVar("CostWater",int(CostWater*10000.0f));

	m_pMain->m_sql.UpdatePreferencesVar("EmailFrom",CURLEncode::URLDecode(m_pWebEm->FindValue("EmailFrom")).c_str());
	m_pMain->m_sql.UpdatePreferencesVar("EmailTo",CURLEncode::URLDecode(m_pWebEm->FindValue("EmailTo")).c_str());
	m_pMain->m_sql.UpdatePreferencesVar("EmailServer",m_pWebEm->FindValue("EmailServer").c_str());
	m_pMain->m_sql.UpdatePreferencesVar("EmailPort",atoi(m_pWebEm->FindValue("EmailPort").c_str()));
	std::string suseemailinnotificationsalerts=m_pWebEm->FindValue("useemailinnotificationsalerts");
	m_pMain->m_sql.UpdatePreferencesVar("UseEmailInNotifications",(suseemailinnotificationsalerts=="on"?1:0));
	std::string sEmailAsAttachment=m_pWebEm->FindValue("EmailAsAttachment");
	m_pMain->m_sql.UpdatePreferencesVar("EmailAsAttachment",(sEmailAsAttachment=="on"?1:0));

	m_pMain->m_sql.UpdatePreferencesVar("DoorbellCommand",atoi(m_pWebEm->FindValue("DoorbellCommand").c_str()));
	m_pMain->m_sql.UpdatePreferencesVar("SmartMeterType",atoi(m_pWebEm->FindValue("SmartMeterType").c_str()));

	std::string EmailUsername=CURLEncode::URLDecode(m_pWebEm->FindValue("EmailUsername"));
	std::string EmailPassword=CURLEncode::URLDecode(m_pWebEm->FindValue("EmailPassword"));
	EmailUsername=base64_encode((const unsigned char*)EmailUsername.c_str(),EmailUsername.size());
	EmailPassword=base64_encode((const unsigned char*)EmailPassword.c_str(),EmailPassword.size());

	m_pMain->m_sql.UpdatePreferencesVar("EmailUsername",EmailUsername.c_str());
	m_pMain->m_sql.UpdatePreferencesVar("EmailPassword",EmailPassword.c_str());

	std::string EnableTabLights=m_pWebEm->FindValue("EnableTabLights");
	m_pMain->m_sql.UpdatePreferencesVar("EnableTabLights",(EnableTabLights=="on"?1:0));
	std::string EnableTabTemp=m_pWebEm->FindValue("EnableTabTemp");
	m_pMain->m_sql.UpdatePreferencesVar("EnableTabTemp",(EnableTabTemp=="on"?1:0));
	std::string EnableTabWeather=m_pWebEm->FindValue("EnableTabWeather");
	m_pMain->m_sql.UpdatePreferencesVar("EnableTabWeather",(EnableTabWeather=="on"?1:0));
	std::string EnableTabUtility=m_pWebEm->FindValue("EnableTabUtility");
	m_pMain->m_sql.UpdatePreferencesVar("EnableTabUtility",(EnableTabUtility=="on"?1:0));
	std::string EnableTabScenes=m_pWebEm->FindValue("EnableTabScenes");
	m_pMain->m_sql.UpdatePreferencesVar("EnableTabScenes",(EnableTabScenes=="on"?1:0));

	m_pMain->m_sql.UpdatePreferencesVar("NotificationSensorInterval",atoi(m_pWebEm->FindValue("NotificationSensorInterval").c_str()));
	m_pMain->m_sql.UpdatePreferencesVar("NotificationSwitchInterval",atoi(m_pWebEm->FindValue("NotificationSwitchInterval").c_str()));

	std::string RaspCamParams=m_pWebEm->FindValue("RaspCamParams");
	if (RaspCamParams!="")
		m_pMain->m_sql.UpdatePreferencesVar("RaspCamParams",RaspCamParams.c_str());


	int rnOldvalue=0;
	m_pMain->m_sql.GetPreferencesVar("RemoteSharedPort", rnOldvalue);

	m_pMain->m_sql.UpdatePreferencesVar("RemoteSharedPort",atoi(m_pWebEm->FindValue("RemoteSharedPort").c_str()));

	int rnvalue=0;
	m_pMain->m_sql.GetPreferencesVar("RemoteSharedPort", rnvalue);

	if (rnvalue!=rnOldvalue)
	{
		if (rnvalue!=0)
		{
			char szPort[100];
			sprintf(szPort,"%d",rnvalue);
			m_pMain->m_sharedserver.StopServer();
			m_pMain->m_sharedserver.StartServer("0.0.0.0",szPort);
			m_pMain->LoadSharedUsers();
		}
	}

	m_pMain->m_sql.UpdatePreferencesVar("Language",m_pWebEm->FindValue("Language").c_str());

	m_pMain->m_sql.GetPreferencesVar("RandomTimerFrame", rnOldvalue);
	rnvalue=atoi(m_pWebEm->FindValue("RandomSpread").c_str());
	if (rnOldvalue!=rnvalue)
	{
		m_pMain->m_sql.UpdatePreferencesVar("RandomTimerFrame",rnvalue);
		m_pMain->m_scheduler.ReloadSchedules();
	}


	return (char*)m_retstr.c_str();
}

char * CWebServer::SetRFXCOMMode()
{
	m_retstr="";
	std::string idx=m_pWebEm->FindValue("idx");
	if (idx=="") {
		return (char*)m_retstr.c_str();
	}
	std::vector<std::vector<std::string> > result;
	std::stringstream szQuery;

	szQuery.clear();
	szQuery.str("");
	szQuery << "SELECT Mode1, Mode2, Mode3, Mode4, Mode5 FROM Hardware WHERE (ID=" << idx << ")";
	result=m_pMain->m_sql.query(szQuery.str());
	if (result.size()<1)
		return (char*)m_retstr.c_str();

	m_retstr="/index.html";

	unsigned char Mode1=atoi(result[0][0].c_str());
	unsigned char Mode2=atoi(result[0][1].c_str());
	unsigned char Mode3=atoi(result[0][2].c_str());
	unsigned char Mode4=atoi(result[0][3].c_str());
	unsigned char Mode5=atoi(result[0][4].c_str());

	tRBUF Response;
	Response.ICMND.msg1=Mode1;
	Response.ICMND.msg2=Mode2;
	Response.ICMND.msg3=Mode3;
	Response.ICMND.msg4=Mode4;
	Response.ICMND.msg5=Mode5;

	Response.IRESPONSE.UNDECODEDenabled=(m_pWebEm->FindValue("undecon")=="on")?1:0;
	Response.IRESPONSE.X10enabled=(m_pWebEm->FindValue("X10")=="on")?1:0;
	Response.IRESPONSE.ARCenabled=(m_pWebEm->FindValue("ARC")=="on")?1:0;
	Response.IRESPONSE.ACenabled=(m_pWebEm->FindValue("AC")=="on")?1:0;
	Response.IRESPONSE.HEEUenabled=(m_pWebEm->FindValue("HomeEasyEU")=="on")?1:0;
	Response.IRESPONSE.MEIANTECHenabled=(m_pWebEm->FindValue("Meiantech")=="on")?1:0;
	Response.IRESPONSE.OREGONenabled=(m_pWebEm->FindValue("OregonScientific")=="on")?1:0;
	Response.IRESPONSE.ATIenabled=(m_pWebEm->FindValue("ATIremote")=="on")?1:0;
	Response.IRESPONSE.VISONICenabled=(m_pWebEm->FindValue("Visonic")=="on")?1:0;
	Response.IRESPONSE.MERTIKenabled=(m_pWebEm->FindValue("Mertik")=="on")?1:0;
	Response.IRESPONSE.LWRFenabled=(m_pWebEm->FindValue("ADLightwaveRF")=="on")?1:0;
	Response.IRESPONSE.HIDEKIenabled=(m_pWebEm->FindValue("HidekiUPM")=="on")?1:0;
	Response.IRESPONSE.LACROSSEenabled=(m_pWebEm->FindValue("LaCrosse")=="on")?1:0;
	Response.IRESPONSE.FS20enabled=(m_pWebEm->FindValue("FS20")=="on")?1:0;
	Response.IRESPONSE.PROGUARDenabled=(m_pWebEm->FindValue("ProGuard")=="on")?1:0;
	Response.IRESPONSE.BLINDST0enabled=(m_pWebEm->FindValue("BlindT0")=="on")?1:0;
	Response.IRESPONSE.BLINDST1enabled=(m_pWebEm->FindValue("BlindT1T2T3T4")=="on")?1:0;
	Response.IRESPONSE.AEenabled=(m_pWebEm->FindValue("AEBlyss")=="on")?1:0;
	Response.IRESPONSE.RUBICSONenabled=(m_pWebEm->FindValue("Rubicson")=="on")?1:0;
	Response.IRESPONSE.FINEOFFSETenabled=(m_pWebEm->FindValue("FineOffsetViking")=="on")?1:0;
	Response.IRESPONSE.LIGHTING4enabled=(m_pWebEm->FindValue("Lighting4")=="on")?1:0;
	Response.IRESPONSE.RSLenabled=(m_pWebEm->FindValue("RSL")=="on")?1:0;
	Response.IRESPONSE.SXenabled=(m_pWebEm->FindValue("ByronSX")=="on")?1:0;
	Response.IRESPONSE.RFU6=(m_pWebEm->FindValue("rfu6")=="on")?1:0;

	m_pMain->SetRFXCOMHardwaremodes(atoi(idx.c_str()),Response.ICMND.msg1,Response.ICMND.msg2,Response.ICMND.msg3,Response.ICMND.msg4,Response.ICMND.msg5);

	return (char*)m_retstr.c_str();
}


char * CWebServer::SetRego6XXType()
{
	m_retstr="";
	std::string idx=m_pWebEm->FindValue("idx");
	if (idx=="") {
		return (char*)m_retstr.c_str();
	}
	std::vector<std::vector<std::string> > result;
	std::stringstream szQuery;

	szQuery.clear();
	szQuery.str("");
	szQuery << "SELECT Mode1, Mode2, Mode3, Mode4, Mode5 FROM Hardware WHERE (ID=" << idx << ")";
	result=m_pMain->m_sql.query(szQuery.str());
	if (result.size()<1)
		return (char*)m_retstr.c_str();

	m_retstr="/index.html";

    unsigned char currentMode1=atoi(result[0][0].c_str());

	std::string sRego6XXType=m_pWebEm->FindValue("Rego6XXType");
    unsigned char newMode1=atoi(sRego6XXType.c_str());
	
    if(currentMode1 != newMode1)
    {
        m_pMain->m_sql.UpdateRFXCOMHardwareDetails(atoi(idx.c_str()), newMode1, 0, 0, 0, 0);
    }
	
	return (char*)m_retstr.c_str();
}

char * CWebServer::SetS0MeterType()
{
	m_retstr="";
	std::string idx=m_pWebEm->FindValue("idx");
	if (idx=="") {
		return (char*)m_retstr.c_str();
	}

	std::vector<std::vector<std::string> > result;
	std::stringstream szQuery;

	szQuery.clear();
	szQuery.str("");
	szQuery << "SELECT Mode1, Mode2, Mode3, Mode4, Mode5 FROM Hardware WHERE (ID=" << idx << ")";
	result=m_pMain->m_sql.query(szQuery.str());
	if (result.size()<1)
		return (char*)m_retstr.c_str();

	m_retstr="/index.html";

	int Mode1=atoi(m_pWebEm->FindValue("S0M1Type").c_str());
	int Mode2=atoi(m_pWebEm->FindValue("M1PulsesPerHour").c_str());
	int Mode3=atoi(m_pWebEm->FindValue("S0M2Type").c_str());
	int Mode4=atoi(m_pWebEm->FindValue("M2PulsesPerHour").c_str());
	m_pMain->m_sql.UpdateRFXCOMHardwareDetails(atoi(idx.c_str()), Mode1, Mode2, Mode3, Mode4, 0);

	m_pMain->RestartHardware(idx);

	return (char*)m_retstr.c_str();
}

char * CWebServer::SetLimitlessType()
{
	m_retstr="";
	std::string idx=m_pWebEm->FindValue("idx");
	if (idx=="") {
		return (char*)m_retstr.c_str();
	}

	std::vector<std::vector<std::string> > result;
	std::stringstream szQuery;

	szQuery.clear();
	szQuery.str("");
	szQuery << "SELECT Mode1, Mode2, Mode3, Mode4, Mode5 FROM Hardware WHERE (ID=" << idx << ")";
	result=m_pMain->m_sql.query(szQuery.str());
	if (result.size()<1)
		return (char*)m_retstr.c_str();

	m_retstr="/index.html";

	int Mode1=atoi(m_pWebEm->FindValue("LimitlessType").c_str());
	int Mode2=atoi(result[0][1].c_str());
	int Mode3=atoi(result[0][2].c_str());
	int Mode4=atoi(result[0][3].c_str());
	m_pMain->m_sql.UpdateRFXCOMHardwareDetails(atoi(idx.c_str()), Mode1, Mode2, Mode3, Mode4, 0);

	m_pMain->RestartHardware(idx);

	return (char*)m_retstr.c_str();
}

void CWebServer::GetJSonDevices(Json::Value &root, const std::string &rused, const std::string &rfilter, const std::string &order, const std::string &rowid, const std::string &planID)
{
	std::vector<std::vector<std::string> > result;
	std::stringstream szQuery;

	time_t now = mytime(NULL);
	struct tm tm1;
	localtime_r(&now,&tm1);
	int SensorTimeOut=60;
	m_pMain->m_sql.GetPreferencesVar("SensorTimeout", SensorTimeOut);

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
	m_pMain->m_sql.GetPreferencesVar("MobileType",nValue);
	root["MobileType"]=nValue;

	nValue=1;
	m_pMain->m_sql.GetPreferencesVar("5MinuteHistoryDays", nValue);
	root["5MinuteHistoryDays"]=nValue;

	char szData[100];
	char szTmp[300];

	char szOrderBy[50];
	if (order=="")
		strcpy(szOrderBy,"LastUpdate DESC");
	else
	{
		sprintf(szOrderBy,"[Order],%s ASC",order.c_str());
	}

	bool bHaveUser=(m_pWebEm->m_actualuser!="");
	int iUser=-1;
	unsigned int totUserDevices=0;
	if (bHaveUser)
	{
		iUser=FindUser(m_pWebEm->m_actualuser.c_str());
		if (iUser!=-1)
		{
			_eUserRights urights=m_users[iUser].userrights;
			if (urights!=URIGHTS_ADMIN)
			{
				szQuery.clear();
				szQuery.str("");
				szQuery << "SELECT DeviceRowID FROM SharedDevices WHERE (SharedUserID == " << m_users[iUser].ID << ")";
				result=m_pMain->m_sql.query(szQuery.str());
				totUserDevices=(unsigned int)result.size();
			}
		}
	}

	int ii=0;
	if (rfilter=="all")
	{
		//also add scenes
		szQuery.clear();
		szQuery.str("");
		if (rowid!="")
			szQuery << "SELECT ID, Name, nValue, LastUpdate, Favorite, SceneType FROM Scenes WHERE (ID==" << rowid << ")";
		else if ((planID!="")&&(planID!="0"))
			szQuery << "SELECT A.ID, A.Name, A.nValue, A.LastUpdate, A.Favorite, A.SceneType FROM Scenes as A, DeviceToPlansMap as B WHERE (B.PlanID==" << planID << ") AND (B.DeviceRowID==a.ID) AND (B.DevSceneType==1) ORDER BY B.[Order]";
		else
			szQuery << "SELECT ID, Name, nValue, LastUpdate, Favorite, SceneType FROM Scenes ORDER BY " << szOrderBy;
		result=m_pMain->m_sql.query(szQuery.str());
		if (result.size()>0)
		{
			std::vector<std::vector<std::string> >::const_iterator itt;
			for (itt=result.begin(); itt!=result.end(); ++itt)
			{
				std::vector<std::string> sd=*itt;

				int nValue= atoi(sd[2].c_str());
				std::string sLastUpdate=sd[3];
				unsigned char favorite = atoi(sd[4].c_str());
				unsigned char scenetype = atoi(sd[5].c_str());
				if (scenetype==0)
				{
					root["result"][ii]["Type"]="Scene";
				}
				else
				{
					root["result"][ii]["Type"]="Group";
				}
				root["result"][ii]["idx"]=sd[0];
				root["result"][ii]["Name"]=sd[1];
				root["result"][ii]["Favorite"]=favorite;
				root["result"][ii]["LastUpdate"]=sLastUpdate;
				root["result"][ii]["TypeImg"]="lightbulb";
				if (nValue==0)
					root["result"][ii]["Status"]="Off";
				else if (nValue==1)
					root["result"][ii]["Status"]="On";
				else
					root["result"][ii]["Status"]="Mixed";
				unsigned long long camIDX=m_pMain->m_cameras.IsDevSceneInCamera(1,sd[0]);
				root["result"][ii]["UsedByCamera"]=(camIDX!=0)?true:false;
				if (camIDX!=0) {
					root["result"][ii]["CameraFeed"]=m_pMain->m_cameras.GetCameraFeedURL(camIDX);
				}
				ii++;
			}
		}
	}

	szQuery.clear();
	szQuery.str("");

	if (totUserDevices==0)
	{
		//All
		if (rowid!="")
			szQuery << "SELECT ID, DeviceID, Unit, Name, Used, Type, SubType, SignalLevel, BatteryLevel, nValue, sValue, LastUpdate, Favorite, SwitchType, HardwareID, AddjValue, AddjMulti, AddjValue2, AddjMulti2, LastLevel, CustomImage FROM DeviceStatus WHERE (ID==" << rowid << ")";
		else if ((planID!="")&&(planID!="0"))
			szQuery << "SELECT A.ID, A.DeviceID, A.Unit, A.Name, A.Used, A.Type, A.SubType, A.SignalLevel, A.BatteryLevel, A.nValue, A.sValue, A.LastUpdate, A.Favorite, A.SwitchType, A.HardwareID, A.AddjValue, A.AddjMulti, A.AddjValue2, A.AddjMulti2, A.LastLevel, A.CustomImage "
			"FROM DeviceStatus as A, DeviceToPlansMap as B WHERE (B.PlanID==" << planID << ") AND (B.DeviceRowID==a.ID) AND (B.DevSceneType==0) ORDER BY B.[Order]";
		else
			szQuery << "SELECT ID, DeviceID, Unit, Name, Used, Type, SubType, SignalLevel, BatteryLevel, nValue, sValue, LastUpdate, Favorite, SwitchType, HardwareID, AddjValue, AddjMulti, AddjValue2, AddjMulti2, LastLevel, CustomImage FROM DeviceStatus ORDER BY " << szOrderBy;
	}
	else
	{
		//Specific devices
		if (rowid!="")
			szQuery << "SELECT A.ID, A.DeviceID, A.Unit, A.Name, A.Used, A.Type, A.SubType, A.SignalLevel, A.BatteryLevel, A.nValue, A.sValue, A.LastUpdate, A.Favorite, A.SwitchType, A.HardwareID, A.AddjValue, A.AddjMulti, A.AddjValue2, A.AddjMulti2, A.LastLevel, A.CustomImage FROM DeviceStatus as A, SharedDevices as B WHERE (B.DeviceRowID==a.ID) AND (B.SharedUserID==" << m_users[iUser].ID << ") AND (A.ID==" << rowid << ")";
		else if ((planID!="")&&(planID!="0"))
			szQuery << "SELECT A.ID, A.DeviceID, A.Unit, A.Name, A.Used, A.Type, A.SubType, A.SignalLevel, A.BatteryLevel, A.nValue, A.sValue, A.LastUpdate, A.Favorite, A.SwitchType, A.HardwareID, A.AddjValue, A.AddjMulti, A.AddjValue2, A.AddjMulti2, A.LastLevel, A.CustomImage "
			"FROM DeviceStatus as A, SharedDevices as B, DeviceToPlansMap as C  WHERE (C.PlanID==" << planID << ") AND (C.DeviceRowID==a.ID) AND (B.DeviceRowID==a.ID) AND (B.SharedUserID==" << m_users[iUser].ID << ") ORDER BY C.[Order]";
		else
			szQuery << "SELECT A.ID, A.DeviceID, A.Unit, A.Name, A.Used, A.Type, A.SubType, A.SignalLevel, A.BatteryLevel, A.nValue, A.sValue, A.LastUpdate, A.Favorite, A.SwitchType, A.HardwareID, A.AddjValue, A.AddjMulti, A.AddjValue2, A.AddjMulti2, A.LastLevel, A.CustomImage FROM DeviceStatus as A, SharedDevices as B WHERE (B.DeviceRowID==a.ID) AND (B.SharedUserID==" << m_users[iUser].ID << ") ORDER BY " << szOrderBy;
	}

	result=m_pMain->m_sql.query(szQuery.str());
	if (result.size()>0)
	{
		std::vector<std::vector<std::string> >::const_iterator itt;
		for (itt=result.begin(); itt!=result.end(); ++itt)
		{
			std::vector<std::string> sd=*itt;

			unsigned char dType=atoi(sd[5].c_str());
			unsigned char dSubType=atoi(sd[6].c_str());
			unsigned char used = atoi(sd[4].c_str());
			int nValue = atoi(sd[9].c_str());
			std::string sValue=sd[10];
			std::string sLastUpdate=sd[11];
			unsigned char favorite = atoi(sd[12].c_str());
			if ((planID!="")&&(planID!="0"))
				favorite=1;
			_eSwitchType switchtype=(_eSwitchType) atoi(sd[13].c_str());
			_eMeterType metertype=(_eMeterType)switchtype;
			int hardwareID= atoi(sd[14].c_str());
			double AddjValue=atof(sd[15].c_str());
			double AddjMulti=atof(sd[16].c_str());
			double AddjValue2=atof(sd[17].c_str());
			double AddjMulti2=atof(sd[18].c_str());
			int LastLevel=atoi(sd[19].c_str());
			int CustomImage=atoi(sd[20].c_str());

			struct tm ntime;
			ntime.tm_isdst=tm1.tm_isdst;
			ntime.tm_year=atoi(sLastUpdate.substr(0,4).c_str())-1900;
			ntime.tm_mon=atoi(sLastUpdate.substr(5,2).c_str())-1;
			ntime.tm_mday=atoi(sLastUpdate.substr(8,2).c_str());
			ntime.tm_hour=atoi(sLastUpdate.substr(11,2).c_str());
			ntime.tm_min=atoi(sLastUpdate.substr(14,2).c_str());
			ntime.tm_sec=atoi(sLastUpdate.substr(17,2).c_str());
			time_t checktime=mktime(&ntime);
			bool bHaveTimeout=(now-checktime>=SensorTimeOut*60);

			if (dType==pTypeTEMP_RAIN)
				continue; //dont want you for now

			if ((rused=="true")&&(!used))
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
						(dType!=pTypeLimitlessLights)&&
						(dType!=pTypeSecurity1)&&
						(dType!=pTypeBlinds)&&
						(dType!=pTypeChime)&&
						(!((dType==pTypeRego6XXValue)&&(dSubType==sTypeRego6XXStatus)))
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
						(dType!=pTypeTEMP_BARO)&&
						(!((dType==pTypeWIND)&&(dSubType==sTypeWIND4)))&&
						(!((dType==pTypeWIND)&&(dSubType==sTypeWINDNoTemp)))&&
						(!((dType==pTypeUV)&&(dSubType==sTypeUV3)))&&
						(dType!=pTypeThermostat1)&&
						(!((dType==pTypeRFXSensor)&&(dSubType==sTypeRFXSensorTemp)))&&
						(dType!=pTypeRego6XXTemp)
						)
						continue;
				}
				else if (rfilter=="weather")
				{
					if (
						(dType!=pTypeWIND)&&
						(dType!=pTypeRAIN)&&
						(dType!=pTypeTEMP_HUM_BARO)&&
						(dType!=pTypeTEMP_BARO)&&
						(dType!=pTypeUV)&&
						(!((dType==pTypeGeneral)&&(dSubType==sTypeVisibility)))&&
						(!((dType==pTypeGeneral)&&(dSubType==sTypeSolarRadiation)))
						)
						continue;
				}
				else if (rfilter=="utility")
				{
					if (
						(dType!=pTypeRFXMeter)&&
						(!((dType==pTypeRFXSensor)&&(dSubType==sTypeRFXSensorAD)))&&
						(!((dType==pTypeRFXSensor)&&(dSubType==sTypeRFXSensorVolt)))&&
						(dType!=pTypeCURRENT)&&
						(dType!=pTypeCURRENTENERGY)&&
						(dType!=pTypeENERGY)&&
						(dType!=pTypePOWER)&&
						(dType!=pTypeP1Power)&&
						(dType!=pTypeP1Gas)&&
						(dType!=pTypeYouLess)&&
						(dType!=pTypeAirQuality)&&
						(!((dType==pTypeGeneral)&&(dSubType==sTypeSoilMoisture)))&&
						(!((dType==pTypeGeneral)&&(dSubType==sTypeLeafWetness)))&&
						(dType!=pTypeLux)&&
						(dType!=pTypeUsage)&&
						(!((dType==pTypeRego6XXValue)&&(dSubType==sTypeRego6XXCounter)))&&
						(!((dType==pTypeThermostat)&&(dSubType==sTypeThermSetpoint)))&&
						(dType!=pTypeWEIGHT)
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
						(dType!=pTypeTEMP_HUM_BARO)&&
						(dType!=pTypeTEMP_BARO)
						)
						continue;
				}
			}

			root["result"][ii]["HardwareID"]=hardwareID;
			if (hardwareID==1000)
			{
				root["result"][ii]["HardwareName"]="System";
			}
			else
			{
				if (_hardwareNames.find(hardwareID)==_hardwareNames.end())
					root["result"][ii]["HardwareName"]="Unknown?";
				else
					root["result"][ii]["HardwareName"]=_hardwareNames[hardwareID];
			}
			root["result"][ii]["idx"]=sd[0];
			sprintf(szData,"%04X",(unsigned int)atoi(sd[1].c_str()));
			if (
				(dType==pTypeTEMP_BARO)||
				(dType==pTypeTEMP)||
				(dType==pTypeTEMP_HUM)||
				(dType==pTypeTEMP_HUM_BARO)||
				(dType==pTypeBARO)||
				(dType==pTypeHUM)
				)
			{
				root["result"][ii]["ID"]=szData;
			}
			else
				root["result"][ii]["ID"]=sd[1];
			root["result"][ii]["Unit"]=atoi(sd[2].c_str());
			root["result"][ii]["Type"]=RFX_Type_Desc(dType,1);
			root["result"][ii]["SubType"]=RFX_Type_SubType_Desc(dType,dSubType);
			root["result"][ii]["TypeImg"]=RFX_Type_Desc(dType,2);
			root["result"][ii]["Name"]=sd[3];
			root["result"][ii]["Used"]=used;
			root["result"][ii]["Favorite"]=favorite;
			root["result"][ii]["SignalLevel"]=atoi(sd[7].c_str());
			root["result"][ii]["BatteryLevel"]=atoi(sd[8].c_str());
			root["result"][ii]["LastUpdate"]=sLastUpdate;
			root["result"][ii]["CustomImage"]=CustomImage;

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
				(dType==pTypeLighting6)||
				(dType==pTypeLimitlessLights)||
				(dType==pTypeBlinds)||
				(dType==pTypeChime)
				)
			{
				//add light details
				std::string lstatus="";
				int llevel=0;
				bool bHaveDimmer=false;
				bool bHaveGroupCmd=false;
				int maxDimLevel=0;

				GetLightStatus(dType,dSubType,nValue,sValue,lstatus,llevel,bHaveDimmer,maxDimLevel,bHaveGroupCmd);

				root["result"][ii]["Status"]=lstatus;

				if (CustomImage<(int)m_custom_light_icons.size())
					root["result"][ii]["Image"]=m_custom_light_icons[CustomImage].RootFile;
				else
					root["result"][ii]["Image"]="Light";
				

				if (switchtype==STYPE_Dimmer)
				{
					root["result"][ii]["Level"]=LastLevel;
					int iLevel=round((float(maxDimLevel)/100.0f)*LastLevel);
					root["result"][ii]["LevelInt"]=iLevel;
				}
				else
				{
					root["result"][ii]["Level"]=llevel;
					root["result"][ii]["LevelInt"]=atoi(sValue.c_str());
				}
				root["result"][ii]["HaveDimmer"]=bHaveDimmer;
				root["result"][ii]["MaxDimLevel"]=maxDimLevel;
				root["result"][ii]["HaveGroupCmd"]=bHaveGroupCmd;
				root["result"][ii]["SwitchType"]=Switch_Type_Desc(switchtype);
				root["result"][ii]["SwitchTypeVal"]=switchtype;
				unsigned long long camIDX=m_pMain->m_cameras.IsDevSceneInCamera(0,sd[0]);
				root["result"][ii]["UsedByCamera"]=(camIDX!=0)?true:false;
				if (camIDX!=0) {
					root["result"][ii]["CameraFeed"]=m_pMain->m_cameras.GetCameraFeedURL(camIDX);
				}

				bool bIsSubDevice=false;
				std::vector<std::vector<std::string> > resultSD;
				std::stringstream szQuerySD;

				szQuerySD.clear();
				szQuerySD.str("");
				szQuerySD << "SELECT ID FROM LightSubDevices WHERE (DeviceRowID=='" << sd[0] << "')";
				resultSD=m_pMain->m_sql.query(szQuerySD.str());
				bIsSubDevice=(resultSD.size()>0);

				root["result"][ii]["IsSubDevice"]=bIsSubDevice;
				
				if (switchtype==STYPE_OnOff)
				{
					root["result"][ii]["AddjValue"]=AddjValue;
					root["result"][ii]["AddjMulti"]=AddjMulti;
					root["result"][ii]["AddjValue2"]=AddjValue2;
					root["result"][ii]["AddjMulti2"]=AddjMulti2;
				}
				else if (switchtype==STYPE_Doorbell)
				{
					root["result"][ii]["TypeImg"]="doorbell";
					root["result"][ii]["Status"]="";//"Pressed";
				}
				else if (switchtype==STYPE_DoorLock)
				{
					root["result"][ii]["TypeImg"]="door";
					root["result"][ii]["InternalState"]=(IsLightSwitchOn(lstatus)==true)?"Open":"Closed";
					if (lstatus=="On") {
						lstatus="Open";
					} else {
						lstatus="Closed";
					}
					root["result"][ii]["Status"]=lstatus;
					root["result"][ii]["AddjValue"]=AddjValue;
					root["result"][ii]["AddjMulti"]=AddjMulti;
					root["result"][ii]["AddjValue2"]=AddjValue2;
					root["result"][ii]["AddjMulti2"]=AddjMulti2;
				}
				else if (switchtype==STYPE_PushOn)
				{
					root["result"][ii]["TypeImg"]="push";
					root["result"][ii]["InternalState"]=(IsLightSwitchOn(lstatus)==true)?"On":"Off";
					root["result"][ii]["AddjValue"]=AddjValue;
					root["result"][ii]["AddjMulti"]=AddjMulti;
					root["result"][ii]["AddjValue2"]=AddjValue2;
					root["result"][ii]["AddjMulti2"]=AddjMulti2;
				}
				else if (switchtype==STYPE_PushOff)
				{
					root["result"][ii]["TypeImg"]="pushoff";
				}
				else if (switchtype==STYPE_X10Siren)
					root["result"][ii]["TypeImg"]="siren";
				else if (switchtype==STYPE_SMOKEDETECTOR)
				{
					root["result"][ii]["TypeImg"]="smoke";
					root["result"][ii]["SwitchTypeVal"]=STYPE_SMOKEDETECTOR;
					root["result"][ii]["SwitchType"] =Switch_Type_Desc(STYPE_SMOKEDETECTOR);
					root["result"][ii]["AddjValue"]=AddjValue;
					root["result"][ii]["AddjMulti"]=AddjMulti;
					root["result"][ii]["AddjValue2"]=AddjValue2;
					root["result"][ii]["AddjMulti2"]=AddjMulti2;
				}
				else if (switchtype==STYPE_Contact)
				{
					root["result"][ii]["TypeImg"]="contact";
					if (lstatus=="On") {
						lstatus="Open";
					} else {
						lstatus="Closed";
					}
					root["result"][ii]["Status"]=lstatus;
					root["result"][ii]["AddjValue"]=AddjValue;
					root["result"][ii]["AddjMulti"]=AddjMulti;
					root["result"][ii]["AddjValue2"]=AddjValue2;
					root["result"][ii]["AddjMulti2"]=AddjMulti2;
				}
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
				else if (switchtype==STYPE_BlindsInverted)
				{
					root["result"][ii]["TypeImg"]="blinds";
					if (lstatus=="On") {
						lstatus="Open";
					} else {
						lstatus="Closed";
					}
					root["result"][ii]["Status"]=lstatus;
				}
				else if (switchtype==STYPE_Dimmer)
				{
					root["result"][ii]["TypeImg"]="dimmer";
				}
				else if (switchtype==STYPE_Motion)
				{
					root["result"][ii]["TypeImg"]="motion";
					root["result"][ii]["AddjValue"]=AddjValue;
					root["result"][ii]["AddjMulti"]=AddjMulti;
					root["result"][ii]["AddjValue2"]=AddjValue2;
					root["result"][ii]["AddjMulti2"]=AddjMulti2;
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
				int maxDimLevel=0;

				GetLightStatus(dType,dSubType,nValue,sValue,lstatus,llevel,bHaveDimmer,maxDimLevel,bHaveGroupCmd);

				root["result"][ii]["Status"]=lstatus;
				root["result"][ii]["HaveDimmer"]=bHaveDimmer;
				root["result"][ii]["MaxDimLevel"]=maxDimLevel;
				root["result"][ii]["HaveGroupCmd"]=bHaveGroupCmd;
                root["result"][ii]["SwitchType"]="Security";
				root["result"][ii]["SwitchTypeVal"]=switchtype; //was 0?;
				root["result"][ii]["TypeImg"]="security";
				if ((dSubType==sTypeKD101) || (dSubType==sTypeSA30) || (switchtype == STYPE_SMOKEDETECTOR)) 
				{
					root["result"][ii]["SwitchTypeVal"]=STYPE_SMOKEDETECTOR;
					root["result"][ii]["TypeImg"]="smoke";
					root["result"][ii]["SwitchType"] =Switch_Type_Desc(STYPE_SMOKEDETECTOR);
					root["result"][ii]["AddjValue"]=AddjValue;
					root["result"][ii]["AddjMulti"]=AddjMulti;
					root["result"][ii]["AddjValue2"]=AddjValue2;
					root["result"][ii]["AddjMulti2"]=AddjMulti2;
				}
				if (switchtype == STYPE_Motion)
				{
					root["result"][ii]["AddjValue"]=AddjValue;
					root["result"][ii]["AddjMulti"]=AddjMulti;
					root["result"][ii]["AddjValue2"]=AddjValue2;
					root["result"][ii]["AddjMulti2"]=AddjMulti2;
				}

				sprintf(szData,"%s", lstatus.c_str());
				root["result"][ii]["Data"]=szData;
			}
			else if ((dType == pTypeTEMP) || (dType == pTypeRego6XXTemp))
			{
				root["result"][ii]["AddjValue"]=AddjValue;
				root["result"][ii]["AddjMulti"]=AddjMulti;
				root["result"][ii]["Temp"]=atof(sValue.c_str());
				sprintf(szData,"%.1f C", atof(sValue.c_str()));
				root["result"][ii]["Data"]=szData;
				root["result"][ii]["HaveTimeout"]=bHaveTimeout;
			}
			else if (dType == pTypeThermostat1)
			{
				std::vector<std::string> strarray;
				StringSplit(sValue, ";", strarray);
				if (strarray.size()==4)
				{
					root["result"][ii]["Temp"]=atoi(strarray[0].c_str());
					root["result"][ii]["HaveTimeout"]=bHaveTimeout;
				}
			}
			else if ((dType==pTypeRFXSensor)&&(dSubType==sTypeRFXSensorTemp))
			{
				root["result"][ii]["AddjValue"]=AddjValue;
				root["result"][ii]["AddjMulti"]=AddjMulti;
				root["result"][ii]["AddjValue2"]=AddjValue2;
				root["result"][ii]["AddjMulti2"]=AddjMulti2;
				root["result"][ii]["Temp"]=atof(sValue.c_str());
				sprintf(szData,"%.1f C", atof(sValue.c_str()));
				root["result"][ii]["Data"]=szData;
				root["result"][ii]["TypeImg"]="temperature";
				root["result"][ii]["HaveTimeout"]=bHaveTimeout;
			}
			else if (dType == pTypeHUM)
			{
				root["result"][ii]["Humidity"]=nValue;
				root["result"][ii]["HumidityStatus"]=RFX_Humidity_Status_Desc(atoi(sValue.c_str()));
				sprintf(szData,"Humidity %d %%", nValue);
				root["result"][ii]["Data"]=szData;
				root["result"][ii]["HaveTimeout"]=bHaveTimeout;
			}
			else if (dType == pTypeTEMP_HUM)
			{
				std::vector<std::string> strarray;
				StringSplit(sValue, ";", strarray);
				if (strarray.size()==3)
				{
					root["result"][ii]["AddjValue"]=AddjValue;
					root["result"][ii]["AddjMulti"]=AddjMulti;
					root["result"][ii]["AddjValue2"]=AddjValue2;
					root["result"][ii]["AddjMulti2"]=AddjMulti2;

					double temp=atof(strarray[0].c_str());
					int humidity=atoi(strarray[1].c_str());

					root["result"][ii]["Temp"]=temp;
					root["result"][ii]["Humidity"]=humidity;
					root["result"][ii]["HumidityStatus"]=RFX_Humidity_Status_Desc(atoi(strarray[2].c_str()));
					sprintf(szData,"%.1f C, %d %%", atof(strarray[0].c_str()),atoi(strarray[1].c_str()));
					root["result"][ii]["Data"]=szData;
					root["result"][ii]["HaveTimeout"]=bHaveTimeout;

					//Calculate dew point

					sprintf(szTmp,"%.2f",CalculateDewPoint(temp,humidity));
					root["result"][ii]["DewPoint"]=szTmp;
				}
			}
			else if (dType == pTypeTEMP_HUM_BARO)
			{
				std::vector<std::string> strarray;
				StringSplit(sValue, ";", strarray);
				if (strarray.size()==5)
				{
					root["result"][ii]["AddjValue"]=AddjValue;
					root["result"][ii]["AddjMulti"]=AddjMulti;
					root["result"][ii]["AddjValue2"]=AddjValue2;
					root["result"][ii]["AddjMulti2"]=AddjMulti2;

					double temp=atof(strarray[0].c_str());
					int humidity=atoi(strarray[1].c_str());

					root["result"][ii]["Temp"]=temp;
					root["result"][ii]["Humidity"]=humidity;
					root["result"][ii]["HumidityStatus"]=RFX_Humidity_Status_Desc(atoi(strarray[2].c_str()));
					root["result"][ii]["Forecast"]=atoi(strarray[4].c_str());

					sprintf(szTmp,"%.2f",CalculateDewPoint(temp,humidity));
					root["result"][ii]["DewPoint"]=szTmp;

					if (dSubType==sTypeTHBFloat)
					{
						root["result"][ii]["Barometer"]=atof(strarray[3].c_str());
						root["result"][ii]["ForecastStr"]=RFX_WSForecast_Desc(atoi(strarray[4].c_str()));
					}
					else
					{
						root["result"][ii]["Barometer"]=atoi(strarray[3].c_str());
						root["result"][ii]["ForecastStr"]=RFX_Forecast_Desc(atoi(strarray[4].c_str()));
					}
					if (dSubType==sTypeTHBFloat)
					{
						sprintf(szData,"%.1f C, %d %%, %.1f hPa",
							atof(strarray[0].c_str()),
							atoi(strarray[1].c_str()),
							atof(strarray[3].c_str())
							);
					}
					else
					{
						sprintf(szData,"%.1f C, %d %%, %d hPa",
							atof(strarray[0].c_str()),
							atoi(strarray[1].c_str()),
							atoi(strarray[3].c_str())
							);
					}
					root["result"][ii]["Data"]=szData;
					root["result"][ii]["HaveTimeout"]=bHaveTimeout;
				}
			}
			else if (dType == pTypeTEMP_BARO)
			{
				std::vector<std::string> strarray;
				StringSplit(sValue, ";", strarray);
				if (strarray.size()>=3)
				{
					root["result"][ii]["AddjValue"]=AddjValue;
					root["result"][ii]["AddjMulti"]=AddjMulti;
					root["result"][ii]["AddjValue2"]=AddjValue2;
					root["result"][ii]["AddjMulti2"]=AddjMulti2;
					root["result"][ii]["Temp"]=atof(strarray[0].c_str());
					int forecast=atoi(strarray[2].c_str());
					if (forecast!=baroForecastNoInfo)
					{
						root["result"][ii]["Forecast"]=forecast;
						root["result"][ii]["ForecastStr"]=RFX_Forecast_Desc(forecast);
					}
					root["result"][ii]["Barometer"]=atof(strarray[1].c_str());

					sprintf(szData,"%.1f C, %.1f hPa",
							atof(strarray[0].c_str()),
							atof(strarray[1].c_str())
							);
					root["result"][ii]["Data"]=szData;
					root["result"][ii]["HaveTimeout"]=bHaveTimeout;
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
					if (dSubType==sTypeUV3)
					{
						root["result"][ii]["AddjValue"]=AddjValue;
						root["result"][ii]["AddjMulti"]=AddjMulti;
						root["result"][ii]["AddjValue2"]=AddjValue2;
						root["result"][ii]["AddjMulti2"]=AddjMulti2;
						root["result"][ii]["Temp"]=strarray[1];
						sprintf(szData,"%.1f UVI, %.1f&deg; C",UVI,Temp);
					}
					else
					{
						sprintf(szData,"%.1f UVI",UVI);
					}
					root["result"][ii]["Data"]=szData;
					root["result"][ii]["HaveTimeout"]=bHaveTimeout;
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

					if (dSubType!=sTypeWIND5)
					{
						int intSpeed=atoi(strarray[2].c_str());
						sprintf(szTmp,"%.1f",float(intSpeed) * m_pMain->m_sql.m_windscale);
						root["result"][ii]["Speed"]=szTmp;
					}

					//if (dSubType!=sTypeWIND6) //problem in RFXCOM firmware? gust=speed?
					{
						int intGust=atoi(strarray[3].c_str());
						sprintf(szTmp,"%.1f",float(intGust) *m_pMain->m_sql.m_windscale);
						root["result"][ii]["Gust"]=szTmp;
					}
					if ((dSubType==sTypeWIND4)||(dSubType==sTypeWINDNoTemp))
					{
						root["result"][ii]["AddjValue"]=AddjValue;
						root["result"][ii]["AddjMulti"]=AddjMulti;
						root["result"][ii]["AddjValue2"]=AddjValue2;
						root["result"][ii]["AddjMulti2"]=AddjMulti2;
						if (dSubType==sTypeWIND4)
						{
							root["result"][ii]["Temp"]=atof(strarray[4].c_str());
						}
						root["result"][ii]["Chill"]=atof(strarray[5].c_str());
					}
					root["result"][ii]["Data"]=sValue;
					root["result"][ii]["HaveTimeout"]=bHaveTimeout;
				}
			}
			else if (dType == pTypeRAIN)
			{
				std::vector<std::string> strarray;
				StringSplit(sValue, ";", strarray);
				if (strarray.size()==2)
				{
					//get lowest value of today, and max rate
					time_t now = mytime(NULL);
					struct tm tm1;
					localtime_r(&now,&tm1);

					struct tm ltime;
					ltime.tm_isdst=tm1.tm_isdst;
					ltime.tm_hour=0;
					ltime.tm_min=0;
					ltime.tm_sec=0;
					ltime.tm_year=tm1.tm_year;
					ltime.tm_mon=tm1.tm_mon;
					ltime.tm_mday=tm1.tm_mday;

					char szDate[40];
					sprintf(szDate,"%04d-%02d-%02d",ltime.tm_year+1900,ltime.tm_mon+1,ltime.tm_mday);

					std::vector<std::vector<std::string> > result2;

					szQuery.clear();
					szQuery.str("");
					if (dSubType!=sTypeRAINWU)
					{
						szQuery << "SELECT MIN(Total), MAX(Total), MAX(Rate) FROM Rain WHERE (DeviceRowID=" << sd[0] << " AND Date>='" << szDate << "')";
					}
					else
					{
						szQuery << "SELECT Total, Total, Rate FROM Rain WHERE (DeviceRowID=" << sd[0] << " AND Date>='" << szDate << "') ORDER BY ROWID DESC LIMIT 1";
					}
					result2=m_pMain->m_sql.query(szQuery.str());
					if (result2.size()>0)
					{
						root["result"][ii]["AddjValue"]=AddjValue;
						root["result"][ii]["AddjMulti"]=AddjMulti;
						root["result"][ii]["AddjValue2"]=AddjValue2;
						root["result"][ii]["AddjMulti2"]=AddjMulti2;

						double total_real=0;
						float rate=0;
						std::vector<std::string> sd2=result2[0];
						if (dSubType!=sTypeRAINWU)
						{
							float total_min=(float)atof(sd2[0].c_str());
							float total_max=(float)atof(strarray[1].c_str());
							rate=(float)atof(sd2[2].c_str());
							if (dSubType==sTypeRAIN2)
								rate/=100.0f;
							total_real=total_max-total_min;
						}
						else
						{
							rate=(float)atof(sd2[2].c_str());
							total_real=atof(sd2[1].c_str());
						}
						total_real*=AddjMulti;

						sprintf(szTmp,"%.1f",total_real);
						root["result"][ii]["Rain"]=szTmp;
						if (dSubType!=sTypeRAIN2)
							sprintf(szTmp,"%.1f",rate);
						else
							sprintf(szTmp,"%.2f",rate);
						root["result"][ii]["RainRate"]=szTmp;
						root["result"][ii]["Data"]=sValue;
						root["result"][ii]["HaveTimeout"]=bHaveTimeout;
					}
					else
					{
						root["result"][ii]["Rain"]="0.0";
						root["result"][ii]["RainRate"]="0.0";
						root["result"][ii]["Data"]="0.0";
						root["result"][ii]["HaveTimeout"]=bHaveTimeout;
					}
				}
			}
			else if (dType == pTypeRFXMeter)
			{
				float EnergyDivider=1000.0f;
				float GasDivider=100.0f;
				float WaterDivider=100.0f;
				int tValue;
				if (m_pMain->m_sql.GetPreferencesVar("MeterDividerEnergy", tValue))
				{
					EnergyDivider=float(tValue);
				}
				if (m_pMain->m_sql.GetPreferencesVar("MeterDividerGas", tValue))
				{
					GasDivider=float(tValue);
				}
				if (m_pMain->m_sql.GetPreferencesVar("MeterDividerWater", tValue))
				{
					WaterDivider=float(tValue);
				}

				//get value of today
				time_t now = mytime(NULL);
				struct tm tm1;
				localtime_r(&now,&tm1);

				struct tm ltime;
				ltime.tm_isdst=tm1.tm_isdst;
				ltime.tm_hour=0;
				ltime.tm_min=0;
				ltime.tm_sec=0;
				ltime.tm_year=tm1.tm_year;
				ltime.tm_mon=tm1.tm_mon;
				ltime.tm_mday=tm1.tm_mday;

				char szDate[40];
				sprintf(szDate,"%04d-%02d-%02d",ltime.tm_year+1900,ltime.tm_mon+1,ltime.tm_mday);

				std::vector<std::vector<std::string> > result2;
				strcpy(szTmp,"0");
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
						musage=float(total_real)/EnergyDivider;
						sprintf(szTmp,"%.03f kWh",musage);
						break;
					case MTYPE_GAS:
						musage=float(total_real)/GasDivider;
						sprintf(szTmp,"%.02f m3",musage);
						break;
					case MTYPE_WATER:
						musage=float(total_real)/WaterDivider;
						sprintf(szTmp,"%.02f m3",musage);
						break;
					case MTYPE_COUNTER:
						sprintf(szTmp,"%llu",total_real);
						break;
					}
				}
				root["result"][ii]["Counter"]=sValue;
				root["result"][ii]["CounterToday"]=szTmp;
				root["result"][ii]["SwitchTypeVal"]=metertype;
				root["result"][ii]["HaveTimeout"]=bHaveTimeout;
				//root["result"][ii]["Data"]=sValue;
			}
			else if (dType == pTypeYouLess)
			{
				float EnergyDivider=1000.0f;
				float GasDivider=100.0f;
				float WaterDivider=100.0f;
				float musage=0;
				int tValue;
				if (m_pMain->m_sql.GetPreferencesVar("MeterDividerEnergy", tValue))
				{
					EnergyDivider=float(tValue);
				}
				if (m_pMain->m_sql.GetPreferencesVar("MeterDividerGas", tValue))
				{
					GasDivider=float(tValue);
				}
				if (m_pMain->m_sql.GetPreferencesVar("MeterDividerWater", tValue))
				{
					WaterDivider=float(tValue);
				}

				//get value of today
				time_t now = mytime(NULL);
				struct tm tm1;
				localtime_r(&now,&tm1);

				struct tm ltime;
				ltime.tm_isdst=tm1.tm_isdst;
				ltime.tm_hour=0;
				ltime.tm_min=0;
				ltime.tm_sec=0;
				ltime.tm_year=tm1.tm_year;
				ltime.tm_mon=tm1.tm_mon;
				ltime.tm_mday=tm1.tm_mday;

				char szDate[40];
				sprintf(szDate,"%04d-%02d-%02d",ltime.tm_year+1900,ltime.tm_mon+1,ltime.tm_mday);

				std::vector<std::vector<std::string> > result2;
				strcpy(szTmp,"0");
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

					musage=0;
					switch (metertype)
					{
					case MTYPE_ENERGY:
						musage=float(total_real)/EnergyDivider;
						sprintf(szTmp,"%.03f kWh",musage);
						break;
					case MTYPE_GAS:
						musage=float(total_real)/GasDivider;
						sprintf(szTmp,"%.02f m3",musage);
						break;
					case MTYPE_WATER:
						musage=float(total_real)/WaterDivider;
						sprintf(szTmp,"%.02f m3",musage);
						break;
					case MTYPE_COUNTER:
						sprintf(szTmp,"%llu",total_real);
						break;
					}
				}
				root["result"][ii]["CounterToday"]=szTmp;

				std::vector<std::string> splitresults;
				StringSplit(sValue, ";", splitresults);

				unsigned long long total_actual;
				std::stringstream s_stra( splitresults[0]);
				s_stra >> total_actual;
				musage=0;
				switch (metertype)
				{
				case MTYPE_ENERGY:
					musage=float(total_actual)/EnergyDivider;
					sprintf(szTmp,"%.03f",musage);
					break;
				case MTYPE_GAS:
				case MTYPE_WATER:
					musage=float(total_actual)/GasDivider;
					sprintf(szTmp,"%.02f",musage);
					break;
				case MTYPE_COUNTER:
					sprintf(szTmp,"%llu",total_actual);
					break;
				}
				root["result"][ii]["Counter"]=szTmp;

				root["result"][ii]["SwitchTypeVal"]=metertype;

				unsigned long long acounter;
				std::stringstream s_str3( sValue );
				s_str3 >> acounter;
				musage=0;
				switch (metertype)
				{
				case MTYPE_ENERGY:
					musage=float(acounter)/EnergyDivider;
					sprintf(szTmp,"%.03f kWh %s Watt",musage,splitresults[1].c_str());
					break;
				case MTYPE_GAS:
					musage=float(acounter)/GasDivider;
					sprintf(szTmp,"%.02f m3",musage);
					break;
				case MTYPE_WATER:
					musage=float(acounter)/WaterDivider;
					sprintf(szTmp,"%.02f m3",musage);
					break;
				case MTYPE_COUNTER:
					sprintf(szTmp,"%llu",acounter);
					break;
				}
				root["result"][ii]["Data"]=szTmp;
				switch (metertype)
				{
				case MTYPE_ENERGY:
					sprintf(szTmp,"%s Watt",splitresults[1].c_str());
					break;
				case MTYPE_GAS:
					sprintf(szTmp,"%s m",splitresults[1].c_str());
					break;
				case MTYPE_WATER:
					sprintf(szTmp,"%s m",splitresults[1].c_str());
					break;
				case MTYPE_COUNTER:
					sprintf(szTmp,"%s",splitresults[1].c_str());
					break;
				}

				root["result"][ii]["Usage"]=szTmp;
				root["result"][ii]["HaveTimeout"]=bHaveTimeout;
			}
			else if (dType == pTypeP1Power)
			{
				std::vector<std::string> splitresults;
				StringSplit(sValue, ";", splitresults);
				if (splitresults.size()!=6)
					continue; //impossible

				float EnergyDivider=1000.0f;
				int tValue;
				if (m_pMain->m_sql.GetPreferencesVar("MeterDividerEnergy", tValue))
				{
					EnergyDivider=float(tValue);
				}

				unsigned long long powerusage1;
				unsigned long long powerusage2;
				unsigned long long powerdeliv1;
				unsigned long long powerdeliv2;
				unsigned long long usagecurrent;
				unsigned long long delivcurrent;

				std::stringstream s_powerusage1(splitresults[0]);
				std::stringstream s_powerusage2(splitresults[1]);
				std::stringstream s_powerdeliv1(splitresults[2]);
				std::stringstream s_powerdeliv2(splitresults[3]);
				std::stringstream s_usagecurrent(splitresults[4]);
				std::stringstream s_delivcurrent(splitresults[5]);

				s_powerusage1 >> powerusage1;
				s_powerusage2 >> powerusage2;
				s_powerdeliv1 >> powerdeliv1;
				s_powerdeliv2 >> powerdeliv2;
				s_usagecurrent >> usagecurrent;
				s_delivcurrent >> delivcurrent;

				unsigned long long powerusage=powerusage1+powerusage2;
				unsigned long long powerdeliv=powerdeliv1+powerdeliv2;

				float musage=0;

				root["result"][ii]["SwitchTypeVal"]=MTYPE_ENERGY;
				musage=float(powerusage)/EnergyDivider;
				sprintf(szTmp,"%.03f",musage);
				root["result"][ii]["Counter"]=szTmp;
				musage=float(powerdeliv)/EnergyDivider;
				sprintf(szTmp,"%.03f",musage);
				root["result"][ii]["CounterDeliv"]=szTmp;

				if (bHaveTimeout)
				{
					usagecurrent=0;
					delivcurrent=0;
				}
				sprintf(szTmp,"%llu Watt",usagecurrent);
				root["result"][ii]["Usage"]=szTmp;
				sprintf(szTmp,"%llu Watt",delivcurrent);
				root["result"][ii]["UsageDeliv"]=szTmp;
				root["result"][ii]["Data"]=sValue;
				root["result"][ii]["HaveTimeout"]=bHaveTimeout;

				//get value of today
				time_t now = mytime(NULL);
				struct tm tm1;
				localtime_r(&now,&tm1);

				struct tm ltime;
				ltime.tm_isdst=tm1.tm_isdst;
				ltime.tm_hour=0;
				ltime.tm_min=0;
				ltime.tm_sec=0;
				ltime.tm_year=tm1.tm_year;
				ltime.tm_mon=tm1.tm_mon;
				ltime.tm_mday=tm1.tm_mday;

				char szDate[40];
				sprintf(szDate,"%04d-%02d-%02d",ltime.tm_year+1900,ltime.tm_mon+1,ltime.tm_mday);

				std::vector<std::vector<std::string> > result2;
				strcpy(szTmp,"0");
				szQuery.clear();
				szQuery.str("");
				szQuery << "SELECT MIN(Value1), MIN(Value2), MIN(Value5), MIN(Value6) FROM MultiMeter WHERE (DeviceRowID=" << sd[0] << " AND Date>='" << szDate << "')";
				result2=m_pMain->m_sql.query(szQuery.str());
				if (result2.size()>0)
				{
					std::vector<std::string> sd2=result2[0];

					unsigned long long total_min_usage_1,total_min_usage_2,total_real_usage;
					unsigned long long total_min_deliv_1,total_min_deliv_2,total_real_deliv;

					std::stringstream s_str1( sd2[0] );
					s_str1 >> total_min_usage_1;
					std::stringstream s_str2( sd2[1] );
					s_str2 >> total_min_deliv_1;
					std::stringstream s_str3( sd2[2] );
					s_str3 >> total_min_usage_2;
					std::stringstream s_str4( sd2[3] );
					s_str4 >> total_min_deliv_2;

					total_real_usage=powerusage-(total_min_usage_1+total_min_usage_2);
					total_real_deliv=powerdeliv-(total_min_deliv_1+total_min_deliv_2);

					musage=float(total_real_usage)/EnergyDivider;
					sprintf(szTmp,"%.03f kWh",musage);
					root["result"][ii]["CounterToday"]=szTmp;
					musage=float(total_real_deliv)/EnergyDivider;
					sprintf(szTmp,"%.03f kWh",musage);
					root["result"][ii]["CounterDelivToday"]=szTmp;
				}
				else
				{
					sprintf(szTmp,"%.03f kWh",0.0f);
					root["result"][ii]["CounterToday"]=szTmp;
					root["result"][ii]["CounterDelivToday"]=szTmp;
				}
			}
			else if (dType == pTypeP1Gas)
			{
				float GasDivider=1000.0f;
				//get lowest value of today
				time_t now = mytime(NULL);
				struct tm tm1;
				localtime_r(&now,&tm1);

				struct tm ltime;
				ltime.tm_isdst=tm1.tm_isdst;
				ltime.tm_hour=0;
				ltime.tm_min=0;
				ltime.tm_sec=0;
				ltime.tm_year=tm1.tm_year;
				ltime.tm_mon=tm1.tm_mon;
				ltime.tm_mday=tm1.tm_mday;

				char szDate[40];
				sprintf(szDate,"%04d-%02d-%02d",ltime.tm_year+1900,ltime.tm_mon+1,ltime.tm_mday);

				std::vector<std::vector<std::string> > result2;
				strcpy(szTmp,"0");
				szQuery.clear();
				szQuery.str("");
				szQuery << "SELECT MIN(Value) FROM Meter WHERE (DeviceRowID=" << sd[0] << " AND Date>='" << szDate << "')";
				result2=m_pMain->m_sql.query(szQuery.str());
				if (result2.size()>0)
				{
					std::vector<std::string> sd2=result2[0];

					unsigned long long total_min_gas,total_real_gas;
					unsigned long long gasactual;

					std::stringstream s_str1( sd2[0] );
					s_str1 >> total_min_gas;
					std::stringstream s_str2( sValue );
					s_str2 >> gasactual;
					float musage=0;

					root["result"][ii]["SwitchTypeVal"]=MTYPE_GAS;

					musage=float(gasactual)/GasDivider;
					sprintf(szTmp,"%.03f",musage);
					root["result"][ii]["Counter"]=szTmp;

					total_real_gas=gasactual-total_min_gas;

					musage=float(total_real_gas)/GasDivider;
					sprintf(szTmp,"%.03f m3",musage);
					root["result"][ii]["CounterToday"]=szTmp;
					root["result"][ii]["Data"]=sValue;
					root["result"][ii]["HaveTimeout"]=bHaveTimeout;
				}
				else
				{
					root["result"][ii]["SwitchTypeVal"]=MTYPE_GAS;
					sprintf(szTmp,"%.03f",0.0f);
					root["result"][ii]["Counter"]=szTmp;
					sprintf(szTmp,"%.03f m3",0.0f);
					root["result"][ii]["CounterToday"]=szTmp;
					root["result"][ii]["Data"]=sValue;
					root["result"][ii]["HaveTimeout"]=bHaveTimeout;
				}
			}
			else if (dType == pTypeCURRENT)
			{
				std::vector<std::string> strarray;
				StringSplit(sValue, ";", strarray);
				if (strarray.size()==3)
				{
					//CM113
					int displaytype=0;
					int voltage=230;
					m_pMain->m_sql.GetPreferencesVar("CM113DisplayType", displaytype);
					m_pMain->m_sql.GetPreferencesVar("ElectricVoltage", voltage);

					if (displaytype==0)
						sprintf(szData,"%.1f A, %.1f A, %.1f A",atof(strarray[0].c_str()),atof(strarray[1].c_str()),atof(strarray[2].c_str()));
					else
						sprintf(szData,"%d Watt, %d Watt, %d Watt",int(atof(strarray[0].c_str())*voltage),int(atof(strarray[1].c_str())*voltage),int(atof(strarray[2].c_str())*voltage));
					root["result"][ii]["Data"]=szData;
					root["result"][ii]["displaytype"]=displaytype;
					root["result"][ii]["HaveTimeout"]=bHaveTimeout;
				}
			}
			else if (dType == pTypeCURRENTENERGY)
			{
				std::vector<std::string> strarray;
				StringSplit(sValue, ";", strarray);
				if (strarray.size()==4)
				{
					//CM180i
					int displaytype=0;
					int voltage=230;
					m_pMain->m_sql.GetPreferencesVar("CM113DisplayType", displaytype);
					m_pMain->m_sql.GetPreferencesVar("ElectricVoltage", voltage);

					double total=atof(strarray[3].c_str());
					if (displaytype==0)
					{
						sprintf(szData,"%.1f A, %.1f A, %.1f A",atof(strarray[0].c_str()),atof(strarray[1].c_str()),atof(strarray[2].c_str()));
					}
					else
					{
						sprintf(szData,"%d Watt, %d Watt, %d Watt",int(atof(strarray[0].c_str())*voltage),int(atof(strarray[1].c_str())*voltage),int(atof(strarray[2].c_str())*voltage));
					}
					if (total>0)
					{
						sprintf(szTmp,", Total: %.3f Wh",total);
						strcat(szData,szTmp);
					}
					root["result"][ii]["Data"]=szData;
					root["result"][ii]["displaytype"]=displaytype;
					root["result"][ii]["HaveTimeout"]=bHaveTimeout;
				}
			}
			else if ((dType == pTypeENERGY)||(dType == pTypePOWER))
			{
				std::vector<std::string> strarray;
				StringSplit(sValue, ";", strarray);
				if (strarray.size()==2)
				{
					double total=atof(strarray[1].c_str())/1000;
					sprintf(szData,"%.3f kWh",total);
					root["result"][ii]["Data"]=szData;
					sprintf(szData,"%ld Watt",atol(strarray[0].c_str()));
					root["result"][ii]["Usage"]=szData;
					root["result"][ii]["SwitchTypeVal"]=MTYPE_ENERGY;
					root["result"][ii]["HaveTimeout"]=bHaveTimeout;
				}
			}
			else if (dType == pTypeAirQuality)
			{
				if (bHaveTimeout)
					nValue=0;
				sprintf(szTmp,"%d ppm",nValue);
				root["result"][ii]["Data"]=szTmp;
				root["result"][ii]["HaveTimeout"]=bHaveTimeout;
				int airquality = nValue;
				if (airquality<700)
					root["result"][ii]["Quality"]="Excellent";
				else if (airquality<900)
					root["result"][ii]["Quality"]="Good";
				else if (airquality<1100)
					root["result"][ii]["Quality"]="Fair";
				else if (airquality<1600)
					root["result"][ii]["Quality"]="Mediocre";
				else
					root["result"][ii]["Quality"]="Bad";
			}
			else if (dType == pTypeThermostat)
			{
				if (sTypeThermSetpoint)
				{
					sprintf(szTmp,"%.1f",atof(sValue.c_str()));
					root["result"][ii]["Data"]=szTmp;
					root["result"][ii]["SetPoint"]=(float)atof(szTmp);
					root["result"][ii]["HaveTimeout"]=bHaveTimeout;
					root["result"][ii]["TypeImg"]="override_mini";
				}
			}
			else if (dType == pTypeGeneral)
			{
				if (dSubType==sTypeVisibility)
				{
					float vis=(float)atof(sValue.c_str());
					if (metertype==0)
					{
						//km
						sprintf(szTmp,"%.1f km",vis);
					}
					else
					{
						//miles
						sprintf(szTmp,"%.1f mi",vis*0.6214f);
					}
					root["result"][ii]["Data"]=szTmp;
					root["result"][ii]["Visibility"]=atof(sValue.c_str());
					root["result"][ii]["HaveTimeout"]=bHaveTimeout;
					root["result"][ii]["TypeImg"]="visibility";
					root["result"][ii]["SwitchTypeVal"]=metertype;
				}
				else if (dSubType==sTypeSolarRadiation)
				{
					float radiation=(float)atof(sValue.c_str());
					sprintf(szTmp,"%.1f Watt/m2",radiation);
					root["result"][ii]["Data"]=szTmp;
					root["result"][ii]["Radiation"]=atof(sValue.c_str());
					root["result"][ii]["HaveTimeout"]=bHaveTimeout;
					root["result"][ii]["TypeImg"]="radiation";
					root["result"][ii]["SwitchTypeVal"]=metertype;
				}
				else if (dSubType==sTypeSoilMoisture)
				{
					sprintf(szTmp,"%d cb",nValue);
					root["result"][ii]["Data"]=szTmp;
					root["result"][ii]["Desc"]=Get_Moisture_Desc(nValue);
					root["result"][ii]["TypeImg"]="moisture";
					root["result"][ii]["HaveTimeout"]=bHaveTimeout;
					root["result"][ii]["SwitchTypeVal"]=metertype;
				}
				else if (dSubType==sTypeLeafWetness)
				{
					sprintf(szTmp,"%d",nValue);
					root["result"][ii]["Data"]=szTmp;
					root["result"][ii]["TypeImg"]="leaf";
					root["result"][ii]["HaveTimeout"]=bHaveTimeout;
					root["result"][ii]["SwitchTypeVal"]=metertype;
				}
			}
			else if (dType == pTypeLux)
			{
				sprintf(szTmp,"%.0f Lux",atof(sValue.c_str()));
				root["result"][ii]["Data"]=szTmp;
				root["result"][ii]["HaveTimeout"]=bHaveTimeout;
			}
			else if (dType == pTypeWEIGHT)
			{
				sprintf(szTmp,"%.1f kg",atof(sValue.c_str()));
				root["result"][ii]["Data"]=szTmp;
				root["result"][ii]["HaveTimeout"]=false;
			}
			else if (dType == pTypeUsage)
			{
				if (dSubType==sTypeElectric)
				{
					sprintf(szData,"%.1f Watt",atof(sValue.c_str()));
					root["result"][ii]["Data"]=szData;
				}
				else
				{
					root["result"][ii]["Data"]=sValue;
				}
				root["result"][ii]["HaveTimeout"]=bHaveTimeout;
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
				root["result"][ii]["HaveTimeout"]=bHaveTimeout;
			}
			else if (dType == pTypeRego6XXValue)
			{
				switch (dSubType)
				{
				case sTypeRego6XXStatus:
                    {
   				        std::string lstatus="On";

	    		        if(atoi(sValue.c_str()) == 0)
                        {
                            lstatus = "Off";
                        }
				        root["result"][ii]["Status"]=lstatus;
				        root["result"][ii]["HaveDimmer"]=false;
				        root["result"][ii]["MaxDimLevel"]=0;
				        root["result"][ii]["HaveGroupCmd"]=false;
				        root["result"][ii]["TypeImg"]="utility";
				        root["result"][ii]["SwitchTypeVal"]=STYPE_OnOff;
				        root["result"][ii]["SwitchType"] =Switch_Type_Desc(STYPE_OnOff);
				        sprintf(szData,"%d", atoi(sValue.c_str()));
				        root["result"][ii]["Data"]=szData;
         			    root["result"][ii]["HaveTimeout"]=bHaveTimeout;

                        if (CustomImage<(int)m_custom_light_icons.size())
					        root["result"][ii]["Image"]=m_custom_light_icons[CustomImage].RootFile;
				        else
					        root["result"][ii]["Image"]="Light";
				
                    }
                    break;
				case sTypeRego6XXCounter:
                    {
				        //get value of today
				        time_t now = mytime(NULL);
				        struct tm tm1;
				        localtime_r(&now,&tm1);

				        struct tm ltime;
				        ltime.tm_isdst=tm1.tm_isdst;
				        ltime.tm_hour=0;
				        ltime.tm_min=0;
				        ltime.tm_sec=0;
				        ltime.tm_year=tm1.tm_year;
				        ltime.tm_mon=tm1.tm_mon;
				        ltime.tm_mday=tm1.tm_mday;

				        char szDate[40];
				        sprintf(szDate,"%04d-%02d-%02d",ltime.tm_year+1900,ltime.tm_mon+1,ltime.tm_mday);

				        std::vector<std::vector<std::string> > result2;
				        strcpy(szTmp,"0");
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
				        }
    					root["result"][ii]["SwitchTypeVal"]=MTYPE_COUNTER;
				        root["result"][ii]["Counter"]=sValue;
				        root["result"][ii]["CounterToday"]=szTmp;
				        root["result"][ii]["Data"]=sValue;
				        root["result"][ii]["HaveTimeout"]=bHaveTimeout;
                    }
                    break;
                }
			}

			ii++;
		}
	}
}


std::string CWebServer::GetInternalCameraSnapshot()
{
	m_retstr="";
	std::vector<unsigned char> camimage;
	if (m_pWebEm->m_lastRequestPath.find("raspberry")!=std::string::npos)
	{
		if (!m_pMain->m_cameras.TakeRaspberrySnapshot(camimage))
			goto exitproc;
	}
	else
	{
		if (!m_pMain->m_cameras.TakeUVCSnapshot(camimage))
			goto exitproc;
	}
	m_retstr.insert( m_retstr.begin(), camimage.begin(), camimage.end() );
	m_pWebEm->m_outputfilename="snapshot.jpg";
exitproc:
	return m_retstr;
}

std::string CWebServer::GetCameraSnapshot()
{
	m_retstr="";
	std::vector<unsigned char> camimage;
	std::string idx=m_pWebEm->FindValue("idx");
	if (idx=="")
		goto exitproc;

	if (!m_pMain->m_cameras.TakeSnapshot(idx, camimage))
		goto exitproc;
	m_retstr.insert( m_retstr.begin(), camimage.begin(), camimage.end() );
	m_pWebEm->m_outputfilename="snapshot.jpg";
exitproc:
	return m_retstr;
}

std::string CWebServer::GetDatabaseBackup()
{
	m_retstr="";
	std::string OutputFileName=szStartupFolder + "backup.db";
	if (m_pMain->m_sql.BackupDatabase(OutputFileName))
	{
		std::ifstream testFile(OutputFileName.c_str(), std::ios::binary);
		std::vector<char> fileContents((std::istreambuf_iterator<char>(testFile)),
			std::istreambuf_iterator<char>());
		if (fileContents.size()>0)
		{
			m_retstr.insert( m_retstr.begin(), fileContents.begin(), fileContents.end() );
			m_pWebEm->m_outputfilename="domoticz.db";
		}
	}
	return m_retstr;
}

std::string CWebServer::GetLanguage()
{
	Json::Value root;
	root["status"]="ERR";
	std::string sValue;
	if (m_pMain->m_sql.GetPreferencesVar("Language", sValue))
	{
		root["status"]="OK";
		root["title"]="GetLanguage";
		root["language"]=sValue;
	}
	m_retstr=root.toStyledString();
	return m_retstr;
}

std::string CWebServer::GetJSonPage()
{
	Json::Value root;
	root["status"]="ERR";

	bool bHaveUser=(m_pWebEm->m_actualuser!="");
	int iUser=-1;
	if (bHaveUser)
	{
		iUser=FindUser(m_pWebEm->m_actualuser.c_str());
	}

	m_retstr="";
	if (!m_pWebEm->HasParams())
	{
		m_retstr=root.toStyledString();

		return m_retstr;
	}

	std::string rtype=m_pWebEm->FindValue("type");

	unsigned long long idx=0;
	if (m_pWebEm->FindValue("idx")!="")
	{
		std::stringstream s_str( m_pWebEm->FindValue("idx") );
		s_str >> idx;
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
		szQuery << "SELECT ID, Name, Enabled, Type, Address, Port, Username, Password, Mode1, Mode2, Mode3, Mode4, Mode5 FROM Hardware ORDER BY ID ASC";
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
				root["result"][ii]["Enabled"]=(sd[2]=="1")?"true":"false";
				root["result"][ii]["Type"]=atoi(sd[3].c_str());
				root["result"][ii]["Address"]=sd[4];
				root["result"][ii]["Port"]=atoi(sd[5].c_str());
				root["result"][ii]["Username"]=sd[6];
				root["result"][ii]["Password"]=sd[7];
				root["result"][ii]["Mode1"]=atoi(sd[8].c_str());
				root["result"][ii]["Mode2"]=atoi(sd[9].c_str());
				root["result"][ii]["Mode3"]=atoi(sd[10].c_str());
				root["result"][ii]["Mode4"]=atoi(sd[11].c_str());
				root["result"][ii]["Mode5"]=atoi(sd[12].c_str());
				ii++;
			}
		}
	} //if (rtype=="hardware")
#ifdef WITH_OPENZWAVE
	else if (rtype=="openzwavenodes")
	{
		std::string hwid=m_pWebEm->FindValue("idx");
		if (hwid=="")
			goto exitjson;
		int iHardwareID=atoi(hwid.c_str());
		CDomoticzHardwareBase *pHardware=m_pMain->GetHardware(iHardwareID);
		if (pHardware==NULL)
			goto exitjson;
		if (pHardware->HwdType!=HTYPE_OpenZWave) //later we also do the Razberry
			goto exitjson;
		COpenZWave *pOZWHardware=(COpenZWave*)pHardware;

		root["status"]="OK";
		root["title"]="OpenZWaveNodes";

		std::stringstream szQuery;
		std::vector<std::vector<std::string> > result;
		szQuery << "SELECT ID,HomeID,NodeID,Name,ProductDescription,PollTime FROM ZWaveNodes WHERE (HardwareID==" << iHardwareID << ")";
		result=m_pMain->m_sql.query(szQuery.str());
		if (result.size()>0)
		{
			std::vector<std::vector<std::string> >::const_iterator itt;
			int ii=0;
			for (itt=result.begin(); itt!=result.end(); ++itt)
			{
				std::vector<std::string> sd=*itt;

				int homeID=atoi(sd[1].c_str());
				int nodeID=atoi(sd[2].c_str());
				if (nodeID>2)
				{
					//Don't include the controller
					COpenZWave::NodeInfo *pNode=pOZWHardware->GetNodeInfo(homeID, nodeID);
					if (pNode==NULL)
						continue;
					root["result"][ii]["idx"]=sd[0];
					root["result"][ii]["HomeID"]=homeID;
					root["result"][ii]["NodeID"]=nodeID;
					root["result"][ii]["Name"]=sd[3];
					root["result"][ii]["Description"]=sd[4];
					root["result"][ii]["PollEnabled"]=(atoi(sd[5].c_str())==1)?"true":"false";
					root["result"][ii]["Version"]=pNode->iVersion;
					root["result"][ii]["Manufacturer_id"]=pNode->Manufacturer_id;
					root["result"][ii]["Manufacturer_name"]=pNode->Manufacturer_name;
					root["result"][ii]["Product_type"]=pNode->Product_type;
					root["result"][ii]["Product_id"]=pNode->Product_id;
					root["result"][ii]["Product_name"]=pNode->Product_name;
					//root["result"][ii]["LastUpdate"]=pNode->m_LastSeen;

					//Add configuration parameters here
					pOZWHardware->GetNodeValuesJson(homeID,nodeID,root,ii);
					ii++;
				}
			}
		}
	}
#endif
	else if (rtype=="devices")
	{
		std::string rfilter=m_pWebEm->FindValue("filter");
		std::string order=m_pWebEm->FindValue("order");
		std::string rused=m_pWebEm->FindValue("used");
		std::string rid=m_pWebEm->FindValue("rid");
		std::string planid=m_pWebEm->FindValue("plan");

		root["status"]="OK";
		root["title"]="Devices";

		GetJSonDevices(root, rused, rfilter,order,rid,planid);

		root["WindScale"]=m_pMain->m_sql.m_windscale*10.0f;
		root["WindSign"]=m_pMain->m_sql.m_windsign;

	} //if (rtype=="devices")
    else if (rtype=="cameras")
	{
        std::string rused=m_pWebEm->FindValue("used");
        
		root["status"]="OK";
		root["title"]="Cameras";
        
		szQuery.clear();
		szQuery.str("");
        if(rused=="true") {
		szQuery << "SELECT ID, Name, Enabled, Address, Port, Username, Password, VideoURL, ImageURL FROM Cameras WHERE (Enabled=='1') ORDER BY ID ASC";
        }
        else {
            szQuery << "SELECT ID, Name, Enabled, Address, Port, Username, Password, VideoURL, ImageURL FROM Cameras ORDER BY ID ASC";
        }
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
				root["result"][ii]["Enabled"]=(sd[2]=="1")?"true":"false";
				root["result"][ii]["Address"]=sd[3];
				root["result"][ii]["Port"]=atoi(sd[4].c_str());
				root["result"][ii]["Username"]=base64_decode(sd[5]);
				root["result"][ii]["Password"]=base64_decode(sd[6]);
				root["result"][ii]["VideoURL"]=sd[7];
				root["result"][ii]["ImageURL"]=sd[8];
				ii++;
			}
		}
	} //if (rtype=="cameras")
	else if (rtype=="users")
	{
		bool bHaveUser=(m_pWebEm->m_actualuser!="");
		int urights=3;
		if (bHaveUser)
		{
			int iUser=-1;
			iUser=FindUser(m_pWebEm->m_actualuser.c_str());
			if (iUser!=-1)
				urights=(int)m_users[iUser].userrights;
		}
		if (urights<2)
			goto exitjson;

		root["status"]="OK";
		root["title"]="Users";

		szQuery.clear();
		szQuery.str("");
		szQuery << "SELECT ID, Active, Username, Password, Rights, RemoteSharing, TabsEnabled FROM USERS ORDER BY ID ASC";
		result=m_pMain->m_sql.query(szQuery.str());
		if (result.size()>0)
		{
			std::vector<std::vector<std::string> >::const_iterator itt;
			int ii=0;
			for (itt=result.begin(); itt!=result.end(); ++itt)
			{
				std::vector<std::string> sd=*itt;

				root["result"][ii]["idx"]=sd[0];
				root["result"][ii]["Enabled"]=(sd[1]=="1")?"true":"false";
				root["result"][ii]["Username"]=base64_decode(sd[2]);
				root["result"][ii]["Password"]=base64_decode(sd[3]);
				root["result"][ii]["Rights"]=atoi(sd[4].c_str());
				root["result"][ii]["RemoteSharing"]=atoi(sd[5].c_str());
				root["result"][ii]["TabsEnabled"]=atoi(sd[6].c_str());
				ii++;
			}
		}
	} //if (rtype=="users")
	else if (rtype=="status-temp")
	{
		root["status"]="OK";
		root["title"]="StatusTemp";

		Json::Value tempjson;

		GetJSonDevices(tempjson, "", "temp","ID","","");

		Json::Value::const_iterator itt;
		int ii=0;
		Json::ArrayIndex rsize=tempjson["result"].size();
		for ( Json::ArrayIndex itt = 0; itt<rsize; itt++)
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

		GetJSonDevices(tempjson, "", "wind","ID","","");

		Json::Value::const_iterator itt;
		int ii=0;
		Json::ArrayIndex rsize=tempjson["result"].size();
		for ( Json::ArrayIndex itt = 0; itt<rsize; itt++)
		{
			root["result"][ii]["idx"]=ii;
			root["result"][ii]["Name"]=tempjson["result"][itt]["Name"];
			root["result"][ii]["Direction"]=tempjson["result"][itt]["Direction"];
			root["result"][ii]["DirectionStr"]=tempjson["result"][itt]["DirectionStr"];
			root["result"][ii]["Gust"]=tempjson["result"][itt]["Gust"];
			root["result"][ii]["Speed"]=tempjson["result"][itt]["Speed"];
			root["result"][ii]["LastUpdate"]=tempjson["result"][itt]["LastUpdate"];

			ii++;
		}
	} //if (rtype=="status-wind")
	else if (rtype=="status-rain")
	{
		root["status"]="OK";
		root["title"]="StatusRain";

		Json::Value tempjson;

		GetJSonDevices(tempjson, "", "rain","ID","","");

		Json::Value::const_iterator itt;
		int ii=0;
		Json::ArrayIndex rsize=tempjson["result"].size();
		for ( Json::ArrayIndex itt = 0; itt<rsize; itt++)
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

		GetJSonDevices(tempjson, "", "uv","ID","","");

		Json::Value::const_iterator itt;
		int ii=0;
		Json::ArrayIndex rsize=tempjson["result"].size();
		for ( Json::ArrayIndex itt = 0; itt<rsize; itt++)
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

		GetJSonDevices(tempjson, "", "baro","ID","","");

		Json::Value::const_iterator itt;
		int ii=0;
		Json::ArrayIndex rsize=tempjson["result"].size();
		for ( Json::ArrayIndex itt = 0; itt<rsize; itt++)
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
			(dType!=pTypeLimitlessLights)&&
			(dType!=pTypeSecurity1)&&
			(dType!=pTypeBlinds)&&
			(dType!=pTypeRego6XXValue)&&
			(dType!=pTypeChime)
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

				int nValue = atoi(sd[1].c_str());
				std::string sValue=sd[2];

				root["result"][ii]["idx"]=sd[0];

				//add light details
				std::string lstatus="";
				int llevel=0;
				bool bHaveDimmer=false;
				bool bHaveGroupCmd=false;
				int maxDimLevel=0;

				GetLightStatus(dType,dSubType,nValue,sValue,lstatus,llevel,bHaveDimmer,maxDimLevel,bHaveGroupCmd);

				if (ii==0)
				{
					root["HaveDimmer"]=bHaveDimmer;
					root["result"][ii]["MaxDimLevel"]=maxDimLevel;
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
		szQuery << "SELECT ID, Active, Time, Type, Cmd, Level, Hue, Days, UseRandomness FROM Timers WHERE (DeviceRowID==" << idx << ") ORDER BY ID";
		result=m_pMain->m_sql.query(szQuery.str());
		if (result.size()>0)
		{
			std::vector<std::vector<std::string> >::const_iterator itt;
			int ii=0;
			for (itt=result.begin(); itt!=result.end(); ++itt)
			{
				std::vector<std::string> sd=*itt;

				unsigned char iLevel=atoi(sd[5].c_str());
				if (iLevel==0)
					iLevel=100;

				root["result"][ii]["idx"]=sd[0];
				root["result"][ii]["Active"]=(atoi(sd[1].c_str())==0)?"false":"true";
				root["result"][ii]["Time"]=sd[2].substr(0,5);
				root["result"][ii]["Type"]=atoi(sd[3].c_str());
				root["result"][ii]["Cmd"]=atoi(sd[4].c_str());
				root["result"][ii]["Level"]=iLevel;
				root["result"][ii]["Hue"]=atoi(sd[6].c_str());
				root["result"][ii]["Days"]=atoi(sd[7].c_str());
				root["result"][ii]["Randomness"]=(atoi(sd[8].c_str())!=0);
				ii++;
			}
		}
	} //(rtype=="timers")
	else if ((rtype=="scenetimers")&&(idx!=0))
	{
		root["status"]="OK";
		root["title"]="SceneTimers";

		szQuery.clear();
		szQuery.str("");
		szQuery << "SELECT ID, Active, Time, Type, Cmd, Level, Hue, Days, UseRandomness FROM SceneTimers WHERE (SceneRowID==" << idx << ") ORDER BY ID";
		result=m_pMain->m_sql.query(szQuery.str());
		if (result.size()>0)
		{
			std::vector<std::vector<std::string> >::const_iterator itt;
			int ii=0;
			for (itt=result.begin(); itt!=result.end(); ++itt)
			{
				std::vector<std::string> sd=*itt;

				unsigned char iLevel=atoi(sd[5].c_str());
				if (iLevel==0)
					iLevel=100;

				root["result"][ii]["idx"]=sd[0];
				root["result"][ii]["Active"]=(atoi(sd[1].c_str())==0)?"false":"true";
				root["result"][ii]["Time"]=sd[2].substr(0,5);
				root["result"][ii]["Type"]=atoi(sd[3].c_str());
				root["result"][ii]["Cmd"]=atoi(sd[4].c_str());
				root["result"][ii]["Level"]=iLevel;
				root["result"][ii]["Days"]=atoi(sd[6].c_str());
				root["result"][ii]["Hue"]=atoi(sd[7].c_str());
				root["result"][ii]["Randomness"]=(atoi(sd[8].c_str())!=0);
				ii++;
			}
		}
	} //(rtype=="scenetimers")
	else if ((rtype=="gettransfers")&&(idx!=0))
	{
		root["status"]="OK";
		root["title"]="GetTransfers";

		szQuery.clear();
		szQuery.str("");
		szQuery << "SELECT Type, SubType FROM DeviceStatus WHERE (ID==" << idx << ")";
		result=m_pMain->m_sql.query(szQuery.str());
		if (result.size()>0)
		{
			szQuery.clear();
			szQuery.str("");

			int dType=atoi(result[0][0].c_str());
			if (
				(dType==pTypeTEMP)||
				(dType==pTypeTEMP_HUM)
				)
			{
				szQuery << "SELECT ID, Name FROM DeviceStatus WHERE (Type==" << result[0][0] << ") AND (ID!=" << idx << ")";
			}
			else
			{
				szQuery << "SELECT ID, Name FROM DeviceStatus WHERE (Type==" << result[0][0] << ") AND (SubType==" << result[0][1] << ") AND (ID!=" << idx << ")";
			}
			result=m_pMain->m_sql.query(szQuery.str());

			std::vector<std::vector<std::string> >::const_iterator itt;
			int ii=0;
			for (itt=result.begin(); itt!=result.end(); ++itt)
			{
				std::vector<std::string> sd=*itt;

				root["result"][ii]["idx"]=sd[0];
				root["result"][ii]["Name"]=sd[1];
				ii++;
			}
		}
	} //(rtype=="gettransfers")
	else if ((rtype=="transferdevice")&&(idx!=0))
	{
		std::string sidx=m_pWebEm->FindValue("idx");
		if (sidx=="")
			goto exitjson;

		std::string newidx=m_pWebEm->FindValue("newidx");
		if (newidx=="")
			goto exitjson;

		root["status"]="OK";
		root["title"]="TransferDevice";

		//transfer device logs
		m_pMain->m_sql.TransferDevice(sidx,newidx);

		//now delete the old device
		m_pMain->m_sql.DeleteDevice(sidx);
		m_pMain->m_scheduler.ReloadSchedules();
	} //(rtype=="transferdevice")
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
				std::string sParams=itt->Params;
				if (sParams=="") {
					sParams="S";
				}
				root["result"][ii]["Params"]=sParams;
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
			root["result"][ii]["Type"]=(itt->bIsScene)?"Scene":"Device";
			root["result"][ii]["RowID"]=itt->RowID;
			root["result"][ii]["DevName"]=itt->DeviceName;
			root["result"][ii]["TimerType"]=Timer_Type_Desc(itt->timerType);
			root["result"][ii]["TimerCmd"]=Timer_Cmd_Desc(itt->timerCmd);
			root["result"][ii]["Days"]=itt->Days;
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

		time_t now = mytime(NULL);
		struct tm tm1;
		localtime_r(&now,&tm1);

		szQuery.clear();
		szQuery.str("");
		szQuery << "SELECT Type, SubType, SwitchType, AddjValue, AddjMulti FROM DeviceStatus WHERE (ID == " << idx << ")";
		result=m_pMain->m_sql.query(szQuery.str());
		if (result.size()<1)
			goto exitjson;

		unsigned char dType=atoi(result[0][0].c_str());
		unsigned char dSubType=atoi(result[0][1].c_str());
		_eMeterType metertype = (_eMeterType)atoi(result[0][2].c_str());
		if ((dType==pTypeP1Power)||(dType==pTypeENERGY)||(dType==pTypePOWER))
			metertype= MTYPE_ENERGY;
		else if (dType==pTypeP1Gas)
			metertype= MTYPE_GAS;
        else if ((dType==pTypeRego6XXValue)&&(dSubType==sTypeRego6XXCounter))
			metertype= MTYPE_COUNTER;

		double AddjValue=atof(result[0][3].c_str());
		double AddjMulti=atof(result[0][4].c_str());

		std::string dbasetable="";
		if (srange=="day") {
			if (sensor=="temp")
				dbasetable="Temperature";
			else if (sensor=="rain")
				dbasetable="Rain";
			else if (sensor=="counter") 
			{
				if ((dType==pTypeP1Power)||(dType==pTypeCURRENT)||(dType==pTypeCURRENTENERGY))
					dbasetable="MultiMeter";
				else
					dbasetable="Meter";
			}
			else if ( (sensor=="wind") || (sensor=="winddir") )
				dbasetable="Wind";
			else if (sensor=="uv")
				dbasetable="UV";
			else
				goto exitjson;
		}
		else
		{
			//week,year,month
			if (sensor=="temp")
				dbasetable="Temperature_Calendar";
			else if (sensor=="rain")
				dbasetable="Rain_Calendar";
			else if (sensor=="counter")
			{
				if (
					(dType==pTypeP1Power)||
					(dType==pTypeCURRENT)||
					(dType==pTypeCURRENTENERGY)||
					(dType==pTypeAirQuality)||
					((dType==pTypeGeneral)&&(dSubType==sTypeVisibility))||
					((dType==pTypeGeneral)&&(dSubType==sTypeSolarRadiation))||
					((dType==pTypeGeneral)&&(dSubType==sTypeSoilMoisture))||
					((dType==pTypeGeneral)&&(dSubType==sTypeLeafWetness))||
					((dType==pTypeRFXSensor)&&(dSubType==sTypeRFXSensorAD))||
					((dType==pTypeRFXSensor)&&(dSubType==sTypeRFXSensorVolt))||
					(dType==pTypeLux)||
					(dType==pTypeWEIGHT)||
					(dType==pTypeUsage)
					)
					dbasetable="MultiMeter_Calendar";
				else
					dbasetable="Meter_Calendar";
			}
			else if ( (sensor=="wind") || (sensor=="winddir") )
				dbasetable="Wind_Calendar";
			else if (sensor=="uv")
				dbasetable="UV_Calendar";
			else
				goto exitjson;
		}


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
							(dType==pTypeRego6XXTemp)||
							(dType==pTypeTEMP)||
							(dType==pTypeTEMP_HUM)||
							(dType==pTypeTEMP_HUM_BARO)||
							(dType==pTypeTEMP_BARO)||
							((dType==pTypeWIND)&&(dSubType==sTypeWIND4))||
							((dType==pTypeWIND)&&(dSubType==sTypeWINDNoTemp))||
							((dType==pTypeUV)&&(dSubType==sTypeUV3))||
							(dType==pTypeThermostat1)||
							((dType==pTypeRFXSensor)&&(dSubType==sTypeRFXSensorTemp))
							)
						{
							root["result"][ii]["te"]=sd[0];
						}
						if (
							((dType==pTypeWIND)&&(dSubType==sTypeWIND4))||
							((dType==pTypeWIND)&&(dSubType==sTypeWINDNoTemp))
							)
						{
							root["result"][ii]["ch"]=sd[1];
						}
						if ((dType==pTypeHUM)||(dType==pTypeTEMP_HUM)||(dType==pTypeTEMP_HUM_BARO))
						{
							root["result"][ii]["hu"]=sd[2];
						}
						if (
							(dType==pTypeTEMP_HUM_BARO)||
							(dType==pTypeTEMP_BARO)
							)
						{
							if (dType==pTypeTEMP_HUM_BARO)
							{
								if (dSubType==sTypeTHBFloat)
								{
									sprintf(szTmp,"%.1f",atof(sd[3].c_str())/10.0f);
									root["result"][ii]["ba"]=szTmp;
								}
								else
									root["result"][ii]["ba"]=sd[3];
							}
							else if (dType==pTypeTEMP_BARO)
							{
								sprintf(szTmp,"%.1f",atof(sd[3].c_str())/10.0f);
								root["result"][ii]["ba"]=szTmp;
							}
						}

						ii++;
					}
				}
			}
			else if (sensor=="counter")
			{
				if (dType==pTypeP1Power)
				{
					root["status"]="OK";
					root["title"]="Graph " + sensor + " " + srange;

					szQuery.clear();
					szQuery.str("");
					szQuery << "SELECT Value1, Value2, Value3, Value4, Value5, Value6, Date FROM " << dbasetable << " WHERE (DeviceRowID==" << idx << ") ORDER BY Date ASC";
					result=m_pMain->m_sql.query(szQuery.str());
					if (result.size()>0)
					{
						std::vector<std::vector<std::string> >::const_iterator itt;
						int ii=0;
						bool bHaveDeliverd=false;
						bool bHaveFirstValue=false;
						long long lastUsage1,lastUsage2,lastDeliv1,lastDeliv2;
						time_t lastTime=0;

						int nMeterType=0;
						m_pMain->m_sql.GetPreferencesVar("SmartMeterType", nMeterType);

						for (itt=result.begin(); itt!=result.end(); ++itt)
						{
							std::vector<std::string> sd=*itt;

							if (nMeterType==0)
							{
								long long actUsage1,actUsage2,actDeliv1,actDeliv2;
								std::stringstream s_str1( sd[0] );
								s_str1 >> actUsage1;
								std::stringstream s_str2( sd[4] );
								s_str2 >> actUsage2;
								std::stringstream s_str3( sd[1] );
								s_str3 >> actDeliv1;
								std::stringstream s_str4( sd[5] );
								s_str4 >> actDeliv2;

								std::string stime=sd[6];
								struct tm ntime;
								time_t atime;
								ntime.tm_isdst=0;
								ntime.tm_year=atoi(stime.substr(0,4).c_str())-1900;
								ntime.tm_mon=atoi(stime.substr(5,2).c_str())-1;
								ntime.tm_mday=atoi(stime.substr(8,2).c_str());
								ntime.tm_hour=atoi(stime.substr(11,2).c_str());
								ntime.tm_min=atoi(stime.substr(14,2).c_str());
								ntime.tm_sec=atoi(stime.substr(17,2).c_str());
								atime=mktime(&ntime);

								if (bHaveFirstValue)
								{
									long curUsage1=(long)(actUsage1-lastUsage1);
									long curUsage2=(long)(actUsage2-lastUsage2);
									long curDeliv1=(long)(actDeliv1-lastDeliv1);
									long curDeliv2=(long)(actDeliv2-lastDeliv2);

									if ((curUsage1<0)||(curUsage1>100000))
										curUsage1=0;
									if ((curUsage2<0)||(curUsage2>100000))
										curUsage2=0;
									if ((curDeliv1<0)||(curDeliv1>100000))
										curDeliv1=0;
									if ((curDeliv2<0)||(curDeliv2>100000))
										curDeliv2=0;

									time_t tdiff=atime-lastTime;
									if (tdiff==0)
										tdiff=1;
									float tlaps=3600.0f/tdiff;
									curUsage1*=int(tlaps);
									curUsage2*=int(tlaps);
									curDeliv1*=int(tlaps);
									curDeliv2*=int(tlaps);

									root["result"][ii]["d"]=sd[6].substr(0,16);

									if ((curDeliv1!=0)||(curDeliv2!=0))
										bHaveDeliverd=true;

									sprintf(szTmp,"%ld",curUsage1);
									root["result"][ii]["v"]=szTmp;
									sprintf(szTmp,"%ld",curUsage2);
									root["result"][ii]["v2"]=szTmp;
									sprintf(szTmp,"%ld",curDeliv1);
									root["result"][ii]["r1"]=szTmp;
									sprintf(szTmp,"%ld",curDeliv2);
									root["result"][ii]["r2"]=szTmp;
									ii++;
								}
								else
								{
									bHaveFirstValue=true;
								}
								lastUsage1=actUsage1;
								lastUsage2=actUsage2;
								lastDeliv1=actDeliv1;
								lastDeliv2=actDeliv2;
								lastTime=atime;
							}
							else
							{
								//this meter has no decimals, so return the use peaks
								root["result"][ii]["d"]=sd[6].substr(0,16);

								if (sd[3]!="0")
									bHaveDeliverd=true;
								root["result"][ii]["v"]=sd[2];
								root["result"][ii]["r1"]=sd[3];
								ii++;

							}
						}
						if (bHaveDeliverd)
						{
							root["delivered"]=true;
						}
					}
				}
				else if (dType==pTypeAirQuality)
				{//day
					root["status"]="OK";
					root["title"]="Graph " + sensor + " " + srange;

					szQuery.clear();
					szQuery.str("");
					szQuery << "SELECT Value, Date FROM " << dbasetable << " WHERE (DeviceRowID==" << idx << ") ORDER BY Date ASC";
					result=m_pMain->m_sql.query(szQuery.str());
					if (result.size()>0)
					{
						std::vector<std::vector<std::string> >::const_iterator itt;
						int ii=0;
						for (itt=result.begin(); itt!=result.end(); ++itt)
						{
							std::vector<std::string> sd=*itt;

							root["result"][ii]["d"]=sd[1].substr(0,16);
							root["result"][ii]["co2"]=sd[0];
							ii++;
						}
					}
				}
				else if ((dType==pTypeGeneral)&&((dSubType==sTypeSoilMoisture)||(dSubType==sTypeLeafWetness)))
				{//day
					root["status"]="OK";
					root["title"]="Graph " + sensor + " " + srange;

					szQuery.clear();
					szQuery.str("");
					szQuery << "SELECT Value, Date FROM " << dbasetable << " WHERE (DeviceRowID==" << idx << ") ORDER BY Date ASC";
					result=m_pMain->m_sql.query(szQuery.str());
					if (result.size()>0)
					{
						std::vector<std::vector<std::string> >::const_iterator itt;
						int ii=0;
						for (itt=result.begin(); itt!=result.end(); ++itt)
						{
							std::vector<std::string> sd=*itt;

							root["result"][ii]["d"]=sd[1].substr(0,16);
							root["result"][ii]["v"]=sd[0];
							ii++;
						}
					}
				}
				else if (
					((dType==pTypeGeneral)&&(dSubType==sTypeVisibility))||
					((dType==pTypeGeneral)&&(dSubType==sTypeSolarRadiation))
					)
				{//day
					root["status"]="OK";
					root["title"]="Graph " + sensor + " " + srange;

					szQuery.clear();
					szQuery.str("");
					szQuery << "SELECT Value, Date FROM " << dbasetable << " WHERE (DeviceRowID==" << idx << ") ORDER BY Date ASC";
					result=m_pMain->m_sql.query(szQuery.str());
					if (result.size()>0)
					{
						std::vector<std::vector<std::string> >::const_iterator itt;
						int ii=0;
						for (itt=result.begin(); itt!=result.end(); ++itt)
						{
							std::vector<std::string> sd=*itt;

							root["result"][ii]["d"]=sd[1].substr(0,16);
							float fValue=float(atof(sd[0].c_str()))/10.0f;
							if (metertype==1)
								fValue*=0.6214f;
							sprintf(szTmp,"%.1f",fValue);
							root["result"][ii]["v"]=szTmp;
							ii++;
						}
					}
				}
				else if ((dType==pTypeRFXSensor)&&((dSubType==sTypeRFXSensorAD)||(dSubType==sTypeRFXSensorVolt)))
				{//day
					root["status"]="OK";
					root["title"]="Graph " + sensor + " " + srange;

					szQuery.clear();
					szQuery.str("");
					szQuery << "SELECT Value, Date FROM " << dbasetable << " WHERE (DeviceRowID==" << idx << ") ORDER BY Date ASC";
					result=m_pMain->m_sql.query(szQuery.str());
					if (result.size()>0)
					{
						std::vector<std::vector<std::string> >::const_iterator itt;
						int ii=0;
						for (itt=result.begin(); itt!=result.end(); ++itt)
						{
							std::vector<std::string> sd=*itt;

							root["result"][ii]["d"]=sd[1].substr(0,16);
							root["result"][ii]["v"]=sd[0];
							ii++;
						}
					}
				}
				else if (dType==pTypeLux)
				{//day
					root["status"]="OK";
					root["title"]="Graph " + sensor + " " + srange;

					szQuery.clear();
					szQuery.str("");
					szQuery << "SELECT Value, Date FROM " << dbasetable << " WHERE (DeviceRowID==" << idx << ") ORDER BY Date ASC";
					result=m_pMain->m_sql.query(szQuery.str());
					if (result.size()>0)
					{
						std::vector<std::vector<std::string> >::const_iterator itt;
						int ii=0;
						for (itt=result.begin(); itt!=result.end(); ++itt)
						{
							std::vector<std::string> sd=*itt;

							root["result"][ii]["d"]=sd[1].substr(0,16);
							root["result"][ii]["lux"]=sd[0];
							ii++;
						}
					}
				}
				else if (dType==pTypeWEIGHT)
				{//day
					root["status"]="OK";
					root["title"]="Graph " + sensor + " " + srange;

					szQuery.clear();
					szQuery.str("");
					szQuery << "SELECT Value, Date FROM " << dbasetable << " WHERE (DeviceRowID==" << idx << ") ORDER BY Date ASC";
					result=m_pMain->m_sql.query(szQuery.str());
					if (result.size()>0)
					{
						std::vector<std::vector<std::string> >::const_iterator itt;
						int ii=0;
						for (itt=result.begin(); itt!=result.end(); ++itt)
						{
							std::vector<std::string> sd=*itt;

							root["result"][ii]["d"]=sd[1].substr(0,16);
							root["result"][ii]["v"]=sd[0];
							ii++;
						}
					}
				}
				else if (dType==pTypeUsage)
				{//day
					root["status"]="OK";
					root["title"]="Graph " + sensor + " " + srange;

					szQuery.clear();
					szQuery.str("");
					szQuery << "SELECT Value, Date FROM " << dbasetable << " WHERE (DeviceRowID==" << idx << ") ORDER BY Date ASC";
					result=m_pMain->m_sql.query(szQuery.str());
					if (result.size()>0)
					{
						std::vector<std::vector<std::string> >::const_iterator itt;
						int ii=0;
						for (itt=result.begin(); itt!=result.end(); ++itt)
						{
							std::vector<std::string> sd=*itt;

							root["result"][ii]["d"]=sd[1].substr(0,16);
							root["result"][ii]["u"]=sd[0];
							ii++;
						}
					}
				}
				else if (dType==pTypeCURRENT)
				{
					root["status"]="OK";
					root["title"]="Graph " + sensor + " " + srange;

					//CM113
					int displaytype=0;
					int voltage=230;
					m_pMain->m_sql.GetPreferencesVar("CM113DisplayType", displaytype);
					m_pMain->m_sql.GetPreferencesVar("ElectricVoltage", voltage);

					root["displaytype"]=displaytype;

					szQuery.clear();
					szQuery.str("");
					szQuery << "SELECT Value1, Value2, Value3, Date FROM " << dbasetable << " WHERE (DeviceRowID==" << idx << ") ORDER BY Date ASC";
					result=m_pMain->m_sql.query(szQuery.str());
					if (result.size()>0)
					{
						std::vector<std::vector<std::string> >::const_iterator itt;
						int ii=0;
						bool bHaveL1=false;
						bool bHaveL2=false;
						bool bHaveL3=false;
						for (itt=result.begin(); itt!=result.end(); ++itt)
						{
							std::vector<std::string> sd=*itt;

							root["result"][ii]["d"]=sd[3].substr(0,16);

							float fval1=(float)atof(sd[0].c_str())/10.0f;
							float fval2=(float)atof(sd[1].c_str())/10.0f;
							float fval3=(float)atof(sd[2].c_str())/10.0f;

							if (fval1!=0)
								bHaveL1=true;
							if (fval2!=0)
								bHaveL2=true;
							if (fval3!=0)
								bHaveL3=true;

							if (displaytype==0)
							{
								sprintf(szTmp,"%.1f",fval1);
								root["result"][ii]["v1"]=szTmp;
								sprintf(szTmp,"%.1f",fval2);
								root["result"][ii]["v2"]=szTmp;
								sprintf(szTmp,"%.1f",fval3);
								root["result"][ii]["v3"]=szTmp;
							}
							else
							{
								sprintf(szTmp,"%d",int(fval1*voltage));
								root["result"][ii]["v1"]=szTmp;
								sprintf(szTmp,"%d",int(fval2*voltage));
								root["result"][ii]["v2"]=szTmp;
								sprintf(szTmp,"%d",int(fval3*voltage));
								root["result"][ii]["v3"]=szTmp;
							}
							ii++;
						}
						if (bHaveL1)
							root["haveL1"]=true;
						if (bHaveL2)
							root["haveL2"]=true;
						if (bHaveL3)
							root["haveL3"]=true;
					}
				}
				else if (dType==pTypeCURRENTENERGY)
				{
					root["status"]="OK";
					root["title"]="Graph " + sensor + " " + srange;

					//CM113
					int displaytype=0;
					int voltage=230;
					m_pMain->m_sql.GetPreferencesVar("CM113DisplayType", displaytype);
					m_pMain->m_sql.GetPreferencesVar("ElectricVoltage", voltage);

					root["displaytype"]=displaytype;

					szQuery.clear();
					szQuery.str("");
					szQuery << "SELECT Value1, Value2, Value3, Date FROM " << dbasetable << " WHERE (DeviceRowID==" << idx << ") ORDER BY Date ASC";
					result=m_pMain->m_sql.query(szQuery.str());
					if (result.size()>0)
					{
						std::vector<std::vector<std::string> >::const_iterator itt;
						int ii=0;
						bool bHaveL1=false;
						bool bHaveL2=false;
						bool bHaveL3=false;
						for (itt=result.begin(); itt!=result.end(); ++itt)
						{
							std::vector<std::string> sd=*itt;

							root["result"][ii]["d"]=sd[3].substr(0,16);

							float fval1=(float)atof(sd[0].c_str())/10.0f;
							float fval2=(float)atof(sd[1].c_str())/10.0f;
							float fval3=(float)atof(sd[2].c_str())/10.0f;

							if (fval1!=0)
								bHaveL1=true;
							if (fval2!=0)
								bHaveL2=true;
							if (fval3!=0)
								bHaveL3=true;

							if (displaytype==0)
							{
								sprintf(szTmp,"%.1f",fval1);
								root["result"][ii]["v1"]=szTmp;
								sprintf(szTmp,"%.1f",fval2);
								root["result"][ii]["v2"]=szTmp;
								sprintf(szTmp,"%.1f",fval3);
								root["result"][ii]["v3"]=szTmp;
							}
							else
							{
								sprintf(szTmp,"%d",int(fval1*voltage));
								root["result"][ii]["v1"]=szTmp;
								sprintf(szTmp,"%d",int(fval2*voltage));
								root["result"][ii]["v2"]=szTmp;
								sprintf(szTmp,"%d",int(fval3*voltage));
								root["result"][ii]["v3"]=szTmp;
							}
							ii++;
						}
						if (bHaveL1)
							root["haveL1"]=true;
						if (bHaveL2)
							root["haveL2"]=true;
						if (bHaveL3)
							root["haveL3"]=true;
					}
				}
				else
				{
					root["status"]="OK";
					root["title"]="Graph " + sensor + " " + srange;

 					float EnergyDivider=1000.0f;
					float GasDivider=100.0f;
					float WaterDivider=100.0f;
					int tValue;
					if (m_pMain->m_sql.GetPreferencesVar("MeterDividerEnergy", tValue))
					{
						EnergyDivider=float(tValue);
					}
					if (m_pMain->m_sql.GetPreferencesVar("MeterDividerGas", tValue))
					{
						GasDivider=float(tValue);
					}
					if (m_pMain->m_sql.GetPreferencesVar("MeterDividerWater", tValue))
					{
						WaterDivider=float(tValue);
					}
					if (dType==pTypeP1Gas)
						GasDivider=1000;
					else if ((dType==pTypeENERGY)||(dType==pTypePOWER))
						EnergyDivider*=100.0f;

					szQuery.clear();
					szQuery.str("");
					int ii=0;
					szQuery << "SELECT Value, Date FROM " << dbasetable << " WHERE (DeviceRowID==" << idx << ") ORDER BY Date ASC";
					result=m_pMain->m_sql.query(szQuery.str());

					bool bHaveFirstValue=false;
					bool bHaveFirstRealValue=false;
					float FirstValue=0;
					unsigned long long ulFirstRealValue=0;
					unsigned long long ulFirstValue=0;
					unsigned long long ulLastValue=0;
					std::string LastDateTime="";

					if (result.size()>0)
					{
						std::vector<std::vector<std::string> >::const_iterator itt;
						for (itt=result.begin(); itt!=result.end(); ++itt)
						{
							std::vector<std::string> sd=*itt;

							std::string actDateTimeHour=sd[1].substr(0,13);
							if (actDateTimeHour!=LastDateTime)
							{
								if (bHaveFirstValue)
								{
									root["result"][ii]["d"]=LastDateTime+":00";

									unsigned long long ulTotalValue=ulLastValue-ulFirstValue;
									if (ulTotalValue==0)
									{
										//Could be the P1 Gas Meter, only transmits one every 1 a 2 hours
										ulTotalValue=ulLastValue-ulFirstRealValue;
									}
									ulFirstRealValue=ulLastValue;
									float TotalValue=float(ulTotalValue);
									if (TotalValue!=0)
									{
										switch (metertype)
										{
										case MTYPE_ENERGY:
											sprintf(szTmp,"%.3f",(TotalValue/EnergyDivider)*1000.0f);	//from kWh -> Watt
											break;
										case MTYPE_GAS:
											sprintf(szTmp,"%.2f",TotalValue/GasDivider);
											break;
										case MTYPE_WATER:
											sprintf(szTmp,"%.2f",TotalValue/WaterDivider);
											break;
										case MTYPE_COUNTER:
											sprintf(szTmp,"%.1f",TotalValue);
											break;
										}
										root["result"][ii]["v"]=szTmp;
										ii++;
									}
								}
								LastDateTime=actDateTimeHour;
								bHaveFirstValue=false;
							}
							std::stringstream s_str1( sd[0] );
							unsigned long long actValue;
							s_str1 >> actValue;

							if (actValue>=ulLastValue)
								ulLastValue=actValue;

							if (!bHaveFirstValue)
							{
								ulFirstValue=ulLastValue;
								bHaveFirstValue=true;
							}
							if (!bHaveFirstRealValue)
							{
								bHaveFirstRealValue=true;
								ulFirstRealValue=ulLastValue;
							}
						}
					}
					if (bHaveFirstValue)
					{
						//add last value
						root["result"][ii]["d"]=LastDateTime+":00";

						unsigned long long ulTotalValue=ulLastValue-ulFirstValue;
						if (ulTotalValue==0)
						{
							if (bHaveFirstRealValue)
							{
								//Could be the P1 Gas Meter, only transmits one every 1 a 2 hours
								ulTotalValue=ulLastValue-ulFirstRealValue;
								ulFirstRealValue=ulLastValue;
							}
						}

						float TotalValue=float(ulTotalValue);

						if (TotalValue!=0)
						{
							switch (metertype)
							{
							case MTYPE_ENERGY:
								sprintf(szTmp,"%.3f",(TotalValue/EnergyDivider)*1000.0f);	//from kWh -> Watt
								break;
							case MTYPE_GAS:
								sprintf(szTmp,"%.2f",TotalValue/GasDivider);
								break;
							case MTYPE_WATER:
								sprintf(szTmp,"%.2f",TotalValue/WaterDivider);
								break;
							case MTYPE_COUNTER:
								sprintf(szTmp,"%.1f",TotalValue);
								break;
							}
							root["result"][ii]["v"]=szTmp;
							ii++;
						}
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

				int LastHour=-1;
				float LastTotal=-1;

				szQuery.clear();
				szQuery.str("");
				szQuery << "SELECT Total, Date FROM " << dbasetable << " WHERE (DeviceRowID==" << idx << ") ORDER BY Date ASC";
				result=m_pMain->m_sql.query(szQuery.str());
				if (result.size()>0)
				{
					std::vector<std::vector<std::string> >::const_iterator itt;
					int ii=0;
					for (itt=result.begin(); itt!=result.end(); ++itt)
					{
						std::vector<std::string> sd=*itt;

						int Hour=atoi(sd[1].substr(12,2).c_str());
						if (Hour!=LastHour)
						{
							if (LastHour!=-1)
							{
								root["result"][ii]["d"]=sd[1].substr(0,16);
								double mmval=(float)atof(sd[0].c_str())-LastTotal;
								mmval*=AddjMulti;
								sprintf(szTmp,"%.1f",mmval);
								root["result"][ii]["mm"]=szTmp;
								ii++;
							}
							LastHour=Hour;
							LastTotal=(float)atof(sd[0].c_str());
						}
					}
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
						sprintf(szTmp,"%.1f",float(intSpeed) * m_pMain->m_sql.m_windscale);
						root["result"][ii]["sp"]=szTmp;
						int intGust=atoi(sd[2].c_str());
						sprintf(szTmp,"%.1f",float(intGust) * m_pMain->m_sql.m_windscale);
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
				szQuery << "SELECT Direction, Speed FROM " << dbasetable << " WHERE (DeviceRowID==" << idx << ") ORDER BY Date ASC";
				result=m_pMain->m_sql.query(szQuery.str());
				if (result.size()>0)
				{
					std::vector<std::vector<std::string> >::const_iterator itt;
					std::map<int,int> _directions;
					float wdirtable[17][8];
					int wdirtabletemp[17][8];
					int ii=0;

					int totalvalues=0;
					//init dir list
					int idir;
					for (idir=0; idir<360+1; idir++)
						_directions[idir]=0;
					for (ii=0; ii<17; ii++)
					{
						for (int jj=0; jj<8; jj++)
						{
							wdirtable[ii][jj]=0;
							wdirtabletemp[ii][jj]=0;
						}
					}

					for (itt=result.begin(); itt!=result.end(); ++itt)
					{
						std::vector<std::string> sd=*itt;
						float fdirection=(float)atof(sd[0].c_str());
						if (fdirection==360)
							fdirection=0;
						int direction=int(fdirection);
						float speed=(float)atof(sd[1].c_str());
						int bucket=int(fdirection/22.5f);

						int speedpos=0;
						if (speed<0.5f) speedpos=0;
						else if (speed<2.0f) speedpos=1;
						else if (speed<4.0f) speedpos=2;
						else if (speed<6.0f) speedpos=3;
						else if (speed<8.0f) speedpos=4;
						else if (speed<10.0f) speedpos=5;
						else speedpos=6;
						wdirtabletemp[bucket][speedpos]++;
						_directions[direction]++;
						totalvalues++;
					}

					float totals[16];
					for (ii=0; ii<16; ii++)
					{
						totals[ii]=0;
					}
					for (ii=0; ii<16; ii++)
					{
						float total=0;
						for (int jj=0; jj<8; jj++)
						{
							float svalue=(100.0f/totalvalues)*wdirtabletemp[ii][jj];
							wdirtable[ii][jj]=svalue;
							total+=svalue;
						}
						wdirtable[ii][7]=total;
					}
					ii=0;
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
		else if (srange=="week")
		{
			if (sensor=="rain") {
				root["status"]="OK";
				root["title"]="Graph " + sensor + " " + srange;

				char szDateStart[40];
				char szDateEnd[40];

				struct tm ltime;
				ltime.tm_isdst=tm1.tm_isdst;
				ltime.tm_hour=0;
				ltime.tm_min=0;
				ltime.tm_sec=0;
				ltime.tm_year=tm1.tm_year;
				ltime.tm_mon=tm1.tm_mon;
				ltime.tm_mday=tm1.tm_mday;

				sprintf(szDateEnd,"%04d-%02d-%02d",ltime.tm_year+1900,ltime.tm_mon+1,ltime.tm_mday);

				//Subtract one week

				ltime.tm_mday -= 7;
				time_t later = mktime(&ltime);
				struct tm tm2;
				localtime_r(&later,&tm2);

				sprintf(szDateStart,"%04d-%02d-%02d",tm2.tm_year+1900,tm2.tm_mon+1,tm2.tm_mday);

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
						double mmval=atof(sd[0].c_str());
						mmval*=AddjMulti;
						sprintf(szTmp,"%.1f",mmval);
						root["result"][ii]["mm"]=szTmp;
						ii++;
					}
				}
				//add today (have to calculate it)
				szQuery.clear();
				szQuery.str("");
				if (dSubType!=sTypeRAINWU)
				{
					szQuery << "SELECT MIN(Total), MAX(Total), MAX(Rate) FROM Rain WHERE (DeviceRowID=" << idx << " AND Date>='" << szDateEnd << "')";
				}
				else
				{
					szQuery << "SELECT Total, Total, Rate FROM Rain WHERE (DeviceRowID=" << idx << " AND Date>='" << szDateEnd << "') ORDER BY ROWID DESC LIMIT 1";
				}
				result=m_pMain->m_sql.query(szQuery.str());
				if (result.size()>0)
				{
					std::vector<std::string> sd=result[0];

					float total_min=(float)atof(sd[0].c_str());
					float total_max=(float)atof(sd[1].c_str());
					int rate=atoi(sd[2].c_str());

					double total_real=0;
					if (dSubType!=sTypeRAINWU)
					{
						total_real=total_max-total_min;
					}
					else
					{
						total_real=total_max;
					}
					total_real*=AddjMulti;
					sprintf(szTmp,"%.1f",total_real);
					root["result"][ii]["d"]=szDateEnd;
					root["result"][ii]["mm"]=szTmp;
					ii++;
				}
			}
			else if (sensor=="counter") 
			{
				root["status"]="OK";
				root["title"]="Graph " + sensor + " " + srange;

				float EnergyDivider=1000.0f;
				float GasDivider=100.0f;
				float WaterDivider=100.0f;
				int tValue;
				if (m_pMain->m_sql.GetPreferencesVar("MeterDividerEnergy", tValue))
				{
					EnergyDivider=float(tValue);
				}
				if (m_pMain->m_sql.GetPreferencesVar("MeterDividerGas", tValue))
				{
					GasDivider=float(tValue);
				}
				if (m_pMain->m_sql.GetPreferencesVar("MeterDividerWater", tValue))
				{
					WaterDivider=float(tValue);
				}
				if (dType==pTypeP1Gas)
					GasDivider=1000;
				else if ((dType==pTypeENERGY)||(dType==pTypePOWER))
					EnergyDivider*=100.0f;
				//else if (dType==pTypeRFXMeter)
					//EnergyDivider*=1000.0f;

				char szDateStart[40];
				char szDateEnd[40];

				struct tm ltime;
				ltime.tm_isdst=tm1.tm_isdst;
				ltime.tm_hour=0;
				ltime.tm_min=0;
				ltime.tm_sec=0;
				ltime.tm_year=tm1.tm_year;
				ltime.tm_mon=tm1.tm_mon;
				ltime.tm_mday=tm1.tm_mday;

				sprintf(szDateEnd,"%04d-%02d-%02d",ltime.tm_year+1900,ltime.tm_mon+1,ltime.tm_mday);

				//Subtract one week

				ltime.tm_mday -= 7;
				time_t later = mktime(&ltime);
				struct tm tm2;
				localtime_r(&later,&tm2);
				sprintf(szDateStart,"%04d-%02d-%02d",tm2.tm_year+1900,tm2.tm_mon+1,tm2.tm_mday);

				szQuery.clear();
				szQuery.str("");
				int ii=0;
				if (dType==pTypeP1Power)
				{
					szQuery << "SELECT Value1,Value2,Value5,Value6,Date FROM " << dbasetable << " WHERE (DeviceRowID==" << idx << " AND Date>='" << szDateStart << "' AND Date<='" << szDateEnd << "') ORDER BY Date ASC";
					result=m_pMain->m_sql.query(szQuery.str());
					if (result.size()>0)
					{
						bool bHaveDeliverd=false;
						std::vector<std::vector<std::string> >::const_iterator itt;
						for (itt=result.begin(); itt!=result.end(); ++itt)
						{
							std::vector<std::string> sd=*itt;
							root["result"][ii]["d"]=sd[4].substr(0,16);
							std::string szValueUsage1=sd[0];
							std::string szValueDeliv1=sd[1];
							std::string szValueUsage2=sd[2];
							std::string szValueDeliv2=sd[3];

							float fUsage1=(float)(atof(szValueUsage1.c_str()));
							float fUsage2=(float)(atof(szValueUsage2.c_str()));
							float fDeliv1=(float)(atof(szValueDeliv1.c_str()));
							float fDeliv2=(float)(atof(szValueDeliv2.c_str()));

							if ((fDeliv1!=0)||(fDeliv2!=0))
								bHaveDeliverd=true;
							sprintf(szTmp,"%.3f",fUsage1/EnergyDivider);
							root["result"][ii]["v"]=szTmp;
							sprintf(szTmp,"%.3f",fUsage2/EnergyDivider);
							root["result"][ii]["v2"]=szTmp;
							sprintf(szTmp,"%.3f",fDeliv1/EnergyDivider);
							root["result"][ii]["r1"]=szTmp;
							sprintf(szTmp,"%.3f",fDeliv2/EnergyDivider);
							root["result"][ii]["r2"]=szTmp;
							ii++;
						}
						if (bHaveDeliverd)
						{
							root["delivered"]=true;
						}
					}
				}
				else
				{
					szQuery << "SELECT Value, Date FROM " << dbasetable << " WHERE (DeviceRowID==" << idx << " AND Date>='" << szDateStart << "' AND Date<='" << szDateEnd << "') ORDER BY Date ASC";
					result=m_pMain->m_sql.query(szQuery.str());
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
								sprintf(szTmp,"%.3f",atof(szValue.c_str())/EnergyDivider);
								szValue=szTmp;
								break;
							case MTYPE_GAS:
								sprintf(szTmp,"%.2f",atof(szValue.c_str())/GasDivider);
								szValue=szTmp;
								break;
							case MTYPE_WATER:
								sprintf(szTmp,"%.2f",atof(szValue.c_str())/WaterDivider);
								szValue=szTmp;
								break;
							}
							root["result"][ii]["v"]=szValue;
							ii++;
						}
					}
				}
				//add today (have to calculate it)
				szQuery.clear();
				szQuery.str("");
				if (dType==pTypeP1Power)
				{
					szQuery << "SELECT MIN(Value1), MAX(Value1), MIN(Value2), MAX(Value2),MIN(Value5), MAX(Value5), MIN(Value6), MAX(Value6) FROM MultiMeter WHERE (DeviceRowID=" << idx << " AND Date>='" << szDateEnd << "')";
					result=m_pMain->m_sql.query(szQuery.str());
					if (result.size()>0)
					{
						std::vector<std::string> sd=result[0];

						unsigned long long total_min_usage_1,total_min_usage_2,total_max_usage_1,total_max_usage_2,total_real_usage_1,total_real_usage_2;
						unsigned long long total_min_deliv_1,total_min_deliv_2,total_max_deliv_1,total_max_deliv_2,total_real_deliv_1,total_real_deliv_2;

						bool bHaveDeliverd=false;

						std::stringstream s_str1( sd[0] );
						s_str1 >> total_min_usage_1;
						std::stringstream s_str2( sd[1] );
						s_str2 >> total_max_usage_1;
						std::stringstream s_str3( sd[4] );
						s_str3 >> total_min_usage_2;
						std::stringstream s_str4( sd[5] );
						s_str4 >> total_max_usage_2;

						total_real_usage_1=total_max_usage_1-total_min_usage_1;
						total_real_usage_2=total_max_usage_2-total_min_usage_2;

						std::stringstream s_str5( sd[2] );
						s_str5 >> total_min_deliv_1;
						std::stringstream s_str6( sd[3] );
						s_str6 >> total_max_deliv_1;
						std::stringstream s_str7( sd[6] );
						s_str7 >> total_min_deliv_2;
						std::stringstream s_str8( sd[7] );
						s_str8 >> total_max_deliv_2;

						total_real_deliv_1=total_max_deliv_1-total_min_deliv_1;
						total_real_deliv_2=total_max_deliv_2-total_min_deliv_2;
						if ((total_real_deliv_1!=0)||(total_real_deliv_2!=0))
							bHaveDeliverd=true;

						root["result"][ii]["d"]=szDateEnd;

						sprintf(szTmp,"%llu",total_real_usage_1);
						std::string szValue=szTmp;
						sprintf(szTmp,"%.3f",atof(szValue.c_str())/EnergyDivider);
						root["result"][ii]["v"]=szTmp;
						sprintf(szTmp,"%llu",total_real_usage_2);
						szValue=szTmp;
						sprintf(szTmp,"%.3f",atof(szValue.c_str())/EnergyDivider);
						root["result"][ii]["v2"]=szTmp;

						sprintf(szTmp,"%llu",total_real_deliv_1);
						szValue=szTmp;
						sprintf(szTmp,"%.3f",atof(szValue.c_str())/EnergyDivider);
						root["result"][ii]["r1"]=szTmp;
						sprintf(szTmp,"%llu",total_real_deliv_2);
						szValue=szTmp;
						sprintf(szTmp,"%.3f",atof(szValue.c_str())/EnergyDivider);
						root["result"][ii]["r2"]=szTmp;

						ii++;
						if (bHaveDeliverd)
						{
							root["delivered"]=true;
						}
					}
				}
				else
				{
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
							sprintf(szTmp,"%.3f",atof(szValue.c_str())/EnergyDivider);
							szValue=szTmp;
							break;
						case MTYPE_GAS:
							sprintf(szTmp,"%.2f",atof(szValue.c_str())/GasDivider);
							szValue=szTmp;
							break;
						case MTYPE_WATER:
							sprintf(szTmp,"%.2f",atof(szValue.c_str())/WaterDivider);
							szValue=szTmp;
							break;
						}

						root["result"][ii]["d"]=szDateEnd;
						root["result"][ii]["v"]=szValue;
						ii++;
					}
				}
			}
		}//week
		else if ( (srange=="month") || (srange=="year" ) )
		{
			char szDateStart[40];
			char szDateEnd[40];

			struct tm ltime;
			ltime.tm_isdst=tm1.tm_isdst;
			ltime.tm_hour=0;
			ltime.tm_min=0;
			ltime.tm_sec=0;
			ltime.tm_year=tm1.tm_year;
			ltime.tm_mon=tm1.tm_mon;
			ltime.tm_mday=tm1.tm_mday;

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
			struct tm tm2;
			localtime_r(&later,&tm2);

			sprintf(szDateStart,"%04d-%02d-%02d",tm2.tm_year+1900,tm2.tm_mon+1,tm2.tm_mday);
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
							(dType==pTypeRego6XXTemp)||(dType==pTypeTEMP)||(dType==pTypeTEMP_HUM)||(dType==pTypeTEMP_HUM_BARO)||(dType==pTypeTEMP_BARO)||(dType==pTypeWIND)||(dType==pTypeThermostat1)||
							((dType==pTypeRFXSensor)&&(dSubType==sTypeRFXSensorTemp))||
							((dType==pTypeUV)&&(dSubType==sTypeUV3))
							)
						{
							bool bOK=true;
							if (dType==pTypeWIND)
							{
								bOK=((dSubType==sTypeWIND4)||(dSubType==sTypeWINDNoTemp));
							}
							if (bOK)
							{
								root["result"][ii]["te"]=sd[1];
								root["result"][ii]["tm"]=sd[0];
							}
						}
						if (
							((dType==pTypeWIND)&&(dSubType==sTypeWIND4))||
							((dType==pTypeWIND)&&(dSubType==sTypeWINDNoTemp))
							)
						{
							root["result"][ii]["ch"]=sd[3];
							root["result"][ii]["cm"]=sd[2];
						}
						if ((dType==pTypeHUM)||(dType==pTypeTEMP_HUM)||(dType==pTypeTEMP_HUM_BARO))
						{
							root["result"][ii]["hu"]=sd[4];
						}
						if (
							(dType==pTypeTEMP_HUM_BARO)||
							(dType==pTypeTEMP_BARO)
							)
						{
							if (dType==pTypeTEMP_HUM_BARO)
							{
								if (dSubType==sTypeTHBFloat)
								{
									sprintf(szTmp,"%.1f",atof(sd[5].c_str())/10.0f);
									root["result"][ii]["ba"]=szTmp;
								}
								else
									root["result"][ii]["ba"]=sd[5];
							}
							else if (dType==pTypeTEMP_BARO)
							{
								sprintf(szTmp,"%.1f",atof(sd[5].c_str())/10.0f);
								root["result"][ii]["ba"]=szTmp;
							}
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
					if (
						((dType==pTypeRego6XXTemp)||(dType==pTypeTEMP)||(dType==pTypeTEMP_HUM)||(dType==pTypeTEMP_HUM_BARO)||(dType==pTypeTEMP_BARO)||(dType==pTypeWIND)||(dType==pTypeThermostat1))||
						((dType==pTypeUV)&&(dSubType==sTypeUV3))||
						((dType==pTypeWIND)&&(dSubType==sTypeWIND4))||
						((dType==pTypeWIND)&&(dSubType==sTypeWINDNoTemp))
						)
					{
						root["result"][ii]["te"]=sd[1];
						root["result"][ii]["tm"]=sd[0];
					}
					if (
						((dType==pTypeWIND)&&(dSubType==sTypeWIND4))||
						((dType==pTypeWIND)&&(dSubType==sTypeWINDNoTemp))
						)
					{
						root["result"][ii]["ch"]=sd[3];
						root["result"][ii]["cm"]=sd[2];
					}
					if ((dType==pTypeHUM)||(dType==pTypeTEMP_HUM)||(dType==pTypeTEMP_HUM_BARO))
					{
						root["result"][ii]["hu"]=sd[4];
					}
					if (
						(dType==pTypeTEMP_HUM_BARO)||
						(dType==pTypeTEMP_BARO)
						)
					{
						if (dType==pTypeTEMP_HUM_BARO)
						{
							if (dSubType==sTypeTHBFloat)
							{
								sprintf(szTmp,"%.1f",atof(sd[5].c_str())/10.0f);
								root["result"][ii]["ba"]=szTmp;
							}
							else
								root["result"][ii]["ba"]=sd[5];
						}
						else if (dType==pTypeTEMP_BARO)
						{
							sprintf(szTmp,"%.1f",atof(sd[5].c_str())/10.0f);
							root["result"][ii]["ba"]=szTmp;
						}
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
						double mmval=atof(sd[0].c_str());
						mmval*=AddjMulti;
						sprintf(szTmp,"%.1f",mmval);
						root["result"][ii]["mm"]=szTmp;
						ii++;
					}
				}
				//add today (have to calculate it)
				szQuery.clear();
				szQuery.str("");
				if (dSubType!=sTypeRAINWU)
				{
					szQuery << "SELECT MIN(Total), MAX(Total), MAX(Rate) FROM Rain WHERE (DeviceRowID=" << idx << " AND Date>='" << szDateEnd << "')";
				}
				else
				{
					szQuery << "SELECT Total, Total, Rate FROM Rain WHERE (DeviceRowID=" << idx << " AND Date>='" << szDateEnd << "') ORDER BY ROWID DESC LIMIT 1";
				}
				result=m_pMain->m_sql.query(szQuery.str());
				if (result.size()>0)
				{
					std::vector<std::string> sd=result[0];

					float total_min=(float)atof(sd[0].c_str());
					float total_max=(float)atof(sd[1].c_str());
					int rate=atoi(sd[2].c_str());

					double total_real=0;
					if (dSubType!=sTypeRAINWU)
					{
						total_real=total_max-total_min;
					}
					else
					{
						total_real=total_max;
					}
					total_real*=AddjMulti;
					sprintf(szTmp,"%.1f",total_real);
					root["result"][ii]["d"]=szDateEnd;
					root["result"][ii]["mm"]=szTmp;
					ii++;
				}
			}
			else if (sensor=="counter") {
				root["status"]="OK";
				root["title"]="Graph " + sensor + " " + srange;

				float EnergyDivider=1000.0f;
				float GasDivider=100.0f;
				float WaterDivider=100.0f;
				int tValue;
				if (m_pMain->m_sql.GetPreferencesVar("MeterDividerEnergy", tValue))
				{
					EnergyDivider=float(tValue);
				}
				if (m_pMain->m_sql.GetPreferencesVar("MeterDividerGas", tValue))
				{
					GasDivider=float(tValue);
				}
				if (m_pMain->m_sql.GetPreferencesVar("MeterDividerWater", tValue))
				{
					WaterDivider=float(tValue);
				}
				if (dType==pTypeP1Gas)
					GasDivider=1000;
				else if ((dType==pTypeENERGY)||(dType==pTypePOWER))
					EnergyDivider*=100.0f;
//				else if (dType==pTypeRFXMeter)
	//				EnergyDivider*=1000.0f;

				szQuery.clear();
				szQuery.str("");
				int ii=0;
				if (dType==pTypeP1Power)
				{
					szQuery << "SELECT Value1,Value2,Value5,Value6, Date FROM " << dbasetable << " WHERE (DeviceRowID==" << idx << " AND Date>='" << szDateStart << "' AND Date<='" << szDateEnd << "') ORDER BY Date ASC";
					result=m_pMain->m_sql.query(szQuery.str());
					if (result.size()>0)
					{
						bool bHaveDeliverd=false;
						std::vector<std::vector<std::string> >::const_iterator itt;
						for (itt=result.begin(); itt!=result.end(); ++itt)
						{
							std::vector<std::string> sd=*itt;

							root["result"][ii]["d"]=sd[4].substr(0,16);

							std::string szUsage1=sd[0];
							std::string szDeliv1=sd[1];
							std::string szUsage2=sd[2];
							std::string szDeliv2=sd[3];

							float fUsage_1=(float)atof(szUsage1.c_str());
							float fUsage_2=(float)atof(szUsage2.c_str());
							float fDeliv_1=(float)atof(szDeliv1.c_str());
							float fDeliv_2=(float)atof(szDeliv2.c_str());

							if ((fDeliv_1!=0)||(fDeliv_2!=0))
								bHaveDeliverd=true;
							sprintf(szTmp,"%.3f",fUsage_1/EnergyDivider);
							root["result"][ii]["v"]=szTmp;
							sprintf(szTmp,"%.3f",fUsage_2/EnergyDivider);
							root["result"][ii]["v2"]=szTmp;
							sprintf(szTmp,"%.3f",fDeliv_1/EnergyDivider);
							root["result"][ii]["r1"]=szTmp;
							sprintf(szTmp,"%.3f",fDeliv_2/EnergyDivider);
							root["result"][ii]["r2"]=szTmp;
							ii++;
						}
						if (bHaveDeliverd)
						{
							root["delivered"]=true;
						}
					}
				}
				else if (dType==pTypeAirQuality)
				{//month/year
					root["status"]="OK";
					root["title"]="Graph " + sensor + " " + srange;

					szQuery << "SELECT Value1,Value2, Date FROM " << dbasetable << " WHERE (DeviceRowID==" << idx << " AND Date>='" << szDateStart << "' AND Date<='" << szDateEnd << "') ORDER BY Date ASC";
					result=m_pMain->m_sql.query(szQuery.str());
					if (result.size()>0)
					{
						std::vector<std::vector<std::string> >::const_iterator itt;
						for (itt=result.begin(); itt!=result.end(); ++itt)
						{
							std::vector<std::string> sd=*itt;

							root["result"][ii]["d"]=sd[2].substr(0,16);
							root["result"][ii]["co2_min"]=sd[0];
							root["result"][ii]["co2_max"]=sd[1];
							ii++;
						}
					}
				}
				else if (
					((dType==pTypeGeneral)&&((dSubType==sTypeSoilMoisture)||(dSubType==sTypeLeafWetness)))||
					((dType==pTypeRFXSensor)&&((dSubType==sTypeRFXSensorAD)||(dSubType==sTypeRFXSensorVolt)))
					)
				{//month/year
					root["status"]="OK";
					root["title"]="Graph " + sensor + " " + srange;

					szQuery << "SELECT Value1,Value2, Date FROM " << dbasetable << " WHERE (DeviceRowID==" << idx << " AND Date>='" << szDateStart << "' AND Date<='" << szDateEnd << "') ORDER BY Date ASC";
					result=m_pMain->m_sql.query(szQuery.str());
					if (result.size()>0)
					{
						std::vector<std::vector<std::string> >::const_iterator itt;
						for (itt=result.begin(); itt!=result.end(); ++itt)
						{
							std::vector<std::string> sd=*itt;

							root["result"][ii]["d"]=sd[2].substr(0,16);
							root["result"][ii]["v_min"]=sd[0];
							root["result"][ii]["v_max"]=sd[1];
							ii++;
						}
					}
				}
				else if (
					((dType==pTypeGeneral)&&(dSubType==sTypeVisibility))||
					((dType==pTypeGeneral)&&(dSubType==sTypeSolarRadiation))
					)
				{//month/year
					root["status"]="OK";
					root["title"]="Graph " + sensor + " " + srange;

					szQuery << "SELECT Value1,Value2, Date FROM " << dbasetable << " WHERE (DeviceRowID==" << idx << " AND Date>='" << szDateStart << "' AND Date<='" << szDateEnd << "') ORDER BY Date ASC";
					result=m_pMain->m_sql.query(szQuery.str());
					if (result.size()>0)
					{
						std::vector<std::vector<std::string> >::const_iterator itt;
						for (itt=result.begin(); itt!=result.end(); ++itt)
						{
							std::vector<std::string> sd=*itt;

							root["result"][ii]["d"]=sd[2].substr(0,16);

							float fValue1=float(atof(sd[0].c_str()))/10.0f;
							float fValue2=float(atof(sd[1].c_str()))/10.0f;
							if (metertype==1)
							{
								fValue1*=0.6214f;
								fValue2*=0.6214f;
							}
							sprintf(szTmp,"%.1f",fValue1);
							root["result"][ii]["v_min"]=szTmp;
							sprintf(szTmp,"%.1f",fValue2);
							root["result"][ii]["v_max"]=szTmp;
							ii++;
						}
					}
				}
				else if (dType==pTypeLux)
				{//month/year
					root["status"]="OK";
					root["title"]="Graph " + sensor + " " + srange;

					szQuery << "SELECT Value1,Value2, Date FROM " << dbasetable << " WHERE (DeviceRowID==" << idx << " AND Date>='" << szDateStart << "' AND Date<='" << szDateEnd << "') ORDER BY Date ASC";
					result=m_pMain->m_sql.query(szQuery.str());
					if (result.size()>0)
					{
						std::vector<std::vector<std::string> >::const_iterator itt;
						for (itt=result.begin(); itt!=result.end(); ++itt)
						{
							std::vector<std::string> sd=*itt;

							root["result"][ii]["d"]=sd[2].substr(0,16);
							root["result"][ii]["lux_min"]=sd[0];
							root["result"][ii]["lux_max"]=sd[1];
							ii++;
						}
					}
				}
				else if (dType==pTypeWEIGHT)
				{//month/year
					root["status"]="OK";
					root["title"]="Graph " + sensor + " " + srange;

					szQuery << "SELECT Value1,Value2, Date FROM " << dbasetable << " WHERE (DeviceRowID==" << idx << " AND Date>='" << szDateStart << "' AND Date<='" << szDateEnd << "') ORDER BY Date ASC";
					result=m_pMain->m_sql.query(szQuery.str());
					if (result.size()>0)
					{
						std::vector<std::vector<std::string> >::const_iterator itt;
						for (itt=result.begin(); itt!=result.end(); ++itt)
						{
							std::vector<std::string> sd=*itt;

							root["result"][ii]["d"]=sd[2].substr(0,16);
							root["result"][ii]["v_min"]=sd[0];
							root["result"][ii]["v_max"]=sd[1];
							ii++;
						}
					}
				}
				else if (dType==pTypeUsage)
				{//month/year
					root["status"]="OK";
					root["title"]="Graph " + sensor + " " + srange;

					szQuery << "SELECT Value1,Value2, Date FROM " << dbasetable << " WHERE (DeviceRowID==" << idx << " AND Date>='" << szDateStart << "' AND Date<='" << szDateEnd << "') ORDER BY Date ASC";
					result=m_pMain->m_sql.query(szQuery.str());
					if (result.size()>0)
					{
						std::vector<std::vector<std::string> >::const_iterator itt;
						for (itt=result.begin(); itt!=result.end(); ++itt)
						{
							std::vector<std::string> sd=*itt;

							root["result"][ii]["d"]=sd[2].substr(0,16);
							root["result"][ii]["u_min"]=sd[0];
							root["result"][ii]["u_max"]=sd[1];
							ii++;
						}
					}
				}
				else if (dType==pTypeCURRENT)
				{
					szQuery << "SELECT Value1,Value2,Value3,Value4,Value5,Value6, Date FROM " << dbasetable << " WHERE (DeviceRowID==" << idx << " AND Date>='" << szDateStart << "' AND Date<='" << szDateEnd << "') ORDER BY Date ASC";
					result=m_pMain->m_sql.query(szQuery.str());
					if (result.size()>0)
					{
						//CM113
						int displaytype=0;
						int voltage=230;
						m_pMain->m_sql.GetPreferencesVar("CM113DisplayType", displaytype);
						m_pMain->m_sql.GetPreferencesVar("ElectricVoltage", voltage);

						root["displaytype"]=displaytype;

						bool bHaveL1=false;
						bool bHaveL2=false;
						bool bHaveL3=false;
						std::vector<std::vector<std::string> >::const_iterator itt;
						for (itt=result.begin(); itt!=result.end(); ++itt)
						{
							std::vector<std::string> sd=*itt;

							root["result"][ii]["d"]=sd[6].substr(0,16);

							float fval1=(float)atof(sd[0].c_str())/10.0f;
							float fval2=(float)atof(sd[1].c_str())/10.0f;
							float fval3=(float)atof(sd[2].c_str())/10.0f;
							float fval4=(float)atof(sd[3].c_str())/10.0f;
							float fval5=(float)atof(sd[4].c_str())/10.0f;
							float fval6=(float)atof(sd[5].c_str())/10.0f;

							if ((fval1!=0)||(fval2!=0))
								bHaveL1=true;
							if ((fval3!=0)||(fval4!=0))
								bHaveL2=true;
							if ((fval5!=0)||(fval6!=0))
								bHaveL3=true;

							if (displaytype==0)
							{
								sprintf(szTmp,"%.1f",fval1);
								root["result"][ii]["v1"]=szTmp;
								sprintf(szTmp,"%.1f",fval2);
								root["result"][ii]["v2"]=szTmp;
								sprintf(szTmp,"%.1f",fval3);
								root["result"][ii]["v3"]=szTmp;
								sprintf(szTmp,"%.1f",fval4);
								root["result"][ii]["v4"]=szTmp;
								sprintf(szTmp,"%.1f",fval5);
								root["result"][ii]["v5"]=szTmp;
								sprintf(szTmp,"%.1f",fval6);
								root["result"][ii]["v6"]=szTmp;
							}
							else
							{
								sprintf(szTmp,"%d",int(fval1*voltage));
								root["result"][ii]["v1"]=szTmp;
								sprintf(szTmp,"%d",int(fval2*voltage));
								root["result"][ii]["v2"]=szTmp;
								sprintf(szTmp,"%d",int(fval3*voltage));
								root["result"][ii]["v3"]=szTmp;
								sprintf(szTmp,"%d",int(fval4*voltage));
								root["result"][ii]["v4"]=szTmp;
								sprintf(szTmp,"%d",int(fval5*voltage));
								root["result"][ii]["v5"]=szTmp;
								sprintf(szTmp,"%d",int(fval6*voltage));
								root["result"][ii]["v6"]=szTmp;
							}

							ii++;
						}
						if (bHaveL1)
							root["haveL1"]=true;
						if (bHaveL2)
							root["haveL2"]=true;
						if (bHaveL3)
							root["haveL3"]=true;
					}
				}
				else if (dType==pTypeCURRENTENERGY)
				{
					szQuery << "SELECT Value1,Value2,Value3,Value4,Value5,Value6, Date FROM " << dbasetable << " WHERE (DeviceRowID==" << idx << " AND Date>='" << szDateStart << "' AND Date<='" << szDateEnd << "') ORDER BY Date ASC";
					result=m_pMain->m_sql.query(szQuery.str());
					if (result.size()>0)
					{
						//CM180i
						int displaytype=0;
						int voltage=230;
						m_pMain->m_sql.GetPreferencesVar("CM113DisplayType", displaytype);
						m_pMain->m_sql.GetPreferencesVar("ElectricVoltage", voltage);

						root["displaytype"]=displaytype;

						bool bHaveL1=false;
						bool bHaveL2=false;
						bool bHaveL3=false;
						std::vector<std::vector<std::string> >::const_iterator itt;
						for (itt=result.begin(); itt!=result.end(); ++itt)
						{
							std::vector<std::string> sd=*itt;

							root["result"][ii]["d"]=sd[6].substr(0,16);

							float fval1=(float)atof(sd[0].c_str())/10.0f;
							float fval2=(float)atof(sd[1].c_str())/10.0f;
							float fval3=(float)atof(sd[2].c_str())/10.0f;
							float fval4=(float)atof(sd[3].c_str())/10.0f;
							float fval5=(float)atof(sd[4].c_str())/10.0f;
							float fval6=(float)atof(sd[5].c_str())/10.0f;

							if ((fval1!=0)||(fval2!=0))
								bHaveL1=true;
							if ((fval3!=0)||(fval4!=0))
								bHaveL2=true;
							if ((fval5!=0)||(fval6!=0))
								bHaveL3=true;

							if (displaytype==0)
							{
								sprintf(szTmp,"%.1f",fval1);
								root["result"][ii]["v1"]=szTmp;
								sprintf(szTmp,"%.1f",fval2);
								root["result"][ii]["v2"]=szTmp;
								sprintf(szTmp,"%.1f",fval3);
								root["result"][ii]["v3"]=szTmp;
								sprintf(szTmp,"%.1f",fval4);
								root["result"][ii]["v4"]=szTmp;
								sprintf(szTmp,"%.1f",fval5);
								root["result"][ii]["v5"]=szTmp;
								sprintf(szTmp,"%.1f",fval6);
								root["result"][ii]["v6"]=szTmp;
							}
							else
							{
								sprintf(szTmp,"%d",int(fval1*voltage));
								root["result"][ii]["v1"]=szTmp;
								sprintf(szTmp,"%d",int(fval2*voltage));
								root["result"][ii]["v2"]=szTmp;
								sprintf(szTmp,"%d",int(fval3*voltage));
								root["result"][ii]["v3"]=szTmp;
								sprintf(szTmp,"%d",int(fval4*voltage));
								root["result"][ii]["v4"]=szTmp;
								sprintf(szTmp,"%d",int(fval5*voltage));
								root["result"][ii]["v5"]=szTmp;
								sprintf(szTmp,"%d",int(fval6*voltage));
								root["result"][ii]["v6"]=szTmp;
							}

							ii++;
						}
						if (bHaveL1)
							root["haveL1"]=true;
						if (bHaveL2)
							root["haveL2"]=true;
						if (bHaveL3)
							root["haveL3"]=true;
					}
				}
				else
				{
					szQuery << "SELECT Value, Date FROM " << dbasetable << " WHERE (DeviceRowID==" << idx << " AND Date>='" << szDateStart << "' AND Date<='" << szDateEnd << "') ORDER BY Date ASC";
					result=m_pMain->m_sql.query(szQuery.str());
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
								sprintf(szTmp,"%.3f",atof(szValue.c_str())/EnergyDivider);
								szValue=szTmp;
								break;
							case MTYPE_GAS:
								sprintf(szTmp,"%.2f",atof(szValue.c_str())/GasDivider);
								szValue=szTmp;
								break;
							case MTYPE_WATER:
								sprintf(szTmp,"%.2f",atof(szValue.c_str())/WaterDivider);
								szValue=szTmp;
								break;
							}
							root["result"][ii]["d"]=sd[1].substr(0,16);
							root["result"][ii]["v"]=szValue;
							ii++;
						}
					}
				}
				//add today (have to calculate it)
				szQuery.clear();
				szQuery.str("");
				if (dType==pTypeP1Power)
				{
					szQuery << "SELECT MIN(Value1), MAX(Value1), MIN(Value2), MAX(Value2), MIN(Value5), MAX(Value5), MIN(Value6), MAX(Value6) FROM MultiMeter WHERE (DeviceRowID=" << idx << " AND Date>='" << szDateEnd << "')";
					bool bHaveDeliverd=false;
					result=m_pMain->m_sql.query(szQuery.str());
					if (result.size()>0)
					{
						std::vector<std::string> sd=result[0];
						unsigned long long total_min_usage_1,total_min_usage_2,total_max_usage_1,total_max_usage_2,total_real_usage_1,total_real_usage_2;
						unsigned long long total_min_deliv_1,total_min_deliv_2,total_max_deliv_1,total_max_deliv_2,total_real_deliv_1,total_real_deliv_2;

						std::stringstream s_str1( sd[0] );
						s_str1 >> total_min_usage_1;
						std::stringstream s_str2( sd[1] );
						s_str2 >> total_max_usage_1;
						std::stringstream s_str3( sd[4] );
						s_str3 >> total_min_usage_2;
						std::stringstream s_str4( sd[5] );
						s_str4 >> total_max_usage_2;

						total_real_usage_1=total_max_usage_1-total_min_usage_1;
						total_real_usage_2=total_max_usage_2-total_min_usage_2;

						std::stringstream s_str5( sd[2] );
						s_str5 >> total_min_deliv_1;
						std::stringstream s_str6( sd[3] );
						s_str6 >> total_max_deliv_1;
						std::stringstream s_str7( sd[6] );
						s_str7 >> total_min_deliv_2;
						std::stringstream s_str8( sd[7] );
						s_str8 >> total_max_deliv_2;

						total_real_deliv_1=total_max_deliv_1-total_min_deliv_1;
						total_real_deliv_2=total_max_deliv_2-total_min_deliv_2;

						if ((total_real_deliv_1!=0)||(total_real_deliv_2!=0))
							bHaveDeliverd=true;

						root["result"][ii]["d"]=szDateEnd;

						std::string szValue;

						sprintf(szTmp,"%llu",total_real_usage_1);
						szValue=szTmp;
						sprintf(szTmp,"%.3f",atof(szValue.c_str())/EnergyDivider);
						root["result"][ii]["v"]=szTmp;
						sprintf(szTmp,"%llu",total_real_usage_2);
						szValue=szTmp;
						sprintf(szTmp,"%.3f",atof(szValue.c_str())/EnergyDivider);
						root["result"][ii]["v2"]=szTmp;

						sprintf(szTmp,"%llu",total_real_deliv_1);
						szValue=szTmp;
						sprintf(szTmp,"%.3f",atof(szValue.c_str())/EnergyDivider);
						root["result"][ii]["r1"]=szTmp;
						sprintf(szTmp,"%llu",total_real_deliv_2);
						szValue=szTmp;
						sprintf(szTmp,"%.3f",atof(szValue.c_str())/EnergyDivider);
						root["result"][ii]["r2"]=szTmp;

						ii++;
					}
					if (bHaveDeliverd)
					{
						root["delivered"]=true;
					}
				}
				else if (dType==pTypeAirQuality)
				{
					szQuery << "SELECT MIN(Value), MAX(Value) FROM Meter WHERE (DeviceRowID=" << idx << " AND Date>='" << szDateEnd << "')";
					result=m_pMain->m_sql.query(szQuery.str());
					if (result.size()>0)
					{
						root["result"][ii]["d"]=szDateEnd;
						root["result"][ii]["co2_min"]=result[0][0];
						root["result"][ii]["co2_max"]=result[0][1];
						ii++;
					}
				}
				else if (
					((dType==pTypeGeneral)&&((dSubType==sTypeSoilMoisture)||(dSubType==sTypeLeafWetness)))||
					((dType==pTypeRFXSensor)&&((dSubType==sTypeRFXSensorAD)||(dSubType==sTypeRFXSensorVolt)))
					)
				{
					szQuery << "SELECT MIN(Value), MAX(Value) FROM Meter WHERE (DeviceRowID=" << idx << " AND Date>='" << szDateEnd << "')";
					result=m_pMain->m_sql.query(szQuery.str());
					if (result.size()>0)
					{
						root["result"][ii]["d"]=szDateEnd;
						root["result"][ii]["v_min"]=result[0][0];
						root["result"][ii]["v_max"]=result[0][1];
						ii++;
					}
				}
				else if (
					((dType==pTypeGeneral)&&(dSubType==sTypeVisibility))||
					((dType==pTypeGeneral)&&(dSubType==sTypeSolarRadiation))
					)
				{
					szQuery << "SELECT MIN(Value), MAX(Value) FROM Meter WHERE (DeviceRowID=" << idx << " AND Date>='" << szDateEnd << "')";
					result=m_pMain->m_sql.query(szQuery.str());
					if (result.size()>0)
					{
						root["result"][ii]["d"]=szDateEnd;
						float fValue1=float(atof(result[0][0].c_str()))/10.0f;
						float fValue2=float(atof(result[0][1].c_str()))/10.0f;
						if (metertype==1)
						{
							fValue1*=0.6214f;
							fValue2*=0.6214f;
						}

						sprintf(szTmp,"%.1f",fValue1);
						root["result"][ii]["v_min"]=szTmp;
						sprintf(szTmp,"%.1f",fValue2);
						root["result"][ii]["v_max"]=szTmp;
						ii++;
					}
				}
				else if (dType==pTypeLux)
				{
					szQuery << "SELECT MIN(Value), MAX(Value) FROM Meter WHERE (DeviceRowID=" << idx << " AND Date>='" << szDateEnd << "')";
					result=m_pMain->m_sql.query(szQuery.str());
					if (result.size()>0)
					{
						root["result"][ii]["d"]=szDateEnd;
						root["result"][ii]["lux_min"]=result[0][0];
						root["result"][ii]["lux_max"]=result[0][1];
						ii++;
					}
				}
				else if (dType==pTypeWEIGHT)
				{
					szQuery << "SELECT MIN(Value), MAX(Value) FROM Meter WHERE (DeviceRowID=" << idx << " AND Date>='" << szDateEnd << "')";
					result=m_pMain->m_sql.query(szQuery.str());
					if (result.size()>0)
					{
						root["result"][ii]["d"]=szDateEnd;
						root["result"][ii]["v_min"]=result[0][0];
						root["result"][ii]["v_max"]=result[0][1];
						ii++;
					}
				}
				else if (dType==pTypeUsage)
				{
					szQuery << "SELECT MIN(Value), MAX(Value) FROM Meter WHERE (DeviceRowID=" << idx << " AND Date>='" << szDateEnd << "')";
					result=m_pMain->m_sql.query(szQuery.str());
					if (result.size()>0)
					{
						root["result"][ii]["d"]=szDateEnd;
						root["result"][ii]["u_min"]=result[0][0];
						root["result"][ii]["u_max"]=result[0][1];
						ii++;
					}
				}
				else
				{
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
							sprintf(szTmp,"%.3f",atof(szValue.c_str())/EnergyDivider);
							szValue=szTmp;
							break;
						case MTYPE_GAS:
							sprintf(szTmp,"%.2f",atof(szValue.c_str())/GasDivider);
							szValue=szTmp;
							break;
						case MTYPE_WATER:
							sprintf(szTmp,"%.2f",atof(szValue.c_str())/WaterDivider);
							szValue=szTmp;
							break;
						}

						root["result"][ii]["d"]=szDateEnd;
						root["result"][ii]["v"]=szValue;
						ii++;
					}
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
						sprintf(szTmp,"%.1f",float(intSpeed) * m_pMain->m_sql.m_windscale);
						root["result"][ii]["sp"]=szTmp;
						int intGust=atoi(sd[4].c_str());
						sprintf(szTmp,"%.1f",float(intGust) * m_pMain->m_sql.m_windscale);
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
					sprintf(szTmp,"%.1f",float(intSpeed) * m_pMain->m_sql.m_windscale);
					root["result"][ii]["sp"]=szTmp;
					int intGust=atoi(sd[4].c_str());
					sprintf(szTmp,"%.1f",float(intGust) * m_pMain->m_sql.m_windscale);
					root["result"][ii]["gu"]=szTmp;
					ii++;
				}
			}
		}//month or year
		else if ((srange.substr(0,1)=="2") && (srange.substr(10,1)=="T") && (srange.substr(11,1)=="2")) // custom range 2013-01-01T2013-12-31
		{
            std::string szDateStart=srange.substr(0,10);
            std::string szDateEnd=srange.substr(11,10);
 		    std::string sgraphtype=m_pWebEm->FindValue("graphtype");
 		    std::string sgraphTemp=m_pWebEm->FindValue("graphTemp");
 		    std::string sgraphChill=m_pWebEm->FindValue("graphChill");
 		    std::string sgraphHum=m_pWebEm->FindValue("graphHum");
 		    std::string sgraphBaro=m_pWebEm->FindValue("graphBaro");
 		    std::string sgraphDew=m_pWebEm->FindValue("graphDew");
          
			if (sensor=="temp") {
				root["status"]="OK";
				root["title"]="Graph " + sensor + " " + srange;

                bool sendTemp = false;
                bool sendChill = false;
                bool sendHum = false;
                bool sendBaro = false;
                bool sendDew = false;

			    if ((sgraphTemp=="true") && 
				    ((dType==pTypeRego6XXTemp)||(dType==pTypeTEMP)||(dType==pTypeTEMP_HUM)||(dType==pTypeTEMP_HUM_BARO)||(dType==pTypeTEMP_BARO)||(dType==pTypeWIND)||(dType==pTypeThermostat1)||
				    ((dType==pTypeUV)&&(dSubType==sTypeUV3))||
				    ((dType==pTypeWIND)&&(dSubType==sTypeWIND4))||
				    ((dType==pTypeWIND)&&(dSubType==sTypeWINDNoTemp))||
				    ((dType==pTypeRFXSensor)&&(dSubType==sTypeRFXSensorTemp)))
				    )
			    {
                    sendTemp = true;
			    }
			    if ((sgraphChill=="true") &&
				    (((dType==pTypeWIND)&&(dSubType==sTypeWIND4))||
				    ((dType==pTypeWIND)&&(dSubType==sTypeWINDNoTemp)))
				    )
			    {
                    sendChill = true;
			    }
			    if ((sgraphHum=="true") &&
                    ((dType==pTypeHUM)||(dType==pTypeTEMP_HUM)||(dType==pTypeTEMP_HUM_BARO))
                    )
			    {
                    sendHum = true;
			    }
			    if ((sgraphBaro=="true") && ((dType==pTypeTEMP_HUM_BARO)||(dType==pTypeTEMP_BARO)))
			    {
                    sendBaro = true;
                }
			    if ((sgraphDew=="true") && ((dType==pTypeTEMP_HUM)||(dType==pTypeTEMP_HUM_BARO)))
			    {
                    sendDew = true;
                }

				szQuery.clear();
				szQuery.str("");
                if(sgraphtype=="1")
                {
                    // Need to get all values of the end date so 23:59:59 is appended to the date string
				    szQuery << "SELECT Temperature, Chill, Humidity, Barometer, Date, DewPoint FROM Temperature WHERE (DeviceRowID==" << idx << " AND Date>='" << szDateStart << "' AND Date<='" << szDateEnd << " 23:59:59') ORDER BY Date ASC";
				    result=m_pMain->m_sql.query(szQuery.str());
				    int ii=0;
				    if (result.size()>0)
				    {
					    std::vector<std::vector<std::string> >::const_iterator itt;
					    for (itt=result.begin(); itt!=result.end(); ++itt)
					    {
						    std::vector<std::string> sd=*itt;

						    root["result"][ii]["d"]=sd[4];//.substr(0,16);
						    if (sendTemp)
						    {
							    root["result"][ii]["te"]=sd[0];
							    root["result"][ii]["tm"]=sd[0];
						    }
						    if (sendChill)
						    {
							    root["result"][ii]["ch"]=sd[1];
							    root["result"][ii]["cm"]=sd[1];
						    }
						    if (sendHum)
						    {
							    root["result"][ii]["hu"]=sd[2];
						    }
						    if (sendBaro)
						    {
								if (dType==pTypeTEMP_HUM_BARO)
								{
									if (dSubType==sTypeTHBFloat)
									{
										sprintf(szTmp,"%.1f",atof(sd[3].c_str())/10.0f);
										root["result"][ii]["ba"]=szTmp;
									}
									else
										root["result"][ii]["ba"]=sd[3];
								}
								else if (dType==pTypeTEMP_BARO)
								{
									sprintf(szTmp,"%.1f",atof(sd[3].c_str())/10.0f);
									root["result"][ii]["ba"]=szTmp;
								}
						    }
						    if (sendDew)
						    {
							    root["result"][ii]["dp"]=sd[5];
						    }
						    ii++;
					    }
                    }
				}
                else
                {
				    szQuery << "SELECT Temp_Min, Temp_Max, Chill_Min, Chill_Max, Humidity, Barometer, Date, DewPoint FROM Temperature_Calendar WHERE (DeviceRowID==" << idx << " AND Date>='" << szDateStart << "' AND Date<='" << szDateEnd << "') ORDER BY Date ASC";
				    result=m_pMain->m_sql.query(szQuery.str());
				    int ii=0;
				    if (result.size()>0)
				    {
					    std::vector<std::vector<std::string> >::const_iterator itt;
					    for (itt=result.begin(); itt!=result.end(); ++itt)
					    {
						    std::vector<std::string> sd=*itt;

						    root["result"][ii]["d"]=sd[6].substr(0,16);
						    if (sendTemp)
						    {
							    root["result"][ii]["te"]=sd[1];
							    root["result"][ii]["tm"]=sd[0];
						    }
						    if (sendChill)
						    {
							    root["result"][ii]["ch"]=sd[3];
							    root["result"][ii]["cm"]=sd[2];
						    }
						    if (sendHum)
						    {
							    root["result"][ii]["hu"]=sd[4];
						    }
						    if (sendBaro)
						    {
								if (dType==pTypeTEMP_HUM_BARO)
								{
									if (dSubType==sTypeTHBFloat)
									{
										sprintf(szTmp,"%.1f",atof(sd[5].c_str())/10.0f);
										root["result"][ii]["ba"]=szTmp;
									}
									else
										root["result"][ii]["ba"]=sd[5];
								}
								else if (dType==pTypeTEMP_BARO)
								{
									sprintf(szTmp,"%.1f",atof(sd[5].c_str())/10.0f);
									root["result"][ii]["ba"]=szTmp;
								}
						    }
						    if (sendDew)
						    {
							    root["result"][ii]["dp"]=sd[7];
						    }
						    ii++;
					    }
                    }

				    //add today (have to calculate it)
				    szQuery.clear();
				    szQuery.str("");
				    szQuery << "SELECT MIN(Temperature), MAX(Temperature), MIN(Chill), MAX(Chill), MAX(Humidity), MAX(Barometer), MIN(DewPoint) FROM Temperature WHERE (DeviceRowID=" << idx << " AND Date>='" << szDateEnd << "')";
				    result=m_pMain->m_sql.query(szQuery.str());
				    if (result.size()>0)
				    {
					    std::vector<std::string> sd=result[0];

					    root["result"][ii]["d"]=szDateEnd;
					    if (sendTemp)
					    {
						    root["result"][ii]["te"]=sd[1];
						    root["result"][ii]["tm"]=sd[0];
					    }
					    if (sendChill)
					    {
						    root["result"][ii]["ch"]=sd[3];
						    root["result"][ii]["cm"]=sd[2];
					    }
					    if (sendHum)
					    {
						    root["result"][ii]["hu"]=sd[4];
					    }
					    if (sendBaro)
					    {
							if (dType==pTypeTEMP_HUM_BARO)
							{
								if (dSubType==sTypeTHBFloat)
								{
									sprintf(szTmp,"%.1f",atof(sd[5].c_str())/10.0f);
									root["result"][ii]["ba"]=szTmp;
								}
								else
									root["result"][ii]["ba"]=sd[5];
							}
							else if (dType==pTypeTEMP_BARO)
							{
								sprintf(szTmp,"%.1f",atof(sd[5].c_str())/10.0f);
								root["result"][ii]["ba"]=szTmp;
							}
					    }
					    if (sendDew)
					    {
						    root["result"][ii]["dp"]=sd[6];
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
				if (dSubType!=sTypeRAINWU)
				{
					szQuery << "SELECT MIN(Total), MAX(Total), MAX(Rate) FROM Rain WHERE (DeviceRowID=" << idx << " AND Date>='" << szDateEnd << "')";
				}
				else
				{
					szQuery << "SELECT Total, Total, Rate FROM Rain WHERE (DeviceRowID=" << idx << " AND Date>='" << szDateEnd << "') ORDER BY ROWID DESC LIMIT 1";
				}
				result=m_pMain->m_sql.query(szQuery.str());
				if (result.size()>0)
				{
					std::vector<std::string> sd=result[0];

					float total_min=(float)atof(sd[0].c_str());
					float total_max=(float)atof(sd[1].c_str());
					int rate=atoi(sd[2].c_str());

					float total_real=0;
					if (dSubType!=sTypeRAINWU)
					{
						total_real=total_max-total_min;
					}
					else
					{
						total_real=total_max;
					}
					sprintf(szTmp,"%.1f",total_real);
					root["result"][ii]["d"]=szDateEnd;
					root["result"][ii]["mm"]=szTmp;
					ii++;
				}
			}
			else if (sensor=="counter") {
				root["status"]="OK";
				root["title"]="Graph " + sensor + " " + srange;

				float EnergyDivider=1000.0f;
				float GasDivider=100.0f;
				float WaterDivider=100.0f;
				int tValue;
				if (m_pMain->m_sql.GetPreferencesVar("MeterDividerEnergy", tValue))
				{
					EnergyDivider=float(tValue);
				}
				if (m_pMain->m_sql.GetPreferencesVar("MeterDividerGas", tValue))
				{
					GasDivider=float(tValue);
				}
				if (m_pMain->m_sql.GetPreferencesVar("MeterDividerWater", tValue))
				{
					WaterDivider=float(tValue);
				}
				if (dType==pTypeP1Gas)
					GasDivider=1000;
				else if ((dType==pTypeENERGY)||(dType==pTypePOWER))
					EnergyDivider*=100.0f;

				szQuery.clear();
				szQuery.str("");
				int ii=0;
				if (dType==pTypeP1Power)
				{
					szQuery << "SELECT Value1,Value2,Value5,Value6, Date FROM " << dbasetable << " WHERE (DeviceRowID==" << idx << " AND Date>='" << szDateStart << "' AND Date<='" << szDateEnd << "') ORDER BY Date ASC";
					result=m_pMain->m_sql.query(szQuery.str());
					if (result.size()>0)
					{
						bool bHaveDeliverd=false;
						std::vector<std::vector<std::string> >::const_iterator itt;
						for (itt=result.begin(); itt!=result.end(); ++itt)
						{
							std::vector<std::string> sd=*itt;

							root["result"][ii]["d"]=sd[4].substr(0,16);

							std::string szUsage1=sd[0];
							std::string szDeliv1=sd[1];
							std::string szUsage2=sd[2];
							std::string szDeliv2=sd[3];

							float fUsage=(float)(atof(szUsage1.c_str())+atof(szUsage2.c_str()));
							float fDeliv=(float)(atof(szDeliv1.c_str())+atof(szDeliv2.c_str()));

							if (fDeliv!=0)
								bHaveDeliverd=true;
							sprintf(szTmp,"%.3f",fUsage/EnergyDivider);
							root["result"][ii]["v"]=szTmp;
							sprintf(szTmp,"%.3f",fDeliv/EnergyDivider);
							root["result"][ii]["v2"]=szTmp;
							ii++;
						}
						if (bHaveDeliverd)
						{
							root["delivered"]=true;
						}
					}
				}
				else
				{
					szQuery << "SELECT Value, Date FROM " << dbasetable << " WHERE (DeviceRowID==" << idx << " AND Date>='" << szDateStart << "' AND Date<='" << szDateEnd << "') ORDER BY Date ASC";
					result=m_pMain->m_sql.query(szQuery.str());
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
								sprintf(szTmp,"%.3f",atof(szValue.c_str())/EnergyDivider);
								szValue=szTmp;
								break;
							case MTYPE_GAS:
								sprintf(szTmp,"%.2f",atof(szValue.c_str())/GasDivider);
								szValue=szTmp;
								break;
							case MTYPE_WATER:
								sprintf(szTmp,"%.2f",atof(szValue.c_str())/WaterDivider);
								szValue=szTmp;
								break;
							}
							root["result"][ii]["d"]=sd[1].substr(0,16);
							root["result"][ii]["v"]=szValue;
							ii++;
						}
					}
				}
				//add today (have to calculate it)
				szQuery.clear();
				szQuery.str("");
				if (dType==pTypeP1Power)
				{
					szQuery << "SELECT MIN(Value1), MAX(Value1), MIN(Value2), MAX(Value2),MIN(Value5), MAX(Value5), MIN(Value6), MAX(Value6) FROM MultiMeter WHERE (DeviceRowID=" << idx << " AND Date>='" << szDateEnd << "')";
					bool bHaveDeliverd=false;
					result=m_pMain->m_sql.query(szQuery.str());
					if (result.size()>0)
					{
						std::vector<std::string> sd=result[0];
						unsigned long long total_min_usage_1,total_min_usage_2,total_max_usage_1,total_max_usage_2,total_real_usage;
						unsigned long long total_min_deliv_1,total_min_deliv_2,total_max_deliv_1,total_max_deliv_2,total_real_deliv;

						std::stringstream s_str1( sd[0] );
						s_str1 >> total_min_usage_1;
						std::stringstream s_str2( sd[1] );
						s_str2 >> total_max_usage_1;
						std::stringstream s_str3( sd[4] );
						s_str3 >> total_min_usage_2;
						std::stringstream s_str4( sd[5] );
						s_str4 >> total_max_usage_2;

						total_real_usage=(total_max_usage_1+total_max_usage_2)-(total_min_usage_1+total_min_usage_2);

						std::stringstream s_str5( sd[2] );
						s_str5 >> total_min_deliv_1;
						std::stringstream s_str6( sd[3] );
						s_str6 >> total_max_deliv_1;
						std::stringstream s_str7( sd[6] );
						s_str7 >> total_min_deliv_2;
						std::stringstream s_str8( sd[7] );
						s_str8 >> total_max_deliv_2;

						total_real_deliv=(total_max_deliv_1+total_max_deliv_2)-(total_min_deliv_1+total_min_deliv_2);

						if (total_real_deliv!=0)
							bHaveDeliverd=true;

						root["result"][ii]["d"]=szDateEnd;

						sprintf(szTmp,"%llu",total_real_usage);
						std::string szValue=szTmp;
						sprintf(szTmp,"%.3f",atof(szValue.c_str())/EnergyDivider);
						root["result"][ii]["v"]=szTmp;
						sprintf(szTmp,"%llu",total_real_deliv);
						szValue=szTmp;
						sprintf(szTmp,"%.3f",atof(szValue.c_str())/EnergyDivider);
						root["result"][ii]["v2"]=szTmp;
						ii++;
						if (bHaveDeliverd)
						{
							root["delivered"]=true;
						}
					}
				}
				else
				{
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
							sprintf(szTmp,"%.3f",atof(szValue.c_str())/EnergyDivider);
							szValue=szTmp;
							break;
						case MTYPE_GAS:
							sprintf(szTmp,"%.2f",atof(szValue.c_str())/GasDivider);
							szValue=szTmp;
							break;
						case MTYPE_WATER:
							sprintf(szTmp,"%.2f",atof(szValue.c_str())/WaterDivider);
							szValue=szTmp;
							break;
						}

						root["result"][ii]["d"]=szDateEnd;
						root["result"][ii]["v"]=szValue;
						ii++;
					}
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
						sprintf(szTmp,"%.1f",float(intSpeed) * m_pMain->m_sql.m_windscale);
						root["result"][ii]["sp"]=szTmp;
						int intGust=atoi(sd[4].c_str());
						sprintf(szTmp,"%.1f",float(intGust) * m_pMain->m_sql.m_windscale);
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
					sprintf(szTmp,"%.1f",float(intSpeed) * m_pMain->m_sql.m_windscale);
					root["result"][ii]["sp"]=szTmp;
					int intGust=atoi(sd[4].c_str());
					sprintf(szTmp,"%.1f",float(intGust) * m_pMain->m_sql.m_windscale);
					root["result"][ii]["gu"]=szTmp;
					ii++;
				}
			}
		}//custom range
	}
	else if (rtype=="command")
	{
		std::string cparam=m_pWebEm->FindValue("param");
		if (cparam=="")
			goto exitjson;
		if (cparam=="getversion")
		{
			char *szVersion=DisplayVersion();
			root["status"]="OK";
			root["title"]="GetVersion";
			root["version"]=szVersion;
		}
		else if (cparam=="getlog")
		{
			root["status"]="OK";
			root["title"]="GetLog";

			time_t lastlogtime=0;
			std::string slastlogtime=m_pWebEm->FindValue("lastlogtime");
			if (slastlogtime!="")
			{
				std::stringstream s_str( slastlogtime );
				s_str >> lastlogtime;
			}

			std::list<CLogger::_tLogLineStruct> logmessages=_log.GetLog();
			std::list<CLogger::_tLogLineStruct>::const_iterator itt;
			int ii=0;
			for (itt=logmessages.begin(); itt!=logmessages.end(); ++itt)
			{
				if (itt->logtime>lastlogtime) 
				{
					std::stringstream szLogTime;
					szLogTime << itt->logtime;
					root["LastLogTime"]=szLogTime.str();
					root["result"][ii]["level"]=(int)itt->level;
					root["result"][ii]["message"]=itt->logmessage;
					ii++;
				}
			}
		}
		else if (cparam=="addplan")
		{
			std::string name=m_pWebEm->FindValue("name");
			root["status"]="OK";
			root["title"]="AddPlan";
			sprintf(szTmp,
				"INSERT INTO Plans (Name) VALUES ('%s')",
				name.c_str()
				);
			result=m_pMain->m_sql.query(szTmp);
		}
		else if (cparam=="updateplan")
		{
			std::string idx=m_pWebEm->FindValue("idx");
			if (idx=="")
				goto exitjson;
			std::string name=m_pWebEm->FindValue("name");
			if (
				(name=="")
				)
				goto exitjson;

			root["status"]="OK";
			root["title"]="UpdatePlan";

			sprintf(szTmp,
				"UPDATE Plans SET Name='%s' WHERE (ID == %s)",
				name.c_str(),
				idx.c_str()
				);
			result=m_pMain->m_sql.query(szTmp);
		}
		else if (cparam=="deleteplan")
		{
			std::string idx=m_pWebEm->FindValue("idx");
			if (idx=="")
				goto exitjson;
			root["status"]="OK";
			root["title"]="DeletePlan";
			sprintf(szTmp,
				"DELETE FROM DeviceToPlansMap WHERE (PlanID == %s)",
				idx.c_str()
				);
			result=m_pMain->m_sql.query(szTmp);
			sprintf(szTmp,
				"DELETE FROM Plans WHERE (ID == %s)",
				idx.c_str()
				);
			result=m_pMain->m_sql.query(szTmp);
		}
		else if (cparam=="getunusedplandevices")
		{
			root["status"]="OK";
			root["title"]="GetUnusedPlanDevices";
			std::string sunique=m_pWebEm->FindValue("unique");
			if (sunique=="")
				goto exitjson;
			int iUnique=(sunique=="true")?1:0;
			int ii=0;

			szQuery.clear();
			szQuery.str("");
			szQuery << "SELECT ID, Name FROM DeviceStatus WHERE (Used==1) ORDER BY Name";
			result=m_pMain->m_sql.query(szQuery.str());
			if (result.size()>0)
			{
				std::vector<std::vector<std::string> >::const_iterator itt;
				for (itt=result.begin(); itt!=result.end(); ++itt)
				{
					std::vector<std::string> sd=*itt;

					bool bDoAdd=true;
					if (iUnique)
					{
						szQuery.clear();
						szQuery.str("");
						szQuery << "SELECT ID FROM DeviceToPlansMap WHERE (DeviceRowID==" << sd[0] << ") AND (DevSceneType==0)";
						result2=m_pMain->m_sql.query(szQuery.str());
						bDoAdd=(result2.size()==0);
					}
					if (bDoAdd)
					{
						root["result"][ii]["type"]=0;
						root["result"][ii]["idx"]=sd[0];
						root["result"][ii]["Name"]=sd[1];
						ii++;
					}
				}
			}
			//Add Scenes
			szQuery.clear();
			szQuery.str("");
			szQuery << "SELECT ID, Name FROM Scenes ORDER BY Name";
			result=m_pMain->m_sql.query(szQuery.str());
			if (result.size()>0)
			{
				std::vector<std::vector<std::string> >::const_iterator itt;
				for (itt=result.begin(); itt!=result.end(); ++itt)
				{
					std::vector<std::string> sd=*itt;

					bool bDoAdd=true;
					if (iUnique)
					{
						szQuery.clear();
						szQuery.str("");
						szQuery << "SELECT ID FROM DeviceToPlansMap WHERE (DeviceRowID==" << sd[0] << ") AND (DevSceneType==1)";
						result2=m_pMain->m_sql.query(szQuery.str());
						bDoAdd=(result2.size()==0);
					}
					if (bDoAdd)
					{
						root["result"][ii]["type"]=1;
						root["result"][ii]["idx"]=sd[0];
						root["result"][ii]["Name"]="[Scene] " + sd[1];
						ii++;
					}
				}
			}//end light/switches

		}
		else if (cparam=="addplanactivedevice")
		{
			std::string idx=m_pWebEm->FindValue("idx");
			std::string sactivetype=m_pWebEm->FindValue("activetype");
			std::string activeidx=m_pWebEm->FindValue("activeidx");
			if (
				(idx=="")||
				(sactivetype=="")||
				(activeidx=="")
				)
				goto exitjson;
			root["status"]="OK";
			root["title"]="AddPlanActiveDevice";

			int activetype=atoi(sactivetype.c_str());

			//check if it is not already there
			szQuery.clear();
			szQuery.str("");
			szQuery << "SELECT ID FROM DeviceToPlansMap WHERE (DeviceRowID==" << activeidx << ") AND (DevSceneType=="<< activetype << ") AND (PlanID==" << idx << ")";
			result2=m_pMain->m_sql.query(szQuery.str());
			if (result2.size()==0)
			{
				sprintf(szTmp,
					"INSERT INTO DeviceToPlansMap (DevSceneType,DeviceRowID, PlanID) VALUES (%d,%s,%s)",
					activetype,
					activeidx.c_str(),
					idx.c_str()
					);
				result=m_pMain->m_sql.query(szTmp);
			}
		}
		else if (cparam=="getplandevices")
		{
			std::string idx=m_pWebEm->FindValue("idx");
			if (idx=="")
				goto exitjson;
			root["status"]="OK";
			root["title"]="GetPlanDevices";
			std::vector<std::vector<std::string> > result;
			std::stringstream szQuery;
			
			szQuery << "SELECT ID, DevSceneType, DeviceRowID, [Order] FROM DeviceToPlansMap WHERE (PlanID=='" << idx << "') ORDER BY [Order]";
			result=m_pMain->m_sql.query(szQuery.str());
			if (result.size()>0)
			{
				std::vector<std::vector<std::string> >::const_iterator itt;
				int ii=0;
				for (itt=result.begin(); itt!=result.end(); ++itt)
				{
					std::vector<std::string> sd=*itt;

					std::string ID=sd[0];
					int DevSceneType=atoi(sd[1].c_str());
					std::string DevSceneRowID=sd[2];

					std::string Name="";
					if (DevSceneType==0)
					{
						std::vector<std::vector<std::string> > result2;
						std::stringstream szQuery2;
						szQuery2 << "SELECT Name FROM DeviceStatus WHERE (ID=='" << DevSceneRowID << "')";
						result2=m_pMain->m_sql.query(szQuery2.str());
						if (result2.size()>0)
						{
							Name=result2[0][0];
						}
					}
					else
					{
						std::vector<std::vector<std::string> > result2;
						std::stringstream szQuery2;
						szQuery2 << "SELECT Name FROM Scenes WHERE (ID=='" << DevSceneRowID << "')";
						result2=m_pMain->m_sql.query(szQuery2.str());
						if (result2.size()>0)
						{
							Name="[Scene] " + result2[0][0];
						}
					}
					if (Name!="")
					{
						root["result"][ii]["idx"]=ID;
						root["result"][ii]["type"]=DevSceneType;
						root["result"][ii]["DevSceneRowID"]=DevSceneRowID;
						root["result"][ii]["order"]=sd[3];
						root["result"][ii]["Name"]=Name;
						ii++;
					}
				}
			}
		}
		else if (cparam=="deleteplandevice")
		{
			std::string idx=m_pWebEm->FindValue("idx");
			if (idx=="")
				goto exitjson;
			root["status"]="OK";
			root["title"]="DeletePlanDevice";
			sprintf(szTmp,"DELETE FROM DeviceToPlansMap WHERE (ID == '%s')",idx.c_str());
			result=m_pMain->m_sql.query(szTmp);
		}
		else if (cparam=="deleteallplandevices")
		{
			std::string idx=m_pWebEm->FindValue("idx");
			if (idx=="")
				goto exitjson;
			root["status"]="OK";
			root["title"]="DeleteAllPlanDevices";
			sprintf(szTmp,"DELETE FROM DeviceToPlansMap WHERE (PlanID == '%s')",idx.c_str());
			result=m_pMain->m_sql.query(szTmp);
		}
		else if (cparam=="changeplanorder")
		{
			std::string idx=m_pWebEm->FindValue("idx");
			if (idx=="")
				goto exitjson;
			std::string sway=m_pWebEm->FindValue("way");
			if (sway=="")
				goto exitjson;
			bool bGoUp=(sway=="0");

			std::string aOrder,oID,oOrder;

			std::vector<std::vector<std::string> > result;
			std::stringstream szQuery;
			szQuery << "SELECT [Order] FROM Plans WHERE (ID=='" << idx << "')";
			result=m_pMain->m_sql.query(szQuery.str());
			if (result.size()<1)
				goto exitjson;
			aOrder=result[0][0];

			szQuery.clear();
			szQuery.str("");

			if (!bGoUp)
			{
				//Get next device order
				szQuery << "SELECT ID, [Order] FROM Plans WHERE ([Order]>'" << aOrder << "') ORDER BY [Order] ASC";
				result=m_pMain->m_sql.query(szQuery.str());
				if (result.size()<1)
					goto exitjson;
				oID=result[0][0];
				oOrder=result[0][1];
			}
			else
			{
				//Get previous device order
				szQuery << "SELECT ID, [Order] FROM Plans WHERE ([Order]<'" << aOrder << "') ORDER BY [Order] DESC";
				result=m_pMain->m_sql.query(szQuery.str());
				if (result.size()<1)
					goto exitjson;
				oID=result[0][0];
				oOrder=result[0][1];
			}
			//Swap them
			root["status"]="OK";
			root["title"]="ChangePlanOrder";

			szQuery.clear();
			szQuery.str("");
			szQuery << "UPDATE Plans SET [Order] = '" << oOrder << "' WHERE (ID='" << idx << "')";
			result=m_pMain->m_sql.query(szQuery.str());
			szQuery.clear();
			szQuery.str("");
			szQuery << "UPDATE Plans SET [Order] = '" << aOrder << "' WHERE (ID='" << oID << "')";
			result=m_pMain->m_sql.query(szQuery.str());
		}
		else if (cparam=="changeplandeviceorder")
		{
			std::string planid=m_pWebEm->FindValue("planid");
			std::string idx=m_pWebEm->FindValue("idx");
			std::string sway=m_pWebEm->FindValue("way");
			if (
				(planid=="")||
				(idx=="")||
				(sway=="")
				)
				goto exitjson;
			bool bGoUp=(sway=="0");

			std::string aOrder,oID,oOrder;

			std::vector<std::vector<std::string> > result;
			std::stringstream szQuery;
			szQuery << "SELECT [Order] FROM DeviceToPlansMap WHERE ((ID=='" << idx << "') AND (PlanID=='" << planid << "'))";
			result=m_pMain->m_sql.query(szQuery.str());
			if (result.size()<1)
				goto exitjson;
			aOrder=result[0][0];

			szQuery.clear();
			szQuery.str("");

			if (!bGoUp)
			{
				//Get next device order
				szQuery << "SELECT ID, [Order] FROM DeviceToPlansMap WHERE (([Order]>'" << aOrder << "') AND (PlanID=='" << planid << "')) ORDER BY [Order] ASC";
				result=m_pMain->m_sql.query(szQuery.str());
				if (result.size()<1)
					goto exitjson;
				oID=result[0][0];
				oOrder=result[0][1];
			}
			else
			{
				//Get previous device order
				szQuery << "SELECT ID, [Order] FROM DeviceToPlansMap WHERE (([Order]<'" << aOrder << "') AND (PlanID=='" << planid << "')) ORDER BY [Order] DESC";
				result=m_pMain->m_sql.query(szQuery.str());
				if (result.size()<1)
					goto exitjson;
				oID=result[0][0];
				oOrder=result[0][1];
			}
			//Swap them
			root["status"]="OK";
			root["title"]="ChangePlanOrder";

			szQuery.clear();
			szQuery.str("");
			szQuery << "UPDATE DeviceToPlansMap SET [Order] = '" << oOrder << "' WHERE (ID='" << idx << "')";
			result=m_pMain->m_sql.query(szQuery.str());
			szQuery.clear();
			szQuery.str("");
			szQuery << "UPDATE DeviceToPlansMap SET [Order] = '" << aOrder << "' WHERE (ID='" << oID << "')";
			result=m_pMain->m_sql.query(szQuery.str());
		}
		else if (cparam=="getactualhistory")
		{
			root["status"]="OK";
			root["title"]="GetActualHistory";

			std::string historyfile = szStartupFolder + "History.txt";

			std::ifstream infile;
			int ii=0;
			infile.open(historyfile.c_str());
			std::string sLine;
			if (infile.is_open())
			{
				while (!infile.eof())
				{
					getline(infile, sLine);
					root["LastLogTime"]="";
					if (sLine.find("Version ")==0)
						root["result"][ii]["level"]=1;
					else
						root["result"][ii]["level"]=0;
					root["result"][ii]["message"]=sLine;
					ii++;
				}
			}
		}
		else if (cparam=="logincheck")
		{
			std::string tmpusrname=m_pWebEm->FindValue("username");
			std::string tmpusrpass=m_pWebEm->FindValue("password");
			if (
				(tmpusrname=="")||
				(tmpusrpass=="")
				)
				goto exitjson;

			std::string usrname;
			std::string usrpass;
			if (request_handler::url_decode(tmpusrname,usrname))
			{
				if (request_handler::url_decode(tmpusrpass,usrpass))
				{
					usrname=base64_decode(usrname);
					usrpass=base64_decode(usrpass);
					int iUser=-1;
					iUser=FindUser(usrname.c_str());
					if (iUser==-1)
						goto exitjson;
					if (m_users[iUser].Password!=usrpass)
						goto exitjson;
					root["status"]="OK";
					root["title"]="logincheck";
				}
			}
		}
		else if (cparam=="getnewhistory")
		{
			root["status"]="OK";
			root["title"]="GetNewHistory";

			std::string historyfile;
			int nValue;
			m_pMain->m_sql.GetPreferencesVar("ReleaseChannel", nValue);
			bool bIsBetaChannel=(nValue!=0);

			std::string szHistoryURL="http://domoticz.sourceforge.net/History.txt";
			if (bIsBetaChannel)
				szHistoryURL="http://domoticz.sourceforge.net/beta/History.txt";
			if (!HTTPClient::GET(szHistoryURL,historyfile))
			{
				historyfile="Unable to get Online History document !!";
			}

			std::istringstream stream(historyfile);
			std::string sLine;
			int ii=0;
			while (std::getline(stream, sLine)) 
			{
				root["LastLogTime"]="";
				if (sLine.find("Version ")==0)
					root["result"][ii]["level"]=1;
				else
					root["result"][ii]["level"]=0;
				root["result"][ii]["message"]=sLine;
				ii++;
			}
		}
		else if (cparam=="getactivetabs")
		{
			root["status"]="OK";
			root["title"]="GetActiveTabs";

			bool bHaveUser=(m_pWebEm->m_actualuser!="");
			int urights=3;
			unsigned long UserID=0;
			if (bHaveUser)
			{
				int iUser=-1;
				iUser=FindUser(m_pWebEm->m_actualuser.c_str());
				if (iUser!=-1)
				{
					urights=(int)m_users[iUser].userrights;
					UserID=m_users[iUser].ID;
				}
				
			}
			root["statuscode"]=urights;

			int nValue;
			std::string sValue;

			if (m_pMain->m_sql.GetPreferencesVar("Language", sValue))
			{
				root["language"]=sValue;
			}
			if (m_pMain->m_sql.GetPreferencesVar("MobileType", nValue))
			{
				root["MobileType"]=nValue;
			}

			root["WindScale"]=m_pMain->m_sql.m_windscale*10.0f;
			root["WindSign"]=m_pMain->m_sql.m_windsign;

			int bEnableTabLight=1;
			int bEnableTabScenes=1;
			int bEnableTabTemp=1;
			int bEnableTabWeather=1;
			int bEnableTabUtility=1;

			if ((UserID!=0)&&(UserID!=10000)) {
				szQuery.clear();
				szQuery.str("");
				szQuery << "SELECT TabsEnabled FROM Users WHERE (ID==" << UserID << ")";
				result=m_pMain->m_sql.query(szQuery.str());
				if (result.size()>0)
				{
					int TabsEnabled=atoi(result[0][0].c_str());
					bEnableTabLight=(TabsEnabled&(1<<0));
					bEnableTabScenes=(TabsEnabled&(1<<1));
					bEnableTabTemp=(TabsEnabled&(1<<2));
					bEnableTabWeather=(TabsEnabled&(1<<3));
					bEnableTabUtility=(TabsEnabled&(1<<4));
				}
			}
			else
			{
				m_pMain->m_sql.GetPreferencesVar("EnableTabLights", bEnableTabLight);
				m_pMain->m_sql.GetPreferencesVar("EnableTabScenes", bEnableTabScenes);
				m_pMain->m_sql.GetPreferencesVar("EnableTabTemp", bEnableTabTemp);
				m_pMain->m_sql.GetPreferencesVar("EnableTabWeather", bEnableTabWeather);
				m_pMain->m_sql.GetPreferencesVar("EnableTabUtility", bEnableTabUtility);
			}

			root["result"]["EnableTabLights"]=bEnableTabLight;
			root["result"]["EnableTabScenes"]=bEnableTabScenes;
			root["result"]["EnableTabTemp"]=bEnableTabTemp;
			root["result"]["EnableTabWeather"]=bEnableTabWeather;
			root["result"]["EnableTabUtility"]=bEnableTabUtility;
		}
		else if (cparam=="sendnotification")
		{
			std::string subject=m_pWebEm->FindValue("subject");
			std::string body=m_pWebEm->FindValue("body");
			if (
				(subject=="")||
				(body=="")
				)
				goto exitjson;
			//Add to queue
			m_pMain->m_sql.SendNotificationEx(subject,body);
			root["status"]="OK";
			root["title"]="SendNotification";
		}
		else if (cparam=="emailcamerasnapshot")
		{
			std::string camidx=m_pWebEm->FindValue("camidx");
			std::string subject=m_pWebEm->FindValue("subject");
			if (
				(camidx=="")||
				(subject=="")
				)
				goto exitjson;
			//Add to queue
			m_pMain->m_sql.AddTaskItem(_tTaskItem::EmailCameraSnapshot(1,camidx,subject));
			root["status"]="OK";
			root["title"]="Email Camera Snapshot";
		}
		else if (cparam=="udevice")
		{
			std::string nvalue=m_pWebEm->FindValue("nvalue");
			std::string svalue=m_pWebEm->FindValue("svalue");

			std::string idx=m_pWebEm->FindValue("idx");

			std::string hid=m_pWebEm->FindValue("hid");
			std::string did=m_pWebEm->FindValue("did");
			std::string dunit=m_pWebEm->FindValue("dunit");
			std::string dtype=m_pWebEm->FindValue("dtype");
			std::string dsubtype=m_pWebEm->FindValue("dsubtype");

			if (idx!="")
			{
				//Get device parameters
				szQuery.clear();
				szQuery.str("");
				szQuery << "SELECT HardwareID, DeviceID, Unit, Type, SubType FROM DeviceStatus WHERE (ID==" << idx << ")";
				result=m_pMain->m_sql.query(szQuery.str());
				if (result.size()>0)
				{
					hid=result[0][0];
					did=result[0][1];
					dunit=result[0][2];
					dtype=result[0][3];
					dsubtype=result[0][4];
				}
			}

			if (
				(hid=="")||
				(did=="")||
				(dunit=="")||
				(dtype=="")||
				(dsubtype=="")
				)
				goto exitjson;
			if ((nvalue=="")&&(svalue==""))
				goto exitjson;

			root["status"]="OK";
			root["title"]="Update Device";

			std::string devname="Unknown";
			m_pMain->m_sql.UpdateValue(
				atoi(hid.c_str()),
				did.c_str(),
				(const unsigned char)atoi(dunit.c_str()),
				(const unsigned char)atoi(dtype.c_str()),
				(const unsigned char)atoi(dsubtype.c_str()),
				12,//signal level,
				255,//battery level
				(const int)atoi(nvalue.c_str()),
				svalue.c_str(),
				devname
				);
		}
		else if (cparam=="system_shutdown")
		{
#ifdef WIN32
			system("shutdown -s -f -t 1 -d up:125:1");
#else
			system("sudo shutdown -h now");
#endif
			root["status"]="OK";
			root["title"]="SystemShutdown";
		}
		else if (cparam=="system_reboot")
		{
#ifdef WIN32
			system("shutdown -r -f -t 1 -d up:125:1");
#else
			system("sudo shutdown -r now");
#endif
			root["status"]="OK";
			root["title"]="SystemReboot";
		}
		else if (cparam=="execute_script")
		{
			std::string scriptname=m_pWebEm->FindValue("scriptname");
			if (scriptname=="")
				goto exitjson;
			if (scriptname.find("..")!=std::string::npos)
				goto exitjson;
#ifdef WIN32
			scriptname = szStartupFolder + "scripts\\" + scriptname;
#else
			scriptname = szStartupFolder + "scripts/" + scriptname;
#endif
			if (!file_exist(scriptname.c_str()))
				goto exitjson;
			std::string script_params=m_pWebEm->FindValue("scriptparams");
			std::string strparm=szStartupFolder;
			if (script_params!="")
			{
				if (strparm.size()>0)
					strparm+=" " + script_params;
				else
					strparm=script_params;
			}
			std::string sdirect=m_pWebEm->FindValue("direct");
			if (sdirect=="true")
			{
#ifdef WIN32
				ShellExecute(NULL,"open",scriptname.c_str(),strparm.c_str(),NULL,SW_SHOWNORMAL);
#else
				std::string lscript=scriptname + " " + strparm;
				system(lscript.c_str());
#endif
			}
			else
			{
				//add script to background worker
				m_pMain->m_sql.AddTaskItem(_tTaskItem::ExecuteScript(1,scriptname,strparm));
			}
			root["status"]="OK";
			root["title"]="ExecuteScript";
		}
		else if (cparam=="getelectragascosts")
		{
			std::string idx=m_pWebEm->FindValue("idx");
			if (idx=="")
				goto exitjson;
			szQuery.clear();
			szQuery.str("");
			szQuery << "SELECT Type, SubType, nValue, sValue FROM DeviceStatus WHERE (ID==" << idx << ")";
			result=m_pMain->m_sql.query(szQuery.str());
			if (result.size()>0)
			{
				std::vector<std::string> sd=result[0];

				int nValue=0;
				root["status"]="OK";
				root["title"]="GetElectraCosts";
				m_pMain->m_sql.GetPreferencesVar("CostEnergy",nValue);
				root["CostEnergy"]=nValue;
				m_pMain->m_sql.GetPreferencesVar("CostEnergyT2",nValue);
				root["CostEnergyT2"]=nValue;
				m_pMain->m_sql.GetPreferencesVar("CostGas",nValue);
				root["CostGas"]=nValue;

				unsigned char dType=atoi(sd[0].c_str());
				unsigned char subType=atoi(sd[1].c_str());
				nValue=(unsigned char)atoi(sd[2].c_str());
				std::string sValue=sd[3];

				if (dType == pTypeP1Power)
				{
					//also provide the counter values

					std::vector<std::string> splitresults;
					StringSplit(sValue, ";", splitresults);
					if (splitresults.size()!=6)
						goto exitjson;

					float EnergyDivider=1000.0f;
					int tValue;
					if (m_pMain->m_sql.GetPreferencesVar("MeterDividerEnergy", tValue))
					{
						EnergyDivider=float(tValue);
					}

					unsigned long long powerusage1;
					unsigned long long powerusage2;
					unsigned long long powerdeliv1;
					unsigned long long powerdeliv2;
					unsigned long long usagecurrent;
					unsigned long long delivcurrent;

					std::stringstream s_powerusage1(splitresults[0]);
					std::stringstream s_powerusage2(splitresults[1]);
					std::stringstream s_powerdeliv1(splitresults[2]);
					std::stringstream s_powerdeliv2(splitresults[3]);
					std::stringstream s_usagecurrent(splitresults[4]);
					std::stringstream s_delivcurrent(splitresults[5]);

					s_powerusage1 >> powerusage1;
					s_powerusage2 >> powerusage2;
					s_powerdeliv1 >> powerdeliv1;
					s_powerdeliv2 >> powerdeliv2;
					s_usagecurrent >> usagecurrent;
					s_delivcurrent >> delivcurrent;

					sprintf(szTmp,"%.03f",float(powerusage1)/EnergyDivider);
					root["CounterT1"]=szTmp;
					sprintf(szTmp,"%.03f",float(powerusage2)/EnergyDivider);
					root["CounterT2"]=szTmp;
					sprintf(szTmp,"%.03f",float(powerdeliv1)/EnergyDivider);
					root["CounterR1"]=szTmp;
					sprintf(szTmp,"%.03f",float(powerdeliv2)/EnergyDivider);
					root["CounterR2"]=szTmp;
				}

			}
		}
		else if (cparam=="checkforupdate")
		{
			bool bHaveUser=(m_pWebEm->m_actualuser!="");
			int urights=3;
			if (bHaveUser)
			{
				int iUser=-1;
				iUser=FindUser(m_pWebEm->m_actualuser.c_str());
				if (iUser!=-1)
					urights=(int)m_users[iUser].userrights;
			}
			root["statuscode"]=urights;
			int nValue=1;
			m_pMain->m_sql.GetPreferencesVar("5MinuteHistoryDays", nValue);
			root["5MinuteHistoryDays"]=nValue;

			utsname my_uname;
			if (uname(&my_uname)<0)
				goto exitjson;

			std::string forced=m_pWebEm->FindValue("forced");
			bool bIsForced=(forced=="true");
			std::string systemname=my_uname.sysname;
			std::string machine=my_uname.machine;
			std::transform(systemname.begin(), systemname.end(), systemname.begin(), ::tolower);
			if ((systemname=="windows")||((machine!="armv6l")&&(machine!="armv7l")))
			{
				//Only Raspberry Pi (Wheezy) for now!
				root["status"]="OK";
				root["title"]="CheckForUpdate";
				root["HaveUpdate"]=false;
				root["IsSupported"]=false;
			}
			else
			{
				int nValue=0;
				m_pMain->m_sql.GetPreferencesVar("UseAutoUpdate",nValue);
				if (nValue==1)
				{
					m_pMain->m_sql.GetPreferencesVar("ReleaseChannel", nValue);
					bool bIsBetaChannel=(nValue!=0);
					std::string szURL="http://domoticz.sourceforge.net/version_" + systemname + "_" + machine + ".h";
					std::string szHistoryURL="http://domoticz.sourceforge.net/History.txt";
					//std::string szURL="http://domoticz.sourceforge.net/svnversion.h";
					if (bIsBetaChannel)
					{
						//szURL="http://domoticz.sourceforge.net/beta/svnversion.h";
						szURL="http://domoticz.sourceforge.net/beta/version_" + systemname + "_" + machine + ".h";
						szHistoryURL="http://domoticz.sourceforge.net/beta/History.txt";
					}
					std::string revfile;

					if (!HTTPClient::GET(szURL,revfile))
						goto exitjson;
					std::vector<std::string> strarray;
					StringSplit(revfile, " ", strarray);
					if (strarray.size()!=3)
						goto exitjson;
					root["status"]="OK";
					root["title"]="CheckForUpdate";
					root["IsSupported"]=true;

					int version=atoi(szAppVersion.substr(szAppVersion.find(".")+1).c_str());
					bool bHaveUpdate=(version<atoi(strarray[2].c_str()));
					if ((bHaveUpdate)&&(!bIsForced))
					{
						time_t atime=mytime(NULL);
						if (atime-m_LastUpdateCheck<12*3600)
						{
							bHaveUpdate=false;
						}
						else
							m_LastUpdateCheck=atime;
					}
					root["HaveUpdate"]=bHaveUpdate;
					root["Revision"]=atoi(strarray[2].c_str());
					root["HistoryURL"]=szHistoryURL;
					root["ActVersion"]=version;
				}
				else
				{
					root["status"]="OK";
					root["title"]="CheckForUpdate";
					root["IsSupported"]=false;
					root["HaveUpdate"]=false;
				}
			}
		}
		else if (cparam=="downloadupdate")
		{
			int nValue;
			m_pMain->m_sql.GetPreferencesVar("ReleaseChannel", nValue);
			bool bIsBetaChannel=(nValue!=0);
			std::string szURL;
			std::string revfile;

			utsname my_uname;
			if (uname(&my_uname)<0)
				goto exitjson;

			std::string forced=m_pWebEm->FindValue("forced");
			bool bIsForced=(forced=="true");
			std::string systemname=my_uname.sysname;
			std::string machine=my_uname.machine;
			std::transform(systemname.begin(), systemname.end(), systemname.begin(), ::tolower);

			if (!bIsBetaChannel)
			{
				//szURL="http://domoticz.sourceforge.net/svnversion.h";
				szURL="http://domoticz.sourceforge.net/version_" + systemname + "_" + machine + ".h";
				//HTTPClient::GET("http://www.domoticz.com/pwiki/piwik.php?idsite=1&amp;rec=1&amp;action_name=DownloadNewVersion&amp;idgoal=2",revfile);
			}
			else
			{
				//szURL="http://domoticz.sourceforge.net/beta/svnversion.h";
				szURL="http://domoticz.sourceforge.net/beta/version_" + systemname + "_" + machine + ".h";
				//HTTPClient::GET("http://www.domoticz.com/pwiki/piwik.php?idsite=1&amp;rec=1&amp;action_name=DownloadNewVersion&amp;idgoal=1",revfile);
			}
			if (!HTTPClient::GET(szURL,revfile))
				goto exitjson;
			std::vector<std::string> strarray;
			StringSplit(revfile, " ", strarray);
			if (strarray.size()!=3)
				goto exitjson;
			int version=atoi(szAppVersion.substr(szAppVersion.find(".")+1).c_str());
			if (version>=atoi(strarray[2].c_str()))
				goto exitjson;
			if (((machine!="armv6l")&&(machine!="armv7l"))||(strstr(my_uname.release,"ARCH+")!=NULL))
				goto exitjson;	//only Raspberry Pi for now
			root["status"]="OK";
			root["title"]="DownloadUpdate";
			std::string downloadURL="http://domoticz.sourceforge.net/domoticz_" + systemname + "_" + machine + ".tgz";
			if (bIsBetaChannel)
				downloadURL="http://domoticz.sourceforge.net/beta/domoticz_" + systemname + "_" + machine + ".tgz";
			m_pMain->GetDomoticzUpdate(downloadURL);
		}
		else if (cparam=="downloadready")
		{
			if (!m_pMain->m_bHaveDownloadedDomoticzUpdate)
				goto exitjson;
			root["status"]="OK";
			root["title"]="DownloadReady";
			root["downloadok"]=(m_pMain->m_bHaveDownloadedDomoticzUpdateSuccessFull)?true:false;
		}
		else if (cparam=="deletedatapoint")
		{
			const std::string idx=m_pWebEm->FindValue("idx");
			const std::string Date=m_pWebEm->FindValue("date");
			if (
				(idx=="")||
				(Date=="")
				)
				goto exitjson;
			root["status"]="OK";
			root["title"]="deletedatapoint";
			m_pMain->m_sql.DeleteDataPoint(idx.c_str(),Date.c_str());
		}
		else if (cparam=="deleteallsubdevices")
		{
			std::string idx=m_pWebEm->FindValue("idx");
			if (idx=="")
				goto exitjson;
			root["status"]="OK";
			root["title"]="DeleteAllSubDevices";
			sprintf(szTmp,"DELETE FROM LightSubDevices WHERE (ParentID == %s)",idx.c_str());
			result=m_pMain->m_sql.query(szTmp);
		}
		else if (cparam=="deletesubdevice")
		{
			std::string idx=m_pWebEm->FindValue("idx");
			if (idx=="")
				goto exitjson;
			root["status"]="OK";
			root["title"]="DeleteSubDevice";
			sprintf(szTmp,"DELETE FROM LightSubDevices WHERE (ID == %s)",idx.c_str());
			result=m_pMain->m_sql.query(szTmp);
		}
		else if (cparam=="addsubdevice")
		{
			std::string idx=m_pWebEm->FindValue("idx");
			std::string subidx=m_pWebEm->FindValue("subidx");
			if ((idx=="")||(subidx==""))
				goto exitjson;
			if (idx==subidx)
				goto exitjson;

			//first check if it is not already a sub device
			szQuery.clear();
			szQuery.str("");
			szQuery << "SELECT ID FROM LightSubDevices WHERE (DeviceRowID=='" << subidx << "') AND (ParentID =='" << idx << "')";
			result=m_pMain->m_sql.query(szQuery.str());
			if (result.size()==0)
			{
				root["status"]="OK";
				root["title"]="AddSubDevice";
				//no it is not, add it
				sprintf(szTmp,
					"INSERT INTO LightSubDevices (DeviceRowID, ParentID) VALUES ('%s','%s')",
					subidx.c_str(),
					idx.c_str()
					);
				result=m_pMain->m_sql.query(szTmp);
			}
		}
		else if (cparam=="addscenedevice")
		{
			std::string idx=m_pWebEm->FindValue("idx");
			std::string devidx=m_pWebEm->FindValue("devidx");
			std::string isscene=m_pWebEm->FindValue("isscene");
			int command=atoi(m_pWebEm->FindValue("command").c_str());
			
			if (
				(idx=="")||
				(devidx=="")||
				(isscene=="")
				)
				goto exitjson;
			int level=atoi(m_pWebEm->FindValue("level").c_str());
			int hue=atoi(m_pWebEm->FindValue("hue").c_str());
			//first check if this device is not the scene code!
			szQuery.clear();
			szQuery.str("");
			szQuery << "SELECT HardwareID, DeviceID, Unit, Type, SubType FROM DeviceStatus WHERE (ID==" << devidx << ")";
			result=m_pMain->m_sql.query(szQuery.str());
			if (result.size()>0)
			{
				szQuery.clear();
				szQuery.str("");
				szQuery << "SELECT HardwareID, DeviceID, Unit, Type, SubType FROM Scenes WHERE (ID==" << idx << ")";
				result2=m_pMain->m_sql.query(szQuery.str());
				if (result2.size()>0)
				{
					if (
						(result[0][0]==result2[0][0])&&
						(result[0][1]==result2[0][1])&&
						(result[0][2]==result2[0][2])&&
						(result[0][3]==result2[0][3])&&
						(result[0][4]==result2[0][4])
						)
					{
						//This is not allowed!
						goto exitjson;
					}
				}
			}
			//first check if it is not already a sub device
			szQuery.clear();
			szQuery.str("");
			szQuery << "SELECT ID FROM SceneDevices WHERE (DeviceRowID=='" << devidx << "') AND (SceneRowID =='" << idx << "')";
			result=m_pMain->m_sql.query(szQuery.str());
			if (result.size()==0)
			{
				root["status"]="OK";
				root["title"]="AddSceneDevice";
				//no it is not, add it
				if (isscene=="true")
				{
					sprintf(szTmp,
						"INSERT INTO SceneDevices (DeviceRowID, SceneRowID, Cmd, Level, Hue) VALUES ('%s','%s',%d,%d,%d)",
						devidx.c_str(),
						idx.c_str(),
						command,
						level,
						hue
						);
				}
				else
				{
					sprintf(szTmp,
						"INSERT INTO SceneDevices (DeviceRowID, SceneRowID, Level,Hue) VALUES ('%s','%s',%d,%d)",
						devidx.c_str(),
						idx.c_str(),
						level,
						hue
						);
				}
				result=m_pMain->m_sql.query(szTmp);
			}
		}
		else if (cparam=="updatescenedevice")
		{
			std::string idx=m_pWebEm->FindValue("idx");
			std::string devidx=m_pWebEm->FindValue("devidx");
			std::string isscene=m_pWebEm->FindValue("isscene");
			int command=atoi(m_pWebEm->FindValue("command").c_str());

			if (
				(idx=="")||
				(devidx=="")||
				(isscene!="true")
				)
				goto exitjson;
			int level=atoi(m_pWebEm->FindValue("level").c_str());
			int hue=atoi(m_pWebEm->FindValue("hue").c_str());
			root["status"]="OK";
			root["title"]="UpdateSceneDevice";
			szQuery.clear();
			szQuery.str("");
			szQuery << "UPDATE SceneDevices SET Cmd=" << command << ", Level=" << level << ", Hue=" << hue << " WHERE (ID == " << idx << ")";
			result=m_pMain->m_sql.query(szQuery.str());
		}
		else if (cparam=="deletescenedevice")
		{
			std::string idx=m_pWebEm->FindValue("idx");
			if (idx=="")
				goto exitjson;
			root["status"]="OK";
			root["title"]="DeleteSceneDevice";
			sprintf(szTmp,"DELETE FROM SceneDevices WHERE (ID == %s)",idx.c_str());
			result=m_pMain->m_sql.query(szTmp);
			sprintf(szTmp,"DELETE FROM CamerasActiveDevices WHERE (DevSceneType==1) AND (DevSceneRowID == %s)",idx.c_str());
			result=m_pMain->m_sql.query(szTmp);

		}
		else if (cparam=="getsubdevices")
		{
			std::string idx=m_pWebEm->FindValue("idx");
			if (idx=="")
				goto exitjson;

			root["status"]="OK";
			root["title"]="GetSubDevices";
			std::vector<std::vector<std::string> > result;
			std::stringstream szQuery;
			szQuery << "SELECT a.ID, b.Name FROM LightSubDevices a, DeviceStatus b WHERE (a.ParentID=='" << idx << "') AND (b.ID == a.DeviceRowID)";
			result=m_pMain->m_sql.query(szQuery.str());
			if (result.size()>0)
			{
				std::vector<std::vector<std::string> >::const_iterator itt;
				int ii=0;
				for (itt=result.begin(); itt!=result.end(); ++itt)
				{
					std::vector<std::string> sd=*itt;

					root["result"][ii]["ID"]=sd[0];
					root["result"][ii]["Name"]=sd[1];
					ii++;
				}
			}
		}
		else if (cparam=="gettimerlist")
		{
			root["status"]="OK";
			root["title"]="GetTimerList";
			std::vector<std::vector<std::string> > result;
			std::stringstream szQuery;
			szQuery << "SELECT t.ID, t.Active, d.[Name], t.DeviceRowID, t.Time, t.Type, t.Cmd, t.Level, t.Days FROM Timers as t, DeviceStatus as d WHERE (d.ID == t.DeviceRowID) ORDER BY d.[Name], t.Time";
			result=m_pMain->m_sql.query(szQuery.str());
			if (result.size()>0)
			{
				std::vector<std::vector<std::string> >::const_iterator itt;
				int ii=0;
				for (itt=result.begin(); itt!=result.end(); ++itt)
				{
					std::vector<std::string> sd=*itt;

					root["result"][ii]["ID"]			=sd[0];
					root["result"][ii]["Active"]		=sd[1];
					root["result"][ii]["Name"]			=sd[2];
					root["result"][ii]["DeviceRowID"]	=sd[3];
					root["result"][ii]["Time"]			=sd[4];
					root["result"][ii]["Type"]			=sd[5];
					root["result"][ii]["Cmd"]			=sd[6];
					root["result"][ii]["Level"]			=sd[7];
					root["result"][ii]["Days"]			=sd[8];
					ii++;
				}
			}
		}
		else if (cparam=="getscenedevices")
		{
			std::string idx=m_pWebEm->FindValue("idx");
			std::string isscene=m_pWebEm->FindValue("isscene");
			
			if (
				(idx=="")||
				(isscene=="")
				)
				goto exitjson;

			root["status"]="OK";
			root["title"]="GetSceneDevices";

			std::vector<std::vector<std::string> > result;
			std::stringstream szQuery;
			szQuery << "SELECT a.ID, b.Name, a.DeviceRowID, b.Type, b.SubType, b.nValue, b.sValue, a.Cmd, a.Level, b.ID, a.[Order], a.Hue FROM SceneDevices a, DeviceStatus b WHERE (a.SceneRowID=='" << idx << "') AND (b.ID == a.DeviceRowID) ORDER BY a.[Order]";
			result=m_pMain->m_sql.query(szQuery.str());
			if (result.size()>0)
			{
				std::vector<std::vector<std::string> >::const_iterator itt;
				int ii=0;
				for (itt=result.begin(); itt!=result.end(); ++itt)
				{
					std::vector<std::string> sd=*itt;

					root["result"][ii]["ID"]=sd[0];
					root["result"][ii]["Name"]=sd[1];
					root["result"][ii]["DevID"]=sd[2];
					root["result"][ii]["DevRealIdx"]=sd[9];
					root["result"][ii]["Order"]=atoi(sd[10].c_str());

					unsigned char devType=atoi(sd[3].c_str());
					unsigned char subType=atoi(sd[4].c_str());
					unsigned char nValue=(unsigned char)atoi(sd[5].c_str());
					std::string sValue=sd[6];
					int command=atoi(sd[7].c_str());
					int level=atoi(sd[8].c_str());

					std::string lstatus="";
					int llevel=0;
					bool bHaveDimmer=false;
					bool bHaveGroupCmd=false;
					int maxDimLevel=0;
					if (isscene=="true")
						GetLightStatus(devType,subType,command,sValue,lstatus,llevel,bHaveDimmer,maxDimLevel,bHaveGroupCmd);
					else
						GetLightStatus(devType,subType,nValue,sValue,lstatus,llevel,bHaveDimmer,maxDimLevel,bHaveGroupCmd);
					root["result"][ii]["IsOn"]=IsLightSwitchOn(lstatus);
					root["result"][ii]["Level"]=level;
					root["result"][ii]["Hue"]=atoi(sd[11].c_str());
					root["result"][ii]["Type"]=RFX_Type_Desc(devType,1);
					root["result"][ii]["SubType"]=RFX_Type_SubType_Desc(devType,subType);
					ii++;
				}
			}
		}
		else if (cparam=="changescenedeviceorder")
		{
			std::string idx=m_pWebEm->FindValue("idx");
			if (idx=="")
				goto exitjson;
			std::string sway=m_pWebEm->FindValue("way");
			if (sway=="")
				goto exitjson;
			bool bGoUp=(sway=="0");

			std::string aScene,aOrder,oID,oOrder;

			//Get actual device order
			std::vector<std::vector<std::string> > result;
			std::stringstream szQuery;
			szQuery << "SELECT SceneRowID, [Order] FROM SceneDevices WHERE (ID=='" << idx << "')";
			result=m_pMain->m_sql.query(szQuery.str());
			if (result.size()<1)
				goto exitjson;
			aScene=result[0][0];
			aOrder=result[0][1];

			szQuery.clear();
			szQuery.str("");

			if (!bGoUp)
			{
				//Get next device order
				szQuery << "SELECT ID, [Order] FROM SceneDevices WHERE (SceneRowID=='" << aScene << "' AND [Order]>'" << aOrder << "') ORDER BY [Order] ASC";
				result=m_pMain->m_sql.query(szQuery.str());
				if (result.size()<1)
					goto exitjson;
				oID=result[0][0];
				oOrder=result[0][1];
			}
			else
			{
				//Get previous device order
				szQuery << "SELECT ID, [Order] FROM SceneDevices WHERE (SceneRowID=='" << aScene << "' AND [Order]<'" << aOrder << "') ORDER BY [Order] DESC";
				result=m_pMain->m_sql.query(szQuery.str());
				if (result.size()<1)
					goto exitjson;
				oID=result[0][0];
				oOrder=result[0][1];
			}
			//Swap them
			root["status"]="OK";
			root["title"]="ChangeSceneDeviceOrder";

			szQuery.clear();
			szQuery.str("");
			szQuery << "UPDATE SceneDevices SET [Order] = '" << oOrder << "' WHERE (ID='" << idx << "')";
			result=m_pMain->m_sql.query(szQuery.str());
			szQuery.clear();
			szQuery.str("");
			szQuery << "UPDATE SceneDevices SET [Order] = '" << aOrder << "' WHERE (ID='" << oID << "')";
			result=m_pMain->m_sql.query(szQuery.str());
		}
		else if (cparam=="deleteallscenedevices")
		{
			std::string idx=m_pWebEm->FindValue("idx");
			if (idx=="")
				goto exitjson;
			root["status"]="OK";
			root["title"]="DeleteAllSceneDevices";
			sprintf(szTmp,"DELETE FROM SceneDevices WHERE (SceneRowID == %s)",idx.c_str());
			result=m_pMain->m_sql.query(szTmp);
		}
		else if (cparam=="getlightswitches")
		{
			root["status"]="OK";
			root["title"]="GetLightSwitches";
			std::vector<std::vector<std::string> > result;
			std::stringstream szQuery;
			szQuery << "SELECT ID, Name, Type, SubType, Used, SwitchType FROM DeviceStatus ORDER BY Name";
			result=m_pMain->m_sql.query(szQuery.str());
			if (result.size()>0)
			{
				std::vector<std::vector<std::string> >::const_iterator itt;
				int ii=0;
				for (itt=result.begin(); itt!=result.end(); ++itt)
				{
					std::vector<std::string> sd=*itt;

					std::string ID=sd[0];
					std::string Name=sd[1];
					int Type=atoi(sd[2].c_str());
					int SubType=atoi(sd[3].c_str());
					int used=atoi(sd[4].c_str());
					_eSwitchType switchtype=(_eSwitchType)atoi(sd[5].c_str());
					bool bdoAdd;
					switch (Type)
					{
					case pTypeLighting1:
					case pTypeLighting2:
					case pTypeLighting3:
					case pTypeLighting4:
					case pTypeLighting5:
					case pTypeLighting6:
					case pTypeLimitlessLights:
					case pTypeSecurity1:
					case pTypeBlinds:
					case pTypeChime:
						bdoAdd=true;
						if (!used)
						{
							bdoAdd=false;
							bool bIsSubDevice=false;
							std::vector<std::vector<std::string> > resultSD;
							std::stringstream szQuerySD;

							szQuerySD.clear();
							szQuerySD.str("");
							szQuerySD << "SELECT ID FROM LightSubDevices WHERE (DeviceRowID=='" << sd[0] << "')";
							resultSD=m_pMain->m_sql.query(szQuerySD.str());
							if (resultSD.size()>0)
								bdoAdd=true;
						}
						if (bdoAdd)
						{
							root["result"][ii]["idx"]=ID;
							root["result"][ii]["Name"]=Name;
							root["result"][ii]["Type"]=Hardware_Type_Desc(Type);
							root["result"][ii]["SubType"]=RFX_Type_SubType_Desc(Type,SubType);
							bool bIsDimmer=(switchtype==STYPE_Dimmer);
							root["result"][ii]["IsDimmer"]=bIsDimmer;
							ii++;
						}
						break;
					}
				}
			}
		}
		else if (cparam=="getlightswitchesscenes")
		{
			root["status"]="OK";
			root["title"]="GetLightSwitchesScenes";
			std::vector<std::vector<std::string> > result;
			std::stringstream szQuery;
			int ii=0;

			//First List/Switch Devices
			szQuery << "SELECT ID, Name, Type, Used FROM DeviceStatus ORDER BY Name";
			result=m_pMain->m_sql.query(szQuery.str());
			if (result.size()>0)
			{
				std::vector<std::vector<std::string> >::const_iterator itt;
				for (itt=result.begin(); itt!=result.end(); ++itt)
				{
					std::vector<std::string> sd=*itt;

					std::string ID=sd[0];
					std::string Name=sd[1];
					int Type=atoi(sd[2].c_str());
					int used=atoi(sd[3].c_str());
					if (used)
					{
						switch (Type)
						{
						case pTypeLighting1:
						case pTypeLighting2:
						case pTypeLighting3:
						case pTypeLighting4:
						case pTypeLighting5:
						case pTypeLighting6:
						case pTypeLimitlessLights:
						case pTypeSecurity1:
						case pTypeBlinds:
						case pTypeChime:
							{
								root["result"][ii]["type"]=0;
								root["result"][ii]["idx"]=ID;
								root["result"][ii]["Name"]="[Light/Switch] " + Name;
								ii++;
							}
							break;
						}
					}
				}
			}//end light/switches
			
			//Add Scenes
			szQuery.clear();
			szQuery.str("");
			szQuery << "SELECT ID, Name FROM Scenes ORDER BY Name";
			result=m_pMain->m_sql.query(szQuery.str());
			if (result.size()>0)
			{
				std::vector<std::vector<std::string> >::const_iterator itt;
				for (itt=result.begin(); itt!=result.end(); ++itt)
				{
					std::vector<std::string> sd=*itt;

					std::string ID=sd[0];
					std::string Name=sd[1];

					root["result"][ii]["type"]=1;
					root["result"][ii]["idx"]=ID;
					root["result"][ii]["Name"]="[Scene] " + Name;
					ii++;
				}
			}//end light/switches
		}
		else if (cparam=="getcamactivedevices")
		{
			std::string idx=m_pWebEm->FindValue("idx");
			if (idx=="")
				goto exitjson;
			root["status"]="OK";
			root["title"]="GetCameraActiveDevices";
			std::vector<std::vector<std::string> > result;
			std::stringstream szQuery;
			//First List/Switch Devices
			szQuery << "SELECT ID, DevSceneType, DevSceneRowID, DevSceneWhen, DevSceneDelay FROM CamerasActiveDevices WHERE (CameraRowID=='" << idx << "') ORDER BY ID";
			result=m_pMain->m_sql.query(szQuery.str());
			if (result.size()>0)
			{
				std::vector<std::vector<std::string> >::const_iterator itt;
				int ii=0;
				for (itt=result.begin(); itt!=result.end(); ++itt)
				{
					std::vector<std::string> sd=*itt;

					std::string ID=sd[0];
					int DevSceneType=atoi(sd[1].c_str());
					std::string DevSceneRowID=sd[2];
					int DevSceneWhen=atoi(sd[3].c_str());
					int DevSceneDelay=atoi(sd[4].c_str());

					std::string Name="";
					if (DevSceneType==0)
					{
						std::vector<std::vector<std::string> > result2;
						std::stringstream szQuery2;
						szQuery2 << "SELECT Name FROM DeviceStatus WHERE (ID=='" << DevSceneRowID << "')";
						result2=m_pMain->m_sql.query(szQuery2.str());
						if (result2.size()>0)
						{
							Name="[Light/Switches] " + result2[0][0];
						}
					}
					else
					{
						std::vector<std::vector<std::string> > result2;
						std::stringstream szQuery2;
						szQuery2 << "SELECT Name FROM Scenes WHERE (ID=='" << DevSceneRowID << "')";
						result2=m_pMain->m_sql.query(szQuery2.str());
						if (result2.size()>0)
						{
							Name="[Scene] " + result2[0][0];
						}
					}
					if (Name!="")
					{
						root["result"][ii]["idx"]=ID;
						root["result"][ii]["type"]=DevSceneType;
						root["result"][ii]["DevSceneRowID"]=DevSceneRowID;
						root["result"][ii]["when"]=DevSceneWhen;
						root["result"][ii]["delay"]=DevSceneDelay;
						root["result"][ii]["Name"]=Name;
						ii++;
					}
				}
			}
		}
		else if (cparam=="addcamactivedevice")
		{
			std::string idx=m_pWebEm->FindValue("idx");
			std::string activeidx=m_pWebEm->FindValue("activeidx");
			std::string sactivetype=m_pWebEm->FindValue("activetype");
			std::string sactivewhen=m_pWebEm->FindValue("activewhen");
			std::string sactivedelay=m_pWebEm->FindValue("activedelay");

			if (
				(idx=="")||
				(activeidx=="")||
				(sactivetype=="")||
				(sactivewhen=="")||
				(sactivedelay=="")
				)
			{
				goto exitjson;
			}

			int activetype=atoi(sactivetype.c_str());
			int activewhen=atoi(sactivewhen.c_str());
			int activedelay=atoi(sactivedelay.c_str());

			//first check if it is not already a Active Device
			szQuery.clear();
			szQuery.str("");
			szQuery << "SELECT ID FROM CamerasActiveDevices WHERE (CameraRowID=='" 
					<< idx << "') AND (DevSceneType==" 
					<< activetype << ") AND (DevSceneRowID=='" << activeidx << "')  AND (DevSceneWhen==" << sactivewhen << ")";
			result=m_pMain->m_sql.query(szQuery.str());
			if (result.size()==0)
			{
				root["status"]="OK";
				root["title"]="AddCameraActiveDevice";
				//no it is not, add it
				sprintf(szTmp,
					"INSERT INTO CamerasActiveDevices (CameraRowID, DevSceneType, DevSceneRowID, DevSceneWhen, DevSceneDelay) VALUES ('%s',%d,'%s',%d,%d)",
					idx.c_str(),
					activetype,
					activeidx.c_str(),
					activewhen,
					activedelay
					);
				result=m_pMain->m_sql.query(szTmp);
				m_pMain->m_cameras.ReloadCameraActiveDevices(idx);
			}
		}
		else if (cparam=="deleteamactivedevice")
		{
			std::string idx=m_pWebEm->FindValue("idx");
			if (idx=="")
				goto exitjson;
			root["status"]="OK";
			root["title"]="DeleteCameraActiveDevice";
			sprintf(szTmp,"DELETE FROM CamerasActiveDevices WHERE (ID == '%s')",idx.c_str());
			result=m_pMain->m_sql.query(szTmp);
		}
		else if (cparam=="deleteallactivecamdevices")
		{
			std::string idx=m_pWebEm->FindValue("idx");
			if (idx=="")
				goto exitjson;
			root["status"]="OK";
			root["title"]="DeleteAllCameraActiveDevices";
			sprintf(szTmp,"DELETE FROM CamerasActiveDevices WHERE (CameraRowID == '%s')",idx.c_str());
			result=m_pMain->m_sql.query(szTmp);
		}
		else if (cparam=="testemail")
		{
			std::string EmailFrom=m_pWebEm->FindValue("EmailFrom");
			std::string EmailTo=m_pWebEm->FindValue("EmailTo");
			std::string EmailServer=m_pWebEm->FindValue("EmailServer");
			std::string EmailUsername=m_pWebEm->FindValue("EmailUsername");
			std::string EmailPassword=m_pWebEm->FindValue("EmailPassword");
			std::string sEmailPort=m_pWebEm->FindValue("EmailPort");

			if (
				(EmailFrom=="")||
				(EmailTo=="")||
				(EmailServer=="")||
				(sEmailPort=="")
				)
				goto exitjson;
			int EmailPort=atoi(sEmailPort.c_str());
			std::string szBody;
			szBody=
				"<html>\n"
				"<body>\n"
				"<b>If you received this, then your email settings worked!</b>\n"
				"</body>\n"
				"</html>\n";

			SMTPClient sclient;
			sclient.SetFrom(CURLEncode::URLDecode(EmailFrom.c_str()));
			sclient.SetTo(CURLEncode::URLDecode(EmailTo.c_str()));
			sclient.SetCredentials(CURLEncode::URLDecode(EmailUsername),CURLEncode::URLDecode(EmailPassword));
			sclient.SetServer(CURLEncode::URLDecode(EmailServer.c_str()),EmailPort);
			sclient.SetSubject(CURLEncode::URLDecode("Test email message from Domoticz!"));
			sclient.SetHTMLBody(szBody);
			bool bRet=sclient.SendEmail();
			if (bRet==true) {
				root["status"]="OK";
				root["title"]="TestEmail";
			}
		}
		else if (cparam=="testswitch")
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

			if (lighttype==6)
			{
				//Blyss
				dtype=pTypeLighting6;
				subtype=sTypeBlyss;
				std::string sgroupcode=m_pWebEm->FindValue("groupcode");
				sunitcode=m_pWebEm->FindValue("unitcode");
				std::string id=m_pWebEm->FindValue("id");
				if (
					(sgroupcode=="")||
					(sunitcode=="")||
					(id=="")
					)
					goto exitjson;
				devid=id+sgroupcode;
			}
			else if (lighttype<10)
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
			else if (lighttype<13)
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
			else if (lighttype<100)
			{
				dtype=pTypeLighting5;
				subtype=lighttype-13;
				std::string id=m_pWebEm->FindValue("id");
				sunitcode=m_pWebEm->FindValue("unitcode");
				if (
					(id=="")||
					(sunitcode=="")
					)
					goto exitjson;
				devid=id;
			}
			else
			{
				if (lighttype==100)
				{
					//Chime/ByronSX
					dtype=pTypeChime;
					subtype=sTypeByronSX;
					std::string id=m_pWebEm->FindValue("id");
					sunitcode=m_pWebEm->FindValue("unitcode");
					if (
						(id=="")||
						(sunitcode=="")
						)
						goto exitjson;
					int iUnitCode=atoi(sunitcode.c_str())-1;
					switch (iUnitCode)
					{
					case 0:
						iUnitCode=chime_sound0;
						break;
					case 1:
						iUnitCode=chime_sound1;
						break;
					case 2:
						iUnitCode=chime_sound2;
						break;
					case 3:
						iUnitCode=chime_sound3;
						break;
					case 4:
						iUnitCode=chime_sound4;
						break;
					case 5:
						iUnitCode=chime_sound5;
						break;
					case 6:
						iUnitCode=chime_sound6;
						break;
					case 7:
						iUnitCode=chime_sound7;
						break;
					}
					sprintf(szTmp,"%d",iUnitCode);
					sunitcode=szTmp;
					devid=id;
				}
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
			m_pMain->SwitchLightInt(sd,switchcmd,0,-1,true);
		}
		else if (cparam=="addswitch")
		{
			std::string hwdid=m_pWebEm->FindValue("hwdid");
			std::string name=m_pWebEm->FindValue("name");
			std::string sswitchtype=m_pWebEm->FindValue("switchtype");
			std::string slighttype=m_pWebEm->FindValue("lighttype");
			std::string maindeviceidx=m_pWebEm->FindValue("maindeviceidx");

			if (
				(hwdid=="")||
				(sswitchtype=="")||
				(slighttype=="")||
				(name=="")
				)
				goto exitjson;
			_eSwitchType switchtype=(_eSwitchType)atoi(sswitchtype.c_str());
			int lighttype=atoi(slighttype.c_str());
			int dtype=0;
			int subtype=0;
			std::string sunitcode;
			std::string devid;

			if (lighttype==6)
			{
				//Blyss
				dtype=pTypeLighting6;
				subtype=sTypeBlyss;
				std::string sgroupcode=m_pWebEm->FindValue("groupcode");
				sunitcode=m_pWebEm->FindValue("unitcode");
				std::string id=m_pWebEm->FindValue("id");
				if (
					(sgroupcode=="")||
					(sunitcode=="")||
					(id=="")
					)
					goto exitjson;
				devid=id+sgroupcode;
			}
			else if (lighttype<10)
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
			else if (lighttype<13)
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
			else if (lighttype<100)
			{
				dtype=pTypeLighting5;
				subtype=lighttype-13;
				std::string id=m_pWebEm->FindValue("id");
				sunitcode=m_pWebEm->FindValue("unitcode");
				if (
					(id=="")||
					(sunitcode=="")
					)
					goto exitjson;
				devid=id;
			}
			else
			{
				if (lighttype==100)
				{
					//Chime/ByronSX
					dtype=pTypeChime;
					subtype=sTypeByronSX;
					std::string id=m_pWebEm->FindValue("id");
					sunitcode=m_pWebEm->FindValue("unitcode");
					if (
						(id=="")||
						(sunitcode=="")
						)
						goto exitjson;
					int iUnitCode=atoi(sunitcode.c_str())-1;
					switch (iUnitCode)
					{
					case 0:
						iUnitCode=chime_sound0;
						break;
					case 1:
						iUnitCode=chime_sound1;
						break;
					case 2:
						iUnitCode=chime_sound2;
						break;
					case 3:
						iUnitCode=chime_sound3;
						break;
					case 4:
						iUnitCode=chime_sound4;
						break;
					case 5:
						iUnitCode=chime_sound5;
						break;
					case 6:
						iUnitCode=chime_sound6;
						break;
					case 7:
						iUnitCode=chime_sound7;
						break;
					}
					sprintf(szTmp,"%d",iUnitCode);
					sunitcode=szTmp;
					devid=id;
				}
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
			std::string devname;
			m_pMain->m_sql.UpdateValue(atoi(hwdid.c_str()), devid.c_str(),atoi(sunitcode.c_str()),dtype,subtype,0,-1,0,devname);
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

			if (maindeviceidx!="")
			{
				if (maindeviceidx!=ID)
				{
					//this is a sub device for another light/switch
					//first check if it is not already a sub device
					szQuery.clear();
					szQuery.str("");
					szQuery << "SELECT ID FROM LightSubDevices WHERE (DeviceRowID=='" << ID << "') AND (ParentID =='" << maindeviceidx << "')";
					result=m_pMain->m_sql.query(szQuery.str());
					if (result.size()==0)
					{
						//no it is not, add it
						sprintf(szTmp,
							"INSERT INTO LightSubDevices (DeviceRowID, ParentID) VALUES ('%s','%s')",
							ID.c_str(),
							maindeviceidx.c_str()
							);
						result=m_pMain->m_sql.query(szTmp);
					}
				}
			}

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
			szQuery << "SELECT Type, SubType, SwitchType FROM DeviceStatus WHERE (ID == " << idx << ")";
			result=m_pMain->m_sql.query(szQuery.str());
			if (result.size()<1)
				goto exitjson;

			root["status"]="OK";
			root["title"]="GetNotificationTypes";
			unsigned char dType=atoi(result[0][0].c_str());
			unsigned char dSubType=atoi(result[0][1].c_str());
			unsigned char switchtype=atoi(result[0][2].c_str());

			int ii=0;
			if (
				(dType==pTypeLighting1)||
				(dType==pTypeLighting2)||
				(dType==pTypeLighting3)||
				(dType==pTypeLighting4)||
				(dType==pTypeLighting5)||
				(dType==pTypeLighting6)||
				(dType==pTypeLimitlessLights)||
				(dType==pTypeSecurity1)||
				(dType==pTypeBlinds)||
				(dType==pTypeChime)
				)
			{
				if (switchtype!=STYPE_PushOff)
				{
					root["result"][ii]["val"]=NTYPE_SWITCH_ON;
					root["result"][ii]["text"]=Notification_Type_Desc(NTYPE_SWITCH_ON,0);
					root["result"][ii]["ptag"]=Notification_Type_Desc(NTYPE_SWITCH_ON,1);
					ii++;
				}
				if (switchtype!=STYPE_PushOn)
				{
					root["result"][ii]["val"]=NTYPE_SWITCH_OFF;
					root["result"][ii]["text"]=Notification_Type_Desc(NTYPE_SWITCH_OFF,0);
					root["result"][ii]["ptag"]=Notification_Type_Desc(NTYPE_SWITCH_OFF,1);
					ii++;
				}
			}
			if (
				(
				(dType==pTypeTEMP)||
				(dType==pTypeTEMP_HUM)||
				(dType==pTypeTEMP_HUM_BARO)||
				(dType==pTypeTEMP_BARO)||
				(dType==pTypeThermostat1)||
				(dType==pTypeRego6XXTemp)||
				((dType==pTypeRFXSensor)&&(dSubType==sTypeRFXSensorTemp))
				)||
				((dType==pTypeUV)&&(dSubType==sTypeUV3))||
				((dType==pTypeWIND)&&(dSubType==sTypeWIND4))||
				((dType==pTypeWIND)&&(dSubType==sTypeWINDNoTemp))
				)
			{
				root["result"][ii]["val"]=NTYPE_TEMPERATURE;
				root["result"][ii]["text"]=Notification_Type_Desc(NTYPE_TEMPERATURE,0);
				root["result"][ii]["ptag"]=Notification_Type_Desc(NTYPE_TEMPERATURE,1);
				ii++;
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
			if (
				(dType==pTypeTEMP_HUM)||
				(dType==pTypeTEMP_HUM_BARO)
				)
			{
				root["result"][ii]["val"]=NTYPE_DEWPOINT;
				root["result"][ii]["text"]=Notification_Type_Desc(NTYPE_DEWPOINT,0);
				root["result"][ii]["ptag"]=Notification_Type_Desc(NTYPE_DEWPOINT,1);
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
				(dType==pTypeBARO)||
				(dType==pTypeTEMP_BARO)
				)
			{
				root["result"][ii]["val"]=NTYPE_BARO;
				root["result"][ii]["text"]=Notification_Type_Desc(NTYPE_BARO,0);
				root["result"][ii]["ptag"]=Notification_Type_Desc(NTYPE_BARO,1);
				ii++;
			}
			if (
				((dType==pTypeRFXMeter)&&(dSubType==sTypeRFXMeterCount))||
				(dType==pTypeYouLess)||
                ((dType==pTypeRego6XXValue)&&(dSubType==sTypeRego6XXCounter))
				)
			{
				if (switchtype==MTYPE_ENERGY)
				{
					root["result"][ii]["val"]=NTYPE_TODAYENERGY;
					root["result"][ii]["text"]=Notification_Type_Desc(NTYPE_TODAYENERGY,0);
					root["result"][ii]["ptag"]=Notification_Type_Desc(NTYPE_TODAYENERGY,1);
				}
				else if (switchtype==MTYPE_GAS)
				{
					root["result"][ii]["val"]=NTYPE_TODAYGAS;
					root["result"][ii]["text"]=Notification_Type_Desc(NTYPE_TODAYGAS,0);
					root["result"][ii]["ptag"]=Notification_Type_Desc(NTYPE_TODAYGAS,1);
				}
				else if (switchtype==MTYPE_COUNTER)
				{
					root["result"][ii]["val"]=NTYPE_TODAYCOUNTER;
					root["result"][ii]["text"]=Notification_Type_Desc(NTYPE_TODAYCOUNTER,0);
					root["result"][ii]["ptag"]=Notification_Type_Desc(NTYPE_TODAYCOUNTER,1);
				}
				else
				{
					//water (same as gas)
					root["result"][ii]["val"]=NTYPE_TODAYGAS;
					root["result"][ii]["text"]=Notification_Type_Desc(NTYPE_TODAYGAS,0);
					root["result"][ii]["ptag"]=Notification_Type_Desc(NTYPE_TODAYGAS,1);
				}
				ii++;
			}
			if (dType==pTypeYouLess)
			{
				root["result"][ii]["val"]=NTYPE_USAGE;
				root["result"][ii]["text"]=Notification_Type_Desc(NTYPE_USAGE,0);
				root["result"][ii]["ptag"]=Notification_Type_Desc(NTYPE_USAGE,1);
				ii++;
			}
			if (dType==pTypeAirQuality)
			{
				root["result"][ii]["val"]=NTYPE_USAGE;
				root["result"][ii]["text"]=Notification_Type_Desc(NTYPE_USAGE,0);
				root["result"][ii]["ptag"]=Notification_Type_Desc(NTYPE_USAGE,1);
				ii++;
			}
			else if ((dType==pTypeGeneral)&&((dSubType==sTypeSoilMoisture)||(dSubType==sTypeLeafWetness)))
			{
				root["result"][ii]["val"]=NTYPE_USAGE;
				root["result"][ii]["text"]=Notification_Type_Desc(NTYPE_USAGE,0);
				root["result"][ii]["ptag"]=Notification_Type_Desc(NTYPE_USAGE,1);
				ii++;
			}
			if ((dType==pTypeGeneral)&&(dSubType==sTypeVisibility))
			{
				root["result"][ii]["val"]=NTYPE_USAGE;
				root["result"][ii]["text"]=Notification_Type_Desc(NTYPE_USAGE,0);
				root["result"][ii]["ptag"]=Notification_Type_Desc(NTYPE_USAGE,1);
				ii++;
			}
			if ((dType==pTypeGeneral)&&(dSubType==sTypeSolarRadiation))
			{
				root["result"][ii]["val"]=NTYPE_USAGE;
				root["result"][ii]["text"]=Notification_Type_Desc(NTYPE_USAGE,0);
				root["result"][ii]["ptag"]=Notification_Type_Desc(NTYPE_USAGE,1);
				ii++;
			}
			if (dType==pTypeLux)
			{
				root["result"][ii]["val"]=NTYPE_USAGE;
				root["result"][ii]["text"]=Notification_Type_Desc(NTYPE_USAGE,0);
				root["result"][ii]["ptag"]=Notification_Type_Desc(NTYPE_USAGE,1);
				ii++;
			}
			if (dType==pTypeWEIGHT)
			{
				root["result"][ii]["val"]=NTYPE_USAGE;
				root["result"][ii]["text"]=Notification_Type_Desc(NTYPE_USAGE,0);
				root["result"][ii]["ptag"]=Notification_Type_Desc(NTYPE_USAGE,1);
				ii++;
			}
			if (dType==pTypeUsage)
			{
				root["result"][ii]["val"]=NTYPE_USAGE;
				root["result"][ii]["text"]=Notification_Type_Desc(NTYPE_USAGE,0);
				root["result"][ii]["ptag"]=Notification_Type_Desc(NTYPE_USAGE,1);
				ii++;
			}
			if (dType==pTypeENERGY)
			{
				root["result"][ii]["val"]=NTYPE_USAGE;
				root["result"][ii]["text"]=Notification_Type_Desc(NTYPE_USAGE,0);
				root["result"][ii]["ptag"]=Notification_Type_Desc(NTYPE_USAGE,1);
				ii++;
			}
			if (dType==pTypePOWER)
			{
				root["result"][ii]["val"]=NTYPE_USAGE;
				root["result"][ii]["text"]=Notification_Type_Desc(NTYPE_USAGE,0);
				root["result"][ii]["ptag"]=Notification_Type_Desc(NTYPE_USAGE,1);
				ii++;
			}
			if ((dType==pTypeCURRENT)&&(dSubType==sTypeELEC1))
			{
				root["result"][ii]["val"]=NTYPE_AMPERE1;
				root["result"][ii]["text"]=Notification_Type_Desc(NTYPE_AMPERE1,0);
				root["result"][ii]["ptag"]=Notification_Type_Desc(NTYPE_AMPERE1,1);
				ii++;
				root["result"][ii]["val"]=NTYPE_AMPERE2;
				root["result"][ii]["text"]=Notification_Type_Desc(NTYPE_AMPERE2,0);
				root["result"][ii]["ptag"]=Notification_Type_Desc(NTYPE_AMPERE2,1);
				ii++;
				root["result"][ii]["val"]=NTYPE_AMPERE3;
				root["result"][ii]["text"]=Notification_Type_Desc(NTYPE_AMPERE3,0);
				root["result"][ii]["ptag"]=Notification_Type_Desc(NTYPE_AMPERE3,1);
				ii++;
			}
			if ((dType==pTypeCURRENTENERGY)&&(dSubType==sTypeELEC4))
			{
				root["result"][ii]["val"]=NTYPE_AMPERE1;
				root["result"][ii]["text"]=Notification_Type_Desc(NTYPE_AMPERE1,0);
				root["result"][ii]["ptag"]=Notification_Type_Desc(NTYPE_AMPERE1,1);
				ii++;
				root["result"][ii]["val"]=NTYPE_AMPERE2;
				root["result"][ii]["text"]=Notification_Type_Desc(NTYPE_AMPERE2,0);
				root["result"][ii]["ptag"]=Notification_Type_Desc(NTYPE_AMPERE2,1);
				ii++;
				root["result"][ii]["val"]=NTYPE_AMPERE3;
				root["result"][ii]["text"]=Notification_Type_Desc(NTYPE_AMPERE3,0);
				root["result"][ii]["ptag"]=Notification_Type_Desc(NTYPE_AMPERE3,1);
				ii++;
			}
			if (dType==pTypeP1Power)
			{
				root["result"][ii]["val"]=NTYPE_USAGE;
				root["result"][ii]["text"]=Notification_Type_Desc(NTYPE_USAGE,0);
				root["result"][ii]["ptag"]=Notification_Type_Desc(NTYPE_USAGE,1);
				ii++;
				root["result"][ii]["val"]=NTYPE_TODAYENERGY;
				root["result"][ii]["text"]=Notification_Type_Desc(NTYPE_TODAYENERGY,0);
				root["result"][ii]["ptag"]=Notification_Type_Desc(NTYPE_TODAYENERGY,1);
				ii++;
			}
			if (dType==pTypeP1Gas)
			{
				root["result"][ii]["val"]=NTYPE_TODAYGAS;
				root["result"][ii]["text"]=Notification_Type_Desc(NTYPE_TODAYGAS,0);
				root["result"][ii]["ptag"]=Notification_Type_Desc(NTYPE_TODAYGAS,1);
				ii++;
			}
			if ((dType==pTypeThermostat)&&(dSubType==sTypeThermSetpoint))
			{
				root["result"][ii]["val"]=NTYPE_TEMPERATURE;
				root["result"][ii]["text"]=Notification_Type_Desc(NTYPE_TEMPERATURE,0);
				root["result"][ii]["ptag"]=Notification_Type_Desc(NTYPE_TEMPERATURE,1);
				ii++;
			}
			if ((dType==pTypeRFXSensor)&&((dSubType==sTypeRFXSensorAD)||(dSubType==sTypeRFXSensorVolt)))
			{
				root["result"][ii]["val"]=NTYPE_USAGE;
				root["result"][ii]["text"]=Notification_Type_Desc(NTYPE_USAGE,0);
				root["result"][ii]["ptag"]=Notification_Type_Desc(NTYPE_USAGE,1);
				ii++;
			}
		}
		else if (cparam=="addnotification")
		{
			std::string idx=m_pWebEm->FindValue("idx");
			if (idx=="")
				goto exitjson;
		
			std::string stype=m_pWebEm->FindValue("ttype");
			std::string swhen=m_pWebEm->FindValue("twhen");
			std::string svalue=m_pWebEm->FindValue("tvalue");
			if ((stype=="")||(swhen=="")||(svalue==""))
				goto exitjson;

			_eNotificationTypes ntype=(_eNotificationTypes)atoi(stype.c_str());
			std::string ttype=Notification_Type_Desc(ntype,1);
			if (
				(ntype==NTYPE_SWITCH_ON)||
				(ntype==NTYPE_SWITCH_OFF)||
				(ntype==NTYPE_DEWPOINT)
				)
			{
				strcpy(szTmp,ttype.c_str());
			}
			else
			{
				unsigned char twhen=(swhen=="0")?'>':'<';
				sprintf(szTmp,"%s;%c;%s",ttype.c_str(),twhen,svalue.c_str());
			}
			bool bOK=m_pMain->m_sql.AddNotification(idx,szTmp);
			if (bOK) {
				root["status"]="OK";
				root["title"]="AddNotification";
			}
		}
		else if (cparam=="updatenotification")
		{
			std::string idx=m_pWebEm->FindValue("idx");
			std::string devidx=m_pWebEm->FindValue("devidx");
			if ((idx=="")||(devidx==""))
				goto exitjson;

			std::string stype=m_pWebEm->FindValue("ttype");
			std::string swhen=m_pWebEm->FindValue("twhen");
			std::string svalue=m_pWebEm->FindValue("tvalue");
			if ((stype=="")||(swhen=="")||(svalue==""))
				goto exitjson;
			root["status"]="OK";
			root["title"]="UpdateNotification";

			//delete old record
			m_pMain->m_sql.RemoveNotification(idx);

			_eNotificationTypes ntype=(_eNotificationTypes)atoi(stype.c_str());
			std::string ttype=Notification_Type_Desc(ntype,1);
			if (
				(ntype==NTYPE_SWITCH_ON)||
				(ntype==NTYPE_SWITCH_OFF)||
				(ntype==NTYPE_DEWPOINT)
				)
			{
				strcpy(szTmp,ttype.c_str());
			}
			else
			{
				unsigned char twhen=(swhen=="0")?'>':'<';
				sprintf(szTmp,"%s;%c;%s",ttype.c_str(),twhen,svalue.c_str());
			}
			m_pMain->m_sql.AddNotification(devidx,szTmp);
		}
		else if (cparam=="deletenotification")
		{
			std::string idx=m_pWebEm->FindValue("idx");
			if (idx=="")
				goto exitjson;

			root["status"]="OK";
			root["title"]="DeleteNotification";

			m_pMain->m_sql.RemoveNotification(idx);
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
			if(atoi(Order1.c_str()) < atoi(Order2.c_str()))
			{
				szQuery << "UPDATE DeviceStatus SET [Order] = [Order]+1 WHERE ([Order] >= " << Order1 << " AND [Order] < " << Order2 << ")";
			}
			else
			{
				szQuery << "UPDATE DeviceStatus SET [Order] = [Order]-1 WHERE ([Order] > " << Order2 << " AND [Order] <= " << Order1 << ")";
			}
			m_pMain->m_sql.query(szQuery.str());

			szQuery.clear();
			szQuery.str("");
			szQuery << "UPDATE DeviceStatus SET [Order] = " << Order1 << " WHERE (ID == " << idx2 << ")";
			m_pMain->m_sql.query(szQuery.str());
		}
		else if (cparam=="switchsceneorder")
		{
			std::string idx1=m_pWebEm->FindValue("idx1");
			std::string idx2=m_pWebEm->FindValue("idx2");
			if ((idx1=="")||(idx2==""))
				goto exitjson;

			std::string Order1,Order2;
			//get device order 1
			szQuery.clear();
			szQuery.str("");
			szQuery << "SELECT [Order] FROM Scenes WHERE (ID == " << idx1 << ")";
			result=m_pMain->m_sql.query(szQuery.str());
			if (result.size()<1)
				goto exitjson;
			Order1=result[0][0];

			//get device order 2
			szQuery.clear();
			szQuery.str("");
			szQuery << "SELECT [Order] FROM Scenes WHERE (ID == " << idx2 << ")";
			result=m_pMain->m_sql.query(szQuery.str());
			if (result.size()<1)
				goto exitjson;
			Order2=result[0][0];

			root["status"]="OK";
			root["title"]="SwitchSceneOrder";

			szQuery.clear();
			szQuery.str("");
			if(atoi(Order1.c_str()) < atoi(Order2.c_str()))
			{
				szQuery << "UPDATE Scenes SET [Order] = [Order]+1 WHERE ([Order] >= " << Order1 << " AND [Order] < " << Order2 << ")";
			}
			else
			{
				szQuery << "UPDATE Scenes SET [Order] = [Order]-1 WHERE ([Order] > " << Order2 << " AND [Order] <= " << Order1 << ")";
			}
			m_pMain->m_sql.query(szQuery.str());

			szQuery.clear();
			szQuery.str("");
			szQuery << "UPDATE Scenes SET [Order] = " << Order1 << " WHERE (ID == " << idx2 << ")";
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
			std::string senabled=m_pWebEm->FindValue("enabled");
			std::string shtype=m_pWebEm->FindValue("htype");
			std::string address=m_pWebEm->FindValue("address");
			std::string sport=m_pWebEm->FindValue("port");
			std::string username=m_pWebEm->FindValue("username");
			std::string password=m_pWebEm->FindValue("password");
			if (
				(name=="")||
				(senabled=="")||
				(shtype=="")||
				(sport=="")
				)
				goto exitjson;
			_eHardwareTypes htype=(_eHardwareTypes)atoi(shtype.c_str());
			int mode1=0;
			int mode2=0;
			int mode3=0;
			int mode4=0;
			int mode5=0;
			int port=atoi(sport.c_str());
			if ((htype==HTYPE_RFXtrx315)||(htype==HTYPE_RFXtrx433)||(htype==HTYPE_P1SmartMeter)||(htype==HTYPE_Rego6XX)||(htype==HTYPE_DavisVantage)||(htype==HTYPE_S0SmartMeter)||(htype==HTYPE_OpenThermGateway)||(htype==HTYPE_TeleinfoMeter)||(htype==HTYPE_OpenZWave))
			{
				//USB
				if ((htype==HTYPE_RFXtrx315)||(htype==HTYPE_RFXtrx433))
				{
				}
			}
			else if ((htype == HTYPE_RFXLAN)||(htype == HTYPE_P1SmartMeterLAN)||(htype == HTYPE_YouLess)||(htype == HTYPE_RazberryZWave)||(htype == HTYPE_OpenThermGatewayTCP)||(htype == HTYPE_LimitlessLights)) {
				//Lan
				if (address=="")
					goto exitjson;
			}
			else if (htype == HTYPE_Domoticz) {
				//Remote Domoticz
				if (address=="")
					goto exitjson;
			}
			else if (htype == HTYPE_TE923) {
				//all fine here!
			}
			else if (htype == HTYPE_VOLCRAFTCO20) {
				//all fine here!
			}
			else if (htype == HTYPE_1WIRE) {
				//all fine here!
			}
			else if (htype == HTYPE_RaspberryBMP085) {
				//all fine here!
			}
			else if (htype == HTYPE_Dummy) {
				//all fine here!
			}
			else if (htype == HTYPE_PiFace) {
				//all fine here!
			}
			else if (htype == HTYPE_Wunderground) {
				if (
					(username=="")||
					(password=="")
					)
					goto exitjson;
			}
			else
				goto exitjson;

			root["status"]="OK";
			root["title"]="AddHardware";
			sprintf(szTmp,
				"INSERT INTO Hardware (Name, Enabled, Type, Address, Port, Username, Password, Mode1, Mode2, Mode3, Mode4, Mode5) VALUES ('%s',%d, %d,'%s',%d,'%s','%s',%d,%d,%d,%d,%d)",
				name.c_str(),
				(senabled=="true")?1:0,
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
			result=m_pMain->m_sql.query(szTmp); //-V519
			if (result.size()>0)
			{
				std::vector<std::string> sd=result[0];
				int ID=atoi(sd[0].c_str());

				m_pMain->AddHardwareFromParams(ID,name,(senabled=="true")?true:false,htype,address,port,username,password,mode1,mode2,mode3,mode4,mode5);
			}
		}
#ifdef WITH_OPENZWAVE
		else if (cparam=="updatezwavenode")
		{
			std::string idx=m_pWebEm->FindValue("idx");
			if (idx=="")
				goto exitjson;
			std::string name=m_pWebEm->FindValue("name");
			std::string senablepolling=m_pWebEm->FindValue("EnablePolling");
			if (
				(name=="")||
				(senablepolling=="")
				)
				goto exitjson;
			root["status"]="OK";
			root["title"]="UpdateZWaveNode";

			sprintf(szTmp,
				"UPDATE ZWaveNodes SET Name='%s', PollTime=%d WHERE (ID==%s)",
				name.c_str(),
				(senablepolling=="true")?1:0,
				idx.c_str()
				);
			result=m_pMain->m_sql.query(szTmp);
			sprintf(szTmp,"SELECT HardwareID from ZWaveNodes WHERE (ID==%s)",idx.c_str());
			result=m_pMain->m_sql.query(szTmp);
			if (result.size()>0)
			{
				int hwid=atoi(result[0][0].c_str());
				CDomoticzHardwareBase *pHardware=m_pMain->GetHardware(hwid);
				if (pHardware!=NULL)
				{
					COpenZWave *pOZWHardware=(COpenZWave*)pHardware;
					pOZWHardware->EnableDisableNodePolling();
				}
			}
		}
		else if (cparam=="applyzwavenodeconfig")
		{
			std::string idx=m_pWebEm->FindValue("idx");
			std::string svaluelist=m_pWebEm->FindValue("valuelist");
			if (
				(idx=="")||
				(svaluelist=="")
				)
				goto exitjson;
			sprintf(szTmp,"SELECT HardwareID,HomeID,NodeID from ZWaveNodes WHERE (ID==%s)",idx.c_str());
			result=m_pMain->m_sql.query(szTmp);
			if (result.size()>0)
			{
				int hwid=atoi(result[0][0].c_str());
				int homeID=atoi(result[0][1].c_str());
				int nodeID=atoi(result[0][2].c_str());
				CDomoticzHardwareBase *pHardware=m_pMain->GetHardware(hwid);
				if (pHardware!=NULL)
				{
					COpenZWave *pOZWHardware=(COpenZWave*)pHardware;
					if (!pOZWHardware->ApplyNodeConfig(homeID,nodeID,svaluelist))
						goto exitjson;
					root["status"]="OK";
					root["title"]="ApplyZWaveNodeConfig";
				}
			}
		}
		else if (cparam=="requestzwavenodeconfig")
		{
			std::string idx=m_pWebEm->FindValue("idx");
			if (idx=="")
				goto exitjson;
			sprintf(szTmp,"SELECT HardwareID,HomeID,NodeID from ZWaveNodes WHERE (ID==%s)",idx.c_str());
			result=m_pMain->m_sql.query(szTmp);
			if (result.size()>0)
			{
				int hwid=atoi(result[0][0].c_str());
				int homeID=atoi(result[0][1].c_str());
				int nodeID=atoi(result[0][2].c_str());
				CDomoticzHardwareBase *pHardware=m_pMain->GetHardware(hwid);
				if (pHardware!=NULL)
				{
					COpenZWave *pOZWHardware=(COpenZWave*)pHardware;
					pOZWHardware->RequestNodeConfig(homeID,nodeID);
					root["status"]="OK";
					root["title"]="RequestZWaveNodeConfig";
				}
			}
		}
#endif
		else if (cparam=="updatehardware")
		{
			std::string idx=m_pWebEm->FindValue("idx");
			if (idx=="")
				goto exitjson;
			std::string name=m_pWebEm->FindValue("name");
			std::string senabled=m_pWebEm->FindValue("enabled");
			std::string shtype=m_pWebEm->FindValue("htype");
			std::string address=m_pWebEm->FindValue("address");
			std::string sport=m_pWebEm->FindValue("port");
			std::string username=m_pWebEm->FindValue("username");
			std::string password=m_pWebEm->FindValue("password");
			if (
				(name=="")||
				(senabled=="")||
				(shtype=="")||
				(sport=="")
				)
				goto exitjson;

			_eHardwareTypes htype=(_eHardwareTypes)atoi(shtype.c_str());

			int port=atoi(sport.c_str());

			if ((htype==HTYPE_RFXtrx315)||(htype==HTYPE_RFXtrx433)||(htype==HTYPE_P1SmartMeter)||(htype==HTYPE_Rego6XX)||(htype==HTYPE_DavisVantage)||(htype==HTYPE_S0SmartMeter)||(htype==HTYPE_OpenThermGateway)||(htype==HTYPE_TeleinfoMeter)||(htype==HTYPE_OpenZWave))
			{
				//USB
			}
			else if ((htype == HTYPE_RFXLAN)||(htype == HTYPE_P1SmartMeterLAN)||(htype == HTYPE_YouLess)||(htype == HTYPE_RazberryZWave)||(htype == HTYPE_OpenThermGatewayTCP)||(htype == HTYPE_LimitlessLights)) {
				//Lan
				if (address=="")
					goto exitjson;
			}
			else if (htype == HTYPE_Domoticz) {
				//Remote Domoticz
				if (address=="")
					goto exitjson;
			}
			else if (htype == HTYPE_TE923) {
				//All fine here
			}
			else if (htype == HTYPE_VOLCRAFTCO20) {
				//All fine here
			}
			else if (htype == HTYPE_1WIRE) {
				//All fine here
			}
			else if (htype == HTYPE_RaspberryBMP085) {
				//All fine here
			}
			else if (htype == HTYPE_Dummy) {
				//All fine here
			}
			else if (htype == HTYPE_PiFace) {
				//All fine here
			}
			else if (htype == HTYPE_Wunderground) {
				if (
					(username=="")||
					(password=="")
					)
					goto exitjson;
			}
			else
				goto exitjson;

			int mode1=atoi(m_pWebEm->FindValue("Mode1").c_str());
			int mode2=atoi(m_pWebEm->FindValue("Mode2").c_str());
			int mode3=atoi(m_pWebEm->FindValue("Mode3").c_str());
			int mode4=atoi(m_pWebEm->FindValue("Mode4").c_str());
			int mode5=atoi(m_pWebEm->FindValue("Mode5").c_str());
			root["status"]="OK";
			root["title"]="UpdateHardware";

			sprintf(szTmp,
				"UPDATE Hardware SET Name='%s', Enabled=%d, Type=%d, Address='%s', Port=%d, Username='%s', Password='%s', Mode1=%d, Mode2=%d, Mode3=%d, Mode4=%d, Mode5=%d WHERE (ID == %s)",
				name.c_str(),
				(senabled=="true")?1:0,
				htype,
				address.c_str(),
				port,
				username.c_str(),
				password.c_str(),
				mode1,mode2,mode3,mode4,mode5,
				idx.c_str()
				);
			result=m_pMain->m_sql.query(szTmp);

			//re-add the device in our system
			int ID=atoi(idx.c_str());
			m_pMain->AddHardwareFromParams(ID,name,(senabled=="true")?true:false,htype,address,port,username,password,mode1,mode2,mode3,mode4,mode5);
		}
		else if (cparam=="addcamera")
		{
			std::string name=m_pWebEm->FindValue("name");
			std::string senabled=m_pWebEm->FindValue("enabled");
			std::string address=m_pWebEm->FindValue("address");
			std::string sport=m_pWebEm->FindValue("port");
			std::string username=m_pWebEm->FindValue("username");
			std::string password=m_pWebEm->FindValue("password");
			std::string tvideourl=m_pWebEm->FindValue("videourl");
			std::string timageurl=m_pWebEm->FindValue("imageurl");
			if (
				(name=="")||
				(address=="")||
				(tvideourl=="")||
				(timageurl=="")
				)
				goto exitjson;

			std::string videourl;
			std::string imageurl;
			if (request_handler::url_decode(tvideourl,videourl))
			{
				if (request_handler::url_decode(timageurl,imageurl))
				{
					videourl=base64_decode(videourl);
					imageurl=base64_decode(imageurl);

					int port=atoi(sport.c_str());
					root["status"]="OK";
					root["title"]="AddCamera";
					sprintf(szTmp,
							"INSERT INTO Cameras (Name, Enabled, Address, Port, Username, Password, VideoURL, ImageURL) VALUES ('%s',%d,'%s',%d,'%s','%s','%s','%s')",
							name.c_str(),
							(senabled=="true")?1:0,
							address.c_str(),
							port,
							base64_encode((const unsigned char*)username.c_str(),username.size()).c_str(),
							base64_encode((const unsigned char*)password.c_str(),password.size()).c_str(),
							videourl.c_str(),
							imageurl.c_str()
							);
					result=m_pMain->m_sql.query(szTmp);
					m_pMain->m_cameras.ReloadCameras();
				}
			}
		}
		else if (cparam=="updatecamera")
		{
			std::string idx=m_pWebEm->FindValue("idx");
			if (idx=="")
				goto exitjson;
			std::string name=m_pWebEm->FindValue("name");
			std::string senabled=m_pWebEm->FindValue("enabled");
			std::string address=m_pWebEm->FindValue("address");
			std::string sport=m_pWebEm->FindValue("port");
    		std::string username=m_pWebEm->FindValue("username");
			std::string password=m_pWebEm->FindValue("password");
			std::string tvideourl=m_pWebEm->FindValue("videourl");
			std::string timageurl=m_pWebEm->FindValue("imageurl");
			if (
				(name=="")||
				(senabled=="")||
				(address=="")||
				(tvideourl=="")||
				(timageurl=="")
                )
				goto exitjson;

			std::string videourl;
			std::string imageurl;
			if (request_handler::url_decode(tvideourl,videourl))
			{
				if (request_handler::url_decode(timageurl,imageurl))
				{
					videourl=base64_decode(videourl);
					imageurl=base64_decode(imageurl);

					int port=atoi(sport.c_str());
		
					root["status"]="OK";
					root["title"]="UpdateCamera";
            
					sprintf(szTmp,
							"UPDATE Cameras SET Name='%s', Enabled=%d, Address='%s', Port=%d, Username='%s', Password='%s', VideoURL='%s', ImageURL='%s' WHERE (ID == %s)",
							name.c_str(),
							(senabled=="true")?1:0,
							address.c_str(),
							port,
							base64_encode((const unsigned char*)username.c_str(),username.size()).c_str(),
							base64_encode((const unsigned char*)password.c_str(),password.size()).c_str(),
							videourl.c_str(),
							imageurl.c_str(),
							idx.c_str()
							);
					result=m_pMain->m_sql.query(szTmp);
					m_pMain->m_cameras.ReloadCameras();
				}
			}
        }
		else if (cparam=="deletecamera")
		{
			std::string idx=m_pWebEm->FindValue("idx");
			if (idx=="")
				goto exitjson;
			root["status"]="OK";
			root["title"]="DeleteCamera";
            
			m_pMain->m_sql.DeleteCamera(idx);
            m_pMain->m_cameras.ReloadCameras();
		}
		else if (cparam=="adduser")
		{
			bool bHaveUser=(m_pWebEm->m_actualuser!="");
			int urights=3;
			if (bHaveUser)
			{
				int iUser=-1;
				iUser=FindUser(m_pWebEm->m_actualuser.c_str());
				if (iUser!=-1)
					urights=(int)m_users[iUser].userrights;
			}
			if (urights<2)
				goto exitjson;

			std::string senabled=m_pWebEm->FindValue("enabled");
			std::string username=m_pWebEm->FindValue("username");
			std::string password=m_pWebEm->FindValue("password");
			std::string srights=m_pWebEm->FindValue("rights");
			std::string sRemoteSharing=m_pWebEm->FindValue("RemoteSharing");
			std::string sTabsEnabled=m_pWebEm->FindValue("TabsEnabled");
			if (
				(senabled=="")||
				(username=="")||
				(password=="")||
				(srights=="")||
				(sRemoteSharing=="")||
				(sTabsEnabled=="")
				)
				goto exitjson;
			int rights=atoi(srights.c_str());
			if (rights!=2)
			{
				if (!FindAdminUser())
				{
					root["message"]="Add a Admin user first! (Or enable Settings/Website Protection)";
					goto exitjson;
				}
			}
			root["status"]="OK";
			root["title"]="AddUser";
			sprintf(szTmp,
				"INSERT INTO Users (Active, Username, Password, Rights, RemoteSharing, TabsEnabled) VALUES (%d,'%s','%s','%d','%d','%d')",
				(senabled=="true")?1:0,
				base64_encode((const unsigned char*)username.c_str(),username.size()).c_str(),
				base64_encode((const unsigned char*)password.c_str(),password.size()).c_str(),
				rights,
				(sRemoteSharing=="true")?1:0,
				atoi(sTabsEnabled.c_str())
				);
			result=m_pMain->m_sql.query(szTmp);
			LoadUsers();
		}
		else if (cparam=="updateuser")
		{
			bool bHaveUser=(m_pWebEm->m_actualuser!="");
			int urights=3;
			if (bHaveUser)
			{
				int iUser=-1;
				iUser=FindUser(m_pWebEm->m_actualuser.c_str());
				if (iUser!=-1)
					urights=(int)m_users[iUser].userrights;
			}
			if (urights<2)
				goto exitjson;

			std::string idx=m_pWebEm->FindValue("idx");
			if (idx=="")
				goto exitjson;
			std::string senabled=m_pWebEm->FindValue("enabled");
			std::string username=m_pWebEm->FindValue("username");
			std::string password=m_pWebEm->FindValue("password");
			std::string srights=m_pWebEm->FindValue("rights");
			std::string sRemoteSharing=m_pWebEm->FindValue("RemoteSharing");
			std::string sTabsEnabled=m_pWebEm->FindValue("TabsEnabled");
			if (
				(senabled=="")||
				(username=="")||
				(password=="")||
				(srights=="")||
				(sRemoteSharing=="")||
				(sTabsEnabled=="")
				)
				goto exitjson;
			int rights=atoi(srights.c_str());
			if (rights!=2)
			{
				if (!FindAdminUser())
				{
					root["message"]="Add a Admin user first! (Or enable Settings/Website Protection)";
					goto exitjson;
				}
			}

			root["status"]="OK";
			root["title"]="UpdateUser";

			sprintf(szTmp,
				"UPDATE Users SET Active=%d, Username='%s', Password='%s', Rights=%d, RemoteSharing=%d, TabsEnabled=%d WHERE (ID == %s)",
				(senabled=="true")?1:0,
				base64_encode((const unsigned char*)username.c_str(),username.size()).c_str(),
				base64_encode((const unsigned char*)password.c_str(),password.size()).c_str(),
				rights,
				(sRemoteSharing=="true")?1:0,
				atoi(sTabsEnabled.c_str()),
				idx.c_str()
				);
			result=m_pMain->m_sql.query(szTmp);
			LoadUsers();
		}
		else if (cparam=="deleteuser")
		{
			bool bHaveUser=(m_pWebEm->m_actualuser!="");
			int urights=3;
			if (bHaveUser)
			{
				int iUser=-1;
				iUser=FindUser(m_pWebEm->m_actualuser.c_str());
				if (iUser!=-1)
					urights=(int)m_users[iUser].userrights;
			}
			if (urights<2)
				goto exitjson;

			std::string idx=m_pWebEm->FindValue("idx");
			if (idx=="")
				goto exitjson;

			root["status"]="OK";
			root["title"]="DeleteUser";
			sprintf(szTmp,"DELETE FROM Users WHERE (ID == %s)",idx.c_str());
			result=m_pMain->m_sql.query(szTmp);

			szQuery.clear();
			szQuery.str("");
			szQuery << "DELETE FROM SharedDevices WHERE (SharedUserID == " << idx << ")";
			result=m_pMain->m_sql.query(szQuery.str()); //-V519

			LoadUsers();
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
			std::string randomness=m_pWebEm->FindValue("randomness");
			std::string scmd=m_pWebEm->FindValue("command");
			std::string sdays=m_pWebEm->FindValue("days");
			std::string slevel=m_pWebEm->FindValue("level");	//in percentage
			std::string shue=m_pWebEm->FindValue("hue");
			if (
				(idx=="")||
				(active=="")||
				(stimertype=="")||
				(shour=="")||
				(smin=="")||
				(randomness=="")||
				(scmd=="")||
				(sdays=="")
				)
				goto exitjson;
			unsigned char hour = atoi(shour.c_str());
			unsigned char min = atoi(smin.c_str());
			unsigned char icmd = atoi(scmd.c_str());
			unsigned char iTimerType=atoi(stimertype.c_str());
			int days=atoi(sdays.c_str());
			unsigned char level=atoi(slevel.c_str());
			int hue=atoi(shue.c_str());
			sprintf(szData,"%02d:%02d",hour,min);
			root["status"]="OK";
			root["title"]="AddTimer";
			sprintf(szTmp,
				"INSERT INTO Timers (Active, DeviceRowID, Time, Type, UseRandomness, Cmd, Level, Hue, Days) VALUES (%d,%s,'%s',%d,%d,%d,%d,%d,%d)",
				(active=="true")?1:0,
				idx.c_str(),
				szData,
				iTimerType,
				(randomness=="true")?1:0,
				icmd,
				level,
				hue,
				days
				);
			result=m_pMain->m_sql.query(szTmp);
			m_pMain->m_scheduler.ReloadSchedules();
		}
		else if (cparam=="addscenetimer")
		{
			std::string idx=m_pWebEm->FindValue("idx");
			std::string active=m_pWebEm->FindValue("active");
			std::string stimertype=m_pWebEm->FindValue("timertype");
			std::string shour=m_pWebEm->FindValue("hour");
			std::string smin=m_pWebEm->FindValue("min");
			std::string randomness=m_pWebEm->FindValue("randomness");
			std::string scmd=m_pWebEm->FindValue("command");
			std::string sdays=m_pWebEm->FindValue("days");
			std::string slevel=m_pWebEm->FindValue("level");	//in percentage
			if (
				(idx=="")||
				(active=="")||
				(stimertype=="")||
				(shour=="")||
				(smin=="")||
				(randomness=="")||
				(scmd=="")||
				(sdays=="")
				)
				goto exitjson;
			unsigned char hour = atoi(shour.c_str());
			unsigned char min = atoi(smin.c_str());
			unsigned char icmd = atoi(scmd.c_str());
			unsigned char iTimerType=atoi(stimertype.c_str());
			int days=atoi(sdays.c_str());
			unsigned char level=atoi(slevel.c_str());
			sprintf(szData,"%02d:%02d",hour,min);
			root["status"]="OK";
			root["title"]="AddSceneTimer";
			sprintf(szTmp,
				"INSERT INTO SceneTimers (Active, SceneRowID, Time, Type, UseRandomness, Cmd, Level, Days) VALUES (%d,%s,'%s',%d,%d,%d,%d,%d)",
				(active=="true")?1:0,
				idx.c_str(),
				szData,
				iTimerType,
				(randomness=="true")?1:0,
				icmd,
				level,
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
			std::string randomness=m_pWebEm->FindValue("randomness");
			std::string scmd=m_pWebEm->FindValue("command");
			std::string sdays=m_pWebEm->FindValue("days");
			std::string slevel=m_pWebEm->FindValue("level");	//in percentage
			std::string shue=m_pWebEm->FindValue("hue");
			if (
				(idx=="")||
				(active=="")||
				(stimertype=="")||
				(shour=="")||
				(smin=="")||
				(randomness=="")||
				(scmd=="")||
				(sdays=="")
				)
				goto exitjson;
			unsigned char hour = atoi(shour.c_str());
			unsigned char min = atoi(smin.c_str());
			unsigned char icmd = atoi(scmd.c_str());
			unsigned char iTimerType=atoi(stimertype.c_str());
			int days=atoi(sdays.c_str());
			unsigned char level=atoi(slevel.c_str());
			int hue=atoi(shue.c_str());
			sprintf(szData,"%02d:%02d",hour,min);
			root["status"]="OK";
			root["title"]="UpdateTimer";
			sprintf(szTmp,
				"UPDATE Timers SET Active=%d, Time='%s', Type=%d, UseRandomness=%d, Cmd=%d, Level=%d, Hue=%d, Days=%d WHERE (ID == %s)",
				(active=="true")?1:0,
				szData,
				iTimerType,
				(randomness=="true")?1:0,
				icmd,
				level,
				hue,
				days,
				idx.c_str()
				);
			result=m_pMain->m_sql.query(szTmp);
			m_pMain->m_scheduler.ReloadSchedules();
		}
		else if (cparam=="updatescenetimer")
		{
			std::string idx=m_pWebEm->FindValue("idx");
			std::string active=m_pWebEm->FindValue("active");
			std::string stimertype=m_pWebEm->FindValue("timertype");
			std::string shour=m_pWebEm->FindValue("hour");
			std::string smin=m_pWebEm->FindValue("min");
			std::string randomness=m_pWebEm->FindValue("randomness");
			std::string scmd=m_pWebEm->FindValue("command");
			std::string sdays=m_pWebEm->FindValue("days");
			std::string slevel=m_pWebEm->FindValue("level");	//in percentage
			if (
				(idx=="")||
				(active=="")||
				(stimertype=="")||
				(shour=="")||
				(smin=="")||
				(randomness=="")||
				(scmd=="")||
				(sdays=="")
				)
				goto exitjson;
			unsigned char hour = atoi(shour.c_str());
			unsigned char min = atoi(smin.c_str());
			unsigned char icmd = atoi(scmd.c_str());
			unsigned char iTimerType=atoi(stimertype.c_str());
			int days=atoi(sdays.c_str());
			unsigned char level=atoi(slevel.c_str());
			sprintf(szData,"%02d:%02d",hour,min);
			root["status"]="OK";
			root["title"]="UpdateSceneTimer";
			sprintf(szTmp,
				"UPDATE SceneTimers SET Active=%d, Time='%s', Type=%d, UseRandomness=%d, Cmd=%d, Level=%d, Days=%d WHERE (ID == %s)",
				(active=="true")?1:0,
				szData,
				iTimerType,
				(randomness=="true")?1:0,
				icmd,
				level,
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
			root["status"]="OK";
			root["title"]="DeleteTimer";
			sprintf(szTmp,
				"DELETE FROM Timers WHERE (ID == %s)",
				idx.c_str()
				);
			result=m_pMain->m_sql.query(szTmp);
			m_pMain->m_scheduler.ReloadSchedules();
		}
		else if (cparam=="deletescenetimer")
		{
			std::string idx=m_pWebEm->FindValue("idx");
			if (idx=="")
				goto exitjson;
			root["status"]="OK";
			root["title"]="DeleteSceneTimer";
			sprintf(szTmp,
				"DELETE FROM SceneTimers WHERE (ID == %s)",
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
				(dType!=pTypeLighting6)&&
				(dType!=pTypeLimitlessLights)&&
				(dType!=pTypeSecurity1)&&
				(dType!=pTypeBlinds)&&
				(dType!=pTypeChime)
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
			root["status"]="OK";
			root["title"]="ClearTimer";
			sprintf(szTmp,
				"DELETE FROM Timers WHERE (DeviceRowID == %s)",
				idx.c_str()
				);
			result=m_pMain->m_sql.query(szTmp);
			m_pMain->m_scheduler.ReloadSchedules();
		}
		else if (cparam=="clearscenetimers")
		{
			std::string idx=m_pWebEm->FindValue("idx");
			if (idx=="")
				goto exitjson;
			root["status"]="OK";
			root["title"]="ClearSceneTimer";
			sprintf(szTmp,
				"DELETE FROM SceneTimers WHERE (SceneRowID == %s)",
				idx.c_str()
				);
			result=m_pMain->m_sql.query(szTmp);
			m_pMain->m_scheduler.ReloadSchedules();
		}
		else if (cparam=="setscenecode")
		{
			std::string idx=m_pWebEm->FindValue("idx");
			std::string cmnd=m_pWebEm->FindValue("cmnd");
			if (
				(idx=="")||
				(cmnd=="")
				)
				goto exitjson;
			std::string devid=m_pWebEm->FindValue("devid");
			if (devid=="")
				goto exitjson;
			root["status"]="OK";
			root["title"]="SetSceneCode";
			szQuery.clear();
			szQuery.str("");
			szQuery << "SELECT HardwareID, DeviceID, Unit, Type, SubType FROM DeviceStatus WHERE (ID==" << devid << ")";
			result=m_pMain->m_sql.query(szQuery.str());
			if (result.size()>0)
			{
				sprintf(szTmp,
					"UPDATE Scenes SET HardwareID=%d, DeviceID='%s', Unit=%d, Type=%d, SubType=%d, ListenCmd=%d WHERE (ID == %s)",
					atoi(result[0][0].c_str()),
					result[0][1].c_str(),
					atoi(result[0][2].c_str()),
					atoi(result[0][3].c_str()),
					atoi(result[0][4].c_str()),
					atoi(cmnd.c_str()),
					idx.c_str()
					);
				result=m_pMain->m_sql.query(szTmp);
				//Sanity Check, remove all SceneDevice that has this code
				szQuery.clear();
				szQuery.str("");
				szQuery << "DELETE FROM SceneDevices WHERE (SceneRowID==" << idx << " AND DeviceRowID==" << devid << ")";
				result=m_pMain->m_sql.query(szQuery.str()); //-V519
			}
		}
		else if (cparam=="removescenecode")
		{
			std::string idx=m_pWebEm->FindValue("idx");
			if (idx=="")
				goto exitjson;
			root["status"]="OK";
			root["title"]="RemoveSceneCode";
			sprintf(szTmp,
				"UPDATE Scenes SET HardwareID=%d, DeviceID='%s', Unit=%d, Type=%d, SubType=%d WHERE (ID == %s)",
				0,
				"",
				0,
				0,
				0,
				idx.c_str()
				);
			result=m_pMain->m_sql.query(szTmp);
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
					sleep_milliseconds(100);
					cntr++;
				}
			}
			if (bReceivedSwitch)
			{
				//check if used
				szQuery.clear();
				szQuery.str("");
				szQuery << "SELECT Name, Used, nValue FROM DeviceStatus WHERE (ID==" << m_pMain->m_sql.m_LastSwitchRowID << ")";
				result=m_pMain->m_sql.query(szQuery.str());
				if (result.size()>0)
				{
					root["status"]="OK";
					root["title"]="LearnSW";
					root["ID"]=m_pMain->m_sql.m_LastSwitchID;
					root["idx"]=m_pMain->m_sql.m_LastSwitchRowID;
					root["Name"]=result[0][0];
					root["Used"]=atoi(result[0][1].c_str());
					root["Cmd"]=atoi(result[0][2].c_str());
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
		else if (cparam == "makescenefavorite")
		{
			std::string idx=m_pWebEm->FindValue("idx");
			std::string sisfavorite=m_pWebEm->FindValue("isfavorite");
			if ((idx=="")||(sisfavorite==""))
				goto exitjson;
			int isfavorite=atoi(sisfavorite.c_str());
			szQuery.clear();
			szQuery.str("");
			szQuery << "UPDATE Scenes SET Favorite=" << isfavorite << " WHERE (ID == " << idx << ")";
			result=m_pMain->m_sql.query(szQuery.str());
			if (result.size()>0)
			{
				root["status"]="OK";
				root["title"]="MakeSceneFavorite";
			}
		} //makescenefavorite
        else if (cparam=="resetsecuritystatus")
		{
			int urights=3;
			if (bHaveUser)
			{
				int iUser=-1;
				iUser=FindUser(m_pWebEm->m_actualuser.c_str());
				if (iUser!=-1)
					urights=(int)m_users[iUser].userrights;
			}
			if (urights<1)
				goto exitjson;

            std::string idx=m_pWebEm->FindValue("idx");
			std::string switchcmd=m_pWebEm->FindValue("switchcmd");

			if ((idx=="")||(switchcmd==""))
				goto exitjson;

			root["status"]="OK";
			root["title"]="ResetSecurityStatus";

            int nValue=-1;
            
            // Change to generic *Security_Status_Desc lookup...
            
            if (switchcmd == "Panic End") {
                nValue = 7;
            }
			else if (switchcmd == "Normal") {
				nValue = 0;
			}
            
            if (nValue>=0)
			{
                
                szQuery.clear();
                szQuery.str("");
                szQuery << "UPDATE DeviceStatus SET nValue=" << nValue << " WHERE (ID == " << idx << ")";
        		result=m_pMain->m_sql.query(szQuery.str());
                if (result.size()>0) {
                    root["status"]="OK";
                    root["title"]="SwitchLight";
                }
                else {
                    goto exitjson;
                }
            }
            else 
			{
				goto exitjson;
            }
        }
		else if (cparam=="logout")
		{
			root["status"]="OK";
			root["title"]="Logout";
			m_retstr="authorize";
			return m_retstr;
			
		}
        else if (cparam=="switchlight")
		{
			int urights=3;
			if (bHaveUser)
			{
				int iUser=-1;
				iUser=FindUser(m_pWebEm->m_actualuser.c_str());
				if (iUser!=-1)
					urights=(int)m_users[iUser].userrights;
			}
			if (urights<1)
				goto exitjson;

			std::string idx=m_pWebEm->FindValue("idx");
			std::string switchcmd=m_pWebEm->FindValue("switchcmd");
			std::string level=m_pWebEm->FindValue("level");
			if ((idx=="")||(switchcmd==""))
				goto exitjson;

			if (m_pMain->SwitchLight(idx,switchcmd,level,"-1")==true)
			{
				root["status"]="OK";
				root["title"]="SwitchLight";
			}
		} //(rtype=="switchlight")
		else if (cparam=="switchscene")
		{
			int urights=3;
			if (bHaveUser)
			{
				int iUser=-1;
				iUser=FindUser(m_pWebEm->m_actualuser.c_str());
				if (iUser!=-1)
					urights=(int)m_users[iUser].userrights;
			}
			if (urights<1)
				goto exitjson;

			std::string idx=m_pWebEm->FindValue("idx");
			std::string switchcmd=m_pWebEm->FindValue("switchcmd");
			if ((idx=="")||(switchcmd==""))
				goto exitjson;

			if (m_pMain->SwitchScene(idx,switchcmd)==true)
			{
				root["status"]="OK";
				root["title"]="SwitchScene";
			}
		} //(rtype=="switchscene")
		else if (cparam =="getSunRiseSet") {
			int nValue=0;
			std::string sValue;
			if (m_pMain->m_sql.GetTempVar("SunRiseSet",nValue,sValue))
			{
				std::vector<std::string> strarray;
				StringSplit(sValue, ";", strarray);
				if (strarray.size()==2)
				{
					struct tm loctime;
					time_t now = mytime(NULL);

					localtime_r (&now, &loctime );
					strftime (szTmp,80,"%b %d %Y %X",&loctime);

					root["status"]="OK";
					root["title"]="getSunRiseSet";
					root["ServerTime"]=szTmp;
					root["Sunrise"]=strarray[0];
					root["Sunset"]=strarray[1];
				}
			}
		}
        else if (cparam =="getServerTime") {

            struct tm loctime;
			time_t now = mytime(NULL);

            localtime_r (&now, &loctime );
			strftime (szTmp,80,"%b %d %Y %X",&loctime);
            
            root["status"]="OK";
			root["title"]="getServerTime";
			root["ServerTime"]=szTmp;
		}
		else if (cparam=="getsecstatus")
		{
			root["status"]="OK";
			root["title"]="GetSecStatus";
			int secstatus=0;
			m_pMain->m_sql.GetPreferencesVar("SecStatus", secstatus);
			root["secstatus"]=secstatus;
		}
		else if (cparam=="setsecstatus")
		{
			std::string ssecstatus=m_pWebEm->FindValue("secstatus");
			std::string seccode=m_pWebEm->FindValue("seccode");
			if ((ssecstatus=="")||(seccode==""))
			{
				root["message"]="WRONG CODE";
				goto exitjson;
			}
			root["title"]="SetSecStatus";
			seccode=base64_encode((const unsigned char*)seccode.c_str(),seccode.size());
			std::string rpassword;
			int nValue=1;
			m_pMain->m_sql.GetPreferencesVar("SecPassword",nValue,rpassword);
			if (seccode!=rpassword)
			{
				root["status"]="ERROR";
				root["message"]="WRONG CODE";
				goto exitjson;
			}
			root["status"]="OK";
			int iSecStatus=atoi(ssecstatus.c_str());
			m_pMain->UpdateDomoticzSecurityStatus(iSecStatus);
		}
		else if (cparam=="setcolbrightnessvalue")
		{
			std::string idx=m_pWebEm->FindValue("idx");
			std::string hue=m_pWebEm->FindValue("hue");
			std::string brightness=m_pWebEm->FindValue("brightness");
			std::string iswhite=m_pWebEm->FindValue("iswhite");
			
			if ((idx=="")||(hue=="")||(brightness=="")||(iswhite==""))
			{
				goto exitjson;
			}

			unsigned long long ID;
			std::stringstream s_strid;
			s_strid << idx;
			s_strid >> ID;

			if (iswhite!="true")
			{
				//convert hue from 360 steps to 255
				double dval;
				dval=(255.0/360.0)*atof(hue.c_str());
				int ival;
				ival=round(dval);
				m_pMain->SwitchLight(ID,"Set Color",ival,-1);
			}
			else
			{
				m_pMain->SwitchLight(ID,"Set White",0,-1);
			}
			sleep_milliseconds(100);
			m_pMain->SwitchLight(ID,"Set Brightness",(unsigned char)atoi(brightness.c_str()),-1);
		}
	} //(rtype=="command")
	else if (rtype=="getshareduserdevices")
	{
		std::string idx=m_pWebEm->FindValue("idx");
		if (idx=="")
			goto exitjson;
		root["status"]="OK";
		root["title"]="GetSharedUserDevices";

		szQuery.clear();
		szQuery.str("");
		szQuery << "SELECT DeviceRowID FROM SharedDevices WHERE (SharedUserID == " << idx << ")";
		result=m_pMain->m_sql.query(szQuery.str());
		if (result.size()>0)
		{
			std::vector<std::vector<std::string> >::const_iterator itt;
			int ii=0;
			for (itt=result.begin(); itt!=result.end(); ++itt)
			{
				std::vector<std::string> sd=*itt;
				root["result"][ii]["DeviceRowIdx"]=sd[0];
				ii++;
			}
		}
	}
	else if (rtype=="setshareduserdevices")
	{
		std::string idx=m_pWebEm->FindValue("idx");
		std::string userdevices=m_pWebEm->FindValue("devices");
		if (idx=="")
			goto exitjson;
		root["status"]="OK";
		root["title"]="SetSharedUserDevices";
		std::vector<std::string> strarray;
		StringSplit(userdevices, ";", strarray);

		//First delete all devices for this user, then add the (new) onces
		szQuery.clear();
		szQuery.str("");
		szQuery << "DELETE FROM SharedDevices WHERE (SharedUserID == " << idx << ")";
		result=m_pMain->m_sql.query(szQuery.str());

		int nDevices=(int)strarray.size();
		for (int ii=0; ii<nDevices; ii++)
		{
			szQuery.clear();
			szQuery.str("");
			szQuery << "INSERT INTO SharedDevices (SharedUserID,DeviceRowID) VALUES ('" << idx << "','" << strarray[ii] << "')";
			result=m_pMain->m_sql.query(szQuery.str());
		}
		m_pMain->LoadSharedUsers();
	}
	else if (rtype=="setused")
	{
		std::string idx=m_pWebEm->FindValue("idx");
		std::string name=m_pWebEm->FindValue("name");
		std::string sused=m_pWebEm->FindValue("used");
		std::string sswitchtype=m_pWebEm->FindValue("switchtype");
		std::string maindeviceidx=m_pWebEm->FindValue("maindeviceidx");
		std::string addjvalue=m_pWebEm->FindValue("addjvalue");
		std::string addjmulti=m_pWebEm->FindValue("addjmulti");
		std::string addjvalue2=m_pWebEm->FindValue("addjvalue2");
		std::string addjmulti2=m_pWebEm->FindValue("addjmulti2");
		std::string setPoint=m_pWebEm->FindValue("setpoint");
		std::string sCustomImage=m_pWebEm->FindValue("customimage");

		int switchtype=-1;
		if (sswitchtype!="")
			switchtype=atoi(sswitchtype.c_str());

		if ((idx=="")||(sused==""))
			goto exitjson;
		int used=(sused=="true")?1:0;
		if (maindeviceidx!="")
			used=0;

		int CustomImage=0;
		if (sCustomImage!="")
			CustomImage=atoi(sCustomImage.c_str());

        std::stringstream sstridx(idx);
        unsigned long long ullidx;
        sstridx >> ullidx;
        m_pMain->m_eventsystem.WWWUpdateSingleState(ullidx, name);
        
		szQuery.clear();
		szQuery.str("");

		if (setPoint!="")
		{
			szQuery << "UPDATE DeviceStatus SET Used=" << used << ", sValue='" << setPoint << "' WHERE (ID == " << idx << ")";
		}
		else
		{

			if (name=="")
			{
				szQuery << "UPDATE DeviceStatus SET Used=" << used << " WHERE (ID == " << idx << ")";
			}
			else
			{
				if (switchtype==-1)
					szQuery << "UPDATE DeviceStatus SET Used=" << used << ", Name='" << name << "' WHERE (ID == " << idx << ")";
				else
					szQuery << "UPDATE DeviceStatus SET Used=" << used << ", Name='" << name << "', SwitchType=" << switchtype << ", CustomImage=" << CustomImage << " WHERE (ID == " << idx << ")";
			}
		}
		result=m_pMain->m_sql.query(szQuery.str());

		if (setPoint!="")
		{
			m_pMain->SetSetPoint(idx,(float)atof(setPoint.c_str()));
		}

		if (addjvalue!="")
		{
			double faddjvalue=atof(addjvalue.c_str());
			szQuery.clear();
			szQuery.str("");
			szQuery << "UPDATE DeviceStatus SET AddjValue=" << faddjvalue << " WHERE (ID == " << idx << ")";
			result=m_pMain->m_sql.query(szQuery.str());
		}
		if (addjmulti!="")
		{
			double faddjmulti=atof(addjmulti.c_str());
			if (faddjmulti==0)
				faddjmulti=1;
			szQuery.clear();
			szQuery.str("");
			szQuery << "UPDATE DeviceStatus SET AddjMulti=" << faddjmulti << " WHERE (ID == " << idx << ")";
			result=m_pMain->m_sql.query(szQuery.str());
		}
		if (addjvalue2!="")
		{
			double faddjvalue2=atof(addjvalue2.c_str());
			szQuery.clear();
			szQuery.str("");
			szQuery << "UPDATE DeviceStatus SET AddjValue2=" << faddjvalue2 << " WHERE (ID == " << idx << ")";
			result=m_pMain->m_sql.query(szQuery.str());
		}
		if (addjmulti2!="")
		{
			double faddjmulti2=atof(addjmulti2.c_str());
			if (faddjmulti2==0)
				faddjmulti2=1;
			szQuery.clear();
			szQuery.str("");
			szQuery << "UPDATE DeviceStatus SET AddjMulti2=" << faddjmulti2 << " WHERE (ID == " << idx << ")";
			result=m_pMain->m_sql.query(szQuery.str());
		}

		if (used==0)
		{
			bool bRemoveSubDevices=(m_pWebEm->FindValue("RemoveSubDevices")=="true");

			if (bRemoveSubDevices)
			{
				//if this device was a slave device, remove it
				sprintf(szTmp,"DELETE FROM LightSubDevices WHERE (DeviceRowID == %s)",idx.c_str());
				m_pMain->m_sql.query(szTmp);
			}
			sprintf(szTmp,"DELETE FROM LightSubDevices WHERE (ParentID == %s)",idx.c_str());
			m_pMain->m_sql.query(szTmp);

			sprintf(szTmp,"DELETE FROM Timers WHERE (DeviceRowID == %s)",idx.c_str());
			m_pMain->m_sql.query(szTmp);
		}

		if (maindeviceidx!="")
		{
			if (maindeviceidx!=idx)
			{
				//this is a sub device for another light/switch
				//first check if it is not already a sub device
				szQuery.clear();
				szQuery.str("");
				szQuery << "SELECT ID FROM LightSubDevices WHERE (DeviceRowID=='" << idx << "') AND (ParentID =='" << maindeviceidx << "')";
				result=m_pMain->m_sql.query(szQuery.str());
				if (result.size()==0)
				{
					//no it is not, add it
					sprintf(szTmp,
						"INSERT INTO LightSubDevices (DeviceRowID, ParentID) VALUES ('%s','%s')",
						idx.c_str(),
						maindeviceidx.c_str()
						);
					result=m_pMain->m_sql.query(szTmp);
				}
			}
		}
		if ((used==0)&&(maindeviceidx==""))
		{
			//really remove it, including log etc
			m_pMain->m_sql.DeleteDevice(idx);
		}
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
		m_pMain->m_scheduler.ReloadSchedules();
	} //(rtype=="deletedevice")
	else if (rtype=="addscene")
	{
		std::string name=m_pWebEm->FindValue("name");
		if (name=="")
		{
			root["status"]="ERR";
			root["message"]="No Scene Name specified!";
			goto exitjson;
		}
		std::string stype=m_pWebEm->FindValue("scenetype");
		if (stype=="")
		{
			root["status"]="ERR";
			root["message"]="No Scene Type specified!";
			goto exitjson;
		}
		if (m_pMain->m_sql.DoesSceneByNameExits(name)==true)
		{
			root["status"]="ERR";
			root["message"]="A Scene with this Name already Exits!";
			goto exitjson;
		}
		root["status"]="OK";
		root["title"]="AddScene";
		sprintf(szTmp,
			"INSERT INTO Scenes (Name,SceneType) VALUES ('%s',%d)",
			name.c_str(),
			atoi(stype.c_str())
			);
		result=m_pMain->m_sql.query(szTmp);

	} //(rtype=="addscene")
	else if (rtype=="deletescene")
	{
		std::string idx=m_pWebEm->FindValue("idx");
		if (idx=="")
			goto exitjson;
		root["status"]="OK";
		root["title"]="DeleteScene";
		sprintf(szTmp,"DELETE FROM Scenes WHERE (ID == %s)",idx.c_str());
		m_pMain->m_sql.query(szTmp);
		sprintf(szTmp,"DELETE FROM SceneDevices WHERE (SceneRowID == %s)",idx.c_str());
		m_pMain->m_sql.query(szTmp);
		sprintf(szTmp,"DELETE FROM SceneTimers WHERE (SceneRowID == %s)",idx.c_str());
		m_pMain->m_sql.query(szTmp);
	} //(rtype=="deletescene")
	else if (rtype=="updatescene")
	{
		std::string idx=m_pWebEm->FindValue("idx");
		std::string name=m_pWebEm->FindValue("name");
		if ((idx=="")||(name==""))
			goto exitjson;
		std::string stype=m_pWebEm->FindValue("scenetype");
		if (stype=="")
		{
			root["status"]="ERR";
			root["message"]="No Scene Type specified!";
			goto exitjson;
		}

		root["status"]="OK";
		root["title"]="UpdateScene";
		sprintf(szTmp,"UPDATE Scenes SET Name='%s', SceneType=%d WHERE (ID == %s)",
				name.c_str(),
				atoi(stype.c_str()),
				idx.c_str()
				);
		m_pMain->m_sql.query(szTmp);
	} //(rtype=="updatescene")
	else if (rtype=="createvirtualsensor")
	{
		std::string idx=m_pWebEm->FindValue("idx");
		std::string ssensortype=m_pWebEm->FindValue("sensortype");
		if ((idx=="")||(ssensortype==""))
			goto exitjson;

		bool bCreated=false;
		int iSensorType=atoi(ssensortype.c_str());

		int HwdID=atoi(idx.c_str());

		//Make a unique number for ID
		szQuery.clear();
		szQuery.str("");
		szQuery << "SELECT MAX(ID) FROM DeviceStatus";
		result=m_pMain->m_sql.query(szQuery.str());

		unsigned long nid=1; //could be the first device ever

		if (result.size()>0)
		{
			nid=atol(result[0][0].c_str());
		}
		nid+=82000;
		char ID[40];
		sprintf(ID,"%ld",nid);

		std::string devname;

		switch (iSensorType)
		{
		case pTypeTEMP:
			m_pMain->m_sql.UpdateValue(HwdID, ID,1,pTypeTEMP,sTypeTEMP1,10,255,0,"0.0",devname);
			bCreated=true;
			break;
		case pTypeHUM:
			m_pMain->m_sql.UpdateValue(HwdID, ID,1,pTypeHUM,sTypeTEMP1,10,255,0,"50",devname);
			bCreated=true;
			break;
		case pTypeTEMP_HUM:
			m_pMain->m_sql.UpdateValue(HwdID, ID,1,pTypeTEMP_HUM,sTypeTH1,10,255,0,"0.0;50;1",devname);
			bCreated=true;
			break;
		case pTypeTEMP_HUM_BARO:
			m_pMain->m_sql.UpdateValue(HwdID, ID,1,pTypeTEMP_HUM_BARO,sTypeTHB1,10,255,0,"0.0;50;1;1010;1",devname);
			bCreated=true;
			break;
		case pTypeWIND:
			m_pMain->m_sql.UpdateValue(HwdID, ID,1,pTypeWIND,sTypeWIND1,10,255,0,"0;N;0;0;0;0",devname);
			bCreated=true;
			break;
		case pTypeRAIN:
			m_pMain->m_sql.UpdateValue(HwdID, ID,1,pTypeRAIN,sTypeRAIN3,10,255,0,"0;0",devname);
			bCreated=true;
			break;
		case pTypeUV:
			m_pMain->m_sql.UpdateValue(HwdID, ID,1,pTypeUV,sTypeUV1,10,255,0,"0;0",devname);
			bCreated=true;
			break;
		}
		if (bCreated)
		{
			root["status"]="OK";
			root["title"]="CreateVirtualSensor";
		}

	} //(rtype=="createvirtualsensor")
	else if (rtype=="custom_light_icons")
	{
		std::vector<_tCustomIcon>::const_iterator itt;
		int ii=0;
		for (itt=m_custom_light_icons.begin(); itt!=m_custom_light_icons.end(); ++itt)
		{
			root["result"][ii]["imageSrc"]=itt->RootFile;
			root["result"][ii]["text"]=itt->Title;
			root["result"][ii]["description"]=itt->Description;
			ii++;
		}
	}
	else if (rtype=="plans")
	{
		root["status"]="OK";
		root["title"]="Plans";

		szQuery.clear();
		szQuery.str("");
		szQuery << "SELECT ID, Name, [Order] FROM Plans ORDER BY [Order]";
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
				root["result"][ii]["Order"]=sd[2];

				unsigned int totDevices=0;

				szQuery.clear();
				szQuery.str("");
				szQuery << "SELECT COUNT(*) FROM DeviceToPlansMap WHERE (PlanID=='" << sd[0] << "')";
				result2=m_pMain->m_sql.query(szQuery.str());
				if (result.size()>0)
				{
					totDevices=(unsigned int)atoi(result2[0][0].c_str());
				}
				root["result"][ii]["Devices"]=totDevices;

				ii++;
			}
		}
	} //plans
	else if (rtype=="scenes")
	{
		root["status"]="OK";
		root["title"]="Scenes";

		szQuery.clear();
		szQuery.str("");
		szQuery << "SELECT ID, Name, HardwareID, Favorite, nValue, SceneType, LastUpdate FROM Scenes ORDER BY [Order]";
		result=m_pMain->m_sql.query(szQuery.str());
		if (result.size()>0)
		{
			std::vector<std::vector<std::string> >::const_iterator itt;
			int ii=0;
			for (itt=result.begin(); itt!=result.end(); ++itt)
			{
				std::vector<std::string> sd=*itt;

				unsigned char nValue=atoi(sd[4].c_str());
				unsigned char scenetype=atoi(sd[5].c_str());
				root["result"][ii]["idx"]=sd[0];
				root["result"][ii]["Name"]=sd[1];
				root["result"][ii]["HardwareID"]=atoi(sd[2].c_str());
				root["result"][ii]["Favorite"]=atoi(sd[3].c_str());

				if (scenetype==0)
				{
					root["result"][ii]["Type"]="Scene";
				}
				else
				{
					root["result"][ii]["Type"]="Group";
				}

				root["result"][ii]["LastUpdate"]=sd[6].c_str();

				if (nValue==0)
					root["result"][ii]["Status"]="Off";
				else if (nValue==1)
					root["result"][ii]["Status"]="On";
				else
					root["result"][ii]["Status"]="Mixed";
				root["result"][ii]["Timers"]=(m_pMain->m_sql.HasSceneTimers(sd[0])==true)?"true":"false";
				unsigned long long camIDX=m_pMain->m_cameras.IsDevSceneInCamera(1,sd[0]);
				root["result"][ii]["UsedByCamera"]=(camIDX!=0)?true:false;
				if (camIDX!=0) {
					root["result"][ii]["CameraFeed"]=m_pMain->m_cameras.GetCameraFeedURL(camIDX);
				}
				ii++;
			}
		}
	} //(rtype=="scenes")
	else if (rtype=="events")
	{
		//root["status"]="OK";
		root["title"]="Events";
        
        std::string cparam=m_pWebEm->FindValue("param");

        if (cparam=="list")
		{
			root["title"]="ListEvents";
            
            int ii=0;
            
            szQuery.clear();
            szQuery.str("");
            szQuery << "SELECT ID, Name, XMLStatement, Status FROM EventMaster ORDER BY ID ASC";
            result=m_pMain->m_sql.query(szQuery.str());
            if (result.size()>0)
            {
                std::vector<std::vector<std::string> >::const_iterator itt;
                for (itt=result.begin(); itt!=result.end(); ++itt)
                {
                    std::vector<std::string> sd=*itt;
                    std::string ID=sd[0];
					std::string Name=sd[1];
                    //std::string XMLStatement=sd[2];
                    std::string eventStatus = sd[3];
                    root["result"][ii]["id"]=ID;
                    root["result"][ii]["name"]=Name;
                    root["result"][ii]["eventstatus"]=eventStatus;
                    ii++;
                }
            }
            root["status"]="OK";
        }
        else if (cparam=="load")
		{
			root["title"]="LoadEvent";
            
            std::string idx=m_pWebEm->FindValue("event");
            if (idx=="")
                goto exitjson;
   
            int ii=0;
            
            szQuery.clear();
            szQuery.str("");
            szQuery << "SELECT ID, Name, XMLStatement, Status FROM EventMaster WHERE (ID==" << idx << ")";
            result=m_pMain->m_sql.query(szQuery.str());
            if (result.size()>0)
            {
                std::vector<std::vector<std::string> >::const_iterator itt;
                for (itt=result.begin(); itt!=result.end(); ++itt)
                {
                    std::vector<std::string> sd=*itt;
                    std::string ID=sd[0];
					std::string Name=sd[1];
                    std::string XMLStatement=sd[2];
                    std::string eventStatus = sd[3];
                    //int Status=atoi(sd[3].c_str());
                    
                    root["result"][ii]["id"]=ID;
                    root["result"][ii]["name"]=Name;
                    root["result"][ii]["xmlstatement"]=XMLStatement;
                    root["result"][ii]["eventstatus"]=eventStatus;
                    ii++;
                }
                root["status"]="OK";
            }
        }

        else if (cparam=="create")
		{
        
			root["title"]="AddEvent";

            std::string eventname=m_pWebEm->FindValue("name");
			if (eventname=="")
				goto exitjson;
            
            std::string eventxml=m_pWebEm->FindValue("xml");
			if (eventxml=="")
				goto exitjson;

            std::string eventactive=m_pWebEm->FindValue("eventstatus");
            if (eventactive=="")
                goto exitjson;
            
            std::string eventid=m_pWebEm->FindValue("eventid");
            
            
            std::string eventlogic=m_pWebEm->FindValue("logicarray");
          	if (eventlogic=="")
				goto exitjson;

            int eventStatus = atoi(eventactive.c_str());
            
            Json::Value jsonRoot;
            Json::Reader reader;
            std::stringstream ssel(eventlogic);
            
            bool parsingSuccessful = reader.parse(ssel, jsonRoot);
            
            if (!parsingSuccessful)
            {
                
                _log.Log(LOG_ERROR,"Webserver eventparser: Invalid data received!");
            
            }
            else {

                szQuery.clear();
                szQuery.str("");
                
                if (eventid=="") {
                    szQuery << "INSERT INTO EventMaster (Name, XMLStatement, Status) VALUES ('" << eventname << "','" << eventxml << "','" << eventStatus <<  "')";
                    m_pMain->m_sql.query(szQuery.str());
                    szQuery.clear();
                    szQuery.str("");
                    szQuery << "SELECT ID FROM EventMaster WHERE (Name == '" << eventname << "')";
                    result=m_pMain->m_sql.query(szQuery.str());
                    if (result.size()>0)
                    {
                        std::vector<std::string> sd=result[0];
                        eventid = sd[0];
                    }
                }
                else {
                    szQuery << "UPDATE EventMaster SET Name='" << eventname << "', XMLStatement ='" << eventxml << "', Status ='" << eventStatus << "' WHERE (ID == '" << eventid << "')";
                    m_pMain->m_sql.query(szQuery.str());
                    szQuery.clear();
                    szQuery.str("");
                    szQuery << "DELETE FROM EventRules WHERE (EMID == '" << eventid << "')";
                    m_pMain->m_sql.query(szQuery.str());
                }
                
                if (eventid == "")
                {
                    //eventid should now never be empty!
                    _log.Log(LOG_ERROR,"Error writing event actions to database!");
                }
                else {
                    const Json::Value array = jsonRoot["eventlogic"];
                    for(int index=0; index<(int)array.size();++index) 
					{
                        std::string conditions = array[index].get("conditions","").asString();
                        std::string actions = array[index].get("actions","").asString();
                        
                        if (actions.find("SendNotification")!= std::string::npos) 
						{
                            actions = stdreplace(actions, "$", "#");
                        }
                        int sequenceNo = index+1;
                        szQuery.clear();
                        szQuery.str("");
                        szQuery << "INSERT INTO EventRules (EMID, Conditions, Actions, SequenceNo) VALUES ('" << eventid << "','" << conditions << "','" << actions << "','" << sequenceNo <<  "')";
                        m_pMain->m_sql.query(szQuery.str());
                    }
                    
                    m_pMain->m_eventsystem.LoadEvents();
                    root["status"]="OK";
                }
            }
        }
        else if (cparam=="delete")
		{
            root["title"]="DeleteEvent";
            std::string idx=m_pWebEm->FindValue("event");
            if (idx=="")
                goto exitjson;
            m_pMain->m_sql.DeleteEvent(idx);
            m_pMain->m_eventsystem.LoadEvents();
            root["status"]="OK";
        }
        else if (cparam=="currentstates")
		{
			std::vector<CEventSystem::_tDeviceStatus> devStates;
			m_pMain->m_eventsystem.WWWGetItemStates(devStates);
			if (devStates.size()==0)
				goto exitjson;
			
			int ii=0;
            std::vector<CEventSystem::_tDeviceStatus>::iterator itt;
            for(itt = devStates.begin(); itt != devStates.end(); ++itt) 
			{
                root["title"]="Current States";
                root["result"][ii]["id"]=itt->ID;
                root["result"][ii]["name"]=itt->deviceName;
                root["result"][ii]["value"]=itt->nValueWording;
                root["result"][ii]["svalues"]=itt->sValue;
                root["result"][ii]["lastupdate"]=itt->lastUpdate;
                ii++;
            }
            root["status"]="OK";
        }
        
	} //(rtype=="events")
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
				else if (Key=="MobileType")
				{
					root["MobileType"]=nValue;
				}
				else if (Key=="LightHistoryDays")
				{
					root["LightHistoryDays"]=nValue;
				}
				else if (Key=="5MinuteHistoryDays")
				{
					root["ShortLogDays"]=nValue;
				}
				else if (Key=="WebUserName")
				{
					root["WebUserName"]=base64_decode(sValue);
				}
				else if (Key=="WebPassword")
				{
					root["WebPassword"]=base64_decode(sValue);
				}
				else if (Key=="SecPassword")
				{
					root["SecPassword"]=base64_decode(sValue);
				}
				else if (Key=="WebLocalNetworks")
				{
					root["WebLocalNetworks"]=sValue;
				}
				else if (Key=="RandomTimerFrame")
				{
					root["RandomTimerFrame"]=nValue;
				}
				else if (Key=="MeterDividerEnergy")
				{
					root["EnergyDivider"]=nValue;
				}
				else if (Key=="MeterDividerGas")
				{
					root["GasDivider"]=nValue;
				}
				else if (Key=="MeterDividerWater")
				{
					root["WaterDivider"]=nValue;
				}
				else if (Key=="ElectricVoltage")
				{
					root["ElectricVoltage"]=nValue;
				}
				else if (Key=="CM113DisplayType")
				{
					root["CM113DisplayType"]=nValue;
				}
				else if (Key=="UseAutoUpdate")
				{
					root["UseAutoUpdate"]=nValue;
				}
				else if (Key=="Rego6XXType")
				{
					root["Rego6XXType"]=nValue;
				}
				else if (Key=="CostEnergy")
				{
					sprintf(szTmp,"%.4f",(float)(nValue)/10000.0f);
					root["CostEnergy"]=szTmp;
				}
				else if (Key=="CostEnergyT2")
				{
					sprintf(szTmp,"%.4f",(float)(nValue)/10000.0f);
					root["CostEnergyT2"]=szTmp;
				}
				else if (Key=="CostGas")
				{
					sprintf(szTmp,"%.4f",(float)(nValue)/10000.0f);
					root["CostGas"]=szTmp;
				}
				else if (Key=="CostWater")
				{
					sprintf(szTmp,"%.4f",(float)(nValue)/10000.0f);
					root["CostWater"]=szTmp;
				}
				else if (Key=="EmailFrom")
				{
					root["EmailFrom"]=sValue;
				}
				else if (Key=="EmailTo")
				{
					root["EmailTo"]=sValue;
				}
				else if (Key=="EmailServer")
				{
					root["EmailServer"]=sValue;
				}
				else if (Key=="EmailPort")
				{
					root["EmailPort"]=nValue;
				}
				else if (Key=="EmailUsername")
				{
					root["EmailUsername"]=base64_decode(sValue);
				}
				else if (Key=="EmailPassword")
				{
					root["EmailPassword"]=base64_decode(sValue);
				}
				else if (Key=="UseEmailInNotifications")
				{
					root["UseEmailInNotifications"]=nValue;
				}
				else if (Key=="EmailAsAttachment")
				{
					root["EmailAsAttachment"]=nValue;
				}
				else if (Key=="DoorbellCommand")
				{
					root["DoorbellCommand"]=nValue;
				}
				else if (Key=="SmartMeterType")
				{
					root["SmartMeterType"]=nValue;
				}
				else if (Key=="EnableTabLights")
				{
					root["EnableTabLights"]=nValue;
				}
				else if (Key=="EnableTabTemp")
				{
					root["EnableTabTemp"]=nValue;
				}
				else if (Key=="EnableTabWeather")
				{
					root["EnableTabWeather"]=nValue;
				}
				else if (Key=="EnableTabUtility")
				{
					root["EnableTabUtility"]=nValue;
				}
				else if (Key=="EnableTabScenes")
				{
					root["EnableTabScenes"]=nValue;
				}
				else if (Key=="NotificationSensorInterval")
				{
					root["NotificationSensorInterval"]=nValue;
				}
				else if (Key=="NotificationSwitchInterval")
				{
					root["NotificationSwitchInterval"]=nValue;
				}
				else if (Key=="RemoteSharedPort")
				{
					root["RemoteSharedPort"]=nValue;
				}
				else if (Key=="Language")
				{
					root["Language"]=sValue;
				}
				else if (Key=="WindUnit")
				{
					root["WindUnit"]=nValue;
				}
				else if (Key=="TempUnit")
				{
					root["TempUnit"]=nValue;
				}
				else if (Key=="AuthenticationMethod")
				{
					root["AuthenticationMethod"]=nValue;
				}
				else if (Key=="ReleaseChannel")
				{
					root["ReleaseChannel"]=nValue;
				}
				else if (Key=="RaspCamParams")
				{
					root["RaspCamParams"]=sValue;
				}
			}
		}
	} //(rtype=="settings")
exitjson:
	m_retstr=root.toStyledString();
	return m_retstr;
}


} //server
}//http

