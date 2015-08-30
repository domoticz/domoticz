#include "stdafx.h"
#include "WebServer.h"
#include <boost/bind.hpp>
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
#ifdef WITH_OPENZWAVE
#include "../hardware/OpenZWave.h"
#endif
#include "../hardware/EnOceanESP2.h"
#include "../hardware/EnOceanESP3.h"
#include "../hardware/Wunderground.h"
#include "../hardware/ForecastIO.h"
#include "../hardware/Kodi.h"
#ifdef WITH_GPIO
#include "../hardware/Gpio.h"
#include "../hardware/GpioPin.h"
#endif // WITH_GPIO
#include "../webserver/Base64.h"
#include "../smtpclient/SMTPClient.h"
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

extern "C" {
#include "../lua/src/lua.h"    
#include "../lua/src/lualib.h"
#include "../lua/src/lauxlib.h"
}


#include "mainstructs.h"

#define round(a) ( int ) ( a + .5 )

extern std::string szUserDataFolder;
extern std::string szWWWFolder;

extern std::string szAppVersion;
extern std::string szAppHash;
extern std::string szAppDate;

extern bool g_bDontCacheWWW;

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
	{ "da", "Danish" },
	{ "de", "German" },
	{ "el", "Greek" },
	{ "fr", "French" },
	{ "fi", "Finnish" },
	{ "hu", "Hungarian" },
	{ "it", "Italian" },
	{ "lt", "Lithuanian" },
	{ "no", "Norwegian" },
	{ "pl", "Polish" },
	{ "pt", "Portuguese" },
	{ "ru", "Russian" },
	{ "sk", "Slovak" },
	//{ "sl", "Slovenian" },
	{ "es", "Spanish" },
	{ "sv", "Swedish" },
	{ "tr", "Turkish" },

	{ NULL, NULL }
};

namespace http {
	namespace server {

