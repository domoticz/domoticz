#include "stdafx.h"
#include "WebServer.h"
#include <boost/bind.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string/join.hpp>
#include <iostream>
#include <fstream>
#include "mainworker.h"
#include "Helper.h"
#include "localtime_r.h"
#include "EventSystem.h"
#include "../webserver/cWebem.h"
#include "../httpclient/HTTPClient.h"
#include "../hardware/hardwaretypes.h"
#include "../hardware/1Wire.h"
#include "../hardware/PhilipsHue.h"
#ifdef WITH_OPENZWAVE
#include "../hardware/OpenZWave.h"
#endif
#include "../hardware/EnOceanESP2.h"
#include "../hardware/EnOceanESP3.h"
#include "../hardware/Wunderground.h"
#include "../hardware/ForecastIO.h"
#include "../hardware/SBFSpot.h"
#ifdef WITH_GPIO
#include "../hardware/Gpio.h"
#include "../hardware/GpioPin.h"
#endif // WITH_GPIO
#include "../hardware/WOL.h"
#include "../hardware/evohome.h"
#include "../hardware/Pinger.h"
#include "../webserver/Base64.h"
#include "../smtpclient/SMTPClient.h"
#include "../json/config.h"
#include "../json/json.h"
#include "Logger.h"
#include "SQLHelper.h"
#include <algorithm>

#ifndef WIN32
#include <sys/utsname.h>
#include <dirent.h>
#else
#include "../msbuild/WindowsHelper.h"
#include "dirent_windows.h"
#endif
#include "../notifications/NotificationHelper.h"

#include "mainstructs.h"

#define round(a) ( int ) ( a + .5 )

extern std::string szUserDataFolder;
extern std::string szWWWFolder;

extern std::string szAppVersion;

struct _tGuiLanguage {
	const char* szShort;
	const char* szLong;
};

static const _tGuiLanguage guiLanguage[] =
{
	{ "en", "English" },
	{ "bg", "Bulgarian" },
	{ "cs", "Czech" },
	{ "nl", "Dutch" },
	{ "de", "German" },
	{ "el", "Greek" },
	{ "fr", "French" },
	{ "fi", "Finnish" },
	{ "hu", "Hungarian" },
	{ "it", "Italian" },
	{ "no", "Norwegian" },
	{ "pl", "Polish" },
	{ "pt", "Portuguese" },
	{ "ru", "Russian" },
	{ "sk", "Slovak" },
	{ "es", "Spanish" },
	{ "sv", "Swedish" },
	{ "tr", "Turkish" },

	{ NULL, NULL }
};

//#define DEBUG_DOWNLOAD

namespace http {
	namespace server {

		CWebServer::CWebServer(void)
		{
			m_pWebEm = NULL;
			m_bDoStop = false;
			m_LastUpdateCheck = 0;
#ifdef WITH_OPENZWAVE
			m_ZW_Hwidx = -1;
#endif
			m_bHaveUpdate=false;
			m_iRevision=0;
		}


		CWebServer::~CWebServer(void)
		{
			StopServer();
			if (m_pWebEm != NULL)
			{
				delete m_pWebEm;
				m_pWebEm = NULL;
			}
		}

		void CWebServer::Do_Work()
		{
			while (!m_bDoStop)
			{
				try
				{
					if (m_pWebEm)
						m_pWebEm->Run();
				}
				catch (std::exception& e)
				{
					if (!m_bDoStop)
					{
						_log.Log(LOG_ERROR, "WebServer stopped by exception, starting again..., %s",e.what());
						if (m_pWebEm)
							m_pWebEm->Stop();
						continue;
					}
				}
				catch (...)
				{
					if (!m_bDoStop)
					{
						_log.Log(LOG_ERROR, "WebServer stopped by exception, starting again...");
						if (m_pWebEm)
							m_pWebEm->Stop();
						continue;
					}
				}
				break;
			}

			_log.Log(LOG_STATUS, "WebServer stopped...");
		}

		void CWebServer::ReloadCustomSwitchIcons()
		{
			m_custom_light_icons.clear();
			m_custom_light_icons_lookup.clear();
			std::string sLine = "";

			//First get them from the switch_icons.txt file
			std::ifstream infile;
			std::string switchlightsfile = szWWWFolder + "/switch_icons.txt";
			infile.open(switchlightsfile.c_str());
			if (infile.is_open())
			{
				int index = 0;
				while (!infile.eof())
				{
					getline(infile, sLine);
					if (sLine.size() != 0)
					{
						std::vector<std::string> results;
						StringSplit(sLine, ";", results);
						if (results.size() == 3)
						{
							_tCustomIcon cImage;
							cImage.idx = index++;
							cImage.RootFile = results[0];
							cImage.Title = results[1];
							cImage.Description = results[2];
							m_custom_light_icons.push_back(cImage);
							m_custom_light_icons_lookup[cImage.idx] = m_custom_light_icons.size()-1;
						}
					}
				}
				infile.close();
			}
			//Now get them from the database (idx 100+)
			std::vector<std::vector<std::string> > result;
			result = m_sql.query("SELECT ID,Base,Name,Description FROM CustomImages");
			if (result.size() > 0)
			{
				std::vector<std::vector<std::string> >::const_iterator itt;
				int ii = 0;
				for (itt = result.begin(); itt != result.end(); ++itt)
				{
					std::vector<std::string> sd = *itt;

					int ID = atoi(sd[0].c_str());

					_tCustomIcon cImage;
					cImage.idx = 100+ID;
					cImage.RootFile = sd[1];
					cImage.Title = sd[2];
					cImage.Description = sd[3];

					std::string IconFile16 = cImage.RootFile + ".png";
					std::string IconFile48On = cImage.RootFile + "48_On.png";
					std::string IconFile48Off = cImage.RootFile + "48_Off.png";

					std::map<std::string, std::string> _dbImageFiles;
					_dbImageFiles["IconSmall"] = szWWWFolder + "/images/" + IconFile16;
					_dbImageFiles["IconOn"] = szWWWFolder + "/images/" + IconFile48On;
					_dbImageFiles["IconOff"] = szWWWFolder + "/images/" + IconFile48Off;

					std::map<std::string, std::string>::const_iterator iItt;

					//Check if files are on disk, else add them
					for (iItt = _dbImageFiles.begin(); iItt != _dbImageFiles.end(); ++iItt)
					{
						std::string TableField = iItt->first;
						std::string IconFile = iItt->second;

						if (!file_exist(IconFile.c_str()))
						{
							//Does not exists, extract it from the database and add it
							std::stringstream szQuery;
							szQuery << "SELECT " << TableField << " FROM CustomImages WHERE ID=" << ID;
							std::vector<std::vector<std::string> > result2;
							result2 = m_sql.queryBlob(szQuery.str());
							if (result2.size() > 0)
							{
								std::ofstream file;
								file.open(IconFile.c_str(), std::ios::out | std::ios::binary);
								if (!file.is_open())
									return;

								file << result2[0][0];
								file.close();
							}
						}
					}

					m_custom_light_icons.push_back(cImage);
					m_custom_light_icons_lookup[cImage.idx] = m_custom_light_icons.size() - 1;
					ii++;
				}
			}
		}

		bool CWebServer::StartServer(const std::string &listenaddress, const std::string &listenport, const std::string &serverpath, const bool bIgnoreUsernamePassword, const std::string &secure_cert_file, const std::string &secure_cert_passphrase)
		{
			StopServer();

			if (listenport.empty())
				return true;

			ReloadCustomSwitchIcons();

			if (m_pWebEm != NULL)
				delete m_pWebEm;
			try {
				m_pWebEm = new http::server::cWebem(
					listenaddress.c_str(),						// address
					listenport.c_str(),							// port
					serverpath.c_str(), secure_cert_file, secure_cert_passphrase);
			}
 			catch (std::exception& e) {
				_log.Log(LOG_ERROR, "Failed to start the web server: %s", e.what());
				if (atoi(listenport.c_str()) < 1024)
					_log.Log(LOG_ERROR, "check privileges for opening ports below 1024");
				else
					_log.Log(LOG_ERROR, "check if no other application is using port: %s", listenport.c_str());
				return false;
			}
			_log.Log(LOG_STATUS, "Webserver started on port: %s", listenport.c_str());

			m_pWebEm->SetDigistRealm("Domoticz.com");

			if (!bIgnoreUsernamePassword)
			{
				LoadUsers();
				std::string WebLocalNetworks;
				int nValue;
				if (m_sql.GetPreferencesVar("WebLocalNetworks", nValue, WebLocalNetworks))
				{
					std::vector<std::string> strarray;
					StringSplit(WebLocalNetworks, ";", strarray);
					std::vector<std::string>::const_iterator itt;
					for (itt = strarray.begin(); itt != strarray.end(); ++itt)
					{
						std::string network = *itt;
						m_pWebEm->AddLocalNetworks(network);
					}
					//add local hostname
					m_pWebEm->AddLocalNetworks("");
				}
			}

			//register callbacks
			m_pWebEm->RegisterIncludeCode("switchtypes",
				boost::bind(
				&CWebServer::DisplaySwitchTypesCombo,	// member function
				this));			// instance of class

			m_pWebEm->RegisterIncludeCode("metertypes",
				boost::bind(
				&CWebServer::DisplayMeterTypesCombo,
				this));

			m_pWebEm->RegisterIncludeCode("timertypes",
				boost::bind(
				&CWebServer::DisplayTimerTypesCombo,	// member function
				this));			// instance of class

			m_pWebEm->RegisterIncludeCode("combolanguage",
				boost::bind(
				&CWebServer::DisplayLanguageCombo,
				this));

			m_pWebEm->RegisterPageCode("/json.htm",
				boost::bind(
				&CWebServer::GetJSonPage,	// member function
				this));			// instance of class

			m_pWebEm->RegisterPageCode("/uploadcustomicon",
				boost::bind(
				&CWebServer::Post_UploadCustomIcon,
				this));

			m_pWebEm->RegisterPageCode("/html5.appcache",
				boost::bind(
				&CWebServer::GetAppCache,
				this));

			m_pWebEm->RegisterPageCode("/camsnapshot.jpg",
				boost::bind(
				&CWebServer::GetCameraSnapshot,
				this));
			m_pWebEm->RegisterPageCode("/backupdatabase.php",
				boost::bind(
				&CWebServer::GetDatabaseBackup,
				this));

			m_pWebEm->RegisterPageCode("/raspberry.cgi",
				boost::bind(
				&CWebServer::GetInternalCameraSnapshot,	// member function
				this));			// instance of class
			m_pWebEm->RegisterPageCode("/uvccapture.cgi",
				boost::bind(
				&CWebServer::GetInternalCameraSnapshot,	// member function
				this));			// instance of class
			//	m_pWebEm->RegisterPageCode( "/videostream.cgi",
			//	boost::bind(
			//&CWebServer::GetInternalCameraSnapshot,	// member function
			//this ) );			// instance of class

			m_pWebEm->RegisterActionCode("storesettings", boost::bind(&CWebServer::PostSettings, this));
			m_pWebEm->RegisterActionCode("setrfxcommode", boost::bind(&CWebServer::SetRFXCOMMode, this));
			m_pWebEm->RegisterActionCode("setrego6xxtype", boost::bind(&CWebServer::SetRego6XXType, this));
			m_pWebEm->RegisterActionCode("sets0metertype", boost::bind(&CWebServer::SetS0MeterType, this));
			m_pWebEm->RegisterActionCode("setlimitlesstype", boost::bind(&CWebServer::SetLimitlessType, this));
			m_pWebEm->RegisterActionCode("setopenthermsettings", boost::bind(&CWebServer::SetOpenThermSettings, this));
			m_pWebEm->RegisterActionCode("setp1usbtype", boost::bind(&CWebServer::SetP1USBType, this));
			m_pWebEm->RegisterActionCode("restoredatabase", boost::bind(&CWebServer::RestoreDatabase, this));
			m_pWebEm->RegisterActionCode("sbfspotimportolddata", boost::bind(&CWebServer::SBFSpotImportOldData, this));

			RegisterCommandCode("getlanguage", boost::bind(&CWebServer::Cmd_GetLanguage, this, _1), true);
			RegisterCommandCode("getthemes", boost::bind(&CWebServer::Cmd_GetThemes, this, _1), true);

			RegisterCommandCode("logincheck", boost::bind(&CWebServer::Cmd_LoginCheck, this, _1), true);
			RegisterCommandCode("getversion", boost::bind(&CWebServer::Cmd_GetVersion, this, _1), true);
			RegisterCommandCode("getlog", boost::bind(&CWebServer::Cmd_GetLog, this, _1));
			RegisterCommandCode("getauth", boost::bind(&CWebServer::Cmd_GetAuth, this, _1), true);

			
			RegisterCommandCode("gethardwaretypes", boost::bind(&CWebServer::Cmd_GetHardwareTypes, this, _1));
			RegisterCommandCode("addhardware", boost::bind(&CWebServer::Cmd_AddHardware, this, _1));
			RegisterCommandCode("updatehardware", boost::bind(&CWebServer::Cmd_UpdateHardware, this, _1));
			RegisterCommandCode("deletehardware", boost::bind(&CWebServer::Cmd_DeleteHardware, this, _1));

			RegisterCommandCode("wolgetnodes", boost::bind(&CWebServer::Cmd_WOLGetNodes, this, _1));
			RegisterCommandCode("woladdnode", boost::bind(&CWebServer::Cmd_WOLAddNode, this, _1));
			RegisterCommandCode("wolupdatenode", boost::bind(&CWebServer::Cmd_WOLUpdateNode, this, _1));
			RegisterCommandCode("wolremovenode", boost::bind(&CWebServer::Cmd_WOLRemoveNode, this, _1));
			RegisterCommandCode("wolclearnodes", boost::bind(&CWebServer::Cmd_WOLClearNodes, this, _1));

			RegisterCommandCode("pingersetmode", boost::bind(&CWebServer::Cmd_PingerSetMode, this, _1));
			RegisterCommandCode("pingergetnodes", boost::bind(&CWebServer::Cmd_PingerGetNodes, this, _1));
			RegisterCommandCode("pingeraddnode", boost::bind(&CWebServer::Cmd_PingerAddNode, this, _1));
			RegisterCommandCode("pingerupdatenode", boost::bind(&CWebServer::Cmd_PingerUpdateNode, this, _1));
			RegisterCommandCode("pingerremovenode", boost::bind(&CWebServer::Cmd_PingerRemoveNode, this, _1));
			RegisterCommandCode("pingerclearnodes", boost::bind(&CWebServer::Cmd_PingerClearNodes, this, _1));

			RegisterCommandCode("savefibarolinkconfig", boost::bind(&CWebServer::Cmd_SaveFibaroLinkConfig, this, _1));
			RegisterCommandCode("getfibarolinkconfig", boost::bind(&CWebServer::Cmd_GetFibaroLinkConfig, this, _1));
			RegisterCommandCode("getfibarolinks", boost::bind(&CWebServer::Cmd_GetFibaroLinks, this, _1));
			RegisterCommandCode("savefibarolink", boost::bind(&CWebServer::Cmd_SaveFibaroLink, this, _1));
			RegisterCommandCode("deletefibarolink", boost::bind(&CWebServer::Cmd_DeleteFibaroLink, this, _1));
			RegisterCommandCode("getdevicevalueoptions", boost::bind(&CWebServer::Cmd_GetDeviceValueOptions, this, _1));
			RegisterCommandCode("getdevicevalueoptionwording", boost::bind(&CWebServer::Cmd_GetDeviceValueOptionWording, this, _1));

			RegisterCommandCode("deleteuservariable", boost::bind(&CWebServer::Cmd_DeleteUserVariable, this, _1));
			RegisterCommandCode("saveuservariable", boost::bind(&CWebServer::Cmd_SaveUserVariable, this, _1));
			RegisterCommandCode("updateuservariable", boost::bind(&CWebServer::Cmd_UpdateUserVariable, this, _1));
			RegisterCommandCode("getuservariables", boost::bind(&CWebServer::Cmd_GetUserVariables, this, _1));
			RegisterCommandCode("getuservariable", boost::bind(&CWebServer::Cmd_GetUserVariable, this, _1));

			RegisterCommandCode("allownewhardware", boost::bind(&CWebServer::Cmd_AllowNewHardware, this, _1));

			RegisterCommandCode("addplan", boost::bind(&CWebServer::Cmd_AddPlan, this, _1));
			RegisterCommandCode("updateplan", boost::bind(&CWebServer::Cmd_UpdatePlan, this, _1));
			RegisterCommandCode("deleteplan", boost::bind(&CWebServer::Cmd_DeletePlan, this, _1));
			RegisterCommandCode("getunusedplandevices", boost::bind(&CWebServer::Cmd_GetUnusedPlanDevices, this, _1));
			RegisterCommandCode("addplanactivedevice", boost::bind(&CWebServer::Cmd_AddPlanActiveDevice, this, _1));
			RegisterCommandCode("getplandevices", boost::bind(&CWebServer::Cmd_GetPlanDevices, this, _1));
			RegisterCommandCode("deleteplandevice", boost::bind(&CWebServer::Cmd_DeletePlanDevice, this, _1));
			RegisterCommandCode("setplandevicecoords", boost::bind(&CWebServer::Cmd_SetPlanDeviceCoords, this, _1));
			RegisterCommandCode("deleteallplandevices", boost::bind(&CWebServer::Cmd_DeleteAllPlanDevices, this, _1));
			RegisterCommandCode("changeplanorder", boost::bind(&CWebServer::Cmd_ChangePlanOrder, this, _1));
			RegisterCommandCode("changeplandeviceorder", boost::bind(&CWebServer::Cmd_ChangePlanDeviceOrder, this, _1));
			RegisterCommandCode("getactualhistory", boost::bind(&CWebServer::Cmd_GetActualHistory, this, _1));
			RegisterCommandCode("getnewhistory", boost::bind(&CWebServer::Cmd_GetNewHistory, this, _1));
			RegisterCommandCode("getconfig", boost::bind(&CWebServer::Cmd_GetConfig, this, _1),true);
			RegisterCommandCode("sendnotification", boost::bind(&CWebServer::Cmd_SendNotification, this, _1));
			RegisterCommandCode("emailcamerasnapshot", boost::bind(&CWebServer::Cmd_EmailCameraSnapshot, this, _1));
			RegisterCommandCode("udevice", boost::bind(&CWebServer::Cmd_UpdateDevice, this, _1));
			RegisterCommandCode("thermostatstate", boost::bind(&CWebServer::Cmd_SetThermostatState, this, _1));
			RegisterCommandCode("system_shutdown", boost::bind(&CWebServer::Cmd_SystemShutdown, this, _1));
			RegisterCommandCode("system_reboot", boost::bind(&CWebServer::Cmd_SystemReboot, this, _1));
			RegisterCommandCode("execute_script", boost::bind(&CWebServer::Cmd_ExcecuteScript, this, _1));
			RegisterCommandCode("getcosts", boost::bind(&CWebServer::Cmd_GetCosts, this, _1));
			RegisterCommandCode("checkforupdate", boost::bind(&CWebServer::Cmd_CheckForUpdate, this, _1));
			RegisterCommandCode("downloadupdate", boost::bind(&CWebServer::Cmd_DownloadUpdate, this, _1));
			RegisterCommandCode("downloadready", boost::bind(&CWebServer::Cmd_DownloadReady, this, _1));
			RegisterCommandCode("deletedatapoint", boost::bind(&CWebServer::Cmd_DeleteDatePoint, this, _1));

			RegisterCommandCode("addtimer", boost::bind(&CWebServer::Cmd_AddTimer, this, _1));
			RegisterCommandCode("updatetimer", boost::bind(&CWebServer::Cmd_UpdateTimer, this, _1));
			RegisterCommandCode("deletetimer", boost::bind(&CWebServer::Cmd_DeleteTimer, this, _1));
			RegisterCommandCode("cleartimers", boost::bind(&CWebServer::Cmd_ClearTimers, this, _1));

			RegisterCommandCode("addscenetimer", boost::bind(&CWebServer::Cmd_AddSceneTimer, this, _1));
			RegisterCommandCode("updatescenetimer", boost::bind(&CWebServer::Cmd_UpdateSceneTimer, this, _1));
			RegisterCommandCode("deletescenetimer", boost::bind(&CWebServer::Cmd_DeleteSceneTimer, this, _1));
			RegisterCommandCode("clearscenetimers", boost::bind(&CWebServer::Cmd_ClearSceneTimers, this, _1));
			RegisterCommandCode("setscenecode", boost::bind(&CWebServer::Cmd_SetSceneCode, this, _1));
			RegisterCommandCode("removescenecode", boost::bind(&CWebServer::Cmd_RemoveSceneCode, this, _1));

			RegisterCommandCode("setsetpoint", boost::bind(&CWebServer::Cmd_SetSetpoint, this, _1));
			RegisterCommandCode("addsetpointtimer", boost::bind(&CWebServer::Cmd_AddSetpointTimer, this, _1));
			RegisterCommandCode("updatesetpointtimer", boost::bind(&CWebServer::Cmd_UpdateSetpointTimer, this, _1));
			RegisterCommandCode("deletesetpointtimer", boost::bind(&CWebServer::Cmd_DeleteSetpointTimer, this, _1));
			RegisterCommandCode("clearsetpointtimers", boost::bind(&CWebServer::Cmd_ClearSetpointTimers, this, _1));

			RegisterCommandCode("serial_devices", boost::bind(&CWebServer::Cmd_GetSerialDevices, this, _1));
			RegisterCommandCode("devices_list", boost::bind(&CWebServer::Cmd_GetDevicesList, this, _1));
			RegisterCommandCode("devices_list_onoff", boost::bind(&CWebServer::Cmd_GetDevicesListOnOff, this, _1));

			RegisterCommandCode("registerhue", boost::bind(&CWebServer::Cmd_RegisterWithPhilipsHue, this, _1));

			RegisterCommandCode("getcustomiconset", boost::bind(&CWebServer::Cmd_GetCustomIconSet, this, _1));
			RegisterCommandCode("deletecustomicon", boost::bind(&CWebServer::Cmd_DeleteCustomIcon, this, _1));
			RegisterCommandCode("renamedevice", boost::bind(&CWebServer::Cmd_RenameDevice, this, _1));


			RegisterRType("graph", boost::bind(&CWebServer::RType_HandleGraph, this, _1));
			RegisterRType("lightlog", boost::bind(&CWebServer::RType_LightLog, this, _1));
			RegisterRType("textlog", boost::bind(&CWebServer::RType_TextLog, this, _1));
			RegisterRType("settings", boost::bind(&CWebServer::RType_Settings, this, _1));
			RegisterRType("events", boost::bind(&CWebServer::RType_Events, this, _1));
			RegisterRType("hardware", boost::bind(&CWebServer::RType_Hardware, this, _1));
			RegisterRType("devices", boost::bind(&CWebServer::RType_Devices, this, _1));
			RegisterRType("deletedevice", boost::bind(&CWebServer::RType_DeleteDevice, this, _1));
			RegisterRType("cameras", boost::bind(&CWebServer::RType_Cameras, this, _1));
			RegisterRType("users", boost::bind(&CWebServer::RType_Users, this, _1));

			RegisterRType("timers", boost::bind(&CWebServer::RType_Timers, this, _1));
			RegisterRType("scenetimers", boost::bind(&CWebServer::RType_SceneTimers, this, _1));
			RegisterRType("setpointtimers", boost::bind(&CWebServer::RType_SetpointTimers, this, _1));

			RegisterRType("gettransfers", boost::bind(&CWebServer::RType_GetTransfers, this, _1));
			RegisterRType("transferdevice", boost::bind(&CWebServer::RType_TransferDevice, this, _1));
			RegisterRType("notifications", boost::bind(&CWebServer::RType_Notifications, this, _1));
			RegisterRType("schedules", boost::bind(&CWebServer::RType_Schedules, this, _1));
			RegisterRType("getshareduserdevices", boost::bind(&CWebServer::RType_GetSharedUserDevices, this, _1));
			RegisterRType("setshareduserdevices", boost::bind(&CWebServer::RType_SetSharedUserDevices, this, _1));
			RegisterRType("setused", boost::bind(&CWebServer::RType_SetUsed, this, _1));
			RegisterRType("scenes", boost::bind(&CWebServer::RType_Scenes, this, _1));
			RegisterRType("addscene", boost::bind(&CWebServer::RType_AddScene, this, _1));
			RegisterRType("deletescene", boost::bind(&CWebServer::RType_DeleteScene, this, _1));
			RegisterRType("updatescene", boost::bind(&CWebServer::RType_UpdateScene, this, _1));
			RegisterRType("createvirtualsensor", boost::bind(&CWebServer::RType_CreateVirtualSensor, this, _1));
			RegisterRType("createevohomesensor", boost::bind(&CWebServer::RType_CreateEvohomeSensor, this, _1));
			RegisterRType("bindevohome", boost::bind(&CWebServer::RType_BindEvohome, this, _1));
			RegisterRType("custom_light_icons", boost::bind(&CWebServer::RType_CustomLightIcons, this, _1));
			RegisterRType("plans", boost::bind(&CWebServer::RType_Plans, this, _1));
			RegisterRType("floorplans", boost::bind(&CWebServer::RType_FloorPlans, this, _1));
#ifdef WITH_OPENZWAVE
			//ZWave
			RegisterCommandCode("updatezwavenode", boost::bind(&CWebServer::Cmd_ZWaveUpdateNode, this, _1));
			RegisterCommandCode("deletezwavenode", boost::bind(&CWebServer::Cmd_ZWaveDeleteNode, this, _1));
			RegisterCommandCode("zwaveinclude", boost::bind(&CWebServer::Cmd_ZWaveInclude, this, _1));
			RegisterCommandCode("zwaveexclude", boost::bind(&CWebServer::Cmd_ZWaveExclude, this, _1));
			RegisterCommandCode("zwavesoftreset", boost::bind(&CWebServer::Cmd_ZWaveSoftReset, this, _1));
			RegisterCommandCode("zwavehardreset", boost::bind(&CWebServer::Cmd_ZWaveHardReset, this, _1));
			RegisterCommandCode("zwavenetworkheal", boost::bind(&CWebServer::Cmd_ZWaveNetworkHeal, this, _1));
			RegisterCommandCode("zwavenodeheal", boost::bind(&CWebServer::Cmd_ZWaveNodeHeal, this, _1));
			RegisterCommandCode("zwavenetworkinfo", boost::bind(&CWebServer::Cmd_ZWaveNetworkInfo, this, _1));
			RegisterCommandCode("zwaveremovegroupnode", boost::bind(&CWebServer::Cmd_ZWaveRemoveGroupNode, this, _1));
			RegisterCommandCode("zwaveaddgroupnode", boost::bind(&CWebServer::Cmd_ZWaveAddGroupNode, this, _1));
			RegisterCommandCode("zwavegroupinfo", boost::bind(&CWebServer::Cmd_ZWaveGroupInfo, this, _1));
			RegisterCommandCode("zwavecancel", boost::bind(&CWebServer::Cmd_ZWaveCancel, this, _1));
			RegisterCommandCode("applyzwavenodeconfig", boost::bind(&CWebServer::Cmd_ApplyZWaveNodeConfig, this, _1));
			RegisterCommandCode("requestzwavenodeconfig", boost::bind(&CWebServer::Cmd_ZWaveRequestNodeConfig, this, _1));
			RegisterCommandCode("zwavestatecheck", boost::bind(&CWebServer::Cmd_ZWaveStateCheck, this, _1));
			RegisterCommandCode("zwavereceiveconfigurationfromothercontroller", boost::bind(&CWebServer::Cmd_ZWaveReceiveConfigurationFromOtherController, this, _1));
			RegisterCommandCode("zwavesendconfigurationtosecondcontroller", boost::bind(&CWebServer::Cmd_ZWaveSendConfigurationToSecondaryController, this, _1));
			RegisterCommandCode("zwavetransferprimaryrole", boost::bind(&CWebServer::Cmd_ZWaveTransferPrimaryRole, this, _1));
			RegisterCommandCode("zwavestartusercodeenrollmentmode", boost::bind(&CWebServer::Cmd_ZWaveSetUserCodeEnrollmentMode, this, _1));
			RegisterCommandCode("zwavegetusercodes", boost::bind(&CWebServer::Cmd_ZWaveGetNodeUserCodes, this, _1));
			RegisterCommandCode("zwaveremoveusercode", boost::bind(&CWebServer::Cmd_ZWaveRemoveUserCode, this, _1));

			m_pWebEm->RegisterPageCode("/zwavegetconfig.php", boost::bind(&CWebServer::ZWaveGetConfigFile,	this));

			m_pWebEm->RegisterPageCode("/ozwcp/poll.xml", boost::bind(&CWebServer::ZWaveCPPollXml, this));
			m_pWebEm->RegisterPageCode("/ozwcp/cp.html", boost::bind(&CWebServer::ZWaveCPIndex, this));
			m_pWebEm->RegisterPageCode("/ozwcp/confparmpost.html", boost::bind(&CWebServer::ZWaveCPNodeGetConf, this));
			m_pWebEm->RegisterPageCode("/ozwcp/refreshpost.html", boost::bind(&CWebServer::ZWaveCPNodeGetValues, this));
			m_pWebEm->RegisterPageCode("/ozwcp/valuepost.html", boost::bind(&CWebServer::ZWaveCPNodeSetValue, this));
			m_pWebEm->RegisterPageCode("/ozwcp/buttonpost.html", boost::bind(&CWebServer::ZWaveCPNodeSetButton, this));
			m_pWebEm->RegisterPageCode("/ozwcp/admpost.html", boost::bind(&CWebServer::ZWaveCPAdminCommand, this));
			m_pWebEm->RegisterPageCode("/ozwcp/nodepost.html", boost::bind(&CWebServer::ZWaveCPNodeChange, this));
			m_pWebEm->RegisterPageCode("/ozwcp/savepost.html", boost::bind(&CWebServer::ZWaveCPSaveConfig, this));
			m_pWebEm->RegisterPageCode("/ozwcp/topopost.html", boost::bind(&CWebServer::ZWaveCPGetTopo, this));
			m_pWebEm->RegisterPageCode("/ozwcp/statpost.html", boost::bind(&CWebServer::ZWaveCPGetStats, this));
			//grouppost.html
			//pollpost.html
			//scenepost.html
			//thpost.html
			RegisterRType("openzwavenodes", boost::bind(&CWebServer::RType_OpenZWaveNodes, this, _1));
#endif	

			m_pWebEm->RegisterWhitelistURLString("/html5.appcache");

			//Start normal worker thread
			m_bDoStop = false;
			m_thread = boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&CWebServer::Do_Work, this)));

			//Check for update (force)
			m_LastUpdateCheck = 0;
			Json::Value root;
			Cmd_CheckForUpdate(root);
			return (m_thread != NULL);
		}

		void CWebServer::StopServer()
		{
			m_bDoStop = true;
			try
			{
				if (m_pWebEm == NULL)
					return;
				m_pWebEm->Stop();
			}
			catch (...)
			{
				
			}
		}

		void CWebServer::SetAuthenticationMethod(int amethod)
		{
			if (m_pWebEm == NULL)
				return;
			m_pWebEm->SetAuthenticationMethod((_eAuthenticationMethod)amethod);
		}
		void CWebServer::SetWebTheme(const std::string &themename)
		{
			if (m_pWebEm == NULL)
				return;
			m_pWebEm->SetWebTheme(themename);
		}

		void CWebServer::RegisterCommandCode(const char* idname, webserver_response_function ResponseFunction, bool bypassAuthentication)
		{
			m_webcommands.insert(std::pair<std::string, webserver_response_function >(std::string(idname), ResponseFunction));
			if (bypassAuthentication)
			{
				m_pWebEm->RegisterWhitelistURLString(idname);
			}
		}

		void CWebServer::RegisterRType(const char* idname, webserver_response_function ResponseFunction)
		{
			m_webrtypes.insert(std::pair<std::string, webserver_response_function >(std::string(idname), ResponseFunction));
		}

		void CWebServer::HandleRType(const std::string &rtype, Json::Value &root)
		{
			std::map < std::string, webserver_response_function >::iterator pf = m_webrtypes.find(rtype);
			if (pf != m_webrtypes.end())
			{
				pf->second(root);
			}
		}

		int GetDirFilesRecursive(const std::string &DirPath, std::map<std::string,int> &_Files)
		{
			DIR* dir;
			struct dirent *ent;
			if ((dir = opendir(DirPath.c_str())) != NULL)
			{
				while ((ent = readdir(dir)) != NULL)
				{
					if (ent->d_type == DT_DIR)
					{
						if ((strcmp(ent->d_name, ".") != 0) && (strcmp(ent->d_name, "..") != 0) && (strcmp(ent->d_name, ".svn") != 0))
						{
							std::string nextdir = DirPath + ent->d_name + "/";
							if (GetDirFilesRecursive(nextdir.c_str(), _Files))
							{
								closedir(dir);
								return 1;
							}
						}
					}
					else
					{
						std::string fname = DirPath + CURLEncode::URLEncode(ent->d_name);
						_Files[fname]=1;
					}
				}
			}
			closedir(dir);
			return 0;
		}

		std::string CWebServer::GetAppCache()
		{
			//Return the appcache file (dynamicly generated)
			std::string response="";
			std::string sLine;
			std::string filename = szWWWFolder + "/html5.appcache";


			std::string sWebTheme="default";
			m_sql.GetPreferencesVar("WebTheme", sWebTheme);

			//Get Dynamic Theme Files
			std::map<std::string,int> _ThemeFiles;
			GetDirFilesRecursive(szWWWFolder + "/styles/" + sWebTheme + "/", _ThemeFiles);

			//Get Dynamic Floorplan Files
			std::map<std::string,int> _FloorplanFiles;
			GetDirFilesRecursive(szWWWFolder + "/images/floorplans/", _FloorplanFiles);

			std::ifstream is(filename.c_str());
			if (is)
			{
				while (!is.eof())
				{
					getline(is, sLine);
					if (sLine != "")
					{
						if (sLine.find("#ThemeFiles") != std::string::npos)
						{
							response += "#Theme=" + sWebTheme + "\n";
							//Add all theme files
							std::map<std::string,int>::const_iterator itt;
							for (itt = _ThemeFiles.begin(); itt != _ThemeFiles.end(); ++itt)
							{
								std::string tfname = (itt->first).substr(szWWWFolder.size() + 1);
								tfname=stdreplace(tfname, "styles/" + sWebTheme, "acttheme");
								response += tfname + "\n";
							}
							continue;
						}
						else if (sLine.find("#Floorplans") != std::string::npos)
						{
							//Add all floorplans
							std::map<std::string,int>::const_iterator itt;
							for (itt = _FloorplanFiles.begin(); itt != _FloorplanFiles.end(); ++itt)
							{
								std::string tfname = (itt->first).substr(szWWWFolder.size() + 1);
								response += tfname + "\n";
							}
							continue;
						}
						else if (sLine.find("#SwitchIcons") != std::string::npos)
						{
							//Add database switch icons
							std::vector<_tCustomIcon>::const_iterator itt;
							for (itt = m_custom_light_icons.begin(); itt != m_custom_light_icons.end(); ++itt)
							{
								if (itt->idx >= 100)
								{
									std::string IconFile16 = itt->RootFile + ".png";
									std::string IconFile48On = itt->RootFile + "48_On.png";
									std::string IconFile48Off = itt->RootFile + "48_Off.png";

									response += "images/" + CURLEncode::URLEncode(IconFile16) + "\n";
									response += "images/" + CURLEncode::URLEncode(IconFile48On) + "\n";
									response += "images/" + CURLEncode::URLEncode(IconFile48Off) + "\n";
								}
							}
						}
					}
					response += sLine + "\n";
				}
			}
			return response;
		}

		std::string CWebServer::GetJSonPage()
		{
			Json::Value root;
			root["status"] = "ERR";

			std::string rtype = m_pWebEm->FindValue("type");
			if (rtype == "command")
			{
				std::string cparam = m_pWebEm->FindValue("param");
				if (cparam == "")
				{
					cparam = m_pWebEm->FindValue("dparam");
					if (cparam == "")
					{
						goto exitjson;
					}
				}
				if (cparam == "logout")
				{
					root["status"] = "OK";
					root["title"] = "Logout";
					m_retstr = "authorize";
					return m_retstr;

				}
				HandleCommand(cparam, root);
			} //(rtype=="command")
			else
				HandleRType(rtype, root);
		exitjson:
			std::string jcallback = m_pWebEm->FindValue("jsoncallback");
			if (jcallback.size() == 0)
				m_retstr = root.toStyledString();
			else
			{
				m_retstr = "var data=" + root.toStyledString() + "\n" + jcallback + "(data);";
			}
			return m_retstr;
		}

		void CWebServer::Cmd_GetLanguage(Json::Value &root)
		{
			std::string sValue;
			if (m_sql.GetPreferencesVar("Language", sValue))
			{
				root["status"] = "OK";
				root["title"] = "GetLanguage";
				root["language"] = sValue;
			}
		}

		void CWebServer::Cmd_GetThemes(Json::Value &root)
		{
			root["status"] = "OK";
			root["title"] = "GetThemes";
			m_mainworker.GetAvailableWebThemes();
			std::vector<std::string>::const_iterator itt;
			int ii = 0;
			for (itt = m_mainworker.m_webthemes.begin(); itt != m_mainworker.m_webthemes.end(); ++itt)
			{
				root["result"][ii]["theme"] = *itt;
				ii++;
			}
		}

		void CWebServer::Cmd_LoginCheck(Json::Value &root)
		{
			std::string tmpusrname = m_pWebEm->FindValue("username");
			std::string tmpusrpass = m_pWebEm->FindValue("password");
			if (
				(tmpusrname == "") ||
				(tmpusrpass == "")
				)
				return;

			std::string rememberme = m_pWebEm->FindValue("rememberme");

			std::string usrname;
			std::string usrpass;
			if (request_handler::url_decode(tmpusrname, usrname))
			{
				if (request_handler::url_decode(tmpusrpass, usrpass))
				{
					usrname = base64_decode(usrname);
					int iUser = -1;
					iUser = FindUser(usrname.c_str());
					if (iUser == -1)
						return;
					if (m_users[iUser].Password != usrpass)
						return;
					root["status"] = "OK";
					root["title"] = "logincheck";
					m_pWebEm->m_actualuser = m_users[iUser].Username;
					m_pWebEm->m_actualuser_rights = m_users[iUser].userrights;
					m_pWebEm->m_bAddNewSession = true;
					m_pWebEm->m_bRemembermeUser = (rememberme == "true");
					root["user"] = m_pWebEm->m_actualuser;
					root["rights"] = m_pWebEm->m_actualuser_rights;
				}
			}
		}

		void CWebServer::Cmd_GetHardwareTypes(Json::Value &root)
		{
			root["status"] = "OK";
			root["title"] = "GetHardwareTypes";
			std::map<std::string, int> _htypes;
			for (int ii = 0; ii < HTYPE_END; ii++)
			{
				bool bDoAdd = true;
#ifndef _DEBUG
#ifdef WIN32
				if (
					(ii == HTYPE_RaspberryBMP085)
					)
				{
					bDoAdd = false;
				}
				else
				{
#ifndef WITH_LIBUSB
					if (
						(ii == HTYPE_VOLCRAFTCO20) ||
						(ii == HTYPE_TE923)
						)
					{
						bDoAdd = false;
					}
#endif

				}
#endif
#endif
#ifndef WITH_OPENZWAVE
				if (ii == HTYPE_OpenZWave)
					bDoAdd = false;
#endif
#ifndef WITH_GPIO
				if (
					(ii == HTYPE_RaspberryGPIO)
					)
				{
					bDoAdd = false;
				}
#endif
				if ((ii == HTYPE_1WIRE) && (!C1Wire::Have1WireSystem()))
					bDoAdd = false;

				if (bDoAdd)
					_htypes[Hardware_Type_Desc(ii)] = ii;
			}
			//return a sorted hardware list
			std::map<std::string, int>::const_iterator itt;
			int ii = 0;
			for (itt = _htypes.begin(); itt != _htypes.end(); ++itt)
			{
				root["result"][ii]["idx"] = itt->second;
				root["result"][ii]["name"] = itt->first;
				ii++;
			}
		}

		void CWebServer::Cmd_AddHardware(Json::Value &root)
		{
			if (m_pWebEm->m_actualuser_rights != 2)
			{
				//No admin user, and not allowed to be here
				return;
			}

			std::string name = CURLEncode::URLDecode(m_pWebEm->FindValue("name"));
			std::string senabled = m_pWebEm->FindValue("enabled");
			std::string shtype = m_pWebEm->FindValue("htype");
			std::string address = m_pWebEm->FindValue("address");
			std::string sport = m_pWebEm->FindValue("port");
			std::string username = CURLEncode::URLDecode(m_pWebEm->FindValue("username"));
			std::string password = CURLEncode::URLDecode(m_pWebEm->FindValue("password"));
			std::string sdatatimeout = m_pWebEm->FindValue("datatimeout");

			if (
				(name == "") ||
				(senabled == "") ||
				(shtype == "") ||
				(sport == "")
				)
				return;
			_eHardwareTypes htype = (_eHardwareTypes)atoi(shtype.c_str());

			int iDataTimeout = atoi(sdatatimeout.c_str());
			int mode1 = 0;
			int mode2 = 0;
			int mode3 = 0;
			int mode4 = 0;
			int mode5 = 0;
			int mode6 = 0;
			int port = atoi(sport.c_str());
			if (IsSerialDevice(htype))
			{
			}
			else if (
				(htype == HTYPE_RFXLAN) || (htype == HTYPE_P1SmartMeterLAN) || (htype == HTYPE_YouLess) || (htype == HTYPE_RazberryZWave) || (htype == HTYPE_OpenThermGatewayTCP) || (htype == HTYPE_LimitlessLights) ||
				(htype == HTYPE_SolarEdgeTCP) || (htype == HTYPE_WOL) || (htype == HTYPE_ECODEVICES) || (htype == HTYPE_Mochad) || (htype == HTYPE_MySensorsTCP) || (htype == HTYPE_MQTT) || (htype == HTYPE_FRITZBOX) ||
				(htype == HTYPE_ETH8020) || (htype == HTYPE_KMTronicTCP) || (htype == HTYPE_SOLARMAXTCP)
				) {
				//Lan
				if (address == "")
					return;
			}
			else if (htype == HTYPE_Domoticz) {
				//Remote Domoticz
				if (address == "")
					return;
			}
			else if (htype == HTYPE_TE923) {
				//all fine here!
			}
			else if (htype == HTYPE_VOLCRAFTCO20) {
				//all fine here!
			}
			else if (htype == HTYPE_System)	{
				//All fine here
			}
			else if (htype == HTYPE_1WIRE) {
				//all fine here!
			}
			else if (htype == HTYPE_Pinger) {
				//all fine here!
			}
			else if (htype == HTYPE_RaspberryBMP085) {
				//all fine here!
			}
			else if (htype == HTYPE_Dummy) {
				//all fine here!
			}
			else if (htype == HTYPE_EVOHOME_SCRIPT || htype == HTYPE_EVOHOME_SERIAL) {
				//all fine here!
			}
			else if (htype == HTYPE_PiFace) {
				//all fine here!
			}
			else if ((htype == HTYPE_Wunderground) || (htype == HTYPE_ForecastIO) || (htype == HTYPE_ICYTHERMOSTAT) || (htype == HTYPE_TOONTHERMOSTAT) || (htype == HTYPE_PVOUTPUT_INPUT) || (htype == HTYPE_NESTTHERMOSTAT)) {
				if (
					(username == "") ||
					(password == "")
					)
					return;
			}
			else if (htype == HTYPE_SBFSpot) {
				if (username == "")
					return;
			}
			else if (htype == HTYPE_HARMONY_HUB) {
				if (
					(username == "") ||
					(password == "") ||
					(address == "")
					)
					return;
			}
			else if (htype == HTYPE_Philips_Hue) {
				if (
					(username == "") ||
					(address == "")
					)
					return;
				if (port == 0)
					port = 80;
			}
			else if (htype == HTYPE_RaspberryGPIO) {
				//all fine here!
			}
			else
				return;

			root["status"] = "OK";
			root["title"] = "AddHardware";

			std::vector<std::vector<std::string> > result;
			char szTmp[300];

			if (htype == HTYPE_Domoticz)
			{
				if (password.size() != 32)
				{
					password = GenerateMD5Hash(password);
				}
			}
			else if (htype == HTYPE_S0SmartMeter)
			{
				address = "0;1000;0;1000;0;1000;0;1000;0;1000";
			}
			else if (htype == HTYPE_Pinger)
			{
				mode1 = 30;
				mode2 = 1000;
			}

			sprintf(szTmp,
				"INSERT INTO Hardware (Name, Enabled, Type, Address, Port, SerialPort, Username, Password, Mode1, Mode2, Mode3, Mode4, Mode5, Mode6, DataTimeout) VALUES ('%s',%d, %d,'%s',%d,'%s','%s','%s',%d,%d,%d,%d,%d,%d,%d)",
				name.c_str(),
				(senabled == "true") ? 1 : 0,
				htype,
				address.c_str(),
				port,
				sport.c_str(),
				username.c_str(),
				password.c_str(),
				mode1, mode2, mode3, mode4, mode5, mode6,
				iDataTimeout
				);
			result = m_sql.query(szTmp);

			//add the device for real in our system
			strcpy(szTmp, "SELECT MAX(ID) FROM Hardware");
			result = m_sql.query(szTmp); //-V519
			if (result.size() > 0)
			{
				std::vector<std::string> sd = result[0];
				int ID = atoi(sd[0].c_str());

				m_mainworker.AddHardwareFromParams(ID, name, (senabled == "true") ? true : false, htype, address, port, sport, username, password, mode1, mode2, mode3, mode4, mode5, mode6, iDataTimeout);
			}
		}

		void CWebServer::Cmd_UpdateHardware(Json::Value &root)
		{
			if (m_pWebEm->m_actualuser_rights != 2)
			{
				//No admin user, and not allowed to be here
				return;
			}

			std::string idx = m_pWebEm->FindValue("idx");
			if (idx == "")
				return;
			std::string name = CURLEncode::URLDecode(m_pWebEm->FindValue("name"));
			std::string senabled = m_pWebEm->FindValue("enabled");
			std::string shtype = m_pWebEm->FindValue("htype");
			std::string address = m_pWebEm->FindValue("address");
			std::string sport = m_pWebEm->FindValue("port");
			std::string username = CURLEncode::URLDecode(m_pWebEm->FindValue("username"));
			std::string password = CURLEncode::URLDecode(m_pWebEm->FindValue("password"));
			std::string sdatatimeout = m_pWebEm->FindValue("datatimeout");

			if (
				(name == "") ||
				(senabled == "") ||
				(shtype == "")
				)
				return;

			bool bEnabled = (senabled == "true") ? true : false;

			_eHardwareTypes htype = (_eHardwareTypes)atoi(shtype.c_str());
			int iDataTimeout = atoi(sdatatimeout.c_str());

			int port = atoi(sport.c_str());

			bool bIsSerial = false;

			if (IsSerialDevice(htype))
			{
				//USB/System
				bIsSerial = true;
				if (bEnabled)
				{
					if (sport.empty())
						return; //need to have a serial port
				}
			}
			else if (
				(htype == HTYPE_RFXLAN) || (htype == HTYPE_P1SmartMeterLAN) || 
				(htype == HTYPE_YouLess) || (htype == HTYPE_RazberryZWave) || (htype == HTYPE_OpenThermGatewayTCP) || (htype == HTYPE_LimitlessLights) || 
				(htype == HTYPE_SolarEdgeTCP) || (htype == HTYPE_WOL) || (htype == HTYPE_ECODEVICES) || (htype == HTYPE_Mochad) || 
				(htype == HTYPE_MySensorsTCP) || (htype == HTYPE_MQTT) || (htype == HTYPE_FRITZBOX) || (htype == HTYPE_ETH8020) || 
				(htype == HTYPE_KMTronicTCP) || (htype == HTYPE_SOLARMAXTCP)
				){
				//Lan
				if (address == "")
					return;
			}
			else if (htype == HTYPE_Domoticz) {
				//Remote Domoticz
				if (address == "")
					return;
			}
			else if (htype == HTYPE_System) {
				//All fine here
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
			else if (htype == HTYPE_Pinger) {
				//All fine here
			}
			else if (htype == HTYPE_RaspberryBMP085) {
				//All fine here
			}
			else if (htype == HTYPE_Dummy) {
				//All fine here
			}
			else if (htype == HTYPE_EVOHOME_SCRIPT || htype == HTYPE_EVOHOME_SERIAL) {
				//All fine here
			}
			else if (htype == HTYPE_PiFace) {
				//All fine here
			}
			else if ((htype == HTYPE_Wunderground) || (htype == HTYPE_ForecastIO) || (htype == HTYPE_ICYTHERMOSTAT) || (htype == HTYPE_TOONTHERMOSTAT) || (htype == HTYPE_PVOUTPUT_INPUT) || (htype == HTYPE_NESTTHERMOSTAT)) {
				if (
					(username == "") ||
					(password == "")
					)
					return;
			}
			else if (htype == HTYPE_HARMONY_HUB) {
				if (
					(username == "") ||
					(password == "") ||
					(address == "")
					)
					return;
			}
			else if (htype == HTYPE_Philips_Hue) {
				if (
					(username == "") ||
					(address == "")
					)
					return;
				if (port == 0)
					port = 80;
			}
			else if (htype == HTYPE_RaspberryGPIO) {
				//all fine here!
			}
			else if (htype == HTYPE_SBFSpot) {
				if (username == "")
					return;
			}
			else
				return;

			int mode1 = atoi(m_pWebEm->FindValue("Mode1").c_str());
			int mode2 = atoi(m_pWebEm->FindValue("Mode2").c_str());
			int mode3 = atoi(m_pWebEm->FindValue("Mode3").c_str());
			int mode4 = atoi(m_pWebEm->FindValue("Mode4").c_str());
			int mode5 = atoi(m_pWebEm->FindValue("Mode5").c_str());
			int mode6 = atoi(m_pWebEm->FindValue("Mode6").c_str());
			root["status"] = "OK";
			root["title"] = "UpdateHardware";

			std::vector<std::vector<std::string> > result;
			char szTmp[300];

			if (htype == HTYPE_Domoticz)
			{
				if (password.size() != 32)
				{
					password = GenerateMD5Hash(password);
				}
			}

			if ((bIsSerial) && (!bEnabled) && (sport.empty()))
			{
				//just disable the device
				sprintf(szTmp,
					"UPDATE Hardware SET Enabled=%d WHERE (ID == %s)",
					(bEnabled == true) ? 1 : 0,
					idx.c_str()
					);
			}
			else
			{
				sprintf(szTmp,
					"UPDATE Hardware SET Name='%s', Enabled=%d, Type=%d, Address='%s', Port=%d, SerialPort='%s', Username='%s', Password='%s', Mode1=%d, Mode2=%d, Mode3=%d, Mode4=%d, Mode5=%d, Mode6=%d, DataTimeout=%d WHERE (ID == %s)",
					name.c_str(),
					(bEnabled == true) ? 1 : 0,
					htype,
					address.c_str(),
					port,
					sport.c_str(),
					username.c_str(),
					password.c_str(),
					mode1, mode2, mode3, mode4, mode5, mode6,
					iDataTimeout,
					idx.c_str()
					);
			}
			result = m_sql.query(szTmp);

			//Special case for internal system monitoring
			if (htype == HTYPE_System)
			{
				m_mainworker.m_hardwaremonitor.StopHardwareMonitor();
				if (bEnabled)
				{
					m_mainworker.m_hardwaremonitor.StartHardwareMonitor();
				}
			}
			else
			{
				//re-add the device in our system
				int ID = atoi(idx.c_str());
				m_mainworker.AddHardwareFromParams(ID, name, bEnabled, htype, address, port, sport, username, password, mode1, mode2, mode3, mode4, mode5, mode6, iDataTimeout);
			}
		}

		void CWebServer::Cmd_WOLGetNodes(Json::Value &root)
		{
			std::string hwid = m_pWebEm->FindValue("idx");
			if (hwid == "")
				return;
			int iHardwareID = atoi(hwid.c_str());
			CDomoticzHardwareBase *pHardware = m_mainworker.GetHardware(iHardwareID);
			if (pHardware == NULL)
				return;
			if (pHardware->HwdType != HTYPE_WOL)
				return;

			root["status"] = "OK";
			root["title"] = "WOLGetNodes";

			std::stringstream szQuery;
			std::vector<std::vector<std::string> > result;
			szQuery << "SELECT ID,Name,MacAddress FROM WOLNodes WHERE (HardwareID==" << iHardwareID << ")";
			result = m_sql.query(szQuery.str());
			if (result.size() > 0)
			{
				std::vector<std::vector<std::string> >::const_iterator itt;
				int ii = 0;
				for (itt = result.begin(); itt != result.end(); ++itt)
				{
					std::vector<std::string> sd = *itt;

					root["result"][ii]["idx"] = sd[0];
					root["result"][ii]["Name"] = sd[1];
					root["result"][ii]["Mac"] = sd[2];
					ii++;
				}
			}
		}

		void CWebServer::Cmd_WOLAddNode(Json::Value &root)
		{
			if (m_pWebEm->m_actualuser_rights != 2)
			{
				//No admin user, and not allowed to be here
				return;
			}

			std::string hwid = m_pWebEm->FindValue("idx");
			std::string name = m_pWebEm->FindValue("name");
			std::string mac = m_pWebEm->FindValue("mac");
			if (
				(hwid == "") ||
				(name == "") ||
				(mac == "")
				)
				return;
			int iHardwareID = atoi(hwid.c_str());
			CDomoticzHardwareBase *pBaseHardware = m_mainworker.GetHardware(iHardwareID);
			if (pBaseHardware == NULL)
				return;
			if (pBaseHardware->HwdType != HTYPE_WOL)
				return;
			CWOL *pHardware = (CWOL*)pBaseHardware;

			root["status"] = "OK";
			root["title"] = "WOLAddNode";
			pHardware->AddNode(name, mac);
		}

		void CWebServer::Cmd_WOLUpdateNode(Json::Value &root)
		{
			if (m_pWebEm->m_actualuser_rights != 2)
			{
				//No admin user, and not allowed to be here
				return;
			}

			std::string hwid = m_pWebEm->FindValue("idx");
			std::string nodeid = m_pWebEm->FindValue("nodeid");
			std::string name = m_pWebEm->FindValue("name");
			std::string mac = m_pWebEm->FindValue("mac");
			if (
				(hwid == "") ||
				(nodeid == "") ||
				(name == "") ||
				(mac == "")
				)
				return;
			int iHardwareID = atoi(hwid.c_str());
			CDomoticzHardwareBase *pBaseHardware = m_mainworker.GetHardware(iHardwareID);
			if (pBaseHardware == NULL)
				return;
			if (pBaseHardware->HwdType != HTYPE_WOL)
				return;
			CWOL *pHardware = (CWOL*)pBaseHardware;

			int NodeID = atoi(nodeid.c_str());
			root["status"] = "OK";
			root["title"] = "WOLUpdateNode";
			pHardware->UpdateNode(NodeID, name, mac);
		}

		void CWebServer::Cmd_WOLRemoveNode(Json::Value &root)
		{
			if (m_pWebEm->m_actualuser_rights != 2)
			{
				//No admin user, and not allowed to be here
				return;
			}

			std::string hwid = m_pWebEm->FindValue("idx");
			std::string nodeid = m_pWebEm->FindValue("nodeid");
			if (
				(hwid == "") ||
				(nodeid == "")
				)
				return;
			int iHardwareID = atoi(hwid.c_str());
			CDomoticzHardwareBase *pBaseHardware = m_mainworker.GetHardware(iHardwareID);
			if (pBaseHardware == NULL)
				return;
			if (pBaseHardware->HwdType != HTYPE_WOL)
				return;
			CWOL *pHardware = (CWOL*)pBaseHardware;

			int NodeID = atoi(nodeid.c_str());
			root["status"] = "OK";
			root["title"] = "WOLRemoveNode";
			pHardware->RemoveNode(NodeID);
		}

		void CWebServer::Cmd_WOLClearNodes(Json::Value &root)
		{
			if (m_pWebEm->m_actualuser_rights != 2)
			{
				//No admin user, and not allowed to be here
				return;
			}

			std::string hwid = m_pWebEm->FindValue("idx");
			if (hwid == "")
				return;
			int iHardwareID = atoi(hwid.c_str());
			CDomoticzHardwareBase *pBaseHardware = m_mainworker.GetHardware(iHardwareID);
			if (pBaseHardware == NULL)
				return;
			if (pBaseHardware->HwdType != HTYPE_WOL)
				return;
			CWOL *pHardware = (CWOL*)pBaseHardware;

			root["status"] = "OK";
			root["title"] = "WOLClearNodes";
			pHardware->RemoveAllNodes();
		}

		void CWebServer::Cmd_PingerGetNodes(Json::Value &root)
		{
			std::string hwid = m_pWebEm->FindValue("idx");
			if (hwid == "")
				return;
			int iHardwareID = atoi(hwid.c_str());
			CDomoticzHardwareBase *pHardware = m_mainworker.GetHardware(iHardwareID);
			if (pHardware == NULL)
				return;
			if (pHardware->HwdType != HTYPE_Pinger)
				return;

			root["status"] = "OK";
			root["title"] = "PingerGetNodes";

			std::stringstream szQuery;
			std::vector<std::vector<std::string> > result;
			szQuery << "SELECT ID,Name,MacAddress,Timeout FROM WOLNodes WHERE (HardwareID==" << iHardwareID << ")";
			result = m_sql.query(szQuery.str());
			if (result.size() > 0)
			{
				std::vector<std::vector<std::string> >::const_iterator itt;
				int ii = 0;
				for (itt = result.begin(); itt != result.end(); ++itt)
				{
					std::vector<std::string> sd = *itt;

					root["result"][ii]["idx"] = sd[0];
					root["result"][ii]["Name"] = sd[1];
					root["result"][ii]["IP"] = sd[2];
					root["result"][ii]["Timeout"] = atoi(sd[3].c_str());
					ii++;
				}
			}
		}

		void CWebServer::Cmd_PingerSetMode(Json::Value &root)
		{
			if (m_pWebEm->m_actualuser_rights != 2)
			{
				//No admin user, and not allowed to be here
				return;
			}
			std::string hwid = m_pWebEm->FindValue("idx");
			std::string mode1 = m_pWebEm->FindValue("mode1");
			std::string mode2 = m_pWebEm->FindValue("mode2");
			if (
				(hwid == "") ||
				(mode1 == "") ||
				(mode2 == "")
				)
				return;
			int iHardwareID = atoi(hwid.c_str());
			CDomoticzHardwareBase *pBaseHardware = m_mainworker.GetHardware(iHardwareID);
			if (pBaseHardware == NULL)
				return;
			if (pBaseHardware->HwdType != HTYPE_Pinger)
				return;
			CPinger *pHardware = (CPinger*)pBaseHardware;

			root["status"] = "OK";
			root["title"] = "PingerSetMode";

			int iMode1 = atoi(mode1.c_str());
			int iMode2 = atoi(mode2.c_str());

			char szTmp[100];
			sprintf(szTmp,
				"UPDATE Hardware SET Mode1=%d, Mode2=%d WHERE (ID == %s)",
				iMode1,
				iMode2,
				hwid.c_str());
			m_sql.query(szTmp);
			pHardware->SetSettings(iMode1,iMode2);
			pHardware->Restart();
		}
			

		void CWebServer::Cmd_PingerAddNode(Json::Value &root)
		{
			if (m_pWebEm->m_actualuser_rights != 2)
			{
				//No admin user, and not allowed to be here
				return;
			}

			std::string hwid = m_pWebEm->FindValue("idx");
			std::string name = m_pWebEm->FindValue("name");
			std::string ip = m_pWebEm->FindValue("ip");
			int Timeout = atoi(m_pWebEm->FindValue("timeout").c_str());
			if (
				(hwid == "") ||
				(name == "") ||
				(ip == "") ||
				(Timeout==0)
				)
				return;
			int iHardwareID = atoi(hwid.c_str());
			CDomoticzHardwareBase *pBaseHardware = m_mainworker.GetHardware(iHardwareID);
			if (pBaseHardware == NULL)
				return;
			if (pBaseHardware->HwdType != HTYPE_Pinger)
				return;
			CPinger *pHardware = (CPinger*)pBaseHardware;

			root["status"] = "OK";
			root["title"] = "PingerAddNode";
			pHardware->AddNode(name, ip, Timeout);
		}

		void CWebServer::Cmd_PingerUpdateNode(Json::Value &root)
		{
			if (m_pWebEm->m_actualuser_rights != 2)
			{
				//No admin user, and not allowed to be here
				return;
			}

			std::string hwid = m_pWebEm->FindValue("idx");
			std::string nodeid = m_pWebEm->FindValue("nodeid");
			std::string name = m_pWebEm->FindValue("name");
			std::string ip = m_pWebEm->FindValue("ip");
			int Timeout = atoi(m_pWebEm->FindValue("timeout").c_str());
			if (
				(hwid == "") ||
				(nodeid == "") ||
				(name == "") ||
				(ip == "") ||
				(Timeout==0)
				)
				return;
			int iHardwareID = atoi(hwid.c_str());
			CDomoticzHardwareBase *pBaseHardware = m_mainworker.GetHardware(iHardwareID);
			if (pBaseHardware == NULL)
				return;
			if (pBaseHardware->HwdType != HTYPE_Pinger)
				return;
			CPinger *pHardware = (CPinger*)pBaseHardware;

			int NodeID = atoi(nodeid.c_str());
			root["status"] = "OK";
			root["title"] = "PingerUpdateNode";
			pHardware->UpdateNode(NodeID, name, ip, Timeout);
		}

		void CWebServer::Cmd_PingerRemoveNode(Json::Value &root)
		{
			if (m_pWebEm->m_actualuser_rights != 2)
			{
				//No admin user, and not allowed to be here
				return;
			}

			std::string hwid = m_pWebEm->FindValue("idx");
			std::string nodeid = m_pWebEm->FindValue("nodeid");
			if (
				(hwid == "") ||
				(nodeid == "")
				)
				return;
			int iHardwareID = atoi(hwid.c_str());
			CDomoticzHardwareBase *pBaseHardware = m_mainworker.GetHardware(iHardwareID);
			if (pBaseHardware == NULL)
				return;
			if (pBaseHardware->HwdType != HTYPE_Pinger)
				return;
			CPinger *pHardware = (CPinger*)pBaseHardware;

			int NodeID = atoi(nodeid.c_str());
			root["status"] = "OK";
			root["title"] = "PingerRemoveNode";
			pHardware->RemoveNode(NodeID);
		}

		void CWebServer::Cmd_PingerClearNodes(Json::Value &root)
		{
			if (m_pWebEm->m_actualuser_rights != 2)
			{
				//No admin user, and not allowed to be here
				return;
			}

			std::string hwid = m_pWebEm->FindValue("idx");
			if (hwid == "")
				return;
			int iHardwareID = atoi(hwid.c_str());
			CDomoticzHardwareBase *pBaseHardware = m_mainworker.GetHardware(iHardwareID);
			if (pBaseHardware == NULL)
				return;
			if (pBaseHardware->HwdType != HTYPE_Pinger)
				return;
			CPinger *pHardware = (CPinger*)pBaseHardware;

			root["status"] = "OK";
			root["title"] = "PingerClearNodes";
			pHardware->RemoveAllNodes();
		}

		void CWebServer::Cmd_SaveFibaroLinkConfig(Json::Value &root)
		{
			if (m_pWebEm->m_actualuser_rights != 2)
			{
				//No admin user, and not allowed to be here
				return;
			}

			std::string remote = m_pWebEm->FindValue("remote");
			std::string username = m_pWebEm->FindValue("username");
			std::string password = m_pWebEm->FindValue("password");
			std::string linkactive = m_pWebEm->FindValue("linkactive");
			std::string isversion4 = m_pWebEm->FindValue("isversion4");
			std::string debugenabled = m_pWebEm->FindValue("debugenabled");
			if (
				(remote == "") ||
				(username == "") ||
				(password == "") ||
				(linkactive == "") ||
				(isversion4 == "") ||
				(debugenabled == "")
				)
				return;
			int ilinkactive = atoi(linkactive.c_str());
			int iisversion4 = atoi(isversion4.c_str());
			int idebugenabled = atoi(debugenabled.c_str());
			m_sql.UpdatePreferencesVar("FibaroIP", remote.c_str());
			m_sql.UpdatePreferencesVar("FibaroUsername", username.c_str());
			m_sql.UpdatePreferencesVar("FibaroPassword", password.c_str());
			m_sql.UpdatePreferencesVar("FibaroActive", ilinkactive);
			m_sql.UpdatePreferencesVar("FibaroVersion4", iisversion4);
			m_sql.UpdatePreferencesVar("FibaroDebug", idebugenabled);
			m_mainworker.m_datapush.UpdateActive();
			root["status"] = "OK";
			root["title"] = "SaveFibaroLinkConfig";
		}

		void CWebServer::Cmd_GetFibaroLinkConfig(Json::Value &root)
		{
			std::string sValue;
			int nValue;
			if (m_sql.GetPreferencesVar("FibaroActive", nValue)) {
				root["FibaroActive"] = nValue;
			}
			else {
				root["FibaroActive"] = 0;
			}
			if (m_sql.GetPreferencesVar("FibaroVersion4", nValue)) {
				root["FibaroVersion4"] = nValue;
			}
			else {
				root["FibaroVersion4"] = 0;
			}
			if (m_sql.GetPreferencesVar("FibaroDebug", nValue)) {
				root["FibaroDebug"] = nValue;
			}
			else {
				root["FibaroDebug"] = 0;
			}
			if (m_sql.GetPreferencesVar("FibaroIP", sValue))
			{
				root["FibaroIP"] = sValue;
			}
			if (m_sql.GetPreferencesVar("FibaroUsername", sValue))
			{
				root["FibaroUsername"] = sValue;
			}
			if (m_sql.GetPreferencesVar("FibaroPassword", sValue))
			{
				root["FibaroPassword"] = sValue;
			}
			root["status"] = "OK";
			root["title"] = "GetFibaroLinkConfig";
		}

		void CWebServer::Cmd_GetFibaroLinks(Json::Value &root)
		{
			std::stringstream szQuery;
			std::vector<std::vector<std::string> > result;
			szQuery << "SELECT A.ID,A.DeviceID,A.Delimitedvalue,A.TargetType,A.TargetVariable,A.TargetDeviceID,A.TargetProperty,A.Enabled, B.Name, A.IncludeUnit FROM FibaroLink as A, DeviceStatus as B WHERE (A.DeviceID==B.ID)";
			result = m_sql.query(szQuery.str());
			if (result.size() > 0)
			{
				std::vector<std::vector<std::string> >::const_iterator itt;
				int ii = 0;
				for (itt = result.begin(); itt != result.end(); ++itt)
				{
					std::vector<std::string> sd = *itt;
					root["result"][ii]["idx"] = sd[0];
					root["result"][ii]["DeviceID"] = sd[1];
					root["result"][ii]["Delimitedvalue"] = sd[2];
					root["result"][ii]["TargetType"] = sd[3];
					root["result"][ii]["TargetVariable"] = sd[4];
					root["result"][ii]["TargetDevice"] = sd[5];
					root["result"][ii]["TargetProperty"] = sd[6];
					root["result"][ii]["Enabled"] = sd[7];
					root["result"][ii]["Name"] = sd[8];
					root["result"][ii]["IncludeUnit"] = sd[9];
					ii++;
				}
			}
			root["status"] = "OK";
			root["title"] = "GetFibaroLinks";
		}

		void CWebServer::Cmd_SaveFibaroLink(Json::Value &root)
		{
			std::string idx = m_pWebEm->FindValue("idx");
			std::string deviceid = m_pWebEm->FindValue("deviceid");
			int deviceidi = atoi(deviceid.c_str());
			std::string valuetosend = m_pWebEm->FindValue("valuetosend");
			std::string targettype = m_pWebEm->FindValue("targettype");
			int targettypei = atoi(targettype.c_str());
			std::string targetvariable = m_pWebEm->FindValue("targetvariable");
			std::string targetdeviceid = m_pWebEm->FindValue("targetdeviceid");
			std::string targetproperty = m_pWebEm->FindValue("targetproperty");
			std::string linkactive = m_pWebEm->FindValue("linkactive");
			std::string includeunit = m_pWebEm->FindValue("includeunit");
			if ((targettypei == 0) && (targetvariable == ""))
				return;
			if ((targettypei == 1) && ((targetdeviceid == "") || (targetproperty == "")))
				return;
			if ((targettypei == 2) && (targetdeviceid == ""))
				return;
			std::vector<std::vector<std::string> > result;
			char szTmp[300];
			if (idx == "0") {
				sprintf(szTmp, "INSERT INTO FibaroLink (DeviceID,DelimitedValue,TargetType,TargetVariable,TargetDeviceID,TargetProperty,IncludeUnit,Enabled) VALUES ('%d','%d','%d','%s','%d','%s','%d','%d')",
					deviceidi,
					atoi(valuetosend.c_str()),
					targettypei,
					targetvariable.c_str(),
					atoi(targetdeviceid.c_str()),
					targetproperty.c_str(),
					atoi(includeunit.c_str()),
					atoi(linkactive.c_str())
					);
			}
			else {
				sprintf(szTmp,
					"UPDATE FibaroLink SET DeviceID='%d', DelimitedValue=%d, TargetType=%d, TargetVariable='%s', TargetDeviceID=%d, TargetProperty='%s', IncludeUnit='%d', Enabled='%d' WHERE (ID == %s)",
					deviceidi,
					atoi(valuetosend.c_str()),
					targettypei,
					targetvariable.c_str(),
					atoi(targetdeviceid.c_str()),
					targetproperty.c_str(),
					atoi(includeunit.c_str()),
					atoi(linkactive.c_str()),
					idx.c_str()
					);
			}
			result = m_sql.query(szTmp);

			root["status"] = "OK";
			root["title"] = "SaveFibaroLink";
		}

		void CWebServer::Cmd_DeleteFibaroLink(Json::Value &root)
		{
			if (m_pWebEm->m_actualuser_rights != 2)
			{
				//No admin user, and not allowed to be here
				return;
			}

			std::string idx = m_pWebEm->FindValue("idx");
			if (idx == "")
				return;
			std::vector<std::vector<std::string> > result;
			char szTmp[300];
			sprintf(szTmp, "DELETE FROM FibaroLink WHERE (ID==%s)", idx.c_str());
			result = m_sql.query(szTmp);

			root["status"] = "OK";
			root["title"] = "DeleteFibaroLink";
		}

		void CWebServer::Cmd_GetDeviceValueOptions(Json::Value &root)
		{
			std::string idx = m_pWebEm->FindValue("idx");
			if (idx == "")
				return;
			std::vector<std::string> result;
			result = m_mainworker.m_datapush.DropdownOptions(atoi(idx.c_str()));
			if ((result.size() == 1) && result[0] == "Status") {
				root["result"][0]["Value"] = 0;
				root["result"][0]["Wording"] = result[0];
			}
			else {
				std::vector<std::string>::const_iterator itt;
				int ii = 0;
				for (itt = result.begin(); itt != result.end(); ++itt)
				{
					std::string ddOption = *itt;
					root["result"][ii]["Value"] = ii + 1;
					root["result"][ii]["Wording"] = ddOption.c_str();
					ii++;
				}

			}
			root["status"] = "OK";
			root["title"] = "GetDeviceValueOptions";
		}

		void CWebServer::Cmd_GetDeviceValueOptionWording(Json::Value &root)
		{
			std::string idx = m_pWebEm->FindValue("idx");
			std::string pos = m_pWebEm->FindValue("pos");
			if ((idx == "") || (pos == ""))
				return;
			std::string wording;
			wording = m_mainworker.m_datapush.DropdownOptionsValue(atoi(idx.c_str()), atoi(pos.c_str()));
			root["wording"] = wording;
			root["status"] = "OK";
			root["title"] = "GetDeviceValueOptions";
		}


		void CWebServer::Cmd_DeleteUserVariable(Json::Value &root)
		{
			std::string idx = m_pWebEm->FindValue("idx");
			if (idx == "")
				return;

			root["status"] = m_sql.DeleteUserVariable(idx);
			root["title"] = "DeleteUserVariable";
		}

		void CWebServer::Cmd_SaveUserVariable(Json::Value &root)
		{
			std::string variablename = m_pWebEm->FindValue("vname");
			std::string variablevalue = m_pWebEm->FindValue("vvalue");
			std::string variabletype = m_pWebEm->FindValue("vtype");
			if ((variablename == "") || (variablevalue == "") || (variabletype == ""))
				return;

			root["status"] = m_sql.SaveUserVariable(variablename, variabletype, variablevalue);
			root["title"] = "SaveUserVariable";
		}

		void CWebServer::Cmd_UpdateUserVariable(Json::Value &root)
		{
			std::string idx = m_pWebEm->FindValue("idx");
			std::string variablename = m_pWebEm->FindValue("vname");
			std::string variablevalue = m_pWebEm->FindValue("vvalue");
			std::string variabletype = m_pWebEm->FindValue("vtype");

			if (idx.empty())
			{
				//Get idx from valuename
				std::stringstream szQuery;
				std::vector<std::vector<std::string> > result;
				szQuery << "SELECT ID FROM UserVariables WHERE Name='" << variablename << "'";
				result = m_sql.query(szQuery.str());
				if (result.empty())
					return;
				idx = result[0][0];
			}

			if (idx.empty() || variablename.empty() || variablevalue.empty() || variabletype.empty())
				return;

			root["status"] = m_sql.UpdateUserVariable(idx, variablename, variabletype, variablevalue, true);
			root["title"] = "UpdateUserVariable";
		}


		void CWebServer::Cmd_GetUserVariables(Json::Value &root)
		{
			std::stringstream szQuery;
			std::vector<std::vector<std::string> > result;
			szQuery << "SELECT ID,Name,ValueType,Value,LastUpdate FROM UserVariables";
			result = m_sql.GetUserVariables();
			if (result.size() > 0)
			{
				std::vector<std::vector<std::string> >::const_iterator itt;
				int ii = 0;
				for (itt = result.begin(); itt != result.end(); ++itt)
				{
					std::vector<std::string> sd = *itt;
					root["result"][ii]["idx"] = sd[0];
					root["result"][ii]["Name"] = sd[1];
					root["result"][ii]["Type"] = sd[2];
					root["result"][ii]["Value"] = sd[3];
					root["result"][ii]["LastUpdate"] = sd[4];
					ii++;
				}
			}
			root["status"] = "OK";
			root["title"] = "GetUserVariables";
		}

		void CWebServer::Cmd_GetUserVariable(Json::Value &root)
		{
			std::string idx = m_pWebEm->FindValue("idx");
			if (idx == "")
				return;

			int iVarID = atoi(idx.c_str());

			std::stringstream szQuery;
			std::vector<std::vector<std::string> > result;
			szQuery << "SELECT ID,Name,ValueType,Value,LastUpdate FROM UserVariables WHERE (ID==" << iVarID << ")";
			result = m_sql.query(szQuery.str());
			if (result.size() > 0)
			{
				std::vector<std::vector<std::string> >::const_iterator itt;
				int ii = 0;
				for (itt = result.begin(); itt != result.end(); ++itt)
				{
					std::vector<std::string> sd = *itt;
					root["result"][ii]["idx"] = sd[0];
					root["result"][ii]["Name"] = sd[1];
					root["result"][ii]["Type"] = sd[2];
					root["result"][ii]["Value"] = sd[3];
					root["result"][ii]["LastUpdate"] = sd[4];
					ii++;
				}
			}
			root["status"] = "OK";
			root["title"] = "GetUserVariable";
		}


		void CWebServer::Cmd_AllowNewHardware(Json::Value &root)
		{
			std::string sTimeout = m_pWebEm->FindValue("timeout");
			if (sTimeout == "")
				return;
			root["status"] = "OK";
			root["title"] = "AllowNewHardware";

			m_sql.AllowNewHardwareTimer(atoi(sTimeout.c_str()));
		}


		void CWebServer::Cmd_DeleteHardware(Json::Value &root)
		{
			if (m_pWebEm->m_actualuser_rights != 2)
			{
				//No admin user, and not allowed to be here
				return;
			}

			std::string idx = m_pWebEm->FindValue("idx");
			if (idx == "")
				return;
			int hwID = atoi(idx.c_str());

			root["status"] = "OK";
			root["title"] = "DeleteHardware";

			m_mainworker.RemoveDomoticzHardware(hwID);
			m_sql.DeleteHardware(idx);
		}

#ifdef WITH_OPENZWAVE
		void CWebServer::RType_OpenZWaveNodes(Json::Value &root)
		{
			std::string hwid = m_pWebEm->FindValue("idx");
			if (hwid == "")
				return;
			int iHardwareID = atoi(hwid.c_str());
			CDomoticzHardwareBase *pHardware = m_mainworker.GetHardware(iHardwareID);
			if (pHardware == NULL)
				return;
			if (pHardware->HwdType != HTYPE_OpenZWave)
				return;
			m_ZW_Hwidx = iHardwareID;
			COpenZWave *pOZWHardware = (COpenZWave*)pHardware;

			root["status"] = "OK";
			root["title"] = "OpenZWaveNodes";

			root["NodesQueried"] = (pOZWHardware->m_awakeNodesQueried) || (pOZWHardware->m_allNodesQueried);
			root["ownNodeId"] = pOZWHardware->m_controllerNodeId;

			std::stringstream szQuery;
			std::vector<std::vector<std::string> > result;
			szQuery << "SELECT ID,HomeID,NodeID,Name,ProductDescription,PollTime FROM ZWaveNodes WHERE (HardwareID==" << iHardwareID << ")";
			result = m_sql.query(szQuery.str());
			if (result.size() > 0)
			{
				std::vector<std::vector<std::string> >::const_iterator itt;
				int ii = 0;
				for (itt = result.begin(); itt != result.end(); ++itt)
				{
					std::vector<std::string> sd = *itt;

					unsigned int homeID = boost::lexical_cast<unsigned int>(sd[1]);
					int nodeID = atoi(sd[2].c_str());
					//if (nodeID>1) //Don't include the controller
					{
						COpenZWave::NodeInfo *pNode = pOZWHardware->GetNodeInfo(homeID, nodeID);
						if (pNode == NULL)
							continue;
						root["result"][ii]["idx"] = sd[0];
						root["result"][ii]["HomeID"] = homeID;
						root["result"][ii]["NodeID"] = nodeID;
						root["result"][ii]["Name"] = sd[3];
						root["result"][ii]["Description"] = sd[4];
						root["result"][ii]["PollEnabled"] = (atoi(sd[5].c_str()) == 1) ? "true" : "false";
						root["result"][ii]["Version"] = pNode->iVersion;
						root["result"][ii]["Manufacturer_id"] = pNode->Manufacturer_id;
						root["result"][ii]["Manufacturer_name"] = pNode->Manufacturer_name;
						root["result"][ii]["Product_type"] = pNode->Product_type;
						root["result"][ii]["Product_id"] = pNode->Product_id;
						root["result"][ii]["Product_name"] = pNode->Product_name;
						root["result"][ii]["State"] = pOZWHardware->GetNodeStateString(homeID, nodeID);
						root["result"][ii]["HaveUserCodes"] = pNode->HaveUserCodes;
						char szDate[80];
						struct tm loctime;
						localtime_r(&pNode->m_LastSeen, &loctime);
						strftime(szDate, 80, "%Y-%m-%d %X", &loctime);

						root["result"][ii]["LastUpdate"] = szDate;

						//Add configuration parameters here
						pOZWHardware->GetNodeValuesJson(homeID, nodeID, root, ii);
						ii++;
					}
				}
			}
		}

		void CWebServer::Cmd_ZWaveUpdateNode(Json::Value &root)
		{
			if (m_pWebEm->m_actualuser_rights != 2)
			{
				//No admin user, and not allowed to be here
				return;
			}

			std::string idx = m_pWebEm->FindValue("idx");
			if (idx == "")
				return;
			std::string name = m_pWebEm->FindValue("name");
			std::string senablepolling = m_pWebEm->FindValue("EnablePolling");
			if (
				(name == "") ||
				(senablepolling == "")
				)
				return;
			root["status"] = "OK";
			root["title"] = "UpdateZWaveNode";

			std::vector<std::vector<std::string> > result;
			char szTmp[300];

			sprintf(szTmp,
				"UPDATE ZWaveNodes SET Name='%s', PollTime=%d WHERE (ID==%s)",
				name.c_str(),
				(senablepolling == "true") ? 1 : 0,
				idx.c_str()
				);
			result = m_sql.query(szTmp);
			sprintf(szTmp, "SELECT HardwareID, HomeID, NodeID from ZWaveNodes WHERE (ID==%s)", idx.c_str());
			result = m_sql.query(szTmp);
			if (result.size() > 0)
			{
				int hwid = atoi(result[0][0].c_str());
				int homeID = atoi(result[0][1].c_str());
				int nodeID = atoi(result[0][2].c_str());
				CDomoticzHardwareBase *pHardware = m_mainworker.GetHardware(hwid);
				if (pHardware != NULL)
				{
					COpenZWave *pOZWHardware = (COpenZWave*)pHardware;
					pOZWHardware->SetNodeName(homeID, nodeID, name);
					pOZWHardware->EnableDisableNodePolling(nodeID);
				}
			}
		}

		void CWebServer::Cmd_ZWaveDeleteNode(Json::Value &root)
		{
			if (m_pWebEm->m_actualuser_rights != 2)
			{
				//No admin user, and not allowed to be here
				return;
			}

			std::string idx = m_pWebEm->FindValue("idx");
			if (idx == "")
				return;
			std::vector<std::vector<std::string> > result;
			char szTmp[300];
			sprintf(szTmp, "SELECT HardwareID,HomeID,NodeID from ZWaveNodes WHERE (ID==%s)", idx.c_str());
			result = m_sql.query(szTmp);
			if (result.size() > 0)
			{
				int hwid = atoi(result[0][0].c_str());
				unsigned int homeID = boost::lexical_cast<unsigned int>(result[0][1]);
				int nodeID = atoi(result[0][2].c_str());
				CDomoticzHardwareBase *pHardware = m_mainworker.GetHardware(hwid);
				if (pHardware != NULL)
				{
					COpenZWave *pOZWHardware = (COpenZWave*)pHardware;
					pOZWHardware->RemoveFailedDevice(nodeID);
					root["status"] = "OK";
					root["title"] = "DeleteZWaveNode";
					sprintf(szTmp, "DELETE FROM ZWaveNodes WHERE (ID==%s)", idx.c_str());
					result = m_sql.query(szTmp);
				}
			}

		}

		void CWebServer::Cmd_ZWaveInclude(Json::Value &root)
		{
			if (m_pWebEm->m_actualuser_rights != 2)
			{
				//No admin user, and not allowed to be here
				return;
			}

			std::string idx = m_pWebEm->FindValue("idx");
			if (idx == "")
				return;
			std::string ssecure = m_pWebEm->FindValue("secure");
			bool bSecure = (ssecure == "true");
			CDomoticzHardwareBase *pHardware = m_mainworker.GetHardware(atoi(idx.c_str()));
			if (pHardware != NULL)
			{
				COpenZWave *pOZWHardware = (COpenZWave*)pHardware;
				m_sql.AllowNewHardwareTimer(5);
				pOZWHardware->IncludeDevice(bSecure);
				root["status"] = "OK";
				root["title"] = "ZWaveInclude";
			}
		}

		void CWebServer::Cmd_ZWaveExclude(Json::Value &root)
		{
			if (m_pWebEm->m_actualuser_rights != 2)
			{
				//No admin user, and not allowed to be here
				return;
			}

			std::string idx = m_pWebEm->FindValue("idx");
			if (idx == "")
				return;
			CDomoticzHardwareBase *pHardware = m_mainworker.GetHardware(atoi(idx.c_str()));
			if (pHardware != NULL)
			{
				COpenZWave *pOZWHardware = (COpenZWave*)pHardware;
				pOZWHardware->ExcludeDevice(1);
				root["status"] = "OK";
				root["title"] = "ZWaveExclude";
			}
		}

		void CWebServer::Cmd_ZWaveSoftReset(Json::Value &root)
		{
			if (m_pWebEm->m_actualuser_rights != 2)
			{
				//No admin user, and not allowed to be here
				return;
			}

			std::string idx = m_pWebEm->FindValue("idx");
			if (idx == "")
				return;
			CDomoticzHardwareBase *pHardware = m_mainworker.GetHardware(atoi(idx.c_str()));
			if (pHardware != NULL)
			{
				COpenZWave *pOZWHardware = (COpenZWave*)pHardware;
				pOZWHardware->SoftResetDevice();
				root["status"] = "OK";
				root["title"] = "ZWaveSoftReset";
			}
		}

		void CWebServer::Cmd_ZWaveHardReset(Json::Value &root)
		{
			if (m_pWebEm->m_actualuser_rights != 2)
			{
				//No admin user, and not allowed to be here
				return;
			}

			std::string idx = m_pWebEm->FindValue("idx");
			if (idx == "")
				return;
			CDomoticzHardwareBase *pHardware = m_mainworker.GetHardware(atoi(idx.c_str()));
			if (pHardware != NULL)
			{
				COpenZWave *pOZWHardware = (COpenZWave*)pHardware;
				pOZWHardware->HardResetDevice();
				root["status"] = "OK";
				root["title"] = "ZWaveHardReset";
			}
		}

		void CWebServer::Cmd_ZWaveStateCheck(Json::Value &root)
		{
			root["title"] = "ZWaveStateCheck";
			std::string idx = m_pWebEm->FindValue("idx");
			if (idx == "")
				return;
			CDomoticzHardwareBase *pHardware = m_mainworker.GetHardware(atoi(idx.c_str()));
			if (pHardware != NULL)
			{
				COpenZWave *pOZWHardware = (COpenZWave*)pHardware;
				if (!pOZWHardware->GetFailedState()) {
					root["status"] = "OK";
				}
			}
			return;
		}

		void CWebServer::Cmd_ZWaveNetworkHeal(Json::Value &root)
		{
			if (m_pWebEm->m_actualuser_rights != 2)
			{
				//No admin user, and not allowed to be here
				return;
			}

			std::string idx = m_pWebEm->FindValue("idx");
			if (idx == "")
				return;
			CDomoticzHardwareBase *pHardware = m_mainworker.GetHardware(atoi(idx.c_str()));
			if (pHardware != NULL)
			{
				COpenZWave *pOZWHardware = (COpenZWave*)pHardware;
				pOZWHardware->HealNetwork();
				root["status"] = "OK";
				root["title"] = "ZWaveHealNetwork";
			}
		}

		void CWebServer::Cmd_ZWaveNodeHeal(Json::Value &root)
		{
			if (m_pWebEm->m_actualuser_rights != 2)
			{
				//No admin user, and not allowed to be here
				return;
			}

			std::string idx = m_pWebEm->FindValue("idx");
			if (idx == "")
				return;
			std::string node = m_pWebEm->FindValue("node");
			if (node == "")
				return;
			CDomoticzHardwareBase *pHardware = m_mainworker.GetHardware(atoi(idx.c_str()));
			if (pHardware != NULL)
			{
				COpenZWave *pOZWHardware = (COpenZWave*)pHardware;
				pOZWHardware->HealNode(atoi(node.c_str()));
				root["status"] = "OK";
				root["title"] = "ZWaveHealNode";
			}
		}

		void CWebServer::Cmd_ZWaveNetworkInfo(Json::Value &root)
		{
			if (m_pWebEm->m_actualuser_rights != 2)
			{
				//No admin user, and not allowed to be here
				return;
			}

			root["title"] = "ZWaveNetworkInfo";

			std::string idx = m_pWebEm->FindValue("idx");
			if (idx == "")
				return;
			int hwID = atoi(idx.c_str());
			CDomoticzHardwareBase *pHardware = m_mainworker.GetHardware(hwID);
			if (pHardware != NULL)
			{
				COpenZWave *pOZWHardware = (COpenZWave*)pHardware;
				std::vector< std::vector< int > > nodevectors;

				if (pOZWHardware->NetworkInfo(hwID, nodevectors)) {

					std::vector<std::vector<int> >::iterator row_iterator;
					std::vector<int>::iterator col_iterator;

					std::vector<int> allnodes;
					int rowCount = 0;
					std::stringstream allnodeslist;
					for (row_iterator = nodevectors.begin(); row_iterator != nodevectors.end(); ++row_iterator) {
						int colCount = 0;
						int nodeID = -1;
						std::vector<int> rest;
						std::stringstream list;

						for (col_iterator = (*row_iterator).begin(); col_iterator != (*row_iterator).end(); ++col_iterator) {
							if (colCount == 0) {
								nodeID = *col_iterator;
							}
							else {
								rest.push_back(*col_iterator);
							}
							colCount++;
						}

						if (nodeID != -1)
						{
							std::copy(rest.begin(), rest.end(), std::ostream_iterator<int>(list, ","));
							root["result"]["mesh"][rowCount]["nodeID"] = nodeID;
							allnodes.push_back(nodeID);
							root["result"]["mesh"][rowCount]["seesNodes"] = list.str();
							rowCount++;
						}
					}
					std::copy(allnodes.begin(), allnodes.end(), std::ostream_iterator<int>(allnodeslist, ","));
					root["result"]["nodes"] = allnodeslist.str();
					root["status"] = "OK";
				}

			}
		}

		void CWebServer::Cmd_ZWaveRemoveGroupNode(Json::Value &root)
		{
			if (m_pWebEm->m_actualuser_rights != 2)
			{
				//No admin user, and not allowed to be here
				return;
			}

			std::string idx = m_pWebEm->FindValue("idx");
			if (idx == "")
				return;
			std::string node = m_pWebEm->FindValue("node");
			if (node == "")
				return;
			std::string group = m_pWebEm->FindValue("group");
			if (group == "")
				return;
			std::string removenode = m_pWebEm->FindValue("removenode");
			if (removenode == "")
				return;
			CDomoticzHardwareBase *pHardware = m_mainworker.GetHardware(atoi(idx.c_str()));
			if (pHardware != NULL)
			{
				COpenZWave *pOZWHardware = (COpenZWave*)pHardware;
				pOZWHardware->RemoveNodeFromGroup(atoi(node.c_str()), atoi(group.c_str()), atoi(removenode.c_str()));
				root["status"] = "OK";
				root["title"] = "ZWaveRemoveGroupNode";
			}
		}

		void CWebServer::Cmd_ZWaveAddGroupNode(Json::Value &root)
		{
			if (m_pWebEm->m_actualuser_rights != 2)
			{
				//No admin user, and not allowed to be here
				return;
			}

			std::string idx = m_pWebEm->FindValue("idx");
			if (idx == "")
				return;
			std::string node = m_pWebEm->FindValue("node");
			if (node == "")
				return;
			std::string group = m_pWebEm->FindValue("group");
			if (group == "")
				return;
			std::string addnode = m_pWebEm->FindValue("addnode");
			if (addnode == "")
				return;
			CDomoticzHardwareBase *pHardware = m_mainworker.GetHardware(atoi(idx.c_str()));
			if (pHardware != NULL)
			{
				COpenZWave *pOZWHardware = (COpenZWave*)pHardware;
				pOZWHardware->AddNodeToGroup(atoi(node.c_str()), atoi(group.c_str()), atoi(addnode.c_str()));
				root["status"] = "OK";
				root["title"] = "ZWaveAddGroupNode";
			}
		}

		void CWebServer::Cmd_ZWaveGroupInfo(Json::Value &root)
		{
			std::string idx = m_pWebEm->FindValue("idx");
			if (idx == "")
				return;
			int iHardwareID = atoi(idx.c_str());

			CDomoticzHardwareBase *pHardware = m_mainworker.GetHardware(atoi(idx.c_str()));
			if (pHardware != NULL)
			{
				COpenZWave *pOZWHardware = (COpenZWave*)pHardware;

				std::stringstream szQuery;
				std::vector<std::vector<std::string> > result;
				szQuery << "SELECT ID,HomeID,NodeID,Name FROM ZWaveNodes WHERE (HardwareID==" << iHardwareID << ")";
				result = m_sql.query(szQuery.str());

				if (result.size() > 0)
				{
					int MaxNoOfGroups = 0;
					std::vector<std::vector<std::string> >::const_iterator itt;
					int ii = 0;
					for (itt = result.begin(); itt != result.end(); ++itt)
					{
						std::vector<std::string> sd = *itt;
						unsigned int homeID = boost::lexical_cast<unsigned int>(sd[1]);
						int nodeID = atoi(sd[2].c_str());
						COpenZWave::NodeInfo *pNode = pOZWHardware->GetNodeInfo(homeID, nodeID);
						if (pNode == NULL)
							continue;
						std::string nodeName = sd[3].c_str();
						int numGroups = pOZWHardware->ListGroupsForNode(nodeID);
						root["result"]["nodes"][ii]["nodeID"] = nodeID;
						root["result"]["nodes"][ii]["nodeName"] = nodeName;
						root["result"]["nodes"][ii]["groupCount"] = numGroups;
						if (numGroups > 0) {
							if (numGroups > MaxNoOfGroups)
								MaxNoOfGroups = numGroups;

							std::vector< int > nodesingroup;
							int gi = 0;
							for (int x = 1; x <= numGroups; x++)
							{
								int numNodesInGroup = pOZWHardware->ListAssociatedNodesinGroup(nodeID, x, nodesingroup);
								if (numNodesInGroup > 0) {
									std::stringstream list;
									std::copy(nodesingroup.begin(), nodesingroup.end(), std::ostream_iterator<int>(list, ","));
									root["result"]["nodes"][ii]["groups"][gi]["id"] = x;
									root["result"]["nodes"][ii]["groups"][gi]["nodes"] = list.str();
								}
								else {
									root["result"]["nodes"][ii]["groups"][gi]["id"] = x;
									root["result"]["nodes"][ii]["groups"][gi]["nodes"] = "";
								}
								gi++;
								nodesingroup.clear();
							}

						}
						ii++;
					}
					root["result"]["MaxNoOfGroups"] = MaxNoOfGroups;

				}
			}
			root["status"] = "OK";
			root["title"] = "ZWaveGroupInfo";
		}

		void CWebServer::Cmd_ZWaveCancel(Json::Value &root)
		{
			std::string idx = m_pWebEm->FindValue("idx");
			if (idx == "")
				return;
			CDomoticzHardwareBase *pHardware = m_mainworker.GetHardware(atoi(idx.c_str()));
			if (pHardware != NULL)
			{
				COpenZWave *pOZWHardware = (COpenZWave*)pHardware;
				pOZWHardware->CancelControllerCommand();
				root["status"] = "OK";
				root["title"] = "ZWaveCancel";
			}
		}

		void CWebServer::Cmd_ApplyZWaveNodeConfig(Json::Value &root)
		{
			if (m_pWebEm->m_actualuser_rights != 2)
			{
				//No admin user, and not allowed to be here
				return;
			}

			std::string idx = m_pWebEm->FindValue("idx");
			std::string svaluelist = m_pWebEm->FindValue("valuelist");
			if (
				(idx == "") ||
				(svaluelist == "")
				)
				return;
			std::vector<std::vector<std::string> > result;
			char szTmp[300];
			sprintf(szTmp, "SELECT HardwareID,HomeID,NodeID from ZWaveNodes WHERE (ID==%s)", idx.c_str());
			result = m_sql.query(szTmp);
			if (result.size() > 0)
			{
				int hwid = atoi(result[0][0].c_str());
				unsigned int homeID = boost::lexical_cast<unsigned int>(result[0][1]);
				int nodeID = atoi(result[0][2].c_str());
				CDomoticzHardwareBase *pHardware = m_mainworker.GetHardware(hwid);
				if (pHardware != NULL)
				{
					COpenZWave *pOZWHardware = (COpenZWave*)pHardware;
					if (!pOZWHardware->ApplyNodeConfig(homeID, nodeID, svaluelist))
						return;
					root["status"] = "OK";
					root["title"] = "ApplyZWaveNodeConfig";
				}
			}
		}

		void CWebServer::Cmd_ZWaveRequestNodeConfig(Json::Value &root)
		{
			std::string idx = m_pWebEm->FindValue("idx");
			if (idx == "")
				return;
			std::vector<std::vector<std::string> > result;
			char szTmp[300];
			sprintf(szTmp, "SELECT HardwareID,HomeID,NodeID from ZWaveNodes WHERE (ID==%s)", idx.c_str());
			result = m_sql.query(szTmp);
			if (result.size() > 0)
			{
				int hwid = atoi(result[0][0].c_str());
				unsigned int homeID = boost::lexical_cast<unsigned int>(result[0][1]);
				int nodeID = atoi(result[0][2].c_str());
				CDomoticzHardwareBase *pHardware = m_mainworker.GetHardware(hwid);
				if (pHardware != NULL)
				{
					COpenZWave *pOZWHardware = (COpenZWave*)pHardware;
					pOZWHardware->RequestNodeConfig(homeID, nodeID);
					root["status"] = "OK";
					root["title"] = "RequestZWaveNodeConfig";
				}
			}
		}

		void CWebServer::Cmd_ZWaveReceiveConfigurationFromOtherController(Json::Value &root)
		{
			if (m_pWebEm->m_actualuser_rights != 2)
			{
				//No admin user, and not allowed to be here
				return;
			}

			std::string idx = m_pWebEm->FindValue("idx");
			if (idx == "")
				return;
			CDomoticzHardwareBase *pHardware = m_mainworker.GetHardware(atoi(idx.c_str()));
			if (pHardware != NULL)
			{
				COpenZWave *pOZWHardware = (COpenZWave*)pHardware;
				pOZWHardware->ReceiveConfigurationFromOtherController();
				root["status"] = "OK";
				root["title"] = "ZWaveReceiveConfigurationFromOtherController";
			}
		}

		void CWebServer::Cmd_ZWaveSendConfigurationToSecondaryController(Json::Value &root)
		{
			if (m_pWebEm->m_actualuser_rights != 2)
			{
				//No admin user, and not allowed to be here
				return;
			}

			std::string idx = m_pWebEm->FindValue("idx");
			if (idx == "")
				return;
			CDomoticzHardwareBase *pHardware = m_mainworker.GetHardware(atoi(idx.c_str()));
			if (pHardware != NULL)
			{
				COpenZWave *pOZWHardware = (COpenZWave*)pHardware;
				pOZWHardware->SendConfigurationToSecondaryController();
				root["status"] = "OK";
				root["title"] = "ZWaveSendConfigToSecond";
			}
		}

		void CWebServer::Cmd_ZWaveTransferPrimaryRole(Json::Value &root)
		{
			if (m_pWebEm->m_actualuser_rights != 2)
			{
				//No admin user, and not allowed to be here
				return;
			}

			std::string idx = m_pWebEm->FindValue("idx");
			if (idx == "")
				return;
			CDomoticzHardwareBase *pHardware = m_mainworker.GetHardware(atoi(idx.c_str()));
			if (pHardware != NULL)
			{
				COpenZWave *pOZWHardware = (COpenZWave*)pHardware;
				pOZWHardware->TransferPrimaryRole();
				root["status"] = "OK";
				root["title"] = "ZWaveTransferPrimaryRole";
			}
		}
		std::string CWebServer::ZWaveGetConfigFile()
		{
			std::string idx = m_pWebEm->FindValue("idx");
			if (idx == "")
				return "";
			m_retstr = "";
			CDomoticzHardwareBase *pHardware = m_mainworker.GetHardware(atoi(idx.c_str()));
			if (pHardware != NULL)
			{
				if (pHardware->HwdType == HTYPE_OpenZWave)
				{
					COpenZWave *pOZWHardware = (COpenZWave*)pHardware;
					std::string szConfigFile = "";
					m_retstr = pOZWHardware->GetConfigFile(szConfigFile);
					if (m_retstr != "")
					{
						m_pWebEm->m_outputfilename = szConfigFile;
					}
				}
			}
			return m_retstr;
		}
		std::string CWebServer::ZWaveCPIndex()
		{
			m_retstr = "";
			CDomoticzHardwareBase *pHardware = m_mainworker.GetHardware(m_ZW_Hwidx);
			if (pHardware != NULL)
			{
				if (pHardware->HwdType == HTYPE_OpenZWave)
				{
					COpenZWave *pOZWHardware = (COpenZWave*)pHardware;
					pOZWHardware->SetAllNodesChanged();
					std::string wwwFile = szWWWFolder + "/ozwcp/cp.html";
					std::ifstream testFile(wwwFile.c_str(), std::ios::binary);
					std::vector<char> fileContents((std::istreambuf_iterator<char>(testFile)),
						std::istreambuf_iterator<char>());
					if (fileContents.size() > 0)
					{
						m_retstr.insert(m_retstr.begin(), fileContents.begin(), fileContents.end());
					}
				}
			}
			return m_retstr;
		}
		std::string CWebServer::ZWaveCPPollXml()
		{
			m_retstr = "";
			CDomoticzHardwareBase *pHardware = m_mainworker.GetHardware(m_ZW_Hwidx);
			if (pHardware != NULL)
			{
				if (pHardware->HwdType == HTYPE_OpenZWave)
				{
					COpenZWave *pOZWHardware = (COpenZWave*)pHardware;
					m_retstr = pOZWHardware->SendPollResponse();
					m_pWebEm->m_outputfilename = "poll.xml";
				}
			}
			return m_retstr;
		}
		std::string CWebServer::ZWaveCPNodeGetConf()
		{
			m_retstr = "";
			if (m_pWebEm->m_ActualRequest.content.find("node") == std::string::npos)
				return "";
			m_pWebEm->MakeValuesFromPostContent(&m_pWebEm->m_ActualRequest);
			std::string sNode = m_pWebEm->FindValue("node");
			if (sNode == "")
				return m_retstr;
			int iNode = atoi(sNode.c_str());
			CDomoticzHardwareBase *pHardware = m_mainworker.GetHardware(m_ZW_Hwidx);
			if (pHardware != NULL)
			{
				if (pHardware->HwdType == HTYPE_OpenZWave)
				{
					COpenZWave *pOZWHardware = (COpenZWave*)pHardware;
					m_retstr = pOZWHardware->SendNodeConfResponse(iNode);
				}
			}
			return m_retstr;
		}
		std::string CWebServer::ZWaveCPNodeGetValues()
		{
			m_retstr = "";
			if (m_pWebEm->m_ActualRequest.content.find("node") == std::string::npos)
				return "";
			m_pWebEm->MakeValuesFromPostContent(&m_pWebEm->m_ActualRequest);
			std::string sNode = m_pWebEm->FindValue("node");
			if (sNode == "")
				return m_retstr;
			int iNode = atoi(sNode.c_str());
			CDomoticzHardwareBase *pHardware = m_mainworker.GetHardware(m_ZW_Hwidx);
			if (pHardware != NULL)
			{
				if (pHardware->HwdType == HTYPE_OpenZWave)
				{
					COpenZWave *pOZWHardware = (COpenZWave*)pHardware;
					m_retstr = pOZWHardware->SendNodeValuesResponse(iNode);
				}
			}
			return m_retstr;
		}
		std::string CWebServer::ZWaveCPNodeSetValue()
		{
			m_retstr = "";
			std::vector<std::string> strarray;
			StringSplit(m_pWebEm->m_ActualRequest.content, "=", strarray);
			if (strarray.size() != 2)
				return "";

			CDomoticzHardwareBase *pHardware = m_mainworker.GetHardware(m_ZW_Hwidx);
			if (pHardware != NULL)
			{
				if (pHardware->HwdType == HTYPE_OpenZWave)
				{
					COpenZWave *pOZWHardware = (COpenZWave*)pHardware;
					m_retstr = pOZWHardware->SetNodeValue(strarray[0], strarray[1]);
				}
			}
			return m_retstr;
		}
		std::string CWebServer::ZWaveCPNodeSetButton()
		{
			m_retstr = "";
			std::vector<std::string> strarray;
			StringSplit(m_pWebEm->m_ActualRequest.content, "=", strarray);
			if (strarray.size() != 2)
				return "";

			CDomoticzHardwareBase *pHardware = m_mainworker.GetHardware(m_ZW_Hwidx);
			if (pHardware != NULL)
			{
				if (pHardware->HwdType == HTYPE_OpenZWave)
				{
					COpenZWave *pOZWHardware = (COpenZWave*)pHardware;
					m_retstr = pOZWHardware->SetNodeButton(strarray[0], strarray[1]);
				}
			}
			return m_retstr;
		}
		std::string CWebServer::ZWaveCPAdminCommand()
		{
			m_retstr = "";
			if (m_pWebEm->m_ActualRequest.content.find("fun") == std::string::npos)
				return "";
			m_pWebEm->MakeValuesFromPostContent(&m_pWebEm->m_ActualRequest);
			std::string sFun = m_pWebEm->FindValue("fun");
			std::string sNode = m_pWebEm->FindValue("node");
			std::string sButton = m_pWebEm->FindValue("button");

			CDomoticzHardwareBase *pHardware = m_mainworker.GetHardware(m_ZW_Hwidx);
			if (pHardware != NULL)
			{
				if (pHardware->HwdType == HTYPE_OpenZWave)
				{
					COpenZWave *pOZWHardware = (COpenZWave*)pHardware;
					m_retstr = pOZWHardware->DoAdminCommand(sFun,atoi(sNode.c_str()),atoi(sButton.c_str()));
				}
			}
			return m_retstr;
		}
		std::string CWebServer::ZWaveCPNodeChange()
		{
			m_retstr = "";
			if (m_pWebEm->m_ActualRequest.content.find("fun") == std::string::npos)
				return "";
			m_pWebEm->MakeValuesFromPostContent(&m_pWebEm->m_ActualRequest);
			std::string sFun = m_pWebEm->FindValue("fun");
			std::string sNode = m_pWebEm->FindValue("node");
			std::string sValue = m_pWebEm->FindValue("value");

			if (sNode.size() > 4)
				sNode = sNode.substr(4);

			CDomoticzHardwareBase *pHardware = m_mainworker.GetHardware(m_ZW_Hwidx);
			if (pHardware != NULL)
			{
				if (pHardware->HwdType == HTYPE_OpenZWave)
				{
					COpenZWave *pOZWHardware = (COpenZWave*)pHardware;
					m_retstr = pOZWHardware->DoNodeChange(sFun, atoi(sNode.c_str()), sValue);
				}
			}
			return m_retstr;
		}
		std::string CWebServer::ZWaveCPSaveConfig()
		{
			m_retstr = "";
			CDomoticzHardwareBase *pHardware = m_mainworker.GetHardware(m_ZW_Hwidx);
			if (pHardware != NULL)
			{
				if (pHardware->HwdType == HTYPE_OpenZWave)
				{
					COpenZWave *pOZWHardware = (COpenZWave*)pHardware;
					m_retstr = pOZWHardware->SaveConfig();
				}
			}
			return m_retstr;
		}
		std::string CWebServer::ZWaveCPGetTopo()
		{
			m_retstr = "";
			CDomoticzHardwareBase *pHardware = m_mainworker.GetHardware(m_ZW_Hwidx);
			if (pHardware != NULL)
			{
				if (pHardware->HwdType == HTYPE_OpenZWave)
				{
					COpenZWave *pOZWHardware = (COpenZWave*)pHardware;
					m_retstr = pOZWHardware->GetCPTopo();
					m_pWebEm->m_outputfilename = "topo.xml";
				}
			}
			return m_retstr;
		}
		std::string CWebServer::ZWaveCPGetStats()
		{
			m_retstr = "";
			//Crashes at OpenZWave::GetNodeStatistics::_data->m_sentTS = m_sentTS.GetAsString();
			/*
			CDomoticzHardwareBase *pHardware = m_mainworker.GetHardware(m_ZW_Hwidx);
			if (pHardware != NULL)
			{
				if (pHardware->HwdType == HTYPE_OpenZWave)
				{
					COpenZWave *pOZWHardware = (COpenZWave*)pHardware;
					m_retstr = pOZWHardware->GetCPStats();
					m_pWebEm->m_outputfilename = "stats.xml";
				}
			}
			*/
			return m_retstr;
		}
		
		

		void CWebServer::Cmd_ZWaveSetUserCodeEnrollmentMode(Json::Value &root)
		{
			if (m_pWebEm->m_actualuser_rights != 2)
			{
				//No admin user, and not allowed to be here
				return;
			}

			std::string idx = m_pWebEm->FindValue("idx");
			if (idx == "")
				return;
			CDomoticzHardwareBase *pHardware = m_mainworker.GetHardware(atoi(idx.c_str()));
			if (pHardware != NULL)
			{
				COpenZWave *pOZWHardware = (COpenZWave*)pHardware;
				root["status"] = "OK";
				root["title"] = "SetUserCodeEnrollmentMode";
				pOZWHardware->SetUserCodeEnrollmentMode();
			}
		}

		void CWebServer::Cmd_ZWaveRemoveUserCode(Json::Value &root)
		{
			if (m_pWebEm->m_actualuser_rights != 2)
			{
				//No admin user, and not allowed to be here
				return;
			}

			std::string idx = m_pWebEm->FindValue("idx");
			std::string scodeindex = m_pWebEm->FindValue("codeindex");
			if (
				(idx == "") ||
				(scodeindex == "")
				)
				return;
			int iCodeIndex = atoi(scodeindex.c_str());

			std::vector<std::vector<std::string> > result;
			char szTmp[300];
			sprintf(szTmp, "SELECT HardwareID,HomeID,NodeID from ZWaveNodes WHERE (ID==%s)", idx.c_str());
			result = m_sql.query(szTmp);
			if (result.size() > 0)
			{
				int hwid = atoi(result[0][0].c_str());
				unsigned int homeID = boost::lexical_cast<unsigned int>(result[0][1]);
				int nodeID = atoi(result[0][2].c_str());
				CDomoticzHardwareBase *pHardware = m_mainworker.GetHardware(hwid);
				if (pHardware != NULL)
				{
					COpenZWave *pOZWHardware = (COpenZWave*)pHardware;
					if (!pOZWHardware->RemoveUserCode(homeID, nodeID, iCodeIndex))
						return;
					root["status"] = "OK";
					root["title"] = "RemoveUserCode";
				}
			}
		}

		void CWebServer::Cmd_ZWaveGetNodeUserCodes(Json::Value &root)
		{
			std::string idx = m_pWebEm->FindValue("idx");
			if (idx == "")
				return;

			std::vector<std::vector<std::string> > result;
			char szTmp[300];
			sprintf(szTmp, "SELECT HardwareID,HomeID,NodeID from ZWaveNodes WHERE (ID==%s)", idx.c_str());
			result = m_sql.query(szTmp);
			if (result.size() > 0)
			{
				int hwid = atoi(result[0][0].c_str());
				unsigned int homeID = boost::lexical_cast<unsigned int>(result[0][1]);
				int nodeID = atoi(result[0][2].c_str());
				CDomoticzHardwareBase *pHardware = m_mainworker.GetHardware(hwid);
				if (pHardware != NULL)
				{
					COpenZWave *pOZWHardware = (COpenZWave*)pHardware;
					if (!pOZWHardware->GetNodeUserCodes(homeID, nodeID, root))
						return;
					root["status"] = "OK";
					root["title"] = "GetUserCodes";
				}
			}
		}

#endif	//#ifdef WITH_OPENZWAVE

		void CWebServer::Cmd_GetLog(Json::Value &root)
		{
			root["status"] = "OK";
			root["title"] = "GetLog";

			time_t lastlogtime = 0;
			std::string slastlogtime = m_pWebEm->FindValue("lastlogtime");
			if (slastlogtime != "")
			{
				std::stringstream s_str(slastlogtime);
				s_str >> lastlogtime;
			}

			std::list<CLogger::_tLogLineStruct> logmessages = _log.GetLog();
			std::list<CLogger::_tLogLineStruct>::const_iterator itt;
			int ii = 0;
			for (itt = logmessages.begin(); itt != logmessages.end(); ++itt)
			{
				if (itt->logtime > lastlogtime)
				{
					std::stringstream szLogTime;
					szLogTime << itt->logtime;
					root["LastLogTime"] = szLogTime.str();
					root["result"][ii]["level"] = static_cast<int>(itt->level);
					root["result"][ii]["message"] = itt->logmessage;
					ii++;
				}
			}
		}

		//Plan Functions
		void CWebServer::Cmd_AddPlan(Json::Value &root)
		{
			if (m_pWebEm->m_actualuser_rights != 2)
			{
				//No admin user, and not allowed to be here
				return;
			}

			std::string name = m_pWebEm->FindValue("name");
			root["status"] = "OK";
			root["title"] = "AddPlan";
			char szTmp[100];
			sprintf(szTmp,
				"INSERT INTO Plans (Name) VALUES ('%s')",
				name.c_str()
				);
			m_sql.query(szTmp);
		}

		void CWebServer::Cmd_UpdatePlan(Json::Value &root)
		{
			if (m_pWebEm->m_actualuser_rights != 2)
			{
				//No admin user, and not allowed to be here
				return;
			}

			std::string idx = m_pWebEm->FindValue("idx");
			if (idx == "")
				return;
			std::string name = m_pWebEm->FindValue("name");
			if (
				(name == "")
				)
				return;

			root["status"] = "OK";
			root["title"] = "UpdatePlan";

			char szTmp[100];
			sprintf(szTmp,
				"UPDATE Plans SET Name='%s' WHERE (ID == %s)",
				name.c_str(),
				idx.c_str()
				);
			m_sql.query(szTmp);
		}

		void CWebServer::Cmd_DeletePlan(Json::Value &root)
		{
			if (m_pWebEm->m_actualuser_rights != 2)
			{
				//No admin user, and not allowed to be here
				return;
			}

			std::string idx = m_pWebEm->FindValue("idx");
			if (idx == "")
				return;
			root["status"] = "OK";
			root["title"] = "DeletePlan";
			char szTmp[100];
			sprintf(szTmp,
				"DELETE FROM DeviceToPlansMap WHERE (PlanID == %s)",
				idx.c_str()
				);
			m_sql.query(szTmp);
			sprintf(szTmp,
				"DELETE FROM Plans WHERE (ID == %s)",
				idx.c_str()
				);
			m_sql.query(szTmp);
		}

		void CWebServer::Cmd_GetUnusedPlanDevices(Json::Value &root)
		{
			root["status"] = "OK";
			root["title"] = "GetUnusedPlanDevices";
			std::string sunique = m_pWebEm->FindValue("unique");
			if (sunique == "")
				return;
			int iUnique = (sunique == "true") ? 1 : 0;
			int ii = 0;

			std::stringstream szQuery;
			std::vector<std::vector<std::string> > result;
			std::vector<std::vector<std::string> > result2;
			szQuery << "SELECT T1.[ID], T1.[Name], T1.[Type], T1.[SubType], T2.[Name] AS HardwareName FROM DeviceStatus as T1, Hardware as T2 WHERE (T1.[Used]==1) AND (T2.[ID]==T1.[HardwareID]) ORDER BY T2.[Name], T1.[Name]";
			result = m_sql.query(szQuery.str());
			if (result.size() > 0)
			{
				std::vector<std::vector<std::string> >::const_iterator itt;
				for (itt = result.begin(); itt != result.end(); ++itt)
				{
					std::vector<std::string> sd = *itt;

					bool bDoAdd = true;
					if (iUnique)
					{
						szQuery.clear();
						szQuery.str("");
						szQuery << "SELECT ID FROM DeviceToPlansMap WHERE (DeviceRowID==" << sd[0] << ") AND (DevSceneType==0)";
						result2 = m_sql.query(szQuery.str());
						bDoAdd = (result2.size() == 0);
					}
					if (bDoAdd)
					{
						int _dtype = atoi(sd[2].c_str());
						std::string Name = "[" + sd[4] + "] " + sd[1] + " (" + RFX_Type_Desc(_dtype, 1) + "/" + RFX_Type_SubType_Desc(_dtype, atoi(sd[3].c_str())) + ")";
						root["result"][ii]["type"] = 0;
						root["result"][ii]["idx"] = sd[0];
						root["result"][ii]["Name"] = Name;
						ii++;
					}
				}
			}
			//Add Scenes
			szQuery.clear();
			szQuery.str("");
			szQuery << "SELECT ID, Name FROM Scenes ORDER BY Name";
			result = m_sql.query(szQuery.str());
			if (result.size() > 0)
			{
				std::vector<std::vector<std::string> >::const_iterator itt;
				for (itt = result.begin(); itt != result.end(); ++itt)
				{
					std::vector<std::string> sd = *itt;

					bool bDoAdd = true;
					if (iUnique)
					{
						szQuery.clear();
						szQuery.str("");
						szQuery << "SELECT ID FROM DeviceToPlansMap WHERE (DeviceRowID==" << sd[0] << ") AND (DevSceneType==1)";
						result2 = m_sql.query(szQuery.str());
						bDoAdd = (result2.size() == 0);
					}
					if (bDoAdd)
					{
						root["result"][ii]["type"] = 1;
						root["result"][ii]["idx"] = sd[0];
						root["result"][ii]["Name"] = "[Scene] " + sd[1];
						ii++;
					}
				}
			}
		}

		void CWebServer::Cmd_AddPlanActiveDevice(Json::Value &root)
		{
			if (m_pWebEm->m_actualuser_rights != 2)
			{
				//No admin user, and not allowed to be here
				return;
			}

			std::string idx = m_pWebEm->FindValue("idx");
			std::string sactivetype = m_pWebEm->FindValue("activetype");
			std::string activeidx = m_pWebEm->FindValue("activeidx");
			if (
				(idx == "") ||
				(sactivetype == "") ||
				(activeidx == "")
				)
				return;
			root["status"] = "OK";
			root["title"] = "AddPlanActiveDevice";

			int activetype = atoi(sactivetype.c_str());

			//check if it is not already there
			std::stringstream szQuery;
			std::vector<std::vector<std::string> > result;
			szQuery << "SELECT ID FROM DeviceToPlansMap WHERE (DeviceRowID==" << activeidx << ") AND (DevSceneType==" << activetype << ") AND (PlanID==" << idx << ")";
			result = m_sql.query(szQuery.str());
			if (result.size() == 0)
			{
				char szTmp[100];
				sprintf(szTmp,
					"INSERT INTO DeviceToPlansMap (DevSceneType,DeviceRowID, PlanID) VALUES (%d,%s,%s)",
					activetype,
					activeidx.c_str(),
					idx.c_str()
					);
				m_sql.query(szTmp);
			}
		}

		void CWebServer::Cmd_GetPlanDevices(Json::Value &root)
		{
			std::string idx = m_pWebEm->FindValue("idx");
			if (idx == "")
				return;
			root["status"] = "OK";
			root["title"] = "GetPlanDevices";

			std::stringstream szQuery;
			std::vector<std::vector<std::string> > result;
			szQuery << "SELECT ID, DevSceneType, DeviceRowID, [Order] FROM DeviceToPlansMap WHERE (PlanID=='" << idx << "') ORDER BY [Order]";
			result = m_sql.query(szQuery.str());
			if (result.size() > 0)
			{
				std::vector<std::vector<std::string> >::const_iterator itt;
				int ii = 0;
				for (itt = result.begin(); itt != result.end(); ++itt)
				{
					std::vector<std::string> sd = *itt;

					std::string ID = sd[0];
					int DevSceneType = atoi(sd[1].c_str());
					std::string DevSceneRowID = sd[2];

					std::string Name = "";
					if (DevSceneType == 0)
					{
						std::vector<std::vector<std::string> > result2;
						std::stringstream szQuery2;
						szQuery2 << "SELECT Name FROM DeviceStatus WHERE (ID=='" << DevSceneRowID << "')";
						result2 = m_sql.query(szQuery2.str());
						if (result2.size() > 0)
						{
							Name = result2[0][0];
						}
					}
					else
					{
						std::vector<std::vector<std::string> > result2;
						std::stringstream szQuery2;
						szQuery2 << "SELECT Name FROM Scenes WHERE (ID=='" << DevSceneRowID << "')";
						result2 = m_sql.query(szQuery2.str());
						if (result2.size() > 0)
						{
							Name = "[Scene] " + result2[0][0];
						}
					}
					if (Name != "")
					{
						root["result"][ii]["idx"] = ID;
						root["result"][ii]["devidx"] = DevSceneRowID;
						root["result"][ii]["type"] = DevSceneType;
						root["result"][ii]["DevSceneRowID"] = DevSceneRowID;
						root["result"][ii]["order"] = sd[3];
						root["result"][ii]["Name"] = Name;
						ii++;
					}
				}
			}
		}

		void CWebServer::Cmd_DeletePlanDevice(Json::Value &root)
		{
			if (m_pWebEm->m_actualuser_rights != 2)
			{
				//No admin user, and not allowed to be here
				return;
			}

			std::string idx = m_pWebEm->FindValue("idx");
			if (idx == "")
				return;
			root["status"] = "OK";
			root["title"] = "DeletePlanDevice";
			char szTmp[100];
			sprintf(szTmp, "DELETE FROM DeviceToPlansMap WHERE (ID == '%s')", idx.c_str());
			m_sql.query(szTmp);
		}

		void CWebServer::Cmd_SetPlanDeviceCoords(Json::Value &root)
		{
			std::string idx = m_pWebEm->FindValue("idx");
			std::string planidx = m_pWebEm->FindValue("planidx");
			std::string xoffset = m_pWebEm->FindValue("xoffset");
			std::string yoffset = m_pWebEm->FindValue("yoffset");
			std::string type = m_pWebEm->FindValue("DevSceneType");
			if ((idx == "") || (planidx == "") || (xoffset == "") || (yoffset == ""))
				return;
			if (type != "1") type = "0";  // 0 = Device, 1 = Scene/Group
			root["status"] = "OK";
			root["title"] = "SetPlanDeviceCoords";
			std::stringstream szQuery;
			szQuery << "UPDATE DeviceToPlansMap SET [XOffset] = " << xoffset << ", [YOffset] = " << yoffset << " WHERE (DeviceRowID='" << idx << "') and (PlanID='" << planidx << "') and (DevSceneType='" << type << "')";
			m_sql.query(szQuery.str());
			_log.Log(LOG_STATUS, "(Floorplan) Device '%s' coordinates set to '%s,%s' in plan '%s'.", idx.c_str(), xoffset.c_str(), yoffset.c_str(), planidx.c_str());
		}

		void CWebServer::Cmd_DeleteAllPlanDevices(Json::Value &root)
		{
			if (m_pWebEm->m_actualuser_rights != 2)
			{
				//No admin user, and not allowed to be here
				return;
			}

			std::string idx = m_pWebEm->FindValue("idx");
			if (idx == "")
				return;
			root["status"] = "OK";
			root["title"] = "DeleteAllPlanDevices";
			char szTmp[100];
			sprintf(szTmp, "DELETE FROM DeviceToPlansMap WHERE (PlanID == '%s')", idx.c_str());
			m_sql.query(szTmp);
		}

		void CWebServer::Cmd_ChangePlanOrder(Json::Value &root)
		{
			std::string idx = m_pWebEm->FindValue("idx");
			if (idx == "")
				return;
			std::string sway = m_pWebEm->FindValue("way");
			if (sway == "")
				return;
			bool bGoUp = (sway == "0");

			std::string aOrder, oID, oOrder;

			std::stringstream szQuery;
			std::vector<std::vector<std::string> > result;
			szQuery << "SELECT [Order] FROM Plans WHERE (ID=='" << idx << "')";
			result = m_sql.query(szQuery.str());
			if (result.size() < 1)
				return;
			aOrder = result[0][0];

			szQuery.clear();
			szQuery.str("");

			if (!bGoUp)
			{
				//Get next device order
				szQuery << "SELECT ID, [Order] FROM Plans WHERE ([Order]>'" << aOrder << "') ORDER BY [Order] ASC";
				result = m_sql.query(szQuery.str());
				if (result.size() < 1)
					return;
				oID = result[0][0];
				oOrder = result[0][1];
			}
			else
			{
				//Get previous device order
				szQuery << "SELECT ID, [Order] FROM Plans WHERE ([Order]<'" << aOrder << "') ORDER BY [Order] DESC";
				result = m_sql.query(szQuery.str());
				if (result.size() < 1)
					return;
				oID = result[0][0];
				oOrder = result[0][1];
			}
			//Swap them
			root["status"] = "OK";
			root["title"] = "ChangePlanOrder";

			szQuery.clear();
			szQuery.str("");
			szQuery << "UPDATE Plans SET [Order] = '" << oOrder << "' WHERE (ID='" << idx << "')";
			result = m_sql.query(szQuery.str());
			szQuery.clear();
			szQuery.str("");
			szQuery << "UPDATE Plans SET [Order] = '" << aOrder << "' WHERE (ID='" << oID << "')";
			result = m_sql.query(szQuery.str());
		}

		void CWebServer::Cmd_ChangePlanDeviceOrder(Json::Value &root)
		{
			std::string planid = m_pWebEm->FindValue("planid");
			std::string idx = m_pWebEm->FindValue("idx");
			std::string sway = m_pWebEm->FindValue("way");
			if (
				(planid == "") ||
				(idx == "") ||
				(sway == "")
				)
				return;
			bool bGoUp = (sway == "0");

			std::string aOrder, oID, oOrder;

			std::stringstream szQuery;
			std::vector<std::vector<std::string> > result;
			szQuery << "SELECT [Order] FROM DeviceToPlansMap WHERE ((ID=='" << idx << "') AND (PlanID=='" << planid << "'))";
			result = m_sql.query(szQuery.str());
			if (result.size() < 1)
				return;
			aOrder = result[0][0];

			szQuery.clear();
			szQuery.str("");

			if (!bGoUp)
			{
				//Get next device order
				szQuery << "SELECT ID, [Order] FROM DeviceToPlansMap WHERE (([Order]>'" << aOrder << "') AND (PlanID=='" << planid << "')) ORDER BY [Order] ASC";
				result = m_sql.query(szQuery.str());
				if (result.size() < 1)
					return;
				oID = result[0][0];
				oOrder = result[0][1];
			}
			else
			{
				//Get previous device order
				szQuery << "SELECT ID, [Order] FROM DeviceToPlansMap WHERE (([Order]<'" << aOrder << "') AND (PlanID=='" << planid << "')) ORDER BY [Order] DESC";
				result = m_sql.query(szQuery.str());
				if (result.size() < 1)
					return;
				oID = result[0][0];
				oOrder = result[0][1];
			}
			//Swap them
			root["status"] = "OK";
			root["title"] = "ChangePlanOrder";

			szQuery.clear();
			szQuery.str("");
			szQuery << "UPDATE DeviceToPlansMap SET [Order] = '" << oOrder << "' WHERE (ID='" << idx << "')";
			result = m_sql.query(szQuery.str());
			szQuery.clear();
			szQuery.str("");
			szQuery << "UPDATE DeviceToPlansMap SET [Order] = '" << aOrder << "' WHERE (ID='" << oID << "')";
			result = m_sql.query(szQuery.str());
		}

		void CWebServer::Cmd_GetVersion(Json::Value &root)
		{
			root["status"] = "OK";
			root["title"] = "GetVersion";
			root["version"] = szAppVersion;

			//Force Check for update
			/*
			Json::Value root2;
			Cmd_CheckForUpdate(root2);
			*/

			int nValue = 1;
			m_sql.GetPreferencesVar("UseAutoUpdate", nValue);
			root["haveupdate"] = (nValue==1)?m_bHaveUpdate:false;
			root["revision"] = m_iRevision;


		}

		void CWebServer::Cmd_GetAuth(Json::Value &root)
		{
			root["status"] = "OK";
			root["title"] = "GetAuth";
			root["user"] = m_pWebEm->m_actualuser;
			root["rights"] = m_pWebEm->m_actualuser_rights;

			int nValue = 0;
		}

		void CWebServer::Cmd_GetActualHistory(Json::Value &root)
		{
			root["status"] = "OK";
			root["title"] = "GetActualHistory";

			std::string historyfile = szUserDataFolder + "History.txt";

			std::ifstream infile;
			int ii = 0;
			infile.open(historyfile.c_str());
			std::string sLine;
			if (infile.is_open())
			{
				while (!infile.eof())
				{
					getline(infile, sLine);
					root["LastLogTime"] = "";
					if (sLine.find("Version ") == 0)
						root["result"][ii]["level"] = 1;
					else
						root["result"][ii]["level"] = 0;
					root["result"][ii]["message"] = sLine;
					ii++;
				}
			}
		}

		void CWebServer::Cmd_GetNewHistory(Json::Value &root)
		{
			root["status"] = "OK";
			root["title"] = "GetNewHistory";

			std::string historyfile;
			int nValue;
			m_sql.GetPreferencesVar("ReleaseChannel", nValue);
			bool bIsBetaChannel = (nValue != 0);

			std::string szHistoryURL = "http://www.domoticz.com/download.php?channel=stable&type=history";
			if (bIsBetaChannel)
			{
				utsname my_uname;
				if (uname(&my_uname) < 0)
					return;

				std::string forced = m_pWebEm->FindValue("forced");
				bool bIsForced = (forced == "true");
				std::string systemname = my_uname.sysname;
				std::string machine = my_uname.machine;
				std::transform(systemname.begin(), systemname.end(), systemname.begin(), ::tolower);

				if (machine == "armv6l")
				{
					//Seems like old arm systems can also use the new arm build
					machine = "armv7l";
				}

				if (((machine != "armv6l") && (machine != "armv7l") && (machine != "x86_64")) || (strstr(my_uname.release, "ARCH+") != NULL))
					szHistoryURL = "http://www.domoticz.com/download.php?channel=beta&type=history";
				else
					szHistoryURL = "http://www.domoticz.com/download.php?channel=beta&type=history&system=" + systemname + "&machine=" + machine;
			}
			if (!HTTPClient::GET(szHistoryURL, historyfile))
			{
				historyfile = "Unable to get Online History document !!";
			}

			std::istringstream stream(historyfile);
			std::string sLine;
			int ii = 0;
			while (std::getline(stream, sLine))
			{
				root["LastLogTime"] = "";
				if (sLine.find("Version ") == 0)
					root["result"][ii]["level"] = 1;
				else
					root["result"][ii]["level"] = 0;
				root["result"][ii]["message"] = sLine;
				ii++;
			}
		}

		void CWebServer::Cmd_GetConfig(Json::Value &root)
		{
			root["status"] = "OK";
			root["title"] = "GetConfig";

			bool bHaveUser = (m_pWebEm->m_actualuser != "");
			int urights = 3;
			unsigned long UserID = 0;
			if (bHaveUser)
			{
				int iUser = -1;
				iUser = FindUser(m_pWebEm->m_actualuser.c_str());
				if (iUser != -1)
				{
					urights = static_cast<int>(m_users[iUser].userrights);
					UserID = m_users[iUser].ID;
				}

			}
			root["statuscode"] = urights;

			int nValue;
			std::string sValue;

			if (m_sql.GetPreferencesVar("Language", sValue))
			{
				root["language"] = sValue;
			}

			nValue = 0;
			int iDashboardType = 0;
			m_sql.GetPreferencesVar("DashboardType", iDashboardType);
			root["DashboardType"] = iDashboardType;
			m_sql.GetPreferencesVar("MobileType", nValue);
			root["MobileType"] = nValue;

			nValue = 1;
			m_sql.GetPreferencesVar("5MinuteHistoryDays", nValue);
			root["FiveMinuteHistoryDays"] = nValue;

			root["AllowWidgetOrdering"] = m_sql.m_bAllowWidgetOrdering;

			root["WindScale"] = m_sql.m_windscale*10.0f;
			root["WindSign"] = m_sql.m_windsign;
			root["TempScale"] = m_sql.m_tempscale;
			root["TempSign"] = m_sql.m_tempsign;

			std::string Latitude = "1";
			std::string Longitude = "1";
			if (m_sql.GetPreferencesVar("Location", nValue, sValue))
			{
				std::vector<std::string> strarray;
				StringSplit(sValue, ";", strarray);

				if (strarray.size() == 2)
				{
					Latitude = strarray[0];
					Longitude = strarray[1];
				}
			}
			root["Latitude"] = Latitude;
			root["Longitude"] = Longitude;

			int bEnableTabDashboard = 1;
			int bEnableTabFloorplans = 1;
			int bEnableTabLight = 1;
			int bEnableTabScenes = 1;
			int bEnableTabTemp = 1;
			int bEnableTabWeather = 1;
			int bEnableTabUtility = 1;
			int bEnableTabCustom = 1;

			std::stringstream szQuery;
			std::vector<std::vector<std::string> > result;

			if ((UserID != 0) && (UserID != 10000))
			{
				szQuery << "SELECT TabsEnabled FROM Users WHERE (ID==" << UserID << ")";
				result = m_sql.query(szQuery.str());
				if (result.size() > 0)
				{
					int TabsEnabled = atoi(result[0][0].c_str());
					bEnableTabLight = (TabsEnabled&(1 << 0));
					bEnableTabScenes = (TabsEnabled&(1 << 1));
					bEnableTabTemp = (TabsEnabled&(1 << 2));
					bEnableTabWeather = (TabsEnabled&(1 << 3));
					bEnableTabUtility = (TabsEnabled&(1 << 4));
					bEnableTabCustom = (TabsEnabled&(1 << 5));
					bEnableTabFloorplans = (TabsEnabled&(1 << 6));
				}
			}
			else
			{
				m_sql.GetPreferencesVar("EnableTabFloorplans", bEnableTabFloorplans);
				m_sql.GetPreferencesVar("EnableTabLights", bEnableTabLight);
				m_sql.GetPreferencesVar("EnableTabScenes", bEnableTabScenes);
				m_sql.GetPreferencesVar("EnableTabTemp", bEnableTabTemp);
				m_sql.GetPreferencesVar("EnableTabWeather", bEnableTabWeather);
				m_sql.GetPreferencesVar("EnableTabUtility", bEnableTabUtility);
				m_sql.GetPreferencesVar("EnableTabCustom", bEnableTabCustom);
			}
			if (iDashboardType == 3)
			{
				//Floorplan , no need to show a tab floorplan
				bEnableTabFloorplans = 0;
			}
			root["result"]["EnableTabDashboard"] = bEnableTabDashboard != 0;
			root["result"]["EnableTabFloorplans"] = bEnableTabFloorplans != 0;
			root["result"]["EnableTabLights"] = bEnableTabLight != 0;
			root["result"]["EnableTabScenes"] = bEnableTabScenes != 0;
			root["result"]["EnableTabTemp"] = bEnableTabTemp != 0;
			root["result"]["EnableTabWeather"] = bEnableTabWeather != 0;
			root["result"]["EnableTabUtility"] = bEnableTabUtility != 0;
			root["result"]["EnableTabCustom"] = bEnableTabCustom != 0;

			if (bEnableTabCustom)
			{
				//Add custom templates
				DIR *lDir;
				struct dirent *ent;
				std::string templatesFolder = szWWWFolder + "/templates";
				int iFile = 0;
				if ((lDir = opendir(templatesFolder.c_str())) != NULL)
				{
					while ((ent = readdir(lDir)) != NULL)
					{
						std::string filename = ent->d_name;
						size_t pos = filename.find(".htm");
						if (pos != std::string::npos)
						{
							std::string shortfile = filename.substr(0, pos);
							root["result"]["templates"][iFile++] = shortfile;
						}
					}
					closedir(lDir);
				}
			}
		}

		void CWebServer::Cmd_SendNotification(Json::Value &root)
		{
			std::string subject = m_pWebEm->FindValue("subject");
			std::string body = m_pWebEm->FindValue("body");
			if (
				(subject == "") ||
				(body == "")
				)
				return;
			//Add to queue
			if (m_notifications.SendMessage(NOTIFYALL, subject, body, std::string(""), false)) {
				root["status"] = "OK";
			}
			root["title"] = "SendNotification";
		}

		void CWebServer::Cmd_EmailCameraSnapshot(Json::Value &root)
		{
			std::string camidx = m_pWebEm->FindValue("camidx");
			std::string subject = m_pWebEm->FindValue("subject");
			if (
				(camidx == "") ||
				(subject == "")
				)
				return;
			//Add to queue
			m_sql.AddTaskItem(_tTaskItem::EmailCameraSnapshot(1, camidx, subject));
			root["status"] = "OK";
			root["title"] = "Email Camera Snapshot";
		}

		void CWebServer::Cmd_UpdateDevice(Json::Value &root)
		{
			std::string nvalue = m_pWebEm->FindValue("nvalue");
			std::string svalue = m_pWebEm->FindValue("svalue");

			std::string idx = m_pWebEm->FindValue("idx");

			std::string hid = m_pWebEm->FindValue("hid");
			std::string did = m_pWebEm->FindValue("did");
			std::string dunit = m_pWebEm->FindValue("dunit");
			std::string dtype = m_pWebEm->FindValue("dtype");
			std::string dsubtype = m_pWebEm->FindValue("dsubtype");

			int signallevel = 12;
			int batterylevel = 255;

			std::string sSignalLevel = m_pWebEm->FindValue("rssi");
			if (sSignalLevel != "")
			{
				signallevel = atoi(sSignalLevel.c_str());
			}
			std::string sBatteryLevel = m_pWebEm->FindValue("battery");
			if (sBatteryLevel != "")
			{
				batterylevel = atoi(sBatteryLevel.c_str());
			}

			std::stringstream szQuery;
			std::vector<std::vector<std::string> > result;

			if (idx != "")
			{
				//Get device parameters
				szQuery << "SELECT HardwareID, DeviceID, Unit, Type, SubType FROM DeviceStatus WHERE (ID==" << idx << ")";
				result = m_sql.query(szQuery.str());
				if (result.size() > 0)
				{
					hid = result[0][0];
					did = result[0][1];
					dunit = result[0][2];
					dtype = result[0][3];
					dsubtype = result[0][4];
				}
			}

			if (
				(hid == "") ||
				(did == "") ||
				(dunit == "") ||
				(dtype == "") ||
				(dsubtype == "")
				)
				return;
			if ((nvalue == "") && (svalue == ""))
				return;

			int iHWID = atoi(hid.c_str());
			int iUnit = atoi(dunit.c_str());
			int iType = atoi(dtype.c_str());
			int iSubType = atoi(dsubtype.c_str());
			int inValue = atoi(nvalue.c_str());

			root["status"] = "OK";
			root["title"] = "Update Device";

			if (iType == pTypeLighting2)
			{
				CDomoticzHardwareBase *pHardware = m_mainworker.GetHardware(iHWID);
				if (pHardware)
				{
					//Send as Lighting 2
					unsigned long ID;
					std::stringstream s_strid;
					s_strid << std::hex << did;
					s_strid >> ID;
					unsigned char ID1 = (unsigned char)((ID & 0xFF000000) >> 24);
					unsigned char ID2 = (unsigned char)((ID & 0x00FF0000) >> 16);
					unsigned char ID3 = (unsigned char)((ID & 0x0000FF00) >> 8);
					unsigned char ID4 = (unsigned char)((ID & 0x000000FF));

					tRBUF lcmd;
					memset(&lcmd, 0, sizeof(RBUF));
					lcmd.LIGHTING2.packetlength = sizeof(lcmd.LIGHTING2) - 1;
					lcmd.LIGHTING2.packettype = pTypeLighting2;
					lcmd.LIGHTING2.subtype = iSubType;
					lcmd.LIGHTING2.id1 = ID1;
					lcmd.LIGHTING2.id2 = ID2;
					lcmd.LIGHTING2.id3 = ID3;
					lcmd.LIGHTING2.id4 = ID4;
					lcmd.LIGHTING2.unitcode = (unsigned char)iUnit;
					lcmd.LIGHTING2.cmnd = (unsigned char)inValue;
					lcmd.LIGHTING2.level = (unsigned char)atoi(svalue.c_str());
					lcmd.LIGHTING2.filler = 0;
					lcmd.LIGHTING2.rssi = signallevel;
					m_mainworker.DecodeRXMessage(pHardware, (const unsigned char *)&lcmd.LIGHTING2);
					return;
				}
			}

			std::string devname = "Unknown";
			m_sql.UpdateValue(
				iHWID,
				did.c_str(),
				(const unsigned char)iUnit,
				(const unsigned char)iType,
				(const unsigned char)iSubType,
				signallevel,//signal level,
				batterylevel,//battery level
				(const int)inValue,
				svalue.c_str(),
				devname,
				false
				);

			if (
				((iType == pTypeThermostat) && (iSubType == sTypeThermSetpoint))||
				((iType == pTypeRadiator1) && (iSubType== sTypeSmartwares))
				)
			{
				int urights = 3;
				bool bHaveUser = (m_pWebEm->m_actualuser != "");
				if (bHaveUser)
				{
					int iUser = -1;
					iUser = FindUser(m_pWebEm->m_actualuser.c_str());
					if (iUser != -1)
					{
						urights = static_cast<int>(m_users[iUser].userrights);
						_log.Log(LOG_STATUS, "User: %s initiated a SetPoint command", m_users[iUser].Username.c_str());
					}
				}
				if (urights < 1)
					return;
				_log.Log(LOG_NORM, "Sending SetPoint to device....");
				m_mainworker.SetSetPoint(idx, static_cast<float>(atof(svalue.c_str())));
			}
			else if ((iType == pTypeGeneral) && (iSubType == sTypeZWaveThermostatMode))
			{
				int urights = 3;
				bool bHaveUser = (m_pWebEm->m_actualuser != "");
				if (bHaveUser)
				{
					int iUser = -1;
					iUser = FindUser(m_pWebEm->m_actualuser.c_str());
					if (iUser != -1)
					{
						urights = static_cast<int>(m_users[iUser].userrights);
						_log.Log(LOG_STATUS, "User: %s initiated a Thermostat Mode command", m_users[iUser].Username.c_str());
					}
				}
				if (urights < 1)
					return;
				_log.Log(LOG_NORM, "Sending Thermostat Mode to device....");
				m_mainworker.SetZWaveThermostatMode(idx, atoi(nvalue.c_str()));
			}
			else if ((iType == pTypeGeneral) && (iSubType == sTypeZWaveThermostatFanMode))
			{
				int urights = 3;
				bool bHaveUser = (m_pWebEm->m_actualuser != "");
				if (bHaveUser)
				{
					int iUser = -1;
					iUser = FindUser(m_pWebEm->m_actualuser.c_str());
					if (iUser != -1)
					{
						urights = static_cast<int>(m_users[iUser].userrights);
						_log.Log(LOG_STATUS, "User: %s initiated a Thermostat Fan Mode command", m_users[iUser].Username.c_str());
					}
				}
				if (urights < 1)
					return;
				_log.Log(LOG_NORM, "Sending Thermostat Fan Mode to device....");
				m_mainworker.SetZWaveThermostatFanMode(idx, atoi(nvalue.c_str()));
			}
		}

		void CWebServer::Cmd_SetThermostatState(Json::Value &root)
		{
			std::string sstate = m_pWebEm->FindValue("state");
			std::string idx = m_pWebEm->FindValue("idx");
			if (
				(idx == "") ||
				(sstate == "")
				)
				return;
			int iState = atoi(sstate.c_str());

			int urights = 3;
			bool bHaveUser = (m_pWebEm->m_actualuser != "");
			if (bHaveUser)
			{
				int iUser = -1;
				iUser = FindUser(m_pWebEm->m_actualuser.c_str());
				if (iUser != -1)
				{
					urights = static_cast<int>(m_users[iUser].userrights);
					_log.Log(LOG_STATUS, "User: %s initiated a Thermostat State change command", m_users[iUser].Username.c_str());
				}
			}
			if (urights < 1)
				return;

			root["status"] = "OK";
			root["title"] = "Set Thermostat State";
			_log.Log(LOG_NORM, "Setting Thermostat State....");
			m_mainworker.SetThermostatState(idx, iState);
		}

		void CWebServer::Cmd_SystemShutdown(Json::Value &root)
		{
#ifdef WIN32
			int ret = system("shutdown -s -f -t 1 -d up:125:1");
#else
			int ret = system("sudo shutdown -h now");
#endif
			if (ret != 0)
			{
				_log.Log(LOG_ERROR, "Error executing shutdown command. returned: %d", ret);
				return;
			}
			root["title"] = "SystemShutdown";
			root["status"] = "OK";
		}

		void CWebServer::Cmd_SystemReboot(Json::Value &root)
		{
#ifdef WIN32
			int ret = system("shutdown -r -f -t 1 -d up:125:1");
#else
			int ret = system("sudo shutdown -r now");
#endif
			if (ret != 0)
			{
				_log.Log(LOG_ERROR, "Error executing reboot command. returned: %d", ret);
				return;
			}
			root["title"] = "SystemReboot";
			root["status"] = "OK";
		}

		void CWebServer::Cmd_ExcecuteScript(Json::Value &root)
		{
			std::string scriptname = m_pWebEm->FindValue("scriptname");
			if (scriptname == "")
				return;
			if (scriptname.find("..") != std::string::npos)
				return;
#ifdef WIN32
			scriptname = szUserDataFolder + "scripts\\" + scriptname;
#else
			scriptname = szUserDataFolder + "scripts/" + scriptname;
#endif
			if (!file_exist(scriptname.c_str()))
				return;
			std::string script_params = m_pWebEm->FindValue("scriptparams");
			std::string strparm = szUserDataFolder;
			if (script_params != "")
			{
				if (strparm.size() > 0)
					strparm += " " + script_params;
				else
					strparm = script_params;
			}
			std::string sdirect = m_pWebEm->FindValue("direct");
			if (sdirect == "true")
			{
				_log.Log(LOG_STATUS, "Executing script: %s", scriptname.c_str());
#ifdef WIN32
				ShellExecute(NULL, "open", scriptname.c_str(), strparm.c_str(), NULL, SW_SHOWNORMAL);
#else
				std::string lscript = scriptname + " " + strparm;
				int ret = system(lscript.c_str());
				if (ret != 0)
				{
					_log.Log(LOG_ERROR, "Error executing script command (%s). returned: %d", lscript.c_str(), ret);
					return;
				}
#endif
			}
			else
			{
				//add script to background worker
				m_sql.AddTaskItem(_tTaskItem::ExecuteScript(1, scriptname, strparm));
			}
			root["title"] = "ExecuteScript";
			root["status"] = "OK";
		}

		void CWebServer::Cmd_GetCosts(Json::Value &root)
		{
			std::string idx = m_pWebEm->FindValue("idx");
			if (idx == "")
				return;
			char szTmp[100];
			std::stringstream szQuery;
			std::vector<std::vector<std::string> > result;
			szQuery << "SELECT Type, SubType, nValue, sValue FROM DeviceStatus WHERE (ID==" << idx << ")";
			result = m_sql.query(szQuery.str());
			if (result.size() > 0)
			{
				std::vector<std::string> sd = result[0];

				int nValue = 0;
				root["status"] = "OK";
				root["title"] = "GetElectraCosts";
				m_sql.GetPreferencesVar("CostEnergy", nValue);
				root["CostEnergy"] = nValue;
				m_sql.GetPreferencesVar("CostEnergyT2", nValue);
				root["CostEnergyT2"] = nValue;
				m_sql.GetPreferencesVar("CostGas", nValue);
				root["CostGas"] = nValue;
				m_sql.GetPreferencesVar("CostWater", nValue);
				root["CostWater"] = nValue;

				unsigned char dType = atoi(sd[0].c_str());
				unsigned char subType = atoi(sd[1].c_str());
				nValue = (unsigned char)atoi(sd[2].c_str());
				std::string sValue = sd[3];

				if (dType == pTypeP1Power)
				{
					//also provide the counter values

					std::vector<std::string> splitresults;
					StringSplit(sValue, ";", splitresults);
					if (splitresults.size() != 6)
						return;

					float EnergyDivider = 1000.0f;
					int tValue;
					if (m_sql.GetPreferencesVar("MeterDividerEnergy", tValue))
					{
						EnergyDivider = float(tValue);
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

					sprintf(szTmp, "%.03f", float(powerusage1) / EnergyDivider);
					root["CounterT1"] = szTmp;
					sprintf(szTmp, "%.03f", float(powerusage2) / EnergyDivider);
					root["CounterT2"] = szTmp;
					sprintf(szTmp, "%.03f", float(powerdeliv1) / EnergyDivider);
					root["CounterR1"] = szTmp;
					sprintf(szTmp, "%.03f", float(powerdeliv2) / EnergyDivider);
					root["CounterR2"] = szTmp;
				}
			}
		}

		void CWebServer::Cmd_CheckForUpdate(Json::Value &root)
		{
			bool bHaveUser = (m_pWebEm->m_actualuser != "");
			int urights = 3;
			if (bHaveUser)
			{
				int iUser = -1;
				iUser = FindUser(m_pWebEm->m_actualuser.c_str());
				if (iUser != -1)
					urights = static_cast<int>(m_users[iUser].userrights);
			}
			root["statuscode"] = urights;

			utsname my_uname;
			if (uname(&my_uname) < 0)
				return;

			std::string forced = m_pWebEm->FindValue("forced");
			bool bIsForced = (forced == "true");

			std::string systemname = my_uname.sysname;
			std::string machine = my_uname.machine;
			std::transform(systemname.begin(), systemname.end(), systemname.begin(), ::tolower);

			if (machine == "armv6l")
			{
				//Seems like old arm systems can also use the new arm build
				machine = "armv7l";
			}

#ifdef DEBUG_DOWNLOAD
			systemname = "linux";
			machine = "armv7l";
#endif

			if ((systemname == "windows") || ((machine != "armv6l") && (machine != "armv7l") && (machine != "x86_64")))
			{
				//Only Raspberry Pi (Wheezy)/Ubuntu for now!
				root["status"] = "OK";
				root["title"] = "CheckForUpdate";
				root["HaveUpdate"] = false;
				root["IsSupported"] = false;
			}
			else
			{
				time_t atime = mytime(NULL);
				if (!bIsForced)
				{
					if (atime - m_LastUpdateCheck < 12 * 3600)
					{
						root["status"] = "OK";
						root["title"] = "CheckForUpdate";
						root["HaveUpdate"] = false;
						root["IsSupported"] = true;
						return;
					}
				}
				m_LastUpdateCheck = atime;

				int nValue = 0;
				m_sql.GetPreferencesVar("UseAutoUpdate", nValue);
				if (nValue == 1)
				{
					m_sql.GetPreferencesVar("ReleaseChannel", nValue);
					bool bIsBetaChannel = (nValue != 0);
					std::string szURL = "http://www.domoticz.com/download.php?channel=stable&type=version&system=" + systemname + "&machine=" + machine;
					std::string szHistoryURL = "http://www.domoticz.com/download.php?channel=stable&type=history&system=" + systemname + "&machine=" + machine;
					if (bIsBetaChannel)
					{
						szURL = "http://www.domoticz.com/download.php?channel=beta&type=version&system=" + systemname + "&machine=" + machine;
						szHistoryURL = "http://www.domoticz.com/download.php?channel=beta&type=history&system=" + systemname + "&machine=" + machine;
					}
					std::string revfile;

					if (!HTTPClient::GET(szURL, revfile))
						return;
					std::vector<std::string> strarray;
					StringSplit(revfile, " ", strarray);
					if (strarray.size() != 3)
						return;
					root["status"] = "OK";
					root["title"] = "CheckForUpdate";
					root["IsSupported"] = true;

					int version = atoi(szAppVersion.substr(szAppVersion.find(".") + 1).c_str());
					m_iRevision = atoi(strarray[2].c_str());
					m_bHaveUpdate = (version < m_iRevision);
#ifdef DEBUG_DOWNLOAD
					m_bHaveUpdate = true;
					bIsForced = true;
#endif
					root["HaveUpdate"] = m_bHaveUpdate;
					root["Revision"] = m_iRevision;
					root["HistoryURL"] = szHistoryURL;
					root["ActVersion"] = version;
				}
				else
				{
					root["status"] = "OK";
					root["title"] = "CheckForUpdate";
					root["IsSupported"] = false;
					root["HaveUpdate"] = false;
				}
			}
		}

		void CWebServer::Cmd_DownloadUpdate(Json::Value &root)
		{
			int nValue;
			m_sql.GetPreferencesVar("ReleaseChannel", nValue);
			bool bIsBetaChannel = (nValue != 0);
			std::string szURL;
			std::string revfile;

			utsname my_uname;
			if (uname(&my_uname) < 0)
				return;

			std::string forced = m_pWebEm->FindValue("forced");
			bool bIsForced = (forced == "true");
			std::string systemname = my_uname.sysname;
			std::string machine = my_uname.machine;
			std::transform(systemname.begin(), systemname.end(), systemname.begin(), ::tolower);

			if (machine == "armv6l")
			{
				//Seems like old arm systems can also use the new arm build
				machine = "armv7l";
			}

#ifdef DEBUG_DOWNLOAD
			systemname = "linux";
			machine = "armv7l";
#endif

			if (!bIsBetaChannel)
			{
				szURL = "http://www.domoticz.com/download.php?channel=stable&type=version&system=" + systemname + "&machine=" + machine;
			}
			else
			{
				szURL = "http://www.domoticz.com/download.php?channel=beta&type=version&system=" + systemname + "&machine=" + machine;
			}
			if (!HTTPClient::GET(szURL, revfile))
				return;
			std::vector<std::string> strarray;
			StringSplit(revfile, " ", strarray);
			if (strarray.size() != 3)
				return;
			int version = atoi(szAppVersion.substr(szAppVersion.find(".") + 1).c_str());
			if (version >= atoi(strarray[2].c_str()))
			{
#ifndef DEBUG_DOWNLOAD
				return;
#endif
			}

			if (((machine != "armv6l") && (machine != "armv7l") && (machine != "x86_64")) || (strstr(my_uname.release, "ARCH+") != NULL))
				return;	//only Raspberry Pi/Ubuntu for now
			root["status"] = "OK";
			root["title"] = "DownloadUpdate";
			std::string downloadURL;
			std::string checksumURL;
			if (!bIsBetaChannel)
			{
				downloadURL = "http://www.domoticz.com/download.php?channel=stable&type=release&system=" + systemname + "&machine=" + machine;
				checksumURL = "http://www.domoticz.com/download.php?channel=stable&type=checksum&system=" + systemname + "&machine=" + machine;
			}
			else
			{
				downloadURL = "http://www.domoticz.com/download.php?channel=beta&type=release&system=" + systemname + "&machine=" + machine;
				checksumURL = "http://www.domoticz.com/download.php?channel=beta&type=checksum&system=" + systemname + "&machine=" + machine;
			}
			m_mainworker.GetDomoticzUpdate(downloadURL, checksumURL);
		}

		void CWebServer::Cmd_DownloadReady(Json::Value &root)
		{
			if (!m_mainworker.m_bHaveDownloadedDomoticzUpdate)
				return;
			root["status"] = "OK";
			root["title"] = "DownloadReady";
			root["downloadok"] = (m_mainworker.m_bHaveDownloadedDomoticzUpdateSuccessFull) ? true : false;
		}

		void CWebServer::Cmd_DeleteDatePoint(Json::Value &root)
		{
			const std::string idx = m_pWebEm->FindValue("idx");
			const std::string Date = m_pWebEm->FindValue("date");
			if (
				(idx == "") ||
				(Date == "")
				)
				return;
			root["status"] = "OK";
			root["title"] = "deletedatapoint";
			m_sql.DeleteDataPoint(idx.c_str(), Date);
		}

		void CWebServer::HandleCommand(const std::string &cparam, Json::Value &root)
		{
			std::map < std::string, webserver_response_function >::iterator pf = m_webcommands.find(cparam);
			if (pf != m_webcommands.end())
			{
				pf->second(root);
				return;
			}

			std::stringstream szQuery;
			std::vector<std::vector<std::string> > result;
			std::vector<std::vector<std::string> > result2;
			char szTmp[300];

			bool bHaveUser = (m_pWebEm->m_actualuser != "");
			int iUser = -1;
			if (bHaveUser)
			{
				iUser = FindUser(m_pWebEm->m_actualuser.c_str());
			}

			if (cparam == "deleteallsubdevices")
			{
				std::string idx = m_pWebEm->FindValue("idx");
				if (idx == "")
					return;
				root["status"] = "OK";
				root["title"] = "DeleteAllSubDevices";
				sprintf(szTmp, "DELETE FROM LightSubDevices WHERE (ParentID == %s)", idx.c_str());
				result = m_sql.query(szTmp);
			}
			else if (cparam == "deletesubdevice")
			{
				std::string idx = m_pWebEm->FindValue("idx");
				if (idx == "")
					return;
				root["status"] = "OK";
				root["title"] = "DeleteSubDevice";
				sprintf(szTmp, "DELETE FROM LightSubDevices WHERE (ID == %s)", idx.c_str());
				result = m_sql.query(szTmp);
			}
			else if (cparam == "addsubdevice")
			{
				std::string idx = m_pWebEm->FindValue("idx");
				std::string subidx = m_pWebEm->FindValue("subidx");
				if ((idx == "") || (subidx == ""))
					return;
				if (idx == subidx)
					return;

				//first check if it is not already a sub device
				szQuery.clear();
				szQuery.str("");
				szQuery << "SELECT ID FROM LightSubDevices WHERE (DeviceRowID=='" << subidx << "') AND (ParentID =='" << idx << "')";
				result = m_sql.query(szQuery.str());
				if (result.size() == 0)
				{
					root["status"] = "OK";
					root["title"] = "AddSubDevice";
					//no it is not, add it
					sprintf(szTmp,
						"INSERT INTO LightSubDevices (DeviceRowID, ParentID) VALUES ('%s','%s')",
						subidx.c_str(),
						idx.c_str()
						);
					result = m_sql.query(szTmp);
				}
			}
			else if (cparam == "addscenedevice")
			{
				std::string idx = m_pWebEm->FindValue("idx");
				std::string devidx = m_pWebEm->FindValue("devidx");
				std::string isscene = m_pWebEm->FindValue("isscene");
				int command = atoi(m_pWebEm->FindValue("command").c_str());
				int ondelay = atoi(m_pWebEm->FindValue("ondelay").c_str());
				int offdelay = atoi(m_pWebEm->FindValue("offdelay").c_str());

				if (
					(idx == "") ||
					(devidx == "") ||
					(isscene == "")
					)
					return;
				int level = atoi(m_pWebEm->FindValue("level").c_str());
				int hue = atoi(m_pWebEm->FindValue("hue").c_str());
				//first check if this device is not the scene code!
				szQuery.clear();
				szQuery.str("");
				szQuery << "SELECT HardwareID, DeviceID, Unit, Type, SubType FROM DeviceStatus WHERE (ID==" << devidx << ")";
				result = m_sql.query(szQuery.str());
				if (result.size() > 0)
				{
					szQuery.clear();
					szQuery.str("");
					szQuery << "SELECT HardwareID, DeviceID, Unit, Type, SubType FROM Scenes WHERE (ID==" << idx << ")";
					result2 = m_sql.query(szQuery.str());
					if (result2.size() > 0)
					{
						if (
							(result[0][0] == result2[0][0]) &&
							(result[0][1] == result2[0][1]) &&
							(result[0][2] == result2[0][2]) &&
							(result[0][3] == result2[0][3]) &&
							(result[0][4] == result2[0][4])
							)
						{
							//This is not allowed!
							return;
						}
					}
				}
				//first check if it is not already a part of this scene/group (with the same OnDelay)
				szQuery.clear();
				szQuery.str("");
				szQuery << "SELECT ID FROM SceneDevices WHERE (DeviceRowID=='" << devidx << "') AND (SceneRowID =='" << idx << "') AND (OnDelay == " << ondelay << ")";
				result = m_sql.query(szQuery.str());
				if (result.size() == 0)
				{
					root["status"] = "OK";
					root["title"] = "AddSceneDevice";
					//no it is not, add it
					if (isscene == "true")
					{
						sprintf(szTmp,
							"INSERT INTO SceneDevices (DeviceRowID, SceneRowID, Cmd, Level, Hue, OnDelay, OffDelay) VALUES ('%s','%s',%d,%d,%d,%d,%d)",
							devidx.c_str(),
							idx.c_str(),
							command,
							level,
							hue,
							ondelay,
							offdelay
							);
					}
					else
					{
						sprintf(szTmp,
							"INSERT INTO SceneDevices (DeviceRowID, SceneRowID, Level, Hue, OnDelay, OffDelay) VALUES ('%s','%s',%d,%d,%d,%d)",
							devidx.c_str(),
							idx.c_str(),
							level,
							hue,
							ondelay,
							offdelay
							);
					}
					result = m_sql.query(szTmp);
				}
			}
			else if (cparam == "updatescenedevice")
			{
				std::string idx = m_pWebEm->FindValue("idx");
				std::string devidx = m_pWebEm->FindValue("devidx");
				std::string isscene = m_pWebEm->FindValue("isscene");
				int command = atoi(m_pWebEm->FindValue("command").c_str());
				int ondelay = atoi(m_pWebEm->FindValue("ondelay").c_str());
				int offdelay = atoi(m_pWebEm->FindValue("offdelay").c_str());

				if (
					(idx == "") ||
					(devidx == "")
					)
					return;
				int level = atoi(m_pWebEm->FindValue("level").c_str());
				int hue = atoi(m_pWebEm->FindValue("hue").c_str());
				root["status"] = "OK";
				root["title"] = "UpdateSceneDevice";
				szQuery.clear();
				szQuery.str("");
				szQuery << "UPDATE SceneDevices SET Cmd=" << command << ", Level=" << level << ", Hue=" << hue << ", OnDelay=" << ondelay << ", OffDelay=" << offdelay << "  WHERE (ID == " << idx << ")";
				result = m_sql.query(szQuery.str());
			}
			else if (cparam == "deletescenedevice")
			{
				std::string idx = m_pWebEm->FindValue("idx");
				if (idx == "")
					return;
				root["status"] = "OK";
				root["title"] = "DeleteSceneDevice";
				sprintf(szTmp, "DELETE FROM SceneDevices WHERE (ID == %s)", idx.c_str());
				result = m_sql.query(szTmp);
				sprintf(szTmp, "DELETE FROM CamerasActiveDevices WHERE (DevSceneType==1) AND (DevSceneRowID == %s)", idx.c_str());
				result = m_sql.query(szTmp);

			}
			else if (cparam == "getsubdevices")
			{
				std::string idx = m_pWebEm->FindValue("idx");
				if (idx == "")
					return;

				root["status"] = "OK";
				root["title"] = "GetSubDevices";
				std::vector<std::vector<std::string> > result;
				std::stringstream szQuery;
				szQuery << "SELECT a.ID, b.Name FROM LightSubDevices a, DeviceStatus b WHERE (a.ParentID=='" << idx << "') AND (b.ID == a.DeviceRowID)";
				result = m_sql.query(szQuery.str());
				if (result.size() > 0)
				{
					std::vector<std::vector<std::string> >::const_iterator itt;
					int ii = 0;
					for (itt = result.begin(); itt != result.end(); ++itt)
					{
						std::vector<std::string> sd = *itt;

						root["result"][ii]["ID"] = sd[0];
						root["result"][ii]["Name"] = sd[1];
						ii++;
					}
				}
			}
			else if (cparam == "gettimerlist")
			{
				root["status"] = "OK";
				root["title"] = "GetTimerList";
				std::vector<std::vector<std::string> > result;
				std::stringstream szQuery;
				szQuery << "SELECT t.ID, t.Active, d.[Name], t.DeviceRowID, t.[Date], t.Time, t.Type, t.Cmd, t.Level, t.Days FROM Timers as t, DeviceStatus as d WHERE (d.ID == t.DeviceRowID) AND (t.TimerPlan==" << m_sql.m_ActiveTimerPlan << ") ORDER BY d.[Name], t.Time";
				result = m_sql.query(szQuery.str());
				if (result.size() > 0)
				{
					std::vector<std::vector<std::string> >::const_iterator itt;
					int ii = 0;
					for (itt = result.begin(); itt != result.end(); ++itt)
					{
						std::vector<std::string> sd = *itt;

						root["result"][ii]["ID"] = sd[0];
						root["result"][ii]["Active"] = sd[1];
						root["result"][ii]["Name"] = sd[2];
						root["result"][ii]["DeviceRowID"] = sd[3];
						root["result"][ii]["Date"] = sd[4];
						root["result"][ii]["Time"] = sd[5];
						root["result"][ii]["Type"] = sd[6];
						root["result"][ii]["Cmd"] = sd[7];
						root["result"][ii]["Level"] = sd[8];
						root["result"][ii]["Days"] = sd[9];
						ii++;
					}
				}
			}
			else if (cparam == "getscenedevices")
			{
				std::string idx = m_pWebEm->FindValue("idx");
				std::string isscene = m_pWebEm->FindValue("isscene");

				if (
					(idx == "") ||
					(isscene == "")
					)
					return;

				root["status"] = "OK";
				root["title"] = "GetSceneDevices";

				std::vector<std::vector<std::string> > result;
				std::stringstream szQuery;
				szQuery << "SELECT a.ID, b.Name, a.DeviceRowID, b.Type, b.SubType, b.nValue, b.sValue, a.Cmd, a.Level, b.ID, a.[Order], a.Hue, a.OnDelay, a.OffDelay FROM SceneDevices a, DeviceStatus b WHERE (a.SceneRowID=='" << idx << "') AND (b.ID == a.DeviceRowID) ORDER BY a.[Order]";
				result = m_sql.query(szQuery.str());
				if (result.size() > 0)
				{
					std::vector<std::vector<std::string> >::const_iterator itt;
					int ii = 0;
					for (itt = result.begin(); itt != result.end(); ++itt)
					{
						std::vector<std::string> sd = *itt;

						root["result"][ii]["ID"] = sd[0];
						root["result"][ii]["Name"] = sd[1];
						root["result"][ii]["DevID"] = sd[2];
						root["result"][ii]["DevRealIdx"] = sd[9];
						root["result"][ii]["Order"] = atoi(sd[10].c_str());
						root["result"][ii]["OnDelay"] = atoi(sd[12].c_str());
						root["result"][ii]["OffDelay"] = atoi(sd[13].c_str());

						unsigned char devType = atoi(sd[3].c_str());
						unsigned char subType = atoi(sd[4].c_str());
						unsigned char nValue = (unsigned char)atoi(sd[5].c_str());
						std::string sValue = sd[6];
						int command = atoi(sd[7].c_str());
						int level = atoi(sd[8].c_str());

						std::string lstatus = "";
						int llevel = 0;
						bool bHaveDimmer = false;
						bool bHaveGroupCmd = false;
						int maxDimLevel = 0;
						if (isscene == "true")
							GetLightStatus(devType, subType, STYPE_OnOff, command, sValue, lstatus, llevel, bHaveDimmer, maxDimLevel, bHaveGroupCmd);
						else
							GetLightStatus(devType, subType, STYPE_OnOff, nValue, sValue, lstatus, llevel, bHaveDimmer, maxDimLevel, bHaveGroupCmd);
						root["result"][ii]["IsOn"] = IsLightSwitchOn(lstatus);
						root["result"][ii]["Level"] = level;
						root["result"][ii]["Hue"] = atoi(sd[11].c_str());
						root["result"][ii]["Type"] = RFX_Type_Desc(devType, 1);
						root["result"][ii]["SubType"] = RFX_Type_SubType_Desc(devType, subType);
						ii++;
					}
				}
			}
			else if (cparam == "changescenedeviceorder")
			{
				std::string idx = m_pWebEm->FindValue("idx");
				if (idx == "")
					return;
				std::string sway = m_pWebEm->FindValue("way");
				if (sway == "")
					return;
				bool bGoUp = (sway == "0");

				std::string aScene, aOrder, oID, oOrder;

				//Get actual device order
				std::vector<std::vector<std::string> > result;
				std::stringstream szQuery;
				szQuery << "SELECT SceneRowID, [Order] FROM SceneDevices WHERE (ID=='" << idx << "')";
				result = m_sql.query(szQuery.str());
				if (result.size() < 1)
					return;
				aScene = result[0][0];
				aOrder = result[0][1];

				szQuery.clear();
				szQuery.str("");

				if (!bGoUp)
				{
					//Get next device order
					szQuery << "SELECT ID, [Order] FROM SceneDevices WHERE (SceneRowID=='" << aScene << "' AND [Order]>'" << aOrder << "') ORDER BY [Order] ASC";
					result = m_sql.query(szQuery.str());
					if (result.size() < 1)
						return;
					oID = result[0][0];
					oOrder = result[0][1];
				}
				else
				{
					//Get previous device order
					szQuery << "SELECT ID, [Order] FROM SceneDevices WHERE (SceneRowID=='" << aScene << "' AND [Order]<'" << aOrder << "') ORDER BY [Order] DESC";
					result = m_sql.query(szQuery.str());
					if (result.size() < 1)
						return;
					oID = result[0][0];
					oOrder = result[0][1];
				}
				//Swap them
				root["status"] = "OK";
				root["title"] = "ChangeSceneDeviceOrder";

				szQuery.clear();
				szQuery.str("");
				szQuery << "UPDATE SceneDevices SET [Order] = '" << oOrder << "' WHERE (ID='" << idx << "')";
				result = m_sql.query(szQuery.str());
				szQuery.clear();
				szQuery.str("");
				szQuery << "UPDATE SceneDevices SET [Order] = '" << aOrder << "' WHERE (ID='" << oID << "')";
				result = m_sql.query(szQuery.str());
			}
			else if (cparam == "deleteallscenedevices")
			{
				std::string idx = m_pWebEm->FindValue("idx");
				if (idx == "")
					return;
				root["status"] = "OK";
				root["title"] = "DeleteAllSceneDevices";
				sprintf(szTmp, "DELETE FROM SceneDevices WHERE (SceneRowID == %s)", idx.c_str());
				result = m_sql.query(szTmp);
			}
			else if (cparam == "getmanualhardware")
			{
				//used by Add Manual Light/Switch dialog
				root["status"] = "OK";
				root["title"] = "GetHardware";
				sprintf(szTmp, "SELECT ID, Name, Type FROM Hardware ORDER BY ID ASC");
				result = m_sql.query(szTmp);
				if (result.size() > 0)
				{
					std::vector<std::vector<std::string> >::const_iterator itt;
					int ii = 0;
					for (itt = result.begin(); itt != result.end(); ++itt)
					{
						std::vector<std::string> sd = *itt;

						int ID = atoi(sd[0].c_str());
						std::string Name = sd[1];
						_eHardwareTypes Type = (_eHardwareTypes)atoi(sd[2].c_str());
						switch (Type)
						{
						case HTYPE_RFXLAN:
						case HTYPE_RFXtrx315:
						case HTYPE_RFXtrx433:
						case HTYPE_EnOceanESP2:
						case HTYPE_EnOceanESP3:
						case HTYPE_Dummy:
						case HTYPE_EVOHOME_SCRIPT:
						case HTYPE_EVOHOME_SERIAL:
						case HTYPE_RaspberryGPIO:
						case HTYPE_RFLINK:
                        root["result"][ii]["idx"] = ID;
							root["result"][ii]["Name"] = Name;
							ii++;
							break;
						}
					}
				}
			}
			else if (cparam == "getgpio")
			{
				//used by Add Manual Light/Switch dialog
				root["title"] = "GetGpio";
#ifdef WITH_GPIO
				std::vector<CGpioPin> pins = CGpio::GetPinList();
				if (pins.size() == 0) {
					root["status"] = "ERROR";
					root["result"][0]["idx"] = 0;
					root["result"][0]["Name"] = "GPIO INIT ERROR";
				}
				else {
					int ii = 0;
					for (std::vector<CGpioPin>::iterator it = pins.begin(); it != pins.end(); ++it) {
						CGpioPin pin = *it;
						root["status"] = "OK";
						root["result"][ii]["idx"] = pin.GetId();
						root["result"][ii]["Name"] = pin.ToString();
						ii++;
					}
				}
#else
				root["status"] = "OK";
				root["result"][0]["idx"] = 0;
				root["result"][0]["Name"] = "N/A";
#endif
			}
			else if (cparam == "getlightswitches")
			{
				root["status"] = "OK";
				root["title"] = "GetLightSwitches";
				std::vector<std::vector<std::string> > result;
				std::stringstream szQuery;
				szQuery << "SELECT ID, Name, Type, SubType, Used, SwitchType FROM DeviceStatus ORDER BY Name";
				result = m_sql.query(szQuery.str());
				if (result.size() > 0)
				{
					std::vector<std::vector<std::string> >::const_iterator itt;
					int ii = 0;
					for (itt = result.begin(); itt != result.end(); ++itt)
					{
						std::vector<std::string> sd = *itt;

						std::string ID = sd[0];
						std::string Name = sd[1];
						int Type = atoi(sd[2].c_str());
						int SubType = atoi(sd[3].c_str());
						int used = atoi(sd[4].c_str());
						_eSwitchType switchtype = (_eSwitchType)atoi(sd[5].c_str());
						bool bdoAdd=false;
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
						case pTypeSecurity2:
						case pTypeEvohome:
						case pTypeEvohomeRelay:	
						case pTypeCurtain:
						case pTypeBlinds:
						case pTypeRFY:
						case pTypeChime:
						case pTypeThermostat2:
						case pTypeThermostat3:
						case pTypeRemote:
						case pTypeRadiator1:
						case pTypeGeneralSwitch:
							bdoAdd = true;
							if (!used)
							{
								bdoAdd = false;
								bool bIsSubDevice = false;
								std::vector<std::vector<std::string> > resultSD;
								std::stringstream szQuerySD;
								szQuerySD << "SELECT ID FROM LightSubDevices WHERE (DeviceRowID=='" << sd[0] << "')";
								resultSD = m_sql.query(szQuerySD.str());
								if (resultSD.size() > 0)
									bdoAdd = true;
							}
							if ((Type == pTypeRadiator1) && (SubType != sTypeSmartwaresSwitchRadiator))
								bdoAdd = false;
							if (bdoAdd)
							{
								root["result"][ii]["idx"] = ID;
								root["result"][ii]["Name"] = Name;
								root["result"][ii]["Type"] = RFX_Type_Desc(Type, 1);
								root["result"][ii]["SubType"] = RFX_Type_SubType_Desc(Type, SubType);
								bool bIsDimmer = (
									(switchtype == STYPE_Dimmer) ||
									(switchtype == STYPE_BlindsPercentage) ||
									(switchtype == STYPE_BlindsPercentageInverted)
									);
								root["result"][ii]["IsDimmer"] = bIsDimmer;
								ii++;
							}
							break;
						}
					}
				}
			}
			else if (cparam == "getlightswitchesscenes")
			{
				root["status"] = "OK";
				root["title"] = "GetLightSwitchesScenes";
				std::vector<std::vector<std::string> > result;
				std::stringstream szQuery;
				int ii = 0;

				//First List/Switch Devices
				szQuery << "SELECT ID, Name, Type, SubType, Used FROM DeviceStatus ORDER BY Name";
				result = m_sql.query(szQuery.str());
				if (result.size() > 0)
				{
					std::vector<std::vector<std::string> >::const_iterator itt;
					for (itt = result.begin(); itt != result.end(); ++itt)
					{
						std::vector<std::string> sd = *itt;

						std::string ID = sd[0];
						std::string Name = sd[1];
						int Type = atoi(sd[2].c_str());
						int SubType = atoi(sd[3].c_str());
						int used = atoi(sd[4].c_str());
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
							case pTypeSecurity2:
							case pTypeEvohome:
							case pTypeEvohomeRelay:
							case pTypeCurtain:
							case pTypeBlinds:
							case pTypeRFY:
							case pTypeChime:
							case pTypeThermostat2:
							case pTypeThermostat3:
							case pTypeRemote:
							case pTypeGeneralSwitch:
								root["result"][ii]["type"] = 0;
								root["result"][ii]["idx"] = ID;
								root["result"][ii]["Name"] = "[Light/Switch] " + Name;
								ii++;
								break;
							case pTypeRadiator1:
								if (SubType == sTypeSmartwaresSwitchRadiator)
								{
									root["result"][ii]["type"] = 0;
									root["result"][ii]["idx"] = ID;
									root["result"][ii]["Name"] = "[Light/Switch] " + Name;
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
				result = m_sql.query(szQuery.str());
				if (result.size() > 0)
				{
					std::vector<std::vector<std::string> >::const_iterator itt;
					for (itt = result.begin(); itt != result.end(); ++itt)
					{
						std::vector<std::string> sd = *itt;

						std::string ID = sd[0];
						std::string Name = sd[1];

						root["result"][ii]["type"] = 1;
						root["result"][ii]["idx"] = ID;
						root["result"][ii]["Name"] = "[Scene] " + Name;
						ii++;
					}
				}//end light/switches
			}
			else if (cparam == "getcamactivedevices")
			{
				std::string idx = m_pWebEm->FindValue("idx");
				if (idx == "")
					return;
				root["status"] = "OK";
				root["title"] = "GetCameraActiveDevices";
				std::vector<std::vector<std::string> > result;
				std::stringstream szQuery;
				//First List/Switch Devices
				szQuery << "SELECT ID, DevSceneType, DevSceneRowID, DevSceneWhen, DevSceneDelay FROM CamerasActiveDevices WHERE (CameraRowID=='" << idx << "') ORDER BY ID";
				result = m_sql.query(szQuery.str());
				if (result.size() > 0)
				{
					std::vector<std::vector<std::string> >::const_iterator itt;
					int ii = 0;
					for (itt = result.begin(); itt != result.end(); ++itt)
					{
						std::vector<std::string> sd = *itt;

						std::string ID = sd[0];
						int DevSceneType = atoi(sd[1].c_str());
						std::string DevSceneRowID = sd[2];
						int DevSceneWhen = atoi(sd[3].c_str());
						int DevSceneDelay = atoi(sd[4].c_str());

						std::string Name = "";
						if (DevSceneType == 0)
						{
							std::vector<std::vector<std::string> > result2;
							std::stringstream szQuery2;
							szQuery2 << "SELECT Name FROM DeviceStatus WHERE (ID=='" << DevSceneRowID << "')";
							result2 = m_sql.query(szQuery2.str());
							if (result2.size() > 0)
							{
								Name = "[Light/Switches] " + result2[0][0];
							}
						}
						else
						{
							std::vector<std::vector<std::string> > result2;
							std::stringstream szQuery2;
							szQuery2 << "SELECT Name FROM Scenes WHERE (ID=='" << DevSceneRowID << "')";
							result2 = m_sql.query(szQuery2.str());
							if (result2.size() > 0)
							{
								Name = "[Scene] " + result2[0][0];
							}
						}
						if (Name != "")
						{
							root["result"][ii]["idx"] = ID;
							root["result"][ii]["type"] = DevSceneType;
							root["result"][ii]["DevSceneRowID"] = DevSceneRowID;
							root["result"][ii]["when"] = DevSceneWhen;
							root["result"][ii]["delay"] = DevSceneDelay;
							root["result"][ii]["Name"] = Name;
							ii++;
						}
					}
				}
			}
			else if (cparam == "addcamactivedevice")
			{
				std::string idx = m_pWebEm->FindValue("idx");
				std::string activeidx = m_pWebEm->FindValue("activeidx");
				std::string sactivetype = m_pWebEm->FindValue("activetype");
				std::string sactivewhen = m_pWebEm->FindValue("activewhen");
				std::string sactivedelay = m_pWebEm->FindValue("activedelay");

				if (
					(idx == "") ||
					(activeidx == "") ||
					(sactivetype == "") ||
					(sactivewhen == "") ||
					(sactivedelay == "")
					)
				{
					return;
				}

				int activetype = atoi(sactivetype.c_str());
				int activewhen = atoi(sactivewhen.c_str());
				int activedelay = atoi(sactivedelay.c_str());

				//first check if it is not already a Active Device
				szQuery.clear();
				szQuery.str("");
				szQuery << "SELECT ID FROM CamerasActiveDevices WHERE (CameraRowID=='"
					<< idx << "') AND (DevSceneType=="
					<< activetype << ") AND (DevSceneRowID=='" << activeidx << "')  AND (DevSceneWhen==" << sactivewhen << ")";
				result = m_sql.query(szQuery.str());
				if (result.size() == 0)
				{
					root["status"] = "OK";
					root["title"] = "AddCameraActiveDevice";
					//no it is not, add it
					sprintf(szTmp,
						"INSERT INTO CamerasActiveDevices (CameraRowID, DevSceneType, DevSceneRowID, DevSceneWhen, DevSceneDelay) VALUES ('%s',%d,'%s',%d,%d)",
						idx.c_str(),
						activetype,
						activeidx.c_str(),
						activewhen,
						activedelay
						);
					result = m_sql.query(szTmp);
					m_mainworker.m_cameras.ReloadCameras();
				}
			}
			else if (cparam == "deleteamactivedevice")
			{
				std::string idx = m_pWebEm->FindValue("idx");
				if (idx == "")
					return;
				root["status"] = "OK";
				root["title"] = "DeleteCameraActiveDevice";
				sprintf(szTmp, "DELETE FROM CamerasActiveDevices WHERE (ID == '%s')", idx.c_str());
				result = m_sql.query(szTmp);
				m_mainworker.m_cameras.ReloadCameras();
			}
			else if (cparam == "deleteallactivecamdevices")
			{
				std::string idx = m_pWebEm->FindValue("idx");
				if (idx == "")
					return;
				root["status"] = "OK";
				root["title"] = "DeleteAllCameraActiveDevices";
				sprintf(szTmp, "DELETE FROM CamerasActiveDevices WHERE (CameraRowID == '%s')", idx.c_str());
				result = m_sql.query(szTmp);
				m_mainworker.m_cameras.ReloadCameras();
			}
			else if (cparam == "testnotification")
			{
				std::string notification_Title = "Domoticz test";
				std::string notification_Message = "Domoticz test message!";
				std::string subsystem = m_pWebEm->FindValue("subsystem");

				m_notifications.ConfigFromGetvars(m_pWebEm, false);
				if (m_notifications.SendMessage(subsystem, notification_Title, notification_Message, std::string(""), false)) {
					root["status"] = "OK";
				}
				/* we need to reload the config, because the values that were set were only for testing */
				m_notifications.LoadConfig();
			}
			else if (cparam == "testswitch")
			{
				std::string hwdid = m_pWebEm->FindValue("hwdid");
				std::string sswitchtype = m_pWebEm->FindValue("switchtype");
				std::string slighttype = m_pWebEm->FindValue("lighttype");
				if (
					(hwdid == "") ||
					(sswitchtype == "") ||
					(slighttype == "")
					)
					return;
				_eSwitchType switchtype = (_eSwitchType)atoi(sswitchtype.c_str());
				int lighttype = atoi(slighttype.c_str());
				int dtype;
				int subtype = 0;
				std::string sunitcode;
				std::string devid;

				if (lighttype == 70)
				{
					//EnOcean (Lighting2 with Base_ID offset)
					dtype = pTypeLighting2;
					subtype = sTypeAC;
					std::string sgroupcode = m_pWebEm->FindValue("groupcode");
					sunitcode = m_pWebEm->FindValue("unitcode");
					int iUnitTest = atoi(sunitcode.c_str());	//only First Rocker_ID at the moment, gives us 128 devices we can control, should be enough!
					if (
						(sunitcode == "") ||
						(sgroupcode == "") ||
						((iUnitTest < 1) || (iUnitTest>128))
						)
						return;
					sunitcode = sgroupcode;//Button A or B
					CDomoticzHardwareBase *pBaseHardware = (CDomoticzHardwareBase*)m_mainworker.GetHardware(atoi(hwdid.c_str()));
					if (pBaseHardware == NULL)
						return;
					if ((pBaseHardware->HwdType != HTYPE_EnOceanESP2) && (pBaseHardware->HwdType != HTYPE_EnOceanESP3))
						return;
					unsigned long rID = 0;
					if (pBaseHardware->HwdType == HTYPE_EnOceanESP2)
					{
						CEnOceanESP2 *pEnoceanHardware = (CEnOceanESP2 *)pBaseHardware;
						rID = pEnoceanHardware->m_id_base + iUnitTest;
					}
					else
					{
						CEnOceanESP3 *pEnoceanHardware = (CEnOceanESP3 *)pBaseHardware;
						rID = pEnoceanHardware->m_id_base + iUnitTest;
					}
					//convert to hex, and we have our ID
					std::stringstream s_strid;
					s_strid << std::hex << std::uppercase << rID;
					devid = s_strid.str();
				}
				else if (lighttype == 68)
				{
#ifdef WITH_GPIO
					dtype = pTypeLighting1;
					subtype = sTypeIMPULS;
					devid = "0";
					sunitcode = m_pWebEm->FindValue("unitcode"); //Unit code = GPIO number

					if (sunitcode == "") {
						root["status"] = "ERROR";
						root["message"] = "No GPIO number given";
						return;
					}
					CGpio *pGpio = (CGpio *)m_mainworker.GetHardware(atoi(hwdid.c_str()));
					if (pGpio == NULL) {
						root["status"] = "ERROR";
						root["message"] = "Could not retrieve GPIO hardware pointer";
						return;
					}
					if (pGpio->HwdType != HTYPE_RaspberryGPIO) {
						root["status"] = "ERROR";
						root["message"] = "Given hardware is not GPIO";
						return;
					}
					CGpioPin *pPin = CGpio::GetPPinById(atoi(sunitcode.c_str()));
					if (pPin == NULL) {
						root["status"] = "ERROR";
						root["message"] = "Given pin does not exist on this GPIO hardware";
						return;
					}
					if (!pPin->GetIsExported()) {
						root["status"] = "ERROR";
						root["message"] = "Given pin is not exported";
						return;
					}
					if (!pPin->GetIsOutput()) {
						root["status"] = "ERROR";
						root["message"] = "Given pin is not configured for output";
						return;
					}
#else
					root["status"] = "ERROR";
					root["message"] = "GPIO support is disabled";
					return;
#endif
				}
				else if (lighttype < 20)
				{
					dtype = pTypeLighting1;
					subtype = lighttype;
					std::string shousecode = m_pWebEm->FindValue("housecode");
					sunitcode = m_pWebEm->FindValue("unitcode");
					if (
						(shousecode == "") ||
						(sunitcode == "")
						)
						return;
					devid = shousecode;
				}
				else if (lighttype < 30)
				{
					dtype = pTypeLighting2;
					subtype = lighttype - 20;
					std::string id = m_pWebEm->FindValue("id");
					sunitcode = m_pWebEm->FindValue("unitcode");
					if (
						(id == "") ||
						(sunitcode == "")
						)
						return;
					devid = id;
				}
				else if (lighttype < 60)
				{
					dtype = pTypeLighting5;
					subtype = lighttype - 50;
					std::string id = m_pWebEm->FindValue("id");
					sunitcode = m_pWebEm->FindValue("unitcode");
					if (
						(id == "") ||
						(sunitcode == "")
						)
						return;
					devid = id;
				}
				else if (lighttype < 70)
				{
					//Blyss
					dtype = pTypeLighting6;
					subtype = lighttype - 60;
					std::string sgroupcode = m_pWebEm->FindValue("groupcode");
					sunitcode = m_pWebEm->FindValue("unitcode");
					std::string id = m_pWebEm->FindValue("id");
					if (
						(sgroupcode == "") ||
						(sunitcode == "") ||
						(id == "")
						)
						return;
					devid = id + sgroupcode;
				}
				else
				{
					if (lighttype == 100)
					{
						//Chime/ByronSX
						dtype = pTypeChime;
						subtype = sTypeByronSX;
						std::string id = m_pWebEm->FindValue("id");
						sunitcode = m_pWebEm->FindValue("unitcode");
						if (
							(id == "") ||
							(sunitcode == "")
							)
							return;
						int iUnitCode = atoi(sunitcode.c_str()) - 1;
						switch (iUnitCode)
						{
						case 0:
							iUnitCode = chime_sound0;
							break;
						case 1:
							iUnitCode = chime_sound1;
							break;
						case 2:
							iUnitCode = chime_sound2;
							break;
						case 3:
							iUnitCode = chime_sound3;
							break;
						case 4:
							iUnitCode = chime_sound4;
							break;
						case 5:
							iUnitCode = chime_sound5;
							break;
						case 6:
							iUnitCode = chime_sound6;
							break;
						case 7:
							iUnitCode = chime_sound7;
							break;
						}
						sprintf(szTmp, "%d", iUnitCode);
						sunitcode = szTmp;
						devid = id;
					}
					else if (lighttype == 101)
					{
						//Curtain Harrison
						dtype = pTypeCurtain;
						subtype = sTypeHarrison;
						std::string shousecode = m_pWebEm->FindValue("housecode");
						sunitcode = m_pWebEm->FindValue("unitcode");
						if (
							(shousecode == "") ||
							(sunitcode == "")
							)
							return;
						devid = shousecode;
					}
					else if (lighttype == 102)
					{
						//RFY
						dtype = pTypeRFY;
						subtype = sTypeRFY;
						std::string id = m_pWebEm->FindValue("id");
						sunitcode = m_pWebEm->FindValue("unitcode");
						if (
							(id == "") ||
							(sunitcode == "")
							)
							return;
						devid = id;
					}
					else if (lighttype == 103)
					{
						//Meiantech
						dtype = pTypeSecurity1;
						subtype = sTypeMeiantech;
						std::string id = m_pWebEm->FindValue("id");
						if (
							(id == "")
							)
							return;
						devid = id;
						sunitcode = "0";
					}
					else if (lighttype == 104)
					{
						//HE105
						dtype = pTypeThermostat2;
						subtype = sTypeHE105;
						sunitcode = m_pWebEm->FindValue("unitcode");
						if (sunitcode == "")
							return;
						//convert to hex, and we have our Unit Code
						std::stringstream s_strid;
						s_strid << std::hex << std::uppercase << sunitcode;
						int iUnitCode;
						s_strid >> iUnitCode;
						sprintf(szTmp, "%d", iUnitCode);
						sunitcode = szTmp;
						devid = "1";
					}
					else if ((lighttype >= 200) && (lighttype < 300))
					{
						dtype = pTypeBlinds;
						subtype = lighttype-200;
						std::string id = m_pWebEm->FindValue("id");
						sunitcode = m_pWebEm->FindValue("unitcode");
						if (
							(id == "") ||
							(sunitcode == "")
							)
							return;
						int iUnitCode = atoi(sunitcode.c_str());
						sprintf(szTmp, "%d", iUnitCode);
						sunitcode = szTmp;
						devid = id;
					}
					else if (lighttype == 301)
					{
						//Smartwares Radiator
						dtype = pTypeRadiator1;
						subtype = sTypeSmartwaresSwitchRadiator;
						std::string id = m_pWebEm->FindValue("id");
						sunitcode = m_pWebEm->FindValue("unitcode");
						if (
							(id == "") ||
							(sunitcode == "")
							)
							return;
						devid = id;
					}
				}
				root["status"] = "OK";
				root["message"] = "OK";
				root["title"] = "TestSwitch";
				std::vector<std::string> sd;

				sd.push_back(hwdid);
				sd.push_back(devid);
				sd.push_back(sunitcode);
				sprintf(szTmp, "%d", dtype);
				sd.push_back(szTmp);
				sprintf(szTmp, "%d", subtype);
				sd.push_back(szTmp);
				sprintf(szTmp, "%d", switchtype);
				sd.push_back(szTmp);
				sd.push_back(""); //StrParam1
				sd.push_back(""); //StrParam2

				std::string switchcmd = "On";
				int level = 0;
				if (lighttype == 70)
				{
					//Special EnOcean case, if it is a dimmer, set a dim value
					if (switchtype == STYPE_Dimmer)
						level = 5;
				}
				m_mainworker.SwitchLightInt(sd, switchcmd, level, -1, true);
			}
			else if (cparam == "addswitch")
			{
				std::string hwdid = m_pWebEm->FindValue("hwdid");
				std::string name = m_pWebEm->FindValue("name");
				std::string sswitchtype = m_pWebEm->FindValue("switchtype");
				std::string slighttype = m_pWebEm->FindValue("lighttype");
				std::string maindeviceidx = m_pWebEm->FindValue("maindeviceidx");

				if (
					(hwdid == "") ||
					(sswitchtype == "") ||
					(slighttype == "") ||
					(name == "")
					)
					return;
				_eSwitchType switchtype = (_eSwitchType)atoi(sswitchtype.c_str());
				int lighttype = atoi(slighttype.c_str());
				int dtype = 0;
				int subtype = 0;
				std::string sunitcode;
				std::string devid;

				if (lighttype == 70)
				{
					//EnOcean (Lighting2 with Base_ID offset)
					dtype = pTypeLighting2;
					subtype = sTypeAC;
					sunitcode = m_pWebEm->FindValue("unitcode");
					std::string sgroupcode = m_pWebEm->FindValue("groupcode");
					int iUnitTest = atoi(sunitcode.c_str());	//gives us 128 devices we can control, should be enough!
					if (
						(sunitcode == "") ||
						(sgroupcode == "") ||
						((iUnitTest < 1) || (iUnitTest>128))
						)
						return;
					sunitcode = sgroupcode;//Button A/B
					CDomoticzHardwareBase *pBaseHardware = (CDomoticzHardwareBase*)m_mainworker.GetHardware(atoi(hwdid.c_str()));
					if (pBaseHardware == NULL)
						return;
					if ((pBaseHardware->HwdType != HTYPE_EnOceanESP2) && (pBaseHardware->HwdType != HTYPE_EnOceanESP3))
						return;
					unsigned long rID = 0;
					if (pBaseHardware->HwdType == HTYPE_EnOceanESP2)
					{
						CEnOceanESP2 *pEnoceanHardware = (CEnOceanESP2*)pBaseHardware;
						if (pEnoceanHardware->m_id_base == 0)
						{
							root["message"] = "BaseID not found, is the hardware running?";
							return;
						}
						rID = pEnoceanHardware->m_id_base + iUnitTest;
					}
					else
					{
						CEnOceanESP3 *pEnoceanHardware = (CEnOceanESP3*)pBaseHardware;
						if (pEnoceanHardware->m_id_base == 0)
						{
							root["message"] = "BaseID not found, is the hardware running?";
							return;
						}
						rID = pEnoceanHardware->m_id_base + iUnitTest;
					}
					//convert to hex, and we have our ID
					std::stringstream s_strid;
					s_strid << std::hex << std::uppercase << rID;
					devid = s_strid.str();
				}
				else if (lighttype == 68)
				{
#ifdef WITH_GPIO
					dtype = pTypeLighting1;
					subtype = sTypeIMPULS;
					devid = "0";
					sunitcode = m_pWebEm->FindValue("unitcode"); //Unit code = GPIO number

					if (sunitcode == "") {
						return;
					}
					CGpio *pGpio = (CGpio *)m_mainworker.GetHardware(atoi(hwdid.c_str()));
					if (pGpio == NULL) {
						return;
					}
					if (pGpio->HwdType != HTYPE_RaspberryGPIO) {
						return;
					}
					CGpioPin *pPin = CGpio::GetPPinById(atoi(sunitcode.c_str()));
					if (pPin == NULL) {
						return;
					}
					if (!pPin->GetIsExported()) {
						return;
					}
#else
					return;
#endif
				}
				else if (lighttype < 20)
				{
					dtype = pTypeLighting1;
					subtype = lighttype;
					std::string shousecode = m_pWebEm->FindValue("housecode");
					sunitcode = m_pWebEm->FindValue("unitcode");
					if (
						(shousecode == "") ||
						(sunitcode == "")
						)
						return;
					devid = shousecode;
				}
				else if (lighttype < 30)
				{
					dtype = pTypeLighting2;
					subtype = lighttype - 20;
					std::string id = m_pWebEm->FindValue("id");
					sunitcode = m_pWebEm->FindValue("unitcode");
					if (
						(id == "") ||
						(sunitcode == "")
						)
						return;
					devid = id;
				}
				else if (lighttype < 60)
				{
					dtype = pTypeLighting5;
					subtype = lighttype - 50;
					std::string id = m_pWebEm->FindValue("id");
					sunitcode = m_pWebEm->FindValue("unitcode");
					if (
						(id == "") ||
						(sunitcode == "")
						)
						return;
					devid = id;
				}
				else if (lighttype < 70)
				{
					//Blyss
					dtype = pTypeLighting6;
					subtype = lighttype - 60;
					std::string sgroupcode = m_pWebEm->FindValue("groupcode");
					sunitcode = m_pWebEm->FindValue("unitcode");
					std::string id = m_pWebEm->FindValue("id");
					if (
						(sgroupcode == "") ||
						(sunitcode == "") ||
						(id == "")
						)
						return;
					devid = id + sgroupcode;
				}
				else if (lighttype == 101)
				{
					//Curtain Harrison
					dtype = pTypeCurtain;
					subtype = sTypeHarrison;
					std::string shousecode = m_pWebEm->FindValue("housecode");
					sunitcode = m_pWebEm->FindValue("unitcode");
					if (
						(shousecode == "") ||
						(sunitcode == "")
						)
						return;
					devid = shousecode;
				}
				else if (lighttype == 102)
				{
					//RFY
					dtype = pTypeRFY;
					subtype = sTypeRFY;
					std::string id = m_pWebEm->FindValue("id");
					sunitcode = m_pWebEm->FindValue("unitcode");
					if (
						(id == "") ||
						(sunitcode == "")
						)
						return;
					devid = id;
				}
				else if (lighttype == 103)
				{
					//Meiantech
					dtype = pTypeSecurity1;
					subtype = sTypeMeiantech;
					std::string id = m_pWebEm->FindValue("id");
					if (
						(id == "")
						)
						return;
					devid = id;
					sunitcode = "0";
				}
				else
				{
					if (lighttype == 100)
					{
						//Chime/ByronSX
						dtype = pTypeChime;
						subtype = sTypeByronSX;
						std::string id = m_pWebEm->FindValue("id");
						sunitcode = m_pWebEm->FindValue("unitcode");
						if (
							(id == "") ||
							(sunitcode == "")
							)
							return;
						int iUnitCode = atoi(sunitcode.c_str()) - 1;
						switch (iUnitCode)
						{
						case 0:
							iUnitCode = chime_sound0;
							break;
						case 1:
							iUnitCode = chime_sound1;
							break;
						case 2:
							iUnitCode = chime_sound2;
							break;
						case 3:
							iUnitCode = chime_sound3;
							break;
						case 4:
							iUnitCode = chime_sound4;
							break;
						case 5:
							iUnitCode = chime_sound5;
							break;
						case 6:
							iUnitCode = chime_sound6;
							break;
						case 7:
							iUnitCode = chime_sound7;
							break;
						}
						sprintf(szTmp, "%d", iUnitCode);
						sunitcode = szTmp;
						devid = id;
					}
					else if (lighttype == 104)
					{
						//HE105
						dtype = pTypeThermostat2;
						subtype = sTypeHE105;
						sunitcode = m_pWebEm->FindValue("unitcode");
						if (sunitcode == "")
							return;
						//convert to hex, and we have our Unit Code
						std::stringstream s_strid;
						s_strid << std::hex << std::uppercase << sunitcode;
						int iUnitCode;
						s_strid >> iUnitCode;
						sprintf(szTmp, "%d", iUnitCode);
						sunitcode = szTmp;
						devid = "1";
					}
					else if ((lighttype >= 200) && (lighttype < 300))
					{
						dtype = pTypeBlinds;
						subtype = lighttype-200;
						std::string id = m_pWebEm->FindValue("id");
						sunitcode = m_pWebEm->FindValue("unitcode");
						if (
							(id == "") ||
							(sunitcode == "")
							)
							return;
						int iUnitCode = atoi(sunitcode.c_str());
						sprintf(szTmp, "%d", iUnitCode);
						sunitcode = szTmp;
						devid = id;
					}
					else if (lighttype == 301)
					{
						//Smartwares Radiator
						std::string id = m_pWebEm->FindValue("id");
						sunitcode = m_pWebEm->FindValue("unitcode");
						if (
							(id == "") ||
							(sunitcode == "")
							)
							return;
						devid = id;

						//For this device, we will also need to add a Radiator type, do that first
						dtype = pTypeRadiator1;
						subtype = sTypeSmartwares;

						//check if switch is unique
						std::vector<std::vector<std::string> > result;
						std::stringstream szQuery;
						szQuery << "SELECT Name FROM DeviceStatus WHERE (HardwareID==" << hwdid << " AND DeviceID=='" << devid << "' AND Unit==" << sunitcode << " AND Type==" << dtype << " AND SubType==" << subtype << ")";
						result = m_sql.query(szQuery.str());
						if (result.size() > 0)
						{
							root["message"] = "Switch already exists!";
							return;
						}
						bool bActEnabledState = m_sql.m_bAcceptNewHardware;
						m_sql.m_bAcceptNewHardware = true;
						std::string devname;
						m_sql.UpdateValue(atoi(hwdid.c_str()), devid.c_str(), atoi(sunitcode.c_str()), dtype, subtype, 0, -1, 0, "20.5", devname);
						m_sql.m_bAcceptNewHardware = bActEnabledState;

						//set name and switchtype
						szQuery.clear();
						szQuery.str("");
						szQuery << "SELECT ID FROM DeviceStatus WHERE (HardwareID==" << hwdid << " AND DeviceID=='" << devid << "' AND Unit==" << sunitcode << " AND Type==" << dtype << " AND SubType==" << subtype << ")";
						result = m_sql.query(szQuery.str());
						if (result.size() < 1)
						{
							root["message"] = "Error finding switch in Database!?!?";
							return;
						}
						std::string ID = result[0][0];

						szQuery.clear();
						szQuery.str("");
						szQuery << "UPDATE DeviceStatus SET Used=1, Name='" << name << "', SwitchType=" << switchtype << " WHERE (ID == " << ID << ")";
						result = m_sql.query(szQuery.str());

						//Now continue to insert the switch
						dtype = pTypeRadiator1;
						subtype = sTypeSmartwaresSwitchRadiator;

					}
				}

				//check if switch is unique
				std::vector<std::vector<std::string> > result;
				std::stringstream szQuery;
				szQuery << "SELECT Name FROM DeviceStatus WHERE (HardwareID==" << hwdid << " AND DeviceID=='" << devid << "' AND Unit==" << sunitcode << " AND Type==" << dtype << " AND SubType==" << subtype << ")";
				result = m_sql.query(szQuery.str());
				if (result.size() > 0)
				{
					root["message"] = "Switch already exists!";
					return;
				}

				bool bActEnabledState = m_sql.m_bAcceptNewHardware;
				m_sql.m_bAcceptNewHardware = true;
				std::string devname;
				m_sql.UpdateValue(atoi(hwdid.c_str()), devid.c_str(), atoi(sunitcode.c_str()), dtype, subtype, 0, -1, 0, devname);
				m_sql.m_bAcceptNewHardware = bActEnabledState;

				//set name and switchtype
				szQuery.clear();
				szQuery.str("");
				szQuery << "SELECT ID FROM DeviceStatus WHERE (HardwareID==" << hwdid << " AND DeviceID=='" << devid << "' AND Unit==" << sunitcode << " AND Type==" << dtype << " AND SubType==" << subtype << ")";
				result = m_sql.query(szQuery.str());
				if (result.size() < 1)
				{
					root["message"] = "Error finding switch in Database!?!?";
					return;
				}
				std::string ID = result[0][0];

				szQuery.clear();
				szQuery.str("");
				szQuery << "UPDATE DeviceStatus SET Used=1, Name='" << name << "', SwitchType=" << switchtype << " WHERE (ID == " << ID << ")";
				result = m_sql.query(szQuery.str());

				if (maindeviceidx != "")
				{
					if (maindeviceidx != ID)
					{
						//this is a sub device for another light/switch
						//first check if it is not already a sub device
						szQuery.clear();
						szQuery.str("");
						szQuery << "SELECT ID FROM LightSubDevices WHERE (DeviceRowID=='" << ID << "') AND (ParentID =='" << maindeviceidx << "')";
						result = m_sql.query(szQuery.str());
						if (result.size() == 0)
						{
							//no it is not, add it
							sprintf(szTmp,
								"INSERT INTO LightSubDevices (DeviceRowID, ParentID) VALUES ('%s','%s')",
								ID.c_str(),
								maindeviceidx.c_str()
								);
							result = m_sql.query(szTmp);
						}
					}
				}

				root["status"] = "OK";
				root["title"] = "AddSwitch";
			}
			else if (cparam == "getnotificationtypes")
			{
				std::string idx = m_pWebEm->FindValue("idx");
				if (idx == "")
					return;
				//First get Device Type/SubType
				szQuery.clear();
				szQuery.str("");
				szQuery << "SELECT Type, SubType, SwitchType FROM DeviceStatus WHERE (ID == " << idx << ")";
				result = m_sql.query(szQuery.str());
				if (result.size() < 1)
					return;

				root["status"] = "OK";
				root["title"] = "GetNotificationTypes";
				unsigned char dType = atoi(result[0][0].c_str());
				unsigned char dSubType = atoi(result[0][1].c_str());
				unsigned char switchtype = atoi(result[0][2].c_str());

				int ii = 0;
				if (
					(dType == pTypeLighting1) ||
					(dType == pTypeLighting2) ||
					(dType == pTypeLighting3) ||
					(dType == pTypeLighting4) ||
					(dType == pTypeLighting5) ||
					(dType == pTypeLighting6) ||
					(dType == pTypeLimitlessLights) ||
					(dType == pTypeSecurity1) ||
					(dType == pTypeSecurity2) ||
					(dType == pTypeEvohome) ||
					(dType == pTypeEvohomeRelay) ||
					(dType == pTypeCurtain) ||
					(dType == pTypeBlinds) ||
					(dType == pTypeRFY) ||
					(dType == pTypeChime) ||
					(dType == pTypeThermostat2) ||
					(dType == pTypeThermostat3) ||
					(dType == pTypeRemote) ||
					(dType == pTypeGeneralSwitch) ||
					((dType == pTypeRadiator1) && (dSubType == sTypeSmartwaresSwitchRadiator))
					)
				{
					if (switchtype != STYPE_PushOff)
					{
						root["result"][ii]["val"] = NTYPE_SWITCH_ON;
						root["result"][ii]["text"] = Notification_Type_Desc(NTYPE_SWITCH_ON, 0);
						root["result"][ii]["ptag"] = Notification_Type_Desc(NTYPE_SWITCH_ON, 1);
						ii++;
					}
					if (switchtype != STYPE_PushOn)
					{
						root["result"][ii]["val"] = NTYPE_SWITCH_OFF;
						root["result"][ii]["text"] = Notification_Type_Desc(NTYPE_SWITCH_OFF, 0);
						root["result"][ii]["ptag"] = Notification_Type_Desc(NTYPE_SWITCH_OFF, 1);
						ii++;
					}
				}
				if (
					(
					(dType == pTypeTEMP) ||
					(dType == pTypeTEMP_HUM) ||
					(dType == pTypeTEMP_HUM_BARO) ||
					(dType == pTypeTEMP_BARO) ||
					(dType == pTypeEvohomeZone) ||
					(dType == pTypeEvohomeWater) ||
					(dType == pTypeThermostat1) ||
					(dType == pTypeRego6XXTemp) ||
					((dType == pTypeRFXSensor) && (dSubType == sTypeRFXSensorTemp))
					) ||
					((dType == pTypeUV) && (dSubType == sTypeUV3)) ||
					((dType == pTypeWIND) && (dSubType == sTypeWIND4)) ||
					((dType == pTypeWIND) && (dSubType == sTypeWINDNoTemp)) ||
					((dType == pTypeGeneral) && (dSubType == sTypeSystemTemp))
					)
				{
					root["result"][ii]["val"] = NTYPE_TEMPERATURE;
					root["result"][ii]["text"] = Notification_Type_Desc(NTYPE_TEMPERATURE, 0);
					root["result"][ii]["ptag"] = Notification_Type_Desc(NTYPE_TEMPERATURE, 1);
					ii++;
				}
				if (
					(dType == pTypeHUM) ||
					(dType == pTypeTEMP_HUM) ||
					(dType == pTypeTEMP_HUM_BARO)
					)
				{
					root["result"][ii]["val"] = NTYPE_HUMIDITY;
					root["result"][ii]["text"] = Notification_Type_Desc(NTYPE_HUMIDITY, 0);
					root["result"][ii]["ptag"] = Notification_Type_Desc(NTYPE_HUMIDITY, 1);
					ii++;
				}
				if (
					(dType == pTypeTEMP_HUM) ||
					(dType == pTypeTEMP_HUM_BARO)
					)
				{
					root["result"][ii]["val"] = NTYPE_DEWPOINT;
					root["result"][ii]["text"] = Notification_Type_Desc(NTYPE_DEWPOINT, 0);
					root["result"][ii]["ptag"] = Notification_Type_Desc(NTYPE_DEWPOINT, 1);
					ii++;
				}
				if (dType == pTypeRAIN)
				{
					root["result"][ii]["val"] = NTYPE_RAIN;
					root["result"][ii]["text"] = Notification_Type_Desc(NTYPE_RAIN, 0);
					root["result"][ii]["ptag"] = Notification_Type_Desc(NTYPE_RAIN, 1);
					ii++;
				}
				if (dType == pTypeWIND)
				{
					root["result"][ii]["val"] = NTYPE_WIND;
					root["result"][ii]["text"] = Notification_Type_Desc(NTYPE_WIND, 0);
					root["result"][ii]["ptag"] = Notification_Type_Desc(NTYPE_WIND, 1);
					ii++;
				}
				if (dType == pTypeUV)
				{
					root["result"][ii]["val"] = NTYPE_UV;
					root["result"][ii]["text"] = Notification_Type_Desc(NTYPE_UV, 0);
					root["result"][ii]["ptag"] = Notification_Type_Desc(NTYPE_UV, 1);
					ii++;
				}
				if (
					(dType == pTypeTEMP_HUM_BARO) ||
					(dType == pTypeBARO) ||
					(dType == pTypeTEMP_BARO)
					)
				{
					root["result"][ii]["val"] = NTYPE_BARO;
					root["result"][ii]["text"] = Notification_Type_Desc(NTYPE_BARO, 0);
					root["result"][ii]["ptag"] = Notification_Type_Desc(NTYPE_BARO, 1);
					ii++;
				}
				if (
					((dType == pTypeRFXMeter) && (dSubType == sTypeRFXMeterCount)) ||
                    ((dType == pTypeGeneral) && (dSubType == sTypeCounterIncremental)) ||
					(dType == pTypeYouLess) ||
					((dType == pTypeRego6XXValue) && (dSubType == sTypeRego6XXCounter))
					)
				{
					if (switchtype == MTYPE_ENERGY)
					{
						root["result"][ii]["val"] = NTYPE_TODAYENERGY;
						root["result"][ii]["text"] = Notification_Type_Desc(NTYPE_TODAYENERGY, 0);
						root["result"][ii]["ptag"] = Notification_Type_Desc(NTYPE_TODAYENERGY, 1);
					}
					else if (switchtype == MTYPE_GAS)
					{
						root["result"][ii]["val"] = NTYPE_TODAYGAS;
						root["result"][ii]["text"] = Notification_Type_Desc(NTYPE_TODAYGAS, 0);
						root["result"][ii]["ptag"] = Notification_Type_Desc(NTYPE_TODAYGAS, 1);
					}
					else if (switchtype == MTYPE_COUNTER)
					{
						root["result"][ii]["val"] = NTYPE_TODAYCOUNTER;
						root["result"][ii]["text"] = Notification_Type_Desc(NTYPE_TODAYCOUNTER, 0);
						root["result"][ii]["ptag"] = Notification_Type_Desc(NTYPE_TODAYCOUNTER, 1);
					}
					else
					{
						//water (same as gas)
						root["result"][ii]["val"] = NTYPE_TODAYGAS;
						root["result"][ii]["text"] = Notification_Type_Desc(NTYPE_TODAYGAS, 0);
						root["result"][ii]["ptag"] = Notification_Type_Desc(NTYPE_TODAYGAS, 1);
					}
					ii++;
				}
				if (dType == pTypeYouLess)
				{
					root["result"][ii]["val"] = NTYPE_USAGE;
					root["result"][ii]["text"] = Notification_Type_Desc(NTYPE_USAGE, 0);
					root["result"][ii]["ptag"] = Notification_Type_Desc(NTYPE_USAGE, 1);
					ii++;
				}
				if (dType == pTypeAirQuality)
				{
					root["result"][ii]["val"] = NTYPE_USAGE;
					root["result"][ii]["text"] = Notification_Type_Desc(NTYPE_USAGE, 0);
					root["result"][ii]["ptag"] = Notification_Type_Desc(NTYPE_USAGE, 1);
					ii++;
				}
				else if ((dType == pTypeGeneral) && ((dSubType == sTypeSoilMoisture) || (dSubType == sTypeLeafWetness)))
				{
					root["result"][ii]["val"] = NTYPE_USAGE;
					root["result"][ii]["text"] = Notification_Type_Desc(NTYPE_USAGE, 0);
					root["result"][ii]["ptag"] = Notification_Type_Desc(NTYPE_USAGE, 1);
					ii++;
				}
				if ((dType == pTypeGeneral) && (dSubType == sTypeVisibility))
				{
					root["result"][ii]["val"] = NTYPE_USAGE;
					root["result"][ii]["text"] = Notification_Type_Desc(NTYPE_USAGE, 0);
					root["result"][ii]["ptag"] = Notification_Type_Desc(NTYPE_USAGE, 1);
					ii++;
				}
				if ((dType == pTypeGeneral) && (dSubType == sTypeDistance))
				{
					root["result"][ii]["val"] = NTYPE_USAGE;
					root["result"][ii]["text"] = Notification_Type_Desc(NTYPE_USAGE, 0);
					root["result"][ii]["ptag"] = Notification_Type_Desc(NTYPE_USAGE, 1);
					ii++;
				}
				if ((dType == pTypeGeneral) && (dSubType == sTypeSolarRadiation))
				{
					root["result"][ii]["val"] = NTYPE_USAGE;
					root["result"][ii]["text"] = Notification_Type_Desc(NTYPE_USAGE, 0);
					root["result"][ii]["ptag"] = Notification_Type_Desc(NTYPE_USAGE, 1);
					ii++;
				}
				if ((dType == pTypeGeneral) && (dSubType == sTypeVoltage))
				{
					root["result"][ii]["val"] = NTYPE_USAGE;
					root["result"][ii]["text"] = Notification_Type_Desc(NTYPE_USAGE, 0);
					root["result"][ii]["ptag"] = Notification_Type_Desc(NTYPE_USAGE, 1);
					ii++;
				}
				if ((dType == pTypeGeneral) && (dSubType == sTypeCurrent))
				{
					root["result"][ii]["val"] = NTYPE_USAGE;
					root["result"][ii]["text"] = Notification_Type_Desc(NTYPE_USAGE, 0);
					root["result"][ii]["ptag"] = Notification_Type_Desc(NTYPE_USAGE, 1);
					ii++;
				}
				if ((dType == pTypeGeneral) && (dSubType == sTypePressure))
				{
					root["result"][ii]["val"] = NTYPE_USAGE;
					root["result"][ii]["text"] = Notification_Type_Desc(NTYPE_USAGE, 0);
					root["result"][ii]["ptag"] = Notification_Type_Desc(NTYPE_USAGE, 1);
					ii++;
				}
				if ((dType == pTypeGeneral) && (dSubType == sTypeBaro))
				{
					root["result"][ii]["val"] = NTYPE_USAGE;
					root["result"][ii]["text"] = Notification_Type_Desc(NTYPE_USAGE, 0);
					root["result"][ii]["ptag"] = Notification_Type_Desc(NTYPE_USAGE, 1);
					ii++;
				}
				if (dType == pTypeLux)
				{
					root["result"][ii]["val"] = NTYPE_USAGE;
					root["result"][ii]["text"] = Notification_Type_Desc(NTYPE_USAGE, 0);
					root["result"][ii]["ptag"] = Notification_Type_Desc(NTYPE_USAGE, 1);
					ii++;
				}
				if ((dType == pTypeGeneral) && (dSubType == sTypeSoundLevel))
				{
					root["result"][ii]["val"] = NTYPE_USAGE;
					root["result"][ii]["text"] = Notification_Type_Desc(NTYPE_USAGE, 0);
					root["result"][ii]["ptag"] = Notification_Type_Desc(NTYPE_USAGE, 1);
					ii++;
				}
				if (dType == pTypeWEIGHT)
				{
					root["result"][ii]["val"] = NTYPE_USAGE;
					root["result"][ii]["text"] = Notification_Type_Desc(NTYPE_USAGE, 0);
					root["result"][ii]["ptag"] = Notification_Type_Desc(NTYPE_USAGE, 1);
					ii++;
				}
				if (dType == pTypeUsage)
				{
					root["result"][ii]["val"] = NTYPE_USAGE;
					root["result"][ii]["text"] = Notification_Type_Desc(NTYPE_USAGE, 0);
					root["result"][ii]["ptag"] = Notification_Type_Desc(NTYPE_USAGE, 1);
					ii++;
				}
				if (dType == pTypeENERGY)
				{
					root["result"][ii]["val"] = NTYPE_USAGE;
					root["result"][ii]["text"] = Notification_Type_Desc(NTYPE_USAGE, 0);
					root["result"][ii]["ptag"] = Notification_Type_Desc(NTYPE_USAGE, 1);
					ii++;
				}
				if (dType == pTypePOWER)
				{
					root["result"][ii]["val"] = NTYPE_USAGE;
					root["result"][ii]["text"] = Notification_Type_Desc(NTYPE_USAGE, 0);
					root["result"][ii]["ptag"] = Notification_Type_Desc(NTYPE_USAGE, 1);
					ii++;
				}
				if ((dType == pTypeCURRENT) && (dSubType == sTypeELEC1))
				{
					root["result"][ii]["val"] = NTYPE_AMPERE1;
					root["result"][ii]["text"] = Notification_Type_Desc(NTYPE_AMPERE1, 0);
					root["result"][ii]["ptag"] = Notification_Type_Desc(NTYPE_AMPERE1, 1);
					ii++;
					root["result"][ii]["val"] = NTYPE_AMPERE2;
					root["result"][ii]["text"] = Notification_Type_Desc(NTYPE_AMPERE2, 0);
					root["result"][ii]["ptag"] = Notification_Type_Desc(NTYPE_AMPERE2, 1);
					ii++;
					root["result"][ii]["val"] = NTYPE_AMPERE3;
					root["result"][ii]["text"] = Notification_Type_Desc(NTYPE_AMPERE3, 0);
					root["result"][ii]["ptag"] = Notification_Type_Desc(NTYPE_AMPERE3, 1);
					ii++;
				}
				if ((dType == pTypeCURRENTENERGY) && (dSubType == sTypeELEC4))
				{
					root["result"][ii]["val"] = NTYPE_AMPERE1;
					root["result"][ii]["text"] = Notification_Type_Desc(NTYPE_AMPERE1, 0);
					root["result"][ii]["ptag"] = Notification_Type_Desc(NTYPE_AMPERE1, 1);
					ii++;
					root["result"][ii]["val"] = NTYPE_AMPERE2;
					root["result"][ii]["text"] = Notification_Type_Desc(NTYPE_AMPERE2, 0);
					root["result"][ii]["ptag"] = Notification_Type_Desc(NTYPE_AMPERE2, 1);
					ii++;
					root["result"][ii]["val"] = NTYPE_AMPERE3;
					root["result"][ii]["text"] = Notification_Type_Desc(NTYPE_AMPERE3, 0);
					root["result"][ii]["ptag"] = Notification_Type_Desc(NTYPE_AMPERE3, 1);
					ii++;
				}
				if (dType == pTypeP1Power)
				{
					root["result"][ii]["val"] = NTYPE_USAGE;
					root["result"][ii]["text"] = Notification_Type_Desc(NTYPE_USAGE, 0);
					root["result"][ii]["ptag"] = Notification_Type_Desc(NTYPE_USAGE, 1);
					ii++;
					root["result"][ii]["val"] = NTYPE_TODAYENERGY;
					root["result"][ii]["text"] = Notification_Type_Desc(NTYPE_TODAYENERGY, 0);
					root["result"][ii]["ptag"] = Notification_Type_Desc(NTYPE_TODAYENERGY, 1);
					ii++;
				}
				if (dType == pTypeP1Gas)
				{
					root["result"][ii]["val"] = NTYPE_TODAYGAS;
					root["result"][ii]["text"] = Notification_Type_Desc(NTYPE_TODAYGAS, 0);
					root["result"][ii]["ptag"] = Notification_Type_Desc(NTYPE_TODAYGAS, 1);
					ii++;
				}
				if ((dType == pTypeThermostat) && (dSubType == sTypeThermSetpoint))
				{
					root["result"][ii]["val"] = NTYPE_TEMPERATURE;
					root["result"][ii]["text"] = Notification_Type_Desc(NTYPE_TEMPERATURE, 0);
					root["result"][ii]["ptag"] = Notification_Type_Desc(NTYPE_TEMPERATURE, 1);
					ii++;
				}
				if ((dType == pTypeEvohomeZone))
				{
					root["result"][ii]["val"]=NTYPE_TEMPERATURE;
					root["result"][ii]["text"]=Notification_Type_Desc(NTYPE_SETPOINT,0); //FIXME NTYPE_SETPOINT implementation?
					root["result"][ii]["ptag"]=Notification_Type_Desc(NTYPE_SETPOINT,1);
					ii++;
				}
				if ((dType == pTypeRFXSensor) && ((dSubType == sTypeRFXSensorAD) || (dSubType == sTypeRFXSensorVolt)))
				{
					root["result"][ii]["val"] = NTYPE_USAGE;
					root["result"][ii]["text"] = Notification_Type_Desc(NTYPE_USAGE, 0);
					root["result"][ii]["ptag"] = Notification_Type_Desc(NTYPE_USAGE, 1);
					ii++;
				}
				if ((dType == pTypeGeneral) && (dSubType == sTypePercentage))
				{
					root["result"][ii]["val"] = NTYPE_PERCENTAGE;
					root["result"][ii]["text"] = Notification_Type_Desc(NTYPE_PERCENTAGE, 0);
					root["result"][ii]["ptag"] = Notification_Type_Desc(NTYPE_PERCENTAGE, 1);
					ii++;
				}
				if ((dType == pTypeGeneral) && (dSubType == sTypeFan))
				{
					root["result"][ii]["val"] = NTYPE_RPM;
					root["result"][ii]["text"] = Notification_Type_Desc(NTYPE_RPM, 0);
					root["result"][ii]["ptag"] = Notification_Type_Desc(NTYPE_RPM, 1);
					ii++;
				}
				if ((dType == pTypeRego6XXValue) && (dSubType == sTypeRego6XXStatus))
				{
					root["result"][ii]["val"] = NTYPE_SWITCH_ON;
					root["result"][ii]["text"] = Notification_Type_Desc(NTYPE_SWITCH_ON, 0);
					root["result"][ii]["ptag"] = Notification_Type_Desc(NTYPE_SWITCH_ON, 1);
					ii++;
					root["result"][ii]["val"] = NTYPE_SWITCH_OFF;
					root["result"][ii]["text"] = Notification_Type_Desc(NTYPE_SWITCH_OFF, 0);
					root["result"][ii]["ptag"] = Notification_Type_Desc(NTYPE_SWITCH_OFF, 1);
					ii++;
				}
			}
			else if (cparam == "addnotification")
			{
				std::string idx = m_pWebEm->FindValue("idx");
				if (idx == "")
					return;

				std::string stype = m_pWebEm->FindValue("ttype");
				std::string swhen = m_pWebEm->FindValue("twhen");
				std::string svalue = m_pWebEm->FindValue("tvalue");
				std::string scustommessage = m_pWebEm->FindValue("tmsg");
				std::string sactivesystems = m_pWebEm->FindValue("tsystems");
				std::string spriority = m_pWebEm->FindValue("tpriority");
				if ((stype == "") || (swhen == "") || (svalue == "") || (spriority == ""))
					return;

				_eNotificationTypes ntype = (_eNotificationTypes)atoi(stype.c_str());
				std::string ttype = Notification_Type_Desc(ntype, 1);
				if (
					(ntype == NTYPE_SWITCH_ON) ||
					(ntype == NTYPE_SWITCH_OFF) ||
					(ntype == NTYPE_DEWPOINT)
					)
				{
					strcpy(szTmp, ttype.c_str());
				}
				else
				{
					unsigned char twhen = (swhen == "0") ? '>' : '<';
					sprintf(szTmp, "%s;%c;%s", ttype.c_str(), twhen, svalue.c_str());
				}
				int priority = atoi(spriority.c_str());
				bool bOK = m_sql.AddNotification(idx, szTmp, scustommessage, sactivesystems, priority);
				if (bOK) {
					root["status"] = "OK";
					root["title"] = "AddNotification";
				}
			}
			else if (cparam == "updatenotification")
			{
				std::string idx = m_pWebEm->FindValue("idx");
				std::string devidx = m_pWebEm->FindValue("devidx");
				if ((idx == "") || (devidx == ""))
					return;

				std::string stype = m_pWebEm->FindValue("ttype");
				std::string swhen = m_pWebEm->FindValue("twhen");
				std::string svalue = m_pWebEm->FindValue("tvalue");
				std::string scustommessage = m_pWebEm->FindValue("tmsg");
				std::string sactivesystems = m_pWebEm->FindValue("tsystems");
				std::string spriority = m_pWebEm->FindValue("tpriority");
				if ((stype == "") || (swhen == "") || (svalue == "") || (spriority == ""))
					return;
				root["status"] = "OK";
				root["title"] = "UpdateNotification";

				//delete old record
				m_sql.RemoveNotification(idx);

				_eNotificationTypes ntype = (_eNotificationTypes)atoi(stype.c_str());
				std::string ttype = Notification_Type_Desc(ntype, 1);
				if (
					(ntype == NTYPE_SWITCH_ON) ||
					(ntype == NTYPE_SWITCH_OFF) ||
					(ntype == NTYPE_DEWPOINT)
					)
				{
					strcpy(szTmp, ttype.c_str());
				}
				else
				{
					unsigned char twhen = (swhen == "0") ? '>' : '<';
					sprintf(szTmp, "%s;%c;%s", ttype.c_str(), twhen, svalue.c_str());
				}
				int priority = atoi(spriority.c_str());
				m_sql.AddNotification(devidx, szTmp, scustommessage, sactivesystems, priority);
			}
			else if (cparam == "deletenotification")
			{
				std::string idx = m_pWebEm->FindValue("idx");
				if (idx == "")
					return;

				root["status"] = "OK";
				root["title"] = "DeleteNotification";

				m_sql.RemoveNotification(idx);
			}
			else if (cparam == "switchdeviceorder")
			{
				std::string idx1 = m_pWebEm->FindValue("idx1");
				std::string idx2 = m_pWebEm->FindValue("idx2");
				if ((idx1 == "") || (idx2 == ""))
					return;
				std::string sroomid = m_pWebEm->FindValue("roomid");
				int roomid = atoi(sroomid.c_str());

				std::string Order1, Order2;
				if (roomid == 0)
				{
					//get device order 1
					szQuery.clear();
					szQuery.str("");
					szQuery << "SELECT [Order] FROM DeviceStatus WHERE (ID == " << idx1 << ")";
					result = m_sql.query(szQuery.str());
					if (result.size() < 1)
						return;
					Order1 = result[0][0];

					//get device order 2
					szQuery.clear();
					szQuery.str("");
					szQuery << "SELECT [Order] FROM DeviceStatus WHERE (ID == " << idx2 << ")";
					result = m_sql.query(szQuery.str());
					if (result.size() < 1)
						return;
					Order2 = result[0][0];

					root["status"] = "OK";
					root["title"] = "SwitchDeviceOrder";

					szQuery.clear();
					szQuery.str("");
					if (atoi(Order1.c_str()) < atoi(Order2.c_str()))
					{
						szQuery << "UPDATE DeviceStatus SET [Order] = [Order]+1 WHERE ([Order] >= " << Order1 << " AND [Order] < " << Order2 << ")";
					}
					else
					{
						szQuery << "UPDATE DeviceStatus SET [Order] = [Order]-1 WHERE ([Order] > " << Order2 << " AND [Order] <= " << Order1 << ")";
					}
					m_sql.query(szQuery.str());

					szQuery.clear();
					szQuery.str("");
					szQuery << "UPDATE DeviceStatus SET [Order] = " << Order1 << " WHERE (ID == " << idx2 << ")";
					m_sql.query(szQuery.str());
				}
				else
				{
					//change order in a room
					//get device order 1
					szQuery.clear();
					szQuery.str("");
					szQuery << "SELECT [Order] FROM DeviceToPlansMap WHERE (DeviceRowID == " << idx1 << ") AND (PlanID==" << roomid << ")";
					result = m_sql.query(szQuery.str());
					if (result.size() < 1)
						return;
					Order1 = result[0][0];

					//get device order 2
					szQuery.clear();
					szQuery.str("");
					szQuery << "SELECT [Order] FROM DeviceToPlansMap WHERE (DeviceRowID == " << idx2 << ") AND (PlanID==" << roomid << ")";
					result = m_sql.query(szQuery.str());
					if (result.size() < 1)
						return;
					Order2 = result[0][0];

					root["status"] = "OK";
					root["title"] = "SwitchDeviceOrder";

					szQuery.clear();
					szQuery.str("");
					if (atoi(Order1.c_str()) < atoi(Order2.c_str()))
					{
						szQuery << "UPDATE DeviceToPlansMap SET [Order] = [Order]+1 WHERE ([Order] >= " << Order1 << " AND [Order] < " << Order2 << ") AND (PlanID==" << roomid << ")";
					}
					else
					{
						szQuery << "UPDATE DeviceToPlansMap SET [Order] = [Order]-1 WHERE ([Order] > " << Order2 << " AND [Order] <= " << Order1 << ") AND (PlanID==" << roomid << ")";
					}
					m_sql.query(szQuery.str());

					szQuery.clear();
					szQuery.str("");
					szQuery << "UPDATE DeviceToPlansMap SET [Order] = " << Order1 << " WHERE (DeviceRowID == " << idx2 << ") AND (PlanID==" << roomid << ")";
					m_sql.query(szQuery.str());
				}
			}
			else if (cparam == "switchsceneorder")
			{
				std::string idx1 = m_pWebEm->FindValue("idx1");
				std::string idx2 = m_pWebEm->FindValue("idx2");
				if ((idx1 == "") || (idx2 == ""))
					return;

				std::string Order1, Order2;
				//get device order 1
				szQuery.clear();
				szQuery.str("");
				szQuery << "SELECT [Order] FROM Scenes WHERE (ID == " << idx1 << ")";
				result = m_sql.query(szQuery.str());
				if (result.size() < 1)
					return;
				Order1 = result[0][0];

				//get device order 2
				szQuery.clear();
				szQuery.str("");
				szQuery << "SELECT [Order] FROM Scenes WHERE (ID == " << idx2 << ")";
				result = m_sql.query(szQuery.str());
				if (result.size() < 1)
					return;
				Order2 = result[0][0];

				root["status"] = "OK";
				root["title"] = "SwitchSceneOrder";

				szQuery.clear();
				szQuery.str("");
				if (atoi(Order1.c_str()) < atoi(Order2.c_str()))
				{
					szQuery << "UPDATE Scenes SET [Order] = [Order]+1 WHERE ([Order] >= " << Order1 << " AND [Order] < " << Order2 << ")";
				}
				else
				{
					szQuery << "UPDATE Scenes SET [Order] = [Order]-1 WHERE ([Order] > " << Order2 << " AND [Order] <= " << Order1 << ")";
				}
				m_sql.query(szQuery.str());

				szQuery.clear();
				szQuery.str("");
				szQuery << "UPDATE Scenes SET [Order] = " << Order1 << " WHERE (ID == " << idx2 << ")";
				m_sql.query(szQuery.str());
			}
			else if (cparam == "clearnotifications")
			{
				std::string idx = m_pWebEm->FindValue("idx");
				if (idx == "")
					return;

				root["status"] = "OK";
				root["title"] = "ClearNotification";

				m_sql.RemoveDeviceNotifications(idx);
			}
			else if (cparam == "addcamera")
			{
				std::string name = m_pWebEm->FindValue("name");
				std::string senabled = m_pWebEm->FindValue("enabled");
				std::string address = m_pWebEm->FindValue("address");
				std::string sport = m_pWebEm->FindValue("port");
				std::string username = m_pWebEm->FindValue("username");
				std::string password = m_pWebEm->FindValue("password");
				std::string timageurl = m_pWebEm->FindValue("imageurl");
				if (
					(name == "") ||
					(address == "") ||
					(timageurl == "")
					)
					return;

				std::string imageurl;
				if (request_handler::url_decode(timageurl, imageurl))
				{
					imageurl = base64_decode(imageurl);

					int port = atoi(sport.c_str());
					root["status"] = "OK";
					root["title"] = "AddCamera";
					sprintf(szTmp,
						"INSERT INTO Cameras (Name, Enabled, Address, Port, Username, Password, ImageURL) VALUES ('%s',%d,'%s',%d,'%s','%s','%s')",
						name.c_str(),
						(senabled == "true") ? 1 : 0,
						address.c_str(),
						port,
						base64_encode((const unsigned char*)username.c_str(), username.size()).c_str(),
						base64_encode((const unsigned char*)password.c_str(), password.size()).c_str(),
						imageurl.c_str()
						);
					result = m_sql.query(szTmp);
					m_mainworker.m_cameras.ReloadCameras();
				}
			}
			else if (cparam == "updatecamera")
			{
				std::string idx = m_pWebEm->FindValue("idx");
				if (idx == "")
					return;
				std::string name = m_pWebEm->FindValue("name");
				std::string senabled = m_pWebEm->FindValue("enabled");
				std::string address = m_pWebEm->FindValue("address");
				std::string sport = m_pWebEm->FindValue("port");
				std::string username = m_pWebEm->FindValue("username");
				std::string password = m_pWebEm->FindValue("password");
				std::string timageurl = m_pWebEm->FindValue("imageurl");
				if (
					(name == "") ||
					(senabled == "") ||
					(address == "") ||
					(timageurl == "")
					)
					return;

				std::string imageurl;
				if (request_handler::url_decode(timageurl, imageurl))
				{
					imageurl = base64_decode(imageurl);

					int port = atoi(sport.c_str());

					root["status"] = "OK";
					root["title"] = "UpdateCamera";

					sprintf(szTmp,
						"UPDATE Cameras SET Name='%s', Enabled=%d, Address='%s', Port=%d, Username='%s', Password='%s', ImageURL='%s' WHERE (ID == %s)",
						name.c_str(),
						(senabled == "true") ? 1 : 0,
						address.c_str(),
						port,
						base64_encode((const unsigned char*)username.c_str(), username.size()).c_str(),
						base64_encode((const unsigned char*)password.c_str(), password.size()).c_str(),
						imageurl.c_str(),
						idx.c_str()
						);
					result = m_sql.query(szTmp);
					m_mainworker.m_cameras.ReloadCameras();
				}
			}
			else if (cparam == "deletecamera")
			{
				std::string idx = m_pWebEm->FindValue("idx");
				if (idx == "")
					return;
				root["status"] = "OK";
				root["title"] = "DeleteCamera";

				m_sql.DeleteCamera(idx);
				m_mainworker.m_cameras.ReloadCameras();
			}
			else if (cparam == "adduser")
			{
				bool bHaveUser = (m_pWebEm->m_actualuser != "");
				int urights = 3;
				if (bHaveUser)
				{
					int iUser = -1;
					iUser = FindUser(m_pWebEm->m_actualuser.c_str());
					if (iUser != -1)
						urights = static_cast<int>(m_users[iUser].userrights);
				}
				if (urights < 2)
					return;

				std::string senabled = m_pWebEm->FindValue("enabled");
				std::string username = m_pWebEm->FindValue("username");
				std::string password = m_pWebEm->FindValue("password");
				std::string srights = m_pWebEm->FindValue("rights");
				std::string sRemoteSharing = m_pWebEm->FindValue("RemoteSharing");
				std::string sTabsEnabled = m_pWebEm->FindValue("TabsEnabled");
				if (
					(senabled == "") ||
					(username == "") ||
					(password == "") ||
					(srights == "") ||
					(sRemoteSharing == "") ||
					(sTabsEnabled == "")
					)
					return;
				int rights = atoi(srights.c_str());
				if (rights != 2)
				{
					if (!FindAdminUser())
					{
						root["message"] = "Add a Admin user first! (Or enable Settings/Website Protection)";
						return;
					}
				}
				root["status"] = "OK";
				root["title"] = "AddUser";
				sprintf(szTmp,
					"INSERT INTO Users (Active, Username, Password, Rights, RemoteSharing, TabsEnabled) VALUES (%d,'%s','%s','%d','%d','%d')",
					(senabled == "true") ? 1 : 0,
					base64_encode((const unsigned char*)username.c_str(), username.size()).c_str(),
					password.c_str(),
					rights,
					(sRemoteSharing == "true") ? 1 : 0,
					atoi(sTabsEnabled.c_str())
					);
				result = m_sql.query(szTmp);
				LoadUsers();
			}
			else if (cparam == "updateuser")
			{
				bool bHaveUser = (m_pWebEm->m_actualuser != "");
				int urights = 3;
				if (bHaveUser)
				{
					int iUser = -1;
					iUser = FindUser(m_pWebEm->m_actualuser.c_str());
					if (iUser != -1)
						urights = static_cast<int>(m_users[iUser].userrights);
				}
				if (urights < 2)
					return;

				std::string idx = m_pWebEm->FindValue("idx");
				if (idx == "")
					return;
				std::string senabled = m_pWebEm->FindValue("enabled");
				std::string username = m_pWebEm->FindValue("username");
				std::string password = m_pWebEm->FindValue("password");
				std::string srights = m_pWebEm->FindValue("rights");
				std::string sRemoteSharing = m_pWebEm->FindValue("RemoteSharing");
				std::string sTabsEnabled = m_pWebEm->FindValue("TabsEnabled");
				if (
					(senabled == "") ||
					(username == "") ||
					(password == "") ||
					(srights == "") ||
					(sRemoteSharing == "") ||
					(sTabsEnabled == "")
					)
					return;
				int rights = atoi(srights.c_str());
				if (rights != 2)
				{
					if (!FindAdminUser())
					{
						root["message"] = "Add a Admin user first! (Or enable Settings/Website Protection)";
						return;
					}
				}

				root["status"] = "OK";
				root["title"] = "UpdateUser";
				sprintf(szTmp,
					"UPDATE Users SET Active=%d, Username='%s', Password='%s', Rights=%d, RemoteSharing=%d, TabsEnabled=%d WHERE (ID == %s)",
					(senabled == "true") ? 1 : 0,
					base64_encode((const unsigned char*)username.c_str(), username.size()).c_str(),
					password.c_str(),
					rights,
					(sRemoteSharing == "true") ? 1 : 0,
					atoi(sTabsEnabled.c_str()),
					idx.c_str()
					);
				result = m_sql.query(szTmp);
				LoadUsers();
			}
			else if (cparam == "deleteuser")
			{
				bool bHaveUser = (m_pWebEm->m_actualuser != "");
				int urights = 3;
				if (bHaveUser)
				{
					int iUser = -1;
					iUser = FindUser(m_pWebEm->m_actualuser.c_str());
					if (iUser != -1)
						urights = static_cast<int>(m_users[iUser].userrights);
				}
				if (urights < 2)
					return;

				std::string idx = m_pWebEm->FindValue("idx");
				if (idx == "")
					return;

				root["status"] = "OK";
				root["title"] = "DeleteUser";
				sprintf(szTmp, "DELETE FROM Users WHERE (ID == %s)", idx.c_str());
				result = m_sql.query(szTmp);

				szQuery.clear();
				szQuery.str("");
				szQuery << "DELETE FROM SharedDevices WHERE (SharedUserID == " << idx << ")";
				result = m_sql.query(szQuery.str()); //-V519

				LoadUsers();
			}
			else if (cparam == "clearlightlog")
			{
				std::string idx = m_pWebEm->FindValue("idx");
				if (idx == "")
					return;
				//First get Device Type/SubType
				szQuery.clear();
				szQuery.str("");
				szQuery << "SELECT Type, SubType FROM DeviceStatus WHERE (ID == " << idx << ")";
				result = m_sql.query(szQuery.str());
				if (result.size() < 1)
					return;

				unsigned char dType = atoi(result[0][0].c_str());
				unsigned char dSubType = atoi(result[0][1].c_str());

				if (
					(dType != pTypeLighting1) &&
					(dType != pTypeLighting2) &&
					(dType != pTypeLighting3) &&
					(dType != pTypeLighting4) &&
					(dType != pTypeLighting5) &&
					(dType != pTypeLighting6) &&
					(dType != pTypeLimitlessLights) &&
					(dType != pTypeSecurity1) &&
					(dType != pTypeSecurity2) &&
					(dType != pTypeEvohome) &&
					(dType != pTypeEvohomeRelay) &&
					(dType != pTypeCurtain) &&
					(dType != pTypeBlinds) &&
					(dType != pTypeRFY) &&
					(dType != pTypeChime) &&
					(dType != pTypeThermostat2) &&
					(dType != pTypeThermostat3) &&
					(dType != pTypeRemote) &&
					(dType != pTypeGeneralSwitch) &&
					(!((dType == pTypeRadiator1) && (dSubType == sTypeSmartwaresSwitchRadiator)))
					)
					return; //no light device! we should not be here!

				root["status"] = "OK";
				root["title"] = "ClearLightLog";

				szQuery.clear();
				szQuery.str("");
				szQuery << "DELETE FROM LightingLog WHERE (DeviceRowID==" << idx << ")";
				result = m_sql.query(szQuery.str());
			}
			else if (cparam == "learnsw")
			{
				m_sql.AllowNewHardwareTimer(5);
				m_sql.m_LastSwitchID = "";
				bool bReceivedSwitch = false;
				unsigned char cntr = 0;
				while ((!bReceivedSwitch) && (cntr < 50))	//wait for max. 5 seconds
				{
					if (m_sql.m_LastSwitchID != "")
					{
						bReceivedSwitch = true;
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
					szQuery << "SELECT Name, Used, nValue FROM DeviceStatus WHERE (ID==" << m_sql.m_LastSwitchRowID << ")";
					result = m_sql.query(szQuery.str());
					if (result.size() > 0)
					{
						root["status"] = "OK";
						root["title"] = "LearnSW";
						root["ID"] = m_sql.m_LastSwitchID;
						root["idx"] = m_sql.m_LastSwitchRowID;
						root["Name"] = result[0][0];
						root["Used"] = atoi(result[0][1].c_str());
						root["Cmd"] = atoi(result[0][2].c_str());
					}
				}
			} //learnsw
			else if (cparam == "makefavorite")
			{
				std::string idx = m_pWebEm->FindValue("idx");
				std::string sisfavorite = m_pWebEm->FindValue("isfavorite");
				if ((idx == "") || (sisfavorite == ""))
					return;
				int isfavorite = atoi(sisfavorite.c_str());
				szQuery.clear();
				szQuery.str("");
				szQuery << "UPDATE DeviceStatus SET Favorite=" << isfavorite << " WHERE (ID == " << idx << ")";
				result = m_sql.query(szQuery.str());
				root["status"] = "OK";
				root["title"] = "MakeFavorite";
			} //makefavorite
			else if (cparam == "makescenefavorite")
			{
				std::string idx = m_pWebEm->FindValue("idx");
				std::string sisfavorite = m_pWebEm->FindValue("isfavorite");
				if ((idx == "") || (sisfavorite == ""))
					return;
				int isfavorite = atoi(sisfavorite.c_str());
				szQuery.clear();
				szQuery.str("");
				szQuery << "UPDATE Scenes SET Favorite=" << isfavorite << " WHERE (ID == " << idx << ")";
				result = m_sql.query(szQuery.str());
				root["status"] = "OK";
				root["title"] = "MakeSceneFavorite";
			} //makescenefavorite
			else if (cparam == "resetsecuritystatus")
			{
				int urights = 3;
				if (bHaveUser)
				{
					int iUser = -1;
					iUser = FindUser(m_pWebEm->m_actualuser.c_str());
					if (iUser != -1)
					{
						urights = static_cast<int>(m_users[iUser].userrights);
						_log.Log(LOG_STATUS, "User: %s initiated a security status reset command", m_users[iUser].Username.c_str());
					}
				}
				if (urights < 1)
					return;

				std::string idx = m_pWebEm->FindValue("idx");
				std::string switchcmd = m_pWebEm->FindValue("switchcmd");

				if ((idx == "") || (switchcmd == ""))
					return;

				root["status"] = "OK";
				root["title"] = "ResetSecurityStatus";

				int nValue = -1;

				// Change to generic *Security_Status_Desc lookup...

				if (switchcmd == "Panic End") {
					nValue = 7;
				}
				else if (switchcmd == "Normal") {
					nValue = 0;
				}

				if (nValue >= 0)
				{
					szQuery.clear();
					szQuery.str("");
					szQuery << "UPDATE DeviceStatus SET nValue=" << nValue << " WHERE (ID == " << idx << ")";
					result = m_sql.query(szQuery.str());
					root["status"] = "OK";
					root["title"] = "SwitchLight";
				}
			}
			else if (cparam == "verifypasscode")
			{
				std::string passcode = m_pWebEm->FindValue("passcode");
				if (passcode == "")
					return;
				//Check if passcode is correct
				passcode = GenerateMD5Hash(passcode);
				std::string rpassword;
				int nValue = 1;
				m_sql.GetPreferencesVar("ProtectionPassword", nValue, rpassword);
				if (passcode == rpassword)
				{
					root["title"] = "VerifyPasscode";
					root["status"] = "OK";
					return;
				}
			}
			else if (cparam=="switchmodal")
			{
				int urights=3;
				if (bHaveUser)
				{
					int iUser=-1;
					iUser=FindUser(m_pWebEm->m_actualuser.c_str());
					if (iUser != -1)
					{
						urights = (int)m_users[iUser].userrights;
						_log.Log(LOG_STATUS, "User: %s initiated a modal command", m_users[iUser].Username.c_str());
					}
				}
				if (urights<1)
					return;

				std::string idx=m_pWebEm->FindValue("idx");
				std::string switchcmd=m_pWebEm->FindValue("status");
				std::string until=m_pWebEm->FindValue("until");//optional until date / time as applicable
				std::string action=m_pWebEm->FindValue("action");//Run action or not (update status only)
				std::string onlyonchange=m_pWebEm->FindValue("ooc");//No update unless the value changed (check if updated)
				//The on action is used to call a script to update the real device so we only want to use it when altering the status in the Domoticz Web Client
				//If we're posting the status from the real device to domoticz we don't want to run the on action script ("action"!=1) to avoid loops and contention
				//""... we only want to log a change (and trigger an event) when the status has actually changed ("ooc"==1) i.e. suppress non transient updates
				if ((idx=="")||(switchcmd==""))
					return;

				std::string passcode=m_pWebEm->FindValue("passcode");
				if (passcode.size()>0) 
				{
					//Check if passcode is correct
					passcode = GenerateMD5Hash(passcode);
					std::string rpassword;
					int nValue=1;
					m_sql.GetPreferencesVar("ProtectionPassword",nValue,rpassword);
					if (passcode!=rpassword)
					{
						root["title"]="Modal";
						root["status"]="ERROR";
						root["message"]="WRONG CODE";
						return;
					}
				}

				if (m_mainworker.SwitchModal(idx,switchcmd,action,onlyonchange,until)==true)//FIXME we need to return a status of already set / no update if ooc=="1" and no status update was performed
				{
					root["status"]="OK";
					root["title"]="Modal";
				}
			} 
			else if (cparam == "switchlight")
			{
				int urights = 3;
				if (bHaveUser)
				{
					int iUser = -1;
					iUser = FindUser(m_pWebEm->m_actualuser.c_str());
					if (iUser != -1)
					{
						urights = static_cast<int>(m_users[iUser].userrights);
						_log.Log(LOG_STATUS, "User: %s initiated a switch command", m_users[iUser].Username.c_str());
					}
				}
				if (urights < 1)
					return;

				std::string idx = m_pWebEm->FindValue("idx");
				std::string switchcmd = m_pWebEm->FindValue("switchcmd");
				std::string level = m_pWebEm->FindValue("level");
				std::string onlyonchange=m_pWebEm->FindValue("ooc");//No update unless the value changed (check if updated)
				if ((idx == "") || (switchcmd == ""))
					return;

				std::string passcode = m_pWebEm->FindValue("passcode");
				if (passcode.size() > 0)
				{
					//Check if passcode is correct
					passcode = GenerateMD5Hash(passcode);
					std::string rpassword;
					int nValue = 1;
					m_sql.GetPreferencesVar("ProtectionPassword", nValue, rpassword);
					if (passcode != rpassword)
					{
						root["title"] = "SwitchLight";
						root["status"] = "ERROR";
						root["message"] = "WRONG CODE";
						return;
					}
				}
				if (switchcmd == "Toggle") {
					//Request current state of switch
					sprintf(szTmp,
						"SELECT [Type],[SubType],SwitchType,nValue,sValue FROM DeviceStatus WHERE (ID = %s)", idx.c_str());
					result = m_sql.query(szTmp);
					if (result.size() > 0)
					{
						std::vector<std::string> sd = result[0];
						unsigned char devType = (unsigned char)atoi(sd[0].c_str());
						unsigned char subType = (unsigned char)atoi(sd[1].c_str());
						_eSwitchType switchtype = (_eSwitchType)atoi(sd[2].c_str());
						int nValue = atoi(sd[3].c_str());
						std::string sValue = sd[4];
						std::string lstatus = "";
						int llevel = 0;
						bool bHaveDimmer = false;
						bool bHaveGroupCmd = false;
						int maxDimLevel = 0;
						GetLightStatus(devType, subType, switchtype, nValue, sValue, lstatus, llevel, bHaveDimmer, maxDimLevel, bHaveGroupCmd);
						switchcmd = (IsLightSwitchOn(lstatus) == true) ? "Off" : "On";
					}
				}
				root["title"] = "SwitchLight";
				if (m_mainworker.SwitchLight(idx, switchcmd, level, "-1", onlyonchange, 0) == true)
				{
					root["status"] = "OK";
				}
				else
				{
					root["status"] = "ERROR";
					root["message"] = "Error sending switch command, check device/hardware !";
				}
			} //(rtype=="switchlight")
			else if (cparam == "switchscene")
			{
				int urights = 3;
				if (bHaveUser)
				{
					int iUser = -1;
					iUser = FindUser(m_pWebEm->m_actualuser.c_str());
					if (iUser != -1)
						urights = static_cast<int>(m_users[iUser].userrights);
				}
				if (urights < 1)
					return;

				std::string idx = m_pWebEm->FindValue("idx");
				std::string switchcmd = m_pWebEm->FindValue("switchcmd");
				if ((idx == "") || (switchcmd == ""))
					return;

				std::string passcode = m_pWebEm->FindValue("passcode");
				if (passcode.size() > 0)
				{
					//Check if passcode is correct
					passcode = GenerateMD5Hash(passcode);
					std::string rpassword;
					int nValue = 1;
					m_sql.GetPreferencesVar("ProtectionPassword", nValue, rpassword);
					if (passcode != rpassword)
					{
						root["title"] = "SwitchScene";
						root["status"] = "ERROR";
						root["message"] = "WRONG CODE";
						return;
					}
				}

				if (m_mainworker.SwitchScene(idx, switchcmd) == true)
				{
					root["status"] = "OK";
					root["title"] = "SwitchScene";
				}
			} //(rtype=="switchscene")
			else if (cparam == "getSunRiseSet") {
				if (!m_mainworker.m_LastSunriseSet.empty())
				{
					std::vector<std::string> strarray;
					StringSplit(m_mainworker.m_LastSunriseSet, ";", strarray);
					if (strarray.size() == 2)
					{
						struct tm loctime;
						time_t now = mytime(NULL);

						localtime_r(&now, &loctime);
						strftime(szTmp, 80, "%b %d %Y %X", &loctime);

						root["status"] = "OK";
						root["title"] = "getSunRiseSet";
						root["ServerTime"] = szTmp;
						root["Sunrise"] = strarray[0];
						root["Sunset"] = strarray[1];
					}
				}
			}
			else if (cparam == "getServerTime") {

				struct tm loctime;
				time_t now = mytime(NULL);

				localtime_r(&now, &loctime);
				strftime(szTmp, 80, "%b %d %Y %X", &loctime);

				root["status"] = "OK";
				root["title"] = "getServerTime";
				root["ServerTime"] = szTmp;
			}
			else if (cparam == "getsecstatus")
			{
				root["status"] = "OK";
				root["title"] = "GetSecStatus";
				int secstatus = 0;
				m_sql.GetPreferencesVar("SecStatus", secstatus);
				root["secstatus"] = secstatus;
			}
			else if (cparam == "setsecstatus")
			{
				std::string ssecstatus = m_pWebEm->FindValue("secstatus");
				std::string seccode = m_pWebEm->FindValue("seccode");
				if ((ssecstatus == "") || (seccode == ""))
				{
					root["message"] = "WRONG CODE";
					return;
				}
				root["title"] = "SetSecStatus";
				std::string rpassword;
				int nValue = 1;
				m_sql.GetPreferencesVar("SecPassword", nValue, rpassword);
				if (seccode != rpassword)
				{
					root["status"] = "ERROR";
					root["message"] = "WRONG CODE";
					return;
				}
				root["status"] = "OK";
				int iSecStatus = atoi(ssecstatus.c_str());
				m_mainworker.UpdateDomoticzSecurityStatus(iSecStatus);
			}
			else if (cparam == "setcolbrightnessvalue")
			{
				std::string idx = m_pWebEm->FindValue("idx");
				std::string hue = m_pWebEm->FindValue("hue");
				std::string brightness = m_pWebEm->FindValue("brightness");
				std::string iswhite = m_pWebEm->FindValue("iswhite");

				if ((idx == "") || (hue == "") || (brightness == "") || (iswhite == ""))
				{
					return;
				}

				unsigned long long ID;
				std::stringstream s_strid;
				s_strid << idx;
				s_strid >> ID;

				if (iswhite != "true")
				{
					//convert hue from 360 steps to 255
					double dval;
					dval = (255.0 / 360.0)*atof(hue.c_str());
					int ival;
					ival = round(dval);
					m_mainworker.SwitchLight(ID, "Set Color", ival, -1,false,0);
				}
				else
				{
					m_mainworker.SwitchLight(ID, "Set White", 0, -1,false,0);
				}
				sleep_milliseconds(100);
				m_mainworker.SwitchLight(ID, "Set Brightness", (unsigned char)atoi(brightness.c_str()), -1,false,0);
			}
			else if (cparam == "brightnessup")
			{
				std::string idx = m_pWebEm->FindValue("idx");

				if (idx == "")
				{
					return;
				}

				unsigned long long ID;
				std::stringstream s_strid;
				s_strid << idx;
				s_strid >> ID;

				m_mainworker.SwitchLight(ID, "Bright Up", 0, -1,false,0);
			}
			else if (cparam == "brightnessdown")
			{
				std::string idx = m_pWebEm->FindValue("idx");

				if (idx == "")
				{
					return;
				}

				unsigned long long ID;
				std::stringstream s_strid;
				s_strid << idx;
				s_strid >> ID;

				m_mainworker.SwitchLight(ID, "Bright Down", 0, -1,false,0);
			}
			else if (cparam == "discoup")
			{
				std::string idx = m_pWebEm->FindValue("idx");

				if (idx == "")
				{
					return;
				}

				unsigned long long ID;
				std::stringstream s_strid;
				s_strid << idx;
				s_strid >> ID;

				m_mainworker.SwitchLight(ID, "Disco Up", 0, -1,false,0);
			}
			else if (cparam == "discodown")
			{
				std::string idx = m_pWebEm->FindValue("idx");

				if (idx == "")
				{
					return;
				}

				unsigned long long ID;
				std::stringstream s_strid;
				s_strid << idx;
				s_strid >> ID;

				m_mainworker.SwitchLight(ID, "Disco Down", 0, -1,false,0);
			}
			else if (cparam == "speedup")
			{
				std::string idx = m_pWebEm->FindValue("idx");

				if (idx == "")
				{
					return;
				}

				unsigned long long ID;
				std::stringstream s_strid;
				s_strid << idx;
				s_strid >> ID;

				m_mainworker.SwitchLight(ID, "Speed Up", 0, -1,false,0);
			}
			else if (cparam == "speeduplong")
			{
				std::string idx = m_pWebEm->FindValue("idx");

				if (idx == "")
				{
					return;
				}

				unsigned long long ID;
				std::stringstream s_strid;
				s_strid << idx;
				s_strid >> ID;

				m_mainworker.SwitchLight(ID, "Speed Up Long", 0, -1,false,0);
			}
			else if (cparam == "speeddown")
			{
				std::string idx = m_pWebEm->FindValue("idx");

				if (idx == "")
				{
					return;
				}

				unsigned long long ID;
				std::stringstream s_strid;
				s_strid << idx;
				s_strid >> ID;

				m_mainworker.SwitchLight(ID, "Speed Down", 0, -1,false,0);
			}
			else if (cparam == "warmer")
			{
				std::string idx = m_pWebEm->FindValue("idx");

				if (idx == "")
				{
					return;
				}

				unsigned long long ID;
				std::stringstream s_strid;
				s_strid << idx;
				s_strid >> ID;

				m_mainworker.SwitchLight(ID, "Warmer", 0, -1,false,0);
			}
			else if (cparam == "cooler")
			{
				std::string idx = m_pWebEm->FindValue("idx");

				if (idx == "")
				{
					return;
				}

				unsigned long long ID;
				std::stringstream s_strid;
				s_strid << idx;
				s_strid >> ID;

				m_mainworker.SwitchLight(ID, "Cooler", 0, -1,false,0);
			}
			else if (cparam == "fulllight")
			{
				std::string idx = m_pWebEm->FindValue("idx");

				if (idx == "")
				{
					return;
				}

				unsigned long long ID;
				std::stringstream s_strid;
				s_strid << idx;
				s_strid >> ID;

				m_mainworker.SwitchLight(ID, "Set Full", 0, -1,false,0);
			}
			else if (cparam == "nightlight")
			{
				std::string idx = m_pWebEm->FindValue("idx");

				if (idx == "")
				{
					return;
				}

				unsigned long long ID;
				std::stringstream s_strid;
				s_strid << idx;
				s_strid >> ID;

				m_mainworker.SwitchLight(ID, "Set Night", 0, -1,false,0);
			}
			else if (cparam == "getfloorplanimages")
			{
				root["status"] = "OK";
				root["title"] = "GetFloorplanImages";

				DIR *lDir;
				struct dirent *ent;
				std::string imagesFolder = szWWWFolder + "/images/floorplans";
				int iFile = 0;
				if ((lDir = opendir(imagesFolder.c_str())) != NULL)
				{
					while ((ent = readdir(lDir)) != NULL)
					{
						std::string filename = ent->d_name;

						std::string temp_filename = filename;
						std::transform(temp_filename.begin(), temp_filename.end(), temp_filename.begin(), ::tolower);

						size_t pos = temp_filename.find(".png");
						if (pos == std::string::npos)
						{
							pos = temp_filename.find(".jpg");
							if (pos == std::string::npos)
							{
								pos = temp_filename.find(".bmp");
							}
						}
						if (pos != std::string::npos)
						{
							root["result"]["images"][iFile++] = filename;
						}
					}
					closedir(lDir);
				}
			}
			else if (cparam == "addfloorplan")
			{
				bool bHaveUser = (m_pWebEm->m_actualuser != "");
				int urights = 3;
				if (bHaveUser)
				{
					int iUser = -1;
					iUser = FindUser(m_pWebEm->m_actualuser.c_str());
					if (iUser != -1)
						urights = static_cast<int>(m_users[iUser].userrights);
				}
				if (urights < 2)
					return;

				std::string name = m_pWebEm->FindValue("name");
				std::string imagefile = m_pWebEm->FindValue("image");
				std::string scalefactor = m_pWebEm->FindValue("scalefactor");
				if (
					(name == "") ||
					(imagefile == "") ||
					(scalefactor == "")
					)
					return;

				root["status"] = "OK";
				root["title"] = "AddFloorplan";
				sprintf(szTmp,
					"INSERT INTO Floorplans (Name,ImageFile,ScaleFactor) VALUES ('%s','%s',%s)",
					name.c_str(),
					imagefile.c_str(),
					scalefactor.c_str()
					);
				result = m_sql.query(szTmp);
				_log.Log(LOG_STATUS, "(Floorplan) '%s' created with image file '%s', Scale Factor %s.", name.c_str(), imagefile.c_str(), scalefactor.c_str());
			}
			else if (cparam == "updatefloorplan")
			{
				bool bHaveUser = (m_pWebEm->m_actualuser != "");
				int urights = 3;
				if (bHaveUser)
				{
					int iUser = -1;
					iUser = FindUser(m_pWebEm->m_actualuser.c_str());
					if (iUser != -1)
						urights = static_cast<int>(m_users[iUser].userrights);
				}
				if (urights < 2)
					return;

				std::string idx = m_pWebEm->FindValue("idx");
				if (idx == "")
					return;
				std::string name = m_pWebEm->FindValue("name");
				std::string imagefile = m_pWebEm->FindValue("image");
				std::string scalefactor = m_pWebEm->FindValue("scalefactor");
				if (
					(name == "") ||
					(imagefile == "")
					)
					return;

				root["status"] = "OK";
				root["title"] = "UpdateFloorplan";

				sprintf(szTmp,
					"UPDATE Floorplans SET Name='%s',ImageFile='%s', ScaleFactor=%s WHERE (ID == %s)",
					name.c_str(),
					imagefile.c_str(),
					scalefactor.c_str(),
					idx.c_str()
					);
				result = m_sql.query(szTmp);
				_log.Log(LOG_STATUS, "(Floorplan) '%s' updated with image file '%s', Scale Factor %s.", name.c_str(), imagefile.c_str(), scalefactor.c_str());
			}
			else if (cparam == "deletefloorplan")
			{
				bool bHaveUser = (m_pWebEm->m_actualuser != "");
				int urights = 3;
				if (bHaveUser)
				{
					int iUser = -1;
					iUser = FindUser(m_pWebEm->m_actualuser.c_str());
					if (iUser != -1)
						urights = static_cast<int>(m_users[iUser].userrights);
				}
				if (urights < 2)
					return;

				std::string idx = m_pWebEm->FindValue("idx");
				if (idx == "")
					return;
				root["status"] = "OK";
				root["title"] = "DeleteFloorplan";
				sprintf(szTmp,
					"UPDATE DeviceToPlansMap SET XOffset=0,YOffset=0 WHERE (PlanID IN (SELECT ID from Plans WHERE (FloorplanID == %s)))",
					idx.c_str()
					);
				result = m_sql.query(szTmp);
				_log.Log(LOG_STATUS, "(Floorplan) Device coordinates reset for all plans on floorplan '%s'.", idx.c_str());
				sprintf(szTmp,
					"UPDATE Plans SET FloorplanID=0,Area='' WHERE (FloorplanID == %s)",
					idx.c_str()
					);
				result = m_sql.query(szTmp);
				_log.Log(LOG_STATUS, "(Floorplan) Plans for floorplan '%s' reset.", idx.c_str());
				sprintf(szTmp,
					"DELETE FROM Floorplans WHERE (ID == %s)",
					idx.c_str()
					);
				result = m_sql.query(szTmp);
				_log.Log(LOG_STATUS, "(Floorplan) Floorplan '%s' deleted.", idx.c_str());
			}
			else if (cparam == "changefloorplanorder")
			{
				std::string idx = m_pWebEm->FindValue("idx");
				if (idx == "")
					return;
				std::string sway = m_pWebEm->FindValue("way");
				if (sway == "")
					return;
				bool bGoUp = (sway == "0");

				std::string aOrder, oID, oOrder;

				std::vector<std::vector<std::string> > result;
				std::stringstream szQuery;
				szQuery << "SELECT [Order] FROM Floorplans WHERE (ID=='" << idx << "')";
				result = m_sql.query(szQuery.str());
				if (result.size() < 1)
					return;
				aOrder = result[0][0];

				szQuery.clear();
				szQuery.str("");

				if (!bGoUp)
				{
					//Get next device order
					szQuery << "SELECT ID, [Order] FROM Floorplans WHERE ([Order]>'" << aOrder << "') ORDER BY [Order] ASC";
					result = m_sql.query(szQuery.str());
					if (result.size() < 1)
						return;
					oID = result[0][0];
					oOrder = result[0][1];
				}
				else
				{
					//Get previous device order
					szQuery << "SELECT ID, [Order] FROM Floorplans WHERE ([Order]<'" << aOrder << "') ORDER BY [Order] DESC";
					result = m_sql.query(szQuery.str());
					if (result.size() < 1)
						return;
					oID = result[0][0];
					oOrder = result[0][1];
				}
				//Swap them
				root["status"] = "OK";
				root["title"] = "ChangeFloorPlanOrder";

				szQuery.clear();
				szQuery.str("");
				szQuery << "UPDATE Floorplans SET [Order] = '" << oOrder << "' WHERE (ID='" << idx << "')";
				result = m_sql.query(szQuery.str());
				szQuery.clear();
				szQuery.str("");
				szQuery << "UPDATE Floorplans SET [Order] = '" << aOrder << "' WHERE (ID='" << oID << "')";
				result = m_sql.query(szQuery.str());
			}
			else if (cparam == "getunusedfloorplanplans")
			{
				root["status"] = "OK";
				root["title"] = "GetUnusedFloorplanPlans";
				int ii = 0;

				szQuery.clear();
				szQuery.str("");
				szQuery << "SELECT ID, Name FROM Plans WHERE (FloorplanID==0) ORDER BY Name";
				result = m_sql.query(szQuery.str());
				if (result.size() > 0)
				{
					std::vector<std::vector<std::string> >::const_iterator itt;
					for (itt = result.begin(); itt != result.end(); ++itt)
					{
						std::vector<std::string> sd = *itt;

						root["result"][ii]["type"] = 0;
						root["result"][ii]["idx"] = sd[0];
						root["result"][ii]["Name"] = sd[1];
						ii++;
					}
				}
			}
			else if (cparam == "getfloorplanplans")
			{
				std::string idx = m_pWebEm->FindValue("idx");
				if (idx == "")
					return;
				root["status"] = "OK";
				root["title"] = "GetFloorplanPlans";
				std::vector<std::vector<std::string> > result;
				std::stringstream szQuery;
				int ii = 0;

				szQuery.clear();
				szQuery.str("");
				szQuery << "SELECT ID, Name, Area FROM Plans WHERE (FloorplanID==" << idx << ") ORDER BY Name";
				result = m_sql.query(szQuery.str());
				if (result.size() > 0)
				{
					std::vector<std::vector<std::string> >::const_iterator itt;
					for (itt = result.begin(); itt != result.end(); ++itt)
					{
						std::vector<std::string> sd = *itt;

						root["result"][ii]["idx"] = sd[0];
						root["result"][ii]["Name"] = sd[1];
						root["result"][ii]["Area"] = sd[2];
						ii++;
					}
				}
			}
			else if (cparam == "addfloorplanplan")
			{
				bool bHaveUser = (m_pWebEm->m_actualuser != "");
				int urights = 3;
				if (bHaveUser)
				{
					int iUser = -1;
					iUser = FindUser(m_pWebEm->m_actualuser.c_str());
					if (iUser != -1)
						urights = static_cast<int>(m_users[iUser].userrights);
				}
				if (urights < 2)
					return;

				std::string idx = m_pWebEm->FindValue("idx");
				std::string planidx = m_pWebEm->FindValue("planidx");
				if (
					(idx == "") ||
					(planidx == "")
					)
					return;
				root["status"] = "OK";
				root["title"] = "AddFloorplanPlan";

				sprintf(szTmp,
					"UPDATE Plans SET FloorplanID='%s' WHERE (ID == %s)",
					idx.c_str(),
					planidx.c_str()
					);
				result = m_sql.query(szTmp);
				_log.Log(LOG_STATUS, "(Floorplan) Plan '%s' added to floorplan '%s'.", planidx.c_str(), idx.c_str());
			}
			else if (cparam == "updatefloorplanplan")
			{
				bool bHaveUser = (m_pWebEm->m_actualuser != "");
				int urights = 3;
				if (bHaveUser)
				{
					int iUser = -1;
					iUser = FindUser(m_pWebEm->m_actualuser.c_str());
					if (iUser != -1)
						urights = static_cast<int>(m_users[iUser].userrights);
				}
				if (urights < 2)
					return;

				std::string planidx = m_pWebEm->FindValue("planidx");
				std::string planarea = m_pWebEm->FindValue("area");
				if (planidx == "")
					return;
				root["status"] = "OK";
				root["title"] = "UpdateFloorplanPlan";

				sprintf(szTmp,
					"UPDATE Plans SET Area='%s' WHERE (ID == %s)",
					planarea.c_str(),
					planidx.c_str()
					);
				result = m_sql.query(szTmp);
				_log.Log(LOG_STATUS, "(Floorplan) Plan '%s' floor area updated to '%s'.", planidx.c_str(), planarea.c_str());
			}
			else if (cparam == "deletefloorplanplan")
			{
				bool bHaveUser = (m_pWebEm->m_actualuser != "");
				int urights = 3;
				if (bHaveUser)
				{
					int iUser = -1;
					iUser = FindUser(m_pWebEm->m_actualuser.c_str());
					if (iUser != -1)
						urights = static_cast<int>(m_users[iUser].userrights);
				}
				if (urights < 2)
					return;

				std::string idx = m_pWebEm->FindValue("idx");
				if (idx == "")
					return;
				root["status"] = "OK";
				root["title"] = "DeleteFloorplanPlan";
				sprintf(szTmp,
					"UPDATE DeviceToPlansMap SET XOffset=0,YOffset=0 WHERE (PlanID == %s)",
					idx.c_str()
					);
				result = m_sql.query(szTmp);
				_log.Log(LOG_STATUS, "(Floorplan) Device coordinates reset for plan '%s'.", idx.c_str());
				sprintf(szTmp,
					"UPDATE Plans SET FloorplanID=0,Area='' WHERE (ID == %s)",
					idx.c_str()
					);
				result = m_sql.query(szTmp);
				_log.Log(LOG_STATUS, "(Floorplan) Plan '%s' floorplan data reset.", idx.c_str());
			}
		}

		char * CWebServer::DisplaySwitchTypesCombo()
		{
			m_retstr = "";
			char szTmp[200];

			std::map<std::string, int> _switchtypes;

			for (int ii = 0; ii < STYPE_END; ii++)
			{
				_switchtypes[Switch_Type_Desc((_eSwitchType)ii)] = ii;
			}
			//return a sorted list
			std::map<std::string, int>::const_iterator itt;
			for (itt = _switchtypes.begin(); itt != _switchtypes.end(); ++itt)
			{
				sprintf(szTmp, "<option value=\"%d\">%s</option>\n", itt->second, itt->first.c_str());
				m_retstr += szTmp;

			}
			return (char*)m_retstr.c_str();
		}

		char * CWebServer::DisplayMeterTypesCombo()
		{
			m_retstr = "";
			char szTmp[200];
			for (int ii = 0; ii < MTYPE_END; ii++)
			{
				sprintf(szTmp, "<option value=\"%d\">%s</option>\n", ii, Meter_Type_Desc((_eMeterType)ii));
				m_retstr += szTmp;
			}
			return (char*)m_retstr.c_str();
		}

		char * CWebServer::DisplayLanguageCombo()
		{
			m_retstr = "";
			//return a sorted list
			std::map<std::string, std::string> _ltypes;
			std::map<std::string, std::string>::const_iterator itt;
			char szTmp[200];
			int ii = 0;
			while (guiLanguage[ii].szShort != NULL)
			{
				_ltypes[guiLanguage[ii].szLong] = guiLanguage[ii].szShort;
				ii++;
			}
			for (itt = _ltypes.begin(); itt != _ltypes.end(); ++itt)
			{
				sprintf(szTmp, "<option value=\"%s\">%s</option>\n", itt->second.c_str(), itt->first.c_str());
				m_retstr += szTmp;

			}

			return (char*)m_retstr.c_str();
		}

		char * CWebServer::DisplayTimerTypesCombo()
		{
			m_retstr = "";
			char szTmp[200];
			for (int ii = 0; ii < TTYPE_END; ii++)
			{
				sprintf(szTmp, "<option data-i18n=\"%s\" value=\"%d\">%s</option>\n", Timer_Type_Desc(ii), ii, Timer_Type_Desc(ii));
				m_retstr += szTmp;
			}
			return (char*)m_retstr.c_str();
		}

		void CWebServer::LoadUsers()
		{
			ClearUserPasswords();
			std::string WebUserName, WebPassword;
			int nValue = 0;
			if (m_sql.GetPreferencesVar("WebUserName", nValue, WebUserName))
			{
				if (m_sql.GetPreferencesVar("WebPassword", nValue, WebPassword))
				{
					if ((WebUserName != "") && (WebPassword != ""))
					{
						WebUserName = base64_decode(WebUserName);
						WebPassword = WebPassword;
						AddUser(10000, WebUserName, WebPassword, URIGHTS_ADMIN, 0xFFFF);

						std::vector<std::vector<std::string> > result;
						result = m_sql.query("SELECT ID, Active, Username, Password, Rights, TabsEnabled FROM Users");
						if (result.size() > 0)
						{
							std::vector<std::vector<std::string> >::const_iterator itt;
							int ii = 0;
							for (itt = result.begin(); itt != result.end(); ++itt)
							{
								std::vector<std::string> sd = *itt;

								int bIsActive = static_cast<int>(atoi(sd[1].c_str()));
								if (bIsActive)
								{
									unsigned long ID = (unsigned long)atol(sd[0].c_str());

									std::string username = base64_decode(sd[2]);
									std::string password = sd[3];

									_eUserRights rights = (_eUserRights)atoi(sd[4].c_str());
									int activetabs = atoi(sd[5].c_str());

									AddUser(ID, username, password, rights, activetabs);
								}
							}
						}
					}
				}
			}
			m_mainworker.LoadSharedUsers();
		}

		void CWebServer::AddUser(const unsigned long ID, const std::string &username, const std::string &password, const int userrights, const int activetabs)
		{
			_tWebUserPassword wtmp;
			wtmp.ID = ID;
			wtmp.Username = username;
			wtmp.Password = password;
			wtmp.userrights = (_eUserRights)userrights;
			wtmp.ActiveTabs = activetabs;
			m_users.push_back(wtmp);

			m_pWebEm->AddUserPassword(ID, username, password, (_eUserRights)userrights, activetabs);
		}

		void CWebServer::ClearUserPasswords()
		{
			m_users.clear();
			m_pWebEm->ClearUserPasswords();
		}

		int CWebServer::FindUser(const char* szUserName)
		{
			int iUser = 0;
			std::vector<_tWebUserPassword>::const_iterator itt;
			for (itt = m_users.begin(); itt != m_users.end(); ++itt)
			{
				if (itt->Username == szUserName)
					return iUser;
				iUser++;
			}
			return -1;
		}

		bool CWebServer::FindAdminUser()
		{
			std::vector<_tWebUserPassword>::const_iterator itt;
			for (itt = m_users.begin(); itt != m_users.end(); ++itt)
			{
				if (itt->userrights == URIGHTS_ADMIN)
					return true;
			}
			return false;
		}

		char * CWebServer::PostSettings()
		{
			m_retstr = "/index.html";

			if (m_pWebEm->m_actualuser_rights != 2)
			{
				//No admin user, and not allowed to be here
				return (char*)m_retstr.c_str();
			}

			std::string Latitude = m_pWebEm->FindValue("Latitude");
			std::string Longitude = m_pWebEm->FindValue("Longitude");
			if ((Latitude != "") && (Longitude != ""))
			{
				std::string LatLong = Latitude + ";" + Longitude;
				m_sql.UpdatePreferencesVar("Location", LatLong.c_str());
				m_mainworker.GetSunSettings();
			}
			m_notifications.ConfigFromGetvars(m_pWebEm, true);
			std::string DashboardType = m_pWebEm->FindValue("DashboardType");
			m_sql.UpdatePreferencesVar("DashboardType", atoi(DashboardType.c_str()));
			std::string MobileType = m_pWebEm->FindValue("MobileType");
			m_sql.UpdatePreferencesVar("MobileType", atoi(MobileType.c_str()));

			int nUnit = atoi(m_pWebEm->FindValue("WindUnit").c_str());
			m_sql.UpdatePreferencesVar("WindUnit", nUnit);
			m_sql.m_windunit = (_eWindUnit)nUnit;

			nUnit = atoi(m_pWebEm->FindValue("TempUnit").c_str());
			m_sql.UpdatePreferencesVar("TempUnit", nUnit);
			m_sql.m_tempunit = (_eTempUnit)nUnit;

			m_sql.SetUnitsAndScale();

			std::string AuthenticationMethod = m_pWebEm->FindValue("AuthenticationMethod");
			_eAuthenticationMethod amethod = (_eAuthenticationMethod)atoi(AuthenticationMethod.c_str());
			m_sql.UpdatePreferencesVar("AuthenticationMethod", static_cast<int>(amethod));
			m_pWebEm->SetAuthenticationMethod(amethod);

			std::string ReleaseChannel = m_pWebEm->FindValue("ReleaseChannel");
			m_sql.UpdatePreferencesVar("ReleaseChannel", atoi(ReleaseChannel.c_str()));

			std::string LightHistoryDays = m_pWebEm->FindValue("LightHistoryDays");
			m_sql.UpdatePreferencesVar("LightHistoryDays", atoi(LightHistoryDays.c_str()));

			std::string s5MinuteHistoryDays = m_pWebEm->FindValue("ShortLogDays");
			m_sql.UpdatePreferencesVar("5MinuteHistoryDays", atoi(s5MinuteHistoryDays.c_str()));

			std::string sElectricVoltage = m_pWebEm->FindValue("ElectricVoltage");
			m_sql.UpdatePreferencesVar("ElectricVoltage", atoi(sElectricVoltage.c_str()));

			std::string sCM113DisplayType = m_pWebEm->FindValue("CM113DisplayType");
			m_sql.UpdatePreferencesVar("CM113DisplayType", atoi(sCM113DisplayType.c_str()));

			std::string WebUserName = m_pWebEm->FindValue("WebUserName");
			std::string WebPassword = m_pWebEm->FindValue("WebPassword");
			std::string WebLocalNetworks = m_pWebEm->FindValue("WebLocalNetworks");
			WebUserName = CURLEncode::URLDecode(WebUserName);
			WebPassword = CURLEncode::URLDecode(WebPassword);
			WebLocalNetworks = CURLEncode::URLDecode(WebLocalNetworks);

			if ((WebUserName == "") || (WebPassword == ""))
			{
				WebUserName = "";
				WebPassword = "";
			}
			WebUserName = base64_encode((const unsigned char*)WebUserName.c_str(), WebUserName.size());
			if (WebPassword.size() != 32)
			{
				WebPassword = GenerateMD5Hash(WebPassword);
			}
			m_sql.UpdatePreferencesVar("WebUserName", WebUserName.c_str());
			m_sql.UpdatePreferencesVar("WebPassword", WebPassword.c_str());
			m_sql.UpdatePreferencesVar("WebLocalNetworks", WebLocalNetworks.c_str());

			LoadUsers();
			m_pWebEm->ClearLocalNetworks();
			std::vector<std::string> strarray;
			StringSplit(WebLocalNetworks, ";", strarray);
			std::vector<std::string>::const_iterator itt;
			for (itt = strarray.begin(); itt != strarray.end(); ++itt)
			{
				std::string network = *itt;
				m_pWebEm->AddLocalNetworks(network);
			}
			//add local hostname
			m_pWebEm->AddLocalNetworks("");

			if (m_pWebEm->m_actualuser == "")
			{
				//Local network could be changed so lets for a check here
				m_pWebEm->m_actualuser_rights = -1;
			}

			std::string SecPassword = m_pWebEm->FindValue("SecPassword");
			SecPassword = CURLEncode::URLDecode(SecPassword);
			if (SecPassword.size() != 32)
			{
				SecPassword = GenerateMD5Hash(SecPassword);
			}
			m_sql.UpdatePreferencesVar("SecPassword", SecPassword.c_str());

			std::string ProtectionPassword = m_pWebEm->FindValue("ProtectionPassword");
			ProtectionPassword = CURLEncode::URLDecode(ProtectionPassword);
			if (ProtectionPassword.size() != 32)
			{
				ProtectionPassword = GenerateMD5Hash(ProtectionPassword);
			}
			m_sql.UpdatePreferencesVar("ProtectionPassword", ProtectionPassword.c_str());

			int EnergyDivider = atoi(m_pWebEm->FindValue("EnergyDivider").c_str());
			int GasDivider = atoi(m_pWebEm->FindValue("GasDivider").c_str());
			int WaterDivider = atoi(m_pWebEm->FindValue("WaterDivider").c_str());
			if (EnergyDivider < 1)
				EnergyDivider = 1000;
			if (GasDivider < 1)
				GasDivider = 100;
			if (WaterDivider < 1)
				WaterDivider = 100;
			m_sql.UpdatePreferencesVar("MeterDividerEnergy", EnergyDivider);
			m_sql.UpdatePreferencesVar("MeterDividerGas", GasDivider);
			m_sql.UpdatePreferencesVar("MeterDividerWater", WaterDivider);

			std::string scheckforupdates = m_pWebEm->FindValue("checkforupdates");
			m_sql.UpdatePreferencesVar("UseAutoUpdate", (scheckforupdates == "on" ? 1 : 0));

			std::string senableautobackup = m_pWebEm->FindValue("enableautobackup");
			m_sql.UpdatePreferencesVar("UseAutoBackup", (senableautobackup == "on" ? 1 : 0));

			float CostEnergy = static_cast<float>(atof(m_pWebEm->FindValue("CostEnergy").c_str()));
			float CostEnergyT2 = static_cast<float>(atof(m_pWebEm->FindValue("CostEnergyT2").c_str()));
			float CostGas = static_cast<float>(atof(m_pWebEm->FindValue("CostGas").c_str()));
			float CostWater = static_cast<float>(atof(m_pWebEm->FindValue("CostWater").c_str()));
			m_sql.UpdatePreferencesVar("CostEnergy", int(CostEnergy*10000.0f));
			m_sql.UpdatePreferencesVar("CostEnergyT2", int(CostEnergyT2*10000.0f));
			m_sql.UpdatePreferencesVar("CostGas", int(CostGas*10000.0f));
			m_sql.UpdatePreferencesVar("CostWater", int(CostWater*10000.0f));

			int rnOldvalue = 0;
			int rnvalue = 0;

			m_sql.GetPreferencesVar("ActiveTimerPlan", rnOldvalue);
			rnvalue = atoi(m_pWebEm->FindValue("ActiveTimerPlan").c_str());
			if (rnOldvalue != rnvalue)
			{
				m_sql.UpdatePreferencesVar("ActiveTimerPlan", rnvalue);
				m_sql.m_ActiveTimerPlan = rnvalue;
				m_mainworker.m_scheduler.ReloadSchedules();
			}
			m_sql.UpdatePreferencesVar("DoorbellCommand", atoi(m_pWebEm->FindValue("DoorbellCommand").c_str()));
			m_sql.UpdatePreferencesVar("SmartMeterType", atoi(m_pWebEm->FindValue("SmartMeterType").c_str()));

			std::string EnableTabFloorplans = m_pWebEm->FindValue("EnableTabFloorplans");
			m_sql.UpdatePreferencesVar("EnableTabFloorplans", (EnableTabFloorplans == "on" ? 1 : 0));
			std::string EnableTabLights = m_pWebEm->FindValue("EnableTabLights");
			m_sql.UpdatePreferencesVar("EnableTabLights", (EnableTabLights == "on" ? 1 : 0));
			std::string EnableTabTemp = m_pWebEm->FindValue("EnableTabTemp");
			m_sql.UpdatePreferencesVar("EnableTabTemp", (EnableTabTemp == "on" ? 1 : 0));
			std::string EnableTabWeather = m_pWebEm->FindValue("EnableTabWeather");
			m_sql.UpdatePreferencesVar("EnableTabWeather", (EnableTabWeather == "on" ? 1 : 0));
			std::string EnableTabUtility = m_pWebEm->FindValue("EnableTabUtility");
			m_sql.UpdatePreferencesVar("EnableTabUtility", (EnableTabUtility == "on" ? 1 : 0));
			std::string EnableTabScenes = m_pWebEm->FindValue("EnableTabScenes");
			m_sql.UpdatePreferencesVar("EnableTabScenes", (EnableTabScenes == "on" ? 1 : 0));

			m_sql.UpdatePreferencesVar("NotificationSensorInterval", atoi(m_pWebEm->FindValue("NotificationSensorInterval").c_str()));
			m_sql.UpdatePreferencesVar("NotificationSwitchInterval", atoi(m_pWebEm->FindValue("NotificationSwitchInterval").c_str()));

			std::string RaspCamParams = m_pWebEm->FindValue("RaspCamParams");
			if (RaspCamParams != "")
				m_sql.UpdatePreferencesVar("RaspCamParams", RaspCamParams.c_str());
			
            std::string UVCParams = m_pWebEm->FindValue("UVCParams");
			if (UVCParams != "")
				m_sql.UpdatePreferencesVar("UVCParams", UVCParams.c_str());

			std::string EnableNewHardware = m_pWebEm->FindValue("AcceptNewHardware");
			int iEnableNewHardware = (EnableNewHardware == "on" ? 1 : 0);
			m_sql.UpdatePreferencesVar("AcceptNewHardware", iEnableNewHardware);
			m_sql.m_bAcceptNewHardware = (iEnableNewHardware == 1);

			std::string HideDisabledHardwareSensors = m_pWebEm->FindValue("HideDisabledHardwareSensors");
			int iHideDisabledHardwareSensors = (HideDisabledHardwareSensors == "on" ? 1 : 0);
			m_sql.UpdatePreferencesVar("HideDisabledHardwareSensors", iHideDisabledHardwareSensors);

			rnOldvalue = 0;
			m_sql.GetPreferencesVar("DisableEventScriptSystem", rnOldvalue);
			std::string DisableEventScriptSystem = m_pWebEm->FindValue("DisableEventScriptSystem");
			int iDisableEventScriptSystem = (DisableEventScriptSystem == "on" ? 1 : 0);
			m_sql.UpdatePreferencesVar("DisableEventScriptSystem", iDisableEventScriptSystem);
			m_sql.m_bDisableEventSystem = (iDisableEventScriptSystem == 1);
			if (iDisableEventScriptSystem != rnOldvalue)
			{
				m_mainworker.m_eventsystem.SetEnabled(!m_sql.m_bDisableEventSystem);
				m_mainworker.m_eventsystem.StartEventSystem();
			}

			std::string EnableWidgetOrdering = m_pWebEm->FindValue("AllowWidgetOrdering");
			int iEnableAllowWidgetOrdering = (EnableWidgetOrdering == "on" ? 1 : 0);
			m_sql.UpdatePreferencesVar("AllowWidgetOrdering", iEnableAllowWidgetOrdering);
			m_sql.m_bAllowWidgetOrdering = (iEnableAllowWidgetOrdering == 1);

			rnOldvalue = 0;
			m_sql.GetPreferencesVar("RemoteSharedPort", rnOldvalue);

			m_sql.UpdatePreferencesVar("RemoteSharedPort", atoi(m_pWebEm->FindValue("RemoteSharedPort").c_str()));

			rnvalue = 0;
			m_sql.GetPreferencesVar("RemoteSharedPort", rnvalue);

			if (rnvalue != rnOldvalue)
			{
				if (rnvalue != 0)
				{
					char szPort[100];
					sprintf(szPort, "%d", rnvalue);
					m_mainworker.m_sharedserver.StopServer();
					m_mainworker.m_sharedserver.StartServer("0.0.0.0", szPort);
					m_mainworker.LoadSharedUsers();
				}
			}

			m_sql.UpdatePreferencesVar("Language", m_pWebEm->FindValue("Language").c_str());
			std::string SelectedTheme = m_pWebEm->FindValue("Themes");
			m_sql.UpdatePreferencesVar("WebTheme", SelectedTheme.c_str());
			m_pWebEm->SetWebTheme(SelectedTheme);

			m_sql.GetPreferencesVar("RandomTimerFrame", rnOldvalue);
			rnvalue = atoi(m_pWebEm->FindValue("RandomSpread").c_str());
			if (rnOldvalue != rnvalue)
			{
				m_sql.UpdatePreferencesVar("RandomTimerFrame", rnvalue);
				m_mainworker.m_scheduler.ReloadSchedules();
			}

			m_sql.UpdatePreferencesVar("SecOnDelay", atoi(m_pWebEm->FindValue("SecOnDelay").c_str()));

			int sensortimeout = atoi(m_pWebEm->FindValue("SensorTimeout").c_str());
			if (sensortimeout < 10)
				sensortimeout = 10;
			m_sql.UpdatePreferencesVar("SensorTimeout", sensortimeout);

			int batterylowlevel = atoi(m_pWebEm->FindValue("BatterLowLevel").c_str());
			if (batterylowlevel > 100)
				batterylowlevel = 100;
			m_sql.GetPreferencesVar("BatteryLowNotification", rnOldvalue);
			m_sql.UpdatePreferencesVar("BatteryLowNotification", batterylowlevel);
			if ((rnOldvalue != batterylowlevel) && (batterylowlevel!=0))
				m_sql.CheckBatteryLow();

			int nValue = 0;
			nValue = atoi(m_pWebEm->FindValue("FloorplanPopupDelay").c_str());
			m_sql.UpdatePreferencesVar("FloorplanPopupDelay", nValue);
			std::string FloorplanFullscreenMode = m_pWebEm->FindValue("FloorplanFullscreenMode");
			m_sql.UpdatePreferencesVar("FloorplanFullscreenMode", (FloorplanFullscreenMode == "on" ? 1 : 0));
			std::string FloorplanAnimateZoom = m_pWebEm->FindValue("FloorplanAnimateZoom");
			m_sql.UpdatePreferencesVar("FloorplanAnimateZoom", (FloorplanAnimateZoom == "on" ? 1 : 0));
			std::string FloorplanShowSensorValues = m_pWebEm->FindValue("FloorplanShowSensorValues");
			m_sql.UpdatePreferencesVar("FloorplanShowSensorValues", (FloorplanShowSensorValues == "on" ? 1 : 0));
			std::string FloorplanShowSwitchValues = m_pWebEm->FindValue("FloorplanShowSwitchValues");
			m_sql.UpdatePreferencesVar("FloorplanShowSwitchValues", (FloorplanShowSwitchValues == "on" ? 1 : 0));
			std::string FloorplanShowSceneNames = m_pWebEm->FindValue("FloorplanShowSceneNames");
			m_sql.UpdatePreferencesVar("FloorplanShowSceneNames", (FloorplanShowSceneNames == "on" ? 1 : 0));
			m_sql.UpdatePreferencesVar("FloorplanRoomColour", CURLEncode::URLDecode(m_pWebEm->FindValue("FloorplanRoomColour").c_str()).c_str());
			m_sql.UpdatePreferencesVar("FloorplanActiveOpacity", atoi(m_pWebEm->FindValue("FloorplanActiveOpacity").c_str()));
			m_sql.UpdatePreferencesVar("FloorplanInactiveOpacity", atoi(m_pWebEm->FindValue("FloorplanInactiveOpacity").c_str()));


			return (char*)m_retstr.c_str();
		}

		char * CWebServer::SBFSpotImportOldData()
		{
			m_retstr = "/index.html";
			if (m_pWebEm->m_actualuser_rights != 2)
			{
				//No admin user, and not allowed to be here
				return (char*)m_retstr.c_str();
			}

			std::string idx = m_pWebEm->FindValue("idx");
			if (idx == "") {
				return (char*)m_retstr.c_str();
			}
			int hardwareID = atoi(idx.c_str());
			CDomoticzHardwareBase *pHardware = m_mainworker.GetHardware(hardwareID);
			if (pHardware != NULL)
			{
				if (pHardware->HwdType == HTYPE_SBFSpot)
				{
					CSBFSpot *pSBFSpot = (CSBFSpot *)pHardware;
					pSBFSpot->ImportOldMonthData();
				}
			}
			return (char*)m_retstr.c_str();
		}

		char * CWebServer::SetOpenThermSettings()
		{
			m_retstr = "/index.html";
			if (m_pWebEm->m_actualuser_rights != 2)
			{
				//No admin user, and not allowed to be here
				return (char*)m_retstr.c_str();
			}

			std::string idx = m_pWebEm->FindValue("idx");
			if (idx == "") {
				return (char*)m_retstr.c_str();
			}
			std::vector<std::vector<std::string> > result;
			std::stringstream szQuery;

			szQuery.clear();
			szQuery.str("");
			szQuery << "SELECT Mode1, Mode2, Mode3, Mode4, Mode5, Mode6 FROM Hardware WHERE (ID=" << idx << ")";
			result = m_sql.query(szQuery.str());
			if (result.size() < 1)
				return (char*)m_retstr.c_str();


			int currentMode1 = atoi(result[0][0].c_str());

			std::string sOutsideTempSensor = m_pWebEm->FindValue("combooutsidesensor");
			int newMode1 = atoi(sOutsideTempSensor.c_str());

			if (currentMode1 != newMode1)
			{
				m_sql.UpdateRFXCOMHardwareDetails(atoi(idx.c_str()), newMode1, 0, 0, 0, 0, 0);
				m_mainworker.RestartHardware(idx);
			}

			return (char*)m_retstr.c_str();
		}

		char * CWebServer::SetRFXCOMMode()
		{
			m_retstr = "/index.html";

			if (m_pWebEm->m_actualuser_rights != 2)
			{
				//No admin user, and not allowed to be here
				return (char*)m_retstr.c_str();
			}

			std::string idx = m_pWebEm->FindValue("idx");
			if (idx == "") {
				return (char*)m_retstr.c_str();
			}
			std::vector<std::vector<std::string> > result;
			std::stringstream szQuery;

			szQuery.clear();
			szQuery.str("");
			szQuery << "SELECT Mode1, Mode2, Mode3, Mode4, Mode5, Mode6 FROM Hardware WHERE (ID=" << idx << ")";
			result = m_sql.query(szQuery.str());
			if (result.size() < 1)
				return (char*)m_retstr.c_str();

			unsigned char Mode1 = atoi(result[0][0].c_str());
			unsigned char Mode2 = atoi(result[0][1].c_str());
			unsigned char Mode3 = atoi(result[0][2].c_str());
			unsigned char Mode4 = atoi(result[0][3].c_str());
			unsigned char Mode5 = atoi(result[0][4].c_str());
			unsigned char Mode6 = atoi(result[0][5].c_str());

			tRBUF Response;
			Response.ICMND.msg1 = Mode1;
			Response.ICMND.msg2 = Mode2;
			Response.ICMND.msg3 = Mode3;
			Response.ICMND.msg4 = Mode4;
			Response.ICMND.msg5 = Mode5;
			Response.ICMND.msg6 = Mode6;

			Response.IRESPONSE.UNDECODEDenabled = (m_pWebEm->FindValue("undecon") == "on") ? 1 : 0;
			Response.IRESPONSE.X10enabled = (m_pWebEm->FindValue("X10") == "on") ? 1 : 0;
			Response.IRESPONSE.ARCenabled = (m_pWebEm->FindValue("ARC") == "on") ? 1 : 0;
			Response.IRESPONSE.ACenabled = (m_pWebEm->FindValue("AC") == "on") ? 1 : 0;
			Response.IRESPONSE.HEEUenabled = (m_pWebEm->FindValue("HomeEasyEU") == "on") ? 1 : 0;
			Response.IRESPONSE.MEIANTECHenabled = (m_pWebEm->FindValue("Meiantech") == "on") ? 1 : 0;
			Response.IRESPONSE.OREGONenabled = (m_pWebEm->FindValue("OregonScientific") == "on") ? 1 : 0;
			Response.IRESPONSE.ATIenabled = (m_pWebEm->FindValue("ATIremote") == "on") ? 1 : 0;
			Response.IRESPONSE.VISONICenabled = (m_pWebEm->FindValue("Visonic") == "on") ? 1 : 0;
			Response.IRESPONSE.MERTIKenabled = (m_pWebEm->FindValue("Mertik") == "on") ? 1 : 0;
			Response.IRESPONSE.LWRFenabled = (m_pWebEm->FindValue("ADLightwaveRF") == "on") ? 1 : 0;
			Response.IRESPONSE.HIDEKIenabled = (m_pWebEm->FindValue("HidekiUPM") == "on") ? 1 : 0;
			Response.IRESPONSE.LACROSSEenabled = (m_pWebEm->FindValue("LaCrosse") == "on") ? 1 : 0;
			Response.IRESPONSE.FS20enabled = (m_pWebEm->FindValue("FS20") == "on") ? 1 : 0;
			Response.IRESPONSE.PROGUARDenabled = (m_pWebEm->FindValue("ProGuard") == "on") ? 1 : 0;
			Response.IRESPONSE.BLINDST0enabled = (m_pWebEm->FindValue("BlindT0") == "on") ? 1 : 0;
			Response.IRESPONSE.BLINDST1enabled = (m_pWebEm->FindValue("BlindT1T2T3T4") == "on") ? 1 : 0;
			Response.IRESPONSE.AEenabled = (m_pWebEm->FindValue("AEBlyss") == "on") ? 1 : 0;
			Response.IRESPONSE.RUBICSONenabled = (m_pWebEm->FindValue("Rubicson") == "on") ? 1 : 0;
			Response.IRESPONSE.FINEOFFSETenabled = (m_pWebEm->FindValue("FineOffsetViking") == "on") ? 1 : 0;
			Response.IRESPONSE.LIGHTING4enabled = (m_pWebEm->FindValue("Lighting4") == "on") ? 1 : 0;
			Response.IRESPONSE.RSLenabled = (m_pWebEm->FindValue("RSL") == "on") ? 1 : 0;
			Response.IRESPONSE.SXenabled = (m_pWebEm->FindValue("ByronSX") == "on") ? 1 : 0;
			Response.IRESPONSE.IMAGINTRONIXenabled = (m_pWebEm->FindValue("ImaginTronix") == "on") ? 1 : 0;
			Response.IRESPONSE.KEELOQenabled = (m_pWebEm->FindValue("Keeloq") == "on") ? 1 : 0;

			m_mainworker.SetRFXCOMHardwaremodes(atoi(idx.c_str()), Response.ICMND.msg1, Response.ICMND.msg2, Response.ICMND.msg3, Response.ICMND.msg4, Response.ICMND.msg5, Response.ICMND.msg6);

			return (char*)m_retstr.c_str();
		}


		char * CWebServer::SetRego6XXType()
		{
			m_retstr = "/index.html";
			if (m_pWebEm->m_actualuser_rights != 2)
			{
				//No admin user, and not allowed to be here
				return (char*)m_retstr.c_str();
			}

			std::string idx = m_pWebEm->FindValue("idx");
			if (idx == "") {
				return (char*)m_retstr.c_str();
			}
			std::vector<std::vector<std::string> > result;
			std::stringstream szQuery;

			szQuery.clear();
			szQuery.str("");
			szQuery << "SELECT Mode1, Mode2, Mode3, Mode4, Mode5, Mode6 FROM Hardware WHERE (ID=" << idx << ")";
			result = m_sql.query(szQuery.str());
			if (result.size() < 1)
				return (char*)m_retstr.c_str();

			unsigned char currentMode1 = atoi(result[0][0].c_str());

			std::string sRego6XXType = m_pWebEm->FindValue("Rego6XXType");
			unsigned char newMode1 = atoi(sRego6XXType.c_str());

			if (currentMode1 != newMode1)
			{
				m_sql.UpdateRFXCOMHardwareDetails(atoi(idx.c_str()), newMode1, 0, 0, 0, 0, 0);
			}

			return (char*)m_retstr.c_str();
		}

		char * CWebServer::RestoreDatabase()
		{
			m_retstr = "/index.html";
			if (m_pWebEm->m_actualuser_rights != 2)
			{
				//No admin user, and not allowed to be here
				return (char*)m_retstr.c_str();
			}

			std::string dbasefile = m_pWebEm->FindValue("dbasefile");
			if (dbasefile == "") {
				return (char*)m_retstr.c_str();
			}
			if (!m_sql.RestoreDatabase(dbasefile))
				return (char*)m_retstr.c_str();
			return (char*)m_retstr.c_str();
		}

		char * CWebServer::SetP1USBType()
		{
			m_retstr = "/index.html";
			if (m_pWebEm->m_actualuser_rights != 2)
			{
				//No admin user, and not allowed to be here
				return (char*)m_retstr.c_str();
			}

			std::string idx = m_pWebEm->FindValue("idx");
			if (idx == "") {
				return (char*)m_retstr.c_str();
			}

			std::vector<std::vector<std::string> > result;
			std::stringstream szQuery;

			szQuery.clear();
			szQuery.str("");
			szQuery << "SELECT Mode1, Mode2, Mode3, Mode4, Mode5, Mode6 FROM Hardware WHERE (ID=" << idx << ")";
			result = m_sql.query(szQuery.str());
			if (result.size() < 1)
				return (char*)m_retstr.c_str();

			int Mode1 = atoi(m_pWebEm->FindValue("P1Baudrate").c_str());
			int Mode2 = 0;
			int Mode3 = 0;
			int Mode4 = 0;
			int Mode5 = 0;
			int Mode6 = 0;
			m_sql.UpdateRFXCOMHardwareDetails(atoi(idx.c_str()), Mode1, Mode2, Mode3, Mode4, Mode5, Mode6);

			m_mainworker.RestartHardware(idx);

			return (char*)m_retstr.c_str();
		}

		char * CWebServer::SetS0MeterType()
		{
			m_retstr = "/index.html";
			if (m_pWebEm->m_actualuser_rights != 2)
			{
				//No admin user, and not allowed to be here
				return (char*)m_retstr.c_str();
			}

			std::string idx = m_pWebEm->FindValue("idx");
			if (idx == "") {
				return (char*)m_retstr.c_str();
			}

			std::stringstream szQuery;
			std::stringstream szAddress;

			std::string S0M1Type = m_pWebEm->FindValue("S0M1Type");
			std::string S0M2Type = m_pWebEm->FindValue("S0M2Type");
			std::string S0M3Type = m_pWebEm->FindValue("S0M3Type");
			std::string S0M4Type = m_pWebEm->FindValue("S0M4Type");
			std::string S0M5Type = m_pWebEm->FindValue("S0M5Type");

			std::string M1PulsesPerHour = m_pWebEm->FindValue("M1PulsesPerHour");
			std::string M2PulsesPerHour = m_pWebEm->FindValue("M2PulsesPerHour");
			std::string M3PulsesPerHour = m_pWebEm->FindValue("M3PulsesPerHour");
			std::string M4PulsesPerHour = m_pWebEm->FindValue("M4PulsesPerHour");
			std::string M5PulsesPerHour = m_pWebEm->FindValue("M5PulsesPerHour");

			szAddress <<
				S0M1Type << ";" << M1PulsesPerHour << ";" <<
				S0M2Type << ";" << M2PulsesPerHour << ";" <<
				S0M3Type << ";" << M3PulsesPerHour << ";" <<
				S0M4Type << ";" << M4PulsesPerHour << ";" <<
				S0M5Type << ";" << M5PulsesPerHour;

			szQuery << "UPDATE Hardware SET Address='" << szAddress.str() << "' WHERE (ID=" << idx << ")";
			m_sql.query(szQuery.str());
			m_mainworker.RestartHardware(idx);
			return (char*)m_retstr.c_str();
		}

		char * CWebServer::SetLimitlessType()
		{
			m_retstr = "/index.html";
			if (m_pWebEm->m_actualuser_rights != 2)
			{
				//No admin user, and not allowed to be here
				return (char*)m_retstr.c_str();
			}
			std::string idx = m_pWebEm->FindValue("idx");
			if (idx == "") {
				return (char*)m_retstr.c_str();
			}

			std::vector<std::vector<std::string> > result;
			std::stringstream szQuery;

			szQuery.clear();
			szQuery.str("");
			szQuery << "SELECT Mode1, Mode2, Mode3, Mode4, Mode5, Mode6 FROM Hardware WHERE (ID=" << idx << ")";
			result = m_sql.query(szQuery.str());
			if (result.size() < 1)
				return (char*)m_retstr.c_str();

			int Mode1 = atoi(m_pWebEm->FindValue("LimitlessType").c_str());
			int Mode2 = atoi(result[0][1].c_str());
			int Mode3 = atoi(result[0][2].c_str());
			int Mode4 = atoi(result[0][3].c_str());
			m_sql.UpdateRFXCOMHardwareDetails(atoi(idx.c_str()), Mode1, Mode2, Mode3, Mode4, 0, 0);

			m_mainworker.RestartHardware(idx);

			return (char*)m_retstr.c_str();
		}

		struct _tHardwareListInt{
			std::string Name;
			bool Enabled;
		} tHardwareList;

		void CWebServer::GetJSonDevices(Json::Value &root, const std::string &rused, const std::string &rfilter, const std::string &order, const std::string &rowid, const std::string &planID, const std::string &floorID, const bool bDisplayHidden, const time_t LastUpdate, const bool bSkipUserCheck)
		{
			std::vector<std::vector<std::string> > result;
			std::stringstream szQuery;

			time_t now = mytime(NULL);
			struct tm tm1;
			localtime_r(&now, &tm1);
			struct tm tLastUpdate;
			localtime_r(&now, &tLastUpdate);

			const time_t iLastUpdate = LastUpdate - 1;

			int SensorTimeOut = 60;
			m_sql.GetPreferencesVar("SensorTimeout", SensorTimeOut);
			int HideDisabledHardwareSensors = 1;
			m_sql.GetPreferencesVar("HideDisabledHardwareSensors", HideDisabledHardwareSensors);

			//Get All Hardware ID's/Names, need them later
			std::map<int, _tHardwareListInt> _hardwareNames;
			szQuery.clear();
			szQuery.str("");
			szQuery << "SELECT ID, Name, Enabled FROM Hardware";
			result = m_sql.query(szQuery.str());
			if (result.size() > 0)
			{
				std::vector<std::vector<std::string> >::const_iterator itt;
				int ii = 0;
				for (itt = result.begin(); itt != result.end(); ++itt)
				{
					std::vector<std::string> sd = *itt;
					_tHardwareListInt tlist;
					int ID = atoi(sd[0].c_str());
					tlist.Name = sd[1];
					tlist.Enabled = (atoi(sd[2].c_str()) != 0);
					_hardwareNames[ID] = tlist;
				}
			}

			root["ActTime"] = static_cast<int>(now);

			char szData[100];
			char szTmp[300];

			if (!m_mainworker.m_LastSunriseSet.empty())
			{
				std::vector<std::string> strarray;
				StringSplit(m_mainworker.m_LastSunriseSet, ";", strarray);
				if (strarray.size() == 2)
				{
					strftime(szTmp, 80, "%b %d %Y %X", &tm1);
					root["ServerTime"] = szTmp;
					root["Sunrise"] = strarray[0];
					root["Sunset"] = strarray[1];
				}
			}

			char szOrderBy[50];
			if (order == "")
				strcpy(szOrderBy, "LastUpdate DESC");
			else
			{
				sprintf(szOrderBy, "[Order],%s ASC", order.c_str());
			}

			unsigned char tempsign = m_sql.m_tempsign[0];

			bool bHaveUser = false;
			int iUser = -1;
			unsigned int totUserDevices = 0;
			bool bShowScenes = true;
			if (!bSkipUserCheck)
			{
				bHaveUser = (m_pWebEm->m_actualuser != "");
				if (bHaveUser)
				{
					iUser = FindUser(m_pWebEm->m_actualuser.c_str());
					if (iUser != -1)
					{
						_eUserRights urights = m_users[iUser].userrights;
						if (urights != URIGHTS_ADMIN)
						{
							szQuery.clear();
							szQuery.str("");
							szQuery << "SELECT DeviceRowID FROM SharedDevices WHERE (SharedUserID == " << m_users[iUser].ID << ")";
							result = m_sql.query(szQuery.str());
							totUserDevices = (unsigned int)result.size();
							bShowScenes = (m_users[iUser].ActiveTabs&(1 << 1)) != 0;
						}
					}
				}
			}

			std::set<std::string> _HiddenDevices;
			bool bAllowDeviceToBeHidden = false;

			int ii = 0;
			if (rfilter == "all")
			{
				if (bShowScenes)
				{
					//add scenes
					szQuery.clear();
					szQuery.str("");

					if (rowid != "")
						szQuery << "SELECT ID, Name, nValue, LastUpdate, Favorite, SceneType, Protected, 0 as XOffset, 0 as YOffset, 0 as PlanID FROM Scenes WHERE (ID==" << rowid << ")";
					else if ((planID != "") && (planID != "0"))
						szQuery << "SELECT A.ID, A.Name, A.nValue, A.LastUpdate, A.Favorite, A.SceneType, A.Protected, B.XOffset, B.YOffset, B.PlanID FROM Scenes as A, DeviceToPlansMap as B WHERE (B.PlanID==" << planID << ") AND (B.DeviceRowID==a.ID) AND (B.DevSceneType==1) ORDER BY B.[Order]";
					else if ((floorID != "") && (floorID != "0"))
						szQuery << "SELECT A.ID, A.Name, A.nValue, A.LastUpdate, A.Favorite, A.SceneType, A.Protected, B.XOffset, B.YOffset, B.PlanID FROM Scenes as A, DeviceToPlansMap as B, Plans as C WHERE (C.FloorplanID==" << floorID << ") AND (C.ID==B.PlanID) AND(B.DeviceRowID==a.ID) AND (B.DevSceneType==1) ORDER BY B.[Order]";
					else
						szQuery << "SELECT ID, Name, nValue, LastUpdate, Favorite, SceneType, Protected, 0 as XOffset, 0 as YOffset, 0 as PlanID FROM Scenes ORDER BY " << szOrderBy;

					result = m_sql.query(szQuery.str());
					if (result.size() > 0)
					{
						std::vector<std::vector<std::string> >::const_iterator itt;
						for (itt = result.begin(); itt != result.end(); ++itt)
						{
							std::vector<std::string> sd = *itt;
							std::string sLastUpdate = sd[3];

							if (iLastUpdate != 0)
							{
								tLastUpdate.tm_isdst = tm1.tm_isdst;
								tLastUpdate.tm_year = atoi(sLastUpdate.substr(0, 4).c_str()) - 1900;
								tLastUpdate.tm_mon = atoi(sLastUpdate.substr(5, 2).c_str()) - 1;
								tLastUpdate.tm_mday = atoi(sLastUpdate.substr(8, 2).c_str());
								tLastUpdate.tm_hour = atoi(sLastUpdate.substr(11, 2).c_str());
								tLastUpdate.tm_min = atoi(sLastUpdate.substr(14, 2).c_str());
								tLastUpdate.tm_sec = atoi(sLastUpdate.substr(17, 2).c_str());
								time_t cLastUpdate = mktime(&tLastUpdate);
								if (cLastUpdate <= iLastUpdate)
									continue;
							}

							int nValue = atoi(sd[2].c_str());
							unsigned char favorite = atoi(sd[4].c_str());
							unsigned char scenetype = atoi(sd[5].c_str());
							int iProtected = atoi(sd[6].c_str());

							if (scenetype == 0)
							{
								root["result"][ii]["Type"] = "Scene";
							}
							else
							{
								root["result"][ii]["Type"] = "Group";
							}
							root["result"][ii]["idx"] = sd[0];
							root["result"][ii]["Name"] = sd[1];
							root["result"][ii]["Favorite"] = favorite;
							root["result"][ii]["Protected"] = (iProtected != 0);
							root["result"][ii]["LastUpdate"] = sLastUpdate;
							root["result"][ii]["TypeImg"] = "lightbulb";
							if (nValue == 0)
								root["result"][ii]["Status"] = "Off";
							else if (nValue == 1)
								root["result"][ii]["Status"] = "On";
							else
								root["result"][ii]["Status"] = "Mixed";
							unsigned long long camIDX = m_mainworker.m_cameras.IsDevSceneInCamera(1, sd[0]);
							root["result"][ii]["UsedByCamera"] = (camIDX != 0) ? true : false;
							if (camIDX != 0) {
								std::stringstream scidx;
								scidx << camIDX;
								root["result"][ii]["CameraIdx"] = scidx.str();
							}
							root["result"][ii]["XOffset"] = atoi(sd[7].c_str());
							root["result"][ii]["YOffset"] = atoi(sd[8].c_str());
							ii++;
						}
					}
				}
			}

			szQuery.clear();
			szQuery.str("");
			if (totUserDevices == 0)
			{
				//All
				if (rowid != "")
					szQuery << "SELECT ID, DeviceID, Unit, Name, Used, Type, SubType, SignalLevel, BatteryLevel, nValue, sValue, LastUpdate, Favorite, SwitchType, HardwareID, AddjValue, AddjMulti, AddjValue2, AddjMulti2, LastLevel, CustomImage, StrParam1, StrParam2, Protected, 0 as XOffset, 0 as YOffset, 0 as PlanID FROM DeviceStatus WHERE (ID==" << rowid << ")";
				else if ((planID != "") && (planID != "0"))
					szQuery << "SELECT A.ID, A.DeviceID, A.Unit, A.Name, A.Used, A.Type, A.SubType, A.SignalLevel, A.BatteryLevel, A.nValue, A.sValue, A.LastUpdate, A.Favorite, A.SwitchType, A.HardwareID, A.AddjValue, A.AddjMulti, A.AddjValue2, A.AddjMulti2, A.LastLevel, A.CustomImage, A.StrParam1, A.StrParam2, A.Protected, B.XOffset, B.YOffset, B.PlanID "
					"FROM DeviceStatus as A, DeviceToPlansMap as B WHERE (B.PlanID==" << planID << ") AND (B.DeviceRowID==a.ID) AND (B.DevSceneType==0) ORDER BY B.[Order]";
				else if ((floorID != "") && (floorID != "0"))
					szQuery << "SELECT A.ID, A.DeviceID, A.Unit, A.Name, A.Used, A.Type, A.SubType, A.SignalLevel, A.BatteryLevel, A.nValue, A.sValue, A.LastUpdate, A.Favorite, A.SwitchType, A.HardwareID, A.AddjValue, A.AddjMulti, A.AddjValue2, A.AddjMulti2, A.LastLevel, A.CustomImage, A.StrParam1, A.StrParam2, A.Protected, B.XOffset, B.YOffset, B.PlanID "
					"FROM DeviceStatus as A, DeviceToPlansMap as B, Plans as C WHERE (C.FloorplanID==" << floorID << ") AND (C.ID==B.PlanID) AND (B.DeviceRowID==a.ID) AND (B.DevSceneType==0) ORDER BY B.[Order]";
				else {
					if (!bDisplayHidden)
					{
						//Build a list of Hidden Devices
						szQuery << "SELECT ID FROM Plans WHERE (Name=='$Hidden Devices')";
						result = m_sql.query(szQuery.str());
						if (result.size() > 0)
						{
							std::string pID = result[0][0];
							szQuery.clear();
							szQuery.str("");
							szQuery << "SELECT DeviceRowID FROM DeviceToPlansMap WHERE (PlanID=='" << pID << "')";
							result = m_sql.query(szQuery.str());
							if (result.size() > 0)
							{
								std::vector<std::vector<std::string> >::const_iterator ittP;
								for (ittP = result.begin(); ittP != result.end(); ++ittP)
								{
									_HiddenDevices.insert(ittP[0][0]);
								}
							}
						}
						bAllowDeviceToBeHidden = true;
						szQuery.clear();
						szQuery.str("");
					}
					szQuery << "SELECT ID, DeviceID, Unit, Name, Used, Type, SubType, SignalLevel, BatteryLevel, nValue, sValue, LastUpdate, Favorite, SwitchType, HardwareID, AddjValue, AddjMulti, AddjValue2, AddjMulti2, LastLevel, CustomImage, StrParam1, StrParam2, Protected, 0 as XOffset, 0 as YOffset, 0 as PlanID FROM DeviceStatus ORDER BY " << szOrderBy;
				}
			}
			else
			{
				//Specific devices
				if (rowid != "")
					szQuery << "SELECT A.ID, A.DeviceID, A.Unit, A.Name, A.Used, A.Type, A.SubType, A.SignalLevel, A.BatteryLevel, A.nValue, A.sValue, A.LastUpdate, A.Favorite, A.SwitchType, A.HardwareID, A.AddjValue, A.AddjMulti, A.AddjValue2, A.AddjMulti2, A.LastLevel, A.CustomImage, A.StrParam1, A.StrParam2, A.Protected, 0 as XOffset, 0 as YOffset, 0 as PlanID FROM DeviceStatus as A, SharedDevices as B WHERE (B.DeviceRowID==a.ID) AND (B.SharedUserID==" << m_users[iUser].ID << ") AND (A.ID==" << rowid << ")";
				else if ((planID != "") && (planID != "0"))
					szQuery << "SELECT A.ID, A.DeviceID, A.Unit, A.Name, A.Used, A.Type, A.SubType, A.SignalLevel, A.BatteryLevel, A.nValue, A.sValue, A.LastUpdate, A.Favorite, A.SwitchType, A.HardwareID, A.AddjValue, A.AddjMulti, A.AddjValue2, A.AddjMulti2, A.LastLevel, A.CustomImage, A.StrParam1, A.StrParam2, A.Protected, C.XOffset, C.YOffset, C.PlanID "
					"FROM DeviceStatus as A, SharedDevices as B, DeviceToPlansMap as C  WHERE (C.PlanID==" << planID << ") AND (C.DeviceRowID==a.ID) AND (B.DeviceRowID==a.ID) AND (B.SharedUserID==" << m_users[iUser].ID << ") ORDER BY C.[Order]";
				else if ((floorID != "") && (floorID != "0"))
					szQuery << "SELECT A.ID, A.DeviceID, A.Unit, A.Name, A.Used, A.Type, A.SubType, A.SignalLevel, A.BatteryLevel, A.nValue, A.sValue, A.LastUpdate, A.Favorite, A.SwitchType, A.HardwareID, A.AddjValue, A.AddjMulti, A.AddjValue2, A.AddjMulti2, A.LastLevel, A.CustomImage, A.StrParam1, A.StrParam2, A.Protected, C.XOffset, C.YOffset, C.PlanID "
					"FROM DeviceStatus as A, SharedDevices as B, DeviceToPlansMap as C, Plans as D  WHERE (D.FloorplanID==" << floorID << ") AND (D.ID==C.PlanID) AND (C.DeviceRowID==a.ID) AND (B.DeviceRowID==a.ID) AND (B.SharedUserID==" << m_users[iUser].ID << ") ORDER BY C.[Order]";
				else {
					if (!bDisplayHidden)
					{
						//Build a list of Hidden Devices
						szQuery << "SELECT ID FROM Plans WHERE (Name=='$Hidden Devices')";
						result = m_sql.query(szQuery.str());
						if (result.size() > 0)
						{
							std::string pID = result[0][0];
							szQuery.clear();
							szQuery.str("");
							szQuery << "SELECT DeviceRowID FROM DeviceToPlansMap WHERE (PlanID=='" << pID << "')";
							result = m_sql.query(szQuery.str());
							if (result.size() > 0)
							{
								std::vector<std::vector<std::string> >::const_iterator ittP;
								for (ittP = result.begin(); ittP != result.end(); ++ittP)
								{
									_HiddenDevices.insert(ittP[0][0]);
								}
							}
						}
						bAllowDeviceToBeHidden = true;
					}
					szQuery.clear();
					szQuery.str("");
					szQuery << "SELECT A.ID, A.DeviceID, A.Unit, A.Name, A.Used, A.Type, A.SubType, A.SignalLevel, A.BatteryLevel, A.nValue, A.sValue, A.LastUpdate, A.Favorite, A.SwitchType, A.HardwareID, A.AddjValue, A.AddjMulti, A.AddjValue2, A.AddjMulti2, A.LastLevel, A.CustomImage, A.StrParam1, A.StrParam2, A.Protected, 0 as XOffset, 0 as YOffset, 0 as PlanID FROM DeviceStatus as A, SharedDevices as B WHERE (B.DeviceRowID==a.ID) AND (B.SharedUserID==" << m_users[iUser].ID << ") ORDER BY " << szOrderBy;
				}
			}

			result = m_sql.query(szQuery.str());
			if (result.size() > 0)
			{
				std::vector<std::vector<std::string> >::const_iterator itt;
				for (itt = result.begin(); itt != result.end(); ++itt)
				{
					std::vector<std::string> sd = *itt;
					std::string sDeviceName = sd[3];

					if (!bDisplayHidden)
					{
						if (_HiddenDevices.find(sd[0]) != _HiddenDevices.end())
							continue;
						if (sDeviceName[0] == '$')
						{
							if (bAllowDeviceToBeHidden)
								continue;
							if (planID.size() > 0)
								sDeviceName = sDeviceName.substr(1);
						}
					}
					int hardwareID = atoi(sd[14].c_str());
					std::map<int, _tHardwareListInt>::iterator hItt = _hardwareNames.find(hardwareID);
					if (hItt != _hardwareNames.end())
					{
						//ignore sensors where the hardware is disabled
						if (HideDisabledHardwareSensors)
						{
							if (!(*hItt).second.Enabled)
								continue;
						}
					}

					unsigned char dType = atoi(sd[5].c_str());
					unsigned char dSubType = atoi(sd[6].c_str());
					unsigned char used = atoi(sd[4].c_str());
					int nValue = atoi(sd[9].c_str());
					std::string sValue = sd[10];
					std::string sLastUpdate = sd[11];
					if (sLastUpdate.size() > 19)
						sLastUpdate = sLastUpdate.substr(0, 19);

					if (iLastUpdate != 0)
					{
						tLastUpdate.tm_isdst = tm1.tm_isdst;
						tLastUpdate.tm_year = atoi(sLastUpdate.substr(0, 4).c_str()) - 1900;
						tLastUpdate.tm_mon = atoi(sLastUpdate.substr(5, 2).c_str()) - 1;
						tLastUpdate.tm_mday = atoi(sLastUpdate.substr(8, 2).c_str());
						tLastUpdate.tm_hour = atoi(sLastUpdate.substr(11, 2).c_str());
						tLastUpdate.tm_min = atoi(sLastUpdate.substr(14, 2).c_str());
						tLastUpdate.tm_sec = atoi(sLastUpdate.substr(17, 2).c_str());
						time_t cLastUpdate = mktime(&tLastUpdate);
						if (cLastUpdate <= iLastUpdate)
							continue;
					}

					unsigned char favorite = atoi(sd[12].c_str());
					if ((planID != "") && (planID != "0"))
						favorite = 1;
					_eSwitchType switchtype = (_eSwitchType)atoi(sd[13].c_str());
					_eMeterType metertype = (_eMeterType)switchtype;
					double AddjValue = atof(sd[15].c_str());
					double AddjMulti = atof(sd[16].c_str());
					double AddjValue2 = atof(sd[17].c_str());
					double AddjMulti2 = atof(sd[18].c_str());
					int LastLevel = atoi(sd[19].c_str());
					int CustomImage = atoi(sd[20].c_str());
					std::string strParam1 = base64_encode((const unsigned char*)sd[21].c_str(), sd[21].size());
					std::string strParam2 = base64_encode((const unsigned char*)sd[22].c_str(), sd[22].size());
					int iProtected = atoi(sd[23].c_str());

					struct tm ntime;
					ntime.tm_isdst = tm1.tm_isdst;
					ntime.tm_year = atoi(sLastUpdate.substr(0, 4).c_str()) - 1900;
					ntime.tm_mon = atoi(sLastUpdate.substr(5, 2).c_str()) - 1;
					ntime.tm_mday = atoi(sLastUpdate.substr(8, 2).c_str());
					ntime.tm_hour = atoi(sLastUpdate.substr(11, 2).c_str());
					ntime.tm_min = atoi(sLastUpdate.substr(14, 2).c_str());
					ntime.tm_sec = atoi(sLastUpdate.substr(17, 2).c_str());
					time_t checktime = mktime(&ntime);
					bool bHaveTimeout = (now - checktime >= SensorTimeOut * 60);


					if (dType == pTypeTEMP_RAIN)
						continue; //dont want you for now

					if ((rused == "true") && (!used))
						continue;

					if (
						(rused == "false") &&
						(used)
						)
						continue;
					if (rfilter != "")
					{
						if (rfilter == "light")
						{
							if (
								(dType != pTypeLighting1) &&
								(dType != pTypeLighting2) &&
								(dType != pTypeLighting3) &&
								(dType != pTypeLighting4) &&
								(dType != pTypeLighting5) &&
								(dType != pTypeLighting6) &&
								(dType != pTypeLimitlessLights) &&
								(dType != pTypeSecurity1) &&
								(dType != pTypeSecurity2) &&
								(dType != pTypeEvohome) &&
								(dType != pTypeEvohomeRelay) &&
								(dType != pTypeCurtain) &&
								(dType != pTypeBlinds) &&
								(dType != pTypeRFY) &&
								(dType != pTypeChime) &&
								(dType != pTypeThermostat2) &&
								(dType != pTypeThermostat3) &&
								(dType != pTypeRemote) &&
								(dType != pTypeGeneralSwitch) &&
								(dType != pTypeChime) &&
								(!((dType == pTypeRego6XXValue) && (dSubType == sTypeRego6XXStatus))) &&
								(!((dType == pTypeRadiator1) && (dSubType == sTypeSmartwaresSwitchRadiator)))
								)
								continue;
						}
						else if (rfilter == "temp")
						{
							if (
								(dType != pTypeTEMP) &&
								(dType != pTypeHUM) &&
								(dType != pTypeTEMP_HUM) &&
								(dType != pTypeTEMP_HUM_BARO) &&
								(dType != pTypeTEMP_BARO) &&
								(dType != pTypeEvohomeZone) &&
								(dType != pTypeEvohomeWater) &&
								(!((dType == pTypeWIND) && (dSubType == sTypeWIND4))) &&
								(!((dType == pTypeWIND) && (dSubType == sTypeWINDNoTemp))) &&
								(!((dType == pTypeUV) && (dSubType == sTypeUV3))) &&
								(!((dType == pTypeGeneral) && (dSubType == sTypeSystemTemp))) &&
								(dType != pTypeThermostat1) &&
								(!((dType == pTypeRFXSensor) && (dSubType == sTypeRFXSensorTemp))) &&
								(dType != pTypeRego6XXTemp)
								)
								continue;
						}
						else if (rfilter == "weather")
						{
							if (
								(dType != pTypeWIND) &&
								(dType != pTypeRAIN) &&
								(dType != pTypeTEMP_HUM_BARO) &&
								(dType != pTypeTEMP_BARO) &&
								(dType != pTypeUV) &&
								(!((dType == pTypeGeneral) && (dSubType == sTypeVisibility))) &&
								(!((dType == pTypeGeneral) && (dSubType == sTypeBaro))) &&
								(!((dType == pTypeGeneral) && (dSubType == sTypeSolarRadiation)))
								)
								continue;
						}
						else if (rfilter == "utility")
						{
							if (
								(dType != pTypeRFXMeter) &&
								(!((dType == pTypeRFXSensor) && (dSubType == sTypeRFXSensorAD))) &&
								(!((dType == pTypeRFXSensor) && (dSubType == sTypeRFXSensorVolt))) &&
								(!((dType == pTypeGeneral) && (dSubType == sTypeVoltage))) &&
								(!((dType == pTypeGeneral) && (dSubType == sTypeCurrent))) &&
								(!((dType == pTypeGeneral) && (dSubType == sTypeTextStatus))) &&
								(!((dType == pTypeGeneral) && (dSubType == sTypeAlert))) &&
								(!((dType == pTypeGeneral) && (dSubType == sTypePressure))) &&
								(!((dType == pTypeGeneral) && (dSubType == sTypeSoilMoisture))) &&
								(!((dType == pTypeGeneral) && (dSubType == sTypeLeafWetness))) &&
								(!((dType == pTypeGeneral) && (dSubType == sTypePercentage))) &&
								(!((dType == pTypeGeneral) && (dSubType == sTypeFan))) &&
								(!((dType == pTypeGeneral) && (dSubType == sTypeSoundLevel))) &&
								(!((dType == pTypeGeneral) && (dSubType == sTypeZWaveClock))) &&
								(!((dType == pTypeGeneral) && (dSubType == sTypeZWaveThermostatMode))) &&
								(!((dType == pTypeGeneral) && (dSubType == sTypeZWaveThermostatFanMode))) &&
								(!((dType == pTypeGeneral) && (dSubType == sTypeDistance))) &&
                                (!((dType == pTypeGeneral) && (dSubType == sTypeCounterIncremental))) &&
								(dType != pTypeCURRENT) &&
								(dType != pTypeCURRENTENERGY) &&
								(dType != pTypeENERGY) &&
								(dType != pTypePOWER) &&
								(dType != pTypeP1Power) &&
								(dType != pTypeP1Gas) &&
								(dType != pTypeYouLess) &&
								(dType != pTypeAirQuality) &&
								(dType != pTypeLux) &&
								(dType != pTypeUsage) &&
								(!((dType == pTypeRego6XXValue) && (dSubType == sTypeRego6XXCounter))) &&
								(!((dType == pTypeThermostat) && (dSubType == sTypeThermSetpoint))) &&
								(dType != pTypeWEIGHT)&&
								(!((dType == pTypeRadiator1) && (dSubType == sTypeSmartwares)))
								)
								continue;
						}
						else if (rfilter == "wind")
						{
							if (
								(dType != pTypeWIND)
								)
								continue;
						}
						else if (rfilter == "rain")
						{
							if (
								(dType != pTypeRAIN)
								)
								continue;
						}
						else if (rfilter == "uv")
						{
							if (
								(dType != pTypeUV)
								)
								continue;
						}
						else if (rfilter == "baro")
						{
							if (
								(dType != pTypeTEMP_HUM_BARO) &&
								(dType != pTypeTEMP_BARO)
								)
								continue;
						}
					}

					root["result"][ii]["HardwareID"] = hardwareID;
					if (_hardwareNames.find(hardwareID) == _hardwareNames.end())
						root["result"][ii]["HardwareName"] = "Unknown?";
					else
						root["result"][ii]["HardwareName"] = _hardwareNames[hardwareID].Name;
					root["result"][ii]["idx"] = sd[0];
					root["result"][ii]["Protected"] = (iProtected != 0);

					CDomoticzHardwareBase *pHardware = m_mainworker.GetHardware(hardwareID);
					if (pHardware != NULL)
					{
						if (pHardware->HwdType == HTYPE_Wunderground)
						{
							CWunderground *pWHardware = (CWunderground *)pHardware;
							std::string forecast_url = pWHardware->GetForecastURL();
							if (forecast_url != "")
							{
								root["result"][ii]["forecast_url"] = base64_encode((const unsigned char*)forecast_url.c_str(), forecast_url.size());
							}
						}
						else if (pHardware->HwdType == HTYPE_ForecastIO)
						{
							CForecastIO *pWHardware = (CForecastIO*)pHardware;
							std::string forecast_url = pWHardware->GetForecastURL();
							if (forecast_url != "")
							{
								root["result"][ii]["forecast_url"] = base64_encode((const unsigned char*)forecast_url.c_str(), forecast_url.size());
							}
						}
					}

					sprintf(szData, "%04X", (unsigned int)atoi(sd[1].c_str()));
					if (
						(dType == pTypeTEMP) ||
						(dType == pTypeTEMP_BARO) ||
						(dType == pTypeTEMP_HUM) ||
						(dType == pTypeTEMP_HUM_BARO) ||
						(dType == pTypeBARO) ||
						(dType == pTypeHUM) ||
						(dType == pTypeWIND) ||
						(dType == pTypeCURRENT) ||
						(dType == pTypeCURRENTENERGY) ||
						(dType == pTypeENERGY) ||
						(dType == pTypeRFXMeter) ||
						(dType == pTypeRFXSensor)
						)
					{
						root["result"][ii]["ID"] = szData;
					}
					else
					{
						root["result"][ii]["ID"] = sd[1];
					}
					root["result"][ii]["Unit"] = atoi(sd[2].c_str());
					root["result"][ii]["Type"] = RFX_Type_Desc(dType, 1);
					root["result"][ii]["SubType"] = RFX_Type_SubType_Desc(dType, dSubType);
					root["result"][ii]["TypeImg"] = RFX_Type_Desc(dType, 2);
					root["result"][ii]["Name"] = sDeviceName;
					root["result"][ii]["Used"] = used;
					root["result"][ii]["Favorite"] = favorite;
					root["result"][ii]["SignalLevel"] = atoi(sd[7].c_str());
					root["result"][ii]["BatteryLevel"] = atoi(sd[8].c_str());
					root["result"][ii]["LastUpdate"] = sLastUpdate;
					root["result"][ii]["CustomImage"] = CustomImage;
					root["result"][ii]["XOffset"] = sd[24].c_str();
					root["result"][ii]["YOffset"] = sd[25].c_str();
					root["result"][ii]["PlanID"] = sd[26].c_str();
					sprintf(szData, "%d, %s", nValue, sValue.c_str());
					root["result"][ii]["Data"] = szData;

					root["result"][ii]["Notifications"] = (m_sql.HasNotifications(sd[0]) == true) ? "true" : "false";
					root["result"][ii]["ShowNotifications"] = true;

					bool bHasTimers = false;

					if (
						(dType == pTypeLighting1) ||
						(dType == pTypeLighting2) ||
						(dType == pTypeLighting3) ||
						(dType == pTypeLighting4) ||
						(dType == pTypeLighting5) ||
						(dType == pTypeLighting6) ||
						(dType == pTypeLimitlessLights) ||
						(dType == pTypeCurtain) ||
						(dType == pTypeBlinds) ||
						(dType == pTypeRFY) ||
						(dType == pTypeChime) ||
						(dType == pTypeThermostat2) ||
						(dType == pTypeThermostat3) ||
						(dType == pTypeRemote)||
						(dType == pTypeGeneralSwitch) ||
						((dType == pTypeRadiator1) && (dSubType == sTypeSmartwaresSwitchRadiator))
						)
					{
						//add light details
						bHasTimers = m_sql.HasTimers(sd[0]);

						bHaveTimeout = false;
#ifdef WITH_OPENZWAVE
						if (pHardware != NULL)
						{
							if (pHardware->HwdType == HTYPE_OpenZWave)
							{
								COpenZWave *pZWave = (COpenZWave*)pHardware;
								unsigned long ID;
								std::stringstream s_strid;
								s_strid << std::hex << sd[1];
								s_strid >> ID;
								int nodeID = (ID & 0x0000FF00) >> 8;
								bHaveTimeout = pZWave->HasNodeFailed(nodeID);
							}
						}
#endif
						root["result"][ii]["HaveTimeout"] = bHaveTimeout;

						std::string lstatus = "";
						int llevel = 0;
						bool bHaveDimmer = false;
						bool bHaveGroupCmd = false;
						int maxDimLevel = 0;

						GetLightStatus(dType, dSubType, switchtype, nValue, sValue, lstatus, llevel, bHaveDimmer, maxDimLevel, bHaveGroupCmd);

						root["result"][ii]["Status"] = lstatus;
						root["result"][ii]["StrParam1"] = strParam1;
						root["result"][ii]["StrParam2"] = strParam2;

						std::string IconFile = "Light";
						std::map<int, int>::const_iterator ittIcon = m_custom_light_icons_lookup.find(CustomImage);
						if (ittIcon != m_custom_light_icons_lookup.end())
						{
							IconFile = m_custom_light_icons[ittIcon->second].RootFile;
						}
						root["result"][ii]["Image"] = IconFile;


						if (switchtype == STYPE_Dimmer)
						{
							root["result"][ii]["Level"] = LastLevel;
							int iLevel = round((float(maxDimLevel) / 100.0f)*LastLevel);
							root["result"][ii]["LevelInt"] = iLevel;
							if (dType == pTypeLimitlessLights)
							{
								llevel = LastLevel;
								if (lstatus == "Set Level")
								{
									sprintf(szTmp, "Set Level: %d %%", LastLevel);
									root["result"][ii]["Status"] = szTmp;
								}
							}
						}
						else
						{
							root["result"][ii]["Level"] = llevel;
							root["result"][ii]["LevelInt"] = atoi(sValue.c_str());
						}
						root["result"][ii]["HaveDimmer"] = bHaveDimmer;
						root["result"][ii]["MaxDimLevel"] = maxDimLevel;
						root["result"][ii]["HaveGroupCmd"] = bHaveGroupCmd;
						root["result"][ii]["SwitchType"] = Switch_Type_Desc(switchtype);
						root["result"][ii]["SwitchTypeVal"] = switchtype;
						unsigned long long camIDX = m_mainworker.m_cameras.IsDevSceneInCamera(0, sd[0]);
						root["result"][ii]["UsedByCamera"] = (camIDX != 0) ? true : false;
						if (camIDX != 0) {
							std::stringstream scidx;
							scidx << camIDX;
							root["result"][ii]["CameraIdx"] = scidx.str();;
						}

						bool bIsSubDevice = false;
						std::vector<std::vector<std::string> > resultSD;
						std::stringstream szQuerySD;

						szQuerySD.clear();
						szQuerySD.str("");
						szQuerySD << "SELECT ID FROM LightSubDevices WHERE (DeviceRowID=='" << sd[0] << "')";
						resultSD = m_sql.query(szQuerySD.str());
						bIsSubDevice = (resultSD.size() > 0);

						root["result"][ii]["IsSubDevice"] = bIsSubDevice;

						if (switchtype == STYPE_OnOff)
						{
							root["result"][ii]["AddjValue"] = AddjValue;
							root["result"][ii]["AddjMulti"] = AddjMulti;
							root["result"][ii]["AddjValue2"] = AddjValue2;
							root["result"][ii]["AddjMulti2"] = AddjMulti2;
						}
						else if (switchtype == STYPE_Doorbell)
						{
							root["result"][ii]["TypeImg"] = "doorbell";
							root["result"][ii]["Status"] = "";//"Pressed";
						}
						else if (switchtype == STYPE_DoorLock)
						{
							root["result"][ii]["TypeImg"] = "door";
							bool bIsOn = IsLightSwitchOn(lstatus);
							root["result"][ii]["InternalState"] = (bIsOn == true) ? "Open" : "Closed";
							if (bIsOn) {
								lstatus = "Open";
							}
							else {
								lstatus = "Closed";
							}
							root["result"][ii]["Status"] = lstatus;
							root["result"][ii]["AddjValue"] = AddjValue;
							root["result"][ii]["AddjMulti"] = AddjMulti;
							root["result"][ii]["AddjValue2"] = AddjValue2;
							root["result"][ii]["AddjMulti2"] = AddjMulti2;
						}
						else if (switchtype == STYPE_PushOn)
						{
							root["result"][ii]["TypeImg"] = "push";
							root["result"][ii]["Status"] = "";
							root["result"][ii]["InternalState"] = (IsLightSwitchOn(lstatus) == true) ? "On" : "Off";
							root["result"][ii]["AddjValue"] = AddjValue;
							root["result"][ii]["AddjMulti"] = AddjMulti;
							root["result"][ii]["AddjValue2"] = AddjValue2;
							root["result"][ii]["AddjMulti2"] = AddjMulti2;
						}
						else if (switchtype == STYPE_PushOff)
						{
							root["result"][ii]["Status"] = "";
							root["result"][ii]["TypeImg"] = "pushoff";
						}
						else if (switchtype == STYPE_X10Siren)
							root["result"][ii]["TypeImg"] = "siren";
						else if (switchtype == STYPE_SMOKEDETECTOR)
						{
							root["result"][ii]["TypeImg"] = "smoke";
							root["result"][ii]["SwitchTypeVal"] = STYPE_SMOKEDETECTOR;
							root["result"][ii]["SwitchType"] = Switch_Type_Desc(STYPE_SMOKEDETECTOR);
							root["result"][ii]["AddjValue"] = AddjValue;
							root["result"][ii]["AddjMulti"] = AddjMulti;
							root["result"][ii]["AddjValue2"] = AddjValue2;
							root["result"][ii]["AddjMulti2"] = AddjMulti2;
						}
						else if (switchtype == STYPE_Contact)
						{
							root["result"][ii]["TypeImg"] = "contact";
							bool bIsOn = IsLightSwitchOn(lstatus);
							if (bIsOn) {
								lstatus = "Open";
							}
							else {
								lstatus = "Closed";
							}
							root["result"][ii]["Status"] = lstatus;
							root["result"][ii]["AddjValue"] = AddjValue;
							root["result"][ii]["AddjMulti"] = AddjMulti;
							root["result"][ii]["AddjValue2"] = AddjValue2;
							root["result"][ii]["AddjMulti2"] = AddjMulti2;
						}
						else if (
							(switchtype == STYPE_Blinds) ||
							(switchtype == STYPE_VenetianBlindsUS) ||
							(switchtype == STYPE_VenetianBlindsEU)
							)
						{
							root["result"][ii]["TypeImg"] = "blinds";
							if (lstatus == "On") {
								lstatus = "Closed";
							}
							else {
								lstatus = "Open";
							}
							root["result"][ii]["Status"] = lstatus;
						}
						else if (switchtype == STYPE_BlindsInverted)
						{
							root["result"][ii]["TypeImg"] = "blinds";
							if (lstatus == "On") {
								lstatus = "Open";
							}
							else {
								lstatus = "Closed";
							}
							root["result"][ii]["Status"] = lstatus;
						}
						else if ((switchtype == STYPE_BlindsPercentage) || (switchtype == STYPE_BlindsPercentageInverted))
						{
							root["result"][ii]["TypeImg"] = "blinds";
							root["result"][ii]["Level"] = LastLevel;
							int iLevel = round((float(maxDimLevel) / 100.0f)*LastLevel);
							root["result"][ii]["LevelInt"] = iLevel;
							if (lstatus == "On") {
								lstatus = (switchtype == STYPE_BlindsPercentage) ? "Closed" : "Open";
							}
							else if (lstatus == "Off") {
								lstatus = (switchtype == STYPE_BlindsPercentage) ? "Open" : "Closed";
							}

							root["result"][ii]["Status"] = lstatus;
						}
						else if (switchtype == STYPE_Dimmer)
						{
							root["result"][ii]["TypeImg"] = "dimmer";
							root["result"][ii]["AddjValue"] = AddjValue;
							root["result"][ii]["AddjMulti"] = AddjMulti;
							root["result"][ii]["AddjValue2"] = AddjValue2;
							root["result"][ii]["AddjMulti2"] = AddjMulti2;
						}
						else if (switchtype == STYPE_Motion)
						{
							root["result"][ii]["TypeImg"] = "motion";
							root["result"][ii]["AddjValue"] = AddjValue;
							root["result"][ii]["AddjMulti"] = AddjMulti;
							root["result"][ii]["AddjValue2"] = AddjValue2;
							root["result"][ii]["AddjMulti2"] = AddjMulti2;
						}
						if (llevel != 0)
							sprintf(szData, "%s, Level: %d %%", lstatus.c_str(), llevel);
						else
							sprintf(szData, "%s", lstatus.c_str());
						root["result"][ii]["Data"] = szData;
					}
					else if (dType == pTypeSecurity1)
					{
						std::string lstatus = "";
						int llevel = 0;
						bool bHaveDimmer = false;
						bool bHaveGroupCmd = false;
						int maxDimLevel = 0;

						GetLightStatus(dType, dSubType, switchtype, nValue, sValue, lstatus, llevel, bHaveDimmer, maxDimLevel, bHaveGroupCmd);

						root["result"][ii]["Status"] = lstatus;
						root["result"][ii]["HaveDimmer"] = bHaveDimmer;
						root["result"][ii]["MaxDimLevel"] = maxDimLevel;
						root["result"][ii]["HaveGroupCmd"] = bHaveGroupCmd;
						root["result"][ii]["SwitchType"] = "Security";
						root["result"][ii]["SwitchTypeVal"] = switchtype; //was 0?;
						root["result"][ii]["TypeImg"] = "security";
						root["result"][ii]["StrParam1"] = strParam1;
						root["result"][ii]["StrParam2"] = strParam2;
						root["result"][ii]["Protected"] = (iProtected != 0);

						if ((dSubType == sTypeKD101) || (dSubType == sTypeSA30) || (switchtype == STYPE_SMOKEDETECTOR))
						{
							root["result"][ii]["SwitchTypeVal"] = STYPE_SMOKEDETECTOR;
							root["result"][ii]["TypeImg"] = "smoke";
							root["result"][ii]["SwitchType"] = Switch_Type_Desc(STYPE_SMOKEDETECTOR);
							root["result"][ii]["AddjValue"] = AddjValue;
							root["result"][ii]["AddjMulti"] = AddjMulti;
							root["result"][ii]["AddjValue2"] = AddjValue2;
							root["result"][ii]["AddjMulti2"] = AddjMulti2;
						}
						if (switchtype == STYPE_Motion)
						{
							root["result"][ii]["AddjValue"] = AddjValue;
							root["result"][ii]["AddjMulti"] = AddjMulti;
							root["result"][ii]["AddjValue2"] = AddjValue2;
							root["result"][ii]["AddjMulti2"] = AddjMulti2;
						}

						sprintf(szData, "%s", lstatus.c_str());
						root["result"][ii]["Data"] = szData;
						root["result"][ii]["HaveTimeout"] = false;
					}
					else if (dType == pTypeSecurity2)
					{
						std::string lstatus = "";
						int llevel = 0;
						bool bHaveDimmer = false;
						bool bHaveGroupCmd = false;
						int maxDimLevel = 0;

						GetLightStatus(dType, dSubType, switchtype, nValue, sValue, lstatus, llevel, bHaveDimmer, maxDimLevel, bHaveGroupCmd);

						root["result"][ii]["Status"] = lstatus;
						root["result"][ii]["HaveDimmer"] = bHaveDimmer;
						root["result"][ii]["MaxDimLevel"] = maxDimLevel;
						root["result"][ii]["HaveGroupCmd"] = bHaveGroupCmd;
						root["result"][ii]["SwitchType"] = "Security";
						root["result"][ii]["SwitchTypeVal"] = switchtype; //was 0?;
						root["result"][ii]["TypeImg"] = "security";
						root["result"][ii]["StrParam1"] = strParam1;
						root["result"][ii]["StrParam2"] = strParam2;
						root["result"][ii]["Protected"] = (iProtected != 0);
						sprintf(szData, "%s", lstatus.c_str());
						root["result"][ii]["Data"] = szData;
						root["result"][ii]["HaveTimeout"] = false;
					}
					else if (dType == pTypeEvohome || dType == pTypeEvohomeRelay)
					{
						std::string lstatus="";
						int llevel=0;
						bool bHaveDimmer=false;
						bool bHaveGroupCmd=false;
						int maxDimLevel=0;

						GetLightStatus(dType, dSubType, switchtype,nValue, sValue, lstatus, llevel, bHaveDimmer, maxDimLevel, bHaveGroupCmd);

						root["result"][ii]["Status"]=lstatus;
						root["result"][ii]["HaveDimmer"]=bHaveDimmer;
						root["result"][ii]["MaxDimLevel"]=maxDimLevel;
						root["result"][ii]["HaveGroupCmd"]=bHaveGroupCmd;
						root["result"][ii]["SwitchType"]="evohome";
						root["result"][ii]["SwitchTypeVal"]=switchtype; //was 0?;
						root["result"][ii]["TypeImg"]="override_mini";
						root["result"][ii]["StrParam1"]=strParam1;
						root["result"][ii]["StrParam2"]=strParam2;
						root["result"][ii]["Protected"]=(iProtected!=0);
						
						sprintf(szData,"%s", lstatus.c_str());
						root["result"][ii]["Data"]=szData;
						root["result"][ii]["HaveTimeout"]=false;
						
						if (dType == pTypeEvohomeRelay)
						{
							root["result"][ii]["SwitchType"]="TPI";
							root["result"][ii]["Level"] = llevel;
							root["result"][ii]["LevelInt"] = atoi(sValue.c_str());
							if(root["result"][ii]["Unit"]>100)
								root["result"][ii]["Protected"]=true;
							
							sprintf(szData,"%s: %d", lstatus.c_str(), atoi(sValue.c_str()));
							root["result"][ii]["Data"]=szData;
						}
					}
					else if ((dType == pTypeEvohomeZone) || (dType == pTypeEvohomeWater))
					{
						root["result"][ii]["HaveTimeout"]=bHaveTimeout;
						root["result"][ii]["TypeImg"]="override_mini";
						
						std::vector<std::string> strarray;
						StringSplit(sValue, ";", strarray);
						if (strarray.size()>=3)
						{
							int i=0;
							root["result"][ii]["AddjValue"]=AddjValue;
							root["result"][ii]["AddjMulti"]=AddjMulti;
							root["result"][ii]["AddjValue2"]=AddjValue2;
							root["result"][ii]["AddjMulti2"]=AddjMulti2;

							double tempCelcius=atof(strarray[i++].c_str());
							double temp=ConvertTemperature(tempCelcius,tempsign);
							double tempSetPoint;
							root["result"][ii]["Temp"]=temp;
							if (dType == pTypeEvohomeZone)
							{
								tempCelcius=atof(strarray[i++].c_str());
								tempSetPoint=ConvertTemperature(tempCelcius,tempsign);
								root["result"][ii]["SetPoint"]=tempSetPoint;
							}
							else
								root["result"][ii]["State"]=strarray[i++];
						
							std::string strstatus=strarray[i++];
							root["result"][ii]["Status"]=strstatus;
							
							if ((dType == pTypeEvohomeZone || dType == pTypeEvohomeWater) && strarray.size()>=4)
							{
								root["result"][ii]["Until"]=strarray[i++];
							}
							if (dType == pTypeEvohomeZone)
							{
								if (strarray.size()>=4)
									sprintf(szData,"%.1f %c, (%.1f %c), %s until %s", temp,tempsign,tempSetPoint,tempsign,strstatus.c_str(),strarray[3].c_str());
								else
									sprintf(szData,"%.1f %c, (%.1f %c), %s", temp,tempsign,tempSetPoint,tempsign,strstatus.c_str());
							}
							else
								if (strarray.size()>=4)
									sprintf(szData,"%.1f %c, %s, %s until %s", temp,tempsign,strarray[1].c_str(),strstatus.c_str(),strarray[3].c_str());
								else
									sprintf(szData,"%.1f %c, %s, %s", temp,tempsign,strarray[1].c_str(),strstatus.c_str());
							root["result"][ii]["Data"]=szData;
							root["result"][ii]["HaveTimeout"]=bHaveTimeout;
						}
					}
					else if ((dType == pTypeTEMP) || (dType == pTypeRego6XXTemp))
					{
						root["result"][ii]["AddjValue"] = AddjValue;
						root["result"][ii]["AddjMulti"] = AddjMulti;
						double tvalue = ConvertTemperature(atof(sValue.c_str()), tempsign);
						root["result"][ii]["Temp"] = tvalue;
						sprintf(szData, "%.1f %c", tvalue, tempsign);
						root["result"][ii]["Data"] = szData;
						root["result"][ii]["HaveTimeout"] = bHaveTimeout;
					}
					else if (dType == pTypeThermostat1)
					{
						std::vector<std::string> strarray;
						StringSplit(sValue, ";", strarray);
						if (strarray.size() == 4)
						{
							double tvalue = ConvertTemperature(atof(strarray[0].c_str()), tempsign);
							root["result"][ii]["Temp"] = tvalue;
							sprintf(szData, "%.1f %c", tvalue, tempsign);
							root["result"][ii]["Data"] = szData;
							root["result"][ii]["HaveTimeout"] = bHaveTimeout;
						}
					}
					else if ((dType == pTypeRFXSensor) && (dSubType == sTypeRFXSensorTemp))
					{
						root["result"][ii]["AddjValue"] = AddjValue;
						root["result"][ii]["AddjMulti"] = AddjMulti;
						root["result"][ii]["AddjValue2"] = AddjValue2;
						root["result"][ii]["AddjMulti2"] = AddjMulti2;
						double tvalue = ConvertTemperature(atof(sValue.c_str()), tempsign);
						root["result"][ii]["Temp"] = tvalue;
						sprintf(szData, "%.1f %c", tvalue, tempsign);
						root["result"][ii]["Data"] = szData;
						root["result"][ii]["TypeImg"] = "temperature";
						root["result"][ii]["HaveTimeout"] = bHaveTimeout;
					}
					else if (dType == pTypeHUM)
					{
						root["result"][ii]["Humidity"] = nValue;
						root["result"][ii]["HumidityStatus"] = RFX_Humidity_Status_Desc(atoi(sValue.c_str()));
						sprintf(szData, "Humidity %d %%", nValue);
						root["result"][ii]["Data"] = szData;
						root["result"][ii]["HaveTimeout"] = bHaveTimeout;
					}
					else if (dType == pTypeTEMP_HUM)
					{
						std::vector<std::string> strarray;
						StringSplit(sValue, ";", strarray);
						if (strarray.size() == 3)
						{
							root["result"][ii]["AddjValue"] = AddjValue;
							root["result"][ii]["AddjMulti"] = AddjMulti;
							root["result"][ii]["AddjValue2"] = AddjValue2;
							root["result"][ii]["AddjMulti2"] = AddjMulti2;

							double tempCelcius = atof(strarray[0].c_str());
							double temp = ConvertTemperature(tempCelcius, tempsign);
							int humidity = atoi(strarray[1].c_str());

							root["result"][ii]["Temp"] = temp;
							root["result"][ii]["Humidity"] = humidity;
							root["result"][ii]["HumidityStatus"] = RFX_Humidity_Status_Desc(atoi(strarray[2].c_str()));
							sprintf(szData, "%.1f %c, %d %%", temp, tempsign, atoi(strarray[1].c_str()));
							root["result"][ii]["Data"] = szData;
							root["result"][ii]["HaveTimeout"] = bHaveTimeout;

							//Calculate dew point

							sprintf(szTmp, "%.2f", ConvertTemperature(CalculateDewPoint(tempCelcius, humidity), tempsign));
							root["result"][ii]["DewPoint"] = szTmp;
						}
					}
					else if (dType == pTypeTEMP_HUM_BARO)
					{
						std::vector<std::string> strarray;
						StringSplit(sValue, ";", strarray);
						if (strarray.size() == 5)
						{
							root["result"][ii]["AddjValue"] = AddjValue;
							root["result"][ii]["AddjMulti"] = AddjMulti;
							root["result"][ii]["AddjValue2"] = AddjValue2;
							root["result"][ii]["AddjMulti2"] = AddjMulti2;

							double tempCelcius = atof(strarray[0].c_str());
							double temp = ConvertTemperature(tempCelcius, tempsign);
							int humidity = atoi(strarray[1].c_str());

							root["result"][ii]["Temp"] = temp;
							root["result"][ii]["Humidity"] = humidity;
							root["result"][ii]["HumidityStatus"] = RFX_Humidity_Status_Desc(atoi(strarray[2].c_str()));
							root["result"][ii]["Forecast"] = atoi(strarray[4].c_str());

							sprintf(szTmp, "%.2f", ConvertTemperature(CalculateDewPoint(tempCelcius, humidity), tempsign));
							root["result"][ii]["DewPoint"] = szTmp;

							if (dSubType == sTypeTHBFloat)
							{
								root["result"][ii]["Barometer"] = atof(strarray[3].c_str());
								root["result"][ii]["ForecastStr"] = RFX_WSForecast_Desc(atoi(strarray[4].c_str()));
							}
							else
							{
								root["result"][ii]["Barometer"] = atoi(strarray[3].c_str());
								root["result"][ii]["ForecastStr"] = RFX_Forecast_Desc(atoi(strarray[4].c_str()));
							}
							if (dSubType == sTypeTHBFloat)
							{
								sprintf(szData, "%.1f %c, %d %%, %.1f hPa",
									temp,
									tempsign,
									atoi(strarray[1].c_str()),
									atof(strarray[3].c_str())
									);
							}
							else
							{
								sprintf(szData, "%.1f %c, %d %%, %d hPa",
									temp,
									tempsign,
									atoi(strarray[1].c_str()),
									atoi(strarray[3].c_str())
									);
							}
							root["result"][ii]["Data"] = szData;
							root["result"][ii]["HaveTimeout"] = bHaveTimeout;
						}
					}
					else if (dType == pTypeTEMP_BARO)
					{
						std::vector<std::string> strarray;
						StringSplit(sValue, ";", strarray);
						if (strarray.size() >= 3)
						{
							root["result"][ii]["AddjValue"] = AddjValue;
							root["result"][ii]["AddjMulti"] = AddjMulti;
							root["result"][ii]["AddjValue2"] = AddjValue2;
							root["result"][ii]["AddjMulti2"] = AddjMulti2;

							double tvalue = ConvertTemperature(atof(strarray[0].c_str()), tempsign);
							root["result"][ii]["Temp"] = tvalue;
							int forecast = atoi(strarray[2].c_str());
							root["result"][ii]["Forecast"] = forecast;
							root["result"][ii]["ForecastStr"] = BMP_Forecast_Desc(forecast);
							root["result"][ii]["Barometer"] = atof(strarray[1].c_str());

							sprintf(szData, "%.1f %c, %.1f hPa",
								tvalue,
								tempsign,
								atof(strarray[1].c_str())
								);
							root["result"][ii]["Data"] = szData;
							root["result"][ii]["HaveTimeout"] = bHaveTimeout;
						}
					}
					else if (dType == pTypeUV)
					{
						std::vector<std::string> strarray;
						StringSplit(sValue, ";", strarray);
						if (strarray.size() == 2)
						{
							float UVI = static_cast<float>(atof(strarray[0].c_str()));
							root["result"][ii]["UVI"] = strarray[0];
							if (dSubType == sTypeUV3)
							{
								root["result"][ii]["AddjValue"] = AddjValue;
								root["result"][ii]["AddjMulti"] = AddjMulti;
								root["result"][ii]["AddjValue2"] = AddjValue2;
								root["result"][ii]["AddjMulti2"] = AddjMulti2;

								double tvalue = ConvertTemperature(atof(strarray[1].c_str()), tempsign);

								root["result"][ii]["Temp"] = tvalue;
								sprintf(szData, "%.1f UVI, %.1f&deg; %c", UVI, tvalue, tempsign);
							}
							else
							{
								sprintf(szData, "%.1f UVI", UVI);
							}
							root["result"][ii]["Data"] = szData;
							root["result"][ii]["HaveTimeout"] = bHaveTimeout;
						}
					}
					else if (dType == pTypeWIND)
					{
						std::vector<std::string> strarray;
						StringSplit(sValue, ";", strarray);
						if (strarray.size() == 6)
						{
							root["result"][ii]["Direction"] = atof(strarray[0].c_str());
							root["result"][ii]["DirectionStr"] = strarray[1];

							if (dSubType != sTypeWIND5)
							{
								int intSpeed = atoi(strarray[2].c_str());
								sprintf(szTmp, "%.1f", float(intSpeed) * m_sql.m_windscale);
								root["result"][ii]["Speed"] = szTmp;
							}

							//if (dSubType!=sTypeWIND6) //problem in RFXCOM firmware? gust=speed?
					{
						int intGust = atoi(strarray[3].c_str());
						sprintf(szTmp, "%.1f", float(intGust) *m_sql.m_windscale);
						root["result"][ii]["Gust"] = szTmp;
					}
							if ((dSubType == sTypeWIND4) || (dSubType == sTypeWINDNoTemp))
							{
								root["result"][ii]["AddjValue"] = AddjValue;
								root["result"][ii]["AddjMulti"] = AddjMulti;
								root["result"][ii]["AddjValue2"] = AddjValue2;
								root["result"][ii]["AddjMulti2"] = AddjMulti2;
								if (dSubType == sTypeWIND4)
								{
									double tvalue = ConvertTemperature(atof(strarray[4].c_str()), tempsign);
									root["result"][ii]["Temp"] = tvalue;
								}
								double tvalue = ConvertTemperature(atof(strarray[5].c_str()), tempsign);
								root["result"][ii]["Chill"] = tvalue;
							}
							root["result"][ii]["Data"] = sValue;
							root["result"][ii]["HaveTimeout"] = bHaveTimeout;
						}
					}
					else if (dType == pTypeRAIN)
					{
						std::vector<std::string> strarray;
						StringSplit(sValue, ";", strarray);
						if (strarray.size() == 2)
						{
							//get lowest value of today, and max rate
							time_t now = mytime(NULL);
							struct tm tm1;
							localtime_r(&now, &tm1);

							struct tm ltime;
							ltime.tm_isdst = tm1.tm_isdst;
							ltime.tm_hour = 0;
							ltime.tm_min = 0;
							ltime.tm_sec = 0;
							ltime.tm_year = tm1.tm_year;
							ltime.tm_mon = tm1.tm_mon;
							ltime.tm_mday = tm1.tm_mday;

							char szDate[40];
							sprintf(szDate, "%04d-%02d-%02d", ltime.tm_year + 1900, ltime.tm_mon + 1, ltime.tm_mday);

							std::vector<std::vector<std::string> > result2;

							szQuery.clear();
							szQuery.str("");
							if (dSubType != sTypeRAINWU)
							{
								szQuery << "SELECT MIN(Total), MAX(Total), MAX(Rate) FROM Rain WHERE (DeviceRowID=" << sd[0] << " AND Date>='" << szDate << "')";
							}
							else
							{
								szQuery << "SELECT Total, Total, Rate FROM Rain WHERE (DeviceRowID=" << sd[0] << " AND Date>='" << szDate << "') ORDER BY ROWID DESC LIMIT 1";
							}
							result2 = m_sql.query(szQuery.str());
							if (result2.size() > 0)
							{
								root["result"][ii]["AddjValue"] = AddjValue;
								root["result"][ii]["AddjMulti"] = AddjMulti;
								root["result"][ii]["AddjValue2"] = AddjValue2;
								root["result"][ii]["AddjMulti2"] = AddjMulti2;

								double total_real = 0;
								float rate = 0;
								std::vector<std::string> sd2 = result2[0];
								if (dSubType != sTypeRAINWU)
								{
									float total_min = static_cast<float>(atof(sd2[0].c_str()));
									float total_max = static_cast<float>(atof(strarray[1].c_str()));
									rate = static_cast<float>(atof(sd2[2].c_str()));
									if (dSubType == sTypeRAIN2)
										rate /= 100.0f;
									total_real = total_max - total_min;
								}
								else
								{
									rate = static_cast<float>(atof(sd2[2].c_str()));
									total_real = atof(sd2[1].c_str());
								}
								total_real *= AddjMulti;

								sprintf(szTmp, "%.1f", total_real);
								root["result"][ii]["Rain"] = szTmp;
								if (dSubType != sTypeRAIN2)
									sprintf(szTmp, "%.1f", rate);
								else
									sprintf(szTmp, "%.2f", rate);
								root["result"][ii]["RainRate"] = szTmp;
								root["result"][ii]["Data"] = sValue;
								root["result"][ii]["HaveTimeout"] = bHaveTimeout;
							}
							else
							{
								root["result"][ii]["Rain"] = "0.0";
								root["result"][ii]["RainRate"] = "0.0";
								root["result"][ii]["Data"] = "0.0";
								root["result"][ii]["HaveTimeout"] = bHaveTimeout;
							}
						}
					}
					else if (dType == pTypeRFXMeter)
					{
						float EnergyDivider = 1000.0f;
						float GasDivider = 100.0f;
						float WaterDivider = 100.0f;
						int tValue;
						if (m_sql.GetPreferencesVar("MeterDividerEnergy", tValue))
						{
							EnergyDivider = float(tValue);
						}
						if (m_sql.GetPreferencesVar("MeterDividerGas", tValue))
						{
							GasDivider = float(tValue);
						}
						if (m_sql.GetPreferencesVar("MeterDividerWater", tValue))
						{
							WaterDivider = float(tValue);
						}

						//get value of today
						time_t now = mytime(NULL);
						struct tm tm1;
						localtime_r(&now, &tm1);

						struct tm ltime;
						ltime.tm_isdst = tm1.tm_isdst;
						ltime.tm_hour = 0;
						ltime.tm_min = 0;
						ltime.tm_sec = 0;
						ltime.tm_year = tm1.tm_year;
						ltime.tm_mon = tm1.tm_mon;
						ltime.tm_mday = tm1.tm_mday;

						char szDate[40];
						sprintf(szDate, "%04d-%02d-%02d", ltime.tm_year + 1900, ltime.tm_mon + 1, ltime.tm_mday);

						std::vector<std::vector<std::string> > result2;
						strcpy(szTmp, "0");
						szQuery.clear();
						szQuery.str("");
						szQuery << "SELECT MIN(Value), MAX(Value) FROM Meter WHERE (DeviceRowID=" << sd[0] << " AND Date>='" << szDate << "')";
						result2 = m_sql.query(szQuery.str());
						if (result2.size() > 0)
						{
							std::vector<std::string> sd2 = result2[0];

							unsigned long long total_min, total_max, total_real;

							std::stringstream s_str1(sd2[0]);
							s_str1 >> total_min;
							std::stringstream s_str2(sd2[1]);
							s_str2 >> total_max;
							total_real = total_max - total_min;
							sprintf(szTmp, "%llu", total_real);

							float musage = 0;
							switch (metertype)
							{
							case MTYPE_ENERGY:
								musage = float(total_real) / EnergyDivider;
								sprintf(szTmp, "%.03f kWh", musage);
								break;
							case MTYPE_GAS:
								musage = float(total_real) / GasDivider;
								sprintf(szTmp, "%.03f m3", musage);
								break;
							case MTYPE_WATER:
								musage = float(total_real) / WaterDivider;
								sprintf(szTmp, "%.03f m3", musage);
								break;
							case MTYPE_COUNTER:
								sprintf(szTmp, "%llu", total_real);
								break;
							}
						}
						root["result"][ii]["Counter"] = sValue;
						root["result"][ii]["CounterToday"] = szTmp;
						root["result"][ii]["SwitchTypeVal"] = metertype;
						root["result"][ii]["HaveTimeout"] = bHaveTimeout;
						float fvalue = static_cast<float>(atof(sValue.c_str()));
						switch (metertype)
						{
						case MTYPE_ENERGY:
							sprintf(szTmp, "%.03f kWh", fvalue / EnergyDivider);
							root["result"][ii]["Data"] = szTmp;
							root["result"][ii]["Counter"] = szTmp;
							break;
						case MTYPE_GAS:
							sprintf(szTmp, "%.03f m3", fvalue / GasDivider);
							root["result"][ii]["Data"] = szTmp;
							root["result"][ii]["Counter"] = szTmp;
							break;
						case MTYPE_WATER:
							sprintf(szTmp, "%.03f m3", fvalue / WaterDivider);
							root["result"][ii]["Data"] = szTmp;
							root["result"][ii]["Counter"] = szTmp;
							break;
						}
					}
                    else if (dType == pTypeGeneral && dSubType == sTypeCounterIncremental)
                    {
                        float EnergyDivider = 1000.0f;
                        float GasDivider = 100.0f;
                        float WaterDivider = 100.0f;
                        int tValue;
                        if (m_sql.GetPreferencesVar("MeterDividerEnergy", tValue))
                        {
                                EnergyDivider = float(tValue);
                        }
                        if (m_sql.GetPreferencesVar("MeterDividerGas", tValue))
                        {
                                GasDivider = float(tValue);
                        }
                        if (m_sql.GetPreferencesVar("MeterDividerWater", tValue))
                        {
                                WaterDivider = float(tValue);
                        }

                        //get value of today
                        time_t now = mytime(NULL);
                        struct tm tm1;
                        localtime_r(&now, &tm1);

                        struct tm ltime;
                        ltime.tm_isdst = tm1.tm_isdst;
                        ltime.tm_hour = 0;
                        ltime.tm_min = 0;
                        ltime.tm_sec = 0;
                        ltime.tm_year = tm1.tm_year;
                        ltime.tm_mon = tm1.tm_mon;
                        ltime.tm_mday = tm1.tm_mday;

                        char szDate[40];
                        sprintf(szDate, "%04d-%02d-%02d", ltime.tm_year + 1900, ltime.tm_mon + 1, ltime.tm_mday);

                        std::vector<std::vector<std::string> > result2;
                        strcpy(szTmp, "0");
                        szQuery.clear();
                        szQuery.str("");
                        szQuery << "SELECT MIN(Value), MAX(Value) FROM Meter WHERE (DeviceRowID=" << sd[0] << " AND Date>='" << szDate << "')";
                        result2 = m_sql.query(szQuery.str());
                        if (result2.size() > 0)
                        {
                            std::vector<std::string> sd2 = result2[0];

                            unsigned long long total_min, total_max, total_real;

                            std::stringstream s_str1(sd2[0]);
                            s_str1 >> total_min;
                            std::stringstream s_str2(sd2[1]);
                            s_str2 >> total_max;
                            total_real = total_max - total_min;
                            sprintf(szTmp, "%llu", total_real);

                            float musage = 0;
                            switch (metertype)
                            {
                            case MTYPE_ENERGY:
                                    musage = float(total_real) / EnergyDivider;
                                    sprintf(szTmp, "%.03f kWh", musage);
                                    break;
                            case MTYPE_GAS:
                                    musage = float(total_real) / GasDivider;
                                    sprintf(szTmp, "%.03f m3", musage);
                                    break;
                            case MTYPE_WATER:
                                    musage = float(total_real) / WaterDivider;
                                    sprintf(szTmp, "%.03f m3", musage);
                                    break;
                            case MTYPE_COUNTER:
                                    sprintf(szTmp, "%llu", total_real);
                                    break;
                            }
                        }
                        root["result"][ii]["Counter"] = sValue;
                        root["result"][ii]["CounterToday"] = szTmp;
                        root["result"][ii]["SwitchTypeVal"] = metertype;
                        root["result"][ii]["HaveTimeout"] = bHaveTimeout;
                        float fvalue = static_cast<float>(atof(sValue.c_str()));
                        switch (metertype)
                        {
                        case MTYPE_ENERGY:
                                sprintf(szTmp, "%.03f kWh", fvalue / EnergyDivider);
                                root["result"][ii]["Data"] = szTmp;
                                root["result"][ii]["Counter"] = szTmp;
                                break;
                        case MTYPE_GAS:
                                sprintf(szTmp, "%.03f m3", fvalue / GasDivider);
                                root["result"][ii]["Data"] = szTmp;
                                root["result"][ii]["Counter"] = szTmp;
                                break;
                        case MTYPE_WATER:
                                sprintf(szTmp, "%.03f m3", fvalue / WaterDivider);
                                root["result"][ii]["Data"] = szTmp;
                                root["result"][ii]["Counter"] = szTmp;
                                break;
                        }
                    }
					else if (dType == pTypeYouLess)
					{
						float EnergyDivider = 1000.0f;
						float GasDivider = 100.0f;
						float WaterDivider = 100.0f;
						float musage = 0;
						int tValue;
						if (m_sql.GetPreferencesVar("MeterDividerEnergy", tValue))
						{
							EnergyDivider = float(tValue);
						}
						if (m_sql.GetPreferencesVar("MeterDividerGas", tValue))
						{
							GasDivider = float(tValue);
						}
						if (m_sql.GetPreferencesVar("MeterDividerWater", tValue))
						{
							WaterDivider = float(tValue);
						}

						//get value of today
						time_t now = mytime(NULL);
						struct tm tm1;
						localtime_r(&now, &tm1);

						struct tm ltime;
						ltime.tm_isdst = tm1.tm_isdst;
						ltime.tm_hour = 0;
						ltime.tm_min = 0;
						ltime.tm_sec = 0;
						ltime.tm_year = tm1.tm_year;
						ltime.tm_mon = tm1.tm_mon;
						ltime.tm_mday = tm1.tm_mday;

						char szDate[40];
						sprintf(szDate, "%04d-%02d-%02d", ltime.tm_year + 1900, ltime.tm_mon + 1, ltime.tm_mday);

						std::vector<std::vector<std::string> > result2;
						strcpy(szTmp, "0");
						szQuery.clear();
						szQuery.str("");
						szQuery << "SELECT MIN(Value), MAX(Value) FROM Meter WHERE (DeviceRowID=" << sd[0] << " AND Date>='" << szDate << "')";
						result2 = m_sql.query(szQuery.str());
						if (result2.size() > 0)
						{
							std::vector<std::string> sd2 = result2[0];

							unsigned long long total_min, total_max, total_real;

							std::stringstream s_str1(sd2[0]);
							s_str1 >> total_min;
							std::stringstream s_str2(sd2[1]);
							s_str2 >> total_max;
							total_real = total_max - total_min;
							sprintf(szTmp, "%llu", total_real);

							musage = 0;
							switch (metertype)
							{
							case MTYPE_ENERGY:
								musage = float(total_real) / EnergyDivider;
								sprintf(szTmp, "%.03f kWh", musage);
								break;
							case MTYPE_GAS:
								musage = float(total_real) / GasDivider;
								sprintf(szTmp, "%.02f m3", musage);
								break;
							case MTYPE_WATER:
								musage = float(total_real) / WaterDivider;
								sprintf(szTmp, "%.02f m3", musage);
								break;
							case MTYPE_COUNTER:
								sprintf(szTmp, "%llu", total_real);
								break;
							}
						}
						root["result"][ii]["CounterToday"] = szTmp;


						std::vector<std::string> splitresults;
						StringSplit(sValue, ";", splitresults);
						if (splitresults.size() < 2)
							continue;

						unsigned long long total_actual;
						std::stringstream s_stra(splitresults[0]);
						s_stra >> total_actual;
						musage = 0;
						switch (metertype)
						{
						case MTYPE_ENERGY:
							musage = float(total_actual) / EnergyDivider;
							sprintf(szTmp, "%.03f", musage);
							break;
						case MTYPE_GAS:
						case MTYPE_WATER:
							musage = float(total_actual) / GasDivider;
							sprintf(szTmp, "%.02f", musage);
							break;
						case MTYPE_COUNTER:
							sprintf(szTmp, "%llu", total_actual);
							break;
						}
						root["result"][ii]["Counter"] = szTmp;

						root["result"][ii]["SwitchTypeVal"] = metertype;

						unsigned long long acounter;
						std::stringstream s_str3(sValue);
						s_str3 >> acounter;
						musage = 0;
						switch (metertype)
						{
						case MTYPE_ENERGY:
							musage = float(acounter) / EnergyDivider;
							sprintf(szTmp, "%.03f kWh %s Watt", musage, splitresults[1].c_str());
							break;
						case MTYPE_GAS:
							musage = float(acounter) / GasDivider;
							sprintf(szTmp, "%.02f m3", musage);
							break;
						case MTYPE_WATER:
							musage = float(acounter) / WaterDivider;
							sprintf(szTmp, "%.02f m3", musage);
							break;
						case MTYPE_COUNTER:
							sprintf(szTmp, "%llu", acounter);
							break;
						}
						root["result"][ii]["Data"] = szTmp;
						switch (metertype)
						{
						case MTYPE_ENERGY:
							sprintf(szTmp, "%s Watt", splitresults[1].c_str());
							break;
						case MTYPE_GAS:
							sprintf(szTmp, "%s m", splitresults[1].c_str());
							break;
						case MTYPE_WATER:
							sprintf(szTmp, "%s m", splitresults[1].c_str());
							break;
						case MTYPE_COUNTER:
							sprintf(szTmp, "%s", splitresults[1].c_str());
							break;
						}

						root["result"][ii]["Usage"] = szTmp;
						root["result"][ii]["HaveTimeout"] = bHaveTimeout;
					}
					else if (dType == pTypeP1Power)
					{
						std::vector<std::string> splitresults;
						StringSplit(sValue, ";", splitresults);
						if (splitresults.size() != 6)
							continue; //impossible

						float EnergyDivider = 1000.0f;
						int tValue;
						if (m_sql.GetPreferencesVar("MeterDividerEnergy", tValue))
						{
							EnergyDivider = float(tValue);
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

						unsigned long long powerusage = powerusage1 + powerusage2;
						unsigned long long powerdeliv = powerdeliv1 + powerdeliv2;
						if (powerdeliv < 2)
							powerdeliv = 0;

						float musage = 0;

						root["result"][ii]["SwitchTypeVal"] = MTYPE_ENERGY;
						musage = float(powerusage) / EnergyDivider;
						sprintf(szTmp, "%.03f", musage);
						root["result"][ii]["Counter"] = szTmp;
						musage = float(powerdeliv) / EnergyDivider;
						sprintf(szTmp, "%.03f", musage);
						root["result"][ii]["CounterDeliv"] = szTmp;

						if (bHaveTimeout)
						{
							usagecurrent = 0;
							delivcurrent = 0;
						}
						sprintf(szTmp, "%llu Watt", usagecurrent);
						root["result"][ii]["Usage"] = szTmp;
						sprintf(szTmp, "%llu Watt", delivcurrent);
						root["result"][ii]["UsageDeliv"] = szTmp;
						root["result"][ii]["Data"] = sValue;
						root["result"][ii]["HaveTimeout"] = bHaveTimeout;

						//get value of today
						time_t now = mytime(NULL);
						struct tm tm1;
						localtime_r(&now, &tm1);

						struct tm ltime;
						ltime.tm_isdst = tm1.tm_isdst;
						ltime.tm_hour = 0;
						ltime.tm_min = 0;
						ltime.tm_sec = 0;
						ltime.tm_year = tm1.tm_year;
						ltime.tm_mon = tm1.tm_mon;
						ltime.tm_mday = tm1.tm_mday;

						char szDate[40];
						sprintf(szDate, "%04d-%02d-%02d", ltime.tm_year + 1900, ltime.tm_mon + 1, ltime.tm_mday);

						std::vector<std::vector<std::string> > result2;
						strcpy(szTmp, "0");
						szQuery.clear();
						szQuery.str("");
						szQuery << "SELECT MIN(Value1), MIN(Value2), MIN(Value5), MIN(Value6) FROM MultiMeter WHERE (DeviceRowID=" << sd[0] << " AND Date>='" << szDate << "')";
						result2 = m_sql.query(szQuery.str());
						if (result2.size() > 0)
						{
							std::vector<std::string> sd2 = result2[0];

							unsigned long long total_min_usage_1, total_min_usage_2, total_real_usage;
							unsigned long long total_min_deliv_1, total_min_deliv_2, total_real_deliv;

							std::stringstream s_str1(sd2[0]);
							s_str1 >> total_min_usage_1;
							std::stringstream s_str2(sd2[1]);
							s_str2 >> total_min_deliv_1;
							std::stringstream s_str3(sd2[2]);
							s_str3 >> total_min_usage_2;
							std::stringstream s_str4(sd2[3]);
							s_str4 >> total_min_deliv_2;

							total_real_usage = powerusage - (total_min_usage_1 + total_min_usage_2);
							total_real_deliv = powerdeliv - (total_min_deliv_1 + total_min_deliv_2);

							musage = float(total_real_usage) / EnergyDivider;
							sprintf(szTmp, "%.03f kWh", musage);
							root["result"][ii]["CounterToday"] = szTmp;
							musage = float(total_real_deliv) / EnergyDivider;
							sprintf(szTmp, "%.03f kWh", musage);
							root["result"][ii]["CounterDelivToday"] = szTmp;
						}
						else
						{
							sprintf(szTmp, "%.03f kWh", 0.0f);
							root["result"][ii]["CounterToday"] = szTmp;
							root["result"][ii]["CounterDelivToday"] = szTmp;
						}
					}
					else if (dType == pTypeP1Gas)
					{
						float GasDivider = 1000.0f;
						//get lowest value of today
						time_t now = mytime(NULL);
						struct tm tm1;
						localtime_r(&now, &tm1);

						struct tm ltime;
						ltime.tm_isdst = tm1.tm_isdst;
						ltime.tm_hour = 0;
						ltime.tm_min = 0;
						ltime.tm_sec = 0;
						ltime.tm_year = tm1.tm_year;
						ltime.tm_mon = tm1.tm_mon;
						ltime.tm_mday = tm1.tm_mday;

						char szDate[40];
						sprintf(szDate, "%04d-%02d-%02d", ltime.tm_year + 1900, ltime.tm_mon + 1, ltime.tm_mday);

						std::vector<std::vector<std::string> > result2;
						strcpy(szTmp, "0");
						szQuery.clear();
						szQuery.str("");
						szQuery << "SELECT MIN(Value) FROM Meter WHERE (DeviceRowID=" << sd[0] << " AND Date>='" << szDate << "')";
						result2 = m_sql.query(szQuery.str());
						if (result2.size() > 0)
						{
							std::vector<std::string> sd2 = result2[0];

							unsigned long long total_min_gas, total_real_gas;
							unsigned long long gasactual;

							std::stringstream s_str1(sd2[0]);
							s_str1 >> total_min_gas;
							std::stringstream s_str2(sValue);
							s_str2 >> gasactual;
							float musage = 0;

							root["result"][ii]["SwitchTypeVal"] = MTYPE_GAS;

							musage = float(gasactual) / GasDivider;
							sprintf(szTmp, "%.03f", musage);
							root["result"][ii]["Counter"] = szTmp;
							total_real_gas = gasactual - total_min_gas;
							musage = float(total_real_gas) / GasDivider;
							sprintf(szTmp, "%.03f m3", musage);
							root["result"][ii]["CounterToday"] = szTmp;
							root["result"][ii]["HaveTimeout"] = bHaveTimeout;
							sprintf(szTmp, "%.03f", atof(sValue.c_str()) / GasDivider);
							root["result"][ii]["Data"] = szTmp;
						}
						else
						{
							root["result"][ii]["SwitchTypeVal"] = MTYPE_GAS;
							sprintf(szTmp, "%.03f", 0.0f);
							root["result"][ii]["Counter"] = szTmp;
							sprintf(szTmp, "%.03f m3", 0.0f);
							root["result"][ii]["CounterToday"] = szTmp;
							sprintf(szTmp, "%.03f", atof(sValue.c_str()) / GasDivider);
							root["result"][ii]["Data"] = szTmp;
							root["result"][ii]["HaveTimeout"] = bHaveTimeout;
						}
					}
					else if (dType == pTypeCURRENT)
					{
						std::vector<std::string> strarray;
						StringSplit(sValue, ";", strarray);
						if (strarray.size() == 3)
						{
							//CM113
							int displaytype = 0;
							int voltage = 230;
							m_sql.GetPreferencesVar("CM113DisplayType", displaytype);
							m_sql.GetPreferencesVar("ElectricVoltage", voltage);

							double val1 = atof(strarray[0].c_str());
							double val2 = atof(strarray[1].c_str());
							double val3 = atof(strarray[2].c_str());

							if (displaytype == 0)
							{
								if ((val2 == 0) && (val3 == 0))
									sprintf(szData, "%.1f A", val1);
								else
									sprintf(szData, "%.1f A, %.1f A, %.1f A", val1, val2, val3);
							}
							else
							{
								if ((val2 == 0) && (val3 == 0))
									sprintf(szData, "%d Watt, %d Watt, %d Watt", int(val1*voltage), int(val2*voltage), int(val3*voltage));
								else
									sprintf(szData, "%d Watt", int(val1*voltage));
							}
							root["result"][ii]["Data"] = szData;
							root["result"][ii]["displaytype"] = displaytype;
							root["result"][ii]["HaveTimeout"] = bHaveTimeout;
						}
					}
					else if (dType == pTypeCURRENTENERGY)
					{
						std::vector<std::string> strarray;
						StringSplit(sValue, ";", strarray);
						if (strarray.size() == 4)
						{
							//CM180i
							int displaytype = 0;
							int voltage = 230;
							m_sql.GetPreferencesVar("CM113DisplayType", displaytype);
							m_sql.GetPreferencesVar("ElectricVoltage", voltage);

							double total = atof(strarray[3].c_str());
							if (displaytype == 0)
							{
								sprintf(szData, "%.1f A, %.1f A, %.1f A", atof(strarray[0].c_str()), atof(strarray[1].c_str()), atof(strarray[2].c_str()));
							}
							else
							{
								sprintf(szData, "%d Watt, %d Watt, %d Watt", int(atof(strarray[0].c_str())*voltage), int(atof(strarray[1].c_str())*voltage), int(atof(strarray[2].c_str())*voltage));
							}
							if (total > 0)
							{
								sprintf(szTmp, ", Total: %.3f Wh", total);
								strcat(szData, szTmp);
							}
							root["result"][ii]["Data"] = szData;
							root["result"][ii]["displaytype"] = displaytype;
							root["result"][ii]["HaveTimeout"] = bHaveTimeout;
						}
					}
					else if ((dType == pTypeENERGY) || (dType == pTypePOWER))
					{
						std::vector<std::string> strarray;
						StringSplit(sValue, ";", strarray);
						if (strarray.size() == 2)
						{
							double total = atof(strarray[1].c_str()) / 1000;

							time_t now = mytime(NULL);
							struct tm tm1;
							localtime_r(&now, &tm1);

							struct tm ltime;
							ltime.tm_isdst = tm1.tm_isdst;
							ltime.tm_hour = 0;
							ltime.tm_min = 0;
							ltime.tm_sec = 0;
							ltime.tm_year = tm1.tm_year;
							ltime.tm_mon = tm1.tm_mon;
							ltime.tm_mday = tm1.tm_mday;

							char szDate[40];
							sprintf(szDate, "%04d-%02d-%02d", ltime.tm_year + 1900, ltime.tm_mon + 1, ltime.tm_mday);

							std::vector<std::vector<std::string> > result2;
							strcpy(szTmp, "0");
							szQuery.clear();
							szQuery.str("");
							szQuery << "SELECT MIN(Value) FROM Meter WHERE (DeviceRowID=" << sd[0] << " AND Date>='" << szDate << "')";
							result2 = m_sql.query(szQuery.str());
							if (result2.size() > 0)
							{
								float EnergyDivider = 1000.0f;
								int tValue;
								if (m_sql.GetPreferencesVar("MeterDividerEnergy", tValue))
								{
									EnergyDivider = float(tValue);
								}
								EnergyDivider *= 100.0;

								std::vector<std::string> sd2 = result2[0];
								double minimum = atof(sd2[0].c_str()) / EnergyDivider;

								sprintf(szData, "%.3f kWh", total);
								root["result"][ii]["Data"] = szData;
								sprintf(szData, "%ld Watt", atol(strarray[0].c_str()));
								root["result"][ii]["Usage"] = szData;
								root["result"][ii]["SwitchTypeVal"] = MTYPE_ENERGY;
								root["result"][ii]["HaveTimeout"] = bHaveTimeout;
								sprintf(szTmp, "%.03f kWh", total - minimum);
								root["result"][ii]["CounterToday"] = szTmp;
							}
							else
							{
								sprintf(szData, "%.3f kWh", total);
								root["result"][ii]["Data"] = szData;
								sprintf(szData, "%ld Watt", atol(strarray[0].c_str()));
								root["result"][ii]["Usage"] = szData;
								root["result"][ii]["SwitchTypeVal"] = MTYPE_ENERGY;
								root["result"][ii]["HaveTimeout"] = bHaveTimeout;
							}
						}
					}
					else if (dType == pTypeAirQuality)
					{
						if (bHaveTimeout)
							nValue = 0;
						sprintf(szTmp, "%d ppm", nValue);
						root["result"][ii]["Data"] = szTmp;
						root["result"][ii]["HaveTimeout"] = bHaveTimeout;
						int airquality = nValue;
						if (airquality < 700)
							root["result"][ii]["Quality"] = "Excellent";
						else if (airquality < 900)
							root["result"][ii]["Quality"] = "Good";
						else if (airquality < 1100)
							root["result"][ii]["Quality"] = "Fair";
						else if (airquality < 1600)
							root["result"][ii]["Quality"] = "Mediocre";
						else
							root["result"][ii]["Quality"] = "Bad";
					}
					else if (dType == pTypeThermostat)
					{
						if (dSubType == sTypeThermSetpoint)
						{
							bHasTimers = m_sql.HasTimers(sd[0]);

							double tempCelcius = atof(sValue.c_str());
							double temp = ConvertTemperature(tempCelcius, tempsign);

							sprintf(szTmp, "%.1f", temp);
							root["result"][ii]["Data"] = szTmp;
							root["result"][ii]["SetPoint"] = szTmp;
							root["result"][ii]["HaveTimeout"] = bHaveTimeout;
							root["result"][ii]["TypeImg"] = "override_mini";
						}
					}
					else if (dType == pTypeRadiator1)
					{
						if (dSubType == sTypeSmartwares)
						{
							bHasTimers = m_sql.HasTimers(sd[0]);

							double tempCelcius = atof(sValue.c_str());
							double temp = ConvertTemperature(tempCelcius, tempsign);

							sprintf(szTmp, "%.1f", temp);
							root["result"][ii]["Data"] = szTmp;
							root["result"][ii]["SetPoint"] = szTmp;
							root["result"][ii]["HaveTimeout"] = bHaveTimeout;
							root["result"][ii]["TypeImg"] = "override_mini";
						}
					}
					else if (dType == pTypeGeneral)
					{
						if (dSubType == sTypeVisibility)
						{
							float vis = static_cast<float>(atof(sValue.c_str()));
							if (metertype == 0)
							{
								//km
								sprintf(szTmp, "%.1f km", vis);
							}
							else
							{
								//miles
								sprintf(szTmp, "%.1f mi", vis*0.6214f);
							}
							root["result"][ii]["Data"] = szTmp;
							root["result"][ii]["Visibility"] = atof(sValue.c_str());
							root["result"][ii]["HaveTimeout"] = bHaveTimeout;
							root["result"][ii]["TypeImg"] = "visibility";
							root["result"][ii]["SwitchTypeVal"] = metertype;
						}
						else if (dSubType == sTypeDistance)
						{
							float vis = static_cast<float>(atof(sValue.c_str()));
							if (metertype == 0)
							{
								//km
								sprintf(szTmp, "%.1f cm", vis);
							}
							else
							{
								//miles
								sprintf(szTmp, "%.1f in", vis*0.6214f);
							}
							root["result"][ii]["Data"] = szTmp;
							root["result"][ii]["HaveTimeout"] = bHaveTimeout;
							root["result"][ii]["TypeImg"] = "visibility";
							root["result"][ii]["SwitchTypeVal"] = metertype;
						}
						else if (dSubType == sTypeSolarRadiation)
						{
							float radiation = static_cast<float>(atof(sValue.c_str()));
							sprintf(szTmp, "%.1f Watt/m2", radiation);
							root["result"][ii]["Data"] = szTmp;
							root["result"][ii]["Radiation"] = atof(sValue.c_str());
							root["result"][ii]["HaveTimeout"] = bHaveTimeout;
							root["result"][ii]["TypeImg"] = "radiation";
							root["result"][ii]["SwitchTypeVal"] = metertype;
						}
						else if (dSubType == sTypeSoilMoisture)
						{
							sprintf(szTmp, "%d cb", nValue);
							root["result"][ii]["Data"] = szTmp;
							root["result"][ii]["Desc"] = Get_Moisture_Desc(nValue);
							root["result"][ii]["TypeImg"] = "moisture";
							root["result"][ii]["HaveTimeout"] = bHaveTimeout;
							root["result"][ii]["SwitchTypeVal"] = metertype;
						}
						else if (dSubType == sTypeLeafWetness)
						{
							sprintf(szTmp, "%d", nValue);
							root["result"][ii]["Data"] = szTmp;
							root["result"][ii]["TypeImg"] = "leaf";
							root["result"][ii]["HaveTimeout"] = bHaveTimeout;
							root["result"][ii]["SwitchTypeVal"] = metertype;
						}
						else if (dSubType == sTypeSystemTemp)
						{
							root["result"][ii]["AddjValue"] = AddjValue;
							root["result"][ii]["AddjMulti"] = AddjMulti;
							double tvalue = ConvertTemperature(atof(sValue.c_str()), tempsign);
							root["result"][ii]["Temp"] = tvalue;
							sprintf(szData, "%.1f %c", tvalue, tempsign);
							root["result"][ii]["Data"] = szData;
							root["result"][ii]["HaveTimeout"] = bHaveTimeout;
							root["result"][ii]["Image"] = "Computer";
							root["result"][ii]["TypeImg"] = "temperature";
							root["result"][ii]["Type"] = "temperature";
						}
						else if (dSubType == sTypePercentage)
						{
							sprintf(szData, "%.2f%%", atof(sValue.c_str()));
							root["result"][ii]["Data"] = szData;
							root["result"][ii]["HaveTimeout"] = bHaveTimeout;
							root["result"][ii]["Image"] = "Computer";
							root["result"][ii]["TypeImg"] = "hardware";
						}
						else if (dSubType == sTypeFan)
						{
							sprintf(szData, "%d RPM", atoi(sValue.c_str()));
							root["result"][ii]["Data"] = szData;
							root["result"][ii]["HaveTimeout"] = bHaveTimeout;
							root["result"][ii]["Image"] = "Fan";
							root["result"][ii]["TypeImg"] = "Fan";
							root["result"][ii]["Type"] = "Speaker";
						}
						else if (dSubType == sTypeSoundLevel)
						{
							sprintf(szData, "%d dB", atoi(sValue.c_str()));
							root["result"][ii]["Data"] = szData;
							root["result"][ii]["TypeImg"] = "Speaker";
							root["result"][ii]["HaveTimeout"] = bHaveTimeout;
						}
						else if (dSubType == sTypeVoltage)
						{
							sprintf(szData, "%.3f V", atof(sValue.c_str()));
							root["result"][ii]["Data"] = szData;
							root["result"][ii]["TypeImg"] = "current";
							root["result"][ii]["HaveTimeout"] = bHaveTimeout;
							root["result"][ii]["Voltage"] = atof(sValue.c_str());
						}
						else if (dSubType == sTypeCurrent)
						{
							sprintf(szData, "%.3f A", atof(sValue.c_str()));
							root["result"][ii]["Data"] = szData;
							root["result"][ii]["TypeImg"] = "current";
							root["result"][ii]["HaveTimeout"] = bHaveTimeout;
							root["result"][ii]["Current"] = atof(sValue.c_str());
						}
						else if (dSubType == sTypeTextStatus)
						{
							root["result"][ii]["Data"] = sValue;
							root["result"][ii]["TypeImg"] = "text";
							root["result"][ii]["HaveTimeout"] = false;
							root["result"][ii]["ShowNotifications"] = false;
						}
						else if (dSubType == sTypeAlert)
						{
							root["result"][ii]["Data"] = sValue;
							root["result"][ii]["TypeImg"] = "Alert";
							root["result"][ii]["Level"] = nValue;
							root["result"][ii]["HaveTimeout"] = false;
							root["result"][ii]["ShowNotifications"] = false;
						}
						else if (dSubType == sTypePressure)
						{
							sprintf(szData, "%.1f Bar", atof(sValue.c_str()));
							root["result"][ii]["Data"] = szData;
							root["result"][ii]["TypeImg"] = "gauge";
							root["result"][ii]["HaveTimeout"] = bHaveTimeout;
							root["result"][ii]["Pressure"] = atof(sValue.c_str());
						}
						else if (dSubType == sTypeBaro)
						{
							sprintf(szData, "%.1f hPa", atof(sValue.c_str()));
							root["result"][ii]["Data"] = szData;
							root["result"][ii]["TypeImg"] = "gauge";
							root["result"][ii]["HaveTimeout"] = bHaveTimeout;

							std::vector<std::string> tstrarray;
							StringSplit(sValue, ";", tstrarray);
							if (tstrarray.size() > 1)
							{
								root["result"][ii]["Barometer"] = atof(tstrarray[0].c_str());
								int forecast = atoi(tstrarray[1].c_str());
								root["result"][ii]["Forecast"] = forecast;
								root["result"][ii]["ForecastStr"] = BMP_Forecast_Desc(forecast);
							}
						}
						else if (dSubType == sTypeZWaveClock)
						{
							std::vector<std::string> tstrarray;
							StringSplit(sValue, ";", tstrarray);
							int day = 0;
							int hour = 0;
							int minute = 0;
							if (tstrarray.size() == 3)
							{
								day = atoi(tstrarray[0].c_str());
								hour = atoi(tstrarray[1].c_str());
								minute = atoi(tstrarray[2].c_str());
							}
							sprintf(szData, "%s %02d:%02d", ZWave_Clock_Days(day), hour, minute);
							root["result"][ii]["DayTime"] = sValue;
							root["result"][ii]["Data"] = szData;
							root["result"][ii]["HaveTimeout"] = bHaveTimeout;
							root["result"][ii]["TypeImg"] = "clock";
						}
						else if (dSubType == sTypeZWaveThermostatMode)
						{
							sprintf(szData, "%s", ZWave_Thermostat_Modes[nValue]);
							root["result"][ii]["Data"] = szData;
							root["result"][ii]["Mode"] = nValue;
							root["result"][ii]["TypeImg"] = "mode";
							root["result"][ii]["HaveTimeout"] = bHaveTimeout;
							bool bAddedSupportedModes = false;
							std::string modes = "";
							//Add supported modes
#ifdef WITH_OPENZWAVE
							if (pHardware)
							{
								if (pHardware->HwdType == HTYPE_OpenZWave)
								{
									COpenZWave *pZWave = (COpenZWave*)pHardware;
									unsigned long ID;
									std::stringstream s_strid;
									s_strid << std::hex << sd[1];
									s_strid >> ID;
									modes = pZWave->GetSupportedThermostatModes(ID);
									bAddedSupportedModes = !modes.empty();
								}
							}
#endif
							if (!bAddedSupportedModes)
							{
								//Add all modes
								int smode = 0;
								while (ZWave_Thermostat_Modes[smode]!=NULL)
								{
									sprintf(szTmp, "%d;%s;", smode, ZWave_Thermostat_Modes[smode]);
									modes += szTmp;
									smode++;
								}
							}
							root["result"][ii]["Modes"] = modes;
						}
						else if (dSubType == sTypeZWaveThermostatFanMode)
						{
							sprintf(szData, "%s", ZWave_Thermostat_Fan_Modes[nValue]);
							root["result"][ii]["Data"] = szData;
							root["result"][ii]["Mode"] = nValue;
							root["result"][ii]["TypeImg"] = "mode";
							root["result"][ii]["HaveTimeout"] = bHaveTimeout;
							//Add supported modes (add all for now)
							bool bAddedSupportedModes = false;
							std::string modes = "";
							//Add supported modes
#ifdef WITH_OPENZWAVE
							if (pHardware)
							{
								if (pHardware->HwdType == HTYPE_OpenZWave)
								{
									COpenZWave *pZWave = (COpenZWave*)pHardware;
									unsigned long ID;
									std::stringstream s_strid;
									s_strid << std::hex << sd[1];
									s_strid >> ID;
									modes = pZWave->GetSupportedThermostatFanModes(ID);
									bAddedSupportedModes = !modes.empty();
								}
							}
#endif
							if (!bAddedSupportedModes)
							{
								int smode = 0;
								while (ZWave_Thermostat_Fan_Modes[smode]!=NULL)
								{
									sprintf(szTmp, "%d;%s;", smode, ZWave_Thermostat_Fan_Modes[smode]);
									modes += szTmp;
									smode++;
								}
							}
							root["result"][ii]["Modes"] = modes;
						}
					}
					else if (dType == pTypeLux)
					{
						sprintf(szTmp, "%.0f Lux", atof(sValue.c_str()));
						root["result"][ii]["Data"] = szTmp;
						root["result"][ii]["HaveTimeout"] = bHaveTimeout;
					}
					else if (dType == pTypeWEIGHT)
					{
						sprintf(szTmp, "%.1f kg", atof(sValue.c_str()));
						root["result"][ii]["Data"] = szTmp;
						root["result"][ii]["HaveTimeout"] = false;
					}
					else if (dType == pTypeUsage)
					{
						if (dSubType == sTypeElectric)
						{
							sprintf(szData, "%.1f Watt", atof(sValue.c_str()));
							root["result"][ii]["Data"] = szData;
						}
						else
						{
							root["result"][ii]["Data"] = sValue;
						}
						root["result"][ii]["HaveTimeout"] = bHaveTimeout;
					}
					else if (dType == pTypeRFXSensor)
					{
						switch (dSubType)
						{
						case sTypeRFXSensorAD:
							sprintf(szData, "%d mV", atoi(sValue.c_str()));
							root["result"][ii]["TypeImg"] = "current";
							break;
						case sTypeRFXSensorVolt:
							sprintf(szData, "%d mV", atoi(sValue.c_str()));
							root["result"][ii]["TypeImg"] = "current";
							break;
						}
						root["result"][ii]["Data"] = szData;
						root["result"][ii]["HaveTimeout"] = bHaveTimeout;
					}
					else if (dType == pTypeRego6XXValue)
					{
						switch (dSubType)
						{
						case sTypeRego6XXStatus:
						{
							std::string lstatus = "On";

							if (atoi(sValue.c_str()) == 0)
							{
								lstatus = "Off";
							}
							root["result"][ii]["Status"] = lstatus;
							root["result"][ii]["HaveDimmer"] = false;
							root["result"][ii]["MaxDimLevel"] = 0;
							root["result"][ii]["HaveGroupCmd"] = false;
							root["result"][ii]["TypeImg"] = "utility";
							root["result"][ii]["SwitchTypeVal"] = STYPE_OnOff;
							root["result"][ii]["SwitchType"] = Switch_Type_Desc(STYPE_OnOff);
							sprintf(szData, "%d", atoi(sValue.c_str()));
							root["result"][ii]["Data"] = szData;
							root["result"][ii]["HaveTimeout"] = bHaveTimeout;
							root["result"][ii]["StrParam1"] = strParam1;
							root["result"][ii]["StrParam2"] = strParam2;
							root["result"][ii]["Protected"] = (iProtected != 0);

							if (CustomImage < static_cast<int>(m_custom_light_icons.size()))
								root["result"][ii]["Image"] = m_custom_light_icons[CustomImage].RootFile;
							else
								root["result"][ii]["Image"] = "Light";

							unsigned long long camIDX = m_mainworker.m_cameras.IsDevSceneInCamera(0, sd[0]);
							root["result"][ii]["UsedByCamera"] = (camIDX != 0) ? true : false;
							if (camIDX != 0) {
								std::stringstream scidx;
								scidx << camIDX;
								root["result"][ii]["CameraIdx"] = scidx.str();
							}

							root["result"][ii]["Level"] = 0;
							root["result"][ii]["LevelInt"] = atoi(sValue.c_str());
							root["result"][ii]["AddjValue"] = AddjValue;
							root["result"][ii]["AddjMulti"] = AddjMulti;
							root["result"][ii]["AddjValue2"] = AddjValue2;
							root["result"][ii]["AddjMulti2"] = AddjMulti2;
						}
							break;
						case sTypeRego6XXCounter:
						{
							//get value of today
							time_t now = mytime(NULL);
							struct tm tm1;
							localtime_r(&now, &tm1);

							struct tm ltime;
							ltime.tm_isdst = tm1.tm_isdst;
							ltime.tm_hour = 0;
							ltime.tm_min = 0;
							ltime.tm_sec = 0;
							ltime.tm_year = tm1.tm_year;
							ltime.tm_mon = tm1.tm_mon;
							ltime.tm_mday = tm1.tm_mday;

							char szDate[40];
							sprintf(szDate, "%04d-%02d-%02d", ltime.tm_year + 1900, ltime.tm_mon + 1, ltime.tm_mday);

							std::vector<std::vector<std::string> > result2;
							strcpy(szTmp, "0");
							szQuery.clear();
							szQuery.str("");
							szQuery << "SELECT MIN(Value), MAX(Value) FROM Meter WHERE (DeviceRowID=" << sd[0] << " AND Date>='" << szDate << "')";
							result2 = m_sql.query(szQuery.str());
							if (result2.size() > 0)
							{
								std::vector<std::string> sd2 = result2[0];

								unsigned long long total_min, total_max, total_real;

								std::stringstream s_str1(sd2[0]);
								s_str1 >> total_min;
								std::stringstream s_str2(sd2[1]);
								s_str2 >> total_max;
								total_real = total_max - total_min;
								sprintf(szTmp, "%llu", total_real);
							}
							root["result"][ii]["SwitchTypeVal"] = MTYPE_COUNTER;
							root["result"][ii]["Counter"] = sValue;
							root["result"][ii]["CounterToday"] = szTmp;
							root["result"][ii]["Data"] = sValue;
							root["result"][ii]["HaveTimeout"] = bHaveTimeout;
						}
							break;
						}
					}
					root["result"][ii]["Timers"] = (bHasTimers == true) ? "true" : "false";
					ii++;
				}
			}
		}


		std::string CWebServer::GetInternalCameraSnapshot()
		{
			m_retstr = "";
			std::vector<unsigned char> camimage;
			if (m_pWebEm->m_lastRequestPath.find("raspberry") != std::string::npos)
			{
				if (!m_mainworker.m_cameras.TakeRaspberrySnapshot(camimage))
					goto exitproc;
			}
			else
			{
				if (!m_mainworker.m_cameras.TakeUVCSnapshot(camimage))
					goto exitproc;
			}
			m_retstr.insert(m_retstr.begin(), camimage.begin(), camimage.end());
			m_pWebEm->m_outputfilename = "snapshot.jpg";
		exitproc:
			return m_retstr;
		}

		std::string CWebServer::GetCameraSnapshot()
		{
			m_retstr = "";
			std::vector<unsigned char> camimage;
			std::string idx = m_pWebEm->FindValue("idx");
			if (idx == "")
				goto exitproc;

			if (!m_mainworker.m_cameras.TakeSnapshot(idx, camimage))
				goto exitproc;
			m_retstr.insert(m_retstr.begin(), camimage.begin(), camimage.end());
			m_pWebEm->m_outputfilename = "snapshot.jpg";
		exitproc:
			return m_retstr;
		}

		std::string CWebServer::GetDatabaseBackup()
		{
			m_retstr = "";
			std::string OutputFileName = szUserDataFolder + "backup.db";
			if (m_sql.BackupDatabase(OutputFileName))
			{
				std::ifstream testFile(OutputFileName.c_str(), std::ios::binary);
				std::vector<char> fileContents((std::istreambuf_iterator<char>(testFile)),
					std::istreambuf_iterator<char>());
				if (fileContents.size() > 0)
				{
					m_retstr.insert(m_retstr.begin(), fileContents.begin(), fileContents.end());
					m_pWebEm->m_outputfilename = "domoticz.db";
				}
			}
			return m_retstr;
		}

		void CWebServer::RType_DeleteDevice(Json::Value &root)
		{
			if (m_pWebEm->m_actualuser_rights != 2)
			{
				//No admin user, and not allowed to be here
				return;
			}

			std::string idx = m_pWebEm->FindValue("idx");
			if (idx == "")
				return;

			root["status"] = "OK";
			root["title"] = "DeleteDevice";
			m_sql.DeleteDevice(idx);
			m_mainworker.m_scheduler.ReloadSchedules();
		}

		void CWebServer::RType_AddScene(Json::Value &root)
		{
			if (m_pWebEm->m_actualuser_rights != 2)
			{
				//No admin user, and not allowed to be here
				return;
			}

			std::string name = m_pWebEm->FindValue("name");
			if (name == "")
			{
				root["status"] = "ERR";
				root["message"] = "No Scene Name specified!";
				return;
			}
			std::string stype = m_pWebEm->FindValue("scenetype");
			if (stype == "")
			{
				root["status"] = "ERR";
				root["message"] = "No Scene Type specified!";
				return;
			}
			if (m_sql.DoesSceneByNameExits(name) == true)
			{
				root["status"] = "ERR";
				root["message"] = "A Scene with this Name already Exits!";
				return;
			}
			root["status"] = "OK";
			root["title"] = "AddScene";
			char szTmp[100];
			sprintf(szTmp,
				"INSERT INTO Scenes (Name,SceneType) VALUES ('%s',%d)",
				name.c_str(),
				atoi(stype.c_str())
				);
			m_sql.query(szTmp);
		}

		void CWebServer::RType_DeleteScene(Json::Value &root)
		{
			if (m_pWebEm->m_actualuser_rights != 2)
			{
				//No admin user, and not allowed to be here
				return;
			}

			std::string idx = m_pWebEm->FindValue("idx");
			if (idx == "")
				return;
			char szTmp[100];
			root["status"] = "OK";
			root["title"] = "DeleteScene";
			sprintf(szTmp, "DELETE FROM Scenes WHERE (ID == %s)", idx.c_str());
			m_sql.query(szTmp);
			sprintf(szTmp, "DELETE FROM SceneDevices WHERE (SceneRowID == %s)", idx.c_str());
			m_sql.query(szTmp);
			sprintf(szTmp, "DELETE FROM SceneTimers WHERE (SceneRowID == %s)", idx.c_str());
			m_sql.query(szTmp);
		}

		void CWebServer::RType_UpdateScene(Json::Value &root)
		{
			if (m_pWebEm->m_actualuser_rights != 2)
			{
				//No admin user, and not allowed to be here
				return;
			}

			std::string idx = m_pWebEm->FindValue("idx");
			std::string name = m_pWebEm->FindValue("name");
			if ((idx == "") || (name == ""))
				return;
			std::string stype = m_pWebEm->FindValue("scenetype");
			if (stype == "")
			{
				root["status"] = "ERR";
				root["message"] = "No Scene Type specified!";
				return;
			}
			std::string tmpstr = m_pWebEm->FindValue("protected");
			int iProtected = (tmpstr == "true") ? 1 : 0;

			std::string onaction = base64_decode(m_pWebEm->FindValue("onaction"));
			std::string offaction = base64_decode(m_pWebEm->FindValue("offaction"));

			char szTmp[200];

			root["status"] = "OK";
			root["title"] = "UpdateScene";
			sprintf(szTmp, "UPDATE Scenes SET Name='%s', SceneType=%d, Protected=%d, OnAction='%s', OffAction='%s' WHERE (ID == %s)",
				name.c_str(),
				atoi(stype.c_str()),
				iProtected,
				onaction.c_str(),
				offaction.c_str(),
				idx.c_str()
				);
			m_sql.query(szTmp);
		}

		void CWebServer::RType_CreateEvohomeSensor(Json::Value &root)
		{
			if (m_pWebEm->m_actualuser_rights != 2)
			{
				//No admin user, and not allowed to be here
				return;
			}

			std::string idx = m_pWebEm->FindValue("idx");
			std::string ssensortype = m_pWebEm->FindValue("sensortype");
			if ((idx == "") || (ssensortype == ""))
				return;

			bool bCreated = false;
			int iSensorType = atoi(ssensortype.c_str());

			int HwdID = atoi(idx.c_str());

			//Make a unique number for ID
			std::stringstream szQuery;
			std::vector<std::vector<std::string> > result;
			szQuery << "SELECT MAX(ID) FROM DeviceStatus";
			result = m_sql.query(szQuery.str());

			unsigned long nid = 1; //could be the first device ever

			if (result.size() > 0)
			{
				nid = atol(result[0][0].c_str());
			}
			nid += 92000;
			char ID[40];
			sprintf(ID, "%ld", nid);
			
			//get zone count
			szQuery.clear();
			szQuery.str("");
			szQuery << "SELECT COUNT(*) FROM DeviceStatus WHERE (HardwareID == " << HwdID << ") AND (Type==" << (int)iSensorType << ")";
			result = m_sql.query(szQuery.str());
			
			int nDevCount=0;
			if (result.size() > 0)
			{
				nDevCount = atol(result[0][0].c_str());
			}

			std::string devname;

			switch (iSensorType)
			{
			case pTypeEvohome: //Controller...should be 1 controller per hardware
				if (nDevCount>=1)
				{
					root["status"] = "ERR";
					root["message"] = "Maximum number of controllers reached";
					return;
				}
				m_sql.UpdateValue(HwdID, ID, 0, pTypeEvohome, sTypeEvohome, 10, 255, 0, "Normal", devname);
				bCreated = true;
				break;
			case pTypeEvohomeZone://max of 12 zones
				if (nDevCount>=CEvohome::m_nMaxZones)
				{
					root["status"] = "ERR";
					root["message"] = "Maximum number of supported zones reached";
					return;
				}
				m_sql.UpdateValue(HwdID, ID, nDevCount+1, pTypeEvohomeZone, sTypeEvohomeZone, 10, 255, 0, "0.0;0.0;Auto", devname);
				bCreated = true;
				break;
			case pTypeEvohomeWater://DHW...should be 1 per hardware
				if (nDevCount>=1)
				{
					root["status"] = "ERR";
					root["message"] = "Maximum number of DHW zones reached";
					return;
				}
				m_sql.UpdateValue(HwdID, ID, 1, pTypeEvohomeWater, sTypeEvohomeWater, 10, 255, 50, "0.0;Off;Auto", devname);
				bCreated = true;
				break;
			}
			if (bCreated)
			{
				root["status"] = "OK";
				root["title"] = "CreateEvohomeSensor";
			}
		}
		
		void CWebServer::RType_BindEvohome(Json::Value &root)
		{
			if (m_pWebEm->m_actualuser_rights != 2)
			{
				//No admin user, and not allowed to be here
				return;
			}

			std::string idx = m_pWebEm->FindValue("idx");
			std::string type = m_pWebEm->FindValue("devtype");
			int HwdID = atoi(idx.c_str());
			CDomoticzHardwareBase *pHardware = m_mainworker.GetHardware(HwdID);
			if (pHardware == NULL)
				return;
			if (pHardware->HwdType != HTYPE_EVOHOME_SERIAL)
				return;
			CEvohome *pEvoHW=(CEvohome*)pHardware;

			int nDevNo=0;
			int nID=0;
			if(type=="Relay")
			{
				//get dev count
				std::stringstream szQuery;
				std::vector<std::vector<std::string> > result;
				szQuery << "SELECT COUNT(*) FROM DeviceStatus WHERE (HardwareID == " << HwdID << ") AND (Type==" << (int)pTypeEvohomeRelay << ") AND (Unit>=64) AND (Unit<96)";
				result = m_sql.query(szQuery.str());
				
				int nDevCount=0;
				if (result.size() > 0)
				{
					nDevCount = atol(result[0][0].c_str());
				}
				
				if(nDevCount>=32)//arbitrary maximum
				{
					root["status"] = "ERR";
					root["message"] = "Maximum number of relays reached";
					return;
				}
				
				nDevNo=nDevCount+64;
				nID=pEvoHW->Bind(nDevNo,CEvohomeID::devRelay);
			}
			else if(type=="OutdoorSensor")
				nID=pEvoHW->Bind(0,CEvohomeID::devSensor);
			if(nID==0)
			{
				root["status"] = "ERR";
				root["message"] = "Timeout when binding device";
				return;
			}
			
			if(type=="Relay")
			{
				std::string devid(CEvohomeID::GetHexID(nID));
				
				std::stringstream szQuery;
				std::vector<std::vector<std::string> > result;
				szQuery << "SELECT ID,DeviceID,Name FROM DeviceStatus WHERE (HardwareID == " << HwdID << ") AND (DeviceID==" << devid << ")";
				result = m_sql.query(szQuery.str());
				if (result.size() > 0)
				{
					root["status"] = "ERR";
					root["message"] = "Device already exists";
					root["Used"] = true;
					root["Name"] = result[0][2];
					return;
				}
				
				std::string devname;
				m_sql.UpdateValue(HwdID, devid.c_str(), nDevNo, pTypeEvohomeRelay, sTypeEvohomeRelay, 10, 255, 0, "Off", devname);
				pEvoHW->SetRelayHeatDemand(nDevNo,0);//initialise heat demand
			}
			root["status"] = "OK";
			root["title"] = "BindEvohome";
			root["Used"] = false;
		}

		void CWebServer::RType_CreateVirtualSensor(Json::Value &root)
		{
			if (m_pWebEm->m_actualuser_rights != 2)
			{
				//No admin user, and not allowed to be here
				return;
			}

			std::string idx = m_pWebEm->FindValue("idx");
			std::string ssensortype = m_pWebEm->FindValue("sensortype");
			if ((idx == "") || (ssensortype == ""))
				return;

			bool bCreated = false;
			int iSensorType = atoi(ssensortype.c_str());

			int HwdID = atoi(idx.c_str());

			//Make a unique number for ID
			std::stringstream szQuery;
			std::vector<std::vector<std::string> > result;
			szQuery << "SELECT MAX(ID) FROM DeviceStatus";
			result = m_sql.query(szQuery.str());

			unsigned long nid = 1; //could be the first device ever

			if (result.size() > 0)
			{
				nid = atol(result[0][0].c_str());
			}
			nid += 82000;
			char ID[40];
			sprintf(ID, "%ld", nid);

			std::string devname;

			switch (iSensorType)
			{
			case 1:
				//Pressure (Bar)
				m_sql.UpdateValue(HwdID, ID, 1, pTypeGeneral, sTypePressure, 12, 255, 0, "0.0", devname);
				bCreated = true;
				break;
			case 2:
				//Percentage
				m_sql.UpdateValue(HwdID, ID, 1, pTypeGeneral, sTypePercentage, 12, 255, 0, "0.0", devname);
				bCreated = true;
				break;
			case 3:
				//Gas
				m_sql.UpdateValue(HwdID, ID, 1, pTypeP1Gas, sTypeP1Gas, 12, 255, 0, "0", devname);
				bCreated = true;
				break;
			case 4:
				//Voltage
				m_sql.UpdateValue(HwdID, ID, 1, pTypeGeneral, sTypeVoltage, 12, 255, 0, "0.000", devname);
				bCreated = true;
				break;
			case 5:
				//Text
				m_sql.UpdateValue(HwdID, ID, 1, pTypeGeneral, sTypeTextStatus, 12, 255, 0, "Hello World", devname);
				bCreated = true;
				break;
			case 6:
				//Switch
				{
					unsigned char ID1 = (unsigned char)((nid & 0xFF000000) >> 24);
					unsigned char ID2 = (unsigned char)((nid & 0x00FF0000) >> 16);
					unsigned char ID3 = (unsigned char)((nid & 0x0000FF00) >> 8);
					unsigned char ID4 = (unsigned char)((nid & 0x000000FF));
					sprintf(ID, "%X%02X%02X%02X", ID1, ID2, ID3, ID4);
					m_sql.UpdateValue(HwdID, ID, 1, pTypeLighting2, sTypeAC, 12, 255, 0, "15", devname);
					bCreated = true;
				}
				break;
			case 7:
				//Alert
				m_sql.UpdateValue(HwdID, ID, 1, pTypeGeneral, sTypeAlert, 12, 255, 0, "No Alert!", devname);
				bCreated = true;
				break;
			case 8:
				//Thermostat Setpoint
				{
					unsigned char ID1 = (unsigned char)((nid & 0xFF000000) >> 24);
					unsigned char ID2 = (unsigned char)((nid & 0x00FF0000) >> 16);
					unsigned char ID3 = (unsigned char)((nid & 0x0000FF00) >> 8);
					unsigned char ID4 = (unsigned char)((nid & 0x000000FF));
					sprintf(ID, "%X%02X%02X%02X", ID1, ID2, ID3, ID4);
				}
				m_sql.UpdateValue(HwdID, ID, 1, pTypeThermostat, sTypeThermSetpoint, 12, 255, 0, "20.5", devname);
				bCreated = true;
				break;
			case 9:
				//Current/Ampere
				m_sql.UpdateValue(HwdID, ID, 1, pTypeCURRENT, sTypeELEC1, 12, 255, 0, "0.0;0.0;0.0", devname);
				bCreated = true;
				break;
			case 10:
				//Sound Level
				m_sql.UpdateValue(HwdID, ID, 1, pTypeGeneral, sTypeSoundLevel, 12, 255, 0, "65", devname);
				bCreated = true;
				break;
			case 11:
				//Barometer (hPa)
				m_sql.UpdateValue(HwdID, ID, 1, pTypeGeneral, sTypeBaro, 12, 255, 0, "1021.34;0", devname);
				bCreated = true;
				break;
			case 12:
				//Visibility (km)
				m_sql.UpdateValue(HwdID, ID, 1, pTypeGeneral, sTypeVisibility, 12, 255, 0, "10.3", devname);
				bCreated = true;
				break;
			case 13:
				//Distance (cm)
				m_sql.UpdateValue(HwdID, ID, 1, pTypeGeneral, sTypeDistance, 12, 255, 0, "123.4", devname);
				bCreated = true;
				break;
			case 14: //Counter Incremental
				m_sql.UpdateValue(HwdID, ID, 1, pTypeGeneral, sTypeCounterIncremental, 12, 255, 0, "0", devname);
				bCreated = true;
				break;
			case pTypeTEMP:
				m_sql.UpdateValue(HwdID, ID, 1, pTypeTEMP, sTypeTEMP1, 10, 255, 0, "0.0", devname);
				bCreated = true;
				break;
			case pTypeHUM:
				m_sql.UpdateValue(HwdID, ID, 1, pTypeHUM, sTypeTEMP1, 10, 255, 50, "1", devname);
				bCreated = true;
				break;
			case pTypeTEMP_HUM:
				m_sql.UpdateValue(HwdID, ID, 1, pTypeTEMP_HUM, sTypeTH1, 10, 255, 0, "0.0;50;1", devname);
				bCreated = true;
				break;
			case pTypeTEMP_HUM_BARO:
				m_sql.UpdateValue(HwdID, ID, 1, pTypeTEMP_HUM_BARO, sTypeTHB1, 10, 255, 0, "0.0;50;1;1010;1", devname);
				bCreated = true;
				break;
			case pTypeWIND:
				m_sql.UpdateValue(HwdID, ID, 1, pTypeWIND, sTypeWIND1, 10, 255, 0, "0;N;0;0;0;0", devname);
				bCreated = true;
				break;
			case pTypeRAIN:
				m_sql.UpdateValue(HwdID, ID, 1, pTypeRAIN, sTypeRAIN3, 10, 255, 0, "0;0", devname);
				bCreated = true;
				break;
			case pTypeUV:
				m_sql.UpdateValue(HwdID, ID, 1, pTypeUV, sTypeUV1, 10, 255, 0, "0;0", devname);
				bCreated = true;
				break;
			case pTypeENERGY:
				m_sql.UpdateValue(HwdID, ID, 1, pTypeENERGY, sTypeELEC2, 10, 255, 0, "0;0.0", devname);
				bCreated = true;
				break;
			case pTypeRFXMeter:
				m_sql.UpdateValue(HwdID, ID, 1, pTypeRFXMeter, sTypeRFXMeterCount, 10, 255, 0, "0", devname);
				bCreated = true;
				break;
			case pTypeAirQuality:
				m_sql.UpdateValue(HwdID, ID, 1, pTypeAirQuality, sTypeVoltcraft, 10, 255, 0, devname);
				bCreated = true;
				break;
			case pTypeUsage:
				m_sql.UpdateValue(HwdID, ID, 1, pTypeUsage, sTypeElectric, 10, 255, 0, "0", devname);
				bCreated = true;
				break;
			case pTypeLux:
				m_sql.UpdateValue(HwdID, ID, 1, pTypeLux, sTypeLux, 10, 255, 0, "0", devname);
				bCreated = true;
				break;
			case pTypeP1Power:
				m_sql.UpdateValue(HwdID, ID, 1, pTypeP1Power, sTypeP1Power, 10, 255, 0, "0;0;0;0;0;0", devname);
				bCreated = true;
				break;
			}
			if (bCreated)
			{
				root["status"] = "OK";
				root["title"] = "CreateVirtualSensor";
			}
		}

		bool compareIconsByName(const http::server::CWebServer::_tCustomIcon &a, const http::server::CWebServer::_tCustomIcon &b)
		{
			return a.Title < b.Title;
		}

		void CWebServer::RType_CustomLightIcons(Json::Value &root)
		{
			std::vector<_tCustomIcon>::const_iterator itt;
			int ii = 0;

			std::vector<_tCustomIcon> temp_custom_light_icons = m_custom_light_icons;
			//Sort by name
			std::sort(temp_custom_light_icons.begin(), temp_custom_light_icons.end(), compareIconsByName);

			for (itt = temp_custom_light_icons.begin(); itt != temp_custom_light_icons.end(); ++itt)
			{
				root["result"][ii]["idx"] = itt->idx;
				root["result"][ii]["imageSrc"] = itt->RootFile;
				root["result"][ii]["text"] = itt->Title;
				root["result"][ii]["description"] = itt->Description;
				ii++;
			}
			root["status"] = "OK";
		}

		void CWebServer::RType_Plans(Json::Value &root)
		{
			root["status"] = "OK";
			root["title"] = "Plans";

			std::string sDisplayHidden = m_pWebEm->FindValue("displayhidden");
			bool bDisplayHidden = (sDisplayHidden == "1");

			std::stringstream szQuery;
			std::vector<std::vector<std::string> > result, result2;
			szQuery << "SELECT ID, Name, [Order] FROM Plans ORDER BY [Order]";
			result = m_sql.query(szQuery.str());
			if (result.size() > 0)
			{
				std::vector<std::vector<std::string> >::const_iterator itt;
				int ii = 0;
				for (itt = result.begin(); itt != result.end(); ++itt)
				{
					std::vector<std::string> sd = *itt;

					std::string Name = sd[1];
					bool bIsHidden = (Name[0] == '$');

					if ((bDisplayHidden) || (!bIsHidden))
					{
						root["result"][ii]["idx"] = sd[0];
						root["result"][ii]["Name"] = Name;
						root["result"][ii]["Order"] = sd[2];

						unsigned int totDevices = 0;

						szQuery.clear();
						szQuery.str("");
						szQuery << "SELECT COUNT(*) FROM DeviceToPlansMap WHERE (PlanID=='" << sd[0] << "')";
						result2 = m_sql.query(szQuery.str());
						if (result.size() > 0)
						{
							totDevices = (unsigned int)atoi(result2[0][0].c_str());
						}
						root["result"][ii]["Devices"] = totDevices;

						ii++;
					}
				}
			}
		}

		void CWebServer::RType_FloorPlans(Json::Value &root)
		{
			root["status"] = "OK";
			root["title"] = "Floorplans";

			std::stringstream szQuery;
			std::vector<std::vector<std::string> > result, result2, result3;

			szQuery.clear();
			szQuery.str("");
			szQuery << "SELECT Key, nValue, sValue FROM Preferences WHERE Key LIKE 'Floorplan%'";
			result = m_sql.query(szQuery.str());
			if (result.size() < 0)
				return;

			std::vector<std::vector<std::string> >::const_iterator itt;
			for (itt = result.begin(); itt != result.end(); ++itt)
			{
				std::vector<std::string> sd = *itt;
				std::string Key = sd[0];
				int nValue = atoi(sd[1].c_str());
				std::string sValue = sd[2];

				if (Key == "FloorplanPopupDelay")
				{
					root["PopupDelay"] = nValue;
				}
				if (Key == "FloorplanFullscreenMode")
				{
					root["FullscreenMode"] = nValue;
				}
				if (Key == "FloorplanAnimateZoom")
				{
					root["AnimateZoom"] = nValue;
				}
				if (Key == "FloorplanShowSensorValues")
				{
					root["ShowSensorValues"] = nValue;
				}
				if (Key == "FloorplanShowSwitchValues")
				{
					root["ShowSwitchValues"] = nValue;
				}
				if (Key == "FloorplanShowSceneNames")
				{
					root["ShowSceneNames"] = nValue;
				}
				if (Key == "FloorplanRoomColour")
				{
					root["RoomColour"] = sValue;
				}
				if (Key == "FloorplanActiveOpacity")
				{
					root["ActiveRoomOpacity"] = nValue;
				}
				if (Key == "FloorplanInactiveOpacity")
				{
					root["InactiveRoomOpacity"] = nValue;
				}
			}

			szQuery.clear();
			szQuery.str("");
			szQuery << "SELECT ID, Name, ImageFile, ScaleFactor, [Order] FROM Floorplans ORDER BY [Order]";
			result2 = m_sql.query(szQuery.str());
			if (result2.size() > 0)
			{
				std::vector<std::vector<std::string> >::const_iterator itt;
				int ii = 0;
				for (itt = result2.begin(); itt != result2.end(); ++itt)
				{
					std::vector<std::string> sd = *itt;

					root["result"][ii]["idx"] = sd[0];
					root["result"][ii]["Name"] = sd[1];
					root["result"][ii]["Image"] = sd[2];
					root["result"][ii]["ScaleFactor"] = sd[3];
					root["result"][ii]["Order"] = sd[4];

					unsigned int totPlans = 0;

					szQuery.clear();
					szQuery.str("");
					szQuery << "SELECT COUNT(*) FROM Plans WHERE (FloorplanID=='" << sd[0] << "')";
					result3 = m_sql.query(szQuery.str());
					if (result3.size() > 0)
					{
						totPlans = (unsigned int)atoi(result3[0][0].c_str());
					}
					root["result"][ii]["Plans"] = totPlans;

					ii++;
				}
			}
		}

		void CWebServer::RType_Scenes(Json::Value &root)
		{
			root["status"] = "OK";
			root["title"] = "Scenes";
			root["AllowWidgetOrdering"] = m_sql.m_bAllowWidgetOrdering;

			std::string sLastUpdate = m_pWebEm->FindValue("lastupdate");

			time_t LastUpdate = 0;
			if (sLastUpdate != "")
			{
				std::stringstream sstr;
				sstr << sLastUpdate;
				sstr >> LastUpdate;
			}

			time_t now = mytime(NULL);
			struct tm tm1;
			localtime_r(&now, &tm1);
			struct tm tLastUpdate;
			localtime_r(&now, &tLastUpdate);

			root["ActTime"] = static_cast<int>(now);

			std::stringstream szQuery;
			std::vector<std::vector<std::string> > result, result2;
			szQuery << "SELECT ID, Name, HardwareID, Favorite, nValue, SceneType, LastUpdate, Protected, DeviceID, Unit, OnAction, OffAction FROM Scenes ORDER BY [Order]";
			result = m_sql.query(szQuery.str());
			if (result.size() > 0)
			{
				std::vector<std::vector<std::string> >::const_iterator itt;
				int ii = 0;
				for (itt = result.begin(); itt != result.end(); ++itt)
				{
					std::vector<std::string> sd = *itt;

					std::string sLastUpdate = sd[6].c_str();

					if (LastUpdate != 0)
					{
						tLastUpdate.tm_isdst = tm1.tm_isdst;
						tLastUpdate.tm_year = atoi(sLastUpdate.substr(0, 4).c_str()) - 1900;
						tLastUpdate.tm_mon = atoi(sLastUpdate.substr(5, 2).c_str()) - 1;
						tLastUpdate.tm_mday = atoi(sLastUpdate.substr(8, 2).c_str());
						tLastUpdate.tm_hour = atoi(sLastUpdate.substr(11, 2).c_str());
						tLastUpdate.tm_min = atoi(sLastUpdate.substr(14, 2).c_str());
						tLastUpdate.tm_sec = atoi(sLastUpdate.substr(17, 2).c_str());
						time_t cLastUpdate = mktime(&tLastUpdate);
						if (cLastUpdate <= LastUpdate)
							continue;
					}

					unsigned char nValue = atoi(sd[4].c_str());
					unsigned char scenetype = atoi(sd[5].c_str());
					int iProtected = atoi(sd[7].c_str());
					int HardwareID = atoi(sd[2].c_str());
					std::string CodeDeviceName = "";

					if (HardwareID != 0)
					{
						CodeDeviceName = "? not found!";
					}

					std::string onaction = "";
					std::string offaction = "";

					std::string DeviceID = sd[8];
					int Unit = atoi(sd[9].c_str());
					onaction = base64_encode((const unsigned char*)sd[10].c_str(), sd[10].size());
					offaction = base64_encode((const unsigned char*)sd[11].c_str(), sd[11].size());

					if (HardwareID != 0)
					{
						//Get learn code device name
						szQuery.clear();
						szQuery.str("");
						szQuery << "SELECT Name FROM DeviceStatus WHERE (HardwareID==" << HardwareID << ") AND (DeviceID=='" << DeviceID << "') AND (Unit==" << Unit << ")";
						result2 = m_sql.query(szQuery.str());
						if (result2.size() > 0)
						{
							CodeDeviceName = result2[0][0];
						}
					}


					root["result"][ii]["idx"] = sd[0];
					root["result"][ii]["Name"] = sd[1];
					root["result"][ii]["HardwareID"] = HardwareID;
					root["result"][ii]["CodeDeviceName"] = CodeDeviceName;
					root["result"][ii]["Favorite"] = atoi(sd[3].c_str());
					root["result"][ii]["Protected"] = (iProtected != 0);
					root["result"][ii]["OnAction"] = onaction;
					root["result"][ii]["OffAction"] = offaction;

					if (scenetype == 0)
					{
						root["result"][ii]["Type"] = "Scene";
					}
					else
					{
						root["result"][ii]["Type"] = "Group";
					}

					root["result"][ii]["LastUpdate"] = sLastUpdate;

					if (nValue == 0)
						root["result"][ii]["Status"] = "Off";
					else if (nValue == 1)
						root["result"][ii]["Status"] = "On";
					else
						root["result"][ii]["Status"] = "Mixed";
					root["result"][ii]["Timers"] = (m_sql.HasSceneTimers(sd[0]) == true) ? "true" : "false";
					unsigned long long camIDX = m_mainworker.m_cameras.IsDevSceneInCamera(1, sd[0]);
					root["result"][ii]["UsedByCamera"] = (camIDX != 0) ? true : false;
					if (camIDX != 0) {
						std::stringstream scidx;
						scidx << camIDX;
						root["result"][ii]["CameraIdx"] = scidx.str();
					}
					ii++;
				}
			}
		}

		void CWebServer::RType_Hardware(Json::Value &root)
		{
			root["status"] = "OK";
			root["title"] = "Hardware";

#ifdef WITH_OPENZWAVE
			m_ZW_Hwidx = -1;
#endif			

			std::stringstream szQuery;
			std::vector<std::vector<std::string> > result;
			szQuery << "SELECT ID, Name, Enabled, Type, Address, Port, SerialPort, Username, Password, Mode1, Mode2, Mode3, Mode4, Mode5, Mode6, DataTimeout FROM Hardware ORDER BY ID ASC";
			result = m_sql.query(szQuery.str());
			if (result.size() > 0)
			{
				std::vector<std::vector<std::string> >::const_iterator itt;
				int ii = 0;
				for (itt = result.begin(); itt != result.end(); ++itt)
				{
					std::vector<std::string> sd = *itt;

					root["result"][ii]["idx"] = sd[0];
					root["result"][ii]["Name"] = sd[1];
					root["result"][ii]["Enabled"] = (sd[2] == "1") ? "true" : "false";
					root["result"][ii]["Type"] = atoi(sd[3].c_str());
					root["result"][ii]["Address"] = sd[4];
					root["result"][ii]["Port"] = atoi(sd[5].c_str());
					root["result"][ii]["SerialPort"] = sd[6];
					root["result"][ii]["Username"] = sd[7];
					root["result"][ii]["Password"] = sd[8];
					root["result"][ii]["Mode1"] = atoi(sd[9].c_str());
					root["result"][ii]["Mode2"] = atoi(sd[10].c_str());
					root["result"][ii]["Mode3"] = atoi(sd[11].c_str());
					root["result"][ii]["Mode4"] = atoi(sd[12].c_str());
					root["result"][ii]["Mode5"] = atoi(sd[13].c_str());
					root["result"][ii]["Mode6"] = atoi(sd[14].c_str());
					root["result"][ii]["DataTimeout"] = atoi(sd[15].c_str());

#ifdef WITH_OPENZWAVE
					//Special case for openzwave (status for nodes queried)
					CDomoticzHardwareBase *pHardware = m_mainworker.GetHardware(atoi(sd[0].c_str()));
					if (pHardware != NULL)
					{
						if (pHardware->HwdType == HTYPE_OpenZWave)
						{
							COpenZWave *pOZWHardware = (COpenZWave*)pHardware;
							root["result"][ii]["version"] = pOZWHardware->GetVersionLong();
							root["result"][ii]["NodesQueried"] = (pOZWHardware->m_awakeNodesQueried || pOZWHardware->m_allNodesQueried);
						}
					}
#endif
					ii++;
				}
			}
		}

		void CWebServer::RType_Devices(Json::Value &root)
		{
			std::string rfilter = m_pWebEm->FindValue("filter");
			std::string order = m_pWebEm->FindValue("order");
			std::string rused = m_pWebEm->FindValue("used");
			std::string rid = m_pWebEm->FindValue("rid");
			std::string planid = m_pWebEm->FindValue("plan");
			std::string floorid = m_pWebEm->FindValue("floor");
			std::string sDisplayHidden = m_pWebEm->FindValue("displayhidden");
			bool bDisplayHidden = (sDisplayHidden == "1");
			std::string sLastUpdate = m_pWebEm->FindValue("lastupdate");

			time_t LastUpdate = 0;
			if (sLastUpdate != "")
			{
				std::stringstream sstr;
				sstr << sLastUpdate;
				sstr >> LastUpdate;
			}

			root["status"] = "OK";
			root["title"] = "Devices";

			GetJSonDevices(root, rused, rfilter, order, rid, planid, floorid, bDisplayHidden, LastUpdate, false);
		}

		void CWebServer::RType_Cameras(Json::Value &root)
		{
			std::string rused = m_pWebEm->FindValue("used");

			root["status"] = "OK";
			root["title"] = "Cameras";

			std::stringstream szQuery;
			std::vector<std::vector<std::string> > result;
			if (rused == "true") {
				szQuery << "SELECT ID, Name, Enabled, Address, Port, Username, Password, ImageURL FROM Cameras WHERE (Enabled=='1') ORDER BY ID ASC";
			}
			else {
				szQuery << "SELECT ID, Name, Enabled, Address, Port, Username, Password, ImageURL FROM Cameras ORDER BY ID ASC";
			}
			result = m_sql.query(szQuery.str());
			if (result.size() > 0)
			{
				std::vector<std::vector<std::string> >::const_iterator itt;
				int ii = 0;
				for (itt = result.begin(); itt != result.end(); ++itt)
				{
					std::vector<std::string> sd = *itt;

					root["result"][ii]["idx"] = sd[0];
					root["result"][ii]["Name"] = sd[1];
					root["result"][ii]["Enabled"] = (sd[2] == "1") ? "true" : "false";
					root["result"][ii]["Address"] = sd[3];
					root["result"][ii]["Port"] = atoi(sd[4].c_str());
					root["result"][ii]["Username"] = base64_decode(sd[5]);
					root["result"][ii]["Password"] = base64_decode(sd[6]);
					root["result"][ii]["ImageURL"] = sd[7];
					ii++;
				}
			}
		}

		void CWebServer::RType_Users(Json::Value &root)
		{
			bool bHaveUser = (m_pWebEm->m_actualuser != "");
			int urights = 3;
			if (bHaveUser)
			{
				int iUser = -1;
				iUser = FindUser(m_pWebEm->m_actualuser.c_str());
				if (iUser != -1)
					urights = static_cast<int>(m_users[iUser].userrights);
			}
			if (urights < 2)
				return;

			root["status"] = "OK";
			root["title"] = "Users";

			std::stringstream szQuery;
			std::vector<std::vector<std::string> > result;
			szQuery << "SELECT ID, Active, Username, Password, Rights, RemoteSharing, TabsEnabled FROM USERS ORDER BY ID ASC";
			result = m_sql.query(szQuery.str());
			if (result.size() > 0)
			{
				std::vector<std::vector<std::string> >::const_iterator itt;
				int ii = 0;
				for (itt = result.begin(); itt != result.end(); ++itt)
				{
					std::vector<std::string> sd = *itt;

					root["result"][ii]["idx"] = sd[0];
					root["result"][ii]["Enabled"] = (sd[1] == "1") ? "true" : "false";
					root["result"][ii]["Username"] = base64_decode(sd[2]);
					root["result"][ii]["Password"] = sd[3];
					root["result"][ii]["Rights"] = atoi(sd[4].c_str());
					root["result"][ii]["RemoteSharing"] = atoi(sd[5].c_str());
					root["result"][ii]["TabsEnabled"] = atoi(sd[6].c_str());
					ii++;
				}
			}
		}

		void CWebServer::RType_Timers(Json::Value &root)
		{
			unsigned long long idx = 0;
			if (m_pWebEm->FindValue("idx") != "")
			{
				std::stringstream s_str(m_pWebEm->FindValue("idx"));
				s_str >> idx;
			}
			if (idx == 0)
				return;
			root["status"] = "OK";
			root["title"] = "Timers";
			char szTmp[50];

			std::stringstream szQuery;
			std::vector<std::vector<std::string> > result;
			szQuery << "SELECT ID, Active, [Date], Time, Type, Cmd, Level, Hue, Days, UseRandomness FROM Timers WHERE (DeviceRowID==" << idx << ") AND (TimerPlan==" << m_sql.m_ActiveTimerPlan << ") ORDER BY ID";
			result = m_sql.query(szQuery.str());
			if (result.size() > 0)
			{
				std::vector<std::vector<std::string> >::const_iterator itt;
				int ii = 0;
				for (itt = result.begin(); itt != result.end(); ++itt)
				{
					std::vector<std::string> sd = *itt;

					unsigned char iLevel = atoi(sd[6].c_str());
					if (iLevel == 0)
						iLevel = 100;

					int iTimerType = atoi(sd[4].c_str());
					std::string sdate = sd[2];
					if ((iTimerType == TTYPE_FIXEDDATETIME) && (sdate.size() == 10))
					{
						int Year = atoi(sdate.substr(0, 4).c_str());
						int Month = atoi(sdate.substr(5, 2).c_str());
						int Day = atoi(sdate.substr(8, 2).c_str());
						sprintf(szTmp, "%02d-%02d-%04d", Month, Day, Year);
						sdate = szTmp;
					}
					else
						sdate = "";

					root["result"][ii]["idx"] = sd[0];
					root["result"][ii]["Active"] = (atoi(sd[1].c_str()) == 0) ? "false" : "true";
					root["result"][ii]["Date"] = sdate;
					root["result"][ii]["Time"] = sd[3].substr(0, 5);
					root["result"][ii]["Type"] = iTimerType;
					root["result"][ii]["Cmd"] = atoi(sd[5].c_str());
					root["result"][ii]["Level"] = iLevel;
					root["result"][ii]["Hue"] = atoi(sd[7].c_str());
					root["result"][ii]["Days"] = atoi(sd[8].c_str());
					root["result"][ii]["Randomness"] = (atoi(sd[9].c_str()) != 0);
					ii++;
				}
			}
		}

		void CWebServer::Cmd_AddTimer(Json::Value &root)
		{
			if (m_pWebEm->m_actualuser_rights != 2)
			{
				//No admin user, and not allowed to be here
				return;
			}

			std::string idx = m_pWebEm->FindValue("idx");
			std::string active = m_pWebEm->FindValue("active");
			std::string stimertype = m_pWebEm->FindValue("timertype");
			std::string sdate = m_pWebEm->FindValue("date");
			std::string shour = m_pWebEm->FindValue("hour");
			std::string smin = m_pWebEm->FindValue("min");
			std::string randomness = m_pWebEm->FindValue("randomness");
			std::string scmd = m_pWebEm->FindValue("command");
			std::string sdays = m_pWebEm->FindValue("days");
			std::string slevel = m_pWebEm->FindValue("level");	//in percentage
			std::string shue = m_pWebEm->FindValue("hue");
			if (
				(idx == "") ||
				(active == "") ||
				(stimertype == "") ||
				(shour == "") ||
				(smin == "") ||
				(randomness == "") ||
				(scmd == "") ||
				(sdays == "")
				)
				return;
			unsigned char iTimerType = atoi(stimertype.c_str());

			char szTmp[200];
			time_t now = mytime(NULL);
			struct tm tm1;
			localtime_r(&now, &tm1);
			int Year = tm1.tm_year + 1900;
			int Month = tm1.tm_mon + 1;
			int Day = tm1.tm_mday;

			if (iTimerType == TTYPE_FIXEDDATETIME)
			{
				if (sdate.size() == 10)
				{
					Month = atoi(sdate.substr(0, 2).c_str());
					Day = atoi(sdate.substr(3, 2).c_str());
					Year = atoi(sdate.substr(6, 4).c_str());
				}
			}

			unsigned char hour = atoi(shour.c_str());
			unsigned char min = atoi(smin.c_str());
			unsigned char icmd = atoi(scmd.c_str());
			int days = atoi(sdays.c_str());
			unsigned char level = atoi(slevel.c_str());
			int hue = atoi(shue.c_str());
			root["status"] = "OK";
			root["title"] = "AddTimer";
			sprintf(szTmp,
				"INSERT INTO Timers (Active, DeviceRowID, [Date], Time, Type, UseRandomness, Cmd, Level, Hue, Days, TimerPlan) VALUES (%d,%s,'%04d-%02d-%02d','%02d:%02d',%d,%d,%d,%d,%d,%d,%d)",
				(active == "true") ? 1 : 0,
				idx.c_str(),
				Year, Month, Day,
				hour, min,
				iTimerType,
				(randomness == "true") ? 1 : 0,
				icmd,
				level,
				hue,
				days,
				m_sql.m_ActiveTimerPlan
				);
			m_sql.query(szTmp);
			m_mainworker.m_scheduler.ReloadSchedules();
		}

		void CWebServer::Cmd_UpdateTimer(Json::Value &root)
		{
			if (m_pWebEm->m_actualuser_rights != 2)
			{
				//No admin user, and not allowed to be here
				return;
			}

			std::string idx = m_pWebEm->FindValue("idx");
			std::string active = m_pWebEm->FindValue("active");
			std::string stimertype = m_pWebEm->FindValue("timertype");
			std::string sdate = m_pWebEm->FindValue("date");
			std::string shour = m_pWebEm->FindValue("hour");
			std::string smin = m_pWebEm->FindValue("min");
			std::string randomness = m_pWebEm->FindValue("randomness");
			std::string scmd = m_pWebEm->FindValue("command");
			std::string sdays = m_pWebEm->FindValue("days");
			std::string slevel = m_pWebEm->FindValue("level");	//in percentage
			std::string shue = m_pWebEm->FindValue("hue");
			if (
				(idx == "") ||
				(active == "") ||
				(stimertype == "") ||
				(shour == "") ||
				(smin == "") ||
				(randomness == "") ||
				(scmd == "") ||
				(sdays == "")
				)
				return;

			char szTmp[200];
			unsigned char iTimerType = atoi(stimertype.c_str());
			time_t now = mytime(NULL);
			struct tm tm1;
			localtime_r(&now, &tm1);
			int Year = tm1.tm_year + 1900;
			int Month = tm1.tm_mon + 1;
			int Day = tm1.tm_mday;

			if (iTimerType == TTYPE_FIXEDDATETIME)
			{
				if (sdate.size() == 10)
				{
					Month = atoi(sdate.substr(0, 2).c_str());
					Day = atoi(sdate.substr(3, 2).c_str());
					Year = atoi(sdate.substr(6, 4).c_str());
				}
			}

			unsigned char hour = atoi(shour.c_str());
			unsigned char min = atoi(smin.c_str());
			unsigned char icmd = atoi(scmd.c_str());
			int days = atoi(sdays.c_str());
			unsigned char level = atoi(slevel.c_str());
			int hue = atoi(shue.c_str());
			root["status"] = "OK";
			root["title"] = "UpdateTimer";
			sprintf(szTmp,
				"UPDATE Timers SET Active=%d, [Date]='%04d-%02d-%02d', Time='%02d:%02d', Type=%d, UseRandomness=%d, Cmd=%d, Level=%d, Hue=%d, Days=%d WHERE (ID == %s)",
				(active == "true") ? 1 : 0,
				Year, Month, Day,
				hour, min,
				iTimerType,
				(randomness == "true") ? 1 : 0,
				icmd,
				level,
				hue,
				days,
				idx.c_str()
				);
			m_sql.query(szTmp);
			m_mainworker.m_scheduler.ReloadSchedules();
		}

		void CWebServer::Cmd_DeleteTimer(Json::Value &root)
		{
			if (m_pWebEm->m_actualuser_rights != 2)
			{
				//No admin user, and not allowed to be here
				return;
			}

			std::string idx = m_pWebEm->FindValue("idx");
			if (idx == "")
				return;
			root["status"] = "OK";
			root["title"] = "DeleteTimer";
			char szTmp[100];
			sprintf(szTmp,
				"DELETE FROM Timers WHERE (ID == %s)",
				idx.c_str()
				);
			m_sql.query(szTmp);
			m_mainworker.m_scheduler.ReloadSchedules();
		}

		void CWebServer::Cmd_ClearTimers(Json::Value &root)
		{
			if (m_pWebEm->m_actualuser_rights != 2)
			{
				//No admin user, and not allowed to be here
				return;
			}

			std::string idx = m_pWebEm->FindValue("idx");
			if (idx == "")
				return;
			root["status"] = "OK";
			root["title"] = "ClearTimer";
			char szTmp[100];
			sprintf(szTmp,
				"DELETE FROM Timers WHERE (DeviceRowID == %s)",
				idx.c_str()
				);
			m_sql.query(szTmp);
			m_mainworker.m_scheduler.ReloadSchedules();
		}

		void CWebServer::RType_SetpointTimers(Json::Value &root)
		{
			unsigned long long idx = 0;
			if (m_pWebEm->FindValue("idx") != "")
			{
				std::stringstream s_str(m_pWebEm->FindValue("idx"));
				s_str >> idx;
			}
			if (idx == 0)
				return;
			root["status"] = "OK";
			root["title"] = "Timers";
			char szTmp[50];

			std::stringstream szQuery;
			std::vector<std::vector<std::string> > result;
			szQuery << "SELECT ID, Active, [Date], Time, Type, Temperature, Days FROM SetpointTimers WHERE (DeviceRowID==" << idx << ") AND (TimerPlan==" << m_sql.m_ActiveTimerPlan << ") ORDER BY ID";
			result = m_sql.query(szQuery.str());
			if (result.size() > 0)
			{
				std::vector<std::vector<std::string> >::const_iterator itt;
				int ii = 0;
				for (itt = result.begin(); itt != result.end(); ++itt)
				{
					std::vector<std::string> sd = *itt;

					int iTimerType = atoi(sd[4].c_str());
					std::string sdate = sd[2];
					if ((iTimerType == TTYPE_FIXEDDATETIME) && (sdate.size() == 10))
					{
						int Year = atoi(sdate.substr(0, 4).c_str());
						int Month = atoi(sdate.substr(5, 2).c_str());
						int Day = atoi(sdate.substr(8, 2).c_str());
						sprintf(szTmp, "%02d-%02d-%04d", Month, Day, Year);
						sdate = szTmp;
					}
					else
						sdate = "";

					root["result"][ii]["idx"] = sd[0];
					root["result"][ii]["Active"] = (atoi(sd[1].c_str()) == 0) ? "false" : "true";
					root["result"][ii]["Date"] = sdate;
					root["result"][ii]["Time"] = sd[3].substr(0, 5);
					root["result"][ii]["Type"] = iTimerType;
					root["result"][ii]["Temperature"] = atof(sd[5].c_str());
					root["result"][ii]["Days"] = atoi(sd[6].c_str());
					ii++;
				}
			}
		}

		void CWebServer::Cmd_SetSetpoint(Json::Value &root)
		{
			bool bHaveUser = (m_pWebEm->m_actualuser != "");
			int iUser = -1;
			int urights = 3;
			if (bHaveUser)
			{
				iUser = FindUser(m_pWebEm->m_actualuser.c_str());
				if (iUser != -1)
				{
					urights = static_cast<int>(m_users[iUser].userrights);
				}
			}
			if (urights < 1)
				return;

			std::string idx = m_pWebEm->FindValue("idx");
			std::string setpoint = m_pWebEm->FindValue("setpoint");
			if (
				(idx == "") ||
				(setpoint == "")
				)
				return;
			root["status"] = "OK";
			root["title"] = "SetSetpoint";
			if (iUser != -1)
			{
				_log.Log(LOG_STATUS, "User: %s initiated a SetPoint command", m_users[iUser].Username.c_str());
			}
			m_mainworker.SetSetPoint(idx, static_cast<float>(atof(setpoint.c_str())));
		}

		void CWebServer::Cmd_AddSetpointTimer(Json::Value &root)
		{
			if (m_pWebEm->m_actualuser_rights != 2)
			{
				//No admin user, and not allowed to be here
				return;
			}

			std::string idx = m_pWebEm->FindValue("idx");
			std::string active = m_pWebEm->FindValue("active");
			std::string stimertype = m_pWebEm->FindValue("timertype");
			std::string sdate = m_pWebEm->FindValue("date");
			std::string shour = m_pWebEm->FindValue("hour");
			std::string smin = m_pWebEm->FindValue("min");
			std::string stvalue = m_pWebEm->FindValue("tvalue");
			std::string sdays = m_pWebEm->FindValue("days");
			if (
				(idx == "") ||
				(active == "") ||
				(stimertype == "") ||
				(shour == "") ||
				(smin == "") ||
				(stvalue == "") ||
				(sdays == "")
				)
				return;
			unsigned char iTimerType = atoi(stimertype.c_str());

			char szTmp[200];
			time_t now = mytime(NULL);
			struct tm tm1;
			localtime_r(&now, &tm1);
			int Year = tm1.tm_year + 1900;
			int Month = tm1.tm_mon + 1;
			int Day = tm1.tm_mday;

			if (iTimerType == TTYPE_FIXEDDATETIME)
			{
				if (sdate.size() == 10)
				{
					Month = atoi(sdate.substr(0, 2).c_str());
					Day = atoi(sdate.substr(3, 2).c_str());
					Year = atoi(sdate.substr(6, 4).c_str());
				}
			}

			unsigned char hour = atoi(shour.c_str());
			unsigned char min = atoi(smin.c_str());
			int days = atoi(sdays.c_str());
			float temperature = static_cast<float>(atof(stvalue.c_str()));
			root["status"] = "OK";
			root["title"] = "AddSetpointTimer";
			sprintf(szTmp,
				"INSERT INTO SetpointTimers (Active, DeviceRowID, [Date], Time, Type, Temperature, Days, TimerPlan) VALUES (%d,%s,'%04d-%02d-%02d','%02d:%02d',%d,%.1f,%d,%d)",
				(active == "true") ? 1 : 0,
				idx.c_str(),
				Year, Month, Day,
				hour, min,
				iTimerType,
				temperature,
				days,
				m_sql.m_ActiveTimerPlan
				);
			m_sql.query(szTmp);
			m_mainworker.m_scheduler.ReloadSchedules();
		}

		void CWebServer::Cmd_UpdateSetpointTimer(Json::Value &root)
		{
			if (m_pWebEm->m_actualuser_rights != 2)
			{
				//No admin user, and not allowed to be here
				return;
			}

			std::string idx = m_pWebEm->FindValue("idx");
			std::string active = m_pWebEm->FindValue("active");
			std::string stimertype = m_pWebEm->FindValue("timertype");
			std::string sdate = m_pWebEm->FindValue("date");
			std::string shour = m_pWebEm->FindValue("hour");
			std::string smin = m_pWebEm->FindValue("min");
			std::string stvalue = m_pWebEm->FindValue("tvalue");
			std::string sdays = m_pWebEm->FindValue("days");
			if (
				(idx == "") ||
				(active == "") ||
				(stimertype == "") ||
				(shour == "") ||
				(smin == "") ||
				(stvalue == "") ||
				(sdays == "")
				)
				return;

			char szTmp[200];
			unsigned char iTimerType = atoi(stimertype.c_str());
			time_t now = mytime(NULL);
			struct tm tm1;
			localtime_r(&now, &tm1);
			int Year = tm1.tm_year + 1900;
			int Month = tm1.tm_mon + 1;
			int Day = tm1.tm_mday;

			if (iTimerType == TTYPE_FIXEDDATETIME)
			{
				if (sdate.size() == 10)
				{
					Month = atoi(sdate.substr(0, 2).c_str());
					Day = atoi(sdate.substr(3, 2).c_str());
					Year = atoi(sdate.substr(6, 4).c_str());
				}
			}

			unsigned char hour = atoi(shour.c_str());
			unsigned char min = atoi(smin.c_str());
			int days = atoi(sdays.c_str());
			float tempvalue = static_cast<float>(atof(stvalue.c_str()));
			root["status"] = "OK";
			root["title"] = "UpdateSetpointTimer";
			sprintf(szTmp,
				"UPDATE SetpointTimers SET Active=%d, [Date]='%04d-%02d-%02d', Time='%02d:%02d', Type=%d, Temperature=%.1f, Days=%d WHERE (ID == %s)",
				(active == "true") ? 1 : 0,
				Year, Month, Day,
				hour, min,
				iTimerType,
				tempvalue,
				days,
				idx.c_str()
				);
			m_sql.query(szTmp);
			m_mainworker.m_scheduler.ReloadSchedules();
		}

		void CWebServer::Cmd_DeleteSetpointTimer(Json::Value &root)
		{
			if (m_pWebEm->m_actualuser_rights != 2)
			{
				//No admin user, and not allowed to be here
				return;
			}

			std::string idx = m_pWebEm->FindValue("idx");
			if (idx == "")
				return;
			root["status"] = "OK";
			root["title"] = "DeleteSetpointTimer";
			char szTmp[100];
			sprintf(szTmp,
				"DELETE FROM SetpointTimers WHERE (ID == %s)",
				idx.c_str()
				);
			m_sql.query(szTmp);
			m_mainworker.m_scheduler.ReloadSchedules();
		}

		void CWebServer::Cmd_ClearSetpointTimers(Json::Value &root)
		{
			if (m_pWebEm->m_actualuser_rights != 2)
			{
				//No admin user, and not allowed to be here
				return;
			}

			std::string idx = m_pWebEm->FindValue("idx");
			if (idx == "")
				return;
			root["status"] = "OK";
			root["title"] = "ClearSetpointTimers";
			char szTmp[100];
			sprintf(szTmp,
				"DELETE FROM SetpointTimers WHERE (DeviceRowID == %s)",
				idx.c_str()
				);
			m_sql.query(szTmp);
			m_mainworker.m_scheduler.ReloadSchedules();
		}

		void CWebServer::RType_SceneTimers(Json::Value &root)
		{
			unsigned long long idx = 0;
			if (m_pWebEm->FindValue("idx") != "")
			{
				std::stringstream s_str(m_pWebEm->FindValue("idx"));
				s_str >> idx;
			}
			if (idx == 0)
				return;
			root["status"] = "OK";
			root["title"] = "SceneTimers";

			char szTmp[40];

			std::stringstream szQuery;
			std::vector<std::vector<std::string> > result;
			szQuery << "SELECT ID, Active, [Date], Time, Type, Cmd, Level, Hue, Days, UseRandomness FROM SceneTimers WHERE (SceneRowID==" << idx << ") AND (TimerPlan==" << m_sql.m_ActiveTimerPlan << ") ORDER BY ID";
			result = m_sql.query(szQuery.str());
			if (result.size() > 0)
			{
				std::vector<std::vector<std::string> >::const_iterator itt;
				int ii = 0;
				for (itt = result.begin(); itt != result.end(); ++itt)
				{
					std::vector<std::string> sd = *itt;

					unsigned char iLevel = atoi(sd[6].c_str());
					if (iLevel == 0)
						iLevel = 100;

					int iTimerType = atoi(sd[4].c_str());
					std::string sdate = sd[2];
					if ((iTimerType == TTYPE_FIXEDDATETIME) && (sdate.size() == 10))
					{
						int Year = atoi(sdate.substr(0, 4).c_str());
						int Month = atoi(sdate.substr(5, 2).c_str());
						int Day = atoi(sdate.substr(8, 2).c_str());
						sprintf(szTmp, "%02d-%02d-%04d", Month, Day, Year);
						sdate = szTmp;
					}
					else
						sdate = "";

					root["result"][ii]["idx"] = sd[0];
					root["result"][ii]["Active"] = (atoi(sd[1].c_str()) == 0) ? "false" : "true";
					root["result"][ii]["Date"] = sdate;
					root["result"][ii]["Time"] = sd[3].substr(0, 5);
					root["result"][ii]["Type"] = atoi(sd[4].c_str());
					root["result"][ii]["Cmd"] = atoi(sd[5].c_str());
					root["result"][ii]["Level"] = iLevel;
					root["result"][ii]["Hue"] = atoi(sd[7].c_str());
					root["result"][ii]["Days"] = atoi(sd[8].c_str());
					root["result"][ii]["Randomness"] = (atoi(sd[9].c_str()) != 0);
					ii++;
				}
			}
		}

		void CWebServer::Cmd_AddSceneTimer(Json::Value &root)
		{
			if (m_pWebEm->m_actualuser_rights != 2)
			{
				//No admin user, and not allowed to be here
				return;
			}

			std::string idx = m_pWebEm->FindValue("idx");
			std::string active = m_pWebEm->FindValue("active");
			std::string stimertype = m_pWebEm->FindValue("timertype");
			std::string sdate = m_pWebEm->FindValue("date");
			std::string shour = m_pWebEm->FindValue("hour");
			std::string smin = m_pWebEm->FindValue("min");
			std::string randomness = m_pWebEm->FindValue("randomness");
			std::string scmd = m_pWebEm->FindValue("command");
			std::string sdays = m_pWebEm->FindValue("days");
			std::string slevel = m_pWebEm->FindValue("level");	//in percentage
			if (
				(idx == "") ||
				(active == "") ||
				(stimertype == "") ||
				(shour == "") ||
				(smin == "") ||
				(randomness == "") ||
				(scmd == "") ||
				(sdays == "")
				)
				return;
			unsigned char iTimerType = atoi(stimertype.c_str());

			char szTmp[200];
			time_t now = mytime(NULL);
			struct tm tm1;
			localtime_r(&now, &tm1);
			int Year = tm1.tm_year + 1900;
			int Month = tm1.tm_mon + 1;
			int Day = tm1.tm_mday;

			if (iTimerType == TTYPE_FIXEDDATETIME)
			{
				if (sdate.size() == 10)
				{
					Month = atoi(sdate.substr(0, 2).c_str());
					Day = atoi(sdate.substr(3, 2).c_str());
					Year = atoi(sdate.substr(6, 4).c_str());
				}
			}

			unsigned char hour = atoi(shour.c_str());
			unsigned char min = atoi(smin.c_str());
			unsigned char icmd = atoi(scmd.c_str());
			int days = atoi(sdays.c_str());
			unsigned char level = atoi(slevel.c_str());
			root["status"] = "OK";
			root["title"] = "AddSceneTimer";
			sprintf(szTmp,
				"INSERT INTO SceneTimers (Active, SceneRowID, [Date], Time, Type, UseRandomness, Cmd, Level, Days, TimerPlan) VALUES (%d,%s,'%04d-%02d-%02d','%02d:%02d',%d,%d,%d,%d,%d,%d)",
				(active == "true") ? 1 : 0,
				idx.c_str(),
				Year, Month, Day,
				hour, min,
				iTimerType,
				(randomness == "true") ? 1 : 0,
				icmd,
				level,
				days,
				m_sql.m_ActiveTimerPlan
				);
			m_sql.query(szTmp);
			m_mainworker.m_scheduler.ReloadSchedules();
		}

		void CWebServer::Cmd_UpdateSceneTimer(Json::Value &root)
		{
			if (m_pWebEm->m_actualuser_rights != 2)
			{
				//No admin user, and not allowed to be here
				return;
			}

			std::string idx = m_pWebEm->FindValue("idx");
			std::string active = m_pWebEm->FindValue("active");
			std::string stimertype = m_pWebEm->FindValue("timertype");
			std::string sdate = m_pWebEm->FindValue("date");
			std::string shour = m_pWebEm->FindValue("hour");
			std::string smin = m_pWebEm->FindValue("min");
			std::string randomness = m_pWebEm->FindValue("randomness");
			std::string scmd = m_pWebEm->FindValue("command");
			std::string sdays = m_pWebEm->FindValue("days");
			std::string slevel = m_pWebEm->FindValue("level");	//in percentage
			if (
				(idx == "") ||
				(active == "") ||
				(stimertype == "") ||
				(shour == "") ||
				(smin == "") ||
				(randomness == "") ||
				(scmd == "") ||
				(sdays == "")
				)
				return;

			unsigned char iTimerType = atoi(stimertype.c_str());

			char szTmp[200];
			time_t now = mytime(NULL);
			struct tm tm1;
			localtime_r(&now, &tm1);
			int Year = tm1.tm_year + 1900;
			int Month = tm1.tm_mon + 1;
			int Day = tm1.tm_mday;

			if (iTimerType == TTYPE_FIXEDDATETIME)
			{
				if (sdate.size() == 10)
				{
					Month = atoi(sdate.substr(0, 2).c_str());
					Day = atoi(sdate.substr(3, 2).c_str());
					Year = atoi(sdate.substr(6, 4).c_str());
				}
			}

			unsigned char hour = atoi(shour.c_str());
			unsigned char min = atoi(smin.c_str());
			unsigned char icmd = atoi(scmd.c_str());
			int days = atoi(sdays.c_str());
			unsigned char level = atoi(slevel.c_str());
			root["status"] = "OK";
			root["title"] = "UpdateSceneTimer";
			sprintf(szTmp,
				"UPDATE SceneTimers SET Active=%d, [Date]='%04d-%02d-%02d', Time='%02d:%02d', Type=%d, UseRandomness=%d, Cmd=%d, Level=%d, Days=%d WHERE (ID == %s)",
				(active == "true") ? 1 : 0,
				Year, Month, Day,
				hour, min,
				iTimerType,
				(randomness == "true") ? 1 : 0,
				icmd,
				level,
				days,
				idx.c_str()
				);
			m_sql.query(szTmp);
			m_mainworker.m_scheduler.ReloadSchedules();
		}

		void CWebServer::Cmd_DeleteSceneTimer(Json::Value &root)
		{
			if (m_pWebEm->m_actualuser_rights != 2)
			{
				//No admin user, and not allowed to be here
				return;
			}

			std::string idx = m_pWebEm->FindValue("idx");
			if (idx == "")
				return;
			root["status"] = "OK";
			root["title"] = "DeleteSceneTimer";
			char szTmp[100];
			sprintf(szTmp,
				"DELETE FROM SceneTimers WHERE (ID == %s)",
				idx.c_str()
				);
			m_sql.query(szTmp);
			m_mainworker.m_scheduler.ReloadSchedules();
		}

		void CWebServer::Cmd_ClearSceneTimers(Json::Value &root)
		{
			if (m_pWebEm->m_actualuser_rights != 2)
			{
				//No admin user, and not allowed to be here
				return;
			}

			std::string idx = m_pWebEm->FindValue("idx");
			if (idx == "")
				return;
			root["status"] = "OK";
			root["title"] = "ClearSceneTimer";
			char szTmp[100];
			sprintf(szTmp,
				"DELETE FROM SceneTimers WHERE (SceneRowID == %s)",
				idx.c_str()
				);
			m_sql.query(szTmp);
			m_mainworker.m_scheduler.ReloadSchedules();
		}

		void CWebServer::Cmd_SetSceneCode(Json::Value &root)
		{
			if (m_pWebEm->m_actualuser_rights != 2)
			{
				//No admin user, and not allowed to be here
				return;
			}

			std::string idx = m_pWebEm->FindValue("idx");
			std::string cmnd = m_pWebEm->FindValue("cmnd");
			if (
				(idx == "") ||
				(cmnd == "")
				)
				return;
			std::string devid = m_pWebEm->FindValue("devid");
			if (devid == "")
				return;
			root["status"] = "OK";
			root["title"] = "SetSceneCode";

			char szTmp[200];

			std::stringstream szQuery;
			std::vector<std::vector<std::string> > result;
			szQuery << "SELECT HardwareID, DeviceID, Unit, Type, SubType FROM DeviceStatus WHERE (ID==" << devid << ")";
			result = m_sql.query(szQuery.str());
			if (result.size() > 0)
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
				m_sql.query(szTmp);
				//Sanity Check, remove all SceneDevice that has this code
				szQuery.clear();
				szQuery.str("");
				szQuery << "DELETE FROM SceneDevices WHERE (SceneRowID==" << idx << " AND DeviceRowID==" << devid << ")";
				m_sql.query(szQuery.str()); //-V519
			}
		}

		void CWebServer::Cmd_RemoveSceneCode(Json::Value &root)
		{
			if (m_pWebEm->m_actualuser_rights != 2)
			{
				//No admin user, and not allowed to be here
				return;
			}

			std::string idx = m_pWebEm->FindValue("idx");
			if (idx == "")
				return;
			root["status"] = "OK";
			root["title"] = "RemoveSceneCode";
			char szTmp[100];
			sprintf(szTmp,
				"UPDATE Scenes SET HardwareID=%d, DeviceID='%s', Unit=%d, Type=%d, SubType=%d WHERE (ID == %s)",
				0,
				"",
				0,
				0,
				0,
				idx.c_str()
				);
			m_sql.query(szTmp);
		}

		void CWebServer::Cmd_GetSerialDevices(Json::Value &root)
		{
			root["status"] = "OK";
			root["title"] = "GetSerialDevices";

			bool bUseDirectPath = false;
			std::vector<std::string> serialports = GetSerialPorts(bUseDirectPath);
			std::vector<std::string>::iterator itt;
			int iDevice = 0;
			int ii = 0;
			for (itt = serialports.begin(); itt != serialports.end(); ++itt)
			{
				root["result"][ii]["name"] = *itt;
				root["result"][ii]["value"] = ii;
				ii++;
			}
		}

		void CWebServer::Cmd_GetDevicesList(Json::Value &root)
		{
			root["status"] = "OK";
			root["title"] = "GetDevicesList";
			int ii = 0;
			std::vector<std::vector<std::string> > result;
			result = m_sql.query("SELECT ID, Name FROM DeviceStatus WHERE (Used == 1) ORDER BY Name");
			if (result.size() > 0)
			{
				std::vector<std::vector<std::string> >::const_iterator itt;
				for (itt = result.begin(); itt != result.end(); ++itt)
				{
					std::vector<std::string> sd = *itt;
					root["result"][ii]["name"] = sd[1];
					root["result"][ii]["value"] = sd[0];
					ii++;
				}
			}
		}

		void CWebServer::Cmd_GetDevicesListOnOff(Json::Value &root)
		{
			root["status"] = "OK";
			root["title"] = "GetDevicesListOnOff";
			int ii = 0;
			std::vector<std::vector<std::string> > result;
			result = m_sql.query("SELECT ID, Name, Type, SubType FROM DeviceStatus WHERE (Used == 1) ORDER BY Name");
			if (result.size() > 0)
			{
				std::vector<std::vector<std::string> >::const_iterator itt;
				for (itt = result.begin(); itt != result.end(); ++itt)
				{
					std::vector<std::string> sd = *itt;
					int dType = atoi(sd[2].c_str());
					int dSubType = atoi(sd[3].c_str());
					std::string sOptions = RFX_Type_SubType_Values(dType, dSubType);
					if (sOptions == "Status")
					{
						root["result"][ii]["name"] = sd[1];
						root["result"][ii]["value"] = sd[0];
						ii++;
					}
				}
			}
		}

		std::string CWebServer::Post_UploadCustomIcon()
		{
			Json::Value root;
			root["title"] = "UploadCustomIcon";
			root["status"] = "ERROR";
			root["error"] = "Invalid";
			if (m_pWebEm->m_actualuser_rights == 2)
			{
				//Only admin user allowed
				std::string zipfile = m_pWebEm->FindValue("file");
				if (zipfile != "")
				{
					std::string ErrorMessage;
					bool bOK = m_sql.InsertCustomIconFromZip(zipfile, ErrorMessage);
					if (bOK)
					{
						root["status"] = "OK";
					}
					else
					{
						root["status"] = "ERROR";
						root["error"] = ErrorMessage;
					}
				}
			}
			std::string jcallback = m_pWebEm->FindValue("jsoncallback");
			if (jcallback.size() == 0)
				m_retstr = root.toStyledString();
			else
			{
				m_retstr = "var data=" + root.toStyledString() + "\n" + jcallback + "(data);";
			}
			return m_retstr;
		}


		void CWebServer::Cmd_RegisterWithPhilipsHue(Json::Value &root)
		{
			root["title"] = "RegisterOnHue";

			std::string sipaddress = m_pWebEm->FindValue("ipaddress");
			std::string sport = m_pWebEm->FindValue("port");
					std::string susername = m_pWebEm->FindValue("username");
			if (
				(sipaddress == "") ||
				(sport == "")
				)
				return;

			std::string sresult = CPhilipsHue::RegisterUser(sipaddress,(unsigned short)atoi(sport.c_str()),susername);
			std::vector<std::string> strarray;
			StringSplit(sresult, ";", strarray);
			if (strarray.size() != 2)
				return;

			if (strarray[0] == "Error") {
				root["statustext"] = strarray[1];
				return;
			}
			root["status"] = "OK";
			root["username"] = strarray[1];
		}

		void CWebServer::Cmd_GetCustomIconSet(Json::Value &root)
		{
			root["status"] = "OK";
			root["title"] = "GetCustomIconSet";
			int ii = 0;
			std::vector<_tCustomIcon>::const_iterator itt;
			for (itt = m_custom_light_icons.begin(); itt != m_custom_light_icons.end(); ++itt)
			{
				if (itt->idx >= 100)
				{
					std::string IconFile16 = "images/" + itt->RootFile + ".png";
					std::string IconFile48On = "images/" + itt->RootFile + "48_On.png";
					std::string IconFile48Off = "images/" + itt->RootFile + "48_Off.png";

					root["result"][ii]["idx"] = itt->idx - 100;
					root["result"][ii]["Title"] = itt->Title;
					root["result"][ii]["Description"] = itt->Description;
					root["result"][ii]["IconFile16"] = IconFile16;
					root["result"][ii]["IconFile48On"] = IconFile48On;
					root["result"][ii]["IconFile48Off"] = IconFile48Off;
					ii++;
				}
			}
		}

		void CWebServer::Cmd_DeleteCustomIcon(Json::Value &root)
		{
			if (m_pWebEm->m_actualuser_rights != 2)
			{
				//No admin user, and not allowed to be here
				return;
			}

			std::string sidx = m_pWebEm->FindValue("idx");
			if (sidx == "")
				return;
			int idx = atoi(sidx.c_str());
			root["status"] = "OK";
			root["title"] = "DeleteCustomIcon";

			std::stringstream szQuery;
			std::vector<std::vector<std::string> > result;
			szQuery << "DELETE FROM CustomImages WHERE (ID == " << idx << ")";
			result = m_sql.query(szQuery.str());

			//Delete icons file from disk
			std::vector<_tCustomIcon>::const_iterator itt;
			for (itt = m_custom_light_icons.begin(); itt != m_custom_light_icons.end(); ++itt)
			{
				if (itt->idx == idx+100)
				{
					std::string IconFile16 = szWWWFolder + "/images/" + itt->RootFile + ".png";
					std::string IconFile48On = szWWWFolder + "/images/" + itt->RootFile + "48_On.png";
					std::string IconFile48Off = szWWWFolder + "/images/" + itt->RootFile + "48_Off.png";
					std::remove(IconFile16.c_str());
					std::remove(IconFile48On.c_str());
					std::remove(IconFile48Off.c_str());
					break;
				}
			}
			ReloadCustomSwitchIcons();
		}

		void CWebServer::Cmd_RenameDevice(Json::Value &root)
		{
			if (m_pWebEm->m_actualuser_rights != 2)
			{
				//No admin user, and not allowed to be here
				return;
			}

			std::string sidx = m_pWebEm->FindValue("idx");
			std::string sname = m_pWebEm->FindValue("name");
			if (
				(sidx == "")||
				(sname == "")
				)
				return;
			int idx = atoi(sidx.c_str());
			root["status"] = "OK";
			root["title"] = "RenameDevice";

			std::stringstream szQuery;
			std::vector<std::vector<std::string> > result;
			szQuery << "UPDATE DeviceStatus SET Name='" << sname << "' WHERE (ID == " << idx << ")";
			result = m_sql.query(szQuery.str());
		}

		void CWebServer::RType_GetTransfers(Json::Value &root)
		{
			root["status"] = "OK";
			root["title"] = "GetTransfers";

			unsigned long long idx = 0;
			if (m_pWebEm->FindValue("idx") != "")
			{
				std::stringstream s_str(m_pWebEm->FindValue("idx"));
				s_str >> idx;
			}

			std::stringstream szQuery;
			std::vector<std::vector<std::string> > result;
			szQuery << "SELECT Type, SubType FROM DeviceStatus WHERE (ID==" << idx << ")";
			result = m_sql.query(szQuery.str());
			if (result.size() > 0)
			{
				szQuery.clear();
				szQuery.str("");

				int dType = atoi(result[0][0].c_str());
				if (
					(dType == pTypeTEMP) ||
					(dType == pTypeTEMP_HUM)
					)
				{
					szQuery << "SELECT ID, Name FROM DeviceStatus WHERE (Type==" << result[0][0] << ") AND (ID!=" << idx << ")";
				}
				else
				{
					szQuery << "SELECT ID, Name FROM DeviceStatus WHERE (Type==" << result[0][0] << ") AND (SubType==" << result[0][1] << ") AND (ID!=" << idx << ")";
				}
				result = m_sql.query(szQuery.str());

				std::vector<std::vector<std::string> >::const_iterator itt;
				int ii = 0;
				for (itt = result.begin(); itt != result.end(); ++itt)
				{
					std::vector<std::string> sd = *itt;

					root["result"][ii]["idx"] = sd[0];
					root["result"][ii]["Name"] = sd[1];
					ii++;
				}
			}
		}

		//Will transfer Newest sensor log to OLD sensor,
		//then set the HardwareID/DeviceID/Unit/Name/Type/Subtype/Unit for the OLD sensor to the NEW sensor ID/Type/Subtype/Unit
		//then delete the NEW sensor
		void CWebServer::RType_TransferDevice(Json::Value &root)
		{
			std::string sidx = m_pWebEm->FindValue("idx");
			if (sidx == "")
				return;

			std::string newidx = m_pWebEm->FindValue("newidx");
			if (newidx == "")
				return;

			std::stringstream szQuery;
			std::vector<std::vector<std::string> > result;

			//Check which device is newer

			time_t now = mytime(NULL);
			struct tm tm1;
			localtime_r(&now, &tm1);
			struct tm LastUpdateTime_A;
			struct tm LastUpdateTime_B;

			szQuery << "SELECT A.LastUpdate, B.LastUpdate FROM DeviceStatus as A, DeviceStatus as B WHERE (A.ID == " << sidx << ") AND (B.ID == " << newidx << ")";
			result = m_sql.query(szQuery.str());
			if (result.size() < 1)
				return;

			std::string sLastUpdate_A = result[0][0];
			std::string sLastUpdate_B = result[0][1];

			LastUpdateTime_A.tm_isdst = tm1.tm_isdst;
			LastUpdateTime_A.tm_year = atoi(sLastUpdate_A.substr(0, 4).c_str()) - 1900;
			LastUpdateTime_A.tm_mon = atoi(sLastUpdate_A.substr(5, 2).c_str()) - 1;
			LastUpdateTime_A.tm_mday = atoi(sLastUpdate_A.substr(8, 2).c_str());
			LastUpdateTime_A.tm_hour = atoi(sLastUpdate_A.substr(11, 2).c_str());
			LastUpdateTime_A.tm_min = atoi(sLastUpdate_A.substr(14, 2).c_str());
			LastUpdateTime_A.tm_sec = atoi(sLastUpdate_A.substr(17, 2).c_str());

			LastUpdateTime_B.tm_isdst = tm1.tm_isdst;
			LastUpdateTime_B.tm_year = atoi(sLastUpdate_B.substr(0, 4).c_str()) - 1900;
			LastUpdateTime_B.tm_mon = atoi(sLastUpdate_B.substr(5, 2).c_str()) - 1;
			LastUpdateTime_B.tm_mday = atoi(sLastUpdate_B.substr(8, 2).c_str());
			LastUpdateTime_B.tm_hour = atoi(sLastUpdate_B.substr(11, 2).c_str());
			LastUpdateTime_B.tm_min = atoi(sLastUpdate_B.substr(14, 2).c_str());
			LastUpdateTime_B.tm_sec = atoi(sLastUpdate_B.substr(17, 2).c_str());

			time_t timeA = mktime(&LastUpdateTime_A);
			time_t timeB = mktime(&LastUpdateTime_B);

			if (timeA < timeB)
			{
				//Swap idx with newidx
				sidx.swap(newidx);
			}

			szQuery.clear();
			szQuery.str("");
			szQuery << "SELECT HardwareID, DeviceID, Unit, Name, Type, SubType, SignalLevel, BatteryLevel, nValue, sValue FROM DeviceStatus WHERE (ID == " << newidx << ")";
			result = m_sql.query(szQuery.str());
			if (result.size() < 1)
				return;

			root["status"] = "OK";
			root["title"] = "TransferDevice";

			//transfer device logs (new to old)
			m_sql.TransferDevice(newidx, sidx);

			//now delete the NEW device
			m_sql.DeleteDevice(newidx);

			m_mainworker.m_scheduler.ReloadSchedules();
		}

		void CWebServer::RType_Notifications(Json::Value &root)
		{
			root["status"] = "OK";
			root["title"] = "Notifications";

			int ii = 0;

			//Add known notification systems
			std::map<std::string, CNotificationBase*>::const_iterator ittNotifiers;
			for (ittNotifiers = m_notifications.m_notifiers.begin(); ittNotifiers != m_notifications.m_notifiers.end(); ++ittNotifiers)
			{
				root["notifiers"][ii]["name"] = ittNotifiers->first;
				root["notifiers"][ii]["description"] = ittNotifiers->first;
				ii++;
			}

			unsigned long long idx = 0;
			if (m_pWebEm->FindValue("idx") != "")
			{
				std::stringstream s_str(m_pWebEm->FindValue("idx"));
				s_str >> idx;
			}
			std::vector<_tNotification> notifications = m_sql.GetNotifications(idx);
			if (notifications.size() > 0)
			{
				std::vector<_tNotification>::const_iterator itt;
				ii = 0;
				for (itt = notifications.begin(); itt != notifications.end(); ++itt)
				{
					root["result"][ii]["idx"] = itt->ID;
					std::string sParams = itt->Params;
					if (sParams == "") {
						sParams = "S";
					}
					root["result"][ii]["Params"] = sParams;
					root["result"][ii]["Priority"] = itt->Priority;
					root["result"][ii]["CustomMessage"] = itt->CustomMessage;
					root["result"][ii]["ActiveSystems"] = itt->ActiveSystems;
					ii++;
				}
			}
		}

		void CWebServer::RType_Schedules(Json::Value &root)
		{
			root["status"] = "OK";
			root["title"] = "Schedules";

			std::vector<tScheduleItem> schedules = m_mainworker.m_scheduler.GetScheduleItems();
			int ii = 0;
			std::vector<tScheduleItem>::iterator itt;
			for (itt = schedules.begin(); itt != schedules.end(); ++itt)
			{
				root["result"][ii]["Type"] = (itt->bIsScene) ? "Scene" : "Device";
				root["result"][ii]["RowID"] = itt->RowID;
				root["result"][ii]["DevName"] = itt->DeviceName;
				root["result"][ii]["TimerType"] = Timer_Type_Desc(itt->timerType);
				root["result"][ii]["TimerCmd"] = Timer_Cmd_Desc(itt->timerCmd);
				root["result"][ii]["IsThermostat"] = itt->bIsThermostat;
				if (itt->bIsThermostat == true)
				{
					char szTemp[10];
					sprintf(szTemp, "%.1f", itt->Temperature);
					root["result"][ii]["Temperature"] = szTemp;
				}
				root["result"][ii]["Days"] = itt->Days;

				struct tm timeinfo;
				localtime_r(&itt->startTime, &timeinfo);

				char *pDate = asctime(&timeinfo);
				if (pDate != NULL)
				{
					pDate[strlen(pDate) - 1] = 0;
					root["result"][ii]["ScheduleDate"] = pDate;
					ii++;
				}
			}
		}

		void CWebServer::RType_GetSharedUserDevices(Json::Value &root)
		{
			std::string idx = m_pWebEm->FindValue("idx");
			if (idx == "")
				return;
			root["status"] = "OK";
			root["title"] = "GetSharedUserDevices";

			std::stringstream szQuery;
			std::vector<std::vector<std::string> > result;
			szQuery << "SELECT DeviceRowID FROM SharedDevices WHERE (SharedUserID == " << idx << ")";
			result = m_sql.query(szQuery.str());
			if (result.size() > 0)
			{
				std::vector<std::vector<std::string> >::const_iterator itt;
				int ii = 0;
				for (itt = result.begin(); itt != result.end(); ++itt)
				{
					std::vector<std::string> sd = *itt;
					root["result"][ii]["DeviceRowIdx"] = sd[0];
					ii++;
				}
			}
		}

		void CWebServer::RType_SetSharedUserDevices(Json::Value &root)
		{
			std::string idx = m_pWebEm->FindValue("idx");
			std::string userdevices = m_pWebEm->FindValue("devices");
			if (idx == "")
				return;
			root["status"] = "OK";
			root["title"] = "SetSharedUserDevices";
			std::vector<std::string> strarray;
			StringSplit(userdevices, ";", strarray);

			//First delete all devices for this user, then add the (new) onces
			std::stringstream szQuery;
			std::vector<std::vector<std::string> > result;
			szQuery << "DELETE FROM SharedDevices WHERE (SharedUserID == " << idx << ")";
			result = m_sql.query(szQuery.str());

			int nDevices = static_cast<int>(strarray.size());
			for (int ii = 0; ii < nDevices; ii++)
			{
				szQuery.clear();
				szQuery.str("");
				szQuery << "INSERT INTO SharedDevices (SharedUserID,DeviceRowID) VALUES ('" << idx << "','" << strarray[ii] << "')";
				result = m_sql.query(szQuery.str());
			}
			m_mainworker.LoadSharedUsers();
		}

		void CWebServer::RType_SetUsed(Json::Value &root)
		{
			if (m_pWebEm->m_actualuser_rights != 2)
			{
				//No admin user, and not allowed to be here
				return;
			}

			std::string idx = m_pWebEm->FindValue("idx");
			std::string deviceid = m_pWebEm->FindValue("deviceid");
			std::string name = m_pWebEm->FindValue("name");
			std::string sused = m_pWebEm->FindValue("used");
			std::string sswitchtype = m_pWebEm->FindValue("switchtype");
			std::string maindeviceidx = m_pWebEm->FindValue("maindeviceidx");
			std::string addjvalue = m_pWebEm->FindValue("addjvalue");
			std::string addjmulti = m_pWebEm->FindValue("addjmulti");
			std::string addjvalue2 = m_pWebEm->FindValue("addjvalue2");
			std::string addjmulti2 = m_pWebEm->FindValue("addjmulti2");
			std::string setPoint = m_pWebEm->FindValue("setpoint");
			std::string state = m_pWebEm->FindValue("state");
			std::string mode = m_pWebEm->FindValue("mode");
			std::string until = m_pWebEm->FindValue("until");
			std::string clock = m_pWebEm->FindValue("clock");
			std::string tmode = m_pWebEm->FindValue("tmode");
			std::string fmode = m_pWebEm->FindValue("fmode");
			std::string sCustomImage = m_pWebEm->FindValue("customimage");

			std::string strunit = m_pWebEm->FindValue("unit");
			std::string strParam1 = base64_decode(m_pWebEm->FindValue("strparam1"));
			std::string strParam2 = base64_decode(m_pWebEm->FindValue("strparam2"));
			std::string tmpstr = m_pWebEm->FindValue("protected");
			bool bHasstrParam1 = m_pWebEm->HasValue("strparam1");
			int iProtected = (tmpstr == "true") ? 1 : 0;

			char szTmp[200];

			bool bHaveUser = (m_pWebEm->m_actualuser != "");
			int iUser = -1;
			if (bHaveUser)
			{
				iUser = FindUser(m_pWebEm->m_actualuser.c_str());
			}

			int switchtype = -1;
			if (sswitchtype != "")
				switchtype = atoi(sswitchtype.c_str());

			if ((idx == "") || (sused == ""))
				return;
			int used = (sused == "true") ? 1 : 0;
			if (maindeviceidx != "")
				used = 0;

			int CustomImage = 0;
			if (sCustomImage != "")
				CustomImage = atoi(sCustomImage.c_str());

			//Strip trailing spaces in 'name'
			name = stdstring_trim(name);

			std::stringstream sstridx(idx);
			unsigned long long ullidx;
			sstridx >> ullidx;
			m_mainworker.m_eventsystem.WWWUpdateSingleState(ullidx, name);

			std::stringstream szQuery;
			std::vector<std::vector<std::string> > result;
	
			szQuery << "SELECT Type,SubType FROM DeviceStatus WHERE (ID == " << idx << ")";
			result=m_sql.query(szQuery.str());
			if (result.size()<1)
				return;
			szQuery.clear();
			szQuery.str("");
			std::vector<std::string> sd = result[0];
				
			unsigned char dType=atoi(sd[0].c_str());
			unsigned char dSubType=atoi(sd[1].c_str());
			
			int nEvoMode=0;
			
			if (setPoint != "" || state!="")
			{
				double tempcelcius = atof(setPoint.c_str());
				if (m_sql.m_tempunit == TEMPUNIT_F)
				{
					//Convert back to celcius
					tempcelcius = (tempcelcius - 32) / 1.8;
				}
				sprintf(szTmp, "%.2f", tempcelcius);
				if(dType == pTypeEvohomeZone || dType == pTypeEvohomeWater)//sql update now done in setsetpoint for evohome devices
				{
					if(mode=="PermanentOverride")
						nEvoMode=1;
					else if(mode=="TemporaryOverride")
						nEvoMode=2;
				}
				else
				{
					szQuery << "UPDATE DeviceStatus SET Used=" << used << ", sValue='" << szTmp << "' WHERE (ID == " << idx << ")";
					m_sql.query(szQuery.str());
					szQuery.clear();
					szQuery.str("");
				}
			}
			if (name == "")
			{
				szQuery << "UPDATE DeviceStatus SET Used=" << used << " WHERE (ID == " << idx << ")";
			}
			else
			{
				if (switchtype == -1)
					szQuery << "UPDATE DeviceStatus SET Used=" << used << ", Name='" << name << "' WHERE (ID == " << idx << ")";
				else
					szQuery << "UPDATE DeviceStatus SET Used=" << used << ", Name='" << name << "', SwitchType=" << switchtype << ", CustomImage=" << CustomImage << " WHERE (ID == " << idx << ")";
			}
			result = m_sql.query(szQuery.str());

			if (bHasstrParam1)
			{
				szQuery.clear();
				szQuery.str("");
				szQuery << "UPDATE DeviceStatus SET StrParam1='" << strParam1 << "', StrParam2='" << strParam2 << "' WHERE (ID == " << idx << ")";
				result = m_sql.query(szQuery.str());
			}

			szQuery.clear();
			szQuery.str("");
			szQuery << "UPDATE DeviceStatus SET Protected=" << iProtected << " WHERE (ID == " << idx << ")";
			result = m_sql.query(szQuery.str());

			if (setPoint != "" || state!="")
			{
				int urights = 3;
				if (bHaveUser)
				{
					int iUser = -1;
					iUser = FindUser(m_pWebEm->m_actualuser.c_str());
					if (iUser != -1)
					{
						urights = static_cast<int>(m_users[iUser].userrights);
						_log.Log(LOG_STATUS, "User: %s initiated a SetPoint command", m_users[iUser].Username.c_str());
					}
				}
				if (urights < 1)
					return;
				if(dType == pTypeEvohomeWater)
					m_mainworker.SetSetPoint(idx, (state=="On")?1.0f:0.0f, nEvoMode, until);//FIXME float not guaranteed precise?
				else if(dType == pTypeEvohomeZone)
					m_mainworker.SetSetPoint(idx, static_cast<float>(atof(setPoint.c_str())), nEvoMode, until);
				else
					m_mainworker.SetSetPoint(idx, static_cast<float>(atof(setPoint.c_str())));
			}
			else if (clock != "")
			{
				int urights = 3;
				if (bHaveUser)
				{
					int iUser = -1;
					iUser = FindUser(m_pWebEm->m_actualuser.c_str());
					if (iUser != -1)
					{
						urights = static_cast<int>(m_users[iUser].userrights);
						_log.Log(LOG_STATUS, "User: %s initiated a SetClock command", m_users[iUser].Username.c_str());
					}
				}
				if (urights < 1)
					return;
				m_mainworker.SetClock(idx, clock);
			}
			else if (tmode != "")
			{
				int urights = 3;
				if (bHaveUser)
				{
					int iUser = -1;
					iUser = FindUser(m_pWebEm->m_actualuser.c_str());
					if (iUser != -1)
					{
						urights = static_cast<int>(m_users[iUser].userrights);
						_log.Log(LOG_STATUS, "User: %s initiated a Thermostat Mode command", m_users[iUser].Username.c_str());
					}
				}
				if (urights < 1)
					return;
				m_mainworker.SetZWaveThermostatMode(idx, atoi(tmode.c_str()));
			}
			else if (fmode != "")
			{
				int urights = 3;
				if (bHaveUser)
				{
					int iUser = -1;
					iUser = FindUser(m_pWebEm->m_actualuser.c_str());
					if (iUser != -1)
					{
						urights = static_cast<int>(m_users[iUser].userrights);
						_log.Log(LOG_STATUS, "User: %s initiated a Thermostat Fan Mode command", m_users[iUser].Username.c_str());
					}
				}
				if (urights < 1)
					return;
				m_mainworker.SetZWaveThermostatFanMode(idx, atoi(fmode.c_str()));
			}

			if (strunit != "")
			{
				szQuery.clear();
				szQuery.str("");
				szQuery << "UPDATE DeviceStatus SET Unit=" << strunit << " WHERE (ID == " << idx << ")";
				result = m_sql.query(szQuery.str());
			}
			//FIXME evohome ...we need the zone id to update the correct zone...but this should be ok as a generic call?
			if (deviceid != "")
			{
				szQuery.clear();
				szQuery.str("");
				szQuery << "UPDATE DeviceStatus SET DeviceID='" << deviceid << "' WHERE (ID == " << idx << ")";
				result = m_sql.query(szQuery.str());
			}
			if (addjvalue != "")
			{
				double faddjvalue = atof(addjvalue.c_str());
				szQuery.clear();
				szQuery.str("");
				szQuery << "UPDATE DeviceStatus SET AddjValue=" << faddjvalue << " WHERE (ID == " << idx << ")";
				result = m_sql.query(szQuery.str());
			}
			if (addjmulti != "")
			{
				double faddjmulti = atof(addjmulti.c_str());
				if (faddjmulti == 0)
					faddjmulti = 1;
				szQuery.clear();
				szQuery.str("");
				szQuery << "UPDATE DeviceStatus SET AddjMulti=" << faddjmulti << " WHERE (ID == " << idx << ")";
				result = m_sql.query(szQuery.str());
			}
			if (addjvalue2 != "")
			{
				double faddjvalue2 = atof(addjvalue2.c_str());
				szQuery.clear();
				szQuery.str("");
				szQuery << "UPDATE DeviceStatus SET AddjValue2=" << faddjvalue2 << " WHERE (ID == " << idx << ")";
				result = m_sql.query(szQuery.str());
			}
			if (addjmulti2 != "")
			{
				double faddjmulti2 = atof(addjmulti2.c_str());
				if (faddjmulti2 == 0)
					faddjmulti2 = 1;
				szQuery.clear();
				szQuery.str("");
				szQuery << "UPDATE DeviceStatus SET AddjMulti2=" << faddjmulti2 << " WHERE (ID == " << idx << ")";
				result = m_sql.query(szQuery.str());
			}

			if (used == 0)
			{
				bool bRemoveSubDevices = (m_pWebEm->FindValue("RemoveSubDevices") == "true");

				if (bRemoveSubDevices)
				{
					//if this device was a slave device, remove it
					sprintf(szTmp, "DELETE FROM LightSubDevices WHERE (DeviceRowID == %s)", idx.c_str());
					m_sql.query(szTmp);
				}
				sprintf(szTmp, "DELETE FROM LightSubDevices WHERE (ParentID == %s)", idx.c_str());
				m_sql.query(szTmp);

				sprintf(szTmp, "DELETE FROM Timers WHERE (DeviceRowID == %s)", idx.c_str());
				m_sql.query(szTmp);
			}

			if (maindeviceidx != "")
			{
				if (maindeviceidx != idx)
				{
					//this is a sub device for another light/switch
					//first check if it is not already a sub device
					szQuery.clear();
					szQuery.str("");
					szQuery << "SELECT ID FROM LightSubDevices WHERE (DeviceRowID=='" << idx << "') AND (ParentID =='" << maindeviceidx << "')";
					result = m_sql.query(szQuery.str());
					if (result.size() == 0)
					{
						//no it is not, add it
						sprintf(szTmp,
							"INSERT INTO LightSubDevices (DeviceRowID, ParentID) VALUES ('%s','%s')",
							idx.c_str(),
							maindeviceidx.c_str()
							);
						result = m_sql.query(szTmp);
					}
				}
			}
			if ((used == 0) && (maindeviceidx == ""))
			{
				//really remove it, including log etc
				m_sql.DeleteDevice(idx);
			}
			if (result.size() > 0)
			{
				root["status"] = "OK";
				root["title"] = "SetUsed";
			}
		}

		struct _tSortedEventsInt
		{
			std::string ID;
			std::string eventstatus;
		};

		void CWebServer::RType_Events(Json::Value &root)
		{
			//root["status"]="OK";
			root["title"] = "Events";

			std::string cparam = m_pWebEm->FindValue("param");
			if (cparam == "")
			{
				cparam = m_pWebEm->FindValue("dparam");
				if (cparam == "")
				{
					return;
				}
			}

			std::stringstream szQuery;
			std::vector<std::vector<std::string> > result;

			if (cparam == "list")
			{
				root["title"] = "ListEvents";
				root["status"] = "OK";

				std::map<std::string, _tSortedEventsInt> _levents;
				szQuery << "SELECT ID, Name, XMLStatement, Status FROM EventMaster ORDER BY ID ASC";
				result = m_sql.query(szQuery.str());
				if (result.size() > 0)
				{
					std::vector<std::vector<std::string> >::const_iterator itt;
					for (itt = result.begin(); itt != result.end(); ++itt)
					{
						std::vector<std::string> sd = *itt;
						std::string ID = sd[0];
						std::string Name = sd[1];
						std::string eventStatus = sd[3];
						_tSortedEventsInt eitem;
						eitem.ID = ID;
						eitem.eventstatus = eventStatus;
						if (_levents.find(Name) != _levents.end())
						{
							//Duplicate event name, add the ID
							szQuery.clear();
							szQuery.str("");
							szQuery << Name << " (" << ID << ")";
							Name = szQuery.str();
						}
						_levents[Name] = eitem;

					}
					//return a sorted event list
					std::map<std::string, _tSortedEventsInt>::const_iterator itt2;
					int ii = 0;
					for (itt2 = _levents.begin(); itt2 != _levents.end(); ++itt2)
					{
						root["result"][ii]["name"] = itt2->first;
						root["result"][ii]["id"] = itt2->second.ID;
						root["result"][ii]["eventstatus"] = itt2->second.eventstatus;
						ii++;
					}
				}
			}
			else if (cparam == "load")
			{
				root["title"] = "LoadEvent";

				std::string idx = m_pWebEm->FindValue("event");
				if (idx == "")
					return;

				int ii = 0;

				szQuery.clear();
				szQuery.str("");
				szQuery << "SELECT ID, Name, XMLStatement, Status FROM EventMaster WHERE (ID==" << idx << ")";
				result = m_sql.query(szQuery.str());
				if (result.size() > 0)
				{
					std::vector<std::vector<std::string> >::const_iterator itt;
					for (itt = result.begin(); itt != result.end(); ++itt)
					{
						std::vector<std::string> sd = *itt;
						std::string ID = sd[0];
						std::string Name = sd[1];
						std::string XMLStatement = sd[2];
						std::string eventStatus = sd[3];
						//int Status=atoi(sd[3].c_str());

						root["result"][ii]["id"] = ID;
						root["result"][ii]["name"] = Name;
						root["result"][ii]["xmlstatement"] = XMLStatement;
						root["result"][ii]["eventstatus"] = eventStatus;
						ii++;
					}
					root["status"] = "OK";
				}
			}

			else if (cparam == "create")
			{

				root["title"] = "AddEvent";

				std::string eventname = m_pWebEm->FindValue("name");
				if (eventname == "")
					return;

				std::string eventxml = m_pWebEm->FindValue("xml");
				if (eventxml == "")
					return;

				std::string eventactive = m_pWebEm->FindValue("eventstatus");
				if (eventactive == "")
					return;

				std::string eventid = m_pWebEm->FindValue("eventid");


				std::string eventlogic = m_pWebEm->FindValue("logicarray");
				if (eventlogic == "")
					return;

				int eventStatus = atoi(eventactive.c_str());

				Json::Value jsonRoot;
				Json::Reader reader;
				std::stringstream ssel(eventlogic);

				bool parsingSuccessful = reader.parse(ssel, jsonRoot);

				if (!parsingSuccessful)
				{

					_log.Log(LOG_ERROR, "Webserver event parser: Invalid data received!");

				}
				else {

					szQuery.clear();
					szQuery.str("");

					if (eventid == "") {
						szQuery << "INSERT INTO EventMaster (Name, XMLStatement, Status) VALUES ('" << eventname << "','" << eventxml << "','" << eventStatus << "')";
						m_sql.query(szQuery.str());
						szQuery.clear();
						szQuery.str("");
						szQuery << "SELECT ID FROM EventMaster WHERE (Name == '" << eventname << "')";
						result = m_sql.query(szQuery.str());
						if (result.size() > 0)
						{
							std::vector<std::string> sd = result[0];
							eventid = sd[0];
						}
					}
					else {
						szQuery << "UPDATE EventMaster SET Name='" << eventname << "', XMLStatement ='" << eventxml << "', Status ='" << eventStatus << "' WHERE (ID == '" << eventid << "')";
						m_sql.query(szQuery.str());
						szQuery.clear();
						szQuery.str("");
						szQuery << "DELETE FROM EventRules WHERE (EMID == '" << eventid << "')";
						m_sql.query(szQuery.str());
					}

					if (eventid == "")
					{
						//eventid should now never be empty!
						_log.Log(LOG_ERROR, "Error writing event actions to database!");
					}
					else {
						const Json::Value array = jsonRoot["eventlogic"];
						for (int index = 0; index < (int)array.size(); ++index)
						{
							std::string conditions = array[index].get("conditions", "").asString();
							std::string actions = array[index].get("actions", "").asString();

							if ((actions.find("SendNotification") != std::string::npos) || (actions.find("SendEmail") != std::string::npos))
							{
								actions = stdreplace(actions, "$", "#");
							}
							int sequenceNo = index + 1;
							szQuery.clear();
							szQuery.str("");
							szQuery << "INSERT INTO EventRules (EMID, Conditions, Actions, SequenceNo) VALUES ('" << eventid << "','" << conditions << "','" << actions << "','" << sequenceNo << "')";
							m_sql.query(szQuery.str());
						}

						m_mainworker.m_eventsystem.LoadEvents();
						root["status"] = "OK";
					}
				}
			}
			else if (cparam == "delete")
			{
				root["title"] = "DeleteEvent";
				std::string idx = m_pWebEm->FindValue("event");
				if (idx == "")
					return;
				m_sql.DeleteEvent(idx);
				m_mainworker.m_eventsystem.LoadEvents();
				root["status"] = "OK";
			}
			else if (cparam == "currentstates")
			{
				std::vector<CEventSystem::_tDeviceStatus> devStates;
				m_mainworker.m_eventsystem.WWWGetItemStates(devStates);
				if (devStates.size() == 0)
					return;

				int ii = 0;
				std::vector<CEventSystem::_tDeviceStatus>::iterator itt;
				for (itt = devStates.begin(); itt != devStates.end(); ++itt)
				{
					root["title"] = "Current States";
					root["result"][ii]["id"] = itt->ID;
					root["result"][ii]["name"] = itt->deviceName;
					root["result"][ii]["value"] = itt->nValueWording;
					root["result"][ii]["svalues"] = itt->sValue;
					root["result"][ii]["lastupdate"] = itt->lastUpdate;
					ii++;
				}
				root["status"] = "OK";
			}
		}

		void CWebServer::RType_Settings(Json::Value &root)
		{
			std::stringstream szQuery;
			std::vector<std::vector<std::string> > result;
			char szTmp[100];

			szQuery.clear();
			szQuery.str("");
			szQuery << "SELECT Key, nValue, sValue FROM Preferences";
			result = m_sql.query(szQuery.str());
			if (result.size() < 0)
				return;
			root["status"] = "OK";
			root["title"] = "settings";

			std::vector<std::vector<std::string> >::const_iterator itt;
			for (itt = result.begin(); itt != result.end(); ++itt)
			{
				std::vector<std::string> sd = *itt;
				std::string Key = sd[0];
				int nValue = atoi(sd[1].c_str());
				std::string sValue = sd[2];

				if (Key == "Location")
				{
					std::vector<std::string> strarray;
					StringSplit(sValue, ";", strarray);

					if (strarray.size() == 2)
					{
						root["Location"]["Latitude"] = strarray[0];
						root["Location"]["Longitude"] = strarray[1];
					}
				}
				/* RK: notification settings */
				if (m_notifications.IsInConfig(Key)) {
					if (sValue == "" && nValue > 0) {
						root[Key] = nValue;
					}
					else {
						root[Key] = sValue;
					}
				}
				else if (Key == "DashboardType")
				{
					root["DashboardType"] = nValue;
				}
				else if (Key == "MobileType")
				{
					root["MobileType"] = nValue;
				}
				else if (Key == "LightHistoryDays")
				{
					root["LightHistoryDays"] = nValue;
				}
				else if (Key == "5MinuteHistoryDays")
				{
					root["ShortLogDays"] = nValue;
				}
				else if (Key == "WebUserName")
				{
					root["WebUserName"] = base64_decode(sValue);
				}
				else if (Key == "WebPassword")
				{
					root["WebPassword"] = sValue;
				}
				else if (Key == "SecPassword")
				{
					root["SecPassword"] = sValue;
				}
				else if (Key == "ProtectionPassword")
				{
					root["ProtectionPassword"] = sValue;
				}
				else if (Key == "WebLocalNetworks")
				{
					root["WebLocalNetworks"] = sValue;
				}
				else if (Key == "RandomTimerFrame")
				{
					root["RandomTimerFrame"] = nValue;
				}
				else if (Key == "MeterDividerEnergy")
				{
					root["EnergyDivider"] = nValue;
				}
				else if (Key == "MeterDividerGas")
				{
					root["GasDivider"] = nValue;
				}
				else if (Key == "MeterDividerWater")
				{
					root["WaterDivider"] = nValue;
				}
				else if (Key == "ElectricVoltage")
				{
					root["ElectricVoltage"] = nValue;
				}
				else if (Key == "CM113DisplayType")
				{
					root["CM113DisplayType"] = nValue;
				}
				else if (Key == "UseAutoUpdate")
				{
					root["UseAutoUpdate"] = nValue;
				}
				else if (Key == "UseAutoBackup")
				{
					root["UseAutoBackup"] = nValue;
				}
				else if (Key == "Rego6XXType")
				{
					root["Rego6XXType"] = nValue;
				}
				else if (Key == "CostEnergy")
				{
					sprintf(szTmp, "%.4f", (float)(nValue) / 10000.0f);
					root["CostEnergy"] = szTmp;
				}
				else if (Key == "CostEnergyT2")
				{
					sprintf(szTmp, "%.4f", (float)(nValue) / 10000.0f);
					root["CostEnergyT2"] = szTmp;
				}
				else if (Key == "CostGas")
				{
					sprintf(szTmp, "%.4f", (float)(nValue) / 10000.0f);
					root["CostGas"] = szTmp;
				}
				else if (Key == "CostWater")
				{
					sprintf(szTmp, "%.4f", (float)(nValue) / 10000.0f);
					root["CostWater"] = szTmp;
				}
				else if (Key == "ActiveTimerPlan")
				{
					root["ActiveTimerPlan"] = nValue;
				}
				else if (Key == "DoorbellCommand")
				{
					root["DoorbellCommand"] = nValue;
				}
				else if (Key == "SmartMeterType")
				{
					root["SmartMeterType"] = nValue;
				}
				else if (Key == "EnableTabFloorplans")
				{
					root["EnableTabFloorplans"] = nValue;
				}
				else if (Key == "EnableTabLights")
				{
					root["EnableTabLights"] = nValue;
				}
				else if (Key == "EnableTabTemp")
				{
					root["EnableTabTemp"] = nValue;
				}
				else if (Key == "EnableTabWeather")
				{
					root["EnableTabWeather"] = nValue;
				}
				else if (Key == "EnableTabUtility")
				{
					root["EnableTabUtility"] = nValue;
				}
				else if (Key == "EnableTabScenes")
				{
					root["EnableTabScenes"] = nValue;
				}
				else if (Key == "NotificationSensorInterval")
				{
					root["NotificationSensorInterval"] = nValue;
				}
				else if (Key == "NotificationSwitchInterval")
				{
					root["NotificationSwitchInterval"] = nValue;
				}
				else if (Key == "RemoteSharedPort")
				{
					root["RemoteSharedPort"] = nValue;
				}
				else if (Key == "Language")
				{
					root["Language"] = sValue;
				}
				else if (Key == "WindUnit")
				{
					root["WindUnit"] = nValue;
				}
				else if (Key == "TempUnit")
				{
					root["TempUnit"] = nValue;
				}
				else if (Key == "AuthenticationMethod")
				{
					root["AuthenticationMethod"] = nValue;
				}
				else if (Key == "ReleaseChannel")
				{
					root["ReleaseChannel"] = nValue;
				}
				else if (Key == "RaspCamParams")
				{
					root["RaspCamParams"] = sValue;
				}
				else if (Key == "UVCParams")
				{
					root["UVCParams"] = sValue;
				}
				else if (Key == "AcceptNewHardware")
				{
					root["AcceptNewHardware"] = nValue;
				}
				else if (Key == "HideDisabledHardwareSensors")
				{
					root["HideDisabledHardwareSensors"] = nValue;
				}
				else if (Key == "DisableEventScriptSystem")
				{
					root["DisableEventScriptSystem"] = nValue;
				}
				else if (Key == "SecOnDelay")
				{
					root["SecOnDelay"] = nValue;
				}
				else if (Key == "AllowWidgetOrdering")
				{
					root["AllowWidgetOrdering"] = nValue;
				}
				else if (Key == "FloorplanPopupDelay")
				{
					root["FloorplanPopupDelay"] = nValue;
				}
				else if (Key == "FloorplanFullscreenMode")
				{
					root["FloorplanFullscreenMode"] = nValue;
				}
				else if (Key == "FloorplanAnimateZoom")
				{
					root["FloorplanAnimateZoom"] = nValue;
				}
				else if (Key == "FloorplanShowSensorValues")
				{
					root["FloorplanShowSensorValues"] = nValue;
				}
				else if (Key == "FloorplanShowSwitchValues")
				{
					root["FloorplanShowSwitchValues"] = nValue;
				}
				else if (Key == "FloorplanShowSceneNames")
				{
					root["FloorplanShowSceneNames"] = nValue;
				}
				else if (Key == "FloorplanRoomColour")
				{
					root["FloorplanRoomColour"] = sValue;
				}
				else if (Key == "FloorplanActiveOpacity")
				{
					root["FloorplanActiveOpacity"] = nValue;
				}
				else if (Key == "FloorplanInactiveOpacity")
				{
					root["FloorplanInactiveOpacity"] = nValue;
				}
				else if (Key == "SensorTimeout")
				{
					root["SensorTimeout"] = nValue;
				}
				else if (Key == "BatteryLowNotification")
				{
					root["BatterLowLevel"] = nValue;
				}
				else if (Key == "WebTheme")
				{
					root["WebTheme"] = sValue;
				}
			}
		}

		void CWebServer::RType_LightLog(Json::Value &root)
		{
			unsigned long long idx = 0;
			if (m_pWebEm->FindValue("idx") != "")
			{
				std::stringstream s_str(m_pWebEm->FindValue("idx"));
				s_str >> idx;
			}
			std::vector<std::vector<std::string> > result;
			std::stringstream szQuery;
			//First get Device Type/SubType
			szQuery.clear();
			szQuery.str("");
			szQuery << "SELECT Type, SubType, SwitchType FROM DeviceStatus WHERE (ID == " << idx << ")";
			result = m_sql.query(szQuery.str());
			if (result.size() < 1)
				return;

			unsigned char dType = atoi(result[0][0].c_str());
			unsigned char dSubType = atoi(result[0][1].c_str());
			_eSwitchType switchtype = (_eSwitchType)atoi(result[0][2].c_str());

			if (
				(dType != pTypeLighting1) &&
				(dType != pTypeLighting2) &&
				(dType != pTypeLighting3) &&
				(dType != pTypeLighting4) &&
				(dType != pTypeLighting5) &&
				(dType != pTypeLighting6) &&
				(dType != pTypeLimitlessLights) &&
				(dType != pTypeSecurity1) &&
				(dType != pTypeSecurity2) &&
				(dType != pTypeEvohome) &&
				(dType != pTypeEvohomeRelay) &&
				(dType != pTypeCurtain) &&
				(dType != pTypeBlinds) &&
				(dType != pTypeRFY) &&
				(dType != pTypeRego6XXValue) &&
				(dType != pTypeChime) &&
				(dType != pTypeThermostat2) &&
				(dType != pTypeThermostat3) &&
				(dType != pTypeRemote)&&
				(dType != pTypeGeneralSwitch) &&
				(!((dType == pTypeRadiator1) && (dSubType == sTypeSmartwaresSwitchRadiator)))
				)
				return; //no light device! we should not be here!

			root["status"] = "OK";
			root["title"] = "LightLog";

			szQuery.clear();
			szQuery.str("");
			szQuery << "SELECT ROWID, nValue, sValue, Date FROM LightingLog WHERE (DeviceRowID==" << idx << ") ORDER BY Date DESC";
			result = m_sql.query(szQuery.str());
			if (result.size() > 0)
			{
				std::vector<std::vector<std::string> >::const_iterator itt;
				int ii = 0;
				for (itt = result.begin(); itt != result.end(); ++itt)
				{
					std::vector<std::string> sd = *itt;

					int nValue = atoi(sd[1].c_str());
					std::string sValue = sd[2];

					root["result"][ii]["idx"] = sd[0];

					//add light details
					std::string lstatus = "";
					int llevel = 0;
					bool bHaveDimmer = false;
					bool bHaveGroupCmd = false;
					int maxDimLevel = 0;

					GetLightStatus(dType, dSubType, switchtype, nValue, sValue, lstatus, llevel, bHaveDimmer, maxDimLevel, bHaveGroupCmd);

					if (ii == 0)
					{
						root["HaveDimmer"] = bHaveDimmer;
						root["result"][ii]["MaxDimLevel"] = maxDimLevel;
						root["HaveGroupCmd"] = bHaveGroupCmd;
					}

					root["result"][ii]["Date"] = sd[3];
					root["result"][ii]["Status"] = lstatus;
					root["result"][ii]["Level"] = llevel;
					root["result"][ii]["Data"] = lstatus;

					ii++;
				}
			}
		}

		void CWebServer::RType_TextLog(Json::Value &root)
		{
			unsigned long long idx = 0;
			if (m_pWebEm->FindValue("idx") != "")
			{
				std::stringstream s_str(m_pWebEm->FindValue("idx"));
				s_str >> idx;
			}
			std::vector<std::vector<std::string> > result;
			std::stringstream szQuery;

			root["status"] = "OK";
			root["title"] = "TextLog";

			szQuery.clear();
			szQuery.str("");
			szQuery << "SELECT ROWID, sValue, Date FROM LightingLog WHERE (DeviceRowID==" << idx << ") ORDER BY Date DESC";
			result = m_sql.query(szQuery.str());
			if (result.size() > 0)
			{
				std::vector<std::vector<std::string> >::const_iterator itt;
				int ii = 0;
				for (itt = result.begin(); itt != result.end(); ++itt)
				{
					std::vector<std::string> sd = *itt;

					std::string sValue = sd[1];

					root["result"][ii]["idx"] = sd[0];
					root["result"][ii]["Date"] = sd[2];
					root["result"][ii]["Data"] = sd[1];
					ii++;
				}
			}
		}

		void CWebServer::RType_HandleGraph(Json::Value &root)
		{
			unsigned long long idx = 0;
			if (m_pWebEm->FindValue("idx") != "")
			{
				std::stringstream s_str(m_pWebEm->FindValue("idx"));
				s_str >> idx;
			}

			std::vector<std::vector<std::string> > result;
			std::stringstream szQuery;
			char szTmp[300];

			std::string sensor = m_pWebEm->FindValue("sensor");
			if (sensor == "")
				return;
			std::string srange = m_pWebEm->FindValue("range");
			if (srange == "")
				return;

			time_t now = mytime(NULL);
			struct tm tm1;
			localtime_r(&now, &tm1);

			szQuery.clear();
			szQuery.str("");
			szQuery << "SELECT Type, SubType, SwitchType, AddjValue, AddjMulti FROM DeviceStatus WHERE (ID == " << idx << ")";
			result = m_sql.query(szQuery.str());
			if (result.size() < 1)
				return;

			unsigned char dType = atoi(result[0][0].c_str());
			unsigned char dSubType = atoi(result[0][1].c_str());
			_eMeterType metertype = (_eMeterType)atoi(result[0][2].c_str());
			if ((dType == pTypeP1Power) || (dType == pTypeENERGY) || (dType == pTypePOWER))
				metertype = MTYPE_ENERGY;
			else if (dType == pTypeP1Gas)
				metertype = MTYPE_GAS;
			else if ((dType == pTypeRego6XXValue) && (dSubType == sTypeRego6XXCounter))
				metertype = MTYPE_COUNTER;

			double AddjValue = atof(result[0][3].c_str());
			double AddjMulti = atof(result[0][4].c_str());

			std::string dbasetable = "";
			if (srange == "day") {
				if (sensor == "temp")
					dbasetable = "Temperature";
				else if (sensor == "rain")
					dbasetable = "Rain";
				else if (sensor == "Percentage")
					dbasetable = "Percentage";
				else if (sensor == "fan")
					dbasetable = "Fan";
				else if (sensor == "counter")
				{
					if ((dType == pTypeP1Power) || (dType == pTypeCURRENT) || (dType == pTypeCURRENTENERGY))
					{
						dbasetable = "MultiMeter";
					}
					else
					{
						dbasetable = "Meter";
					}
				}
				else if ((sensor == "wind") || (sensor == "winddir"))
					dbasetable = "Wind";
				else if (sensor == "uv")
					dbasetable = "UV";
				else
					return;
			}
			else
			{
				//week,year,month
				if (sensor == "temp")
					dbasetable = "Temperature_Calendar";
				else if (sensor == "rain")
					dbasetable = "Rain_Calendar";
				else if (sensor == "Percentage")
					dbasetable = "Percentage_Calendar";
				else if (sensor == "fan")
					dbasetable = "Fan_Calendar";
				else if (sensor == "counter")
				{
					if (
						(dType == pTypeP1Power) ||
						(dType == pTypeCURRENT) ||
						(dType == pTypeCURRENTENERGY) ||
						(dType == pTypeAirQuality) ||
						((dType == pTypeGeneral) && (dSubType == sTypeVisibility)) ||
						((dType == pTypeGeneral) && (dSubType == sTypeDistance)) ||
						((dType == pTypeGeneral) && (dSubType == sTypeSolarRadiation)) ||
						((dType == pTypeGeneral) && (dSubType == sTypeSoilMoisture)) ||
						((dType == pTypeGeneral) && (dSubType == sTypeLeafWetness)) ||
						((dType == pTypeRFXSensor) && (dSubType == sTypeRFXSensorAD)) ||
						((dType == pTypeRFXSensor) && (dSubType == sTypeRFXSensorVolt)) ||
						((dType == pTypeGeneral) && (dSubType == sTypeVoltage)) ||
						((dType == pTypeGeneral) && (dSubType == sTypeCurrent)) ||
						((dType == pTypeGeneral) && (dSubType == sTypePressure)) ||
						((dType == pTypeGeneral) && (dSubType == sTypeSoundLevel)) ||
						(dType == pTypeLux) ||
						(dType == pTypeWEIGHT) ||
						(dType == pTypeUsage)
						)
						dbasetable = "MultiMeter_Calendar";
					else
						dbasetable = "Meter_Calendar";
				}
				else if ((sensor == "wind") || (sensor == "winddir"))
					dbasetable = "Wind_Calendar";
				else if (sensor == "uv")
					dbasetable = "UV_Calendar";
				else
					return;
			}
			unsigned char tempsign = m_sql.m_tempsign[0];

			if (srange == "day")
			{
				if (sensor == "temp") {
					root["status"] = "OK";
					root["title"] = "Graph " + sensor + " " + srange;

					szQuery.clear();
					szQuery.str("");
					szQuery << "SELECT Temperature, Chill, Humidity, Barometer, Date, SetPoint FROM " << dbasetable << " WHERE (DeviceRowID==" << idx << ") ORDER BY Date ASC";
					result = m_sql.query(szQuery.str());
					if (result.size() > 0)
					{
						std::vector<std::vector<std::string> >::const_iterator itt;
						int ii = 0;
						for (itt = result.begin(); itt != result.end(); ++itt)
						{
							std::vector<std::string> sd = *itt;

							root["result"][ii]["d"] = sd[4].substr(0, 16);
							if (
								(dType == pTypeRego6XXTemp) ||
								(dType == pTypeTEMP) ||
								(dType == pTypeTEMP_HUM) ||
								(dType == pTypeTEMP_HUM_BARO) ||
								(dType == pTypeTEMP_BARO) ||
								((dType == pTypeWIND) && (dSubType == sTypeWIND4)) ||
								((dType == pTypeWIND) && (dSubType == sTypeWINDNoTemp)) ||
								((dType == pTypeUV) && (dSubType == sTypeUV3)) ||
								(dType == pTypeThermostat1) ||
								((dType == pTypeRFXSensor) && (dSubType == sTypeRFXSensorTemp)) ||
								((dType == pTypeGeneral) && (dSubType == sTypeSystemTemp)) ||
								((dType == pTypeGeneral) && (dSubType == sTypeBaro)) ||
								((dType == pTypeThermostat) && (dSubType == sTypeThermSetpoint)) ||
								(dType == pTypeEvohomeZone) ||
								(dType == pTypeEvohomeWater)
								)
							{
								double tvalue = ConvertTemperature(atof(sd[0].c_str()), tempsign);
								root["result"][ii]["te"] = tvalue;
							}
							if (
								((dType == pTypeWIND) && (dSubType == sTypeWIND4)) ||
								((dType == pTypeWIND) && (dSubType == sTypeWINDNoTemp))
								)
							{
								double tvalue = ConvertTemperature(atof(sd[1].c_str()), tempsign);
								root["result"][ii]["ch"] = tvalue;
							}
							if ((dType == pTypeHUM) || (dType == pTypeTEMP_HUM) || (dType == pTypeTEMP_HUM_BARO))
							{
								root["result"][ii]["hu"] = sd[2];
							}
							if (
								(dType == pTypeTEMP_HUM_BARO) ||
								(dType == pTypeTEMP_BARO)||
								((dType == pTypeGeneral) && (dSubType == sTypeBaro))
								)
							{
								if (dType == pTypeTEMP_HUM_BARO)
								{
									if (dSubType == sTypeTHBFloat)
									{
										sprintf(szTmp, "%.1f", atof(sd[3].c_str()) / 10.0f);
										root["result"][ii]["ba"] = szTmp;
									}
									else
										root["result"][ii]["ba"] = sd[3];
								}
								else if (dType == pTypeTEMP_BARO)
								{
									sprintf(szTmp, "%.1f", atof(sd[3].c_str()) / 10.0f);
									root["result"][ii]["ba"] = szTmp;
								}
								else if ((dType == pTypeGeneral) && (dSubType == sTypeBaro))
								{
									sprintf(szTmp, "%.1f", atof(sd[3].c_str()) / 10.0f);
									root["result"][ii]["ba"] = szTmp;
								}
							}
							if((dType == pTypeEvohomeZone) || (dType == pTypeEvohomeWater))
							{
								double se = ConvertTemperature(atof(sd[5].c_str()), tempsign);
								root["result"][ii]["se"] = se;
							}

							ii++;
						}
					}
				}
				else if (sensor == "Percentage") {
					root["status"] = "OK";
					root["title"] = "Graph " + sensor + " " + srange;

					szQuery.clear();
					szQuery.str("");
					szQuery << "SELECT Percentage, Date FROM " << dbasetable << " WHERE (DeviceRowID==" << idx << ") ORDER BY Date ASC";
					result = m_sql.query(szQuery.str());
					if (result.size() > 0)
					{
						std::vector<std::vector<std::string> >::const_iterator itt;
						int ii = 0;
						for (itt = result.begin(); itt != result.end(); ++itt)
						{
							std::vector<std::string> sd = *itt;

							root["result"][ii]["d"] = sd[1].substr(0, 16);
							root["result"][ii]["v"] = sd[0];
							ii++;
						}
					}
				}
				else if (sensor == "fan") {
					root["status"] = "OK";
					root["title"] = "Graph " + sensor + " " + srange;

					szQuery.clear();
					szQuery.str("");
					szQuery << "SELECT Speed, Date FROM " << dbasetable << " WHERE (DeviceRowID==" << idx << ") ORDER BY Date ASC";
					result = m_sql.query(szQuery.str());
					if (result.size() > 0)
					{
						std::vector<std::vector<std::string> >::const_iterator itt;
						int ii = 0;
						for (itt = result.begin(); itt != result.end(); ++itt)
						{
							std::vector<std::string> sd = *itt;

							root["result"][ii]["d"] = sd[1].substr(0, 16);
							root["result"][ii]["v"] = sd[0];
							ii++;
						}
					}
				}

				else if (sensor == "counter")
				{
					if (dType == pTypeP1Power)
					{
						root["status"] = "OK";
						root["title"] = "Graph " + sensor + " " + srange;

						szQuery.clear();
						szQuery.str("");
						szQuery << "SELECT Value1, Value2, Value3, Value4, Value5, Value6, Date FROM " << dbasetable << " WHERE (DeviceRowID==" << idx << ") ORDER BY Date ASC";
						result = m_sql.query(szQuery.str());
						if (result.size() > 0)
						{
							std::vector<std::vector<std::string> >::const_iterator itt;
							int ii = 0;
							bool bHaveDeliverd = false;
							bool bHaveFirstValue = false;
							long long lastUsage1, lastUsage2, lastDeliv1, lastDeliv2;
							time_t lastTime = 0;

							long long firstUsage1, firstUsage2, firstDeliv1, firstDeliv2;

							int nMeterType = 0;
							m_sql.GetPreferencesVar("SmartMeterType", nMeterType);

							int lastDay = 0;

							for (itt = result.begin(); itt != result.end(); ++itt)
							{
								std::vector<std::string> sd = *itt;

								if (nMeterType == 0)
								{
									long long actUsage1, actUsage2, actDeliv1, actDeliv2;
									std::stringstream s_str1(sd[0]);
									s_str1 >> actUsage1;
									std::stringstream s_str2(sd[4]);
									s_str2 >> actUsage2;
									std::stringstream s_str3(sd[1]);
									s_str3 >> actDeliv1;
									std::stringstream s_str4(sd[5]);
									s_str4 >> actDeliv2;

									std::string stime = sd[6];
									struct tm ntime;
									time_t atime;
									ntime.tm_isdst = -1;
									ntime.tm_year = atoi(stime.substr(0, 4).c_str()) - 1900;
									ntime.tm_mon = atoi(stime.substr(5, 2).c_str()) - 1;
									ntime.tm_mday = atoi(stime.substr(8, 2).c_str());
									ntime.tm_hour = atoi(stime.substr(11, 2).c_str());
									ntime.tm_min = atoi(stime.substr(14, 2).c_str());
									ntime.tm_sec = atoi(stime.substr(17, 2).c_str());
									atime = mktime(&ntime);

									if (lastDay != ntime.tm_mday)
									{
										lastDay = ntime.tm_mday;
										firstUsage1 = actUsage1;
										firstUsage2 = actUsage2;
										firstDeliv1 = actDeliv1;
										firstDeliv2 = actDeliv2;
									}

									if (bHaveFirstValue)
									{
										long curUsage1 = (long)(actUsage1 - lastUsage1);
										long curUsage2 = (long)(actUsage2 - lastUsage2);
										long curDeliv1 = (long)(actDeliv1 - lastDeliv1);
										long curDeliv2 = (long)(actDeliv2 - lastDeliv2);

										if ((curUsage1 < 0) || (curUsage1>100000))
											curUsage1 = 0;
										if ((curUsage2 < 0) || (curUsage2>100000))
											curUsage2 = 0;
										if ((curDeliv1 < 0) || (curDeliv1>100000))
											curDeliv1 = 0;
										if ((curDeliv2 < 0) || (curDeliv2>100000))
											curDeliv2 = 0;

										time_t tdiff = atime - lastTime;
										if (tdiff == 0)
											tdiff = 1;
										float tlaps = 3600.0f / tdiff;
										curUsage1 *= int(tlaps);
										curUsage2 *= int(tlaps);
										curDeliv1 *= int(tlaps);
										curDeliv2 *= int(tlaps);

										root["result"][ii]["d"] = sd[6].substr(0, 16);

										if ((curDeliv1 != 0) || (curDeliv2 != 0))
											bHaveDeliverd = true;

										sprintf(szTmp, "%ld", curUsage1);
										root["result"][ii]["v"] = szTmp;
										sprintf(szTmp, "%ld", curUsage2);
										root["result"][ii]["v2"] = szTmp;
										sprintf(szTmp, "%ld", curDeliv1);
										root["result"][ii]["r1"] = szTmp;
										sprintf(szTmp, "%ld", curDeliv2);
										root["result"][ii]["r2"] = szTmp;

										long pUsage1 = (long)(actUsage1 - firstUsage1);
										long pUsage2 = (long)(actUsage2 - firstUsage2);

										sprintf(szTmp, "%ld", pUsage1 + pUsage2);
										root["result"][ii]["eu"] = szTmp;
										if (bHaveDeliverd)
										{
											long pDeliv1 = (long)(actDeliv1 - firstDeliv1);
											long pDeliv2 = (long)(actDeliv2 - firstDeliv2);
											sprintf(szTmp, "%ld", pDeliv1 + pDeliv2);
											root["result"][ii]["eg"] = szTmp;
										}

										ii++;
									}
									else
									{
										bHaveFirstValue = true;
										if ((ntime.tm_hour != 0) && (ntime.tm_min != 0))
										{
											atime -= 24 * 60 * 60;
											struct tm ltime;
											localtime_r(&atime, &ltime);
											int year = ltime.tm_year+1900;
											int mon = ltime.tm_mon+1;
											int day = ltime.tm_mday;
											sprintf(szTmp, "%04d-%02d-%02d", year, mon, day);
											szQuery.clear();
											szQuery.str("");
											szQuery << "SELECT Counter1, Counter2, Counter3, Counter4 FROM Multimeter_Calendar WHERE (DeviceRowID==" << idx << ") AND (Date=='" << szTmp << "')";
											std::vector<std::vector<std::string> > result2;
											result2 = m_sql.query(szQuery.str());
											if (!result2.empty())
											{
												std::vector<std::string> sd = result2[0];
												std::stringstream s_str1(sd[0]);
												s_str1 >> firstUsage1;
												std::stringstream s_str2(sd[1]);
												s_str2 >> firstDeliv1;
												std::stringstream s_str3(sd[2]);
												s_str3 >> firstUsage2;
												std::stringstream s_str4(sd[3]);
												s_str4 >> firstDeliv2;
												lastDay = ntime.tm_mday;
											}
										}

									}
									lastUsage1 = actUsage1;
									lastUsage2 = actUsage2;
									lastDeliv1 = actDeliv1;
									lastDeliv2 = actDeliv2;
									lastTime = atime;
								}
								else
								{
									//this meter has no decimals, so return the use peaks
									root["result"][ii]["d"] = sd[6].substr(0, 16);

									if (sd[3] != "0")
										bHaveDeliverd = true;
									root["result"][ii]["v"] = sd[2];
									root["result"][ii]["r1"] = sd[3];
									ii++;

								}
							}
							if (bHaveDeliverd)
							{
								root["delivered"] = true;
							}
						}
					}
					else if (dType == pTypeAirQuality)
					{//day
						root["status"] = "OK";
						root["title"] = "Graph " + sensor + " " + srange;

						szQuery.clear();
						szQuery.str("");
						szQuery << "SELECT Value, Date FROM " << dbasetable << " WHERE (DeviceRowID==" << idx << ") ORDER BY Date ASC";
						result = m_sql.query(szQuery.str());
						if (result.size() > 0)
						{
							std::vector<std::vector<std::string> >::const_iterator itt;
							int ii = 0;
							for (itt = result.begin(); itt != result.end(); ++itt)
							{
								std::vector<std::string> sd = *itt;

								root["result"][ii]["d"] = sd[1].substr(0, 16);
								root["result"][ii]["co2"] = sd[0];
								ii++;
							}
						}
					}
					else if ((dType == pTypeGeneral) && ((dSubType == sTypeSoilMoisture) || (dSubType == sTypeLeafWetness)))
					{//day
						root["status"] = "OK";
						root["title"] = "Graph " + sensor + " " + srange;

						szQuery.clear();
						szQuery.str("");
						szQuery << "SELECT Value, Date FROM " << dbasetable << " WHERE (DeviceRowID==" << idx << ") ORDER BY Date ASC";
						result = m_sql.query(szQuery.str());
						if (result.size() > 0)
						{
							std::vector<std::vector<std::string> >::const_iterator itt;
							int ii = 0;
							for (itt = result.begin(); itt != result.end(); ++itt)
							{
								std::vector<std::string> sd = *itt;

								root["result"][ii]["d"] = sd[1].substr(0, 16);
								root["result"][ii]["v"] = sd[0];
								ii++;
							}
						}
					}
					else if (
						((dType == pTypeGeneral) && (dSubType == sTypeVisibility)) ||
						((dType == pTypeGeneral) && (dSubType == sTypeDistance)) ||
						((dType == pTypeGeneral) && (dSubType == sTypeSolarRadiation)) ||
						((dType == pTypeGeneral) && (dSubType == sTypeVoltage)) ||
						((dType == pTypeGeneral) && (dSubType == sTypeCurrent)) ||
						((dType == pTypeGeneral) && (dSubType == sTypePressure)) ||
						((dType == pTypeGeneral) && (dSubType == sTypeSoundLevel))
						)
					{//day
						root["status"] = "OK";
						root["title"] = "Graph " + sensor + " " + srange;
						float vdiv = 10.0f;
						if (
							((dType == pTypeGeneral) && (dSubType == sTypeVoltage)) ||
							((dType == pTypeGeneral) && (dSubType == sTypeCurrent))
							)
						{
							vdiv = 1000.0f;
						}
						szQuery.clear();
						szQuery.str("");
						szQuery << "SELECT Value, Date FROM " << dbasetable << " WHERE (DeviceRowID==" << idx << ") ORDER BY Date ASC";
						result = m_sql.query(szQuery.str());
						if (result.size() > 0)
						{
							std::vector<std::vector<std::string> >::const_iterator itt;
							int ii = 0;
							for (itt = result.begin(); itt != result.end(); ++itt)
							{
								std::vector<std::string> sd = *itt;

								root["result"][ii]["d"] = sd[1].substr(0, 16);
								float fValue = float(atof(sd[0].c_str())) / vdiv;
								if (metertype == 1)
									fValue *= 0.6214f;
								if ((dType == pTypeGeneral) && (dSubType == sTypeVoltage))
									sprintf(szTmp, "%.3f", fValue);
								else if ((dType == pTypeGeneral) && (dSubType == sTypeCurrent))
									sprintf(szTmp, "%.3f", fValue);
								else
									sprintf(szTmp, "%.1f", fValue);
								root["result"][ii]["v"] = szTmp;
								ii++;
							}
						}
					}
					else if ((dType == pTypeRFXSensor) && ((dSubType == sTypeRFXSensorAD) || (dSubType == sTypeRFXSensorVolt)))
					{//day
						root["status"] = "OK";
						root["title"] = "Graph " + sensor + " " + srange;

						szQuery.clear();
						szQuery.str("");
						szQuery << "SELECT Value, Date FROM " << dbasetable << " WHERE (DeviceRowID==" << idx << ") ORDER BY Date ASC";
						result = m_sql.query(szQuery.str());
						if (result.size() > 0)
						{
							std::vector<std::vector<std::string> >::const_iterator itt;
							int ii = 0;
							for (itt = result.begin(); itt != result.end(); ++itt)
							{
								std::vector<std::string> sd = *itt;

								root["result"][ii]["d"] = sd[1].substr(0, 16);
								root["result"][ii]["v"] = sd[0];
								ii++;
							}
						}
					}
					else if (dType == pTypeLux)
					{//day
						root["status"] = "OK";
						root["title"] = "Graph " + sensor + " " + srange;

						szQuery.clear();
						szQuery.str("");
						szQuery << "SELECT Value, Date FROM " << dbasetable << " WHERE (DeviceRowID==" << idx << ") ORDER BY Date ASC";
						result = m_sql.query(szQuery.str());
						if (result.size() > 0)
						{
							std::vector<std::vector<std::string> >::const_iterator itt;
							int ii = 0;
							for (itt = result.begin(); itt != result.end(); ++itt)
							{
								std::vector<std::string> sd = *itt;

								root["result"][ii]["d"] = sd[1].substr(0, 16);
								root["result"][ii]["lux"] = sd[0];
								ii++;
							}
						}
					}
					else if (dType == pTypeWEIGHT)
					{//day
						root["status"] = "OK";
						root["title"] = "Graph " + sensor + " " + srange;

						szQuery.clear();
						szQuery.str("");
						szQuery << "SELECT Value, Date FROM " << dbasetable << " WHERE (DeviceRowID==" << idx << ") ORDER BY Date ASC";
						result = m_sql.query(szQuery.str());
						if (result.size() > 0)
						{
							std::vector<std::vector<std::string> >::const_iterator itt;
							int ii = 0;
							for (itt = result.begin(); itt != result.end(); ++itt)
							{
								std::vector<std::string> sd = *itt;

								root["result"][ii]["d"] = sd[1].substr(0, 16);
								sprintf(szTmp, "%.1f", atof(sd[0].c_str()) / 10.0f);
								root["result"][ii]["v"] = szTmp;
								ii++;
							}
						}
					}
					else if (dType == pTypeUsage)
					{//day
						root["status"] = "OK";
						root["title"] = "Graph " + sensor + " " + srange;

						szQuery.clear();
						szQuery.str("");
						szQuery << "SELECT Value, Date FROM " << dbasetable << " WHERE (DeviceRowID==" << idx << ") ORDER BY Date ASC";
						result = m_sql.query(szQuery.str());
						if (result.size() > 0)
						{
							std::vector<std::vector<std::string> >::const_iterator itt;
							int ii = 0;
							for (itt = result.begin(); itt != result.end(); ++itt)
							{
								std::vector<std::string> sd = *itt;

								root["result"][ii]["d"] = sd[1].substr(0, 16);
								root["result"][ii]["u"] = atof(sd[0].c_str()) / 10.0f;
								ii++;
							}
						}
					}
					else if (dType == pTypeCURRENT)
					{
						root["status"] = "OK";
						root["title"] = "Graph " + sensor + " " + srange;

						//CM113
						int displaytype = 0;
						int voltage = 230;
						m_sql.GetPreferencesVar("CM113DisplayType", displaytype);
						m_sql.GetPreferencesVar("ElectricVoltage", voltage);

						root["displaytype"] = displaytype;

						szQuery.clear();
						szQuery.str("");
						szQuery << "SELECT Value1, Value2, Value3, Date FROM " << dbasetable << " WHERE (DeviceRowID==" << idx << ") ORDER BY Date ASC";
						result = m_sql.query(szQuery.str());
						if (result.size() > 0)
						{
							std::vector<std::vector<std::string> >::const_iterator itt;
							int ii = 0;
							bool bHaveL1 = false;
							bool bHaveL2 = false;
							bool bHaveL3 = false;
							for (itt = result.begin(); itt != result.end(); ++itt)
							{
								std::vector<std::string> sd = *itt;

								root["result"][ii]["d"] = sd[3].substr(0, 16);

								float fval1 = static_cast<float>(atof(sd[0].c_str()) / 10.0f);
								float fval2 = static_cast<float>(atof(sd[1].c_str()) / 10.0f);
								float fval3 = static_cast<float>(atof(sd[2].c_str()) / 10.0f);

								if (fval1 != 0)
									bHaveL1 = true;
								if (fval2 != 0)
									bHaveL2 = true;
								if (fval3 != 0)
									bHaveL3 = true;

								if (displaytype == 0)
								{
									sprintf(szTmp, "%.1f", fval1);
									root["result"][ii]["v1"] = szTmp;
									sprintf(szTmp, "%.1f", fval2);
									root["result"][ii]["v2"] = szTmp;
									sprintf(szTmp, "%.1f", fval3);
									root["result"][ii]["v3"] = szTmp;
								}
								else
								{
									sprintf(szTmp, "%d", int(fval1*voltage));
									root["result"][ii]["v1"] = szTmp;
									sprintf(szTmp, "%d", int(fval2*voltage));
									root["result"][ii]["v2"] = szTmp;
									sprintf(szTmp, "%d", int(fval3*voltage));
									root["result"][ii]["v3"] = szTmp;
								}
								ii++;
							}
							if (
								(!bHaveL1) &&
								(!bHaveL2) &&
								(!bHaveL3)
								) {
								root["haveL1"] = true; //show at least something
							}
							else {
								if (bHaveL1)
									root["haveL1"] = true;
								if (bHaveL2)
									root["haveL2"] = true;
								if (bHaveL3)
									root["haveL3"] = true;
							}
						}
					}
					else if (dType == pTypeCURRENTENERGY)
					{
						root["status"] = "OK";
						root["title"] = "Graph " + sensor + " " + srange;

						//CM113
						int displaytype = 0;
						int voltage = 230;
						m_sql.GetPreferencesVar("CM113DisplayType", displaytype);
						m_sql.GetPreferencesVar("ElectricVoltage", voltage);

						root["displaytype"] = displaytype;

						szQuery.clear();
						szQuery.str("");
						szQuery << "SELECT Value1, Value2, Value3, Date FROM " << dbasetable << " WHERE (DeviceRowID==" << idx << ") ORDER BY Date ASC";
						result = m_sql.query(szQuery.str());
						if (result.size() > 0)
						{
							std::vector<std::vector<std::string> >::const_iterator itt;
							int ii = 0;
							bool bHaveL1 = false;
							bool bHaveL2 = false;
							bool bHaveL3 = false;
							for (itt = result.begin(); itt != result.end(); ++itt)
							{
								std::vector<std::string> sd = *itt;

								root["result"][ii]["d"] = sd[3].substr(0, 16);

								float fval1 = static_cast<float>(atof(sd[0].c_str()) / 10.0f);
								float fval2 = static_cast<float>(atof(sd[1].c_str()) / 10.0f);
								float fval3 = static_cast<float>(atof(sd[2].c_str()) / 10.0f);

								if (fval1 != 0)
									bHaveL1 = true;
								if (fval2 != 0)
									bHaveL2 = true;
								if (fval3 != 0)
									bHaveL3 = true;

								if (displaytype == 0)
								{
									sprintf(szTmp, "%.1f", fval1);
									root["result"][ii]["v1"] = szTmp;
									sprintf(szTmp, "%.1f", fval2);
									root["result"][ii]["v2"] = szTmp;
									sprintf(szTmp, "%.1f", fval3);
									root["result"][ii]["v3"] = szTmp;
								}
								else
								{
									sprintf(szTmp, "%d", int(fval1*voltage));
									root["result"][ii]["v1"] = szTmp;
									sprintf(szTmp, "%d", int(fval2*voltage));
									root["result"][ii]["v2"] = szTmp;
									sprintf(szTmp, "%d", int(fval3*voltage));
									root["result"][ii]["v3"] = szTmp;
								}
								ii++;
							}
							if (
								(!bHaveL1) &&
								(!bHaveL2) &&
								(!bHaveL3)
								) {
								root["haveL1"] = true; //show at least something
							}
							else {
								if (bHaveL1)
									root["haveL1"] = true;
								if (bHaveL2)
									root["haveL2"] = true;
								if (bHaveL3)
									root["haveL3"] = true;
							}
						}
					}
					else if ((dType == pTypeENERGY) || (dType == pTypePOWER))
					{
						root["status"] = "OK";
						root["title"] = "Graph " + sensor + " " + srange;

						float EnergyDivider = 1000.0f;
						float GasDivider = 100.0f;
						float WaterDivider = 100.0f;
						int tValue;
						if (m_sql.GetPreferencesVar("MeterDividerEnergy", tValue))
						{
							EnergyDivider = float(tValue);
						}
						if (m_sql.GetPreferencesVar("MeterDividerGas", tValue))
						{
							GasDivider = float(tValue);
						}
						if (m_sql.GetPreferencesVar("MeterDividerWater", tValue))
						{
							WaterDivider = float(tValue);
						}
						if (dType == pTypeP1Gas)
							GasDivider = 1000;
						else if ((dType == pTypeENERGY) || (dType == pTypePOWER))
							EnergyDivider *= 100.0f;

						//First check if we had any usage in the short log, if not, its probably a meter without usage
						szQuery.clear();
						szQuery.str("");
						bool bHaveUsage = true;
						szQuery << "SELECT MIN([Usage]), MAX([Usage]) FROM " << dbasetable << " WHERE (DeviceRowID==" << idx << ")";
						result = m_sql.query(szQuery.str());
						if (result.size() > 0)
						{
							std::stringstream s_str1(result[0][0]);
							unsigned long long minValue;
							s_str1 >> minValue;
							std::stringstream s_str2(result[0][1]);
							unsigned long long maxValue;
							s_str2 >> maxValue;
							if ((minValue == 0) && (maxValue == 0))
							{
								bHaveUsage = false;
							}
						}

						szQuery.clear();
						szQuery.str("");
						int ii = 0;
						szQuery << "SELECT Value,[Usage], Date FROM " << dbasetable << " WHERE (DeviceRowID==" << idx << ") ORDER BY Date ASC";
						result = m_sql.query(szQuery.str());

						int method = 0;
						std::string sMethod = m_pWebEm->FindValue("method");
						if (sMethod.size() > 0)
							method = atoi(sMethod.c_str());
						if (bHaveUsage == false)
							method = 0;
						if (method != 0)
						{
							//realtime graph
							if ((dType == pTypeENERGY) || (dType == pTypePOWER))
								EnergyDivider /= 100.0f;
						}
						root["method"] = method;
						bool bHaveFirstValue = false;
						bool bHaveFirstRealValue = false;
						float FirstValue = 0;
						unsigned long long ulFirstRealValue = 0;
						unsigned long long ulFirstValue = 0;
						unsigned long long ulLastValue = 0;
						std::string LastDateTime = "";
						time_t lastTime = 0;

						if (result.size() > 0)
						{
							std::vector<std::vector<std::string> >::const_iterator itt;
							for (itt = result.begin(); itt != result.end(); ++itt)
							{
								std::vector<std::string> sd = *itt;

								if (method == 0)
								{
									//bars / hour
									std::string actDateTimeHour = sd[2].substr(0, 13);
									if (actDateTimeHour != LastDateTime)
									{
										if (bHaveFirstValue)
										{
											root["result"][ii]["d"] = LastDateTime + ":00";

											unsigned long long ulTotalValue = ulLastValue - ulFirstValue;
											if (ulTotalValue == 0)
											{
												//Could be the P1 Gas Meter, only transmits one every 1 a 2 hours
												ulTotalValue = ulLastValue - ulFirstRealValue;
											}
											ulFirstRealValue = ulLastValue;
											float TotalValue = float(ulTotalValue);
											if (TotalValue != 0)
											{
												switch (metertype)
												{
												case MTYPE_ENERGY:
													sprintf(szTmp, "%.3f", (TotalValue / EnergyDivider)*1000.0f);	//from kWh -> Watt
													break;
												case MTYPE_GAS:
													sprintf(szTmp, "%.2f", TotalValue / GasDivider);
													break;
												case MTYPE_WATER:
													sprintf(szTmp, "%.2f", TotalValue / WaterDivider);
													break;
												case MTYPE_COUNTER:
													sprintf(szTmp, "%.1f", TotalValue);
													break;
												}
												root["result"][ii]["v"] = szTmp;
												ii++;
											}
										}
										LastDateTime = actDateTimeHour;
										bHaveFirstValue = false;
									}
									std::stringstream s_str1(sd[0]);
									unsigned long long actValue;
									s_str1 >> actValue;

									if (actValue >= ulLastValue)
										ulLastValue = actValue;

									if (!bHaveFirstValue)
									{
										ulFirstValue = ulLastValue;
										bHaveFirstValue = true;
									}
									if (!bHaveFirstRealValue)
									{
										bHaveFirstRealValue = true;
										ulFirstRealValue = ulLastValue;
									}
								}
								else
								{
									std::stringstream s_str1(sd[1]);
									unsigned long long actValue;
									s_str1 >> actValue;

									root["result"][ii]["d"] = sd[2].substr(0, 16);

									float TotalValue = float(actValue);
									switch (metertype)
									{
									case MTYPE_ENERGY:
										sprintf(szTmp, "%.3f", (TotalValue / EnergyDivider)*1000.0f);	//from kWh -> Watt
										break;
									case MTYPE_GAS:
										sprintf(szTmp, "%.2f", TotalValue / GasDivider);
										break;
									case MTYPE_WATER:
										sprintf(szTmp, "%.2f", TotalValue / WaterDivider);
										break;
									case MTYPE_COUNTER:
										sprintf(szTmp, "%.1f", TotalValue);
										break;
									}
									root["result"][ii]["v"] = szTmp;
									ii++;
								}
							}
						}
					}
					else
					{
						root["status"] = "OK";
						root["title"] = "Graph " + sensor + " " + srange;

						float EnergyDivider = 1000.0f;
						float GasDivider = 100.0f;
						float WaterDivider = 100.0f;
						int tValue;
						if (m_sql.GetPreferencesVar("MeterDividerEnergy", tValue))
						{
							EnergyDivider = float(tValue);
						}
						if (m_sql.GetPreferencesVar("MeterDividerGas", tValue))
						{
							GasDivider = float(tValue);
						}
						if (m_sql.GetPreferencesVar("MeterDividerWater", tValue))
						{
							WaterDivider = float(tValue);
						}
						if (dType == pTypeP1Gas)
							GasDivider = 1000.0f;
						else if ((dType == pTypeENERGY) || (dType == pTypePOWER))
							EnergyDivider *= 100.0f;

						szQuery.clear();
						szQuery.str("");
						int ii = 0;
						szQuery << "SELECT Value, Date FROM " << dbasetable << " WHERE (DeviceRowID==" << idx << ") ORDER BY Date ASC";
						result = m_sql.query(szQuery.str());

						int method = 0;
						std::string sMethod = m_pWebEm->FindValue("method");
						if (sMethod.size() > 0)
							method = atoi(sMethod.c_str());

						bool bHaveFirstValue = false;
						bool bHaveFirstRealValue = false;
						float FirstValue = 0;
						unsigned long long ulFirstRealValue = 0;
						unsigned long long ulFirstValue = 0;
						unsigned long long ulLastValue = 0;

						std::string LastDateTime = "";
						time_t lastTime = 0;

						if (result.size() > 0)
						{
							std::vector<std::vector<std::string> >::const_iterator itt;
							for (itt = result.begin(); itt != result.end(); ++itt)
							{
								std::vector<std::string> sd = *itt;

								if (method == 0)
								{
									//bars / hour

									std::stringstream s_str1(sd[0]);
									unsigned long long actValue;
									s_str1 >> actValue;

									std::string actDateTimeHour = sd[1].substr(0, 13);
									if (actDateTimeHour != LastDateTime)
									{
										if (bHaveFirstValue)
										{
											struct tm ntime;
											time_t atime;
											ntime.tm_isdst = -1;
											ntime.tm_year = atoi(actDateTimeHour.substr(0, 4).c_str()) - 1900;
											ntime.tm_mon = atoi(actDateTimeHour.substr(5, 2).c_str()) - 1;
											ntime.tm_mday = atoi(actDateTimeHour.substr(8, 2).c_str());
											ntime.tm_hour = atoi(actDateTimeHour.substr(11, 2).c_str());
											ntime.tm_min = 0;
											ntime.tm_sec = 0;
											atime = mktime(&ntime);
											atime -= 3600; //subtract one hour
											localtime_r(&atime, &ntime);
											char szTime[50];
											sprintf(szTime, "%04d-%02d-%02d %02d:00", ntime.tm_year + 1900, ntime.tm_mon + 1, ntime.tm_mday, ntime.tm_hour);
											root["result"][ii]["d"] = szTime;

											float TotalValue = float(actValue - ulFirstValue);

											if (TotalValue != 0)
											{
												switch (metertype)
												{
												case MTYPE_ENERGY:
													sprintf(szTmp, "%.3f", (TotalValue / EnergyDivider)*1000.0f);	//from kWh -> Watt
													break;
												case MTYPE_GAS:
													sprintf(szTmp, "%.3f", TotalValue / GasDivider);
													break;
												case MTYPE_WATER:
													sprintf(szTmp, "%.3f", TotalValue / WaterDivider);
													break;
												case MTYPE_COUNTER:
													sprintf(szTmp, "%.1f", TotalValue);
													break;
												}
												root["result"][ii]["v"] = szTmp;
												ii++;
											}
										}
										ulFirstValue = actValue;
										LastDateTime = actDateTimeHour;
									}

									if (!bHaveFirstValue)
									{
										ulFirstValue = actValue;
										bHaveFirstValue = true;
									}
									ulLastValue = actValue;
								}
								else
								{
									//realtime graph
									std::stringstream s_str1(sd[0]);
									unsigned long long actValue;
									s_str1 >> actValue;

									std::string stime = sd[1];
									struct tm ntime;
									time_t atime;
									ntime.tm_isdst = -1;
									ntime.tm_year = atoi(stime.substr(0, 4).c_str()) - 1900;
									ntime.tm_mon = atoi(stime.substr(5, 2).c_str()) - 1;
									ntime.tm_mday = atoi(stime.substr(8, 2).c_str());
									ntime.tm_hour = atoi(stime.substr(11, 2).c_str());
									ntime.tm_min = atoi(stime.substr(14, 2).c_str());
									ntime.tm_sec = atoi(stime.substr(17, 2).c_str());
									atime = mktime(&ntime);

									if (bHaveFirstRealValue)
									{
										long long curValue = actValue - ulLastValue;

										time_t tdiff = atime - lastTime;
										if (tdiff == 0)
											tdiff = 1;
										float tlaps = 3600.0f / tdiff;
										curValue *= int(tlaps);

										root["result"][ii]["d"] = sd[1].substr(0, 16);

										float TotalValue = float(curValue);
										if (TotalValue != 0)
										{
											switch (metertype)
											{
											case MTYPE_ENERGY:
												sprintf(szTmp, "%.3f", (TotalValue / EnergyDivider)*1000.0f);	//from kWh -> Watt
												break;
											case MTYPE_GAS:
												sprintf(szTmp, "%.2f", TotalValue / GasDivider);
												break;
											case MTYPE_WATER:
												sprintf(szTmp, "%.2f", TotalValue / WaterDivider);
												break;
											case MTYPE_COUNTER:
												sprintf(szTmp, "%.1f", TotalValue);
												break;
											}
											root["result"][ii]["v"] = szTmp;
											ii++;
										}

									}
									else
										bHaveFirstRealValue = true;
									ulLastValue = actValue;
									lastTime = atime;
								}
							}
						}
						if ((bHaveFirstValue) && (method == 0))
						{
							//add last value
							root["result"][ii]["d"] = LastDateTime + ":00";

							unsigned long long ulTotalValue = ulLastValue - ulFirstValue;

							float TotalValue = float(ulTotalValue);

							if (TotalValue != 0)
							{
								switch (metertype)
								{
								case MTYPE_ENERGY:
									sprintf(szTmp, "%.3f", (TotalValue / EnergyDivider)*1000.0f);	//from kWh -> Watt
									break;
								case MTYPE_GAS:
									sprintf(szTmp, "%.3f", TotalValue / GasDivider);
									break;
								case MTYPE_WATER:
									sprintf(szTmp, "%.3f", TotalValue / WaterDivider);
									break;
								case MTYPE_COUNTER:
									sprintf(szTmp, "%.1f", TotalValue);
									break;
								}
								root["result"][ii]["v"] = szTmp;
								ii++;
							}
						}
					}
				}
				else if (sensor == "uv") {
					root["status"] = "OK";
					root["title"] = "Graph " + sensor + " " + srange;

					szQuery.clear();
					szQuery.str("");
					szQuery << "SELECT Level, Date FROM " << dbasetable << " WHERE (DeviceRowID==" << idx << ") ORDER BY Date ASC";
					result = m_sql.query(szQuery.str());
					if (result.size() > 0)
					{
						std::vector<std::vector<std::string> >::const_iterator itt;
						int ii = 0;
						for (itt = result.begin(); itt != result.end(); ++itt)
						{
							std::vector<std::string> sd = *itt;

							root["result"][ii]["d"] = sd[1].substr(0, 16);
							root["result"][ii]["uvi"] = sd[0];
							ii++;
						}
					}
				}
				else if (sensor == "rain") {
					root["status"] = "OK";
					root["title"] = "Graph " + sensor + " " + srange;

					int LastHour = -1;
					float LastTotalPreviousHour = -1;

					float LastValue = -1;
					std::string LastDate = "";

					szQuery.clear();
					szQuery.str("");
					szQuery << "SELECT Total, Date FROM " << dbasetable << " WHERE (DeviceRowID==" << idx << ") ORDER BY Date ASC";
					result = m_sql.query(szQuery.str());
					if (result.size() > 0)
					{
						std::vector<std::vector<std::string> >::const_iterator itt;
						int ii = 0;
						for (itt = result.begin(); itt != result.end(); ++itt)
						{
							std::vector<std::string> sd = *itt;
							float ActTotal = static_cast<float>(atof(sd[0].c_str()));
							int Hour = atoi(sd[1].substr(11, 2).c_str());
							if (Hour != LastHour)
							{
								if (LastHour != -1)
								{
									int NextCalculatedHour = (LastHour + 1) % 24;
									if (Hour != NextCalculatedHour)
									{
										//Looks like we have a GAP somewhere, finish the last hour
										root["result"][ii]["d"] = LastDate;
										double mmval = ActTotal - LastValue;
										mmval *= AddjMulti;
										sprintf(szTmp, "%.1f", mmval);
										root["result"][ii]["mm"] = szTmp;
										ii++;
									}
									else
									{
										root["result"][ii]["d"] = sd[1].substr(0, 16);
										double mmval = ActTotal - LastTotalPreviousHour;
										mmval *= AddjMulti;
										sprintf(szTmp, "%.1f", mmval);
										root["result"][ii]["mm"] = szTmp;
										ii++;
									}
								}
								LastHour = Hour;
								LastTotalPreviousHour = ActTotal;
							}
							LastValue = ActTotal;
							LastDate = sd[1];
						}
					}
				}
				else if (sensor == "wind") {
					root["status"] = "OK";
					root["title"] = "Graph " + sensor + " " + srange;

					szQuery.clear();
					szQuery.str("");
					szQuery << "SELECT Direction, Speed, Gust, Date FROM " << dbasetable << " WHERE (DeviceRowID==" << idx << ") ORDER BY Date ASC";
					result = m_sql.query(szQuery.str());
					if (result.size() > 0)
					{
						std::vector<std::vector<std::string> >::const_iterator itt;
						int ii = 0;
						for (itt = result.begin(); itt != result.end(); ++itt)
						{
							std::vector<std::string> sd = *itt;

							root["result"][ii]["d"] = sd[3].substr(0, 16);
							root["result"][ii]["di"] = sd[0];

							int intSpeed = atoi(sd[1].c_str());
							sprintf(szTmp, "%.1f", float(intSpeed) * m_sql.m_windscale);
							root["result"][ii]["sp"] = szTmp;
							int intGust = atoi(sd[2].c_str());
							sprintf(szTmp, "%.1f", float(intGust) * m_sql.m_windscale);
							root["result"][ii]["gu"] = szTmp;
							ii++;
						}
					}
				}
				else if (sensor == "winddir") {
					root["status"] = "OK";
					root["title"] = "Graph " + sensor + " " + srange;

					szQuery.clear();
					szQuery.str("");
					szQuery << "SELECT Direction, Speed, Gust FROM " << dbasetable << " WHERE (DeviceRowID==" << idx << ") ORDER BY Date ASC";
					result = m_sql.query(szQuery.str());
					if (result.size() > 0)
					{
						std::vector<std::vector<std::string> >::const_iterator itt;
						std::map<int, int> _directions;
						int wdirtabletemp[17][8];
						std::string szLegendLabels[7];
						int ii = 0;

						int totalvalues = 0;
						//init dir list
						int idir;
						for (idir = 0; idir < 360 + 1; idir++)
							_directions[idir] = 0;
						for (ii = 0; ii < 17; ii++)
						{
							for (int jj = 0; jj < 8; jj++)
							{
								wdirtabletemp[ii][jj] = 0;
							}
						}

						if (m_sql.m_windunit == WINDUNIT_MS)
						{
							szLegendLabels[0] = "&lt; 0.5 " + m_sql.m_windsign;
							szLegendLabels[1] = "0.5-2 " + m_sql.m_windsign;
							szLegendLabels[2] = "2-4 " + m_sql.m_windsign;
							szLegendLabels[3] = "4-6 " + m_sql.m_windsign;
							szLegendLabels[4] = "6-8 " + m_sql.m_windsign;
							szLegendLabels[5] = "8-10 " + m_sql.m_windsign;
							szLegendLabels[6] = "&gt; 10" + m_sql.m_windsign;
						}
						else if (m_sql.m_windunit == WINDUNIT_KMH)
						{
							szLegendLabels[0] = "&lt; 2 " + m_sql.m_windsign;
							szLegendLabels[1] = "2-4 " + m_sql.m_windsign;
							szLegendLabels[2] = "4-6 " + m_sql.m_windsign;
							szLegendLabels[3] = "6-10 " + m_sql.m_windsign;
							szLegendLabels[4] = "10-20 " + m_sql.m_windsign;
							szLegendLabels[5] = "20-36 " + m_sql.m_windsign;
							szLegendLabels[6] = "&gt; 36" + m_sql.m_windsign;
						}
						else if (m_sql.m_windunit == WINDUNIT_MPH)
						{
							szLegendLabels[0] = "&lt; 3 " + m_sql.m_windsign;
							szLegendLabels[1] = "3-7 " + m_sql.m_windsign;
							szLegendLabels[2] = "7-12 " + m_sql.m_windsign;
							szLegendLabels[3] = "12-18 " + m_sql.m_windsign;
							szLegendLabels[4] = "18-24 " + m_sql.m_windsign;
							szLegendLabels[5] = "24-46 " + m_sql.m_windsign;
							szLegendLabels[6] = "&gt; 46" + m_sql.m_windsign;
						}
						else if (m_sql.m_windunit == WINDUNIT_Knots)
						{
							szLegendLabels[0] = "&lt; 3 " + m_sql.m_windsign;
							szLegendLabels[1] = "3-7 " + m_sql.m_windsign;
							szLegendLabels[2] = "7-17 " + m_sql.m_windsign;
							szLegendLabels[3] = "17-27 " + m_sql.m_windsign;
							szLegendLabels[4] = "27-34 " + m_sql.m_windsign;
							szLegendLabels[5] = "34-41 " + m_sql.m_windsign;
							szLegendLabels[6] = "&gt; 41" + m_sql.m_windsign;
						}
						else {
							//Todo !
							szLegendLabels[0] = "&lt; 0.5 " + m_sql.m_windsign;
							szLegendLabels[1] = "0.5-2 " + m_sql.m_windsign;
							szLegendLabels[2] = "2-4 " + m_sql.m_windsign;
							szLegendLabels[3] = "4-6 " + m_sql.m_windsign;
							szLegendLabels[4] = "6-8 " + m_sql.m_windsign;
							szLegendLabels[5] = "8-10 " + m_sql.m_windsign;
							szLegendLabels[6] = "&gt; 10" + m_sql.m_windsign;
						}
						

						for (itt = result.begin(); itt != result.end(); ++itt)
						{
							std::vector<std::string> sd = *itt;
							float fdirection = static_cast<float>(atof(sd[0].c_str()));
							if (fdirection == 360)
								fdirection = 0;
							int direction = int(fdirection);
							float speed = static_cast<float>(atof(sd[1].c_str())) * m_sql.m_windscale;
							float gust = static_cast<float>(atof(sd[2].c_str())) * m_sql.m_windscale;
							int bucket = int(fdirection / 22.5f);

							int speedpos = 0;

							if (m_sql.m_windunit == WINDUNIT_MS)
							{
								if (gust < 0.5f) speedpos = 0;
								else if (gust < 2.0f) speedpos = 1;
								else if (gust < 4.0f) speedpos = 2;
								else if (gust < 6.0f) speedpos = 3;
								else if (gust < 8.0f) speedpos = 4;
								else if (gust < 10.0f) speedpos = 5;
								else speedpos = 6;
							}
							else if (m_sql.m_windunit == WINDUNIT_KMH)
							{
								if (gust < 2.0f) speedpos = 0;
								else if (gust < 4.0f) speedpos = 1;
								else if (gust < 6.0f) speedpos = 2;
								else if (gust < 10.0f) speedpos = 3;
								else if (gust < 20.0f) speedpos = 4;
								else if (gust < 36.0f) speedpos = 5;
								else speedpos = 6;
							}
							else if (m_sql.m_windunit == WINDUNIT_MPH)
							{
								if (gust < 3.0f) speedpos = 0;
								else if (gust < 7.0f) speedpos = 1;
								else if (gust < 12.0f) speedpos = 2;
								else if (gust < 18.0f) speedpos = 3;
								else if (gust < 24.0f) speedpos = 4;
								else if (gust < 46.0f) speedpos = 5;
								else speedpos = 6;
							}
							else if (m_sql.m_windunit == WINDUNIT_Knots)
							{
								if (gust < 3.0f) speedpos = 0;
								else if (gust < 7.0f) speedpos = 1;
								else if (gust < 17.0f) speedpos = 2;
								else if (gust < 27.0f) speedpos = 3;
								else if (gust < 34.0f) speedpos = 4;
								else if (gust < 41.0f) speedpos = 5;
								else speedpos = 6;
							}
							else
							{
								//Still todo !
								if (gust < 0.5f) speedpos = 0;
								else if (gust < 2.0f) speedpos = 1;
								else if (gust < 4.0f) speedpos = 2;
								else if (gust < 6.0f) speedpos = 3;
								else if (gust < 8.0f) speedpos = 4;
								else if (gust < 10.0f) speedpos = 5;
								else speedpos = 6;
							}
							wdirtabletemp[bucket][speedpos]++;
							_directions[direction]++;
							totalvalues++;
						}

						for (int jj = 0; jj < 7; jj++)
						{
							root["result_speed"][jj]["label"] = szLegendLabels[jj];

							for (ii = 0; ii < 16; ii++)
							{
								float svalue = 0;
								if (totalvalues > 0)
								{
									svalue = (100.0f / totalvalues)*wdirtabletemp[ii][jj];
								}
								sprintf(szTmp, "%.2f", svalue);
								root["result_speed"][jj]["sp"][ii] = szTmp;
							}
						}
						ii = 0;
						for (idir = 0; idir < 360 + 1; idir++)
						{
							if (_directions[idir] != 0)
							{
								root["result"][ii]["dig"] = idir;
								float percentage = 0;
								if (totalvalues > 0)
								{
									percentage = (float(100.0 / float(totalvalues))*float(_directions[idir]));
								}
								sprintf(szTmp, "%.2f", percentage);
								root["result"][ii]["div"] = szTmp;
								ii++;
							}
						}
					}
				}

			}//day
			else if (srange == "week")
			{
				if (sensor == "rain") {
					root["status"] = "OK";
					root["title"] = "Graph " + sensor + " " + srange;

					char szDateStart[40];
					char szDateEnd[40];

					struct tm ltime;
					ltime.tm_isdst = tm1.tm_isdst;
					ltime.tm_hour = 0;
					ltime.tm_min = 0;
					ltime.tm_sec = 0;
					ltime.tm_year = tm1.tm_year;
					ltime.tm_mon = tm1.tm_mon;
					ltime.tm_mday = tm1.tm_mday;

					sprintf(szDateEnd, "%04d-%02d-%02d", ltime.tm_year + 1900, ltime.tm_mon + 1, ltime.tm_mday);

					//Subtract one week

					ltime.tm_mday -= 7;
					time_t later = mktime(&ltime);
					struct tm tm2;
					localtime_r(&later, &tm2);

					sprintf(szDateStart, "%04d-%02d-%02d", tm2.tm_year + 1900, tm2.tm_mon + 1, tm2.tm_mday);

					szQuery.clear();
					szQuery.str("");
					szQuery << "SELECT Total, Rate, Date FROM " << dbasetable << " WHERE (DeviceRowID==" << idx << " AND Date>='" << szDateStart << "' AND Date<='" << szDateEnd << "') ORDER BY Date ASC";
					result = m_sql.query(szQuery.str());
					int ii = 0;
					if (result.size() > 0)
					{
						std::vector<std::vector<std::string> >::const_iterator itt;
						for (itt = result.begin(); itt != result.end(); ++itt)
						{
							std::vector<std::string> sd = *itt;

							root["result"][ii]["d"] = sd[2].substr(0, 16);
							double mmval = atof(sd[0].c_str());
							mmval *= AddjMulti;
							sprintf(szTmp, "%.1f", mmval);
							root["result"][ii]["mm"] = szTmp;
							ii++;
						}
					}
					//add today (have to calculate it)
					szQuery.clear();
					szQuery.str("");
					if (dSubType != sTypeRAINWU)
					{
						szQuery << "SELECT MIN(Total), MAX(Total), MAX(Rate) FROM Rain WHERE (DeviceRowID=" << idx << " AND Date>='" << szDateEnd << "')";
					}
					else
					{
						szQuery << "SELECT Total, Total, Rate FROM Rain WHERE (DeviceRowID=" << idx << " AND Date>='" << szDateEnd << "') ORDER BY ROWID DESC LIMIT 1";
					}
					result = m_sql.query(szQuery.str());
					if (result.size() > 0)
					{
						std::vector<std::string> sd = result[0];

						float total_min = static_cast<float>(atof(sd[0].c_str()));
						float total_max = static_cast<float>(atof(sd[1].c_str()));
						int rate = atoi(sd[2].c_str());

						double total_real = 0;
						if (dSubType != sTypeRAINWU)
						{
							total_real = total_max - total_min;
						}
						else
						{
							total_real = total_max;
						}
						total_real *= AddjMulti;
						sprintf(szTmp, "%.1f", total_real);
						root["result"][ii]["d"] = szDateEnd;
						root["result"][ii]["mm"] = szTmp;
						ii++;
					}
				}
				else if (sensor == "counter")
				{
					root["status"] = "OK";
					root["title"] = "Graph " + sensor + " " + srange;

					float EnergyDivider = 1000.0f;
					float GasDivider = 100.0f;
					float WaterDivider = 100.0f;
					int tValue;
					if (m_sql.GetPreferencesVar("MeterDividerEnergy", tValue))
					{
						EnergyDivider = float(tValue);
					}
					if (m_sql.GetPreferencesVar("MeterDividerGas", tValue))
					{
						GasDivider = float(tValue);
					}
					if (m_sql.GetPreferencesVar("MeterDividerWater", tValue))
					{
						WaterDivider = float(tValue);
					}
					if (dType == pTypeP1Gas)
						GasDivider = 1000.0f;
					else if ((dType == pTypeENERGY) || (dType == pTypePOWER))
						EnergyDivider *= 100.0f;
					//else if (dType==pTypeRFXMeter)
					//EnergyDivider*=1000.0f;

					char szDateStart[40];
					char szDateEnd[40];

					struct tm ltime;
					ltime.tm_isdst = tm1.tm_isdst;
					ltime.tm_hour = 0;
					ltime.tm_min = 0;
					ltime.tm_sec = 0;
					ltime.tm_year = tm1.tm_year;
					ltime.tm_mon = tm1.tm_mon;
					ltime.tm_mday = tm1.tm_mday;

					sprintf(szDateEnd, "%04d-%02d-%02d", ltime.tm_year + 1900, ltime.tm_mon + 1, ltime.tm_mday);

					//Subtract one week

					ltime.tm_mday -= 7;
					time_t later = mktime(&ltime);
					struct tm tm2;
					localtime_r(&later, &tm2);
					sprintf(szDateStart, "%04d-%02d-%02d", tm2.tm_year + 1900, tm2.tm_mon + 1, tm2.tm_mday);

					szQuery.clear();
					szQuery.str("");
					int ii = 0;
					if (dType == pTypeP1Power)
					{
						szQuery << "SELECT Value1,Value2,Value5,Value6,Date FROM " << dbasetable << " WHERE (DeviceRowID==" << idx << " AND Date>='" << szDateStart << "' AND Date<='" << szDateEnd << "') ORDER BY Date ASC";
						result = m_sql.query(szQuery.str());
						if (result.size() > 0)
						{
							bool bHaveDeliverd = false;
							std::vector<std::vector<std::string> >::const_iterator itt;
							for (itt = result.begin(); itt != result.end(); ++itt)
							{
								std::vector<std::string> sd = *itt;
								root["result"][ii]["d"] = sd[4].substr(0, 16);
								std::string szValueUsage1 = sd[0];
								std::string szValueDeliv1 = sd[1];
								std::string szValueUsage2 = sd[2];
								std::string szValueDeliv2 = sd[3];

								float fUsage1 = (float)(atof(szValueUsage1.c_str()));
								float fUsage2 = (float)(atof(szValueUsage2.c_str()));
								float fDeliv1 = (float)(atof(szValueDeliv1.c_str()));
								float fDeliv2 = (float)(atof(szValueDeliv2.c_str()));

								if ((fDeliv1 != 0) || (fDeliv2 != 0))
									bHaveDeliverd = true;
								sprintf(szTmp, "%.3f", fUsage1 / EnergyDivider);
								root["result"][ii]["v"] = szTmp;
								sprintf(szTmp, "%.3f", fUsage2 / EnergyDivider);
								root["result"][ii]["v2"] = szTmp;
								sprintf(szTmp, "%.3f", fDeliv1 / EnergyDivider);
								root["result"][ii]["r1"] = szTmp;
								sprintf(szTmp, "%.3f", fDeliv2 / EnergyDivider);
								root["result"][ii]["r2"] = szTmp;
								ii++;
							}
							if (bHaveDeliverd)
							{
								root["delivered"] = true;
							}
						}
					}
					else
					{
						szQuery << "SELECT Value, Date FROM " << dbasetable << " WHERE (DeviceRowID==" << idx << " AND Date>='" << szDateStart << "' AND Date<='" << szDateEnd << "') ORDER BY Date ASC";
						result = m_sql.query(szQuery.str());
						if (result.size() > 0)
						{
							std::vector<std::vector<std::string> >::const_iterator itt;
							for (itt = result.begin(); itt != result.end(); ++itt)
							{
								std::vector<std::string> sd = *itt;

								root["result"][ii]["d"] = sd[1].substr(0, 16);
								std::string szValue = sd[0];
								switch (metertype)
								{
								case MTYPE_ENERGY:
									sprintf(szTmp, "%.3f", atof(szValue.c_str()) / EnergyDivider);
									szValue = szTmp;
									break;
								case MTYPE_GAS:
									sprintf(szTmp, "%.3f", atof(szValue.c_str()) / GasDivider);
									szValue = szTmp;
									break;
								case MTYPE_WATER:
									sprintf(szTmp, "%.3f", atof(szValue.c_str()) / WaterDivider);
									szValue = szTmp;
									break;
								}
								root["result"][ii]["v"] = szValue;
								ii++;
							}
						}
					}
					//add today (have to calculate it)
					szQuery.clear();
					szQuery.str("");
					if (dType == pTypeP1Power)
					{
						szQuery << "SELECT MIN(Value1), MAX(Value1), MIN(Value2), MAX(Value2),MIN(Value5), MAX(Value5), MIN(Value6), MAX(Value6) FROM MultiMeter WHERE (DeviceRowID=" << idx << " AND Date>='" << szDateEnd << "')";
						result = m_sql.query(szQuery.str());
						if (result.size() > 0)
						{
							std::vector<std::string> sd = result[0];

							unsigned long long total_min_usage_1, total_min_usage_2, total_max_usage_1, total_max_usage_2, total_real_usage_1, total_real_usage_2;
							unsigned long long total_min_deliv_1, total_min_deliv_2, total_max_deliv_1, total_max_deliv_2, total_real_deliv_1, total_real_deliv_2;

							bool bHaveDeliverd = false;

							std::stringstream s_str1(sd[0]);
							s_str1 >> total_min_usage_1;
							std::stringstream s_str2(sd[1]);
							s_str2 >> total_max_usage_1;
							std::stringstream s_str3(sd[4]);
							s_str3 >> total_min_usage_2;
							std::stringstream s_str4(sd[5]);
							s_str4 >> total_max_usage_2;

							total_real_usage_1 = total_max_usage_1 - total_min_usage_1;
							total_real_usage_2 = total_max_usage_2 - total_min_usage_2;

							std::stringstream s_str5(sd[2]);
							s_str5 >> total_min_deliv_1;
							std::stringstream s_str6(sd[3]);
							s_str6 >> total_max_deliv_1;
							std::stringstream s_str7(sd[6]);
							s_str7 >> total_min_deliv_2;
							std::stringstream s_str8(sd[7]);
							s_str8 >> total_max_deliv_2;

							total_real_deliv_1 = total_max_deliv_1 - total_min_deliv_1;
							total_real_deliv_2 = total_max_deliv_2 - total_min_deliv_2;
							if ((total_real_deliv_1 != 0) || (total_real_deliv_2 != 0))
								bHaveDeliverd = true;

							root["result"][ii]["d"] = szDateEnd;

							sprintf(szTmp, "%llu", total_real_usage_1);
							std::string szValue = szTmp;
							sprintf(szTmp, "%.3f", atof(szValue.c_str()) / EnergyDivider);
							root["result"][ii]["v"] = szTmp;
							sprintf(szTmp, "%llu", total_real_usage_2);
							szValue = szTmp;
							sprintf(szTmp, "%.3f", atof(szValue.c_str()) / EnergyDivider);
							root["result"][ii]["v2"] = szTmp;

							sprintf(szTmp, "%llu", total_real_deliv_1);
							szValue = szTmp;
							sprintf(szTmp, "%.3f", atof(szValue.c_str()) / EnergyDivider);
							root["result"][ii]["r1"] = szTmp;
							sprintf(szTmp, "%llu", total_real_deliv_2);
							szValue = szTmp;
							sprintf(szTmp, "%.3f", atof(szValue.c_str()) / EnergyDivider);
							root["result"][ii]["r2"] = szTmp;

							ii++;
							if (bHaveDeliverd)
							{
								root["delivered"] = true;
							}
						}
					}
					else
					{
						szQuery << "SELECT MIN(Value), MAX(Value) FROM Meter WHERE (DeviceRowID=" << idx << " AND Date>='" << szDateEnd << "')";
						result = m_sql.query(szQuery.str());
						if (result.size() > 0)
						{
							std::vector<std::string> sd = result[0];

							unsigned long long total_min, total_max, total_real;

							std::stringstream s_str1(sd[0]);
							s_str1 >> total_min;
							std::stringstream s_str2(sd[1]);
							s_str2 >> total_max;
							total_real = total_max - total_min;
							sprintf(szTmp, "%llu", total_real);
							std::string szValue = szTmp;
							switch (metertype)
							{
							case MTYPE_ENERGY:
								sprintf(szTmp, "%.3f", atof(szValue.c_str()) / EnergyDivider);
								szValue = szTmp;
								break;
							case MTYPE_GAS:
								sprintf(szTmp, "%.3f", atof(szValue.c_str()) / GasDivider);
								szValue = szTmp;
								break;
							case MTYPE_WATER:
								sprintf(szTmp, "%.3f", atof(szValue.c_str()) / WaterDivider);
								szValue = szTmp;
								break;
							}

							root["result"][ii]["d"] = szDateEnd;
							root["result"][ii]["v"] = szValue;
							ii++;
						}
					}
				}
			}//week
			else if ((srange == "month") || (srange == "year"))
			{
				char szDateStart[40];
				char szDateEnd[40];
				char szDateStartPrev[40];
				char szDateEndPrev[40];

				std::string sactmonth = m_pWebEm->FindValue("actmonth");
				std::string sactyear = m_pWebEm->FindValue("actyear");

				int actMonth = atoi(sactmonth.c_str());
				int actYear = atoi(sactyear.c_str());

				if ((sactmonth != "") && (sactyear != ""))
				{
					sprintf(szDateStart, "%04d-%02d-%02d", actYear, actMonth, 1);
					sprintf(szDateStartPrev, "%04d-%02d-%02d", actYear - 1, actMonth, 1);
					actMonth++;
					if (actMonth == 13)
					{
						actMonth = 1;
						actYear++;
					}
					sprintf(szDateEnd, "%04d-%02d-%02d", actYear, actMonth, 1);
					sprintf(szDateEndPrev, "%04d-%02d-%02d", actYear - 1, actMonth, 1);
				}
				else if (sactyear != "")
				{
					sprintf(szDateStart, "%04d-%02d-%02d", actYear, 1, 1);
					sprintf(szDateStartPrev, "%04d-%02d-%02d", actYear - 1, 1, 1);
					actYear++;
					sprintf(szDateEnd, "%04d-%02d-%02d", actYear, 1, 1);
					sprintf(szDateEndPrev, "%04d-%02d-%02d", actYear - 1, 1, 1);
				}
				else
				{
					struct tm ltime;
					ltime.tm_isdst = tm1.tm_isdst;
					ltime.tm_hour = 0;
					ltime.tm_min = 0;
					ltime.tm_sec = 0;
					ltime.tm_year = tm1.tm_year;
					ltime.tm_mon = tm1.tm_mon;
					ltime.tm_mday = tm1.tm_mday;

					sprintf(szDateEnd, "%04d-%02d-%02d", ltime.tm_year + 1900, ltime.tm_mon + 1, ltime.tm_mday);
					sprintf(szDateEndPrev, "%04d-%02d-%02d", ltime.tm_year + 1900 - 1, ltime.tm_mon + 1, ltime.tm_mday);

					if (srange == "month")
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
					localtime_r(&later, &tm2);

					sprintf(szDateStart, "%04d-%02d-%02d", tm2.tm_year + 1900, tm2.tm_mon + 1, tm2.tm_mday);
					sprintf(szDateStartPrev, "%04d-%02d-%02d", tm2.tm_year + 1900 - 1, tm2.tm_mon + 1, tm2.tm_mday);
				}

				if (sensor == "temp") {
					root["status"] = "OK";
					root["title"] = "Graph " + sensor + " " + srange;

					//Actual Year
					szQuery.clear();
					szQuery.str("");
					szQuery << "SELECT Temp_Min, Temp_Max, Chill_Min, Chill_Max, Humidity, Barometer, Temp_Avg, Date, SetPoint_Min, SetPoint_Max, SetPoint_Avg FROM " << dbasetable << " WHERE (DeviceRowID==" << idx << " AND Date>='" << szDateStart << "' AND Date<='" << szDateEnd << "') ORDER BY Date ASC";
					result = m_sql.query(szQuery.str());
					int ii = 0;
					if (result.size() > 0)
					{
						std::vector<std::vector<std::string> >::const_iterator itt;
						for (itt = result.begin(); itt != result.end(); ++itt)
						{
							std::vector<std::string> sd = *itt;

							root["result"][ii]["d"] = sd[7].substr(0, 16);

							if (
								(dType == pTypeRego6XXTemp) || (dType == pTypeTEMP) || (dType == pTypeTEMP_HUM) || (dType == pTypeTEMP_HUM_BARO) || (dType == pTypeTEMP_BARO) || (dType == pTypeWIND) || (dType == pTypeThermostat1) ||
								((dType == pTypeRFXSensor) && (dSubType == sTypeRFXSensorTemp)) ||
								((dType == pTypeUV) && (dSubType == sTypeUV3)) ||
								((dType == pTypeGeneral) && (dSubType == sTypeSystemTemp)) ||
								((dType == pTypeThermostat) && (dSubType == sTypeThermSetpoint)) ||
								(dType == pTypeEvohomeZone) || (dType == pTypeEvohomeWater)||
								((dType == pTypeGeneral) && (dSubType == sTypeBaro))
								)
							{
								bool bOK = true;
								if (dType == pTypeWIND)
								{
									bOK = ((dSubType == sTypeWIND4) || (dSubType == sTypeWINDNoTemp));
								}
								if (bOK)
								{
									double te = ConvertTemperature(atof(sd[1].c_str()), tempsign);
									double tm = ConvertTemperature(atof(sd[0].c_str()), tempsign);
									double ta = ConvertTemperature(atof(sd[6].c_str()), tempsign);
									root["result"][ii]["te"] = te;
									root["result"][ii]["tm"] = tm;
									root["result"][ii]["ta"] = ta;
								}
							}
							if (
								((dType == pTypeWIND) && (dSubType == sTypeWIND4)) ||
								((dType == pTypeWIND) && (dSubType == sTypeWINDNoTemp))
								)
							{
								double ch = ConvertTemperature(atof(sd[3].c_str()), tempsign);
								double cm = ConvertTemperature(atof(sd[2].c_str()), tempsign);
								root["result"][ii]["ch"] = ch;
								root["result"][ii]["cm"] = cm;
							}
							if ((dType == pTypeHUM) || (dType == pTypeTEMP_HUM) || (dType == pTypeTEMP_HUM_BARO))
							{
								root["result"][ii]["hu"] = sd[4];
							}
							if (
								(dType == pTypeTEMP_HUM_BARO) ||
								(dType == pTypeTEMP_BARO)||
								((dType == pTypeGeneral) && (dSubType == sTypeBaro))
								)
							{
								if (dType == pTypeTEMP_HUM_BARO)
								{
									if (dSubType == sTypeTHBFloat)
									{
										sprintf(szTmp, "%.1f", atof(sd[5].c_str()) / 10.0f);
										root["result"][ii]["ba"] = szTmp;
									}
									else
										root["result"][ii]["ba"] = sd[5];
								}
								else if (dType == pTypeTEMP_BARO)
								{
									sprintf(szTmp, "%.1f", atof(sd[5].c_str()) / 10.0f);
									root["result"][ii]["ba"] = szTmp;
								}
								else if ((dType == pTypeGeneral) && (dSubType == sTypeBaro))
								{
									sprintf(szTmp, "%.1f", atof(sd[5].c_str()) / 10.0f);
									root["result"][ii]["ba"] = szTmp;
								}
							}
							if((dType == pTypeEvohomeZone) || (dType == pTypeEvohomeWater))
							{
								double sm = ConvertTemperature(atof(sd[8].c_str()), tempsign);
								double sx = ConvertTemperature(atof(sd[9].c_str()), tempsign);
								double se = ConvertTemperature(atof(sd[10].c_str()), tempsign);
								root["result"][ii]["sm"] = sm;
								root["result"][ii]["se"] = se;
								root["result"][ii]["sx"] = sx;
							}
							ii++;
						}
					}
					//add today (have to calculate it)
					szQuery.clear();
					szQuery.str("");
					szQuery << "SELECT MIN(Temperature), MAX(Temperature), MIN(Chill), MAX(Chill), MAX(Humidity), MAX(Barometer), AVG(Temperature), MIN(SetPoint), MAX(SetPoint), AVG(SetPoint) FROM Temperature WHERE (DeviceRowID=" << idx << " AND Date>='" << szDateEnd << "')";
					result = m_sql.query(szQuery.str());
					if (result.size() > 0)
					{
						std::vector<std::string> sd = result[0];

						root["result"][ii]["d"] = szDateEnd;
						if (
							((dType == pTypeRego6XXTemp) || (dType == pTypeTEMP) || (dType == pTypeTEMP_HUM) || (dType == pTypeTEMP_HUM_BARO) || (dType == pTypeTEMP_BARO) || (dType == pTypeWIND) || (dType == pTypeThermostat1)) ||
							((dType == pTypeUV) && (dSubType == sTypeUV3)) ||
							((dType == pTypeWIND) && (dSubType == sTypeWIND4)) ||
							((dType == pTypeWIND) && (dSubType == sTypeWINDNoTemp)) ||
							(dType == pTypeEvohomeZone) || (dType == pTypeEvohomeWater)
							)
						{
							double te = ConvertTemperature(atof(sd[1].c_str()), tempsign);
							double tm = ConvertTemperature(atof(sd[0].c_str()), tempsign);
							double ta = ConvertTemperature(atof(sd[6].c_str()), tempsign);

							root["result"][ii]["te"] = te;
							root["result"][ii]["tm"] = tm;
							root["result"][ii]["ta"] = ta;
						}
						if (
							((dType == pTypeWIND) && (dSubType == sTypeWIND4)) ||
							((dType == pTypeWIND) && (dSubType == sTypeWINDNoTemp))
							)
						{
							double ch = ConvertTemperature(atof(sd[3].c_str()), tempsign);
							double cm = ConvertTemperature(atof(sd[2].c_str()), tempsign);
							root["result"][ii]["ch"] = ch;
							root["result"][ii]["cm"] = cm;
						}
						if ((dType == pTypeHUM) || (dType == pTypeTEMP_HUM) || (dType == pTypeTEMP_HUM_BARO))
						{
							root["result"][ii]["hu"] = sd[4];
						}
						if (
							(dType == pTypeTEMP_HUM_BARO) ||
							(dType == pTypeTEMP_BARO)||
							((dType == pTypeGeneral) && (dSubType == sTypeBaro))
							)
						{
							if (dType == pTypeTEMP_HUM_BARO)
							{
								if (dSubType == sTypeTHBFloat)
								{
									sprintf(szTmp, "%.1f", atof(sd[5].c_str()) / 10.0f);
									root["result"][ii]["ba"] = szTmp;
								}
								else
									root["result"][ii]["ba"] = sd[5];
							}
							else if (dType == pTypeTEMP_BARO)
							{
								sprintf(szTmp, "%.1f", atof(sd[5].c_str()) / 10.0f);
								root["result"][ii]["ba"] = szTmp;
							}
							else if ((dType == pTypeGeneral) && (dSubType == sTypeBaro))
							{
								sprintf(szTmp, "%.1f", atof(sd[5].c_str()) / 10.0f);
								root["result"][ii]["ba"] = szTmp;
							}
						}
						if((dType == pTypeEvohomeZone) || (dType == pTypeEvohomeWater))
						{
							double sx = ConvertTemperature(atof(sd[8].c_str()), tempsign);
							double sm = ConvertTemperature(atof(sd[7].c_str()), tempsign);
							double se = ConvertTemperature(atof(sd[9].c_str()), tempsign);
							root["result"][ii]["se"] = se;
							root["result"][ii]["sm"] = sm;
							root["result"][ii]["sx"] = sx;
						}
						ii++;
					}
					//Previous Year
					szQuery.clear();
					szQuery.str("");
					szQuery << "SELECT Temp_Min, Temp_Max, Chill_Min, Chill_Max, Humidity, Barometer, Temp_Avg, Date, SetPoint_Min, SetPoint_Max, SetPoint_Avg FROM " << dbasetable << " WHERE (DeviceRowID==" << idx << " AND Date>='" << szDateStartPrev << "' AND Date<='" << szDateEndPrev << "') ORDER BY Date ASC";
					result = m_sql.query(szQuery.str());
					if (result.size() > 0)
					{
						int iPrev = 0;
						std::vector<std::vector<std::string> >::const_iterator itt;
						for (itt = result.begin(); itt != result.end(); ++itt)
						{
							std::vector<std::string> sd = *itt;

							root["resultprev"][iPrev]["d"] = sd[7].substr(0, 16);

							if (
								(dType == pTypeRego6XXTemp) || (dType == pTypeTEMP) || (dType == pTypeTEMP_HUM) || (dType == pTypeTEMP_HUM_BARO) || (dType == pTypeTEMP_BARO) || (dType == pTypeWIND) || (dType == pTypeThermostat1) ||
								((dType == pTypeRFXSensor) && (dSubType == sTypeRFXSensorTemp)) ||
								((dType == pTypeUV) && (dSubType == sTypeUV3)) ||
								((dType == pTypeGeneral) && (dSubType == sTypeSystemTemp)) ||
								((dType == pTypeThermostat) && (dSubType == sTypeThermSetpoint)) ||
								(dType == pTypeEvohomeZone) || (dType == pTypeEvohomeWater)
								)
							{
								bool bOK = true;
								if (dType == pTypeWIND)
								{
									bOK = ((dSubType == sTypeWIND4) || (dSubType == sTypeWINDNoTemp));
								}
								if (bOK)
								{
									double te = ConvertTemperature(atof(sd[1].c_str()), tempsign);
									double tm = ConvertTemperature(atof(sd[0].c_str()), tempsign);
									double ta = ConvertTemperature(atof(sd[6].c_str()), tempsign);
									root["resultprev"][iPrev]["te"] = te;
									root["resultprev"][iPrev]["tm"] = tm;
									root["resultprev"][iPrev]["ta"] = ta;
								}
							}
							if (
								((dType == pTypeWIND) && (dSubType == sTypeWIND4)) ||
								((dType == pTypeWIND) && (dSubType == sTypeWINDNoTemp))
								)
							{
								double ch = ConvertTemperature(atof(sd[3].c_str()), tempsign);
								double cm = ConvertTemperature(atof(sd[2].c_str()), tempsign);
								root["resultprev"][iPrev]["ch"] = ch;
								root["resultprev"][iPrev]["cm"] = cm;
							}
							if ((dType == pTypeHUM) || (dType == pTypeTEMP_HUM) || (dType == pTypeTEMP_HUM_BARO))
							{
								root["resultprev"][iPrev]["hu"] = sd[4];
							}
							if (
								(dType == pTypeTEMP_HUM_BARO) ||
								(dType == pTypeTEMP_BARO)||
								((dType == pTypeGeneral) && (dSubType == sTypeBaro))
								)
							{
								if (dType == pTypeTEMP_HUM_BARO)
								{
									if (dSubType == sTypeTHBFloat)
									{
										sprintf(szTmp, "%.1f", atof(sd[5].c_str()) / 10.0f);
										root["resultprev"][iPrev]["ba"] = szTmp;
									}
									else
										root["resultprev"][iPrev]["ba"] = sd[5];
								}
								else if (dType == pTypeTEMP_BARO)
								{
									sprintf(szTmp, "%.1f", atof(sd[5].c_str()) / 10.0f);
									root["resultprev"][iPrev]["ba"] = szTmp;
								}
								else if ((dType == pTypeGeneral) && (dSubType == sTypeBaro))
								{
									sprintf(szTmp, "%.1f", atof(sd[5].c_str()) / 10.0f);
									root["resultprev"][iPrev]["ba"] = szTmp;
								}
							}
							if ((dType == pTypeEvohomeZone) || (dType == pTypeEvohomeWater))
							{
								double sx = ConvertTemperature(atof(sd[8].c_str()), tempsign);
								double sm = ConvertTemperature(atof(sd[7].c_str()), tempsign);
								double se = ConvertTemperature(atof(sd[9].c_str()), tempsign);
								root["resultprev"][iPrev]["se"] = se;
								root["resultprev"][iPrev]["sm"] = sm;
								root["resultprev"][iPrev]["sx"] = sx;
							}
							iPrev++;
						}
					}
				}
				else if (sensor == "Percentage") {
					root["status"] = "OK";
					root["title"] = "Graph " + sensor + " " + srange;

					szQuery.clear();
					szQuery.str("");
					szQuery << "SELECT Percentage_Min, Percentage_Max, Percentage_Avg, Date FROM " << dbasetable << " WHERE (DeviceRowID==" << idx << " AND Date>='" << szDateStart << "' AND Date<='" << szDateEnd << "') ORDER BY Date ASC";
					result = m_sql.query(szQuery.str());
					int ii = 0;
					if (result.size() > 0)
					{
						std::vector<std::vector<std::string> >::const_iterator itt;
						for (itt = result.begin(); itt != result.end(); ++itt)
						{
							std::vector<std::string> sd = *itt;

							root["result"][ii]["d"] = sd[3].substr(0, 16);
							root["result"][ii]["v_min"] = sd[0];
							root["result"][ii]["v_max"] = sd[1];
							root["result"][ii]["v_avg"] = sd[2];
							ii++;
						}
					}
					//add today (have to calculate it)
					szQuery.clear();
					szQuery.str("");
					szQuery << "SELECT MIN(Percentage), MAX(Percentage), AVG(Percentage) FROM Percentage WHERE (DeviceRowID=" << idx << " AND Date>='" << szDateEnd << "')";
					result = m_sql.query(szQuery.str());
					if (result.size() > 0)
					{
						std::vector<std::string> sd = result[0];
						root["result"][ii]["d"] = szDateEnd;
						root["result"][ii]["v_min"] = sd[0];
						root["result"][ii]["v_max"] = sd[1];
						root["result"][ii]["v_avg"] = sd[2];
						ii++;
					}

				}
				else if (sensor == "fan") {
					root["status"] = "OK";
					root["title"] = "Graph " + sensor + " " + srange;

					szQuery.clear();
					szQuery.str("");
					szQuery << "SELECT Speed_Min, Speed_Max, Date FROM " << dbasetable << " WHERE (DeviceRowID==" << idx << " AND Date>='" << szDateStart << "' AND Date<='" << szDateEnd << "') ORDER BY Date ASC";
					result = m_sql.query(szQuery.str());
					int ii = 0;
					if (result.size() > 0)
					{
						std::vector<std::vector<std::string> >::const_iterator itt;
						for (itt = result.begin(); itt != result.end(); ++itt)
						{
							std::vector<std::string> sd = *itt;

							root["result"][ii]["d"] = sd[2].substr(0, 16);
							root["result"][ii]["v_max"] = sd[1];
							root["result"][ii]["v_min"] = sd[0];
							ii++;
						}
					}
					//add today (have to calculate it)
					szQuery.clear();
					szQuery.str("");
					szQuery << "SELECT MIN(Speed), MAX(Speed) FROM Fan WHERE (DeviceRowID=" << idx << " AND Date>='" << szDateEnd << "')";
					result = m_sql.query(szQuery.str());
					if (result.size() > 0)
					{
						std::vector<std::string> sd = result[0];
						root["result"][ii]["d"] = szDateEnd;
						root["result"][ii]["v_max"] = sd[1];
						root["result"][ii]["v_min"] = sd[0];
						ii++;
					}

				}
				else if (sensor == "uv") {
					root["status"] = "OK";
					root["title"] = "Graph " + sensor + " " + srange;

					szQuery.clear();
					szQuery.str("");
					szQuery << "SELECT Level, Date FROM " << dbasetable << " WHERE (DeviceRowID==" << idx << " AND Date>='" << szDateStart << "' AND Date<='" << szDateEnd << "') ORDER BY Date ASC";
					result = m_sql.query(szQuery.str());
					int ii = 0;
					if (result.size() > 0)
					{
						std::vector<std::vector<std::string> >::const_iterator itt;
						for (itt = result.begin(); itt != result.end(); ++itt)
						{
							std::vector<std::string> sd = *itt;

							root["result"][ii]["d"] = sd[1].substr(0, 16);
							root["result"][ii]["uvi"] = sd[0];
							ii++;
						}
					}
					//add today (have to calculate it)
					szQuery.clear();
					szQuery.str("");
					szQuery << "SELECT MAX(Level) FROM UV WHERE (DeviceRowID=" << idx << " AND Date>='" << szDateEnd << "')";
					result = m_sql.query(szQuery.str());
					if (result.size() > 0)
					{
						std::vector<std::string> sd = result[0];

						root["result"][ii]["d"] = szDateEnd;
						root["result"][ii]["uvi"] = sd[0];
						ii++;
					}
					//Previous Year
					szQuery.clear();
					szQuery.str("");
					szQuery << "SELECT Level, Date FROM " << dbasetable << " WHERE (DeviceRowID==" << idx << " AND Date>='" << szDateStartPrev << "' AND Date<='" << szDateEndPrev << "') ORDER BY Date ASC";
					result = m_sql.query(szQuery.str());
					if (result.size() > 0)
					{
						int iPrev = 0;
						std::vector<std::vector<std::string> >::const_iterator itt;
						for (itt = result.begin(); itt != result.end(); ++itt)
						{
							std::vector<std::string> sd = *itt;

							root["resultprev"][iPrev]["d"] = sd[1].substr(0, 16);
							root["resultprev"][iPrev]["uvi"] = sd[0];
							iPrev++;
						}
					}
				}
				else if (sensor == "rain") {
					root["status"] = "OK";
					root["title"] = "Graph " + sensor + " " + srange;

					szQuery.clear();
					szQuery.str("");
					szQuery << "SELECT Total, Rate, Date FROM " << dbasetable << " WHERE (DeviceRowID==" << idx << " AND Date>='" << szDateStart << "' AND Date<='" << szDateEnd << "') ORDER BY Date ASC";
					result = m_sql.query(szQuery.str());
					int ii = 0;
					if (result.size() > 0)
					{
						std::vector<std::vector<std::string> >::const_iterator itt;
						for (itt = result.begin(); itt != result.end(); ++itt)
						{
							std::vector<std::string> sd = *itt;

							root["result"][ii]["d"] = sd[2].substr(0, 16);
							double mmval = atof(sd[0].c_str());
							mmval *= AddjMulti;
							sprintf(szTmp, "%.1f", mmval);
							root["result"][ii]["mm"] = szTmp;
							ii++;
						}
					}
					//add today (have to calculate it)
					szQuery.clear();
					szQuery.str("");
					if (dSubType != sTypeRAINWU)
					{
						szQuery << "SELECT MIN(Total), MAX(Total), MAX(Rate) FROM Rain WHERE (DeviceRowID=" << idx << " AND Date>='" << szDateEnd << "')";
					}
					else
					{
						szQuery << "SELECT Total, Total, Rate FROM Rain WHERE (DeviceRowID=" << idx << " AND Date>='" << szDateEnd << "') ORDER BY ROWID DESC LIMIT 1";
					}
					result = m_sql.query(szQuery.str());
					if (result.size() > 0)
					{
						std::vector<std::string> sd = result[0];

						float total_min = static_cast<float>(atof(sd[0].c_str()));
						float total_max = static_cast<float>(atof(sd[1].c_str()));
						int rate = atoi(sd[2].c_str());

						double total_real = 0;
						if (dSubType != sTypeRAINWU)
						{
							total_real = total_max - total_min;
						}
						else
						{
							total_real = total_max;
						}
						total_real *= AddjMulti;
						sprintf(szTmp, "%.1f", total_real);
						root["result"][ii]["d"] = szDateEnd;
						root["result"][ii]["mm"] = szTmp;
						ii++;
					}
					//Previous Year
					szQuery.clear();
					szQuery.str("");
					szQuery << "SELECT Total, Rate, Date FROM " << dbasetable << " WHERE (DeviceRowID==" << idx << " AND Date>='" << szDateStartPrev << "' AND Date<='" << szDateEndPrev << "') ORDER BY Date ASC";
					result = m_sql.query(szQuery.str());
					if (result.size() > 0)
					{
						int iPrev = 0;
						std::vector<std::vector<std::string> >::const_iterator itt;
						for (itt = result.begin(); itt != result.end(); ++itt)
						{
							std::vector<std::string> sd = *itt;

							root["resultprev"][iPrev]["d"] = sd[2].substr(0, 16);
							double mmval = atof(sd[0].c_str());
							mmval *= AddjMulti;
							sprintf(szTmp, "%.1f", mmval);
							root["resultprev"][iPrev]["mm"] = szTmp;
							iPrev++;
						}
					}
				}
				else if (sensor == "counter") {
					root["status"] = "OK";
					root["title"] = "Graph " + sensor + " " + srange;

					int nValue = 0;
					std::string sValue = "";

					szQuery.clear();
					szQuery.str("");
					szQuery << "SELECT nValue, sValue FROM DeviceStatus WHERE (ID==" << idx << ")";
					result = m_sql.query(szQuery.str());
					if (result.size() > 0)
					{
						std::vector<std::string> sd = result[0];
						nValue = atoi(sd[0].c_str());
						sValue = sd[1];
					}

					float EnergyDivider = 1000.0f;
					float GasDivider = 100.0f;
					float WaterDivider = 100.0f;
					int tValue;
					if (m_sql.GetPreferencesVar("MeterDividerEnergy", tValue))
					{
						EnergyDivider = float(tValue);
					}
					if (m_sql.GetPreferencesVar("MeterDividerGas", tValue))
					{
						GasDivider = float(tValue);
					}
					if (m_sql.GetPreferencesVar("MeterDividerWater", tValue))
					{
						WaterDivider = float(tValue);
					}
					if (dType == pTypeP1Gas)
						GasDivider = 1000.0f;
					else if ((dType == pTypeENERGY) || (dType == pTypePOWER))
						EnergyDivider *= 100.0f;
					//				else if (dType==pTypeRFXMeter)
					//				EnergyDivider*=1000.0f;

					szQuery.clear();
					szQuery.str("");
					int ii = 0;
					int iPrev = 0;
					if (dType == pTypeP1Power)
					{
						//Actual Year
						szQuery << "SELECT Value1,Value2,Value5,Value6, Date, Counter1, Counter2, Counter3, Counter4 FROM " << dbasetable << " WHERE (DeviceRowID==" << idx << " AND Date>='" << szDateStart << "' AND Date<='" << szDateEnd << "') ORDER BY Date ASC";
						result = m_sql.query(szQuery.str());
						if (result.size() > 0)
						{
							bool bHaveDeliverd = false;
							std::vector<std::vector<std::string> >::const_iterator itt;
							for (itt = result.begin(); itt != result.end(); ++itt)
							{
								std::vector<std::string> sd = *itt;

								root["result"][ii]["d"] = sd[4].substr(0, 16);

								double counter_1 = atof(sd[5].c_str());
								double counter_2 = atof(sd[6].c_str());
								double counter_3 = atof(sd[7].c_str());
								double counter_4 = atof(sd[8].c_str());

								std::string szUsage1 = sd[0];
								std::string szDeliv1 = sd[1];
								std::string szUsage2 = sd[2];
								std::string szDeliv2 = sd[3];

								float fUsage_1 = static_cast<float>(atof(szUsage1.c_str()));
								float fUsage_2 = static_cast<float>(atof(szUsage2.c_str()));
								float fDeliv_1 = static_cast<float>(atof(szDeliv1.c_str()));
								float fDeliv_2 = static_cast<float>(atof(szDeliv2.c_str()));

								if ((fDeliv_1 != 0) || (fDeliv_2 != 0))
									bHaveDeliverd = true;
								sprintf(szTmp, "%.3f", fUsage_1 / EnergyDivider);
								root["result"][ii]["v"] = szTmp;
								sprintf(szTmp, "%.3f", fUsage_2 / EnergyDivider);
								root["result"][ii]["v2"] = szTmp;
								sprintf(szTmp, "%.3f", fDeliv_1 / EnergyDivider);
								root["result"][ii]["r1"] = szTmp;
								sprintf(szTmp, "%.3f", fDeliv_2 / EnergyDivider);
								root["result"][ii]["r2"] = szTmp;

								if (counter_1 != 0)
								{
									sprintf(szTmp, "%.3f", (counter_1 - fUsage_1) / EnergyDivider);
								}
								else
								{
									strcpy(szTmp, "0");
								}
								root["result"][ii]["c1"] = szTmp;
								if (counter_3 != 0)
								{
									sprintf(szTmp, "%.3f", (counter_3 - fUsage_2) / EnergyDivider);
								}
								else
								{
									strcpy(szTmp, "0");
								}
								root["result"][ii]["c3"] = szTmp;
								ii++;
							}
							if (bHaveDeliverd)
							{
								root["delivered"] = true;
							}
						}
						//Previous Year
						szQuery.clear();
						szQuery.str("");
						szQuery << "SELECT Value1,Value2,Value5,Value6, Date FROM " << dbasetable << " WHERE (DeviceRowID==" << idx << " AND Date>='" << szDateStartPrev << "' AND Date<='" << szDateEndPrev << "') ORDER BY Date ASC";
						result = m_sql.query(szQuery.str());
						if (result.size() > 0)
						{
							bool bHaveDeliverd = false;
							std::vector<std::vector<std::string> >::const_iterator itt;
							for (itt = result.begin(); itt != result.end(); ++itt)
							{
								std::vector<std::string> sd = *itt;

								root["resultprev"][iPrev]["d"] = sd[4].substr(0, 16);

								std::string szUsage1 = sd[0];
								std::string szDeliv1 = sd[1];
								std::string szUsage2 = sd[2];
								std::string szDeliv2 = sd[3];

								float fUsage_1 = static_cast<float>(atof(szUsage1.c_str()));
								float fUsage_2 = static_cast<float>(atof(szUsage2.c_str()));
								float fDeliv_1 = static_cast<float>(atof(szDeliv1.c_str()));
								float fDeliv_2 = static_cast<float>(atof(szDeliv2.c_str()));

								if ((fDeliv_1 != 0) || (fDeliv_2 != 0))
									bHaveDeliverd = true;
								sprintf(szTmp, "%.3f", fUsage_1 / EnergyDivider);
								root["resultprev"][iPrev]["v"] = szTmp;
								sprintf(szTmp, "%.3f", fUsage_2 / EnergyDivider);
								root["resultprev"][iPrev]["v2"] = szTmp;
								sprintf(szTmp, "%.3f", fDeliv_1 / EnergyDivider);
								root["resultprev"][iPrev]["r1"] = szTmp;
								sprintf(szTmp, "%.3f", fDeliv_2 / EnergyDivider);
								root["resultprev"][iPrev]["r2"] = szTmp;
								iPrev++;
							}
							if (bHaveDeliverd)
							{
								root["delivered"] = true;
							}
						}
					}
					else if (dType == pTypeAirQuality)
					{//month/year
						root["status"] = "OK";
						root["title"] = "Graph " + sensor + " " + srange;

						szQuery << "SELECT Value1,Value2, Date FROM " << dbasetable << " WHERE (DeviceRowID==" << idx << " AND Date>='" << szDateStart << "' AND Date<='" << szDateEnd << "') ORDER BY Date ASC";
						result = m_sql.query(szQuery.str());
						if (result.size() > 0)
						{
							std::vector<std::vector<std::string> >::const_iterator itt;
							for (itt = result.begin(); itt != result.end(); ++itt)
							{
								std::vector<std::string> sd = *itt;

								root["result"][ii]["d"] = sd[2].substr(0, 16);
								root["result"][ii]["co2_min"] = sd[0];
								root["result"][ii]["co2_max"] = sd[1];
								ii++;
							}
						}
					}
					else if (
						((dType == pTypeGeneral) && ((dSubType == sTypeSoilMoisture) || (dSubType == sTypeLeafWetness))) ||
						((dType == pTypeRFXSensor) && ((dSubType == sTypeRFXSensorAD) || (dSubType == sTypeRFXSensorVolt)))
						)
					{//month/year
						root["status"] = "OK";
						root["title"] = "Graph " + sensor + " " + srange;

						szQuery << "SELECT Value1,Value2, Date FROM " << dbasetable << " WHERE (DeviceRowID==" << idx << " AND Date>='" << szDateStart << "' AND Date<='" << szDateEnd << "') ORDER BY Date ASC";
						result = m_sql.query(szQuery.str());
						if (result.size() > 0)
						{
							std::vector<std::vector<std::string> >::const_iterator itt;
							for (itt = result.begin(); itt != result.end(); ++itt)
							{
								std::vector<std::string> sd = *itt;

								root["result"][ii]["d"] = sd[2].substr(0, 16);
								root["result"][ii]["v_min"] = sd[0];
								root["result"][ii]["v_max"] = sd[1];
								ii++;
							}
						}
					}
					else if (
						((dType == pTypeGeneral) && (dSubType == sTypeVisibility)) ||
						((dType == pTypeGeneral) && (dSubType == sTypeDistance)) ||
						((dType == pTypeGeneral) && (dSubType == sTypeSolarRadiation)) ||
						((dType == pTypeGeneral) && (dSubType == sTypeVoltage)) ||
						((dType == pTypeGeneral) && (dSubType == sTypeCurrent)) ||
						((dType == pTypeGeneral) && (dSubType == sTypePressure)) ||
						((dType == pTypeGeneral) && (dSubType == sTypeSoundLevel))
						)
					{//month/year
						root["status"] = "OK";
						root["title"] = "Graph " + sensor + " " + srange;

						float vdiv = 10.0f;
						if (
							((dType == pTypeGeneral) && (dSubType == sTypeVoltage)) ||
							((dType == pTypeGeneral) && (dSubType == sTypeCurrent))
							)
						{
							vdiv = 1000.0f;
						}

						szQuery << "SELECT Value1,Value2, Date FROM " << dbasetable << " WHERE (DeviceRowID==" << idx << " AND Date>='" << szDateStart << "' AND Date<='" << szDateEnd << "') ORDER BY Date ASC";
						result = m_sql.query(szQuery.str());
						if (result.size() > 0)
						{
							std::vector<std::vector<std::string> >::const_iterator itt;
							for (itt = result.begin(); itt != result.end(); ++itt)
							{
								std::vector<std::string> sd = *itt;

								root["result"][ii]["d"] = sd[2].substr(0, 16);

								float fValue1 = float(atof(sd[0].c_str())) / vdiv;
								float fValue2 = float(atof(sd[1].c_str())) / vdiv;
								if (metertype == 1)
								{
									fValue1 *= 0.6214f;
									fValue2 *= 0.6214f;
								}
								if ((dType == pTypeGeneral) && (dSubType == sTypeVoltage))
									sprintf(szTmp, "%.3f", fValue1);
								else if ((dType == pTypeGeneral) && (dSubType == sTypeCurrent))
									sprintf(szTmp, "%.3f", fValue1);
								else
									sprintf(szTmp, "%.1f", fValue1);
								root["result"][ii]["v_min"] = szTmp;
								if ((dType == pTypeGeneral) && (dSubType == sTypeVoltage))
									sprintf(szTmp, "%.3f", fValue2);
								else if ((dType == pTypeGeneral) && (dSubType == sTypeCurrent))
									sprintf(szTmp, "%.3f", fValue2);
								else
									sprintf(szTmp, "%.1f", fValue2);
								root["result"][ii]["v_max"] = szTmp;
								ii++;
							}
						}
					}
					else if (dType == pTypeLux)
					{//month/year
						root["status"] = "OK";
						root["title"] = "Graph " + sensor + " " + srange;

						szQuery << "SELECT Value1,Value2, Date FROM " << dbasetable << " WHERE (DeviceRowID==" << idx << " AND Date>='" << szDateStart << "' AND Date<='" << szDateEnd << "') ORDER BY Date ASC";
						result = m_sql.query(szQuery.str());
						if (result.size() > 0)
						{
							std::vector<std::vector<std::string> >::const_iterator itt;
							for (itt = result.begin(); itt != result.end(); ++itt)
							{
								std::vector<std::string> sd = *itt;

								root["result"][ii]["d"] = sd[2].substr(0, 16);
								root["result"][ii]["lux_min"] = sd[0];
								root["result"][ii]["lux_max"] = sd[1];
								ii++;
							}
						}
					}
					else if (dType == pTypeWEIGHT)
					{//month/year
						root["status"] = "OK";
						root["title"] = "Graph " + sensor + " " + srange;

						szQuery << "SELECT Value1,Value2, Date FROM " << dbasetable << " WHERE (DeviceRowID==" << idx << " AND Date>='" << szDateStart << "' AND Date<='" << szDateEnd << "') ORDER BY Date ASC";
						result = m_sql.query(szQuery.str());
						if (result.size() > 0)
						{
							std::vector<std::vector<std::string> >::const_iterator itt;
							for (itt = result.begin(); itt != result.end(); ++itt)
							{
								std::vector<std::string> sd = *itt;

								root["result"][ii]["d"] = sd[2].substr(0, 16);
								sprintf(szTmp, "%.1f", atof(sd[0].c_str()) / 10.0f);
								root["result"][ii]["v_min"] = szTmp;
								sprintf(szTmp, "%.1f", atof(sd[1].c_str()) / 10.0f);
								root["result"][ii]["v_max"] = szTmp;
								ii++;
							}
						}
					}
					else if (dType == pTypeUsage)
					{//month/year
						root["status"] = "OK";
						root["title"] = "Graph " + sensor + " " + srange;

						szQuery << "SELECT Value1,Value2, Date FROM " << dbasetable << " WHERE (DeviceRowID==" << idx << " AND Date>='" << szDateStart << "' AND Date<='" << szDateEnd << "') ORDER BY Date ASC";
						result = m_sql.query(szQuery.str());
						if (result.size() > 0)
						{
							std::vector<std::vector<std::string> >::const_iterator itt;
							for (itt = result.begin(); itt != result.end(); ++itt)
							{
								std::vector<std::string> sd = *itt;

								root["result"][ii]["d"] = sd[2].substr(0, 16);
								root["result"][ii]["u_min"] = atof(sd[0].c_str()) / 10.0f;
								root["result"][ii]["u_max"] = atof(sd[1].c_str()) / 10.0f;
								ii++;
							}
						}
					}
					else if (dType == pTypeCURRENT)
					{
						szQuery << "SELECT Value1,Value2,Value3,Value4,Value5,Value6, Date FROM " << dbasetable << " WHERE (DeviceRowID==" << idx << " AND Date>='" << szDateStart << "' AND Date<='" << szDateEnd << "') ORDER BY Date ASC";
						result = m_sql.query(szQuery.str());
						if (result.size() > 0)
						{
							//CM113
							int displaytype = 0;
							int voltage = 230;
							m_sql.GetPreferencesVar("CM113DisplayType", displaytype);
							m_sql.GetPreferencesVar("ElectricVoltage", voltage);

							root["displaytype"] = displaytype;

							bool bHaveL1 = false;
							bool bHaveL2 = false;
							bool bHaveL3 = false;
							std::vector<std::vector<std::string> >::const_iterator itt;
							for (itt = result.begin(); itt != result.end(); ++itt)
							{
								std::vector<std::string> sd = *itt;

								root["result"][ii]["d"] = sd[6].substr(0, 16);

								float fval1 = static_cast<float>(atof(sd[0].c_str()) / 10.0f);
								float fval2 = static_cast<float>(atof(sd[1].c_str()) / 10.0f);
								float fval3 = static_cast<float>(atof(sd[2].c_str()) / 10.0f);
								float fval4 = static_cast<float>(atof(sd[3].c_str()) / 10.0f);
								float fval5 = static_cast<float>(atof(sd[4].c_str()) / 10.0f);
								float fval6 = static_cast<float>(atof(sd[5].c_str()) / 10.0f);

								if ((fval1 != 0) || (fval2 != 0))
									bHaveL1 = true;
								if ((fval3 != 0) || (fval4 != 0))
									bHaveL2 = true;
								if ((fval5 != 0) || (fval6 != 0))
									bHaveL3 = true;

								if (displaytype == 0)
								{
									sprintf(szTmp, "%.1f", fval1);
									root["result"][ii]["v1"] = szTmp;
									sprintf(szTmp, "%.1f", fval2);
									root["result"][ii]["v2"] = szTmp;
									sprintf(szTmp, "%.1f", fval3);
									root["result"][ii]["v3"] = szTmp;
									sprintf(szTmp, "%.1f", fval4);
									root["result"][ii]["v4"] = szTmp;
									sprintf(szTmp, "%.1f", fval5);
									root["result"][ii]["v5"] = szTmp;
									sprintf(szTmp, "%.1f", fval6);
									root["result"][ii]["v6"] = szTmp;
								}
								else
								{
									sprintf(szTmp, "%d", int(fval1*voltage));
									root["result"][ii]["v1"] = szTmp;
									sprintf(szTmp, "%d", int(fval2*voltage));
									root["result"][ii]["v2"] = szTmp;
									sprintf(szTmp, "%d", int(fval3*voltage));
									root["result"][ii]["v3"] = szTmp;
									sprintf(szTmp, "%d", int(fval4*voltage));
									root["result"][ii]["v4"] = szTmp;
									sprintf(szTmp, "%d", int(fval5*voltage));
									root["result"][ii]["v5"] = szTmp;
									sprintf(szTmp, "%d", int(fval6*voltage));
									root["result"][ii]["v6"] = szTmp;
								}

								ii++;
							}
							if (
								(!bHaveL1) &&
								(!bHaveL2) &&
								(!bHaveL3)
								) {
								root["haveL1"] = true; //show at least something
							}
							else {
								if (bHaveL1)
									root["haveL1"] = true;
								if (bHaveL2)
									root["haveL2"] = true;
								if (bHaveL3)
									root["haveL3"] = true;
							}
						}
					}
					else if (dType == pTypeCURRENTENERGY)
					{
						szQuery << "SELECT Value1,Value2,Value3,Value4,Value5,Value6, Date FROM " << dbasetable << " WHERE (DeviceRowID==" << idx << " AND Date>='" << szDateStart << "' AND Date<='" << szDateEnd << "') ORDER BY Date ASC";
						result = m_sql.query(szQuery.str());
						if (result.size() > 0)
						{
							//CM180i
							int displaytype = 0;
							int voltage = 230;
							m_sql.GetPreferencesVar("CM113DisplayType", displaytype);
							m_sql.GetPreferencesVar("ElectricVoltage", voltage);

							root["displaytype"] = displaytype;

							bool bHaveL1 = false;
							bool bHaveL2 = false;
							bool bHaveL3 = false;
							std::vector<std::vector<std::string> >::const_iterator itt;
							for (itt = result.begin(); itt != result.end(); ++itt)
							{
								std::vector<std::string> sd = *itt;

								root["result"][ii]["d"] = sd[6].substr(0, 16);

								float fval1 = static_cast<float>(atof(sd[0].c_str()) / 10.0f);
								float fval2 = static_cast<float>(atof(sd[1].c_str()) / 10.0f);
								float fval3 = static_cast<float>(atof(sd[2].c_str()) / 10.0f);
								float fval4 = static_cast<float>(atof(sd[3].c_str()) / 10.0f);
								float fval5 = static_cast<float>(atof(sd[4].c_str()) / 10.0f);
								float fval6 = static_cast<float>(atof(sd[5].c_str()) / 10.0f);

								if ((fval1 != 0) || (fval2 != 0))
									bHaveL1 = true;
								if ((fval3 != 0) || (fval4 != 0))
									bHaveL2 = true;
								if ((fval5 != 0) || (fval6 != 0))
									bHaveL3 = true;

								if (displaytype == 0)
								{
									sprintf(szTmp, "%.1f", fval1);
									root["result"][ii]["v1"] = szTmp;
									sprintf(szTmp, "%.1f", fval2);
									root["result"][ii]["v2"] = szTmp;
									sprintf(szTmp, "%.1f", fval3);
									root["result"][ii]["v3"] = szTmp;
									sprintf(szTmp, "%.1f", fval4);
									root["result"][ii]["v4"] = szTmp;
									sprintf(szTmp, "%.1f", fval5);
									root["result"][ii]["v5"] = szTmp;
									sprintf(szTmp, "%.1f", fval6);
									root["result"][ii]["v6"] = szTmp;
								}
								else
								{
									sprintf(szTmp, "%d", int(fval1*voltage));
									root["result"][ii]["v1"] = szTmp;
									sprintf(szTmp, "%d", int(fval2*voltage));
									root["result"][ii]["v2"] = szTmp;
									sprintf(szTmp, "%d", int(fval3*voltage));
									root["result"][ii]["v3"] = szTmp;
									sprintf(szTmp, "%d", int(fval4*voltage));
									root["result"][ii]["v4"] = szTmp;
									sprintf(szTmp, "%d", int(fval5*voltage));
									root["result"][ii]["v5"] = szTmp;
									sprintf(szTmp, "%d", int(fval6*voltage));
									root["result"][ii]["v6"] = szTmp;
								}

								ii++;
							}
							if (
								(!bHaveL1) &&
								(!bHaveL2) &&
								(!bHaveL3)
								) {
								root["haveL1"] = true; //show at least something
							}
							else {
								if (bHaveL1)
									root["haveL1"] = true;
								if (bHaveL2)
									root["haveL2"] = true;
								if (bHaveL3)
									root["haveL3"] = true;
							}
						}
					}
					else
					{
						if (dType == pTypeP1Gas)
						{
							//Add last counter value
							sprintf(szTmp, "%.3f", atof(sValue.c_str()) / 1000.0);
							root["counter"] = szTmp;
						}
						else if (dType == pTypeENERGY)
						{
							size_t spos = sValue.find(";");
							if (spos != std::string::npos)
							{
								sprintf(szTmp, "%.3f", atof(sValue.substr(spos + 1).c_str()));
								root["counter"] = szTmp;
							}
						}
						else if (dType == pTypeRFXMeter)
						{
							//Add last counter value
							float fvalue = static_cast<float>(atof(sValue.c_str()));
							switch (metertype)
							{
							case MTYPE_ENERGY:
								sprintf(szTmp, "%.3f", fvalue / EnergyDivider);
								break;
							case MTYPE_GAS:
								sprintf(szTmp, "%.2f", fvalue / GasDivider);
								break;
							case MTYPE_WATER:
								sprintf(szTmp, "%.2f", fvalue / WaterDivider);
								break;
							}
							root["counter"] = szTmp;
						}
						else if (dType == pTypeYouLess)
						{
							std::vector<std::string> results;
							StringSplit(sValue, ";", results);
							if (results.size() == 2)
							{
								//Add last counter value
								float fvalue = static_cast<float>(atof(results[0].c_str()));
								switch (metertype)
								{
								case MTYPE_ENERGY:
									sprintf(szTmp, "%.3f", fvalue / EnergyDivider);
									break;
								case MTYPE_GAS:
									sprintf(szTmp, "%.2f", fvalue / GasDivider);
									break;
								case MTYPE_WATER:
									sprintf(szTmp, "%.2f", fvalue / WaterDivider);
									break;
								}
								root["counter"] = szTmp;
							}
						}
						//Actual Year
						szQuery << "SELECT Value, Date, Counter FROM " << dbasetable << " WHERE (DeviceRowID==" << idx << " AND Date>='" << szDateStart << "' AND Date<='" << szDateEnd << "') ORDER BY Date ASC";
						result = m_sql.query(szQuery.str());
						if (result.size() > 0)
						{
							std::vector<std::vector<std::string> >::const_iterator itt;
							for (itt = result.begin(); itt != result.end(); ++itt)
							{
								std::vector<std::string> sd = *itt;

								root["result"][ii]["d"] = sd[1].substr(0, 16);

								std::string szValue = sd[0];

								double fcounter = atof(sd[2].c_str());

								switch (metertype)
								{
								case MTYPE_ENERGY:
									sprintf(szTmp, "%.3f", atof(szValue.c_str()) / EnergyDivider);
									root["result"][ii]["v"] = szTmp;
									if (fcounter != 0)
										sprintf(szTmp, "%.3f", (fcounter - atof(szValue.c_str())) / EnergyDivider);
									else
										strcpy(szTmp, "0");
									root["result"][ii]["c"] = szTmp;
									break;
								case MTYPE_GAS:
									sprintf(szTmp, "%.2f", atof(szValue.c_str()) / GasDivider);
									root["result"][ii]["v"] = szTmp;
									if (fcounter != 0)
										sprintf(szTmp, "%.2f", (fcounter - atof(szValue.c_str())) / GasDivider);
									else
										strcpy(szTmp, "0");
									root["result"][ii]["c"] = szTmp;
									break;
								case MTYPE_WATER:
									sprintf(szTmp, "%.2f", atof(szValue.c_str()) / WaterDivider);
									root["result"][ii]["v"] = szTmp;
									if (fcounter != 0)
										sprintf(szTmp, "%.2f", (fcounter - atof(szValue.c_str())) / WaterDivider);
									else
										strcpy(szTmp, "0");
									root["result"][ii]["c"] = szTmp;
									break;
								}
								ii++;
							}
						}
						//Past Year
						szQuery.clear();
						szQuery.str("");
						szQuery << "SELECT Value, Date, Counter FROM " << dbasetable << " WHERE (DeviceRowID==" << idx << " AND Date>='" << szDateStartPrev << "' AND Date<='" << szDateEndPrev << "') ORDER BY Date ASC";
						result = m_sql.query(szQuery.str());
						if (result.size() > 0)
						{
							std::vector<std::vector<std::string> >::const_iterator itt;
							for (itt = result.begin(); itt != result.end(); ++itt)
							{
								std::vector<std::string> sd = *itt;

								root["resultprev"][iPrev]["d"] = sd[1].substr(0, 16);

								std::string szValue = sd[0];
								switch (metertype)
								{
								case MTYPE_ENERGY:
									sprintf(szTmp, "%.3f", atof(szValue.c_str()) / EnergyDivider);
									root["resultprev"][iPrev]["v"] = szTmp;
									break;
								case MTYPE_GAS:
									sprintf(szTmp, "%.2f", atof(szValue.c_str()) / GasDivider);
									root["resultprev"][iPrev]["v"] = szTmp;
									break;
								case MTYPE_WATER:
									sprintf(szTmp, "%.2f", atof(szValue.c_str()) / WaterDivider);
									root["resultprev"][iPrev]["v"] = szTmp;
									break;
								}
								iPrev++;
							}
						}
					}
					//add today (have to calculate it)

					if ((sactmonth != "") || (sactyear != ""))
					{
						struct tm loctime;
						time_t now = mytime(NULL);
						localtime_r(&now, &loctime);
						if ((sactmonth != "") && (sactyear != ""))
						{
							bool bIsThisMonth = (atoi(sactyear.c_str()) == loctime.tm_year + 1900) && (atoi(sactmonth.c_str()) == loctime.tm_mon + 1);
							if (bIsThisMonth)
							{
								sprintf(szDateEnd, "%04d-%02d-%02d", loctime.tm_year + 1900, loctime.tm_mon + 1, loctime.tm_mday);
							}
						}
						else if (sactyear != "")
						{
							bool bIsThisYear = (atoi(sactyear.c_str()) == loctime.tm_year + 1900);
							if (bIsThisYear)
							{
								sprintf(szDateEnd, "%04d-%02d-%02d", loctime.tm_year + 1900, loctime.tm_mon + 1, loctime.tm_mday);
							}

						}
					}

					szQuery.clear();
					szQuery.str("");
					if (dType == pTypeP1Power)
					{
						szQuery << "SELECT MIN(Value1), MAX(Value1), MIN(Value2), MAX(Value2), MIN(Value5), MAX(Value5), MIN(Value6), MAX(Value6) FROM MultiMeter WHERE (DeviceRowID=" << idx << " AND Date>='" << szDateEnd << "')";
						bool bHaveDeliverd = false;
						result = m_sql.query(szQuery.str());
						if (result.size() > 0)
						{
							std::vector<std::string> sd = result[0];
							unsigned long long total_min_usage_1, total_min_usage_2, total_max_usage_1, total_max_usage_2, total_real_usage_1, total_real_usage_2;
							unsigned long long total_min_deliv_1, total_min_deliv_2, total_max_deliv_1, total_max_deliv_2, total_real_deliv_1, total_real_deliv_2;

							std::stringstream s_str1(sd[0]);
							s_str1 >> total_min_usage_1;
							std::stringstream s_str2(sd[1]);
							s_str2 >> total_max_usage_1;
							std::stringstream s_str3(sd[4]);
							s_str3 >> total_min_usage_2;
							std::stringstream s_str4(sd[5]);
							s_str4 >> total_max_usage_2;

							total_real_usage_1 = total_max_usage_1 - total_min_usage_1;
							total_real_usage_2 = total_max_usage_2 - total_min_usage_2;

							std::stringstream s_str5(sd[2]);
							s_str5 >> total_min_deliv_1;
							std::stringstream s_str6(sd[3]);
							s_str6 >> total_max_deliv_1;
							std::stringstream s_str7(sd[6]);
							s_str7 >> total_min_deliv_2;
							std::stringstream s_str8(sd[7]);
							s_str8 >> total_max_deliv_2;

							total_real_deliv_1 = total_max_deliv_1 - total_min_deliv_1;
							total_real_deliv_2 = total_max_deliv_2 - total_min_deliv_2;

							if ((total_real_deliv_1 != 0) || (total_real_deliv_2 != 0))
								bHaveDeliverd = true;

							root["result"][ii]["d"] = szDateEnd;

							std::string szValue;

							sprintf(szTmp, "%llu", total_real_usage_1);
							szValue = szTmp;
							sprintf(szTmp, "%.3f", atof(szValue.c_str()) / EnergyDivider);
							root["result"][ii]["v"] = szTmp;
							sprintf(szTmp, "%llu", total_real_usage_2);
							szValue = szTmp;
							sprintf(szTmp, "%.3f", atof(szValue.c_str()) / EnergyDivider);
							root["result"][ii]["v2"] = szTmp;

							sprintf(szTmp, "%llu", total_real_deliv_1);
							szValue = szTmp;
							sprintf(szTmp, "%.3f", atof(szValue.c_str()) / EnergyDivider);
							root["result"][ii]["r1"] = szTmp;
							sprintf(szTmp, "%llu", total_real_deliv_2);
							szValue = szTmp;
							sprintf(szTmp, "%.3f", atof(szValue.c_str()) / EnergyDivider);
							root["result"][ii]["r2"] = szTmp;

							ii++;
						}
						if (bHaveDeliverd)
						{
							root["delivered"] = true;
						}
					}
					else if (dType == pTypeAirQuality)
					{
						szQuery << "SELECT MIN(Value), MAX(Value) FROM Meter WHERE (DeviceRowID=" << idx << " AND Date>='" << szDateEnd << "')";
						result = m_sql.query(szQuery.str());
						if (result.size() > 0)
						{
							root["result"][ii]["d"] = szDateEnd;
							root["result"][ii]["co2_min"] = result[0][0];
							root["result"][ii]["co2_max"] = result[0][1];
							ii++;
						}
					}
					else if (
						((dType == pTypeGeneral) && ((dSubType == sTypeSoilMoisture) || (dSubType == sTypeLeafWetness))) ||
						((dType == pTypeRFXSensor) && ((dSubType == sTypeRFXSensorAD) || (dSubType == sTypeRFXSensorVolt)))
						)
					{
						szQuery << "SELECT MIN(Value), MAX(Value) FROM Meter WHERE (DeviceRowID=" << idx << " AND Date>='" << szDateEnd << "')";
						result = m_sql.query(szQuery.str());
						if (result.size() > 0)
						{
							root["result"][ii]["d"] = szDateEnd;
							root["result"][ii]["v_min"] = result[0][0];
							root["result"][ii]["v_max"] = result[0][1];
							ii++;
						}
					}
					else if (
						((dType == pTypeGeneral) && (dSubType == sTypeVisibility)) ||
						((dType == pTypeGeneral) && (dSubType == sTypeDistance)) ||
						((dType == pTypeGeneral) && (dSubType == sTypeSolarRadiation)) ||
						((dType == pTypeGeneral) && (dSubType == sTypeVoltage)) ||
						((dType == pTypeGeneral) && (dSubType == sTypeCurrent)) ||
						((dType == pTypeGeneral) && (dSubType == sTypePressure)) ||
						((dType == pTypeGeneral) && (dSubType == sTypeSoundLevel))
						)
					{
						float vdiv = 10.0f;
						if (
							((dType == pTypeGeneral) && (dSubType == sTypeVoltage)) ||
							((dType == pTypeGeneral) && (dSubType == sTypeCurrent))
							)
						{
							vdiv = 1000.0f;
						}

						szQuery << "SELECT MIN(Value), MAX(Value) FROM Meter WHERE (DeviceRowID=" << idx << " AND Date>='" << szDateEnd << "')";
						result = m_sql.query(szQuery.str());
						if (result.size() > 0)
						{
							root["result"][ii]["d"] = szDateEnd;
							float fValue1 = float(atof(result[0][0].c_str())) / vdiv;
							float fValue2 = float(atof(result[0][1].c_str())) / vdiv;
							if (metertype == 1)
							{
								fValue1 *= 0.6214f;
								fValue2 *= 0.6214f;
							}

							if ((dType == pTypeGeneral) && (dSubType == sTypeVoltage))
								sprintf(szTmp, "%.3f", fValue1);
							else if ((dType == pTypeGeneral) && (dSubType == sTypeCurrent))
								sprintf(szTmp, "%.3f", fValue1);
							else
								sprintf(szTmp, "%.1f", fValue1);
							root["result"][ii]["v_min"] = szTmp;
							if ((dType == pTypeGeneral) && (dSubType == sTypeVoltage))
								sprintf(szTmp, "%.3f", fValue2);
							else if ((dType == pTypeGeneral) && (dSubType == sTypeCurrent))
								sprintf(szTmp, "%.3f", fValue2);
							else
								sprintf(szTmp, "%.1f", fValue2);
							root["result"][ii]["v_max"] = szTmp;
							ii++;
						}
					}
					else if (dType == pTypeLux)
					{
						szQuery << "SELECT MIN(Value), MAX(Value) FROM Meter WHERE (DeviceRowID=" << idx << " AND Date>='" << szDateEnd << "')";
						result = m_sql.query(szQuery.str());
						if (result.size() > 0)
						{
							root["result"][ii]["d"] = szDateEnd;
							root["result"][ii]["lux_min"] = result[0][0];
							root["result"][ii]["lux_max"] = result[0][1];
							ii++;
						}
					}
					else if (dType == pTypeWEIGHT)
					{
						szQuery << "SELECT MIN(Value), MAX(Value) FROM Meter WHERE (DeviceRowID=" << idx << " AND Date>='" << szDateEnd << "')";
						result = m_sql.query(szQuery.str());
						if (result.size() > 0)
						{
							root["result"][ii]["d"] = szDateEnd;
							sprintf(szTmp, "%.1f", atof(result[0][0].c_str()) / 10.0f);
							root["result"][ii]["v_min"] = szTmp;
							sprintf(szTmp, "%.1f", atof(result[0][1].c_str()) / 10.0f);
							root["result"][ii]["v_max"] = szTmp;
							ii++;
						}
					}
					else if (dType == pTypeUsage)
					{
						szQuery << "SELECT MIN(Value), MAX(Value) FROM Meter WHERE (DeviceRowID=" << idx << " AND Date>='" << szDateEnd << "')";
						result = m_sql.query(szQuery.str());
						if (result.size() > 0)
						{
							root["result"][ii]["d"] = szDateEnd;
							root["result"][ii]["u_min"] = atof(result[0][0].c_str()) / 10.0f;
							root["result"][ii]["u_max"] = atof(result[0][1].c_str()) / 10.0f;
							ii++;
						}
					}
					else
					{
						szQuery << "SELECT MIN(Value), MAX(Value) FROM Meter WHERE (DeviceRowID=" << idx << " AND Date>='" << szDateEnd << "')";
						result = m_sql.query(szQuery.str());
						if (result.size() > 0)
						{
							std::vector<std::string> sd = result[0];
							unsigned long long total_min, total_max, total_real;

							std::stringstream s_str1(sd[0]);
							s_str1 >> total_min;
							std::stringstream s_str2(sd[1]);
							s_str2 >> total_max;
							total_real = total_max - total_min;
							sprintf(szTmp, "%llu", total_real);

							root["result"][ii]["d"] = szDateEnd;

							std::string szValue = szTmp;
							switch (metertype)
							{
							case MTYPE_ENERGY:
								sprintf(szTmp, "%.3f", atof(szValue.c_str()) / EnergyDivider);
								root["result"][ii]["v"] = szTmp;
								sprintf(szTmp, "%.3f", (atof(sValue.c_str()) - atof(szValue.c_str())) / EnergyDivider);
								root["result"][ii]["c"] = szTmp;
								break;
							case MTYPE_GAS:
								sprintf(szTmp, "%.2f", atof(szValue.c_str()) / GasDivider);
								root["result"][ii]["v"] = szTmp;
								sprintf(szTmp, "%.2f", (atof(sValue.c_str()) - atof(szValue.c_str())) / GasDivider);
								root["result"][ii]["c"] = szTmp;
								break;
							case MTYPE_WATER:
								sprintf(szTmp, "%.2f", atof(szValue.c_str()) / WaterDivider);
								root["result"][ii]["v"] = szTmp;
								sprintf(szTmp, "%.2f", (atof(sValue.c_str()) - atof(szValue.c_str())) / WaterDivider);
								root["result"][ii]["c"] = szTmp;
								break;
							}
							ii++;
						}
					}
				}
				else if (sensor == "wind") {
					root["status"] = "OK";
					root["title"] = "Graph " + sensor + " " + srange;

					int ii = 0;

					szQuery.clear();
					szQuery.str("");
					szQuery << "SELECT Direction, Speed_Min, Speed_Max, Gust_Min, Gust_Max, Date FROM " << dbasetable << " WHERE (DeviceRowID==" << idx << " AND Date>='" << szDateStart << "' AND Date<='" << szDateEnd << "') ORDER BY Date ASC";
					result = m_sql.query(szQuery.str());
					if (result.size() > 0)
					{
						std::vector<std::vector<std::string> >::const_iterator itt;
						for (itt = result.begin(); itt != result.end(); ++itt)
						{
							std::vector<std::string> sd = *itt;

							root["result"][ii]["d"] = sd[5].substr(0, 16);
							root["result"][ii]["di"] = sd[0];

							int intSpeed = atoi(sd[2].c_str());
							sprintf(szTmp, "%.1f", float(intSpeed) * m_sql.m_windscale);
							root["result"][ii]["sp"] = szTmp;
							int intGust = atoi(sd[4].c_str());
							sprintf(szTmp, "%.1f", float(intGust) * m_sql.m_windscale);
							root["result"][ii]["gu"] = szTmp;
							ii++;
						}
					}
					//add today (have to calculate it)
					szQuery.clear();
					szQuery.str("");
					szQuery << "SELECT AVG(Direction), MIN(Speed), MAX(Speed), MIN(Gust), MAX(Gust) FROM Wind WHERE (DeviceRowID==" << idx << " AND Date>='" << szDateEnd << "') ORDER BY Date ASC";
					result = m_sql.query(szQuery.str());
					if (result.size() > 0)
					{
						std::vector<std::string> sd = result[0];

						root["result"][ii]["d"] = szDateEnd;
						root["result"][ii]["di"] = sd[0];

						int intSpeed = atoi(sd[2].c_str());
						sprintf(szTmp, "%.1f", float(intSpeed) * m_sql.m_windscale);
						root["result"][ii]["sp"] = szTmp;
						int intGust = atoi(sd[4].c_str());
						sprintf(szTmp, "%.1f", float(intGust) * m_sql.m_windscale);
						root["result"][ii]["gu"] = szTmp;
						ii++;
					}
				}
			}//month or year
			else if ((srange.substr(0, 1) == "2") && (srange.substr(10, 1) == "T") && (srange.substr(11, 1) == "2")) // custom range 2013-01-01T2013-12-31
			{
				std::string szDateStart = srange.substr(0, 10);
				std::string szDateEnd = srange.substr(11, 10);
				std::string sgraphtype = m_pWebEm->FindValue("graphtype");
				std::string sgraphTemp = m_pWebEm->FindValue("graphTemp");
				std::string sgraphChill = m_pWebEm->FindValue("graphChill");
				std::string sgraphHum = m_pWebEm->FindValue("graphHum");
				std::string sgraphBaro = m_pWebEm->FindValue("graphBaro");
				std::string sgraphDew = m_pWebEm->FindValue("graphDew");
				std::string sgraphSet = m_pWebEm->FindValue("graphSet");

				if (sensor == "temp") {
					root["status"] = "OK";
					root["title"] = "Graph " + sensor + " " + srange;

					bool sendTemp = false;
					bool sendChill = false;
					bool sendHum = false;
					bool sendBaro = false;
					bool sendDew = false;
					bool sendSet = false;

					if ((sgraphTemp == "true") &&
						((dType == pTypeRego6XXTemp) || (dType == pTypeTEMP) || (dType == pTypeTEMP_HUM) || (dType == pTypeTEMP_HUM_BARO) || (dType == pTypeTEMP_BARO) || (dType == pTypeWIND) || (dType == pTypeThermostat1) ||
						((dType == pTypeUV) && (dSubType == sTypeUV3)) ||
						((dType == pTypeWIND) && (dSubType == sTypeWIND4)) ||
						((dType == pTypeWIND) && (dSubType == sTypeWINDNoTemp)) ||
						((dType == pTypeRFXSensor) && (dSubType == sTypeRFXSensorTemp)) ||
						((dType == pTypeThermostat) && (dSubType == sTypeThermSetpoint)) ||
						(dType == pTypeEvohomeZone) || (dType == pTypeEvohomeWater)
						)
						)
					{
						sendTemp = true;
					}
					if ((sgraphSet == "true") &&
						((dType == pTypeEvohomeZone) || (dType == pTypeEvohomeWater))) //FIXME cheat for water setpoint is just on or off
					{
						sendSet = true;
					}
					if ((sgraphChill == "true") &&
						(((dType == pTypeWIND) && (dSubType == sTypeWIND4)) ||
						((dType == pTypeWIND) && (dSubType == sTypeWINDNoTemp)))
						)
					{
						sendChill = true;
					}
					if ((sgraphHum == "true") &&
						((dType == pTypeHUM) || (dType == pTypeTEMP_HUM) || (dType == pTypeTEMP_HUM_BARO))
						)
					{
						sendHum = true;
					}
					if ((sgraphBaro == "true") && (
						(dType == pTypeTEMP_HUM_BARO) || 
						(dType == pTypeTEMP_BARO) ||
						((dType == pTypeGeneral) && (dSubType == sTypeBaro))
						))
					{
						sendBaro = true;
					}
					if ((sgraphDew == "true") && ((dType == pTypeTEMP_HUM) || (dType == pTypeTEMP_HUM_BARO)))
					{
						sendDew = true;
					}

					szQuery.clear();
					szQuery.str("");
					if (sgraphtype == "1")
					{
						// Need to get all values of the end date so 23:59:59 is appended to the date string
						szQuery << "SELECT Temperature, Chill, Humidity, Barometer, Date, DewPoint, SetPoint FROM Temperature WHERE (DeviceRowID==" << idx << " AND Date>='" << szDateStart << "' AND Date<='" << szDateEnd << " 23:59:59') ORDER BY Date ASC";
						result = m_sql.query(szQuery.str());
						int ii = 0;
						if (result.size() > 0)
						{
							std::vector<std::vector<std::string> >::const_iterator itt;
							for (itt = result.begin(); itt != result.end(); ++itt)
							{
								std::vector<std::string> sd = *itt;

								root["result"][ii]["d"] = sd[4];//.substr(0,16);
								if (sendTemp)
								{
									double te = ConvertTemperature(atof(sd[0].c_str()), tempsign);
									double tm = ConvertTemperature(atof(sd[0].c_str()), tempsign);
									root["result"][ii]["te"] = te;
									root["result"][ii]["tm"] = tm;
								}
								if (sendChill)
								{
									double ch = ConvertTemperature(atof(sd[1].c_str()), tempsign);
									double cm = ConvertTemperature(atof(sd[1].c_str()), tempsign);
									root["result"][ii]["ch"] = ch;
									root["result"][ii]["cm"] = cm;
								}
								if (sendHum)
								{
									root["result"][ii]["hu"] = sd[2];
								}
								if (sendBaro)
								{
									if (dType == pTypeTEMP_HUM_BARO)
									{
										if (dSubType == sTypeTHBFloat)
										{
											sprintf(szTmp, "%.1f", atof(sd[3].c_str()) / 10.0f);
											root["result"][ii]["ba"] = szTmp;
										}
										else
											root["result"][ii]["ba"] = sd[3];
									}
									else if (dType == pTypeTEMP_BARO)
									{
										sprintf(szTmp, "%.1f", atof(sd[3].c_str()) / 10.0f);
										root["result"][ii]["ba"] = szTmp;
									}
									else if ((dType == pTypeGeneral) && (dSubType == sTypeBaro))
									{
										sprintf(szTmp, "%.1f", atof(sd[3].c_str()) / 10.0f);
										root["result"][ii]["ba"] = szTmp;
									}
								}
								if (sendDew)
								{
									double dp = ConvertTemperature(atof(sd[5].c_str()), tempsign);
									root["result"][ii]["dp"] = dp;
								}
						if (sendSet)
						{
							double se = ConvertTemperature(atof(sd[6].c_str()), tempsign);
							root["result"][ii]["se"] = se;
						}
								ii++;
							}
						}
					}
					else
					{
						szQuery << "SELECT Temp_Min, Temp_Max, Chill_Min, Chill_Max, Humidity, Barometer, Date, DewPoint, Temp_Avg, SetPoint_Min, SetPoint_Max, SetPoint_Avg FROM Temperature_Calendar WHERE (DeviceRowID==" << idx << " AND Date>='" << szDateStart << "' AND Date<='" << szDateEnd << "') ORDER BY Date ASC";
						result = m_sql.query(szQuery.str());
						int ii = 0;
						if (result.size() > 0)
						{
							std::vector<std::vector<std::string> >::const_iterator itt;
							for (itt = result.begin(); itt != result.end(); ++itt)
							{
								std::vector<std::string> sd = *itt;

								root["result"][ii]["d"] = sd[6].substr(0, 16);
								if (sendTemp)
								{
									double te = ConvertTemperature(atof(sd[1].c_str()), tempsign);
									double tm = ConvertTemperature(atof(sd[0].c_str()), tempsign);
									double ta = ConvertTemperature(atof(sd[8].c_str()), tempsign);

									root["result"][ii]["te"] = te;
									root["result"][ii]["tm"] = tm;
									root["result"][ii]["ta"] = ta;
								}
								if (sendChill)
								{
									double ch = ConvertTemperature(atof(sd[3].c_str()), tempsign);
									double cm = ConvertTemperature(atof(sd[2].c_str()), tempsign);

									root["result"][ii]["ch"] = ch;
									root["result"][ii]["cm"] = cm;
								}
								if (sendHum)
								{
									root["result"][ii]["hu"] = sd[4];
								}
								if (sendBaro)
								{
									if (dType == pTypeTEMP_HUM_BARO)
									{
										if (dSubType == sTypeTHBFloat)
										{
											sprintf(szTmp, "%.1f", atof(sd[5].c_str()) / 10.0f);
											root["result"][ii]["ba"] = szTmp;
										}
										else
											root["result"][ii]["ba"] = sd[5];
									}
									else if (dType == pTypeTEMP_BARO)
									{
										sprintf(szTmp, "%.1f", atof(sd[5].c_str()) / 10.0f);
										root["result"][ii]["ba"] = szTmp;
									}
									else if ((dType == pTypeGeneral) && (dSubType == sTypeBaro))
									{
										sprintf(szTmp, "%.1f", atof(sd[5].c_str()) / 10.0f);
										root["result"][ii]["ba"] = szTmp;
									}
								}
								if (sendDew)
								{
									double dp = ConvertTemperature(atof(sd[7].c_str()), tempsign);
									root["result"][ii]["dp"] = dp;
								}
								if (sendSet)
								{
									double sm = ConvertTemperature(atof(sd[9].c_str()), tempsign);
									double sx = ConvertTemperature(atof(sd[10].c_str()), tempsign);
									double se = ConvertTemperature(atof(sd[11].c_str()), tempsign);
									root["result"][ii]["sm"] = sm;
									root["result"][ii]["se"] = se;
									root["result"][ii]["sx"] = sx;
									char szTmp[1024];
									sprintf(szTmp,"%.1f %.1f %.1f",sm,se,sx);
									_log.Log(LOG_STATUS,szTmp);
									
								}
								ii++;
							}
						}

						//add today (have to calculate it)
						szQuery.clear();
						szQuery.str("");
						szQuery << "SELECT MIN(Temperature), MAX(Temperature), MIN(Chill), MAX(Chill), MAX(Humidity), MAX(Barometer), MIN(DewPoint), AVG(Temperature), MIN(SetPoint), MAX(SetPoint), AVG(SetPoint) FROM Temperature WHERE (DeviceRowID=" << idx << " AND Date>='" << szDateEnd << "')";
						result = m_sql.query(szQuery.str());
						if (result.size() > 0)
						{
							std::vector<std::string> sd = result[0];

							root["result"][ii]["d"] = szDateEnd;
							if (sendTemp)
							{
								double te = ConvertTemperature(atof(sd[1].c_str()), tempsign);
								double tm = ConvertTemperature(atof(sd[0].c_str()), tempsign);
								double ta = ConvertTemperature(atof(sd[7].c_str()), tempsign);

								root["result"][ii]["te"] = te;
								root["result"][ii]["tm"] = tm;
								root["result"][ii]["ta"] = ta;
							}
							if (sendChill)
							{
								double ch = ConvertTemperature(atof(sd[3].c_str()), tempsign);
								double cm = ConvertTemperature(atof(sd[2].c_str()), tempsign);
								root["result"][ii]["ch"] = ch;
								root["result"][ii]["cm"] = cm;
							}
							if (sendHum)
							{
								root["result"][ii]["hu"] = sd[4];
							}
							if (sendBaro)
							{
								if (dType == pTypeTEMP_HUM_BARO)
								{
									if (dSubType == sTypeTHBFloat)
									{
										sprintf(szTmp, "%.1f", atof(sd[5].c_str()) / 10.0f);
										root["result"][ii]["ba"] = szTmp;
									}
									else
										root["result"][ii]["ba"] = sd[5];
								}
								else if (dType == pTypeTEMP_BARO)
								{
									sprintf(szTmp, "%.1f", atof(sd[5].c_str()) / 10.0f);
									root["result"][ii]["ba"] = szTmp;
								}
								else if ((dType == pTypeGeneral) && (dSubType == sTypeBaro))
								{
									sprintf(szTmp, "%.1f", atof(sd[5].c_str()) / 10.0f);
									root["result"][ii]["ba"] = szTmp;
								}
							}
							if (sendDew)
							{
								double dp = ConvertTemperature(atof(sd[6].c_str()), tempsign);
								root["result"][ii]["dp"] = dp;
							}
					if (sendSet)
					{
						double sm = ConvertTemperature(atof(sd[8].c_str()), tempsign);
						double sx = ConvertTemperature(atof(sd[9].c_str()), tempsign);
						double se = ConvertTemperature(atof(sd[10].c_str()), tempsign);
						
						root["result"][ii]["sm"] = sm;
						root["result"][ii]["se"] = se;
						root["result"][ii]["sx"] = sx;
					}
							ii++;
						}
					}
				}
				else if (sensor == "uv") {
					root["status"] = "OK";
					root["title"] = "Graph " + sensor + " " + srange;

					szQuery.clear();
					szQuery.str("");
					szQuery << "SELECT Level, Date FROM " << dbasetable << " WHERE (DeviceRowID==" << idx << " AND Date>='" << szDateStart << "' AND Date<='" << szDateEnd << "') ORDER BY Date ASC";
					result = m_sql.query(szQuery.str());
					int ii = 0;
					if (result.size() > 0)
					{
						std::vector<std::vector<std::string> >::const_iterator itt;
						for (itt = result.begin(); itt != result.end(); ++itt)
						{
							std::vector<std::string> sd = *itt;

							root["result"][ii]["d"] = sd[1].substr(0, 16);
							root["result"][ii]["uvi"] = sd[0];
							ii++;
						}
					}
					//add today (have to calculate it)
					szQuery.clear();
					szQuery.str("");
					szQuery << "SELECT MAX(Level) FROM UV WHERE (DeviceRowID=" << idx << " AND Date>='" << szDateEnd << "')";
					result = m_sql.query(szQuery.str());
					if (result.size() > 0)
					{
						std::vector<std::string> sd = result[0];

						root["result"][ii]["d"] = szDateEnd;
						root["result"][ii]["uvi"] = sd[0];
						ii++;
					}
				}
				else if (sensor == "rain") {
					root["status"] = "OK";
					root["title"] = "Graph " + sensor + " " + srange;

					szQuery.clear();
					szQuery.str("");
					szQuery << "SELECT Total, Rate, Date FROM " << dbasetable << " WHERE (DeviceRowID==" << idx << " AND Date>='" << szDateStart << "' AND Date<='" << szDateEnd << "') ORDER BY Date ASC";
					result = m_sql.query(szQuery.str());
					int ii = 0;
					if (result.size() > 0)
					{
						std::vector<std::vector<std::string> >::const_iterator itt;
						for (itt = result.begin(); itt != result.end(); ++itt)
						{
							std::vector<std::string> sd = *itt;

							root["result"][ii]["d"] = sd[2].substr(0, 16);
							root["result"][ii]["mm"] = sd[0];
							ii++;
						}
					}
					//add today (have to calculate it)
					szQuery.clear();
					szQuery.str("");
					if (dSubType != sTypeRAINWU)
					{
						szQuery << "SELECT MIN(Total), MAX(Total), MAX(Rate) FROM Rain WHERE (DeviceRowID=" << idx << " AND Date>='" << szDateEnd << "')";
					}
					else
					{
						szQuery << "SELECT Total, Total, Rate FROM Rain WHERE (DeviceRowID=" << idx << " AND Date>='" << szDateEnd << "') ORDER BY ROWID DESC LIMIT 1";
					}
					result = m_sql.query(szQuery.str());
					if (result.size() > 0)
					{
						std::vector<std::string> sd = result[0];

						float total_min = static_cast<float>(atof(sd[0].c_str()));
						float total_max = static_cast<float>(atof(sd[1].c_str()));
						int rate = atoi(sd[2].c_str());

						float total_real = 0;
						if (dSubType != sTypeRAINWU)
						{
							total_real = total_max - total_min;
						}
						else
						{
							total_real = total_max;
						}
						sprintf(szTmp, "%.1f", total_real);
						root["result"][ii]["d"] = szDateEnd;
						root["result"][ii]["mm"] = szTmp;
						ii++;
					}
				}
				else if (sensor == "counter") {
					root["status"] = "OK";
					root["title"] = "Graph " + sensor + " " + srange;

					float EnergyDivider = 1000.0f;
					float GasDivider = 100.0f;
					float WaterDivider = 100.0f;
					int tValue;
					if (m_sql.GetPreferencesVar("MeterDividerEnergy", tValue))
					{
						EnergyDivider = float(tValue);
					}
					if (m_sql.GetPreferencesVar("MeterDividerGas", tValue))
					{
						GasDivider = float(tValue);
					}
					if (m_sql.GetPreferencesVar("MeterDividerWater", tValue))
					{
						WaterDivider = float(tValue);
					}
					if (dType == pTypeP1Gas)
						GasDivider = 1000.0f;
					else if ((dType == pTypeENERGY) || (dType == pTypePOWER))
						EnergyDivider *= 100.0f;

					szQuery.clear();
					szQuery.str("");
					int ii = 0;
					if (dType == pTypeP1Power)
					{
						szQuery << "SELECT Value1,Value2,Value5,Value6, Date FROM " << dbasetable << " WHERE (DeviceRowID==" << idx << " AND Date>='" << szDateStart << "' AND Date<='" << szDateEnd << "') ORDER BY Date ASC";
						result = m_sql.query(szQuery.str());
						if (result.size() > 0)
						{
							bool bHaveDeliverd = false;
							std::vector<std::vector<std::string> >::const_iterator itt;
							for (itt = result.begin(); itt != result.end(); ++itt)
							{
								std::vector<std::string> sd = *itt;

								root["result"][ii]["d"] = sd[4].substr(0, 16);

								std::string szUsage1 = sd[0];
								std::string szDeliv1 = sd[1];
								std::string szUsage2 = sd[2];
								std::string szDeliv2 = sd[3];

								float fUsage = (float)(atof(szUsage1.c_str()) + atof(szUsage2.c_str()));
								float fDeliv = (float)(atof(szDeliv1.c_str()) + atof(szDeliv2.c_str()));

								if (fDeliv != 0)
									bHaveDeliverd = true;
								sprintf(szTmp, "%.3f", fUsage / EnergyDivider);
								root["result"][ii]["v"] = szTmp;
								sprintf(szTmp, "%.3f", fDeliv / EnergyDivider);
								root["result"][ii]["v2"] = szTmp;
								ii++;
							}
							if (bHaveDeliverd)
							{
								root["delivered"] = true;
							}
						}
					}
					else
					{
						szQuery << "SELECT Value, Date FROM " << dbasetable << " WHERE (DeviceRowID==" << idx << " AND Date>='" << szDateStart << "' AND Date<='" << szDateEnd << "') ORDER BY Date ASC";
						result = m_sql.query(szQuery.str());
						if (result.size() > 0)
						{
							std::vector<std::vector<std::string> >::const_iterator itt;
							for (itt = result.begin(); itt != result.end(); ++itt)
							{
								std::vector<std::string> sd = *itt;

								std::string szValue = sd[0];
								switch (metertype)
								{
								case MTYPE_ENERGY:
									sprintf(szTmp, "%.3f", atof(szValue.c_str()) / EnergyDivider);
									szValue = szTmp;
									break;
								case MTYPE_GAS:
									sprintf(szTmp, "%.2f", atof(szValue.c_str()) / GasDivider);
									szValue = szTmp;
									break;
								case MTYPE_WATER:
									sprintf(szTmp, "%.2f", atof(szValue.c_str()) / WaterDivider);
									szValue = szTmp;
									break;
								}
								root["result"][ii]["d"] = sd[1].substr(0, 16);
								root["result"][ii]["v"] = szValue;
								ii++;
							}
						}
					}
					//add today (have to calculate it)
					szQuery.clear();
					szQuery.str("");
					if (dType == pTypeP1Power)
					{
						szQuery << "SELECT MIN(Value1), MAX(Value1), MIN(Value2), MAX(Value2),MIN(Value5), MAX(Value5), MIN(Value6), MAX(Value6) FROM MultiMeter WHERE (DeviceRowID=" << idx << " AND Date>='" << szDateEnd << "')";
						bool bHaveDeliverd = false;
						result = m_sql.query(szQuery.str());
						if (result.size() > 0)
						{
							std::vector<std::string> sd = result[0];
							unsigned long long total_min_usage_1, total_min_usage_2, total_max_usage_1, total_max_usage_2, total_real_usage;
							unsigned long long total_min_deliv_1, total_min_deliv_2, total_max_deliv_1, total_max_deliv_2, total_real_deliv;

							std::stringstream s_str1(sd[0]);
							s_str1 >> total_min_usage_1;
							std::stringstream s_str2(sd[1]);
							s_str2 >> total_max_usage_1;
							std::stringstream s_str3(sd[4]);
							s_str3 >> total_min_usage_2;
							std::stringstream s_str4(sd[5]);
							s_str4 >> total_max_usage_2;

							total_real_usage = (total_max_usage_1 + total_max_usage_2) - (total_min_usage_1 + total_min_usage_2);

							std::stringstream s_str5(sd[2]);
							s_str5 >> total_min_deliv_1;
							std::stringstream s_str6(sd[3]);
							s_str6 >> total_max_deliv_1;
							std::stringstream s_str7(sd[6]);
							s_str7 >> total_min_deliv_2;
							std::stringstream s_str8(sd[7]);
							s_str8 >> total_max_deliv_2;

							total_real_deliv = (total_max_deliv_1 + total_max_deliv_2) - (total_min_deliv_1 + total_min_deliv_2);

							if (total_real_deliv != 0)
								bHaveDeliverd = true;

							root["result"][ii]["d"] = szDateEnd;

							sprintf(szTmp, "%llu", total_real_usage);
							std::string szValue = szTmp;
							sprintf(szTmp, "%.3f", atof(szValue.c_str()) / EnergyDivider);
							root["result"][ii]["v"] = szTmp;
							sprintf(szTmp, "%llu", total_real_deliv);
							szValue = szTmp;
							sprintf(szTmp, "%.3f", atof(szValue.c_str()) / EnergyDivider);
							root["result"][ii]["v2"] = szTmp;
							ii++;
							if (bHaveDeliverd)
							{
								root["delivered"] = true;
							}
						}
					}
					else
					{
						szQuery << "SELECT MIN(Value), MAX(Value) FROM Meter WHERE (DeviceRowID=" << idx << " AND Date>='" << szDateEnd << "')";
						result = m_sql.query(szQuery.str());
						if (result.size() > 0)
						{
							std::vector<std::string> sd = result[0];
							unsigned long long total_min, total_max, total_real;

							std::stringstream s_str1(sd[0]);
							s_str1 >> total_min;
							std::stringstream s_str2(sd[1]);
							s_str2 >> total_max;
							total_real = total_max - total_min;
							sprintf(szTmp, "%llu", total_real);

							std::string szValue = szTmp;
							switch (metertype)
							{
							case MTYPE_ENERGY:
								sprintf(szTmp, "%.3f", atof(szValue.c_str()) / EnergyDivider);
								szValue = szTmp;
								break;
							case MTYPE_GAS:
								sprintf(szTmp, "%.2f", atof(szValue.c_str()) / GasDivider);
								szValue = szTmp;
								break;
							case MTYPE_WATER:
								sprintf(szTmp, "%.2f", atof(szValue.c_str()) / WaterDivider);
								szValue = szTmp;
								break;
							}

							root["result"][ii]["d"] = szDateEnd;
							root["result"][ii]["v"] = szValue;
							ii++;
						}
					}
				}
				else if (sensor == "wind") {
					root["status"] = "OK";
					root["title"] = "Graph " + sensor + " " + srange;

					int ii = 0;

					szQuery.clear();
					szQuery.str("");
					szQuery << "SELECT Direction, Speed_Min, Speed_Max, Gust_Min, Gust_Max, Date FROM " << dbasetable << " WHERE (DeviceRowID==" << idx << " AND Date>='" << szDateStart << "' AND Date<='" << szDateEnd << "') ORDER BY Date ASC";
					result = m_sql.query(szQuery.str());
					if (result.size() > 0)
					{
						std::vector<std::vector<std::string> >::const_iterator itt;
						for (itt = result.begin(); itt != result.end(); ++itt)
						{
							std::vector<std::string> sd = *itt;

							root["result"][ii]["d"] = sd[5].substr(0, 16);
							root["result"][ii]["di"] = sd[0];

							int intSpeed = atoi(sd[2].c_str());
							sprintf(szTmp, "%.1f", float(intSpeed) * m_sql.m_windscale);
							root["result"][ii]["sp"] = szTmp;
							int intGust = atoi(sd[4].c_str());
							sprintf(szTmp, "%.1f", float(intGust) * m_sql.m_windscale);
							root["result"][ii]["gu"] = szTmp;
							ii++;
						}
					}
					//add today (have to calculate it)
					szQuery.clear();
					szQuery.str("");
					szQuery << "SELECT AVG(Direction), MIN(Speed), MAX(Speed), MIN(Gust), MAX(Gust) FROM Wind WHERE (DeviceRowID==" << idx << " AND Date>='" << szDateEnd << "') ORDER BY Date ASC";
					result = m_sql.query(szQuery.str());
					if (result.size() > 0)
					{
						std::vector<std::string> sd = result[0];

						root["result"][ii]["d"] = szDateEnd;
						root["result"][ii]["di"] = sd[0];

						int intSpeed = atoi(sd[2].c_str());
						sprintf(szTmp, "%.1f", float(intSpeed) * m_sql.m_windscale);
						root["result"][ii]["sp"] = szTmp;
						int intGust = atoi(sd[4].c_str());
						sprintf(szTmp, "%.1f", float(intGust) * m_sql.m_windscale);
						root["result"][ii]["gu"] = szTmp;
						ii++;
					}
				}
			}//custom range
		}

	} //server
}//http