		CWebServer::CWebServer(void)
		{
			m_pWebEm = NULL;
			m_bDoStop = false;
#ifdef WITH_OPENZWAVE
			m_ZW_Hwidx = -1;
#endif
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
			result = m_sql.safe_query("SELECT ID,Base,Name,Description FROM CustomImages");
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
							std::vector<std::vector<std::string> > result2;
							result2 = m_sql.safe_queryBlob("SELECT %s FROM CustomImages WHERE ID=%d", TableField.c_str(), ID);
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

			int tries = 0;
			bool exception = false;
			std::string listen_address = listenaddress;

			do {
				try {
					exception = false;
					m_pWebEm = new http::server::cWebem(
						listen_address.c_str(),						// address
						listenport.c_str(),							// port
						serverpath.c_str(), secure_cert_file, secure_cert_passphrase);
				}
				catch (std::exception& e) {
					exception = true;
					switch (tries) {
					case 0:
						listen_address = "::";
						break;
					case 1:
						listen_address = "0.0.0.0";
						break;
					case 2:
						_log.Log(LOG_ERROR, "Failed to start the web server: %s", e.what());
						if (atoi(listenport.c_str()) < 1024)
							_log.Log(LOG_ERROR, "check privileges for opening ports below 1024");
						else
							_log.Log(LOG_ERROR, "check if no other application is using port: %s", listenport.c_str());
						return false;
					}
					tries++;
				}
			} while (exception);
			if (listen_address != "0.0.0.0" && listen_address != "::")
				_log.Log(LOG_STATUS, "Webserver started on address: %s, port: %s", listen_address.c_str(), listenport.c_str());
			else
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
			m_pWebEm->RegisterIncludeCode("switchtypes", boost::bind(&CWebServer::DisplaySwitchTypesCombo, this));
			m_pWebEm->RegisterIncludeCode("metertypes", boost::bind(&CWebServer::DisplayMeterTypesCombo, this));
			m_pWebEm->RegisterIncludeCode("timertypes", boost::bind(&CWebServer::DisplayTimerTypesCombo, this));
			m_pWebEm->RegisterIncludeCode("combolanguage", boost::bind(&CWebServer::DisplayLanguageCombo, this));

			m_pWebEm->RegisterPageCode("/json.htm", boost::bind(&CWebServer::GetJSonPage, this));
			m_pWebEm->RegisterPageCode("/uploadcustomicon", boost::bind(&CWebServer::Post_UploadCustomIcon, this));
			m_pWebEm->RegisterPageCode("/html5.appcache", boost::bind(&CWebServer::GetAppCache, this));
			m_pWebEm->RegisterPageCode("/camsnapshot.jpg", boost::bind(&CWebServer::GetCameraSnapshot, this));
			m_pWebEm->RegisterPageCode("/backupdatabase.php", boost::bind(&CWebServer::GetDatabaseBackup, this));
			m_pWebEm->RegisterPageCode("/raspberry.cgi", boost::bind(&CWebServer::GetInternalCameraSnapshot, this));
			m_pWebEm->RegisterPageCode("/uvccapture.cgi", boost::bind(&CWebServer::GetInternalCameraSnapshot, this));

			m_pWebEm->RegisterActionCode("storesettings", boost::bind(&CWebServer::PostSettings, this));
			m_pWebEm->RegisterActionCode("setrfxcommode", boost::bind(&CWebServer::SetRFXCOMMode, this));
			m_pWebEm->RegisterActionCode("rfxupgradefirmware", boost::bind(&CWebServer::RFXComUpgradeFirmware, this));
			RegisterCommandCode("rfxfirmwaregetpercentage", boost::bind(&CWebServer::Cmd_RFXComGetFirmwarePercentage, this, _1), true);
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

			RegisterCommandCode("kodisetmode", boost::bind(&CWebServer::Cmd_KodiSetMode, this, _1));
			RegisterCommandCode("kodigetnodes", boost::bind(&CWebServer::Cmd_KodiGetNodes, this, _1));
			RegisterCommandCode("kodiaddnode", boost::bind(&CWebServer::Cmd_KodiAddNode, this, _1));
			RegisterCommandCode("kodiupdatenode", boost::bind(&CWebServer::Cmd_KodiUpdateNode, this, _1));
			RegisterCommandCode("kodiremovenode", boost::bind(&CWebServer::Cmd_KodiRemoveNode, this, _1));
			RegisterCommandCode("kodiclearnodes", boost::bind(&CWebServer::Cmd_KodiClearNodes, this, _1));
			RegisterCommandCode("kodimediacommand", boost::bind(&CWebServer::Cmd_KodiMediaCommand, this, _1));

			RegisterCommandCode("savefibarolinkconfig", boost::bind(&CWebServer::Cmd_SaveFibaroLinkConfig, this, _1));
			RegisterCommandCode("getfibarolinkconfig", boost::bind(&CWebServer::Cmd_GetFibaroLinkConfig, this, _1));
			RegisterCommandCode("getfibarolinks", boost::bind(&CWebServer::Cmd_GetFibaroLinks, this, _1));
			RegisterCommandCode("savefibarolink", boost::bind(&CWebServer::Cmd_SaveFibaroLink, this, _1));
			RegisterCommandCode("deletefibarolink", boost::bind(&CWebServer::Cmd_DeleteFibaroLink, this, _1));

			RegisterCommandCode("savehttplinkconfig", boost::bind(&CWebServer::Cmd_SaveHttpLinkConfig, this, _1));
			RegisterCommandCode("gethttplinkconfig", boost::bind(&CWebServer::Cmd_GetHttpLinkConfig, this, _1));
			RegisterCommandCode("gethttplinks", boost::bind(&CWebServer::Cmd_GetHttpLinks, this, _1));
			RegisterCommandCode("savehttplink", boost::bind(&CWebServer::Cmd_SaveHttpLink, this, _1));
			RegisterCommandCode("deletehttplink", boost::bind(&CWebServer::Cmd_DeleteHttpLink, this, _1));
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
			RegisterCommandCode("udevices", boost::bind(&CWebServer::Cmd_UpdateDevices, this, _1));
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
			RegisterCommandCode("enabletimer", boost::bind(&CWebServer::Cmd_EnableTimer, this, _1));
			RegisterCommandCode("disabletimer", boost::bind(&CWebServer::Cmd_DisableTimer, this, _1));
			RegisterCommandCode("cleartimers", boost::bind(&CWebServer::Cmd_ClearTimers, this, _1));

			RegisterCommandCode("addscenetimer", boost::bind(&CWebServer::Cmd_AddSceneTimer, this, _1));
			RegisterCommandCode("updatescenetimer", boost::bind(&CWebServer::Cmd_UpdateSceneTimer, this, _1));
			RegisterCommandCode("deletescenetimer", boost::bind(&CWebServer::Cmd_DeleteSceneTimer, this, _1));
			RegisterCommandCode("enablescenetimer", boost::bind(&CWebServer::Cmd_EnableSceneTimer, this, _1));
			RegisterCommandCode("disablescenetimer", boost::bind(&CWebServer::Cmd_DisableSceneTimer, this, _1));
			RegisterCommandCode("clearscenetimers", boost::bind(&CWebServer::Cmd_ClearSceneTimers, this, _1));
			RegisterCommandCode("getsceneactivations", boost::bind(&CWebServer::Cmd_GetSceneActivations, this, _1));
			RegisterCommandCode("addscenecode", boost::bind(&CWebServer::Cmd_AddSceneCode, this, _1));
			RegisterCommandCode("removescenecode", boost::bind(&CWebServer::Cmd_RemoveSceneCode, this, _1));
			RegisterCommandCode("clearscenecodes", boost::bind(&CWebServer::Cmd_ClearSceneCodes, this, _1));

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
			RegisterCommandCode("updatecustomicon", boost::bind(&CWebServer::Cmd_UpdateCustomIcon, this, _1));

			RegisterCommandCode("renamedevice", boost::bind(&CWebServer::Cmd_RenameDevice, this, _1));
			RegisterCommandCode("setunused", boost::bind(&CWebServer::Cmd_SetUnused, this, _1));

			RegisterCommandCode("addlogmessage", boost::bind(&CWebServer::Cmd_AddLogMessage, this, _1));

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

			RegisterCommandCode("zwaveisnodeincluded", boost::bind(&CWebServer::Cmd_ZWaveIsNodeIncluded, this, _1));
			RegisterCommandCode("zwaveisnodeexcluded", boost::bind(&CWebServer::Cmd_ZWaveIsNodeExcluded, this, _1));

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
			std::string response = "";
			if (g_bDontCacheWWW)
			{
				return response;
			}
			//Return the appcache file (dynamicly generated)
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
								stdreplace(tfname, "styles/" + sWebTheme, "acttheme");
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
				return;//Only admin user allowed

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
				(htype == HTYPE_ETH8020) || (htype == HTYPE_KMTronicTCP) || (htype == HTYPE_SOLARMAXTCP) || (htype == HTYPE_SatelIntegra)
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
			else if (htype == HTYPE_Kodi) {
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
			else if (
				(htype == HTYPE_Wunderground) || 
				(htype == HTYPE_ForecastIO) || 
				(htype == HTYPE_ICYTHERMOSTAT) || 
				(htype == HTYPE_TOONTHERMOSTAT) || 
				(htype == HTYPE_PVOUTPUT_INPUT) || 
				(htype == HTYPE_NESTTHERMOSTAT) ||
				(htype == HTYPE_ANNATHERMOSTAT) ||
				(htype == HTYPE_THERMOSMART) ||
				(htype == HTYPE_NetatmoWeatherStation)
				)
			{
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
			else if (htype == HTYPE_WINDDELEN) {
					std::string mill_id = m_pWebEm->FindValue("Mode1");
					if (
					(mill_id == "") ||
					(sport == "")
					)
					
						return;
					mode1 = atoi(mill_id.c_str());
			}
			else if (htype == HTYPE_RaspberryGPIO) {
				//all fine here!
			}
			else
				return;

			root["status"] = "OK";
			root["title"] = "AddHardware";

			std::vector<std::vector<std::string> > result;

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
			else if (htype == HTYPE_Kodi)
			{
				mode1 = 30;
				mode2 = 1000;
			}

			m_sql.safe_query(
				"INSERT INTO Hardware (Name, Enabled, Type, Address, Port, SerialPort, Username, Password, Mode1, Mode2, Mode3, Mode4, Mode5, Mode6, DataTimeout) VALUES ('%q',%d, %d,'%q',%d,'%q','%q','%q',%d,%d,%d,%d,%d,%d,%d)",
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
		
			//add the device for real in our system
			result = m_sql.safe_query("SELECT MAX(ID) FROM Hardware");
			if (result.size() > 0)
			{
				std::vector<std::string> sd = result[0];
				int ID = atoi(sd[0].c_str());

				m_mainworker.AddHardwareFromParams(ID, name, (senabled == "true") ? true : false, htype, address, port, sport, username, password, mode1, mode2, mode3, mode4, mode5, mode6, iDataTimeout, true);
			}
		}

		void CWebServer::Cmd_UpdateHardware(Json::Value &root)
		{
			if (m_pWebEm->m_actualuser_rights != 2)
				return;//Only admin user allowed

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
				(htype == HTYPE_KMTronicTCP) || (htype == HTYPE_SOLARMAXTCP) || (htype == HTYPE_SatelIntegra)
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
			else if (htype == HTYPE_Kodi) {
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
			else if (
				(htype == HTYPE_Wunderground) || 
				(htype == HTYPE_ForecastIO) || 
				(htype == HTYPE_ICYTHERMOSTAT) || 
				(htype == HTYPE_TOONTHERMOSTAT) || 
				(htype == HTYPE_PVOUTPUT_INPUT) || 
				(htype == HTYPE_NESTTHERMOSTAT) ||
				(htype == HTYPE_ANNATHERMOSTAT) ||
				(htype == HTYPE_THERMOSMART) ||
				(htype == HTYPE_NetatmoWeatherStation)
				)
			{
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
			else if (htype == HTYPE_WINDDELEN) {
				std::string mill_id = m_pWebEm->FindValue("Mode1");
				if (
				  (mill_id == "") ||
				  (sport == "")
				)
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
				m_sql.safe_query(
					"UPDATE Hardware SET Enabled=%d WHERE (ID == '%q')",
					(bEnabled == true) ? 1 : 0,
					idx.c_str()
					);
			}
			else
			{
				m_sql.safe_query(
					"UPDATE Hardware SET Name='%q', Enabled=%d, Type=%d, Address='%q', Port=%d, SerialPort='%q', Username='%q', Password='%q', Mode1=%d, Mode2=%d, Mode3=%d, Mode4=%d, Mode5=%d, Mode6=%d, DataTimeout=%d WHERE (ID == '%q')",
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
				m_mainworker.AddHardwareFromParams(ID, name, bEnabled, htype, address, port, sport, username, password, mode1, mode2, mode3, mode4, mode5, mode6, iDataTimeout, true);
			}
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
				std::vector<std::vector<std::string> > result;
				result = m_sql.safe_query("SELECT ID FROM UserVariables WHERE Name='%q'",
					variablename.c_str());
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

			std::vector<std::vector<std::string> > result;
			result = m_sql.safe_query("SELECT ID,Name,ValueType,Value,LastUpdate FROM UserVariables WHERE (ID==%d)",
				iVarID);
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
				return;//Only admin user allowed

			std::string idx = m_pWebEm->FindValue("idx");
			if (idx == "")
				return;
			int hwID = atoi(idx.c_str());

			root["status"] = "OK";
			root["title"] = "DeleteHardware";

			m_mainworker.RemoveDomoticzHardware(hwID);
			m_sql.DeleteHardware(idx);
		}

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
				return;//Only admin user allowed

			std::string name = m_pWebEm->FindValue("name");
			root["status"] = "OK";
			root["title"] = "AddPlan";
			m_sql.safe_query(
				"INSERT INTO Plans (Name) VALUES ('%q')",
				name.c_str()
				);
		}

		void CWebServer::Cmd_UpdatePlan(Json::Value &root)
		{
			if (m_pWebEm->m_actualuser_rights != 2)
				return;//Only admin user allowed

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

			m_sql.safe_query(
				"UPDATE Plans SET Name='%q' WHERE (ID == '%q')",
				name.c_str(),
				idx.c_str()
				);
		}

		void CWebServer::Cmd_DeletePlan(Json::Value &root)
		{
			if (m_pWebEm->m_actualuser_rights != 2)
				return;//Only admin user allowed

			std::string idx = m_pWebEm->FindValue("idx");
			if (idx == "")
				return;
			root["status"] = "OK";
			root["title"] = "DeletePlan";
			m_sql.safe_query(
				"DELETE FROM DeviceToPlansMap WHERE (PlanID == '%q')",
				idx.c_str()
				);
			m_sql.safe_query(
				"DELETE FROM Plans WHERE (ID == '%q')",
				idx.c_str()
				);
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

			std::vector<std::vector<std::string> > result;
			std::vector<std::vector<std::string> > result2;
			result = m_sql.safe_query("SELECT T1.[ID], T1.[Name], T1.[Type], T1.[SubType], T2.[Name] AS HardwareName FROM DeviceStatus as T1, Hardware as T2 WHERE (T1.[Used]==1) AND (T2.[ID]==T1.[HardwareID]) ORDER BY T2.[Name], T1.[Name]");
			if (result.size() > 0)
			{
				std::vector<std::vector<std::string> >::const_iterator itt;
				for (itt = result.begin(); itt != result.end(); ++itt)
				{
					std::vector<std::string> sd = *itt;

					bool bDoAdd = true;
					if (iUnique)
					{
						result2 = m_sql.safe_query("SELECT ID FROM DeviceToPlansMap WHERE (DeviceRowID=='%q') AND (DevSceneType==0)",
							sd[0].c_str());
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
			result = m_sql.safe_query("SELECT ID, Name FROM Scenes ORDER BY Name");
			if (result.size() > 0)
			{
				std::vector<std::vector<std::string> >::const_iterator itt;
				for (itt = result.begin(); itt != result.end(); ++itt)
				{
					std::vector<std::string> sd = *itt;

					bool bDoAdd = true;
					if (iUnique)
					{
						result2 = m_sql.safe_query("SELECT ID FROM DeviceToPlansMap WHERE (DeviceRowID=='%q') AND (DevSceneType==1)",
							sd[0].c_str());
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
				return;//Only admin user allowed

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
			std::vector<std::vector<std::string> > result;
			result = m_sql.safe_query("SELECT ID FROM DeviceToPlansMap WHERE (DeviceRowID=='%q') AND (DevSceneType==%d) AND (PlanID=='%q')",
				activeidx.c_str(), activetype, idx.c_str());
			if (result.size() == 0)
			{
				m_sql.safe_query(
					"INSERT INTO DeviceToPlansMap (DevSceneType,DeviceRowID, PlanID) VALUES (%d,'%q','%q')",
					activetype,
					activeidx.c_str(),
					idx.c_str()
					);
			}
		}

		void CWebServer::Cmd_GetPlanDevices(Json::Value &root)
		{
			std::string idx = m_pWebEm->FindValue("idx");
			if (idx == "")
				return;
			root["status"] = "OK";
			root["title"] = "GetPlanDevices";

			std::vector<std::vector<std::string> > result;
			result = m_sql.safe_query("SELECT ID, DevSceneType, DeviceRowID, [Order] FROM DeviceToPlansMap WHERE (PlanID=='%q') ORDER BY [Order]",
				idx.c_str());
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
						result2 = m_sql.safe_query("SELECT Name FROM DeviceStatus WHERE (ID=='%q')",
							DevSceneRowID.c_str());
						if (result2.size() > 0)
						{
							Name = result2[0][0];
						}
					}
					else
					{
						std::vector<std::vector<std::string> > result2;
						result2 = m_sql.safe_query("SELECT Name FROM Scenes WHERE (ID=='%q')",
							DevSceneRowID.c_str());
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
				return;//Only admin user allowed

			std::string idx = m_pWebEm->FindValue("idx");
			if (idx == "")
				return;
			root["status"] = "OK";
			root["title"] = "DeletePlanDevice";
			m_sql.safe_query("DELETE FROM DeviceToPlansMap WHERE (ID == '%q')", idx.c_str());
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
			m_sql.safe_query("UPDATE DeviceToPlansMap SET [XOffset] = '%q', [YOffset] = '%q' WHERE (DeviceRowID='%q') and (PlanID='%q') and (DevSceneType='%q')",
				xoffset.c_str(), yoffset.c_str(), idx.c_str(), planidx.c_str(), type.c_str());
			_log.Log(LOG_STATUS, "(Floorplan) Device '%s' coordinates set to '%s,%s' in plan '%s'.", idx.c_str(), xoffset.c_str(), yoffset.c_str(), planidx.c_str());
		}

		void CWebServer::Cmd_DeleteAllPlanDevices(Json::Value &root)
		{
			if (m_pWebEm->m_actualuser_rights != 2)
				return;//Only admin user allowed

			std::string idx = m_pWebEm->FindValue("idx");
			if (idx == "")
				return;
			root["status"] = "OK";
			root["title"] = "DeleteAllPlanDevices";
			m_sql.safe_query("DELETE FROM DeviceToPlansMap WHERE (PlanID == '%q')", idx.c_str());
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

			std::vector<std::vector<std::string> > result;
			result = m_sql.safe_query("SELECT [Order] FROM Plans WHERE (ID=='%q')",
				idx.c_str());
			if (result.size() < 1)
				return;
			aOrder = result[0][0];

			if (!bGoUp)
			{
				//Get next device order
				result = m_sql.safe_query("SELECT ID, [Order] FROM Plans WHERE ([Order]>'%q') ORDER BY [Order] ASC",
					aOrder.c_str());
				if (result.size() < 1)
					return;
				oID = result[0][0];
				oOrder = result[0][1];
			}
			else
			{
				//Get previous device order
				result = m_sql.safe_query("SELECT ID, [Order] FROM Plans WHERE ([Order]<'%q') ORDER BY [Order] DESC",
					aOrder.c_str());
				if (result.size() < 1)
					return;
				oID = result[0][0];
				oOrder = result[0][1];
			}
			//Swap them
			root["status"] = "OK";
			root["title"] = "ChangePlanOrder";

			m_sql.safe_query("UPDATE Plans SET [Order] = '%q' WHERE (ID='%q')",
				oOrder.c_str(), idx.c_str());
			m_sql.safe_query("UPDATE Plans SET [Order] = '%q' WHERE (ID='%q')",
				aOrder.c_str(), oID.c_str());
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

			std::vector<std::vector<std::string> > result;
			result = m_sql.safe_query("SELECT [Order] FROM DeviceToPlansMap WHERE ((ID=='%q') AND (PlanID=='%q'))",
				idx.c_str(), planid.c_str());
			if (result.size() < 1)
				return;
			aOrder = result[0][0];

			if (!bGoUp)
			{
				//Get next device order
				result = m_sql.safe_query("SELECT ID, [Order] FROM DeviceToPlansMap WHERE (([Order]>'%q') AND (PlanID=='%q')) ORDER BY [Order] ASC",
					aOrder.c_str(), planid.c_str());
				if (result.size() < 1)
					return;
				oID = result[0][0];
				oOrder = result[0][1];
			}
			else
			{
				//Get previous device order
				result = m_sql.safe_query("SELECT ID, [Order] FROM DeviceToPlansMap WHERE (([Order]<'%q') AND (PlanID=='%q')) ORDER BY [Order] DESC",
					aOrder.c_str(), planid.c_str());
				if (result.size() < 1)
					return;
				oID = result[0][0];
				oOrder = result[0][1];
			}
			//Swap them
			root["status"] = "OK";
			root["title"] = "ChangePlanOrder";

			m_sql.safe_query("UPDATE DeviceToPlansMap SET [Order] = '%q' WHERE (ID='%q')",
				oOrder.c_str(), idx.c_str());
			m_sql.safe_query("UPDATE DeviceToPlansMap SET [Order] = '%q' WHERE (ID='%q')",
				aOrder.c_str(), oID.c_str());
		}

		void CWebServer::Cmd_GetVersion(Json::Value &root)
		{
			root["status"] = "OK";
			root["title"] = "GetVersion";
			root["version"] = szAppVersion;
			root["hash"] = szAppHash;
			root["build_time"] = szAppDate;

			int nValue = 1;
			m_sql.GetPreferencesVar("UseAutoUpdate", nValue);

			if (m_pWebEm->m_actualuser_rights != 2)
			{
				//only admin users will receive the update notification
				root["haveupdate"] = false;
			}
			else
			{
				root["haveupdate"] = (nValue == 1) ? m_mainworker.m_bHaveUpdate : false;
			}
			root["revision"] = m_mainworker.m_iRevision;
		}

		void CWebServer::Cmd_GetAuth(Json::Value &root)
		{
			root["status"] = "OK";
			root["title"] = "GetAuth";
			root["user"] = m_pWebEm->m_actualuser;
			root["rights"] = m_pWebEm->m_actualuser_rights;
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
			//if (m_pWebEm->m_actualuser_rights != 2)
				//return;//Only admin user allowed

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

			nValue = 1;
			m_sql.GetPreferencesVar("ShowUpdateEffect", nValue);
			root["result"]["ShowUpdatedEffect"] = (nValue == 1);

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

			std::vector<std::vector<std::string> > result;

			if ((UserID != 0) && (UserID != 10000))
			{
				result = m_sql.safe_query("SELECT TabsEnabled FROM Users WHERE (ID==%lu)",
					UserID);
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

		void CWebServer::luaThread(lua_State *lua_state, const std::string &filename)
		{
			int status;
	
			status = lua_pcall(lua_state, 0, LUA_MULTRET, 0);
			report_errors(lua_state, status);
			lua_close(lua_state);
		}

		void CWebServer::luaStop(lua_State *L, lua_Debug *ar)
		{
			if (ar->event == LUA_HOOKCOUNT)
			{
				(void)ar;  /* unused arg. */
				lua_sethook(L, NULL, 0, 0);
				luaL_error(L, "WebServer: Lua script execution exceeds maximum number of lines");
				lua_close(L);
			}
		}

		void CWebServer::report_errors(lua_State *L, int status)
		{
			if (status != 0) {
				_log.Log(LOG_ERROR, "WebServer: %s", lua_tostring(L, -1));
				lua_pop(L, 1); // remove error message
			}
		}

		static int l_domoticz_updateDevice(lua_State* lua_state)
		{
			int nargs = lua_gettop(lua_state);
			if (nargs >= 3 && nargs <= 5)
			{
				// Supported format ares :
				// - idx (integer), svalue (string), nvalue (string), [rssi(integer)], [battery(integer)]
				// - idx (integer), svalue (string,) nvalue (integer), [rssi(integer)], [battery(integer)]
				if (lua_isnumber(lua_state, 1) && (lua_isstring(lua_state, 2) || lua_isnumber(lua_state, 2)) && lua_isstring(lua_state, 3))
				{
					// Extract the parameters from the lua 'updateDevice' function	
					int idx = lua_tointeger(lua_state, 1);
					std::string nvalue = lua_tostring(lua_state, 2);
					std::string svalue = lua_tostring(lua_state, 3);
					if (((lua_isstring(lua_state, 3) && nvalue.empty()) && svalue.empty()))
					{
						_log.Log(LOG_ERROR, "WebServer (updateDevice from LUA) : nvalue and svalue are empty ");
						return 0;
					}

					// Parse
					int invalue = (!nvalue.empty()) ? atoi(nvalue.c_str()) : 0;
					int signallevel = 12;
					if (nargs >= 4 && lua_isnumber(lua_state, 4))
					{
						signallevel = lua_tointeger(lua_state, 4);
					}
					int batterylevel = 255;
					if (nargs == 5 && lua_isnumber(lua_state, 5))
					{
						batterylevel = lua_tointeger(lua_state, 5);
					}
					_log.Log(LOG_NORM, "WebServer (updateDevice from LUA) : idx=%d nvalue=%s svalue=%s invalue=%d signallevel=%d batterylevel=%d", idx, nvalue.c_str(), svalue.c_str(), invalue, signallevel, batterylevel);

					// Get the raw device parameters
					std::vector<std::vector<std::string> > result;
					result = m_sql.safe_query("SELECT HardwareID, DeviceID, Unit, Type, SubType FROM DeviceStatus WHERE (ID==%d)", idx);
					if (result.empty())
						return 0;
					std::string hid = result[0][0];
					std::string did = result[0][1];
					std::string dunit = result[0][2];
					std::string dtype = result[0][3];
					std::string dsubtype = result[0][4];

					int HardwareID = atoi(hid.c_str());
					std::string DeviceID = did;
					int unit = atoi(dunit.c_str());
					int devType = atoi(dtype.c_str());
					int subType = atoi(dsubtype.c_str());

					std::stringstream sstr;
					unsigned long long ulIdx;
					sstr << idx;
					sstr >> ulIdx;
					m_mainworker.UpdateDevice(HardwareID, DeviceID, unit, devType, subType, invalue, svalue, signallevel, batterylevel);
				}
				else
				{
					_log.Log(LOG_ERROR, "WebServer (updateDevice from LUA) : Incorrect parameters type");
				}
			}
			else
			{
				_log.Log(LOG_ERROR, "WebServer (updateDevice from LUA) : Not enough parameters");
			}
			return 0;
		}

		static int l_domoticz_print(lua_State* lua_state)
		{
			int nargs = lua_gettop(lua_state);

			for (int i = 1; i <= nargs; i++)
			{
				if (lua_isstring(lua_state, i))
				{
					//std::string lstring=lua_tostring(lua_state, i);
					_log.Log(LOG_NORM, "WebServer: udevices: %s", lua_tostring(lua_state, i));
				}
				else
				{
					/* non strings? */
				}
			}
			return 0;
		}

		void CWebServer::Cmd_UpdateDevices(Json::Value &root)
		{
			std::stringstream lua_DirT;
#ifdef WIN32
			lua_DirT << szUserDataFolder << "scripts\\lua_parsers\\";
#else
			lua_DirT << szUserDataFolder << "scripts/lua_parsers/";
#endif
			std::string lua_Dir = lua_DirT.str();

			std::string script = m_pWebEm->FindValue("script");
			if (script.empty() )
			{
				return;
			}
			std::string content = m_pWebEm->m_ActualRequest.content;

			lua_State *lua_state;
			lua_state = luaL_newstate();

			luaL_openlibs(lua_state);
			lua_pushcfunction(lua_state, l_domoticz_print);
			lua_setglobal(lua_state, "print");

			lua_pushcfunction(lua_state, l_domoticz_updateDevice);
			lua_setglobal(lua_state, "domoticz_updateDevice");

			lua_createtable(lua_state, 1, 0);
			lua_pushstring(lua_state, "content");
			lua_pushstring(lua_state, content.c_str());
			lua_rawset(lua_state, -3);
			lua_setglobal(lua_state, "request");

			std::string fullfilename = lua_Dir + script;

			int status = luaL_loadfile(lua_state, fullfilename.c_str());
			if (status == 0)
			{
				lua_sethook(lua_state, luaStop, LUA_MASKCOUNT, 10000000);
				boost::thread aluaThread(boost::bind(&CWebServer::luaThread, this, lua_state, fullfilename));
				aluaThread.timed_join(boost::posix_time::seconds(10));
				root["status"] = "OK";
				root["title"] = "Update Device";
			}
			else
			{
				report_errors(lua_state, status);
				lua_close(lua_state);
			}
		}

		void CWebServer::Cmd_UpdateDevice(Json::Value &root)
		{
			std::string idx = m_pWebEm->FindValue("idx");
			std::string hid = m_pWebEm->FindValue("hid");
			std::string did = m_pWebEm->FindValue("did");
			std::string dunit = m_pWebEm->FindValue("dunit");
			std::string dtype = m_pWebEm->FindValue("dtype");
			std::string dsubtype = m_pWebEm->FindValue("dsubtype");

			std::string nvalue = m_pWebEm->FindValue("nvalue");
			std::string svalue = m_pWebEm->FindValue("svalue");

			if ( (nvalue.empty() && svalue.empty()) )
			{
				return;
			}

			int signallevel = 12;
			int batterylevel = 255;

			if (idx.empty())
			{
				//No index supplied, check if raw parameters where supplied
				if (
					(hid.empty()) ||
					(did.empty()) ||
					(dunit.empty()) ||
					(dtype.empty()) ||
					(dsubtype.empty())
					)
					return;
			}
			else
			{
				//Get the raw device parameters
				std::vector<std::vector<std::string> > result;
				result = m_sql.safe_query("SELECT HardwareID, DeviceID, Unit, Type, SubType FROM DeviceStatus WHERE (ID=='%q')",
					idx.c_str());
				if (result.empty())
					return;
				hid = result[0][0];
				did = result[0][1];
				dunit = result[0][2];
				dtype = result[0][3];
				dsubtype = result[0][4];
			}

			int HardwareID = atoi(hid.c_str());
			std::string DeviceID = did;
			int unit = atoi(dunit.c_str());
			int devType = atoi(dtype.c_str());
			int subType = atoi(dsubtype.c_str());

			std::stringstream sstr;

			unsigned long long ulIdx;
			sstr << idx;
			sstr >> ulIdx;

			int invalue = (!nvalue.empty()) ? atoi(nvalue.c_str()) : 0;


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
			if (m_mainworker.UpdateDevice(HardwareID, DeviceID, unit, devType, subType, invalue, svalue, signallevel, batterylevel))
			{
				root["status"] = "OK";
				root["title"] = "Update Device";
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
			std::vector<std::vector<std::string> > result;
			result = m_sql.safe_query("SELECT Type, SubType, nValue, sValue FROM DeviceStatus WHERE (ID=='%q')",
				idx.c_str());
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

				int tValue=1000;
				if (m_sql.GetPreferencesVar("MeterDividerWater", tValue))
				{
					root["DividerWater"] = float(tValue);
				}

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

			root["status"] = "OK";
			root["title"] = "CheckForUpdate";
			root["HaveUpdate"] = false;
			root["revision"] = m_mainworker.m_iRevision;

			if (m_pWebEm->m_actualuser_rights != 2)
				return; //Only admin users may update

			int nValue = 0;
			m_sql.GetPreferencesVar("UseAutoUpdate", nValue);
			if (nValue != 1)
			{
				return;
			}

			bool bIsForced = (m_pWebEm->FindValue("forced") == "true");

			root["HaveUpdate"] = m_mainworker.IsUpdateAvailable(bIsForced);
			root["revision"] = m_mainworker.m_iRevision;
		}

		void CWebServer::Cmd_DownloadUpdate(Json::Value &root)
		{
			if (!m_mainworker.StartDownloadUpdate())
				return;
			root["status"] = "OK";
			root["title"] = "DownloadUpdate";
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
				result = m_sql.safe_query("DELETE FROM LightSubDevices WHERE (ParentID == '%q')", idx.c_str());
			}
			else if (cparam == "deletesubdevice")
			{
				std::string idx = m_pWebEm->FindValue("idx");
				if (idx == "")
					return;
				root["status"] = "OK";
				root["title"] = "DeleteSubDevice";
				result = m_sql.safe_query("DELETE FROM LightSubDevices WHERE (ID == '%q')", idx.c_str());
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
				result = m_sql.safe_query("SELECT ID FROM LightSubDevices WHERE (DeviceRowID=='%q') AND (ParentID =='%q')",
					subidx.c_str(), idx.c_str());
				if (result.size() == 0)
				{
					root["status"] = "OK";
					root["title"] = "AddSubDevice";
					//no it is not, add it
					result = m_sql.safe_query(
						"INSERT INTO LightSubDevices (DeviceRowID, ParentID) VALUES ('%q','%q')",
						subidx.c_str(),
						idx.c_str()
						);
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

				std::string rState = (command == 1) ? "On" : "Off";

				//first check if this device is not the scene code!
				result = m_sql.safe_query("SELECT HardwareID, DeviceID, Unit, Type, SubType, SwitchType FROM DeviceStatus WHERE (ID=='%q')",
					devidx.c_str());
				if (result.size() > 0)
				{
					int dType = atoi(result[0][3].c_str());
					int sType = atoi(result[0][4].c_str());
					_eSwitchType switchtype = (_eSwitchType)atoi(result[0][5].c_str());
					unsigned char scommand;
					if (GetLightCommand(dType, sType, switchtype, rState, scommand))
					{
						command = scommand;
					}

					result2 = m_sql.safe_query("SELECT HardwareID, DeviceID, Unit, Type, SubType FROM Scenes WHERE (ID=='%q')",
						idx.c_str());
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
				result = m_sql.safe_query("SELECT ID FROM SceneDevices WHERE (DeviceRowID=='%q') AND (SceneRowID =='%q') AND (OnDelay == %d)",
					devidx.c_str(), idx.c_str(), ondelay);
				if (result.size() == 0)
				{
					root["status"] = "OK";
					root["title"] = "AddSceneDevice";
					//no it is not, add it
					if (isscene == "true")
					{
						m_sql.safe_query(
							"INSERT INTO SceneDevices (DeviceRowID, SceneRowID, Cmd, Level, Hue, OnDelay, OffDelay) VALUES ('%q','%q',%d,%d,%d,%d,%d)",
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
						m_sql.safe_query(
							"INSERT INTO SceneDevices (DeviceRowID, SceneRowID, Level, Hue, OnDelay, OffDelay) VALUES ('%q','%q',%d,%d,%d,%d)",
							devidx.c_str(),
							idx.c_str(),
							level,
							hue,
							ondelay,
							offdelay
							);
					}
				}
			}
			else if (cparam == "updatescenedevice")
			{
				std::string idx = m_pWebEm->FindValue("idx");
				std::string devidx = m_pWebEm->FindValue("devidx");
				int command = atoi(m_pWebEm->FindValue("command").c_str());
				int ondelay = atoi(m_pWebEm->FindValue("ondelay").c_str());
				int offdelay = atoi(m_pWebEm->FindValue("offdelay").c_str());

				if (
					(idx == "") ||
					(devidx == "")
					)
					return;

				std::string rState = (command == 1) ? "On" : "Off";

				//first check if this device is not the scene code!
				result = m_sql.safe_query("SELECT HardwareID, DeviceID, Unit, Type, SubType, SwitchType FROM DeviceStatus WHERE (ID=='%q')",
					devidx.c_str());
				if (result.size() > 0)
				{
					int dType = atoi(result[0][3].c_str());
					int sType = atoi(result[0][4].c_str());
					_eSwitchType switchtype = (_eSwitchType)atoi(result[0][5].c_str());
					unsigned char scommand;
					if (GetLightCommand(dType, sType, switchtype, rState, scommand))
					{
						command = scommand;
					}
				}
				int level = atoi(m_pWebEm->FindValue("level").c_str());
				int hue = atoi(m_pWebEm->FindValue("hue").c_str());
				root["status"] = "OK";
				root["title"] = "UpdateSceneDevice";
				result = m_sql.safe_query(
					"UPDATE SceneDevices SET Cmd=%d, Level=%d, Hue=%d, OnDelay=%d, OffDelay=%d  WHERE (ID == '%q')",
					command, level, hue, ondelay, offdelay, idx.c_str());
			}
			else if (cparam == "deletescenedevice")
			{
				std::string idx = m_pWebEm->FindValue("idx");
				if (idx == "")
					return;
				root["status"] = "OK";
				root["title"] = "DeleteSceneDevice";
				m_sql.safe_query("DELETE FROM SceneDevices WHERE (ID == '%q')", idx.c_str());
				m_sql.safe_query("DELETE FROM CamerasActiveDevices WHERE (DevSceneType==1) AND (DevSceneRowID == '%q')", idx.c_str());
			}
			else if (cparam == "getsubdevices")
			{
				std::string idx = m_pWebEm->FindValue("idx");
				if (idx == "")
					return;

				root["status"] = "OK";
				root["title"] = "GetSubDevices";
				std::vector<std::vector<std::string> > result;
				result = m_sql.safe_query("SELECT a.ID, b.Name FROM LightSubDevices a, DeviceStatus b WHERE (a.ParentID=='%q') AND (b.ID == a.DeviceRowID)",
					idx.c_str());
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
				result = m_sql.safe_query("SELECT t.ID, t.Active, d.[Name], t.DeviceRowID, t.[Date], t.Time, t.Type, t.Cmd, t.Level, t.Days FROM Timers as t, DeviceStatus as d WHERE (d.ID == t.DeviceRowID) AND (t.TimerPlan==%d) ORDER BY d.[Name], t.Time",
					m_sql.m_ActiveTimerPlan);
				if (result.size() > 0)
				{
					std::vector<std::vector<std::string> >::const_iterator itt;
					int ii = 0;
					for (itt = result.begin(); itt != result.end(); ++itt)
					{
						std::vector<std::string> sd = *itt;

						int iTimerType = atoi(sd[6].c_str());
						std::string sdate = sd[4];
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

						root["result"][ii]["ID"] = sd[0];
						root["result"][ii]["Active"] = sd[1];
						root["result"][ii]["Name"] = sd[2];
						root["result"][ii]["DeviceRowID"] = sd[3];
						root["result"][ii]["Date"] = sdate;
						root["result"][ii]["Time"] = sd[5];
						root["result"][ii]["Type"] = iTimerType;
						root["result"][ii]["Cmd"] = sd[7];
						root["result"][ii]["Level"] = sd[8];
						root["result"][ii]["Days"] = sd[9];
						ii++;
					}
				}
			}
			else if (cparam == "getscenetimerlist")
			{
				root["status"] = "OK";
				root["title"] = "GetSceneTimerList";
				std::vector<std::vector<std::string> > result;
				result = m_sql.safe_query("SELECT t.ID, t.Active, s.[Name], t.SceneRowID, t.[Date], t.Time, t.Type, t.Cmd, t.Level, t.Days FROM SceneTimers as t, Scenes as s WHERE (s.ID == t.SceneRowID) AND (t.TimerPlan==%d) ORDER BY s.[Name], t.Time",
					m_sql.m_ActiveTimerPlan);
				if (result.size() > 0)
				{
					std::vector<std::vector<std::string> >::const_iterator itt;
					int ii = 0;
					for (itt = result.begin(); itt != result.end(); ++itt)
					{
						std::vector<std::string> sd = *itt;

						int iTimerType = atoi(sd[6].c_str());
						std::string sdate = sd[4];
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

						root["result"][ii]["ID"] = sd[0];
						root["result"][ii]["Active"] = sd[1];
						root["result"][ii]["Name"] = sd[2];
						root["result"][ii]["SceneRowID"] = sd[3];
						root["result"][ii]["Date"] = sdate;
						root["result"][ii]["Time"] = sd[5];
						root["result"][ii]["Type"] = iTimerType;
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
				result = m_sql.safe_query("SELECT a.ID, b.Name, a.DeviceRowID, b.Type, b.SubType, b.nValue, b.sValue, a.Cmd, a.Level, b.ID, a.[Order], a.Hue, a.OnDelay, a.OffDelay FROM SceneDevices a, DeviceStatus b WHERE (a.SceneRowID=='%q') AND (b.ID == a.DeviceRowID) ORDER BY a.[Order]",
					idx.c_str());
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
				result = m_sql.safe_query("SELECT SceneRowID, [Order] FROM SceneDevices WHERE (ID=='%q')",
					idx.c_str());
				if (result.size() < 1)
					return;
				aScene = result[0][0];
				aOrder = result[0][1];

				if (!bGoUp)
				{
					//Get next device order
					result = m_sql.safe_query("SELECT ID, [Order] FROM SceneDevices WHERE (SceneRowID=='%q' AND [Order]>'%q') ORDER BY [Order] ASC",
						aScene.c_str(), aOrder.c_str());
					if (result.size() < 1)
						return;
					oID = result[0][0];
					oOrder = result[0][1];
				}
				else
				{
					//Get previous device order
					result = m_sql.safe_query("SELECT ID, [Order] FROM SceneDevices WHERE (SceneRowID=='%q' AND [Order]<'%q') ORDER BY [Order] DESC",
						aScene.c_str(), aOrder.c_str());
					if (result.size() < 1)
						return;
					oID = result[0][0];
					oOrder = result[0][1];
				}
				//Swap them
				root["status"] = "OK";
				root["title"] = "ChangeSceneDeviceOrder";

				result = m_sql.safe_query("UPDATE SceneDevices SET [Order] = '%q' WHERE (ID='%q')",
					oOrder.c_str(), idx.c_str());
				result = m_sql.safe_query("UPDATE SceneDevices SET [Order] = '%q' WHERE (ID='%q')",
					aOrder.c_str(), oID.c_str());
			}
			else if (cparam == "deleteallscenedevices")
			{
				std::string idx = m_pWebEm->FindValue("idx");
				if (idx == "")
					return;
				root["status"] = "OK";
				root["title"] = "DeleteAllSceneDevices";
				result = m_sql.safe_query("DELETE FROM SceneDevices WHERE (SceneRowID == %q)", idx.c_str());
			}
			else if (cparam == "getmanualhardware")
			{
				//used by Add Manual Light/Switch dialog
				root["status"] = "OK";
				root["title"] = "GetHardware";
				result = m_sql.safe_query("SELECT ID, Name, Type FROM Hardware ORDER BY ID ASC");
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
				result = m_sql.safe_query("SELECT ID, Name, Type, SubType, Used, SwitchType FROM DeviceStatus ORDER BY Name");
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
								resultSD = m_sql.safe_query("SELECT ID FROM LightSubDevices WHERE (DeviceRowID=='%q')",
									sd[0].c_str());
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
				int ii = 0;

				//First List/Switch Devices
				result = m_sql.safe_query("SELECT ID, Name, Type, SubType, Used FROM DeviceStatus ORDER BY Name");
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
				result = m_sql.safe_query("SELECT ID, Name FROM Scenes ORDER BY Name");
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
				//First List/Switch Devices
				result = m_sql.safe_query("SELECT ID, DevSceneType, DevSceneRowID, DevSceneWhen, DevSceneDelay FROM CamerasActiveDevices WHERE (CameraRowID=='%q') ORDER BY ID",
					idx.c_str());
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
							result2 = m_sql.safe_query("SELECT Name FROM DeviceStatus WHERE (ID=='%q')",
								DevSceneRowID.c_str());
							if (result2.size() > 0)
							{
								Name = "[Light/Switches] " + result2[0][0];
							}
						}
						else
						{
							std::vector<std::vector<std::string> > result2;
							result2 = m_sql.safe_query("SELECT Name FROM Scenes WHERE (ID=='%q')",
								DevSceneRowID.c_str());
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
				result = m_sql.safe_query(
					"SELECT ID FROM CamerasActiveDevices WHERE (CameraRowID=='%q')"
					" AND (DevSceneType==%d) AND (DevSceneRowID=='%q')"
					" AND (DevSceneWhen==%d)",
					idx.c_str(), activetype, activeidx.c_str(), activewhen);
				if (result.size() == 0)
				{
					root["status"] = "OK";
					root["title"] = "AddCameraActiveDevice";
					//no it is not, add it
					result = m_sql.safe_query(
						"INSERT INTO CamerasActiveDevices (CameraRowID, DevSceneType, DevSceneRowID, DevSceneWhen, DevSceneDelay) VALUES ('%q',%d,'%q',%d,%d)",
						idx.c_str(),
						activetype,
						activeidx.c_str(),
						activewhen,
						activedelay
						);
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
				result = m_sql.safe_query("DELETE FROM CamerasActiveDevices WHERE (ID == '%q')", idx.c_str());
				m_mainworker.m_cameras.ReloadCameras();
			}
			else if (cparam == "deleteallactivecamdevices")
			{
				std::string idx = m_pWebEm->FindValue("idx");
				if (idx == "")
					return;
				root["status"] = "OK";
				root["title"] = "DeleteAllCameraActiveDevices";
				result = m_sql.safe_query("DELETE FROM CamerasActiveDevices WHERE (CameraRowID == '%q')", idx.c_str());
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
					if ((subtype != sTypeEMW100) && (subtype != sTypeLivolo) && (subtype != sTypeLivoloAppliance))
						devid = "00" + id;
					else
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
					if ((subtype != sTypeEMW100) && (subtype != sTypeLivolo) && (subtype != sTypeLivoloAppliance))
						devid = "00" + id;
					else
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
						result = m_sql.safe_query(
							"SELECT Name FROM DeviceStatus WHERE (HardwareID=='%q' AND DeviceID=='%q' AND Unit=='%q' AND Type==%d AND SubType==%d)",
							hwdid.c_str(), devid.c_str(), sunitcode.c_str(), dtype, subtype);
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
						result = m_sql.safe_query(
							"SELECT ID FROM DeviceStatus WHERE (HardwareID=='%q' AND DeviceID=='%q' AND Unit=='%q' AND Type==%d AND SubType==%d)",
							hwdid.c_str(), devid.c_str(), sunitcode.c_str(), dtype, subtype);
						if (result.size() < 1)
						{
							root["message"] = "Error finding switch in Database!?!?";
							return;
						}
						std::string ID = result[0][0];

						m_sql.safe_query(
							"UPDATE DeviceStatus SET Used=1, Name='%q', SwitchType=%d WHERE (ID == '%q')",
							name.c_str(), switchtype, ID.c_str());

						//Now continue to insert the switch
						dtype = pTypeRadiator1;
						subtype = sTypeSmartwaresSwitchRadiator;

					}
				}

				//check if switch is unique
				std::vector<std::vector<std::string> > result;
				result = m_sql.safe_query(
					"SELECT Name FROM DeviceStatus WHERE (HardwareID=='%q' AND DeviceID=='%q' AND Unit=='%q' AND Type==%d AND SubType==%d)",
					hwdid.c_str(), devid.c_str(), sunitcode.c_str(), dtype, subtype);
				if (result.size() > 0)
				{
					root["message"] = "Switch already exists!";
					return;
				}

                // ----------- RFlink "Manual Add" Fix -----------
                CDomoticzHardwareBase *pBaseHardware = m_mainworker.GetHardware(atoi(hwdid.c_str()));
				if (pBaseHardware != NULL)
				{
					if (pBaseHardware->HwdType == HTYPE_RFLINK) {
						if (dtype == pTypeLighting1){
							dtype = pTypeGeneralSwitch;

							std::stringstream s_strid;
							s_strid << std::hex << atoi(devid.c_str());
							devid = s_strid.str();
							devid = "000000" + devid;
						}
						else
							if (dtype == pTypeLighting2){
								dtype = pTypeGeneralSwitch;
								subtype = sSwitchTypeAC;
								devid = "0" + devid;
							}
					}
				}
                // -----------------------------------------------
                
				bool bActEnabledState = m_sql.m_bAcceptNewHardware;
				m_sql.m_bAcceptNewHardware = true;
				std::string devname;
				m_sql.UpdateValue(atoi(hwdid.c_str()), devid.c_str(), atoi(sunitcode.c_str()), dtype, subtype, 0, -1, 0, devname);
				m_sql.m_bAcceptNewHardware = bActEnabledState;

				//set name and switchtype
				result = m_sql.safe_query(
					"SELECT ID FROM DeviceStatus WHERE (HardwareID=='%q' AND DeviceID=='%q' AND Unit=='%q' AND Type==%d AND SubType==%d)",
					hwdid.c_str(), devid.c_str(), sunitcode.c_str(), dtype, subtype);
				if (result.size() < 1)
				{
					root["message"] = "Error finding switch in Database!?!?";
					return;
				}
				std::string ID = result[0][0];

				m_sql.safe_query(
					"UPDATE DeviceStatus SET Used=1, Name='%q', SwitchType=%d WHERE (ID == '%q')",
					name.c_str(), switchtype, ID.c_str());

				if (maindeviceidx != "")
				{
					if (maindeviceidx != ID)
					{
						//this is a sub device for another light/switch
						//first check if it is not already a sub device
						result = m_sql.safe_query(
							"SELECT ID FROM LightSubDevices WHERE (DeviceRowID=='%q') AND (ParentID =='%q')",
							ID.c_str(), maindeviceidx.c_str());
						if (result.size() == 0)
						{
							//no it is not, add it
							result = m_sql.safe_query(
								"INSERT INTO LightSubDevices (DeviceRowID, ParentID) VALUES ('%q','%q')",
								ID.c_str(),
								maindeviceidx.c_str()
								);
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
				result = m_sql.safe_query("SELECT Type, SubType, SwitchType FROM DeviceStatus WHERE (ID == '%q')",
					idx.c_str());
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
					if ((switchtype == MTYPE_ENERGY)|| (switchtype == MTYPE_ENERGY_GENERATED))
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
				std::string ssendalways = m_pWebEm->FindValue("tsendalways");
				if ((stype.empty()) || (swhen.empty()) || (svalue.empty()) || (spriority.empty()) || (ssendalways.empty()))
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
				bool bOK = m_notifications.AddNotification(idx, szTmp, scustommessage, sactivesystems, priority, (ssendalways == "true") ? true : false);
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
				std::string ssendalways = m_pWebEm->FindValue("tsendalways");

				if ((stype.empty()) || (swhen.empty()) || (svalue.empty()) || (spriority.empty()) || (ssendalways.empty()))
					return;
				root["status"] = "OK";
				root["title"] = "UpdateNotification";

				//delete old record
				m_notifications.RemoveNotification(idx);

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
				m_notifications.AddNotification(devidx, szTmp, scustommessage, sactivesystems, priority, (ssendalways == "true") ? true : false);
			}
			else if (cparam == "deletenotification")
			{
				std::string idx = m_pWebEm->FindValue("idx");
				if (idx == "")
					return;

				root["status"] = "OK";
				root["title"] = "DeleteNotification";

				m_notifications.RemoveNotification(idx);
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
					result = m_sql.safe_query("SELECT [Order] FROM DeviceStatus WHERE (ID == '%q')",
						idx1.c_str());
					if (result.size() < 1)
						return;
					Order1 = result[0][0];

					//get device order 2
					result = m_sql.safe_query("SELECT [Order] FROM DeviceStatus WHERE (ID == '%q')",
						idx2.c_str());
					if (result.size() < 1)
						return;
					Order2 = result[0][0];

					root["status"] = "OK";
					root["title"] = "SwitchDeviceOrder";

					if (atoi(Order1.c_str()) < atoi(Order2.c_str()))
					{
						m_sql.safe_query(
							"UPDATE DeviceStatus SET [Order] = [Order]+1 WHERE ([Order] >= '%q' AND [Order] < '%q')",
							Order1.c_str(), Order2.c_str());
					}
					else
					{
						m_sql.safe_query(
							"UPDATE DeviceStatus SET [Order] = [Order]-1 WHERE ([Order] > '%q' AND [Order] <= '%q')",
							Order2.c_str(), Order1.c_str());
					}

					m_sql.safe_query("UPDATE DeviceStatus SET [Order] = '%q' WHERE (ID == '%q')",
						Order1.c_str(), idx2.c_str());
				}
				else
				{
					//change order in a room
					//get device order 1
					result = m_sql.safe_query("SELECT [Order] FROM DeviceToPlansMap WHERE (DeviceRowID == '%q') AND (PlanID==%d)",
						idx1.c_str(), roomid);
					if (result.size() < 1)
						return;
					Order1 = result[0][0];

					//get device order 2
					result = m_sql.safe_query("SELECT [Order] FROM DeviceToPlansMap WHERE (DeviceRowID == '%q') AND (PlanID==%d)",
						idx2.c_str(), roomid);
					if (result.size() < 1)
						return;
					Order2 = result[0][0];

					root["status"] = "OK";
					root["title"] = "SwitchDeviceOrder";

					if (atoi(Order1.c_str()) < atoi(Order2.c_str()))
					{
						m_sql.safe_query(
							"UPDATE DeviceToPlansMap SET [Order] = [Order]+1 WHERE ([Order] >= '%q' AND [Order] < '%q') AND (PlanID==%d)",
							Order1.c_str(), Order2.c_str(), roomid);
					}
					else
					{
						m_sql.safe_query(
							"UPDATE DeviceToPlansMap SET [Order] = [Order]-1 WHERE ([Order] > '%q' AND [Order] <= '%q') AND (PlanID==%d)",
							Order2.c_str(), Order1.c_str(), roomid);
					}

					m_sql.safe_query("UPDATE DeviceToPlansMap SET [Order] = '%q' WHERE (DeviceRowID == '%q') AND (PlanID==%d)",
						Order1.c_str(), idx2.c_str(), roomid);
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
				result = m_sql.safe_query("SELECT [Order] FROM Scenes WHERE (ID == '%q')",
					idx1.c_str());
				if (result.size() < 1)
					return;
				Order1 = result[0][0];

				//get device order 2
				result = m_sql.safe_query("SELECT [Order] FROM Scenes WHERE (ID == '%q')",
					idx2.c_str());
				if (result.size() < 1)
					return;
				Order2 = result[0][0];

				root["status"] = "OK";
				root["title"] = "SwitchSceneOrder";

				if (atoi(Order1.c_str()) < atoi(Order2.c_str()))
				{
					m_sql.safe_query(
						"UPDATE Scenes SET [Order] = [Order]+1 WHERE ([Order] >= '%q' AND [Order] < '%q')",
						Order1.c_str(), Order2.c_str());
				}
				else
				{
					m_sql.safe_query(
						"UPDATE Scenes SET [Order] = [Order]-1 WHERE ([Order] > '%q' AND [Order] <= '%q')",
						Order2.c_str(), Order1.c_str());
				}

				m_sql.safe_query("UPDATE Scenes SET [Order] = '%q' WHERE (ID == '%q')",
					Order1.c_str(), idx2.c_str());
			}
			else if (cparam == "clearnotifications")
			{
				std::string idx = m_pWebEm->FindValue("idx");
				if (idx == "")
					return;

				root["status"] = "OK";
				root["title"] = "ClearNotification";

				m_notifications.RemoveDeviceNotifications(idx);
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
					m_sql.safe_query(
						"INSERT INTO Cameras (Name, Enabled, Address, Port, Username, Password, ImageURL) VALUES ('%q',%d,'%q',%d,'%q','%q','%q')",
						name.c_str(),
						(senabled == "true") ? 1 : 0,
						address.c_str(),
						port,
						base64_encode((const unsigned char*)username.c_str(), username.size()).c_str(),
						base64_encode((const unsigned char*)password.c_str(), password.size()).c_str(),
						imageurl.c_str()
						);
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

					m_sql.safe_query(
						"UPDATE Cameras SET Name='%q', Enabled=%d, Address='%q', Port=%d, Username='%q', Password='%q', ImageURL='%q' WHERE (ID == '%q')",
						name.c_str(),
						(senabled == "true") ? 1 : 0,
						address.c_str(),
						port,
						base64_encode((const unsigned char*)username.c_str(), username.size()).c_str(),
						base64_encode((const unsigned char*)password.c_str(), password.size()).c_str(),
						imageurl.c_str(),
						idx.c_str()
						);
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
				m_sql.safe_query(
					"INSERT INTO Users (Active, Username, Password, Rights, RemoteSharing, TabsEnabled) VALUES (%d,'%q','%q','%d','%d','%d')",
					(senabled == "true") ? 1 : 0,
					base64_encode((const unsigned char*)username.c_str(), username.size()).c_str(),
					password.c_str(),
					rights,
					(sRemoteSharing == "true") ? 1 : 0,
					atoi(sTabsEnabled.c_str())
					);
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
				m_sql.safe_query(
					"UPDATE Users SET Active=%d, Username='%q', Password='%q', Rights=%d, RemoteSharing=%d, TabsEnabled=%d WHERE (ID == '%q')",
					(senabled == "true") ? 1 : 0,
					base64_encode((const unsigned char*)username.c_str(), username.size()).c_str(),
					password.c_str(),
					rights,
					(sRemoteSharing == "true") ? 1 : 0,
					atoi(sTabsEnabled.c_str()),
					idx.c_str()
					);
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
				m_sql.safe_query("DELETE FROM Users WHERE (ID == '%q')", idx.c_str());

				m_sql.safe_query("DELETE FROM SharedDevices WHERE (SharedUserID == '%q')",
					idx.c_str());

				LoadUsers();
			}
			else if (cparam == "clearlightlog")
			{
				std::string idx = m_pWebEm->FindValue("idx");
				if (idx == "")
					return;
				//First get Device Type/SubType
				result = m_sql.safe_query("SELECT Type, SubType FROM DeviceStatus WHERE (ID == '%q')",
					idx.c_str());
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
					(!((dType == pTypeRadiator1) && (dSubType == sTypeSmartwaresSwitchRadiator)))&&
					(!((dType == pTypeGeneral) && (dSubType == sTypeTextStatus))) &&
					(!((dType == pTypeGeneral) && (dSubType == sTypeAlert)))
					)
					return; //no light device! we should not be here!

				root["status"] = "OK";
				root["title"] = "ClearLightLog";

				result = m_sql.safe_query("DELETE FROM LightingLog WHERE (DeviceRowID=='%q')",
					idx.c_str());
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
					result = m_sql.safe_query("SELECT Name, Used, nValue FROM DeviceStatus WHERE (ID==%llu)",
						m_sql.m_LastSwitchRowID);
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
				m_sql.safe_query("UPDATE DeviceStatus SET Favorite=%d WHERE (ID == '%q')",
					isfavorite, idx.c_str());
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
				m_sql.safe_query("UPDATE Scenes SET Favorite=%d WHERE (ID == '%q')",
					isfavorite, idx.c_str());
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
					m_sql.safe_query("UPDATE DeviceStatus SET nValue=%d WHERE (ID == '%q')",
						nValue, idx.c_str());
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
					result = m_sql.safe_query(
						"SELECT [Type],[SubType],SwitchType,nValue,sValue FROM DeviceStatus WHERE (ID = '%q')", idx.c_str());
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
						//strftime(szTmp, 80, "%b %d %Y %X", &loctime);
						strftime(szTmp, 80, "%Y-%m-%d %X", &loctime);

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
				//strftime(szTmp, 80, "%b %d %Y %X", &loctime);
				strftime(szTmp, 80, "%Y-%m-%d %X", &loctime);

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
				m_sql.safe_query(
					"INSERT INTO Floorplans (Name,ImageFile,ScaleFactor) VALUES ('%q','%q',%q)",
					name.c_str(),
					imagefile.c_str(),
					scalefactor.c_str()
					);
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

				m_sql.safe_query(
					"UPDATE Floorplans SET Name='%q',ImageFile='%q', ScaleFactor='%q' WHERE (ID == '%q')",
					name.c_str(),
					imagefile.c_str(),
					scalefactor.c_str(),
					idx.c_str()
					);
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
				 m_sql.safe_query(
					"UPDATE DeviceToPlansMap SET XOffset=0,YOffset=0 WHERE (PlanID IN (SELECT ID from Plans WHERE (FloorplanID == '%q')))",
					idx.c_str()
					);
				_log.Log(LOG_STATUS, "(Floorplan) Device coordinates reset for all plans on floorplan '%s'.", idx.c_str());
				m_sql.safe_query(
					"UPDATE Plans SET FloorplanID=0,Area='' WHERE (FloorplanID == '%q')",
					idx.c_str()
					);
				_log.Log(LOG_STATUS, "(Floorplan) Plans for floorplan '%s' reset.", idx.c_str());
				m_sql.safe_query(
					"DELETE FROM Floorplans WHERE (ID == '%q')",
					idx.c_str()
					);
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
				result = m_sql.safe_query("SELECT [Order] FROM Floorplans WHERE (ID=='%q')",
					idx.c_str());
				if (result.size() < 1)
					return;
				aOrder = result[0][0];

				if (!bGoUp)
				{
					//Get next device order
					result = m_sql.safe_query("SELECT ID, [Order] FROM Floorplans WHERE ([Order]>'%q') ORDER BY [Order] ASC",
						aOrder.c_str());
					if (result.size() < 1)
						return;
					oID = result[0][0];
					oOrder = result[0][1];
				}
				else
				{
					//Get previous device order
					result = m_sql.safe_query("SELECT ID, [Order] FROM Floorplans WHERE ([Order]<'%q') ORDER BY [Order] DESC",
						aOrder.c_str());
					if (result.size() < 1)
						return;
					oID = result[0][0];
					oOrder = result[0][1];
				}
				//Swap them
				root["status"] = "OK";
				root["title"] = "ChangeFloorPlanOrder";

				m_sql.safe_query("UPDATE Floorplans SET [Order] = '%q' WHERE (ID='%q')",
					oOrder.c_str(), idx.c_str());
				m_sql.safe_query("UPDATE Floorplans SET [Order] = '%q' WHERE (ID='%q')",
					aOrder.c_str(), oID.c_str());
			}
			else if (cparam == "getunusedfloorplanplans")
			{
				root["status"] = "OK";
				root["title"] = "GetUnusedFloorplanPlans";
				int ii = 0;

				result = m_sql.safe_query("SELECT ID, Name FROM Plans WHERE (FloorplanID==0) ORDER BY Name");
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

				result = m_sql.safe_query("SELECT ID, Name, Area FROM Plans WHERE (FloorplanID=='%q') ORDER BY Name",
					idx.c_str());
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

				 m_sql.safe_query(
					"UPDATE Plans SET FloorplanID='%q' WHERE (ID == '%q')",
					idx.c_str(),
					planidx.c_str()
					);
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

				m_sql.safe_query(
					"UPDATE Plans SET Area='%q' WHERE (ID == '%q')",
					planarea.c_str(),
					planidx.c_str()
					);
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
				m_sql.safe_query(
					"UPDATE DeviceToPlansMap SET XOffset=0,YOffset=0 WHERE (PlanID == '%q')",
					idx.c_str()
					);
				_log.Log(LOG_STATUS, "(Floorplan) Device coordinates reset for plan '%s'.", idx.c_str());
				m_sql.safe_query(
					"UPDATE Plans SET FloorplanID=0,Area='' WHERE (ID == '%q')",
					idx.c_str()
					);
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
						result = m_sql.safe_query("SELECT ID, Active, Username, Password, Rights, TabsEnabled FROM Users");
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
			std::string EnableTabCustom = m_pWebEm->FindValue("EnableTabCustom");
			m_sql.UpdatePreferencesVar("EnableTabCustom", (EnableTabCustom == "on" ? 1 : 0));

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

			std::string ShowUpdateEffect = m_pWebEm->FindValue("ShowUpdateEffect");
			int iShowUpdateEffect = (ShowUpdateEffect == "on" ? 1 : 0);
			m_sql.UpdatePreferencesVar("ShowUpdateEffect", iShowUpdateEffect);

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
					m_mainworker.m_sharedserver.StartServer("::", szPort);
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

			m_notifications.LoadConfig();

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

			m_mainworker.StopDomoticzHardware();

			bool bOK = m_sql.RestoreDatabase(dbasefile);
			m_mainworker.AddAllDomoticzHardware();
			if (!bOK)
				return (char*)m_retstr.c_str();
			return (char*)m_retstr.c_str();
		}

		struct _tHardwareListInt{
			std::string Name;
			bool Enabled;
		} tHardwareList;

		void CWebServer::GetJSonDevices(Json::Value &root, const std::string &rused, const std::string &rfilter, const std::string &order, const std::string &rowid, const std::string &planID, const std::string &floorID, const bool bDisplayHidden, const time_t LastUpdate, const bool bSkipUserCheck)
		{
			std::vector<std::vector<std::string> > result;

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
			result = m_sql.safe_query("SELECT ID, Name, Enabled FROM Hardware");
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

			char szData[250];
			char szTmp[300];

			if (!m_mainworker.m_LastSunriseSet.empty())
			{
				std::vector<std::string> strarray;
				StringSplit(m_mainworker.m_LastSunriseSet, ";", strarray);
				if (strarray.size() == 2)
				{
					//strftime(szTmp, 80, "%b %d %Y %X", &tm1);
					strftime(szTmp, 80, "%Y-%m-%d %X", &tm1);
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
							result = m_sql.safe_query("SELECT DeviceRowID FROM SharedDevices WHERE (SharedUserID == %lu)",
								m_users[iUser].ID);
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
					if (rowid != "")
						result = m_sql.safe_query(
							"SELECT ID, Name, nValue, LastUpdate, Favorite, SceneType, Protected, 0 as XOffset, 0 as YOffset, 0 as PlanID, Description FROM Scenes WHERE (ID=='%q')",
							rowid.c_str());
					else if ((planID != "") && (planID != "0"))
						result = m_sql.safe_query(
							"SELECT A.ID, A.Name, A.nValue, A.LastUpdate, A.Favorite, A.SceneType, A.Protected, B.XOffset, B.YOffset, B.PlanID, A.Description FROM Scenes as A, DeviceToPlansMap as B WHERE (B.PlanID=='%q') AND (B.DeviceRowID==a.ID) AND (B.DevSceneType==1) ORDER BY B.[Order]",
							planID.c_str());
					else if ((floorID != "") && (floorID != "0"))
						result = m_sql.safe_query(
								"SELECT A.ID, A.Name, A.nValue, A.LastUpdate, A.Favorite, A.SceneType, A.Protected, B.XOffset, B.YOffset, B.PlanID, A.Description FROM Scenes as A, DeviceToPlansMap as B, Plans as C WHERE (C.FloorplanID=='%q') AND (C.ID==B.PlanID) AND(B.DeviceRowID==a.ID) AND (B.DevSceneType==1) ORDER BY B.[Order]",
								floorID.c_str());
					else
						result = m_sql.safe_query(
							"SELECT ID, Name, nValue, LastUpdate, Favorite, SceneType, Protected, 0 as XOffset, 0 as YOffset, 0 as PlanID, Description FROM Scenes ORDER BY %s",
							szOrderBy);

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
							root["result"][ii]["Description"] = sd[10];
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

			if (totUserDevices == 0)
			{
				//All
				if (rowid != "")
					result = m_sql.safe_query(
						"SELECT ID, DeviceID, Unit, Name, Used, Type, SubType,"
						" SignalLevel, BatteryLevel, nValue, sValue,"
						" LastUpdate, Favorite, SwitchType, HardwareID,"
						" AddjValue, AddjMulti, AddjValue2, AddjMulti2,"
						" LastLevel, CustomImage, StrParam1, StrParam2,"
						" Protected, 0 as XOffset, 0 as YOffset, 0 as PlanID, Description "
						"FROM DeviceStatus WHERE (ID=='%q')",
						rowid.c_str());
				else if ((planID != "") && (planID != "0"))
					result = m_sql.safe_query(
						"SELECT A.ID, A.DeviceID, A.Unit, A.Name, A.Used,"
						" A.Type, A.SubType, A.SignalLevel, A.BatteryLevel,"
						" A.nValue, A.sValue, A.LastUpdate, A.Favorite,"
						" A.SwitchType, A.HardwareID, A.AddjValue,"
						" A.AddjMulti, A.AddjValue2, A.AddjMulti2,"
						" A.LastLevel, A.CustomImage, A.StrParam1,"
						" A.StrParam2, A.Protected, B.XOffset, B.YOffset,"
						" B.PlanID, A.Description "
						"FROM DeviceStatus as A, DeviceToPlansMap as B "
						"WHERE (B.PlanID=='%q') AND (B.DeviceRowID==a.ID)"
						" AND (B.DevSceneType==0) ORDER BY B.[Order]",
						planID.c_str());
				else if ((floorID != "") && (floorID != "0"))
					result = m_sql.safe_query(
						"SELECT A.ID, A.DeviceID, A.Unit, A.Name, A.Used,"
						" A.Type, A.SubType, A.SignalLevel, A.BatteryLevel,"
						" A.nValue, A.sValue, A.LastUpdate, A.Favorite,"
						" A.SwitchType, A.HardwareID, A.AddjValue,"
						" A.AddjMulti, A.AddjValue2, A.AddjMulti2,"
						" A.LastLevel, A.CustomImage, A.StrParam1,"
						" A.StrParam2, A.Protected, B.XOffset, B.YOffset,"
						" B.PlanID, A.Description "
						"FROM DeviceStatus as A, DeviceToPlansMap as B,"
						" Plans as C "
						"WHERE (C.FloorplanID=='%q') AND (C.ID==B.PlanID)"
						" AND (B.DeviceRowID==a.ID) AND (B.DevSceneType==0) "
						"ORDER BY B.[Order]",
						floorID.c_str());
				else {
					if (!bDisplayHidden)
					{
						//Build a list of Hidden Devices
						result = m_sql.safe_query("SELECT ID FROM Plans WHERE (Name=='$Hidden Devices')");
						if (result.size() > 0)
						{
							std::string pID = result[0][0];
							result = m_sql.safe_query("SELECT DeviceRowID FROM DeviceToPlansMap WHERE (PlanID=='%q')",
								pID.c_str());
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
					result = m_sql.safe_query(
						"SELECT ID, DeviceID, Unit, Name, Used, Type, SubType,"
						" SignalLevel, BatteryLevel, nValue, sValue,"
						" LastUpdate, Favorite, SwitchType, HardwareID,"
						" AddjValue, AddjMulti, AddjValue2, AddjMulti2,"
						" LastLevel, CustomImage, StrParam1, StrParam2,"
						" Protected, 0 as XOffset, 0 as YOffset, 0 as PlanID, Description "
						"FROM DeviceStatus ORDER BY %s",
						szOrderBy);
				}
			}
			else
			{
				//Specific devices
				if (rowid != "")
					result = m_sql.safe_query(
						"SELECT A.ID, A.DeviceID, A.Unit, A.Name, A.Used,"
						" A.Type, A.SubType, A.SignalLevel, A.BatteryLevel,"
						" A.nValue, A.sValue, A.LastUpdate, A.Favorite,"
						" A.SwitchType, A.HardwareID, A.AddjValue,"
						" A.AddjMulti, A.AddjValue2, A.AddjMulti2,"
						" A.LastLevel, A.CustomImage, A.StrParam1,"
						" A.StrParam2, A.Protected, 0 as XOffset,"
						" 0 as YOffset, 0 as PlanID, A.Description "
						"FROM DeviceStatus as A, SharedDevices as B "
						"WHERE (B.DeviceRowID==a.ID)"
						" AND (B.SharedUserID==%lu) AND (A.ID=='%q')",
						m_users[iUser].ID, rowid.c_str());
				else if ((planID != "") && (planID != "0"))
					result = m_sql.safe_query(
						"SELECT A.ID, A.DeviceID, A.Unit, A.Name, A.Used,"
						" A.Type, A.SubType, A.SignalLevel, A.BatteryLevel,"
						" A.nValue, A.sValue, A.LastUpdate, A.Favorite,"
						" A.SwitchType, A.HardwareID, A.AddjValue,"
						" A.AddjMulti, A.AddjValue2, A.AddjMulti2,"
						" A.LastLevel, A.CustomImage, A.StrParam1,"
						" A.StrParam2, A.Protected, C.XOffset,"
						" C.YOffset, C.PlanID, A.Description "
						"FROM DeviceStatus as A, SharedDevices as B,"
						" DeviceToPlansMap as C "
						"WHERE (C.PlanID=='%q') AND (C.DeviceRowID==a.ID)"
						" AND (B.DeviceRowID==a.ID) "
						"AND (B.SharedUserID==%lu) ORDER BY C.[Order]",
						planID.c_str(), m_users[iUser].ID);
				else if ((floorID != "") && (floorID != "0"))
					result = m_sql.safe_query(
						"SELECT A.ID, A.DeviceID, A.Unit, A.Name, A.Used,"
						" A.Type, A.SubType, A.SignalLevel, A.BatteryLevel,"
						" A.nValue, A.sValue, A.LastUpdate, A.Favorite,"
						" A.SwitchType, A.HardwareID, A.AddjValue,"
						" A.AddjMulti, A.AddjValue2, A.AddjMulti2,"
						" A.LastLevel, A.CustomImage, A.StrParam1,"
						" A.StrParam2, A.Protected, C.XOffset, C.YOffset,"
						" C.PlanID, A.Description "
						"FROM DeviceStatus as A, SharedDevices as B,"
						" DeviceToPlansMap as C, Plans as D "
						"WHERE (D.FloorplanID=='%q') AND (D.ID==C.PlanID)"
						" AND (C.DeviceRowID==a.ID) AND (B.DeviceRowID==a.ID)"
						" AND (B.SharedUserID==%lu) ORDER BY C.[Order]",
						floorID.c_str(), m_users[iUser].ID);
				else {
					if (!bDisplayHidden)
					{
						//Build a list of Hidden Devices
						result = m_sql.safe_query("SELECT ID FROM Plans WHERE (Name=='$Hidden Devices')");
						if (result.size() > 0)
						{
							std::string pID = result[0][0];
							result = m_sql.safe_query("SELECT DeviceRowID FROM DeviceToPlansMap WHERE (PlanID=='%q')",
								pID.c_str());
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
					result = m_sql.safe_query(
						"SELECT A.ID, A.DeviceID, A.Unit, A.Name, A.Used,"
						" A.Type, A.SubType, A.SignalLevel, A.BatteryLevel,"
						" A.nValue, A.sValue, A.LastUpdate, A.Favorite,"
						" A.SwitchType, A.HardwareID, A.AddjValue,"
						" A.AddjMulti, A.AddjValue2, A.AddjMulti2,"
						" A.LastLevel, A.CustomImage, A.StrParam1,"
						" A.StrParam2, A.Protected, 0 as XOffset,"
						" 0 as YOffset, 0 as PlanID, A.Description "
						"FROM DeviceStatus as A, SharedDevices as B "
						"WHERE (B.DeviceRowID==a.ID)"
						" AND (B.SharedUserID==%lu) ORDER BY %s",
						m_users[iUser].ID, szOrderBy);
				}
			}

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

					std::string Description = sd[27];

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
						(dType == pTypeRAIN) ||
						(dType == pTypeUV) ||
						(dType == pTypeCURRENT) ||
						(dType == pTypeCURRENTENERGY) ||
						(dType == pTypeENERGY) ||
						(dType == pTypeRFXMeter) ||
						(dType == pTypeAirQuality) ||
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
					root["result"][ii]["Description"] = Description;
					root["result"][ii]["Used"] = used;
					root["result"][ii]["Favorite"] = favorite;
					root["result"][ii]["SignalLevel"] = atoi(sd[7].c_str());
					root["result"][ii]["BatteryLevel"] = atoi(sd[8].c_str());
					root["result"][ii]["LastUpdate"] = sLastUpdate;
					root["result"][ii]["CustomImage"] = CustomImage;
					root["result"][ii]["XOffset"] = sd[24].c_str();
					root["result"][ii]["YOffset"] = sd[25].c_str();
					root["result"][ii]["PlanID"] = sd[26].c_str();
					root["result"][ii]["AddjValue"] = AddjValue;
					root["result"][ii]["AddjMulti"] = AddjMulti;
					root["result"][ii]["AddjValue2"] = AddjValue2;
					root["result"][ii]["AddjMulti2"] = AddjMulti2;
					sprintf(szData, "%d, %s", nValue, sValue.c_str());
					root["result"][ii]["Data"] = szData;

					root["result"][ii]["Notifications"] = (m_notifications.HasNotifications(sd[0]) == true) ? "true" : "false";
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
							root["result"][ii]["CameraIdx"] = scidx.str();
						}

						bool bIsSubDevice = false;
						std::vector<std::vector<std::string> > resultSD;
						std::stringstream szQuerySD;

						resultSD = m_sql.safe_query("SELECT ID FROM LightSubDevices WHERE (DeviceRowID=='%q')",
							sd[0].c_str());
						bIsSubDevice = (resultSD.size() > 0);

						root["result"][ii]["IsSubDevice"] = bIsSubDevice;

						if (switchtype == STYPE_Doorbell)
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
						}
						else if (switchtype == STYPE_PushOn)
						{
							root["result"][ii]["TypeImg"] = "push";
							root["result"][ii]["Status"] = "";
							root["result"][ii]["InternalState"] = (IsLightSwitchOn(lstatus) == true) ? "On" : "Off";
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
						}
						else if (switchtype == STYPE_Media)
						{
							root["result"][ii]["TypeImg"] = "Media";
							root["result"][ii]["Status"] = Media_Player_States((_eMediaStatus)nValue);
							lstatus = sValue;
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
							else if (lstatus == "Stop") {
								lstatus = "Stopped";
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
						}
						else if (switchtype == STYPE_Motion)
						{
							root["result"][ii]["TypeImg"] = "motion";
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

							if (dSubType != sTypeRAINWU)
							{
								result2 = m_sql.safe_query(
									"SELECT MIN(Total), MAX(Total), MAX(Rate) FROM Rain WHERE (DeviceRowID='%q' AND Date>='%q')",
									sd[0].c_str(), szDate);
							}
							else
							{
								result2 = m_sql.safe_query(
									"SELECT Total, Total, Rate FROM Rain WHERE (DeviceRowID='%q' AND Date>='%q') ORDER BY ROWID DESC LIMIT 1",
									sd[0].c_str(), szDate);
							}
							if (result2.size() > 0)
							{
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
						result2 = m_sql.safe_query("SELECT MIN(Value), MAX(Value) FROM Meter WHERE (DeviceRowID='%q' AND Date>='%q')",
							sd[0].c_str(), szDate);
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
							case MTYPE_ENERGY_GENERATED:
								musage = float(total_real) / EnergyDivider;
								sprintf(szTmp, "%.03f kWh", musage);
								break;
							case MTYPE_GAS:
								musage = float(total_real) / GasDivider;
								sprintf(szTmp, "%.03f m3", musage);
								break;
							case MTYPE_WATER:
								sprintf(szTmp, "%llu Liter", total_real);
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
						case MTYPE_ENERGY_GENERATED:
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
                        result2 = m_sql.safe_query("SELECT MIN(Value), MAX(Value) FROM Meter WHERE (DeviceRowID='%q' AND Date>='%q')",
                            sd[0].c_str(), szDate);
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
							case MTYPE_ENERGY_GENERATED:
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
						root["result"][ii]["TypeImg"] = "counter";
                        float fvalue = static_cast<float>(atof(sValue.c_str()));
                        switch (metertype)
                        {
                        case MTYPE_ENERGY:
						case MTYPE_ENERGY_GENERATED:
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
						result2 = m_sql.safe_query("SELECT MIN(Value), MAX(Value) FROM Meter WHERE (DeviceRowID='%q' AND Date>='%q')",
							sd[0].c_str(), szDate);
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
							case MTYPE_ENERGY_GENERATED:
								musage = float(total_real) / EnergyDivider;
								sprintf(szTmp, "%.03f kWh", musage);
								break;
							case MTYPE_GAS:
								musage = float(total_real) / GasDivider;
								sprintf(szTmp, "%.02f m3", musage);
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
						case MTYPE_ENERGY_GENERATED:
							musage = float(total_actual) / EnergyDivider;
							sprintf(szTmp, "%.03f", musage);
							break;
						case MTYPE_GAS:
						case MTYPE_WATER:
							musage = float(total_actual) / GasDivider;
							sprintf(szTmp, "%.03f", musage);
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
						case MTYPE_ENERGY_GENERATED:
							musage = float(acounter) / EnergyDivider;
							sprintf(szTmp, "%.03f kWh %s Watt", musage, splitresults[1].c_str());
							break;
						case MTYPE_GAS:
							musage = float(acounter) / GasDivider;
							sprintf(szTmp, "%.02f m3", musage);
							break;
						case MTYPE_WATER:
							musage = float(acounter) / WaterDivider;
							sprintf(szTmp, "%.03f m3", musage);
							break;
						case MTYPE_COUNTER:
							sprintf(szTmp, "%llu", acounter);
							break;
						}
						root["result"][ii]["Data"] = szTmp;
						switch (metertype)
						{
						case MTYPE_ENERGY:
						case MTYPE_ENERGY_GENERATED:
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
						result2 = m_sql.safe_query("SELECT MIN(Value1), MIN(Value2), MIN(Value5), MIN(Value6) FROM MultiMeter WHERE (DeviceRowID='%q' AND Date>='%q')",
							sd[0].c_str(), szDate);
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
						result2 = m_sql.safe_query("SELECT MIN(Value) FROM Meter WHERE (DeviceRowID='%q' AND Date>='%q')",
							sd[0].c_str(), szDate);
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
									sprintf(szData, "%d Watt", int(val1*voltage));
								else
									sprintf(szData, "%d Watt, %d Watt, %d Watt", int(val1*voltage), int(val2*voltage), int(val3*voltage));
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
							result2 = m_sql.safe_query("SELECT MIN(Value) FROM Meter WHERE (DeviceRowID='%q' AND Date>='%q')",
								sd[0].c_str(), szDate);
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
								root["result"][ii]["HaveTimeout"] = bHaveTimeout;
							}
							root["result"][ii]["SwitchTypeVal"] = switchtype; //MTYPE_ENERGY

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
							root["result"][ii]["HaveTimeout"] = false; //this device does not provide feedback, so no timeout!
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
							result2 = m_sql.safe_query("SELECT MIN(Value), MAX(Value) FROM Meter WHERE (DeviceRowID='%q' AND Date>='%q')",
								sd[0].c_str(), szDate);
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
				return;//Only admin user allowed

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
				return;//Only admin user allowed

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
			m_sql.safe_query(
				"INSERT INTO Scenes (Name,SceneType) VALUES ('%q',%d)",
				name.c_str(),
				atoi(stype.c_str())
				);
		}

		void CWebServer::RType_DeleteScene(Json::Value &root)
		{
			if (m_pWebEm->m_actualuser_rights != 2)
				return;//Only admin user allowed

			std::string idx = m_pWebEm->FindValue("idx");
			if (idx == "")
				return;
			root["status"] = "OK";
			root["title"] = "DeleteScene";
			m_sql.safe_query("DELETE FROM Scenes WHERE (ID == '%q')", idx.c_str());
			m_sql.safe_query("DELETE FROM SceneDevices WHERE (SceneRowID == '%q')", idx.c_str());
			m_sql.safe_query("DELETE FROM SceneTimers WHERE (SceneRowID == '%q')", idx.c_str());
		}

		void CWebServer::RType_UpdateScene(Json::Value &root)
		{
			if (m_pWebEm->m_actualuser_rights != 2)
				return;//Only admin user allowed

			std::string idx = m_pWebEm->FindValue("idx");
			std::string name = m_pWebEm->FindValue("name");
			std::string description = m_pWebEm->FindValue("description");
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


			root["status"] = "OK";
			root["title"] = "UpdateScene";
			m_sql.safe_query("UPDATE Scenes SET Name='%q', Description='%q', SceneType=%d, Protected=%d, OnAction='%q', OffAction='%q' WHERE (ID == '%q')",
				name.c_str(),
				description.c_str(),
				atoi(stype.c_str()),
				iProtected,
				onaction.c_str(),
				offaction.c_str(),
				idx.c_str()
				);
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

			std::vector<std::vector<std::string> > result, result2;
			result = m_sql.safe_query("SELECT ID, Name, [Order] FROM Plans ORDER BY [Order]");
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

						result2 = m_sql.safe_query("SELECT COUNT(*) FROM DeviceToPlansMap WHERE (PlanID=='%q')",
							sd[0].c_str());
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

			std::vector<std::vector<std::string> > result, result2, result3;

			result = m_sql.safe_query("SELECT Key, nValue, sValue FROM Preferences WHERE Key LIKE 'Floorplan%%'");
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

			result2 = m_sql.safe_query("SELECT ID, Name, ImageFile, ScaleFactor, [Order] FROM Floorplans ORDER BY [Order]");
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

					result3 = m_sql.safe_query("SELECT COUNT(*) FROM Plans WHERE (FloorplanID=='%q')",
						sd[0].c_str());
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

			std::vector<std::vector<std::string> > result, result2;
			result = m_sql.safe_query("SELECT ID, Name, Activators, Favorite, nValue, SceneType, LastUpdate, Protected, OnAction, OffAction, Description FROM Scenes ORDER BY [Order]");
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

					std::string onaction = base64_encode((const unsigned char*)sd[8].c_str(), sd[8].size());
					std::string offaction = base64_encode((const unsigned char*)sd[9].c_str(), sd[9].size());

					root["result"][ii]["idx"] = sd[0];
					root["result"][ii]["Name"] = sd[1];
					root["result"][ii]["Description"] = sd[10];
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
			if (!m_mainworker.m_LastSunriseSet.empty())
			{
				std::vector<std::string> strarray;
				StringSplit(m_mainworker.m_LastSunriseSet, ";", strarray);
				if (strarray.size() == 2)
				{
					char szTmp[100];
					//strftime(szTmp, 80, "%b %d %Y %X", &tm1);
					strftime(szTmp, 80, "%Y-%m-%d %X", &tm1);
					root["ServerTime"] = szTmp;
					root["Sunrise"] = strarray[0];
					root["Sunset"] = strarray[1];
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

			std::vector<std::vector<std::string> > result;
			result = m_sql.safe_query("SELECT ID, Name, Enabled, Type, Address, Port, SerialPort, Username, Password, Mode1, Mode2, Mode3, Mode4, Mode5, Mode6, DataTimeout FROM Hardware ORDER BY ID ASC");
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

			std::vector<std::vector<std::string> > result;
			result = m_sql.safe_query("SELECT ID, Active, Username, Password, Rights, RemoteSharing, TabsEnabled FROM USERS ORDER BY ID ASC");
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

		void CWebServer::Cmd_GetSceneActivations(Json::Value &root)
		{
			if (m_pWebEm->m_actualuser_rights != 2)
				return;//Only admin user allowed

			std::string idx = m_pWebEm->FindValue("idx");
			if (idx == "")
				return;

			root["status"] = "OK";
			root["title"] = "GetSceneActivations";

			std::vector<std::vector<std::string> > result, result2;
			result = m_sql.safe_query("SELECT Activators FROM Scenes WHERE (ID==%q)", idx.c_str());
			if (result.empty())
				return;
			int ii = 0;
			std::string Activators = result[0][0];
			if (!Activators.empty())
			{
				//Get Activator device names
				std::vector<std::string> arrayActivators;
				StringSplit(Activators, ";", arrayActivators);
				std::vector<std::string>::const_iterator ittAct;
				for (ittAct = arrayActivators.begin(); ittAct != arrayActivators.end(); ++ittAct)
				{
					std::string ID = *ittAct;
					result2 = m_sql.safe_query("SELECT Name FROM DeviceStatus WHERE (ID==%q)", ID.c_str());
					if (result2.size() > 0)
					{
						std::stringstream sstr;
						unsigned long long dID;
						sstr << ID;
						sstr >> dID;
						root["result"][ii]["idx"] = dID;
						root["result"][ii]["name"] = result2[0][0];
						ii++;
					}
				}
			}
		}

		void CWebServer::Cmd_AddSceneCode(Json::Value &root)
		{
			if (m_pWebEm->m_actualuser_rights != 2)
				return;//Only admin user allowed

			std::string sceneidx = m_pWebEm->FindValue("sceneidx");
			std::string idx = m_pWebEm->FindValue("idx");
			std::string cmnd = m_pWebEm->FindValue("cmnd");
			if (
				(sceneidx == "") ||
				(idx == "") ||
				(cmnd == "")
				)
				return;
			root["status"] = "OK";
			root["title"] = "AddSceneCode";

			//First check if we do not already have this device as activation code
			bool bFound = false;
			std::vector<std::vector<std::string> > result, result2;
			result = m_sql.safe_query("SELECT Activators FROM Scenes WHERE (ID==%q)", sceneidx.c_str());
			if (result.empty())
				return;
			int ii = 0;
			std::string Activators = result[0][0];
			if (!Activators.empty())
			{
				//Get Activator device names
				std::vector<std::string> arrayActivators;
				StringSplit(Activators, ";", arrayActivators);
				std::vector<std::string>::const_iterator ittAct;
				for (ittAct = arrayActivators.begin(); ittAct != arrayActivators.end(); ++ittAct)
				{
					std::string ID = *ittAct;
					if (ID == idx)
					{
						return; //already there!
					}
				}
			}
			if (!Activators.empty())
				Activators += ";";
			Activators += idx;
			m_sql.safe_query("UPDATE Scenes SET Activators='%q' WHERE (ID==%q)", Activators.c_str(), sceneidx.c_str());
		}

		void CWebServer::Cmd_RemoveSceneCode(Json::Value &root)
		{
			if (m_pWebEm->m_actualuser_rights != 2)
				return;//Only admin user allowed

			std::string sceneidx = m_pWebEm->FindValue("sceneidx");
			std::string idx = m_pWebEm->FindValue("idx");
			if (
				(idx == "") ||
				(sceneidx == "")
				)
				return;
			root["status"] = "OK";
			root["title"] = "RemoveSceneCode";

			std::vector<std::vector<std::string> > result, result2;
			result = m_sql.safe_query("SELECT Activators FROM Scenes WHERE (ID==%q)", sceneidx.c_str());
			if (result.empty())
				return;
			int ii = 0;
			std::string Activators = result[0][0];
			if (!Activators.empty())
			{
				//Get Activator device names
				std::vector<std::string> arrayActivators;
				StringSplit(Activators, ";", arrayActivators);
				std::vector<std::string>::const_iterator ittAct;
				std::string newActivation = "";
				for (ittAct = arrayActivators.begin(); ittAct != arrayActivators.end(); ++ittAct)
				{
					std::string ID = *ittAct;
					if (ID != idx)
					{
						if (!newActivation.empty())
							newActivation += ";";
						newActivation += ID;
					}
				}
				if (Activators != newActivation)
				{
					m_sql.safe_query("UPDATE Scenes SET Activators='%q' WHERE (ID==%q)", newActivation.c_str(), sceneidx.c_str());
				}
			}
		}

		void CWebServer::Cmd_ClearSceneCodes(Json::Value &root)
		{
			if (m_pWebEm->m_actualuser_rights != 2)
				return;//Only admin user allowed

			std::string sceneidx = m_pWebEm->FindValue("sceneidx");
			if (sceneidx == "")
				return;
			root["status"] = "OK";
			root["title"] = "ClearSceneCode";

			m_sql.safe_query("UPDATE Scenes SET Activators='' WHERE (ID==%q)", sceneidx.c_str());
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
			result = m_sql.safe_query("SELECT ID, Name FROM DeviceStatus WHERE (Used == 1) ORDER BY Name");
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
			result = m_sql.safe_query("SELECT ID, Name, Type, SubType FROM DeviceStatus WHERE (Used == 1) ORDER BY Name");
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
				return;//Only admin user allowed

			std::string sidx = m_pWebEm->FindValue("idx");
			if (sidx == "")
				return;
			int idx = atoi(sidx.c_str());
			root["status"] = "OK";
			root["title"] = "DeleteCustomIcon";

			m_sql.safe_query("DELETE FROM CustomImages WHERE (ID == %d)", idx);

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

		void CWebServer::Cmd_UpdateCustomIcon(Json::Value &root)
		{
			if (m_pWebEm->m_actualuser_rights != 2)
				return;//Only admin user allowed

			std::string sidx = m_pWebEm->FindValue("idx");
			std::string sname = m_pWebEm->FindValue("name");
			std::string sdescription = m_pWebEm->FindValue("description");
			if (
				(sidx.empty()) ||
				(sname.empty()) ||
				(sdescription.empty())
				)
				return;

			int idx = atoi(sidx.c_str());
			root["status"] = "OK";
			root["title"] = "UpdateCustomIcon";

			m_sql.safe_query("UPDATE CustomImages SET Name='%q', Description='%q' WHERE (ID == %d)", sname.c_str(), sdescription.c_str(), idx);
			ReloadCustomSwitchIcons();
		}

		void CWebServer::Cmd_RenameDevice(Json::Value &root)
		{
			if (m_pWebEm->m_actualuser_rights != 2)
				return;//Only admin user allowed

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

			std::vector<std::vector<std::string> > result;
			m_sql.safe_query("UPDATE DeviceStatus SET Name='%q' WHERE (ID == %d)", sname.c_str(), idx);
		}

		void CWebServer::Cmd_SetUnused(Json::Value &root)
		{
			if (m_pWebEm->m_actualuser_rights != 2)
				return;//Only admin user allowed

			std::string sidx = m_pWebEm->FindValue("idx");
			if (sidx.empty())
				return;
			int idx = atoi(sidx.c_str());
			root["status"] = "OK";
			root["title"] = "SetUnused";
			m_sql.safe_query("UPDATE DeviceStatus SET Used=0 WHERE (ID == %d)", idx);
		}

		void CWebServer::Cmd_AddLogMessage(Json::Value &root)
		{
			std::string smessage = m_pWebEm->FindValue("message");
			if (smessage.empty())
				return;
			root["status"] = "OK";
			root["title"] = "AddLogMessage";

			_log.Log(LOG_STATUS, "%s", smessage.c_str());
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

			std::vector<std::vector<std::string> > result;
			result = m_sql.safe_query("SELECT Type, SubType FROM DeviceStatus WHERE (ID==%llu)",
				idx);
			if (result.size() > 0)
			{
				int dType = atoi(result[0][0].c_str());
				if (
					(dType == pTypeTEMP) ||
					(dType == pTypeTEMP_HUM)
					)
				{
					result = m_sql.safe_query(
						"SELECT ID, Name FROM DeviceStatus WHERE (Type=='%q') AND (ID!=%llu)",
						result[0][0].c_str(), idx);
				}
				else
				{
					result = m_sql.safe_query(
						"SELECT ID, Name FROM DeviceStatus WHERE (Type=='%q') AND (SubType=='%q') AND (ID!=%llu)",
						result[0][0].c_str(), result[0][1].c_str(), idx);
				}

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

			std::vector<std::vector<std::string> > result;

			//Check which device is newer

			time_t now = mytime(NULL);
			struct tm tm1;
			localtime_r(&now, &tm1);
			struct tm LastUpdateTime_A;
			struct tm LastUpdateTime_B;

			result = m_sql.safe_query(
				"SELECT A.LastUpdate, B.LastUpdate FROM DeviceStatus as A, DeviceStatus as B WHERE (A.ID == '%q') AND (B.ID == '%q')",
				sidx.c_str(), newidx.c_str());
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

			result = m_sql.safe_query(
				"SELECT HardwareID, DeviceID, Unit, Name, Type, SubType, SignalLevel, BatteryLevel, nValue, sValue FROM DeviceStatus WHERE (ID == '%q')",
				newidx.c_str());
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
			std::vector<_tNotification> notifications = m_notifications.GetNotifications(idx);
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
					root["result"][ii]["SendAlways"] = itt->SendAlways;
					root["result"][ii]["CustomMessage"] = itt->CustomMessage;
					root["result"][ii]["ActiveSystems"] = itt->ActiveSystems;
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

			std::vector<std::vector<std::string> > result;
			result = m_sql.safe_query("SELECT DeviceRowID FROM SharedDevices WHERE (SharedUserID == '%q')",
				idx.c_str());
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
			m_sql.safe_query("DELETE FROM SharedDevices WHERE (SharedUserID == '%q')", idx.c_str());

			int nDevices = static_cast<int>(strarray.size());
			for (int ii = 0; ii < nDevices; ii++)
			{
				m_sql.safe_query("INSERT INTO SharedDevices (SharedUserID,DeviceRowID) VALUES ('%q','%q')", idx.c_str(), strarray[ii].c_str());
			}
			m_mainworker.LoadSharedUsers();
		}

		void CWebServer::RType_SetUsed(Json::Value &root)
		{
			if (m_pWebEm->m_actualuser_rights != 2)
				return;//Only admin user allowed

			std::string idx = m_pWebEm->FindValue("idx");
			std::string deviceid = m_pWebEm->FindValue("deviceid");
			std::string name = m_pWebEm->FindValue("name");
			std::string description = m_pWebEm->FindValue("description");
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

			//Strip trailing spaces in 'description'
			description = stdstring_trim(description);

			std::stringstream sstridx(idx);
			unsigned long long ullidx;
			sstridx >> ullidx;
			m_mainworker.m_eventsystem.WWWUpdateSingleState(ullidx, name);

			std::vector<std::vector<std::string> > result;

			result=m_sql.safe_query("SELECT Type,SubType FROM DeviceStatus WHERE (ID == '%q')",
				idx.c_str());
			if (result.size()<1)
				return;
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
					m_sql.safe_query("UPDATE DeviceStatus SET Used=%d, sValue='%q' WHERE (ID == '%q')",
						used, szTmp, idx.c_str());
				}
			}
			if (name == "")
			{
				m_sql.safe_query("UPDATE DeviceStatus SET Used=%d WHERE (ID == '%q')",
					used, idx.c_str());
			}
			else
			{
				if (switchtype == -1)
					m_sql.safe_query("UPDATE DeviceStatus SET Used=%d, Name='%q', Description='%q' WHERE (ID == '%q')",
					used, name.c_str(), description.c_str(), idx.c_str());
				else
					m_sql.safe_query(
					"UPDATE DeviceStatus SET Used=%d, Name='%q', Description='%q', SwitchType=%d, CustomImage=%d WHERE (ID == '%q')",
					used, name.c_str(), description.c_str(), switchtype, CustomImage, idx.c_str());
			}

			if (bHasstrParam1)
			{
				m_sql.safe_query("UPDATE DeviceStatus SET StrParam1='%q', StrParam2='%q' WHERE (ID == '%q')",
					strParam1.c_str(), strParam2.c_str(), idx.c_str());
			}

			m_sql.safe_query("UPDATE DeviceStatus SET Protected=%d WHERE (ID == '%q')", iProtected, idx.c_str());

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
				m_sql.safe_query("UPDATE DeviceStatus SET Unit='%q' WHERE (ID == '%q')",
					strunit.c_str(), idx.c_str());
			}
			//FIXME evohome ...we need the zone id to update the correct zone...but this should be ok as a generic call?
			if (deviceid != "")
			{
				m_sql.safe_query("UPDATE DeviceStatus SET DeviceID='%q' WHERE (ID == '%q')",
					deviceid.c_str(), idx.c_str());
			}
			if (addjvalue != "")
			{
				double faddjvalue = atof(addjvalue.c_str());
				m_sql.safe_query("UPDATE DeviceStatus SET AddjValue=%f WHERE (ID == '%q')",
					faddjvalue, idx.c_str());
			}
			if (addjmulti != "")
			{
				double faddjmulti = atof(addjmulti.c_str());
				if (faddjmulti == 0)
					faddjmulti = 1;
				m_sql.safe_query("UPDATE DeviceStatus SET AddjMulti=%f WHERE (ID == '%q')",
					faddjmulti, idx.c_str());
			}
			if (addjvalue2 != "")
			{
				double faddjvalue2 = atof(addjvalue2.c_str());
				m_sql.safe_query("UPDATE DeviceStatus SET AddjValue2=%f WHERE (ID == '%q')",
					faddjvalue2, idx.c_str());
			}
			if (addjmulti2 != "")
			{
				double faddjmulti2 = atof(addjmulti2.c_str());
				if (faddjmulti2 == 0)
					faddjmulti2 = 1;
				m_sql.safe_query("UPDATE DeviceStatus SET AddjMulti2=%f WHERE (ID == '%q')",
					faddjmulti2, idx.c_str());
			}

			if (used == 0)
			{
				bool bRemoveSubDevices = (m_pWebEm->FindValue("RemoveSubDevices") == "true");

				if (bRemoveSubDevices)
				{
					//if this device was a slave device, remove it
					m_sql.safe_query("DELETE FROM LightSubDevices WHERE (DeviceRowID == '%q')", idx.c_str());
				}
				m_sql.safe_query("DELETE FROM LightSubDevices WHERE (ParentID == '%q')", idx.c_str());

				m_sql.safe_query("DELETE FROM Timers WHERE (DeviceRowID == '%q')", idx.c_str());
			}

			if (maindeviceidx != "")
			{
				if (maindeviceidx != idx)
				{
					//this is a sub device for another light/switch
					//first check if it is not already a sub device
					result = m_sql.safe_query("SELECT ID FROM LightSubDevices WHERE (DeviceRowID=='%q') AND (ParentID =='%q')",
						idx.c_str(), maindeviceidx.c_str());
					if (result.size() == 0)
					{
						//no it is not, add it
						m_sql.safe_query(
							"INSERT INTO LightSubDevices (DeviceRowID, ParentID) VALUES ('%q','%q')",
							idx.c_str(),
							maindeviceidx.c_str()
							);
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

		void CWebServer::RType_Settings(Json::Value &root)
		{
			std::vector<std::vector<std::string> > result;
			char szTmp[100];

			result = m_sql.safe_query("SELECT Key, nValue, sValue FROM Preferences");
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
				else if (Key == "EnableTabCustom")
				{
					root["EnableTabCustom"] = nValue;
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
				else if (Key == "ShowUpdateEffect")
				{
					root["ShowUpdateEffect"] = nValue;
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
			//First get Device Type/SubType
			result = m_sql.safe_query("SELECT Type, SubType, SwitchType FROM DeviceStatus WHERE (ID == %llu)",
				idx);
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

			result = m_sql.safe_query("SELECT ROWID, nValue, sValue, Date FROM LightingLog WHERE (DeviceRowID==%llu) ORDER BY Date DESC",
				idx);
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

					if (switchtype != STYPE_Media)
					{
						GetLightStatus(dType, dSubType, switchtype, nValue, sValue, lstatus, llevel, bHaveDimmer, maxDimLevel, bHaveGroupCmd);
					}
					else
					{
						lstatus = sValue;
					}

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

			result = m_sql.safe_query("SELECT ROWID, sValue, Date FROM LightingLog WHERE (DeviceRowID==%llu) ORDER BY Date DESC",
				idx);
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

			result = m_sql.safe_query("SELECT Type, SubType, SwitchType, AddjValue, AddjMulti FROM DeviceStatus WHERE (ID == %llu)",
				idx);
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

					result = m_sql.safe_query("SELECT Temperature, Chill, Humidity, Barometer, Date, SetPoint FROM %s WHERE (DeviceRowID==%llu) ORDER BY Date ASC", dbasetable.c_str(), idx);
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
								(dType == pTypeRadiator1) ||
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

					result = m_sql.safe_query("SELECT Percentage, Date FROM %s WHERE (DeviceRowID==%llu) ORDER BY Date ASC", dbasetable.c_str(), idx);
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

					result = m_sql.safe_query("SELECT Speed, Date FROM %s WHERE (DeviceRowID==%llu) ORDER BY Date ASC", dbasetable.c_str(), idx);
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

						result = m_sql.safe_query("SELECT Value1, Value2, Value3, Value4, Value5, Value6, Date FROM %s WHERE (DeviceRowID==%llu) ORDER BY Date ASC", dbasetable.c_str(), idx);
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
											std::vector<std::vector<std::string> > result2;
											result2 = m_sql.safe_query(
												"SELECT Counter1, Counter2, Counter3, Counter4 FROM Multimeter_Calendar WHERE (DeviceRowID==%llu) AND (Date=='%q')",
												idx, szTmp);
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

						result = m_sql.safe_query("SELECT Value, Date FROM %s WHERE (DeviceRowID==%llu) ORDER BY Date ASC", dbasetable.c_str(), idx);
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

						result = m_sql.safe_query("SELECT Value, Date FROM %s WHERE (DeviceRowID==%llu) ORDER BY Date ASC", dbasetable.c_str(), idx);
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
						result = m_sql.safe_query("SELECT Value, Date FROM %s WHERE (DeviceRowID==%llu) ORDER BY Date ASC", dbasetable.c_str(), idx);
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

						result = m_sql.safe_query("SELECT Value, Date FROM %s WHERE (DeviceRowID==%llu) ORDER BY Date ASC", dbasetable.c_str(), idx);
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

						result = m_sql.safe_query("SELECT Value, Date FROM %s WHERE (DeviceRowID==%llu) ORDER BY Date ASC", dbasetable.c_str(), idx);
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

						result = m_sql.safe_query("SELECT Value, Date FROM %s WHERE (DeviceRowID==%llu) ORDER BY Date ASC", dbasetable.c_str(), idx);
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

						result = m_sql.safe_query("SELECT Value, Date FROM %s WHERE (DeviceRowID==%llu) ORDER BY Date ASC", dbasetable.c_str(), idx);
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

						result = m_sql.safe_query("SELECT Value1, Value2, Value3, Date FROM %s WHERE (DeviceRowID==%llu) ORDER BY Date ASC", dbasetable.c_str(), idx);
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

						result = m_sql.safe_query("SELECT Value1, Value2, Value3, Date FROM %s WHERE (DeviceRowID==%llu) ORDER BY Date ASC", dbasetable.c_str(), idx);
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
						bool bHaveUsage = true;
						result = m_sql.safe_query("SELECT MIN([Usage]), MAX([Usage]) FROM %s WHERE (DeviceRowID==%llu)", dbasetable.c_str(), idx);
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

						int ii = 0;
						result = m_sql.safe_query("SELECT Value,[Usage], Date FROM %s WHERE (DeviceRowID==%llu) ORDER BY Date ASC", dbasetable.c_str(), idx);

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
												case MTYPE_ENERGY_GENERATED:
													sprintf(szTmp, "%.3f", (TotalValue / EnergyDivider)*1000.0f);	//from kWh -> Watt
													break;
												case MTYPE_GAS:
													sprintf(szTmp, "%.2f", TotalValue / GasDivider);
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
									case MTYPE_ENERGY_GENERATED:
										sprintf(szTmp, "%.3f", (TotalValue / EnergyDivider)*1000.0f);	//from kWh -> Watt
										break;
									case MTYPE_GAS:
										sprintf(szTmp, "%.2f", TotalValue / GasDivider);
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

						int ii = 0;
						result = m_sql.safe_query("SELECT Value, Date FROM %s WHERE (DeviceRowID==%llu) ORDER BY Date ASC", dbasetable.c_str(), idx);

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
												case MTYPE_ENERGY_GENERATED:
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
											case MTYPE_ENERGY_GENERATED:
												sprintf(szTmp, "%.3f", (TotalValue / EnergyDivider)*1000.0f);	//from kWh -> Watt
												break;
											case MTYPE_GAS:
												sprintf(szTmp, "%.2f", TotalValue / GasDivider);
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
								case MTYPE_ENERGY_GENERATED:
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

					result = m_sql.safe_query("SELECT Level, Date FROM %s WHERE (DeviceRowID==%llu) ORDER BY Date ASC", dbasetable.c_str(), idx);
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

					result = m_sql.safe_query("SELECT Total, Date FROM %s WHERE (DeviceRowID==%llu) ORDER BY Date ASC", dbasetable.c_str(), idx);
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

					result = m_sql.safe_query("SELECT Direction, Speed, Gust, Date FROM %s WHERE (DeviceRowID==%llu) ORDER BY Date ASC", dbasetable.c_str(), idx);
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

					result = m_sql.safe_query("SELECT Direction, Speed, Gust FROM %s WHERE (DeviceRowID==%llu) ORDER BY Date ASC", dbasetable.c_str(), idx);
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

					result = m_sql.safe_query("SELECT Total, Rate, Date FROM %s WHERE (DeviceRowID==%llu AND Date>='%q' AND Date<='%q') ORDER BY Date ASC", dbasetable.c_str(), idx, szDateStart, szDateEnd);
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
					if (dSubType != sTypeRAINWU)
					{
						result = m_sql.safe_query(
							"SELECT MIN(Total), MAX(Total), MAX(Rate) FROM Rain WHERE (DeviceRowID=%llu AND Date>='%q')",
							idx, szDateEnd);
					}
					else
					{
						result = m_sql.safe_query(
							"SELECT Total, Total, Rate FROM Rain WHERE (DeviceRowID=%llu AND Date>='%q') ORDER BY ROWID DESC LIMIT 1",
							idx, szDateEnd);
					}
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

					int ii = 0;
					if (dType == pTypeP1Power)
					{
						result = m_sql.safe_query("SELECT Value1,Value2,Value5,Value6,Date FROM %s WHERE (DeviceRowID==%llu AND Date>='%q' AND Date<='%q') ORDER BY Date ASC", dbasetable.c_str(), idx, szDateStart, szDateEnd);
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
						result = m_sql.safe_query("SELECT Value, Date FROM %s WHERE (DeviceRowID==%llu AND Date>='%q' AND Date<='%q') ORDER BY Date ASC", dbasetable.c_str(), idx, szDateStart, szDateEnd);
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
								case MTYPE_ENERGY_GENERATED:
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
					if (dType == pTypeP1Power)
					{
						result = m_sql.safe_query(
							"SELECT MIN(Value1), MAX(Value1), MIN(Value2), MAX(Value2),MIN(Value5), MAX(Value5), MIN(Value6), MAX(Value6) FROM MultiMeter WHERE (DeviceRowID==%llu AND Date>='%q')",
							idx, szDateEnd);
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
						result = m_sql.safe_query("SELECT MIN(Value), MAX(Value) FROM Meter WHERE (DeviceRowID==%llu AND Date>='%q')",
							idx, szDateEnd);
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
							case MTYPE_ENERGY_GENERATED:
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
					result = m_sql.safe_query(
						"SELECT Temp_Min, Temp_Max, Chill_Min, Chill_Max,"
						" Humidity, Barometer, Temp_Avg, Date, SetPoint_Min,"
						" SetPoint_Max, SetPoint_Avg "
						"FROM %s WHERE (DeviceRowID==%llu AND Date>='%q'"
						" AND Date<='%q') ORDER BY Date ASC",
						dbasetable.c_str(), idx, szDateStart, szDateEnd);
					int ii = 0;
					if (result.size() > 0)
					{
						std::vector<std::vector<std::string> >::const_iterator itt;
						for (itt = result.begin(); itt != result.end(); ++itt)
						{
							std::vector<std::string> sd = *itt;

							root["result"][ii]["d"] = sd[7].substr(0, 16);

							if (
								(dType == pTypeRego6XXTemp) || (dType == pTypeTEMP) || (dType == pTypeTEMP_HUM) || (dType == pTypeTEMP_HUM_BARO) || (dType == pTypeTEMP_BARO) || (dType == pTypeWIND) || (dType == pTypeThermostat1) || (dType == pTypeRadiator1) ||
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
					result = m_sql.safe_query(
						"SELECT MIN(Temperature), MAX(Temperature),"
						" MIN(Chill), MAX(Chill), MAX(Humidity),"
						" MAX(Barometer), AVG(Temperature), MIN(SetPoint),"
						" MAX(SetPoint), AVG(SetPoint) "
						"FROM Temperature WHERE (DeviceRowID==%llu"
						" AND Date>='%q')",
						idx, szDateEnd);
					if (result.size() > 0)
					{
						std::vector<std::string> sd = result[0];

						root["result"][ii]["d"] = szDateEnd;
						if (
							((dType == pTypeRego6XXTemp) || (dType == pTypeTEMP) || (dType == pTypeTEMP_HUM) || (dType == pTypeTEMP_HUM_BARO) || (dType == pTypeTEMP_BARO) || (dType == pTypeWIND) || (dType == pTypeThermostat1) || (dType == pTypeRadiator1)) ||
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
					result = m_sql.safe_query(
						"SELECT Temp_Min, Temp_Max, Chill_Min, Chill_Max,"
						" Humidity, Barometer, Temp_Avg, Date, SetPoint_Min,"
						" SetPoint_Max, SetPoint_Avg "
						"FROM %s WHERE (DeviceRowID==%llu AND Date>='%q'"
						" AND Date<='%q') ORDER BY Date ASC",
						dbasetable.c_str(), idx, szDateStartPrev, szDateEndPrev);
					if (result.size() > 0)
					{
						int iPrev = 0;
						std::vector<std::vector<std::string> >::const_iterator itt;
						for (itt = result.begin(); itt != result.end(); ++itt)
						{
							std::vector<std::string> sd = *itt;

							root["resultprev"][iPrev]["d"] = sd[7].substr(0, 16);

							if (
								(dType == pTypeRego6XXTemp) || (dType == pTypeTEMP) || (dType == pTypeTEMP_HUM) || (dType == pTypeTEMP_HUM_BARO) || (dType == pTypeTEMP_BARO) || (dType == pTypeWIND) || (dType == pTypeThermostat1) || (dType == pTypeRadiator1) ||
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

					result = m_sql.safe_query("SELECT Percentage_Min, Percentage_Max, Percentage_Avg, Date FROM %s WHERE (DeviceRowID==%llu AND Date>='%q' AND Date<='%q') ORDER BY Date ASC", dbasetable.c_str(), idx, szDateStart, szDateEnd);
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
					result = m_sql.safe_query(
						"SELECT MIN(Percentage), MAX(Percentage), AVG(Percentage) FROM Percentage WHERE (DeviceRowID=%llu AND Date>='%q')",
						idx, szDateEnd);
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

					result = m_sql.safe_query("SELECT Speed_Min, Speed_Max, Date FROM %s WHERE (DeviceRowID==%llu AND Date>='%q' AND Date<='%q') ORDER BY Date ASC", dbasetable.c_str(), idx, szDateStart, szDateEnd);
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
					result = m_sql.safe_query("SELECT MIN(Speed), MAX(Speed) FROM Fan WHERE (DeviceRowID=%llu AND Date>='%q')",
						idx, szDateEnd);
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

					result = m_sql.safe_query("SELECT Level, Date FROM %s WHERE (DeviceRowID==%llu AND Date>='%q' AND Date<='%q') ORDER BY Date ASC", dbasetable.c_str(), idx, szDateStart, szDateEnd);
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
					result = m_sql.safe_query(
						"SELECT MAX(Level) FROM UV WHERE (DeviceRowID=%llu AND Date>='%q')",
						idx, szDateEnd);
					if (result.size() > 0)
					{
						std::vector<std::string> sd = result[0];

						root["result"][ii]["d"] = szDateEnd;
						root["result"][ii]["uvi"] = sd[0];
						ii++;
					}
					//Previous Year
					result = m_sql.safe_query("SELECT Level, Date FROM %s WHERE (DeviceRowID==%llu AND Date>='%q' AND Date<='%q') ORDER BY Date ASC", dbasetable.c_str(), idx, szDateStartPrev, szDateEndPrev);
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

					result = m_sql.safe_query("SELECT Total, Rate, Date FROM %s WHERE (DeviceRowID==%llu AND Date>='%q' AND Date<='%q') ORDER BY Date ASC", dbasetable.c_str(), idx, szDateStart, szDateEnd);
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
					if (dSubType != sTypeRAINWU)
					{
						result = m_sql.safe_query(
							"SELECT MIN(Total), MAX(Total), MAX(Rate) FROM Rain WHERE (DeviceRowID=%llu AND Date>='%q')",
							idx, szDateEnd);
					}
					else
					{
						result = m_sql.safe_query(
							"SELECT Total, Total, Rate FROM Rain WHERE (DeviceRowID=%llu AND Date>='%q') ORDER BY ROWID DESC LIMIT 1",
							idx, szDateEnd);
					}
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
					result = m_sql.safe_query(
						"SELECT Total, Rate, Date FROM %s WHERE (DeviceRowID==%llu AND Date>='%q' AND Date<='%q') ORDER BY Date ASC", dbasetable.c_str(), idx, szDateStartPrev, szDateEndPrev);
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

					result = m_sql.safe_query("SELECT nValue, sValue FROM DeviceStatus WHERE (ID==%llu)",
						idx);
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

					int ii = 0;
					int iPrev = 0;
					if (dType == pTypeP1Power)
					{
						//Actual Year
						result = m_sql.safe_query(
							"SELECT Value1,Value2,Value5,Value6, Date,"
							" Counter1, Counter2, Counter3, Counter4 "
							"FROM %s WHERE (DeviceRowID==%llu AND Date>='%q'"
							" AND Date<='%q') ORDER BY Date ASC",
							dbasetable.c_str(), idx, szDateStart, szDateEnd);
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
						result = m_sql.safe_query(
							"SELECT Value1,Value2,Value5,Value6, Date "
							"FROM %s WHERE (DeviceRowID==%llu AND Date>='%q' AND Date<='%q') ORDER BY Date ASC",
							dbasetable.c_str(), idx, szDateStartPrev, szDateEndPrev);
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

						result = m_sql.safe_query("SELECT Value1,Value2, Date FROM %s WHERE (DeviceRowID==%llu AND Date>='%q' AND Date<='%q') ORDER BY Date ASC", dbasetable.c_str(), idx, szDateStart, szDateEnd);
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

						result = m_sql.safe_query("SELECT Value1,Value2, Date FROM %s WHERE (DeviceRowID==%llu AND Date>='%q' AND Date<='%q') ORDER BY Date ASC", dbasetable.c_str(), idx, szDateStart, szDateEnd);
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

						result = m_sql.safe_query("SELECT Value1,Value2, Date FROM %s WHERE (DeviceRowID==%llu AND Date>='%q' AND Date<='%q') ORDER BY Date ASC", dbasetable.c_str(), idx, szDateStart, szDateEnd);
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

						result = m_sql.safe_query("SELECT Value1,Value2, Date FROM %s WHERE (DeviceRowID==%llu AND Date>='%q' AND Date<='%q') ORDER BY Date ASC", dbasetable.c_str(), idx, szDateStart, szDateEnd);
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

						result = m_sql.safe_query(
							"SELECT Value1,Value2, Date FROM %s WHERE (DeviceRowID==%llu AND Date>='%q' AND Date<='%q') ORDER BY Date ASC", dbasetable.c_str(), idx, szDateStart, szDateEnd);
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

						result = m_sql.safe_query(
							"SELECT Value1,Value2, Date FROM %s WHERE (DeviceRowID==%llu AND Date>='%q' AND Date<='%q') ORDER BY Date ASC", dbasetable.c_str(), idx, szDateStart, szDateEnd);
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
						result = m_sql.safe_query("SELECT Value1,Value2,Value3,Value4,Value5,Value6, Date FROM %s WHERE (DeviceRowID==%llu AND Date>='%q' AND Date<='%q') ORDER BY Date ASC", dbasetable.c_str(), idx, szDateStart, szDateEnd);
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
						result = m_sql.safe_query("SELECT Value1,Value2,Value3,Value4,Value5,Value6, Date FROM %s WHERE (DeviceRowID==%llu AND Date>='%q' AND Date<='%q') ORDER BY Date ASC", dbasetable.c_str(), idx, szDateStart, szDateEnd);
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
							case MTYPE_ENERGY_GENERATED:
								sprintf(szTmp, "%.3f", fvalue / EnergyDivider);
								break;
							case MTYPE_GAS:
								sprintf(szTmp, "%.2f", fvalue / GasDivider);
								break;
							case MTYPE_WATER:
								sprintf(szTmp, "%.3f", fvalue / WaterDivider);
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
								case MTYPE_ENERGY_GENERATED:
									sprintf(szTmp, "%.3f", fvalue / EnergyDivider);
									break;
								case MTYPE_GAS:
									sprintf(szTmp, "%.2f", fvalue / GasDivider);
									break;
								case MTYPE_WATER:
									sprintf(szTmp, "%.3f", fvalue / WaterDivider);
									break;
								}
								root["counter"] = szTmp;
							}
						}
						//Actual Year
						result = m_sql.safe_query("SELECT Value, Date, Counter FROM %s WHERE (DeviceRowID==%llu AND Date>='%q' AND Date<='%q') ORDER BY Date ASC", dbasetable.c_str(), idx, szDateStart, szDateEnd);
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
								case MTYPE_ENERGY_GENERATED:
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
									sprintf(szTmp, "%.3f", atof(szValue.c_str()) / WaterDivider);
									root["result"][ii]["v"] = szTmp;
									if (fcounter != 0)
										sprintf(szTmp, "%.3f", (fcounter - atof(szValue.c_str())) / WaterDivider);
									else
										strcpy(szTmp, "0");
									root["result"][ii]["c"] = szTmp;
									break;
								}
								ii++;
							}
						}
						//Past Year
						result = m_sql.safe_query("SELECT Value, Date, Counter FROM %s WHERE (DeviceRowID==%llu AND Date>='%q' AND Date<='%q') ORDER BY Date ASC", dbasetable.c_str(), idx, szDateStartPrev, szDateEndPrev);
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
								case MTYPE_ENERGY_GENERATED:
									sprintf(szTmp, "%.3f", atof(szValue.c_str()) / EnergyDivider);
									root["resultprev"][iPrev]["v"] = szTmp;
									break;
								case MTYPE_GAS:
									sprintf(szTmp, "%.2f", atof(szValue.c_str()) / GasDivider);
									root["resultprev"][iPrev]["v"] = szTmp;
									break;
								case MTYPE_WATER:
									sprintf(szTmp, "%.3f", atof(szValue.c_str()) / WaterDivider);
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

					if (dType == pTypeP1Power)
					{
						result = m_sql.safe_query(
							"SELECT MIN(Value1), MAX(Value1), MIN(Value2),"
							" MAX(Value2), MIN(Value5), MAX(Value5),"
							" MIN(Value6), MAX(Value6) "
							"FROM MultiMeter WHERE (DeviceRowID=%llu"
							" AND Date>='%q')",
							idx, szDateEnd);
						bool bHaveDeliverd = false;
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
						result = m_sql.safe_query(
							"SELECT MIN(Value), MAX(Value) FROM Meter WHERE (DeviceRowID==%llu AND Date>='%q')",
							idx, szDateEnd);
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
						result = m_sql.safe_query(
							"SELECT MIN(Value), MAX(Value) FROM Meter WHERE (DeviceRowID==%llu AND Date>='%q')",
							idx, szDateEnd);
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

						result = m_sql.safe_query(
							"SELECT MIN(Value), MAX(Value) FROM Meter WHERE (DeviceRowID==%llu AND Date>='%q')",
							idx, szDateEnd);
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
						result = m_sql.safe_query(
							"SELECT MIN(Value), MAX(Value) FROM Meter WHERE (DeviceRowID==%llu AND Date>='%q')",
							idx, szDateEnd);
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
						result = m_sql.safe_query(
							"SELECT MIN(Value), MAX(Value) FROM Meter WHERE (DeviceRowID==%llu AND Date>='%q')",
							idx, szDateEnd);
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
						result = m_sql.safe_query(
							"SELECT MIN(Value), MAX(Value) FROM Meter WHERE (DeviceRowID=%llu AND Date>='%q')",
							idx, szDateEnd);
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
						result = m_sql.safe_query(
							"SELECT MIN(Value), MAX(Value) FROM Meter WHERE (DeviceRowID==%llu AND Date>='%q')",
							idx, szDateEnd);
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
							case MTYPE_ENERGY_GENERATED:
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
								sprintf(szTmp, "%.3f", atof(szValue.c_str()) / WaterDivider);
								root["result"][ii]["v"] = szTmp;
								sprintf(szTmp, "%.3f", (atof(sValue.c_str()) - atof(szValue.c_str())) / WaterDivider);
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

					result = m_sql.safe_query(
						"SELECT Direction, Speed_Min, Speed_Max, Gust_Min,"
						" Gust_Max, Date "
						"FROM %s WHERE (DeviceRowID==%llu AND Date>='%q'"
						" AND Date<='%q') ORDER BY Date ASC",
						dbasetable.c_str(), idx, szDateStart, szDateEnd);
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
					result = m_sql.safe_query(
						"SELECT AVG(Direction), MIN(Speed), MAX(Speed),"
						" MIN(Gust), MAX(Gust) "
						"FROM Wind WHERE (DeviceRowID==%llu AND Date>='%q') ORDER BY Date ASC",
						idx, szDateEnd);
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
						((dType == pTypeRego6XXTemp) || (dType == pTypeTEMP) || (dType == pTypeTEMP_HUM) || (dType == pTypeTEMP_HUM_BARO) || (dType == pTypeTEMP_BARO) || (dType == pTypeWIND) || (dType == pTypeThermostat1) || (dType == pTypeRadiator1) ||
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

					if (sgraphtype == "1")
					{
						// Need to get all values of the end date so 23:59:59 is appended to the date string
						result = m_sql.safe_query(
							"SELECT Temperature, Chill, Humidity, Barometer,"
							" Date, DewPoint, SetPoint "
							"FROM Temperature WHERE (DeviceRowID==%llu"
							" AND Date>='%q' AND Date<='%q 23:59:59') ORDER BY Date ASC",
							idx, szDateStart.c_str(), szDateEnd.c_str());
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
						result = m_sql.safe_query(
							"SELECT Temp_Min, Temp_Max, Chill_Min, Chill_Max,"
							" Humidity, Barometer, Date, DewPoint, Temp_Avg,"
							" SetPoint_Min, SetPoint_Max, SetPoint_Avg "
							"FROM Temperature_Calendar "
							"WHERE (DeviceRowID==%llu AND Date>='%q'"
							" AND Date<='%q') ORDER BY Date ASC",
							idx, szDateStart.c_str(), szDateEnd.c_str());
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
						result = m_sql.safe_query(
							"SELECT MIN(Temperature), MAX(Temperature),"
							" MIN(Chill), MAX(Chill), MAX(Humidity),"
							" MAX(Barometer), MIN(DewPoint), AVG(Temperature),"
							" MIN(SetPoint), MAX(SetPoint), AVG(SetPoint) "
							"FROM Temperature WHERE (DeviceRowID==%llu AND Date>='%q')",
							idx, szDateEnd.c_str());
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

					result = m_sql.safe_query(
						"SELECT Level, Date FROM %s WHERE (DeviceRowID==%llu"
						" AND Date>='%q' AND Date<='%q') ORDER BY Date ASC",
						dbasetable.c_str(), idx, szDateStart.c_str(), szDateEnd.c_str());
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
					result = m_sql.safe_query(
						"SELECT MAX(Level) FROM UV WHERE (DeviceRowID==%llu AND Date>='%q')",
						idx, szDateEnd.c_str());
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

					result = m_sql.safe_query(
						"SELECT Total, Rate, Date FROM %s "
						"WHERE (DeviceRowID==%llu AND Date>='%q' AND Date<='%q') ORDER BY Date ASC",
						dbasetable.c_str(), idx, szDateStart.c_str(), szDateEnd.c_str());
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
					if (dSubType != sTypeRAINWU)
					{
						result = m_sql.safe_query(
							"SELECT MIN(Total), MAX(Total), MAX(Rate) FROM Rain WHERE (DeviceRowID==%llu AND Date>='%q')",
							idx, szDateEnd.c_str());
					}
					else
					{
						result = m_sql.safe_query(
							"SELECT Total, Total, Rate FROM Rain WHERE (DeviceRowID==%llu AND Date>='%q') ORDER BY ROWID DESC LIMIT 1",
							idx, szDateEnd.c_str());
					}
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

					int ii = 0;
					if (dType == pTypeP1Power)
					{
						result = m_sql.safe_query(
							"SELECT Value1,Value2,Value5,Value6, Date "
							"FROM %s WHERE (DeviceRowID==%llu AND Date>='%q'"
							" AND Date<='%q') ORDER BY Date ASC",
							dbasetable.c_str(), idx, szDateStart.c_str(), szDateEnd.c_str());
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
						result = m_sql.safe_query("SELECT Value, Date FROM %s WHERE (DeviceRowID==%llu AND Date>='%q' AND Date<='%q') ORDER BY Date ASC", dbasetable.c_str(), idx, szDateStart.c_str(), szDateEnd.c_str());
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
								case MTYPE_ENERGY_GENERATED:
									sprintf(szTmp, "%.3f", atof(szValue.c_str()) / EnergyDivider);
									szValue = szTmp;
									break;
								case MTYPE_GAS:
									sprintf(szTmp, "%.2f", atof(szValue.c_str()) / GasDivider);
									szValue = szTmp;
									break;
								case MTYPE_WATER:
									sprintf(szTmp, "%.3f", atof(szValue.c_str()) / WaterDivider);
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
					if (dType == pTypeP1Power)
					{
						result = m_sql.safe_query(
							"SELECT MIN(Value1), MAX(Value1), MIN(Value2),"
							" MAX(Value2),MIN(Value5), MAX(Value5),"
							" MIN(Value6), MAX(Value6) "
							"FROM MultiMeter WHERE (DeviceRowID==%llu AND Date>='%q')",
							idx, szDateEnd.c_str());
						bool bHaveDeliverd = false;
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
						result = m_sql.safe_query(
							"SELECT MIN(Value), MAX(Value) FROM Meter WHERE (DeviceRowID==%llu AND Date>='%q')",
							idx, szDateEnd.c_str());
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
							case MTYPE_ENERGY_GENERATED:
								sprintf(szTmp, "%.3f", atof(szValue.c_str()) / EnergyDivider);
								szValue = szTmp;
								break;
							case MTYPE_GAS:
								sprintf(szTmp, "%.2f", atof(szValue.c_str()) / GasDivider);
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
				else if (sensor == "wind") {
					root["status"] = "OK";
					root["title"] = "Graph " + sensor + " " + srange;

					int ii = 0;

					result = m_sql.safe_query(
						"SELECT Direction, Speed_Min, Speed_Max, Gust_Min,"
						" Gust_Max, Date "
						"FROM %s WHERE (DeviceRowID==%llu AND Date>='%q'"
						" AND Date<='%q') ORDER BY Date ASC",
						dbasetable.c_str(), idx, szDateStart.c_str(), szDateEnd.c_str());
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
					result = m_sql.safe_query(
						"SELECT AVG(Direction), MIN(Speed), MAX(Speed), MIN(Gust), MAX(Gust) FROM Wind WHERE (DeviceRowID==%llu AND Date>='%q') ORDER BY Date ASC",
						idx, szDateEnd.c_str());
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

