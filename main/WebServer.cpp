#include "stdafx.h"
#include <iostream>
#include <fstream>
#include <stdarg.h>
#include <json/json.h>
#include <algorithm>
#include "WebServer.h"
#include "WebServerHelper.h"
#include "mainworker.h"
#include "Helper.h"
#include "EventSystem.h"
#include "HTMLSanitizer.h"
#include "dzVents.h"
#include "json_helper.h"
#include "LuaHandler.h"
#include "Logger.h"
#include "SQLHelper.h"
#include "../httpclient/HTTPClient.h"
#include "../hardware/hardwaretypes.h"
#include "../webserver/Base64.h"
#include "../smtpclient/SMTPClient.h"
#include "../push/BasePush.h"
#include "../notifications/NotificationHelper.h"

#ifdef ENABLE_PYTHON
#include "../hardware/plugins/Plugins.h"
#endif

#ifndef WIN32
#include <sys/utsname.h>
#include <dirent.h>
#else
#include "../msbuild/WindowsHelper.h"
#include "dirent_windows.h"
#endif

#define __STDC_FORMAT_MACROS
#include <inttypes.h>

// Some Hardware related includes
#include "../hardware/AccuWeather.h"
#include "../hardware/Buienradar.h"
#include "../hardware/DarkSky.h"
#include "../hardware/VisualCrossing.h"
#include "../hardware/Meteorologisk.h"
#include "../hardware/OpenWeatherMap.h"
#include "../hardware/Wunderground.h"

#include "../hardware/Limitless.h"

#define round(a) (int)(a + .5)

extern std::string szStartupFolder;
extern std::string szUserDataFolder;
extern std::string szWWWFolder;

extern std::string szAppVersion;
extern int iAppRevision;
extern std::string szAppHash;
extern std::string szAppDate;
extern std::string szPyVersion;

namespace http
{
	namespace server
	{
		struct _tHardwareListInt
		{
			std::string Name;
			int HardwareTypeVal;
			std::string HardwareType;
			bool Enabled;
			std::string Mode1; // Used to flag DimmerType as relative for some old LimitLessLight type bulbs
			std::string Mode2; // Used to flag DimmerType as relative for some old LimitLessLight type bulbs
		};

		CWebServer::CWebServer()
		{
			m_pWebEm = nullptr;
			m_bDoStop = false;
			m_failcount = 0;
		}

		CWebServer::~CWebServer()
		{
			// RK, we call StopServer() instead of just deleting m_pWebEm. The Do_Work thread might still be accessing that object
			StopServer();
		}

		void CWebServer::Do_Work()
		{
			bool exception_thrown = false;
			while (!m_bDoStop)
			{
				exception_thrown = false;
				try
				{
					if (m_pWebEm)
					{
						m_pWebEm->Run();
					}
				}
				catch (std::exception& e)
				{
					_log.Log(LOG_ERROR, "WebServer(%s) exception occurred : '%s'", m_server_alias.c_str(), e.what());
					exception_thrown = true;
				}
				catch (...)
				{
					_log.Log(LOG_ERROR, "WebServer(%s) unknown exception occurred", m_server_alias.c_str());
					exception_thrown = true;
				}
				if (exception_thrown)
				{
					_log.Log(LOG_STATUS, "WebServer(%s) restart server in 5 seconds", m_server_alias.c_str());
					sleep_milliseconds(5000); // prevents from an exception flood
					continue;
				}
				break;
			}
			_log.Log(LOG_STATUS, "WebServer(%s) stopped", m_server_alias.c_str());
		}

		bool CWebServer::StartServer(server_settings& settings, const std::string& serverpath, const bool bIgnoreUsernamePassword)
		{
			if (!settings.is_enabled())
				return true;

			m_server_alias = (settings.is_secure() == true) ? "SSL" : "HTTP";

			std::string sRealm = (settings.is_secure() == true) ? "https://" : "http://";

			if (!settings.vhostname.empty())
				sRealm += settings.vhostname;
			else
				sRealm += (settings.listening_address == "::") ? "domoticz.local" : settings.listening_address;
			if (settings.listening_port != "80" || settings.listening_port != "443")
				sRealm += ":" + settings.listening_port;
			sRealm += "/";

			ReloadCustomSwitchIcons();

			int tries = 0;
			bool exception = false;

			_log.Debug(DEBUG_WEBSERVER, "CWebServer::StartServer() : settings : %s", settings.to_string().c_str());
			_log.Debug(DEBUG_AUTH, "CWebServer::StartServer() : IAM settings : %s", m_iamsettings.to_string().c_str());
			do
			{
				try
				{
					exception = false;
					m_pWebEm = new http::server::cWebem(settings, serverpath);
				}
				catch (std::exception& e)
				{
					exception = true;
					switch (tries)
					{
					case 0:
						_log.Log(LOG_STATUS, "WebServer(%s) startup failed on address %s with port: %s: %s, trying ::", m_server_alias.c_str(),
							settings.listening_address.c_str(), settings.listening_port.c_str(), e.what());
						settings.listening_address = "::";
						break;
					case 1:
						_log.Log(LOG_STATUS, "WebServer(%s) startup failed on address %s with port: %s: %s, trying 0.0.0.0", m_server_alias.c_str(),
							settings.listening_address.c_str(), settings.listening_port.c_str(), e.what());
						settings.listening_address = "0.0.0.0";
						break;
					case 2:
						_log.Log(LOG_ERROR, "WebServer(%s) startup failed on address %s with port: %s: %s", m_server_alias.c_str(), settings.listening_address.c_str(),
							settings.listening_port.c_str(), e.what());
						if (atoi(settings.listening_port.c_str()) < 1024)
							_log.Log(LOG_ERROR, "WebServer(%s) check privileges for opening ports below 1024", m_server_alias.c_str());
						else
							_log.Log(LOG_ERROR, "WebServer(%s) check if no other application is using port: %s", m_server_alias.c_str(),
								settings.listening_port.c_str());
						return false;
					}
					tries++;
				}
			} while (exception);

			_log.Log(LOG_STATUS, "WebServer(%s) started on address: %s with port %s", m_server_alias.c_str(), settings.listening_address.c_str(), settings.listening_port.c_str());

			m_pWebEm->SetDigistRealm(sRealm);
			m_pWebEm->SetSessionStore(this);

			LoadUsers();

			std::string TrustedNetworks;
			if (m_sql.GetPreferencesVar("WebLocalNetworks", TrustedNetworks))
			{
				std::vector<std::string> strarray;
				StringSplit(TrustedNetworks, ";", strarray);
				for (const auto& str : strarray)
					m_pWebEm->AddTrustedNetworks(str);
			}
			if (bIgnoreUsernamePassword)
			{
				m_pWebEm->AddTrustedNetworks("0.0.0.0/0");	// IPv4
				m_pWebEm->AddTrustedNetworks("::");	// IPv6
				_log.Log(LOG_ERROR, "SECURITY RISK! Allowing access without username/password as all incoming traffic is considered trusted! Change admin password asap and restart Domoticz!");

				if (m_users.empty())
				{
					AddUser(99999, "tmpadmin", "tmpadmin", "", (_eUserRights)URIGHTS_ADMIN, 0x1F);
					_log.Debug(DEBUG_AUTH, "[Start server] Added tmpadmin User as no active Users where found!");
				}
			}

			// register callbacks
			if (m_iamsettings.is_enabled())
			{
				m_pWebEm->RegisterPageCode(
					m_iamsettings.auth_url.c_str(), [this](auto&& session, auto&& req, auto&& rep) { GetOauth2AuthCode(session, req, rep); }, true);
				m_pWebEm->RegisterPageCode(
					m_iamsettings.token_url.c_str(), [this](auto&& session, auto&& req, auto&& rep) { PostOauth2AccessToken(session, req, rep); }, true);
				m_pWebEm->RegisterPageCode(
					m_iamsettings.discovery_url.c_str(), [this](auto&& session, auto&& req, auto&& rep) { GetOpenIDConfiguration(session, req, rep); }, true);
			}

			m_pWebEm->RegisterPageCode("/json.htm", [this](auto&& session, auto&& req, auto&& rep) { GetJSonPage(session, req, rep); });
			// These 'Pages' should probably be 'moved' to become Command codes handled by the 'json.htm API', so we get all API calls through one entry point
			// And why .php or .cgi while all these commands are NOT handled by a PHP or CGI processor but by Domoticz ?? Legacy? Rename these?
			m_pWebEm->RegisterPageCode("/backupdatabase.php", [this](auto&& session, auto&& req, auto&& rep) { GetDatabaseBackup(session, req, rep); });
			m_pWebEm->RegisterPageCode("/camsnapshot.jpg", [this](auto&& session, auto&& req, auto&& rep) { GetCameraSnapshot(session, req, rep); });
			m_pWebEm->RegisterPageCode("/raspberry.cgi", [this](auto&& session, auto&& req, auto&& rep) { GetInternalCameraSnapshot(session, req, rep); });
			m_pWebEm->RegisterPageCode("/uvccapture.cgi", [this](auto&& session, auto&& req, auto&& rep) { GetInternalCameraSnapshot(session, req, rep); });
			// Maybe handle these differently? (Or remove)
			m_pWebEm->RegisterPageCode("/images/floorplans/plan", [this](auto&& session, auto&& req, auto&& rep) { GetFloorplanImage(session, req, rep); });
			m_pWebEm->RegisterPageCode("/service-worker.js", [this](auto&& session, auto&& req, auto&& rep) { GetServiceWorker(session, req, rep); });

			// End of 'Pages' to be moved...

			m_pWebEm->RegisterActionCode("setrfxcommode", [this](auto&& session, auto&& req, auto&& redirect_uri) { SetRFXCOMMode(session, req, redirect_uri); });
			m_pWebEm->RegisterActionCode("rfxupgradefirmware", [this](auto&& session, auto&& req, auto&& redirect_uri) { RFXComUpgradeFirmware(session, req, redirect_uri); });
			m_pWebEm->RegisterActionCode("setrego6xxtype", [this](auto&& session, auto&& req, auto&& redirect_uri) { SetRego6XXType(session, req, redirect_uri); });
			m_pWebEm->RegisterActionCode("sets0metertype", [this](auto&& session, auto&& req, auto&& redirect_uri) { SetS0MeterType(session, req, redirect_uri); });
			m_pWebEm->RegisterActionCode("setlimitlesstype", [this](auto&& session, auto&& req, auto&& redirect_uri) { SetLimitlessType(session, req, redirect_uri); });

			m_pWebEm->RegisterActionCode("uploadfloorplanimage", [this](auto&& session, auto&& req, auto&& redirect_uri) { UploadFloorplanImage(session, req, redirect_uri); });

			m_pWebEm->RegisterActionCode("setopenthermsettings", [this](auto&& session, auto&& req, auto&& redirect_uri) { SetOpenThermSettings(session, req, redirect_uri); });

			m_pWebEm->RegisterActionCode("reloadpiface", [this](auto&& session, auto&& req, auto&& redirect_uri) { ReloadPiFace(session, req, redirect_uri); });
			m_pWebEm->RegisterActionCode("restoredatabase", [this](auto&& session, auto&& req, auto&& redirect_uri) { RestoreDatabase(session, req, redirect_uri); });
			m_pWebEm->RegisterActionCode("sbfspotimportolddata", [this](auto&& session, auto&& req, auto&& redirect_uri) { SBFSpotImportOldData(session, req, redirect_uri); });

			// Commands that do NOT require authentication
			RegisterCommandCode("gettimertypes", [this](auto&& session, auto&& req, auto&& root) { Cmd_GetTimerTypes(session, req, root); }, true);
			RegisterCommandCode("getlanguages", [this](auto&& session, auto&& req, auto&& root) { Cmd_GetLanguages(session, req, root); }, true);
			RegisterCommandCode("getswitchtypes", [this](auto&& session, auto&& req, auto&& root) { Cmd_GetSwitchTypes(session, req, root); }, true);
			RegisterCommandCode("getmetertypes", [this](auto&& session, auto&& req, auto&& root) { Cmd_GetMeterTypes(session, req, root); }, true);
			RegisterCommandCode("getthemes", [this](auto&& session, auto&& req, auto&& root) { Cmd_GetThemes(session, req, root); }, true);
			RegisterCommandCode("gettitle", [this](auto&& session, auto&& req, auto&& root) { Cmd_GetTitle(session, req, root); }, true);
			RegisterCommandCode("logincheck", [this](auto&& session, auto&& req, auto&& root) { Cmd_LoginCheck(session, req, root); }, true);

			RegisterCommandCode("getversion", [this](auto&& session, auto&& req, auto&& root) { Cmd_GetVersion(session, req, root); }, true);
			RegisterCommandCode("getauth", [this](auto&& session, auto&& req, auto&& root) { Cmd_GetAuth(session, req, root); }, true);
			RegisterCommandCode("getuptime", [this](auto&& session, auto&& req, auto&& root) { Cmd_GetUptime(session, req, root); }, true);
			RegisterCommandCode("getconfig", [this](auto&& session, auto&& req, auto&& root) { Cmd_GetConfig(session, req, root); }, true);

			RegisterCommandCode("rfxfirmwaregetpercentage", [this](auto&& session, auto&& req, auto&& root) { Cmd_RFXComGetFirmwarePercentage(session, req, root); }, true);

			// Commands that require authentication
			RegisterCommandCode("sendopenthermcommand", [this](auto&& session, auto&& req, auto&& root) { Cmd_SendOpenThermCommand(session, req, root); });

			RegisterCommandCode("storesettings", [this](auto&& session, auto&& req, auto&& root) { Cmd_PostSettings(session, req, root); });
			RegisterCommandCode("getlog", [this](auto&& session, auto&& req, auto&& root) { Cmd_GetLog(session, req, root); });
			RegisterCommandCode("clearlog", [this](auto&& session, auto&& req, auto&& root) { Cmd_ClearLog(session, req, root); });
			RegisterCommandCode("gethardwaretypes", [this](auto&& session, auto&& req, auto&& root) { Cmd_GetHardwareTypes(session, req, root); });
			RegisterCommandCode("addhardware", [this](auto&& session, auto&& req, auto&& root) { Cmd_AddHardware(session, req, root); });
			RegisterCommandCode("updatehardware", [this](auto&& session, auto&& req, auto&& root) { Cmd_UpdateHardware(session, req, root); });
			RegisterCommandCode("deletehardware", [this](auto&& session, auto&& req, auto&& root) { Cmd_DeleteHardware(session, req, root); });

			RegisterCommandCode("addcamera", [this](auto&& session, auto&& req, auto&& root) { Cmd_AddCamera(session, req, root); });
			RegisterCommandCode("updatecamera", [this](auto&& session, auto&& req, auto&& root) { Cmd_UpdateCamera(session, req, root); });
			RegisterCommandCode("deletecamera", [this](auto&& session, auto&& req, auto&& root) { Cmd_DeleteCamera(session, req, root); });

			RegisterCommandCode("getapplications", [this](auto&& session, auto&& req, auto&& root) { Cmd_GetApplications(session, req, root); });
			RegisterCommandCode("addapplication", [this](auto&& session, auto&& req, auto&& root) { Cmd_AddApplication(session, req, root); });
			RegisterCommandCode("updateapplication", [this](auto&& session, auto&& req, auto&& root) { Cmd_UpdateApplication(session, req, root); });
			RegisterCommandCode("deleteapplication", [this](auto&& session, auto&& req, auto&& root) { Cmd_DeleteApplication(session, req, root); });

			RegisterCommandCode("wolgetnodes", [this](auto&& session, auto&& req, auto&& root) { Cmd_WOLGetNodes(session, req, root); });
			RegisterCommandCode("woladdnode", [this](auto&& session, auto&& req, auto&& root) { Cmd_WOLAddNode(session, req, root); });
			RegisterCommandCode("wolupdatenode", [this](auto&& session, auto&& req, auto&& root) { Cmd_WOLUpdateNode(session, req, root); });
			RegisterCommandCode("wolremovenode", [this](auto&& session, auto&& req, auto&& root) { Cmd_WOLRemoveNode(session, req, root); });
			RegisterCommandCode("wolclearnodes", [this](auto&& session, auto&& req, auto&& root) { Cmd_WOLClearNodes(session, req, root); });

			RegisterCommandCode("mysensorsgetnodes", [this](auto&& session, auto&& req, auto&& root) { Cmd_MySensorsGetNodes(session, req, root); });
			RegisterCommandCode("mysensorsgetchilds", [this](auto&& session, auto&& req, auto&& root) { Cmd_MySensorsGetChilds(session, req, root); });
			RegisterCommandCode("mysensorsupdatenode", [this](auto&& session, auto&& req, auto&& root) { Cmd_MySensorsUpdateNode(session, req, root); });
			RegisterCommandCode("mysensorsremovenode", [this](auto&& session, auto&& req, auto&& root) { Cmd_MySensorsRemoveNode(session, req, root); });
			RegisterCommandCode("mysensorsremovechild", [this](auto&& session, auto&& req, auto&& root) { Cmd_MySensorsRemoveChild(session, req, root); });
			RegisterCommandCode("mysensorsupdatechild", [this](auto&& session, auto&& req, auto&& root) { Cmd_MySensorsUpdateChild(session, req, root); });

			RegisterCommandCode("pingersetmode", [this](auto&& session, auto&& req, auto&& root) { Cmd_PingerSetMode(session, req, root); });
			RegisterCommandCode("pingergetnodes", [this](auto&& session, auto&& req, auto&& root) { Cmd_PingerGetNodes(session, req, root); });
			RegisterCommandCode("pingeraddnode", [this](auto&& session, auto&& req, auto&& root) { Cmd_PingerAddNode(session, req, root); });
			RegisterCommandCode("pingerupdatenode", [this](auto&& session, auto&& req, auto&& root) { Cmd_PingerUpdateNode(session, req, root); });
			RegisterCommandCode("pingerremovenode", [this](auto&& session, auto&& req, auto&& root) { Cmd_PingerRemoveNode(session, req, root); });
			RegisterCommandCode("pingerclearnodes", [this](auto&& session, auto&& req, auto&& root) { Cmd_PingerClearNodes(session, req, root); });

			RegisterCommandCode("kodisetmode", [this](auto&& session, auto&& req, auto&& root) { Cmd_KodiSetMode(session, req, root); });
			RegisterCommandCode("kodigetnodes", [this](auto&& session, auto&& req, auto&& root) { Cmd_KodiGetNodes(session, req, root); });
			RegisterCommandCode("kodiaddnode", [this](auto&& session, auto&& req, auto&& root) { Cmd_KodiAddNode(session, req, root); });
			RegisterCommandCode("kodiupdatenode", [this](auto&& session, auto&& req, auto&& root) { Cmd_KodiUpdateNode(session, req, root); });
			RegisterCommandCode("kodiremovenode", [this](auto&& session, auto&& req, auto&& root) { Cmd_KodiRemoveNode(session, req, root); });
			RegisterCommandCode("kodiclearnodes", [this](auto&& session, auto&& req, auto&& root) { Cmd_KodiClearNodes(session, req, root); });
			RegisterCommandCode("kodimediacommand", [this](auto&& session, auto&& req, auto&& root) { Cmd_KodiMediaCommand(session, req, root); });

			RegisterCommandCode("panasonicsetmode", [this](auto&& session, auto&& req, auto&& root) { Cmd_PanasonicSetMode(session, req, root); });
			RegisterCommandCode("panasonicgetnodes", [this](auto&& session, auto&& req, auto&& root) { Cmd_PanasonicGetNodes(session, req, root); });
			RegisterCommandCode("panasonicaddnode", [this](auto&& session, auto&& req, auto&& root) { Cmd_PanasonicAddNode(session, req, root); });
			RegisterCommandCode("panasonicupdatenode", [this](auto&& session, auto&& req, auto&& root) { Cmd_PanasonicUpdateNode(session, req, root); });
			RegisterCommandCode("panasonicremovenode", [this](auto&& session, auto&& req, auto&& root) { Cmd_PanasonicRemoveNode(session, req, root); });
			RegisterCommandCode("panasonicclearnodes", [this](auto&& session, auto&& req, auto&& root) { Cmd_PanasonicClearNodes(session, req, root); });
			RegisterCommandCode("panasonicmediacommand", [this](auto&& session, auto&& req, auto&& root) { Cmd_PanasonicMediaCommand(session, req, root); });

			RegisterCommandCode("heossetmode", [this](auto&& session, auto&& req, auto&& root) { Cmd_HEOSSetMode(session, req, root); });
			RegisterCommandCode("heosmediacommand", [this](auto&& session, auto&& req, auto&& root) { Cmd_HEOSMediaCommand(session, req, root); });

			RegisterCommandCode("onkyoeiscpcommand", [this](auto&& session, auto&& req, auto&& root) { Cmd_OnkyoEiscpCommand(session, req, root); });

			RegisterCommandCode("bleboxsetmode", [this](auto&& session, auto&& req, auto&& root) { Cmd_BleBoxSetMode(session, req, root); });
			RegisterCommandCode("bleboxgetnodes", [this](auto&& session, auto&& req, auto&& root) { Cmd_BleBoxGetNodes(session, req, root); });
			RegisterCommandCode("bleboxaddnode", [this](auto&& session, auto&& req, auto&& root) { Cmd_BleBoxAddNode(session, req, root); });
			RegisterCommandCode("bleboxremovenode", [this](auto&& session, auto&& req, auto&& root) { Cmd_BleBoxRemoveNode(session, req, root); });
			RegisterCommandCode("bleboxclearnodes", [this](auto&& session, auto&& req, auto&& root) { Cmd_BleBoxClearNodes(session, req, root); });
			RegisterCommandCode("bleboxautosearchingnodes", [this](auto&& session, auto&& req, auto&& root) { Cmd_BleBoxAutoSearchingNodes(session, req, root); });
			RegisterCommandCode("bleboxupdatefirmware", [this](auto&& session, auto&& req, auto&& root) { Cmd_BleBoxUpdateFirmware(session, req, root); });

			RegisterCommandCode("lmssetmode", [this](auto&& session, auto&& req, auto&& root) { Cmd_LMSSetMode(session, req, root); });
			RegisterCommandCode("lmsgetnodes", [this](auto&& session, auto&& req, auto&& root) { Cmd_LMSGetNodes(session, req, root); });
			RegisterCommandCode("lmsgetplaylists", [this](auto&& session, auto&& req, auto&& root) { Cmd_LMSGetPlaylists(session, req, root); });
			RegisterCommandCode("lmsmediacommand", [this](auto&& session, auto&& req, auto&& root) { Cmd_LMSMediaCommand(session, req, root); });
			RegisterCommandCode("lmsdeleteunuseddevices", [this](auto&& session, auto&& req, auto&& root) { Cmd_LMSDeleteUnusedDevices(session, req, root); });

			RegisterCommandCode("savefibarolinkconfig", [this](auto&& session, auto&& req, auto&& root) { Cmd_SaveFibaroLinkConfig(session, req, root); });
			RegisterCommandCode("getfibarolinkconfig", [this](auto&& session, auto&& req, auto&& root) { Cmd_GetFibaroLinkConfig(session, req, root); });
			RegisterCommandCode("getfibarolinks", [this](auto&& session, auto&& req, auto&& root) { Cmd_GetFibaroLinks(session, req, root); });
			RegisterCommandCode("savefibarolink", [this](auto&& session, auto&& req, auto&& root) { Cmd_SaveFibaroLink(session, req, root); });
			RegisterCommandCode("deletefibarolink", [this](auto&& session, auto&& req, auto&& root) { Cmd_DeleteFibaroLink(session, req, root); });

			RegisterCommandCode("saveinfluxlinkconfig", [this](auto&& session, auto&& req, auto&& root) { Cmd_SaveInfluxLinkConfig(session, req, root); });
			RegisterCommandCode("getinfluxlinkconfig", [this](auto&& session, auto&& req, auto&& root) { Cmd_GetInfluxLinkConfig(session, req, root); });
			RegisterCommandCode("getinfluxlinks", [this](auto&& session, auto&& req, auto&& root) { Cmd_GetInfluxLinks(session, req, root); });
			RegisterCommandCode("saveinfluxlink", [this](auto&& session, auto&& req, auto&& root) { Cmd_SaveInfluxLink(session, req, root); });
			RegisterCommandCode("deleteinfluxlink", [this](auto&& session, auto&& req, auto&& root) { Cmd_DeleteInfluxLink(session, req, root); });

			RegisterCommandCode("savehttplinkconfig", [this](auto&& session, auto&& req, auto&& root) { Cmd_SaveHttpLinkConfig(session, req, root); });
			RegisterCommandCode("gethttplinkconfig", [this](auto&& session, auto&& req, auto&& root) { Cmd_GetHttpLinkConfig(session, req, root); });
			RegisterCommandCode("gethttplinks", [this](auto&& session, auto&& req, auto&& root) { Cmd_GetHttpLinks(session, req, root); });
			RegisterCommandCode("savehttplink", [this](auto&& session, auto&& req, auto&& root) { Cmd_SaveHttpLink(session, req, root); });
			RegisterCommandCode("deletehttplink", [this](auto&& session, auto&& req, auto&& root) { Cmd_DeleteHttpLink(session, req, root); });

			RegisterCommandCode("savegooglepubsublinkconfig", [this](auto&& session, auto&& req, auto&& root) { Cmd_SaveGooglePubSubLinkConfig(session, req, root); });
			RegisterCommandCode("getgooglepubsublinkconfig", [this](auto&& session, auto&& req, auto&& root) { Cmd_GetGooglePubSubLinkConfig(session, req, root); });
			RegisterCommandCode("getgooglepubsublinks", [this](auto&& session, auto&& req, auto&& root) { Cmd_GetGooglePubSubLinks(session, req, root); });
			RegisterCommandCode("savegooglepubsublink", [this](auto&& session, auto&& req, auto&& root) { Cmd_SaveGooglePubSubLink(session, req, root); });
			RegisterCommandCode("deletegooglepubsublink", [this](auto&& session, auto&& req, auto&& root) { Cmd_DeleteGooglePubSubLink(session, req, root); });

			RegisterCommandCode("getdevicevalueoptions", [this](auto&& session, auto&& req, auto&& root) { Cmd_GetDeviceValueOptions(session, req, root); });
			RegisterCommandCode("getdevicevalueoptionwording", [this](auto&& session, auto&& req, auto&& root) { Cmd_GetDeviceValueOptionWording(session, req, root); });

			RegisterCommandCode("adduservariable", [this](auto&& session, auto&& req, auto&& root) { Cmd_AddUserVariable(session, req, root); });
			RegisterCommandCode("updateuservariable", [this](auto&& session, auto&& req, auto&& root) { Cmd_UpdateUserVariable(session, req, root); });
			RegisterCommandCode("deleteuservariable", [this](auto&& session, auto&& req, auto&& root) { Cmd_DeleteUserVariable(session, req, root); });
			RegisterCommandCode("getuservariables", [this](auto&& session, auto&& req, auto&& root) { Cmd_GetUserVariables(session, req, root); });
			RegisterCommandCode("getuservariable", [this](auto&& session, auto&& req, auto&& root) { Cmd_GetUserVariable(session, req, root); });

			RegisterCommandCode("allownewhardware", [this](auto&& session, auto&& req, auto&& root) { Cmd_AllowNewHardware(session, req, root); });

			RegisterCommandCode("addplan", [this](auto&& session, auto&& req, auto&& root) { Cmd_AddPlan(session, req, root); });
			RegisterCommandCode("updateplan", [this](auto&& session, auto&& req, auto&& root) { Cmd_UpdatePlan(session, req, root); });
			RegisterCommandCode("deleteplan", [this](auto&& session, auto&& req, auto&& root) { Cmd_DeletePlan(session, req, root); });
			RegisterCommandCode("getunusedplandevices", [this](auto&& session, auto&& req, auto&& root) { Cmd_GetUnusedPlanDevices(session, req, root); });
			RegisterCommandCode("addplanactivedevice", [this](auto&& session, auto&& req, auto&& root) { Cmd_AddPlanActiveDevice(session, req, root); });
			RegisterCommandCode("getplandevices", [this](auto&& session, auto&& req, auto&& root) { Cmd_GetPlanDevices(session, req, root); });
			RegisterCommandCode("deleteplandevice", [this](auto&& session, auto&& req, auto&& root) { Cmd_DeletePlanDevice(session, req, root); });
			RegisterCommandCode("setplandevicecoords", [this](auto&& session, auto&& req, auto&& root) { Cmd_SetPlanDeviceCoords(session, req, root); });
			RegisterCommandCode("deleteallplandevices", [this](auto&& session, auto&& req, auto&& root) { Cmd_DeleteAllPlanDevices(session, req, root); });
			RegisterCommandCode("changeplanorder", [this](auto&& session, auto&& req, auto&& root) { Cmd_ChangePlanOrder(session, req, root); });
			RegisterCommandCode("changeplandeviceorder", [this](auto&& session, auto&& req, auto&& root) { Cmd_ChangePlanDeviceOrder(session, req, root); });

			RegisterCommandCode("gettimerplans", [this](auto&& session, auto&& req, auto&& root) { Cmd_GetTimerPlans(session, req, root); });
			RegisterCommandCode("addtimerplan", [this](auto&& session, auto&& req, auto&& root) { Cmd_AddTimerPlan(session, req, root); });
			RegisterCommandCode("updatetimerplan", [this](auto&& session, auto&& req, auto&& root) { Cmd_UpdateTimerPlan(session, req, root); });
			RegisterCommandCode("deletetimerplan", [this](auto&& session, auto&& req, auto&& root) { Cmd_DeleteTimerPlan(session, req, root); });
			RegisterCommandCode("duplicatetimerplan", [this](auto&& session, auto&& req, auto&& root) { Cmd_DuplicateTimerPlan(session, req, root); });

			RegisterCommandCode("getactualhistory", [this](auto&& session, auto&& req, auto&& root) { Cmd_GetActualHistory(session, req, root); });
			RegisterCommandCode("getnewhistory", [this](auto&& session, auto&& req, auto&& root) { Cmd_GetNewHistory(session, req, root); });

			RegisterCommandCode("getmyprofile", [this](auto&& session, auto&& req, auto&& root) { Cmd_GetMyProfile(session, req, root); });
			RegisterCommandCode("updatemyprofile", [this](auto&& session, auto&& req, auto&& root) { Cmd_UpdateMyProfile(session, req, root); });

			RegisterCommandCode("getlocation", [this](auto&& session, auto&& req, auto&& root) { Cmd_GetLocation(session, req, root); });
			RegisterCommandCode("getforecastconfig", [this](auto&& session, auto&& req, auto&& root) { Cmd_GetForecastConfig(session, req, root); });
			RegisterCommandCode("sendnotification", [this](auto&& session, auto&& req, auto&& root) { Cmd_SendNotification(session, req, root); });
			RegisterCommandCode("emailcamerasnapshot", [this](auto&& session, auto&& req, auto&& root) { Cmd_EmailCameraSnapshot(session, req, root); });
			RegisterCommandCode("udevice", [this](auto&& session, auto&& req, auto&& root) { Cmd_UpdateDevice(session, req, root); });
			RegisterCommandCode("udevices", [this](auto&& session, auto&& req, auto&& root) { Cmd_UpdateDevices(session, req, root); });
			RegisterCommandCode("thermostatstate", [this](auto&& session, auto&& req, auto&& root) { Cmd_SetThermostatState(session, req, root); });
			RegisterCommandCode("system_shutdown", [this](auto&& session, auto&& req, auto&& root) { Cmd_SystemShutdown(session, req, root); });
			RegisterCommandCode("system_reboot", [this](auto&& session, auto&& req, auto&& root) { Cmd_SystemReboot(session, req, root); });
			RegisterCommandCode("execute_script", [this](auto&& session, auto&& req, auto&& root) { Cmd_ExcecuteScript(session, req, root); });
			RegisterCommandCode("getcosts", [this](auto&& session, auto&& req, auto&& root) { Cmd_GetCosts(session, req, root); });
			RegisterCommandCode("checkforupdate", [this](auto&& session, auto&& req, auto&& root) { Cmd_CheckForUpdate(session, req, root); });
			RegisterCommandCode("downloadupdate", [this](auto&& session, auto&& req, auto&& root) { Cmd_DownloadUpdate(session, req, root); });
			RegisterCommandCode("downloadready", [this](auto&& session, auto&& req, auto&& root) { Cmd_DownloadReady(session, req, root); });
			RegisterCommandCode("update_application", [this](auto&& session, auto&& req, auto&& root) { Cmd_ApplicationUpdate(session, req, root); });
			RegisterCommandCode("deletedatapoint", [this](auto&& session, auto&& req, auto&& root) { Cmd_DeleteDataPoint(session, req, root); });
			RegisterCommandCode("deletedaterange", [this](auto&& session, auto&& req, auto&& root) { Cmd_DeleteDateRange(session, req, root); });
			RegisterCommandCode("customevent", [this](auto&& session, auto&& req, auto&& root) { Cmd_CustomEvent(session, req, root); });

			RegisterCommandCode("setactivetimerplan", [this](auto&& session, auto&& req, auto&& root) { Cmd_SetActiveTimerPlan(session, req, root); });
			RegisterCommandCode("addtimer", [this](auto&& session, auto&& req, auto&& root) { Cmd_AddTimer(session, req, root); });
			RegisterCommandCode("updatetimer", [this](auto&& session, auto&& req, auto&& root) { Cmd_UpdateTimer(session, req, root); });
			RegisterCommandCode("deletetimer", [this](auto&& session, auto&& req, auto&& root) { Cmd_DeleteTimer(session, req, root); });
			RegisterCommandCode("enabletimer", [this](auto&& session, auto&& req, auto&& root) { Cmd_EnableTimer(session, req, root); });
			RegisterCommandCode("disabletimer", [this](auto&& session, auto&& req, auto&& root) { Cmd_DisableTimer(session, req, root); });
			RegisterCommandCode("cleartimers", [this](auto&& session, auto&& req, auto&& root) { Cmd_ClearTimers(session, req, root); });

			RegisterCommandCode("addscenetimer", [this](auto&& session, auto&& req, auto&& root) { Cmd_AddSceneTimer(session, req, root); });
			RegisterCommandCode("updatescenetimer", [this](auto&& session, auto&& req, auto&& root) { Cmd_UpdateSceneTimer(session, req, root); });
			RegisterCommandCode("deletescenetimer", [this](auto&& session, auto&& req, auto&& root) { Cmd_DeleteSceneTimer(session, req, root); });
			RegisterCommandCode("enablescenetimer", [this](auto&& session, auto&& req, auto&& root) { Cmd_EnableSceneTimer(session, req, root); });
			RegisterCommandCode("disablescenetimer", [this](auto&& session, auto&& req, auto&& root) { Cmd_DisableSceneTimer(session, req, root); });
			RegisterCommandCode("clearscenetimers", [this](auto&& session, auto&& req, auto&& root) { Cmd_ClearSceneTimers(session, req, root); });
			RegisterCommandCode("getsceneactivations", [this](auto&& session, auto&& req, auto&& root) { Cmd_GetSceneActivations(session, req, root); });
			RegisterCommandCode("addscenecode", [this](auto&& session, auto&& req, auto&& root) { Cmd_AddSceneCode(session, req, root); });
			RegisterCommandCode("removescenecode", [this](auto&& session, auto&& req, auto&& root) { Cmd_RemoveSceneCode(session, req, root); });
			RegisterCommandCode("clearscenecodes", [this](auto&& session, auto&& req, auto&& root) { Cmd_ClearSceneCodes(session, req, root); });
			RegisterCommandCode("renamescene", [this](auto&& session, auto&& req, auto&& root) { Cmd_RenameScene(session, req, root); });

			RegisterCommandCode("setsetpoint", [this](auto&& session, auto&& req, auto&& root) { Cmd_SetSetpoint(session, req, root); });
			RegisterCommandCode("addsetpointtimer", [this](auto&& session, auto&& req, auto&& root) { Cmd_AddSetpointTimer(session, req, root); });
			RegisterCommandCode("updatesetpointtimer", [this](auto&& session, auto&& req, auto&& root) { Cmd_UpdateSetpointTimer(session, req, root); });
			RegisterCommandCode("deletesetpointtimer", [this](auto&& session, auto&& req, auto&& root) { Cmd_DeleteSetpointTimer(session, req, root); });
			RegisterCommandCode("enablesetpointtimer", [this](auto&& session, auto&& req, auto&& root) { Cmd_EnableSetpointTimer(session, req, root); });
			RegisterCommandCode("disablesetpointtimer", [this](auto&& session, auto&& req, auto&& root) { Cmd_DisableSetpointTimer(session, req, root); });
			RegisterCommandCode("clearsetpointtimers", [this](auto&& session, auto&& req, auto&& root) { Cmd_ClearSetpointTimers(session, req, root); });

			RegisterCommandCode("serial_devices", [this](auto&& session, auto&& req, auto&& root) { Cmd_GetSerialDevices(session, req, root); });
			RegisterCommandCode("devices_list", [this](auto&& session, auto&& req, auto&& root) { Cmd_GetDevicesList(session, req, root); });
			RegisterCommandCode("devices_list_onoff", [this](auto&& session, auto&& req, auto&& root) { Cmd_GetDevicesListOnOff(session, req, root); });

			RegisterCommandCode("registerhue", [this](auto&& session, auto&& req, auto&& root) { Cmd_PhilipsHueRegister(session, req, root); });

			RegisterCommandCode("getcustomiconset", [this](auto&& session, auto&& req, auto&& root) { Cmd_GetCustomIconSet(session, req, root); });
			RegisterCommandCode("uploadcustomicon", [this](auto&& session, auto&& req, auto&& root) { Cmd_UploadCustomIcon(session, req, root); });
			RegisterCommandCode("deletecustomicon", [this](auto&& session, auto&& req, auto&& root) { Cmd_DeleteCustomIcon(session, req, root); });
			RegisterCommandCode("updatecustomicon", [this](auto&& session, auto&& req, auto&& root) { Cmd_UpdateCustomIcon(session, req, root); });

			RegisterCommandCode("renamedevice", [this](auto&& session, auto&& req, auto&& root) { Cmd_RenameDevice(session, req, root); });
			RegisterCommandCode("setdevused", [this](auto&& session, auto&& req, auto&& root) { Cmd_SetDeviceUsed(session, req, root); });

			RegisterCommandCode("addlogmessage", [this](auto&& session, auto&& req, auto&& root) { Cmd_AddLogMessage(session, req, root); });
			RegisterCommandCode("clearshortlog", [this](auto&& session, auto&& req, auto&& root) { Cmd_ClearShortLog(session, req, root); });
			RegisterCommandCode("vacuumdatabase", [this](auto&& session, auto&& req, auto&& root) { Cmd_VacuumDatabase(session, req, root); });

			RegisterCommandCode("addmobiledevice", [this](auto&& session, auto&& req, auto&& root) { Cmd_AddMobileDevice(session, req, root); });
			RegisterCommandCode("updatemobiledevice", [this](auto&& session, auto&& req, auto&& root) { Cmd_UpdateMobileDevice(session, req, root); });
			RegisterCommandCode("deletemobiledevice", [this](auto&& session, auto&& req, auto&& root) { Cmd_DeleteMobileDevice(session, req, root); });

			RegisterCommandCode("addyeelight", [this](auto&& session, auto&& req, auto&& root) { Cmd_AddYeeLight(session, req, root); });

			RegisterCommandCode("addArilux", [this](auto&& session, auto&& req, auto&& root) { Cmd_AddArilux(session, req, root); });

			RegisterCommandCode("tellstickApplySettings", [this](auto&& session, auto&& req, auto&& root) { Cmd_TellstickApplySettings(session, req, root); });

			// Migrated RTypes to regular commands
			RegisterCommandCode("getusers", [this](auto&& session, auto&& req, auto&& root) { Cmd_GetUsers(session, req, root); });
			RegisterCommandCode("getsettings", [this](auto&& session, auto&& req, auto&& root) { Cmd_GetSettings(session, req, root); });
			RegisterCommandCode("getdevices", [this](auto&& session, auto&& req, auto&& root) { Cmd_GetDevices(session, req, root); });
			RegisterCommandCode("gethardware", [this](auto&& session, auto&& req, auto&& root) { Cmd_GetHardware(session, req, root); });
			RegisterCommandCode("events", [this](auto&& session, auto&& req, auto&& root) { Cmd_Events(session, req, root); });
			RegisterCommandCode("getnotifications", [this](auto&& session, auto&& req, auto&& root) { Cmd_GetNotifications(session, req, root); });
			RegisterCommandCode("createvirtualsensor", [this](auto&& session, auto&& req, auto&& root) { Cmd_CreateMappedSensor(session, req, root); });
			RegisterCommandCode("createdevice", [this](auto&& session, auto&& req, auto&& root) { Cmd_CreateDevice(session, req, root); });

			RegisterCommandCode("getscenelog", [this](auto&& session, auto&& req, auto&& root) { Cmd_GetSceneLog(session, req, root); });
			RegisterCommandCode("getscenes", [this](auto&& session, auto&& req, auto&& root) { Cmd_GetScenes(session, req, root); });
			RegisterCommandCode("addscene", [this](auto&& session, auto&& req, auto&& root) { Cmd_AddScene(session, req, root); });
			RegisterCommandCode("deletescene", [this](auto&& session, auto&& req, auto&& root) { Cmd_DeleteScene(session, req, root); });
			RegisterCommandCode("updatescene", [this](auto&& session, auto&& req, auto&& root) { Cmd_UpdateScene(session, req, root); });
			RegisterCommandCode("getmobiles", [this](auto&& session, auto&& req, auto&& root) { Cmd_GetMobiles(session, req, root); });
			RegisterCommandCode("getcameras", [this](auto&& session, auto&& req, auto&& root) { Cmd_GetCameras(session, req, root); });
			RegisterCommandCode("getcameras_user", [this](auto&& session, auto&& req, auto&& root) { Cmd_GetCamerasUser(session, req, root); });
			RegisterCommandCode("getschedules", [this](auto&& session, auto&& req, auto&& root) { Cmd_GetSchedules(session, req, root); });
			RegisterCommandCode("gettimers", [this](auto&& session, auto&& req, auto&& root) { Cmd_GetTimers(session, req, root); });
			RegisterCommandCode("getscenetimers", [this](auto&& session, auto&& req, auto&& root) { Cmd_GetSceneTimers(session, req, root); });
			RegisterCommandCode("getsetpointtimers", [this](auto&& session, auto&& req, auto&& root) { Cmd_GetSetpointTimers(session, req, root); });
			RegisterCommandCode("getplans", [this](auto&& session, auto&& req, auto&& root) { Cmd_GetPlans(session, req, root); });
			RegisterCommandCode("getfloorplans", [this](auto&& session, auto&& req, auto&& root) { Cmd_GetFloorPlans(session, req, root); });
			RegisterCommandCode("getlightlog", [this](auto&& session, auto&& req, auto&& root) { Cmd_GetLightLog(session, req, root); });
			RegisterCommandCode("gettextlog", [this](auto&& session, auto&& req, auto&& root) { Cmd_GetTextLog(session, req, root); });
			RegisterCommandCode("gettransfers", [this](auto&& session, auto&& req, auto&& root) { Cmd_GetTransfers(session, req, root); });
			RegisterCommandCode("dotransferdevice", [this](auto&& session, auto&& req, auto&& root) { Cmd_DoTransferDevice(session, req, root); });
			RegisterCommandCode("createrflinkdevice", [this](auto&& session, auto&& req, auto&& root) { Cmd_CreateRFLinkDevice(session, req, root); });
			RegisterCommandCode("createevohomesensor", [this](auto&& session, auto&& req, auto&& root) { Cmd_CreateEvohomeSensor(session, req, root); });
			RegisterCommandCode("bindevohome", [this](auto&& session, auto&& req, auto&& root) { Cmd_BindEvohome(session, req, root); });
			RegisterCommandCode("custom_light_icons", [this](auto&& session, auto&& req, auto&& root) { Cmd_CustomLightIcons(session, req, root); });
			RegisterCommandCode("deletedevice", [this](auto&& session, auto&& req, auto&& root) { Cmd_DeleteDevice(session, req, root); });
			RegisterCommandCode("getshareduserdevices", [this](auto&& session, auto&& req, auto&& root) { Cmd_GetSharedUserDevices(session, req, root); });
			RegisterCommandCode("setshareduserdevices", [this](auto&& session, auto&& req, auto&& root) { Cmd_SetSharedUserDevices(session, req, root); });
			RegisterCommandCode("graph", [this](auto&& session, auto&& req, auto&& root) { Cmd_HandleGraph(session, req, root); });
			RegisterCommandCode("rclientslog", [this](auto&& session, auto&& req, auto&& root) { Cmd_RemoteWebClientsLog(session, req, root); });
			RegisterCommandCode("setused", [this](auto&& session, auto&& req, auto&& root) { Cmd_SetUsed(session, req, root); });

			// Migrated ActionCodes to regular commands
			RegisterCommandCode("setccmetertype", [this](auto&& session, auto&& req, auto&& root) { Cmd_SetCurrentCostUSBType(session, req, root); });

			RegisterCommandCode("clearuserdevices", [this](auto&& session, auto&& req, auto&& root) { Cmd_ClearUserDevices(session, req, root); });

			//MQTT-AD
			RegisterCommandCode("mqttadgetconfig", [this](auto&& session, auto&& req, auto&& root) { Cmd_MQTTAD_GetConfig(session, req, root); });
			RegisterCommandCode("mqttupdatenumber", [this](auto&& session, auto&& req, auto&& root) { Cmd_MQTTAD_UpdateNumber(session, req, root); });

			// EnOcean helpers cmds
			RegisterCommandCode("enoceangetmanufacturers", [this](auto&& session, auto&& req, auto&& root) { Cmd_EnOceanGetManufacturers(session, req, root); });
			RegisterCommandCode("enoceangetrorgs", [this](auto&& session, auto&& req, auto&& root) { Cmd_EnOceanGetRORGs(session, req, root); });
			RegisterCommandCode("enoceangetprofiles", [this](auto&& session, auto&& req, auto&& root) { Cmd_EnOceanGetProfiles(session, req, root); });

			// EnOcean ESP3 cmds
			RegisterCommandCode("esp3enablelearnmode", [this](auto&& session, auto&& req, auto&& root) { Cmd_EnOceanESP3EnableLearnMode(session, req, root); });
			RegisterCommandCode("esp3isnodeteachedin", [this](auto&& session, auto&& req, auto&& root) { Cmd_EnOceanESP3IsNodeTeachedIn(session, req, root); });
			RegisterCommandCode("esp3cancelteachin", [this](auto&& session, auto&& req, auto&& root) { Cmd_EnOceanESP3CancelTeachIn(session, req, root); });

			RegisterCommandCode("esp3controllerreset", [this](auto&& session, auto&& req, auto&& root) { Cmd_EnOceanESP3ControllerReset(session, req, root); });

			RegisterCommandCode("esp3updatenode", [this](auto&& session, auto&& req, auto&& root) { Cmd_EnOceanESP3UpdateNode(session, req, root); });
			RegisterCommandCode("esp3deletenode", [this](auto&& session, auto&& req, auto&& root) { Cmd_EnOceanESP3DeleteNode(session, req, root); });
			RegisterCommandCode("esp3getnodes", [this](auto&& session, auto&& req, auto&& root) { Cmd_EnOceanESP3GetNodes(session, req, root); });

			//Whitelist
			m_pWebEm->RegisterWhitelistURLString("/images/floorplans/plan");

			_log.Debug(DEBUG_WEBSERVER, "WebServer(%s) started with %d Registered Commands", m_server_alias.c_str(), (int)m_webcommands.size());
			m_pWebEm->DebugRegistrations();

			// Start normal worker thread
			m_bDoStop = false;
			m_thread = std::make_shared<std::thread>([this] { Do_Work(); });
			std::string server_name = "WebServer_" + settings.listening_port;
			SetThreadName(m_thread->native_handle(), server_name.c_str());
			return (m_thread != nullptr);
		}

		void CWebServer::StopServer()
		{
			m_bDoStop = true;
			try
			{
				if (m_pWebEm == nullptr)
					return;
				m_pWebEm->Stop();
				if (m_thread)
				{
					m_thread->join();
					m_thread.reset();
				}
				delete m_pWebEm;
				m_pWebEm = nullptr;
			}
			catch (...)
			{
			}
		}

		void CWebServer::SetWebCompressionMode(const _eWebCompressionMode gzmode)
		{
			if (m_pWebEm == nullptr)
				return;
			m_pWebEm->SetWebCompressionMode(gzmode);
		}

		void CWebServer::SetAllowPlainBasicAuth(const bool allow)
		{
			if (m_pWebEm == nullptr)
				return;
			m_pWebEm->SetAllowPlainBasicAuth(allow);
		}

		void CWebServer::SetWebTheme(const std::string& themename)
		{
			if (m_pWebEm == nullptr)
				return;
			m_pWebEm->SetWebTheme(themename);
		}

		void CWebServer::SetWebRoot(const std::string& webRoot)
		{
			if (m_pWebEm == nullptr)
				return;
			m_pWebEm->SetWebRoot(webRoot);
		}

		void CWebServer::SetIamSettings(const iamserver::iam_settings& iamsettings)
		{
			m_iamsettings = iamsettings;
		}

		void CWebServer::RegisterCommandCode(const char* idname, const webserver_response_function& ResponseFunction, bool bypassAuthentication)
		{
			if (m_webcommands.find(idname) != m_webcommands.end())
			{
				_log.Debug(DEBUG_WEBSERVER, "CWebServer::RegisterCommandCode :%s already registered", idname);
				return;
			}
			m_webcommands.insert(std::pair<std::string, webserver_response_function>(std::string(idname), ResponseFunction));
			if (bypassAuthentication)
			{
				m_pWebEm->RegisterWhitelistCommandsString(idname);
			}
		}

		bool CWebServer::IsIdxForUser(const WebEmSession* pSession, const int Idx)
		{
			if (pSession->rights == 2)
				return true;
			if (pSession->rights == 0)
				return false; // viewer
			// User
			int iUser = FindUser(pSession->username.c_str());
			if ((iUser < 0) || (iUser >= (int)m_users.size()))
				return false;

			if (m_users[iUser].TotSensors == 0)
				return true; // all sensors

			std::vector<std::vector<std::string>> result =
				m_sql.safe_query("SELECT DeviceRowID FROM SharedDevices WHERE (SharedUserID == '%d') AND (DeviceRowID == '%d')", m_users[iUser].ID, Idx);
			return (!result.empty());
		}

		void CWebServer::LoadUsers()
		{
			ClearUserPasswords();
			// Add Users
			std::vector<std::vector<std::string>> result;
			result = m_sql.safe_query("SELECT ID, Active, Username, Password, MFAsecret, Rights, TabsEnabled FROM Users");
			if (!result.empty())
			{
				for (const auto& sd : result)
				{
					int bIsActive = static_cast<int>(atoi(sd[1].c_str()));
					if (bIsActive)
					{
						unsigned long ID = (unsigned long)atol(sd[0].c_str());

						std::string username = base64_decode(sd[2]);
						std::string password = sd[3];
						std::string mfatoken = sd[4];

						_eUserRights rights = (_eUserRights)atoi(sd[5].c_str());
						int activetabs = atoi(sd[6].c_str());

						AddUser(ID, username, password, mfatoken, rights, activetabs);
					}
				}
			}
			// Add 'Applications' as User with special privilege URIGHTS_CLIENTID
			result.clear();
			result = m_sql.safe_query("SELECT ID, Active, Public, Applicationname, Secret, Pemfile FROM Applications");
			if (!result.empty())
			{
				for (const auto& sd : result)
				{
					int bIsActive = static_cast<int>(atoi(sd[1].c_str()));
					if (bIsActive)
					{
						unsigned long ID = (unsigned long)m_iamsettings.getUserIdxOffset() + (unsigned long)atol(sd[0].c_str());
						int bPublic = static_cast<int>(atoi(sd[2].c_str()));
						std::string applicationname = sd[3];
						std::string secret = sd[4];
						std::string pemfile = sd[5];
						if (bPublic && secret.empty())
							secret = GenerateMD5Hash(pemfile);
						AddUser(ID, applicationname, secret, "", URIGHTS_CLIENTID, bPublic, pemfile);
					}
				}
			}

			m_mainworker.LoadSharedUsers();
		}

		void CWebServer::AddUser(const unsigned long ID, const std::string& username, const std::string& password, const std::string& mfatoken, const int userrights, const int activetabs, const std::string& pemfile)
		{
			if (m_pWebEm == nullptr)
				return;
			std::vector<std::vector<std::string>> result = m_sql.safe_query("SELECT COUNT(*) FROM SharedDevices WHERE (SharedUserID == '%d')", ID);
			if (result.empty())
				return;

			// Let's see if we can load the public/private keyfile for this user/client
			std::string privkey = "";
			std::string pubkey = "";
			if (!pemfile.empty())
			{
				std::string sErr = "";
				std::ifstream ifs;

				std::string szTmpFile = szUserDataFolder + pemfile;

				ifs.open(szTmpFile);
				if (ifs.is_open())
				{
					std::string sLine = "";
					int i = 0;
					bool bPriv = false;
					bool bPrivFound = false;
					bool bPub = false;
					bool bPubFound = false;
					while (std::getline(ifs, sLine))
					{
						sLine += '\n';	// Newlines need to be added so the SSL library understands the Public/Private keys
						if (sLine.find("-----BEGIN PUBLIC KEY") != std::string::npos)
						{
							bPub = true;
						}
						if (sLine.find("-----BEGIN PRIVATE KEY") != std::string::npos)
						{
							bPriv = true;
						}
						if (bPriv)
							privkey += sLine;
						if (bPub)
							pubkey += sLine;
						if (sLine.find("-----END PUBLIC KEY") != std::string::npos)
						{
							if (bPub)
								bPubFound = true;
							bPub = false;
						}
						if (sLine.find("-----END PRIVATE KEY") != std::string::npos)
						{
							if (bPriv)
								bPrivFound = true;
							bPriv = false;
						}
						i++;
					}
					_log.Debug(DEBUG_AUTH, "Add User: Found PEMfile (%s) for User (%s) with %d lines. PubKey (%d), PrivKey (%d)", szTmpFile.c_str(), username.c_str(), i, bPubFound, bPrivFound);
					ifs.close();
					if (!bPubFound)
						sErr = "Unable to find a Public key within the PEMfile";
					else if (!bPrivFound)
						_log.Log(LOG_STATUS, "AddUser: Pemfile (%s) only has a Public key, so only verification is possible. Token generation has to be done external.", szTmpFile.c_str());
				}
				else
					sErr = "Unable to find/open file";

				if (!sErr.empty())
				{
					_log.Log(LOG_STATUS, "AddUser: Unable to load and process given PEMfile (%s) (%s)!", szTmpFile.c_str(), sErr.c_str());
					return;
				}
			}

			_tWebUserPassword wtmp;
			wtmp.ID = ID;
			wtmp.Username = username;
			wtmp.Password = password;
			wtmp.Mfatoken = mfatoken;
			wtmp.PrivKey = privkey;
			wtmp.PubKey = pubkey;
			wtmp.userrights = (_eUserRights)userrights;
			wtmp.ActiveTabs = activetabs;
			wtmp.TotSensors = atoi(result[0][0].c_str());
			m_users.push_back(wtmp);

			_tUserAccessCode utmp;
			utmp.ID = ID;
			utmp.UserName = username;
			utmp.clientID = -1;
			utmp.ExpTime = 0;
			utmp.AuthCode = "";
			utmp.Scope = "";
			utmp.RedirectUri = "";
			m_accesscodes.push_back(utmp);

			m_pWebEm->AddUserPassword(ID, username, password, mfatoken, (_eUserRights)userrights, activetabs, privkey, pubkey);
		}

		void CWebServer::ClearUserPasswords()
		{
			m_users.clear();
			m_accesscodes.clear();
			if (m_pWebEm)
				m_pWebEm->ClearUserPasswords();
		}

		int CWebServer::FindClient(const char* szClientName)
		{
			int iClient = 0;
			for (const auto& user : m_users)
			{
				if ((user.Username == szClientName) && (user.userrights == URIGHTS_CLIENTID))
					return iClient;
				iClient++;
			}
			return -1;
		}

		int CWebServer::FindUser(const char* szUserName)
		{
			int iUser = 0;
			for (const auto& user : m_users)
			{
				if ((user.Username == szUserName) && (user.userrights != URIGHTS_CLIENTID))
					return iUser;
				iUser++;
			}
			return -1;
		}

		bool CWebServer::FindAdminUser()
		{
			return std::any_of(m_users.begin(), m_users.end(), [](const _tWebUserPassword& user) { return user.userrights == URIGHTS_ADMIN; });
		}

		int CWebServer::CountAdminUsers()
		{
			int iAdmins = 0;
			for (const auto& user : m_users)
			{
				if (user.userrights == URIGHTS_ADMIN)
					iAdmins++;
			}
			return iAdmins;
		}

		void CWebServer::GetJSonDevices(Json::Value& root, const std::string& rused, const std::string& rfilter, const std::string& order, const std::string& rowid, const std::string& planID,
			const std::string& floorID, const bool bDisplayHidden, const bool bDisplayDisabled, const bool bFetchFavorites, const time_t LastUpdate,
			const std::string& username, const std::string& hardwareid)
		{
			std::vector<std::vector<std::string>> result;

			time_t now = mytime(nullptr);
			struct tm tm1;
			localtime_r(&now, &tm1);
			struct tm tLastUpdate;
			localtime_r(&now, &tLastUpdate);

			const time_t iLastUpdate = LastUpdate - 1;

			int SensorTimeOut = 60;
			m_sql.GetPreferencesVar("SensorTimeout", SensorTimeOut);

			// Get All Hardware ID's/Names, need them later
			std::map<int, _tHardwareListInt> _hardwareNames;
			result = m_sql.safe_query("SELECT ID, Name, Enabled, Type, Mode1, Mode2 FROM Hardware");
			if (!result.empty())
			{
				for (const auto& sd : result)
				{
					_tHardwareListInt tlist;
					int ID = atoi(sd[0].c_str());
					tlist.Name = sd[1];
					tlist.Enabled = (atoi(sd[2].c_str()) != 0);
					tlist.HardwareTypeVal = atoi(sd[3].c_str());
#ifndef ENABLE_PYTHON
					tlist.HardwareType = Hardware_Type_Desc(tlist.HardwareTypeVal);
#else
					if (tlist.HardwareTypeVal != HTYPE_PythonPlugin)
					{
						tlist.HardwareType = Hardware_Type_Desc(tlist.HardwareTypeVal);
					}
					else
					{
						tlist.HardwareType = PluginHardwareDesc(ID);
					}
#endif
					tlist.Mode1 = sd[4];
					tlist.Mode2 = sd[5];
					_hardwareNames[ID] = tlist;
				}
			}

			root["ActTime"] = static_cast<int>(now);

			char szTmp[300];

			if (!m_mainworker.m_LastSunriseSet.empty())
			{
				std::vector<std::string> strarray;
				StringSplit(m_mainworker.m_LastSunriseSet, ";", strarray);
				if (strarray.size() == 10)
				{
					// strftime(szTmp, 80, "%b %d %Y %X", &tm1);
					strftime(szTmp, 80, "%Y-%m-%d %X", &tm1);
					root["ServerTime"] = szTmp;
					root["Sunrise"] = strarray[0];
					root["Sunset"] = strarray[1];
					root["SunAtSouth"] = strarray[2];
					root["CivTwilightStart"] = strarray[3];
					root["CivTwilightEnd"] = strarray[4];
					root["NautTwilightStart"] = strarray[5];
					root["NautTwilightEnd"] = strarray[6];
					root["AstrTwilightStart"] = strarray[7];
					root["AstrTwilightEnd"] = strarray[8];
					root["DayLength"] = strarray[9];
				}
			}

			char szOrderBy[50];
			std::string szQuery;
			bool isAlpha = true;
			const std::string orderBy = order;
			for (char i : orderBy)
			{
				if (!isalpha(i))
				{
					isAlpha = false;
				}
			}
			if (order.empty() || (!isAlpha))
			{
				strcpy(szOrderBy, "A.[Order],A.LastUpdate DESC");
			}
			else
			{
				sprintf(szOrderBy, "A.[Order],A.%s ASC", order.c_str());
			}

			unsigned char tempsign = m_sql.m_tempsign[0];

			bool bHaveUser = false;
			int iUser = -1;
			unsigned int totUserDevices = 0;
			bool bShowScenes = true;
			bHaveUser = (!username.empty());
			if (bHaveUser)
			{
				iUser = FindUser(username.c_str());
				if (iUser != -1)
				{
					
					if (m_users[iUser].TotSensors > 0)
					{
						bool bSkipSelectedDevices = false;
						if (m_users[iUser].userrights == URIGHTS_ADMIN)
						{
							bSkipSelectedDevices = (rused == "all");
						}
						if (!bSkipSelectedDevices)
						{
							result = m_sql.safe_query("SELECT COUNT(*) FROM SharedDevices WHERE (SharedUserID == %lu)", m_users[iUser].ID);
							if (!result.empty())
							{
								totUserDevices = (unsigned int)std::stoi(result[0][0]);
							}
						}
					}
					bShowScenes = (m_users[iUser].ActiveTabs & (1 << 1)) != 0;
				}
			}

			std::set<std::string> _HiddenDevices;
			bool bAllowDeviceToBeHidden = false;

			int ii = 0;
			if (rfilter == "all")
			{
				if ((bShowScenes) && ((rused == "all") || (rused == "true")))
				{
					// add scenes
					if (!rowid.empty())
						result = m_sql.safe_query("SELECT A.ID, A.Name, A.nValue, A.LastUpdate, A.Favorite, A.SceneType,"
							" A.Protected, B.XOffset, B.YOffset, B.PlanID, A.Description"
							" FROM Scenes as A"
							" LEFT OUTER JOIN DeviceToPlansMap as B ON (B.DeviceRowID==a.ID) AND (B.DevSceneType==1)"
							" WHERE (A.ID=='%q')",
							rowid.c_str());
					else if ((!planID.empty()) && (planID != "0"))
						result = m_sql.safe_query("SELECT A.ID, A.Name, A.nValue, A.LastUpdate, A.Favorite, A.SceneType,"
							" A.Protected, B.XOffset, B.YOffset, B.PlanID, A.Description"
							" FROM Scenes as A, DeviceToPlansMap as B WHERE (B.PlanID=='%q')"
							" AND (B.DeviceRowID==a.ID) AND (B.DevSceneType==1) ORDER BY B.[Order]",
							planID.c_str());
					else if ((!floorID.empty()) && (floorID != "0"))
						result = m_sql.safe_query("SELECT A.ID, A.Name, A.nValue, A.LastUpdate, A.Favorite, A.SceneType,"
							" A.Protected, B.XOffset, B.YOffset, B.PlanID, A.Description"
							" FROM Scenes as A, DeviceToPlansMap as B, Plans as C"
							" WHERE (C.FloorplanID=='%q') AND (C.ID==B.PlanID) AND (B.DeviceRowID==a.ID)"
							" AND (B.DevSceneType==1) ORDER BY B.[Order]",
							floorID.c_str());
					else
					{
						szQuery = ("SELECT A.ID, A.Name, A.nValue, A.LastUpdate, A.Favorite, A.SceneType,"
							" A.Protected, B.XOffset, B.YOffset, B.PlanID, A.Description"
							" FROM Scenes as A"
							" LEFT OUTER JOIN DeviceToPlansMap as B ON (B.DeviceRowID==a.ID) AND (B.DevSceneType==1)"
							" ORDER BY ");
						szQuery += szOrderBy;
						result = m_sql.safe_query(szQuery.c_str(), order.c_str());
					}

					if (!result.empty())
					{
						for (const auto& sd : result)
						{
							unsigned char favorite = atoi(sd[4].c_str());
							// Check if we only want favorite devices
							if ((bFetchFavorites) && (!favorite))
								continue;

							std::string sLastUpdate = sd[3];

							if (iLastUpdate != 0)
							{
								time_t cLastUpdate;
								ParseSQLdatetime(cLastUpdate, tLastUpdate, sLastUpdate, tm1.tm_isdst);
								if (cLastUpdate <= iLastUpdate)
									continue;
							}

							int nValue = atoi(sd[2].c_str());

							unsigned char scenetype = atoi(sd[5].c_str());
							int iProtected = atoi(sd[6].c_str());

							std::string sSceneName = sd[1];
							if (!bDisplayHidden && sSceneName[0] == '$')
							{
								continue;
							}

							if (scenetype == 0)
							{
								root["result"][ii]["Type"] = "Scene";
								root["result"][ii]["TypeImg"] = "scene";
								root["result"][ii]["Image"] = "Push";
							}
							else
							{
								root["result"][ii]["Type"] = "Group";
								root["result"][ii]["TypeImg"] = "group";
							}

							// has this scene/group already been seen, now with different plan?
							// assume results are ordered such that same device is adjacent
							// if the idx and the Type are equal (type to prevent matching against Scene with same idx)
							std::string thisIdx = sd[0];

							if ((ii > 0) && thisIdx == root["result"][ii - 1]["idx"].asString())
							{
								std::string typeOfThisOne = root["result"][ii]["Type"].asString();
								if (typeOfThisOne == root["result"][ii - 1]["Type"].asString())
								{
									root["result"][ii - 1]["PlanIDs"].append(atoi(sd[9].c_str()));
									continue;
								}
							}

							root["result"][ii]["idx"] = sd[0];
							root["result"][ii]["Name"] = sSceneName;
							root["result"][ii]["Description"] = sd[10];
							root["result"][ii]["Favorite"] = favorite;
							root["result"][ii]["Protected"] = (iProtected != 0);
							root["result"][ii]["LastUpdate"] = sLastUpdate;
							root["result"][ii]["PlanID"] = sd[9].c_str();
							Json::Value jsonArray;
							jsonArray.append(atoi(sd[9].c_str()));
							root["result"][ii]["PlanIDs"] = jsonArray;

							if (nValue == 0)
								root["result"][ii]["Status"] = "Off";
							else if (nValue == 1)
								root["result"][ii]["Status"] = "On";
							else
								root["result"][ii]["Status"] = "Mixed";
							root["result"][ii]["Data"] = root["result"][ii]["Status"];
							uint64_t camIDX = m_mainworker.m_cameras.IsDevSceneInCamera(1, sd[0]);
							root["result"][ii]["UsedByCamera"] = (camIDX != 0) ? true : false;
							if (camIDX != 0)
							{
								std::stringstream scidx;
								scidx << camIDX;
								root["result"][ii]["CameraIdx"] = scidx.str();
								root["result"][ii]["CameraAspect"] = m_mainworker.m_cameras.GetCameraAspectRatio(scidx.str());
							}
							root["result"][ii]["XOffset"] = atoi(sd[7].c_str());
							root["result"][ii]["YOffset"] = atoi(sd[8].c_str());
							ii++;
						}
					}
				}
			}

			char szData[320];
			if (totUserDevices == 0)
			{
				// All
				if (!rowid.empty())
				{
					//_log.Log(LOG_STATUS, "Getting device with id: %s", rowid.c_str());
					result = m_sql.safe_query("SELECT A.ID, A.DeviceID, A.Unit, A.Name, A.Used, A.Type, A.SubType,"
						" A.SignalLevel, A.BatteryLevel, A.nValue, A.sValue,"
						" A.LastUpdate, A.Favorite, A.SwitchType, A.HardwareID,"
						" A.AddjValue, A.AddjMulti, A.AddjValue2, A.AddjMulti2,"
						" A.LastLevel, A.CustomImage, A.StrParam1, A.StrParam2,"
						" A.Protected, IFNULL(B.XOffset,0), IFNULL(B.YOffset,0), IFNULL(B.PlanID,0), A.Description,"
						" A.Options, A.Color "
						"FROM DeviceStatus A LEFT OUTER JOIN DeviceToPlansMap as B ON (B.DeviceRowID==a.ID) "
						"WHERE (A.ID=='%q')",
						rowid.c_str());
				}
				else if ((!planID.empty()) && (planID != "0"))
					result = m_sql.safe_query("SELECT A.ID, A.DeviceID, A.Unit, A.Name, A.Used,"
						" A.Type, A.SubType, A.SignalLevel, A.BatteryLevel,"
						" A.nValue, A.sValue, A.LastUpdate, A.Favorite,"
						" A.SwitchType, A.HardwareID, A.AddjValue,"
						" A.AddjMulti, A.AddjValue2, A.AddjMulti2,"
						" A.LastLevel, A.CustomImage, A.StrParam1,"
						" A.StrParam2, A.Protected, B.XOffset, B.YOffset,"
						" B.PlanID, A.Description,"
						" A.Options, A.Color "
						"FROM DeviceStatus as A, DeviceToPlansMap as B "
						"WHERE (B.PlanID=='%q') AND (B.DeviceRowID==a.ID)"
						" AND (B.DevSceneType==0) ORDER BY B.[Order]",
						planID.c_str());
				else if ((!floorID.empty()) && (floorID != "0"))
					result = m_sql.safe_query("SELECT A.ID, A.DeviceID, A.Unit, A.Name, A.Used,"
						" A.Type, A.SubType, A.SignalLevel, A.BatteryLevel,"
						" A.nValue, A.sValue, A.LastUpdate, A.Favorite,"
						" A.SwitchType, A.HardwareID, A.AddjValue,"
						" A.AddjMulti, A.AddjValue2, A.AddjMulti2,"
						" A.LastLevel, A.CustomImage, A.StrParam1,"
						" A.StrParam2, A.Protected, B.XOffset, B.YOffset,"
						" B.PlanID, A.Description,"
						" A.Options, A.Color "
						"FROM DeviceStatus as A, DeviceToPlansMap as B,"
						" Plans as C "
						"WHERE (C.FloorplanID=='%q') AND (C.ID==B.PlanID)"
						" AND (B.DeviceRowID==a.ID) AND (B.DevSceneType==0) "
						"ORDER BY B.[Order]",
						floorID.c_str());
				else
				{
					if (!bDisplayHidden)
					{
						// Build a list of Hidden Devices
						result = m_sql.safe_query("SELECT ID FROM Plans WHERE (Name=='$Hidden Devices')");
						if (!result.empty())
						{
							std::string pID = result[0][0];
							result = m_sql.safe_query("SELECT DeviceRowID FROM DeviceToPlansMap WHERE (PlanID=='%q') AND (DevSceneType==0)", pID.c_str());
							if (!result.empty())
							{
								for (const auto& r : result)
								{
									_HiddenDevices.insert(r[0]);
								}
							}
						}
						bAllowDeviceToBeHidden = true;
					}

					if (order.empty() || (!isAlpha))
						strcpy(szOrderBy, "A.[Order],A.LastUpdate DESC");
					else
					{
						sprintf(szOrderBy, "A.[Order],A.%s ASC", order.c_str());
					}
					//_log.Log(LOG_STATUS, "Getting all devices: order by %s ", szOrderBy);
					if (!hardwareid.empty())
					{
						szQuery = ("SELECT A.ID, A.DeviceID, A.Unit, A.Name, A.Used,A.Type, A.SubType,"
							" A.SignalLevel, A.BatteryLevel, A.nValue, A.sValue,"
							" A.LastUpdate, A.Favorite, A.SwitchType, A.HardwareID,"
							" A.AddjValue, A.AddjMulti, A.AddjValue2, A.AddjMulti2,"
							" A.LastLevel, A.CustomImage, A.StrParam1, A.StrParam2,"
							" A.Protected, IFNULL(B.XOffset,0), IFNULL(B.YOffset,0), IFNULL(B.PlanID,0), A.Description,"
							" A.Options, A.Color "
							"FROM DeviceStatus as A LEFT OUTER JOIN DeviceToPlansMap as B "
							"ON (B.DeviceRowID==a.ID) AND (B.DevSceneType==0) "
							"WHERE (A.HardwareID == %q) "
							"ORDER BY ");
						szQuery += szOrderBy;
						result = m_sql.safe_query(szQuery.c_str(), hardwareid.c_str(), order.c_str());
					}
					else
					{
						szQuery = ("SELECT A.ID, A.DeviceID, A.Unit, A.Name, A.Used,A.Type, A.SubType,"
							" A.SignalLevel, A.BatteryLevel, A.nValue, A.sValue,"
							" A.LastUpdate, A.Favorite, A.SwitchType, A.HardwareID,"
							" A.AddjValue, A.AddjMulti, A.AddjValue2, A.AddjMulti2,"
							" A.LastLevel, A.CustomImage, A.StrParam1, A.StrParam2,"
							" A.Protected, IFNULL(B.XOffset,0), IFNULL(B.YOffset,0), IFNULL(B.PlanID,0), A.Description,"
							" A.Options, A.Color "
							"FROM DeviceStatus as A LEFT OUTER JOIN DeviceToPlansMap as B "
							"ON (B.DeviceRowID==a.ID) AND (B.DevSceneType==0) "
							"ORDER BY ");
						szQuery += szOrderBy;
						result = m_sql.safe_query(szQuery.c_str(), order.c_str());
					}
				}
			}
			else
			{
				if (iUser == -1)
				{
					return;
				}
				// Specific devices
				if (!rowid.empty())
				{
					//_log.Log(LOG_STATUS, "Getting device with id: %s for user %lu", rowid.c_str(), m_users[iUser].ID);
					result = m_sql.safe_query("SELECT A.ID, A.DeviceID, A.Unit, A.Name, A.Used,"
						" A.Type, A.SubType, A.SignalLevel, A.BatteryLevel,"
						" A.nValue, A.sValue, A.LastUpdate, B.Favorite,"
						" A.SwitchType, A.HardwareID, A.AddjValue,"
						" A.AddjMulti, A.AddjValue2, A.AddjMulti2,"
						" A.LastLevel, A.CustomImage, A.StrParam1,"
						" A.StrParam2, A.Protected, 0 as XOffset,"
						" 0 as YOffset, 0 as PlanID, A.Description,"
						" A.Options, A.Color "
						"FROM DeviceStatus as A, SharedDevices as B "
						"WHERE (B.DeviceRowID==a.ID)"
						" AND (B.SharedUserID==%lu) AND (A.ID=='%q')",
						m_users[iUser].ID, rowid.c_str());
				}
				else if ((!planID.empty()) && (planID != "0"))
					result = m_sql.safe_query("SELECT A.ID, A.DeviceID, A.Unit, A.Name, A.Used,"
						" A.Type, A.SubType, A.SignalLevel, A.BatteryLevel,"
						" A.nValue, A.sValue, A.LastUpdate, B.Favorite,"
						" A.SwitchType, A.HardwareID, A.AddjValue,"
						" A.AddjMulti, A.AddjValue2, A.AddjMulti2,"
						" A.LastLevel, A.CustomImage, A.StrParam1,"
						" A.StrParam2, A.Protected, C.XOffset,"
						" C.YOffset, C.PlanID, A.Description,"
						" A.Options, A.Color "
						"FROM DeviceStatus as A, SharedDevices as B,"
						" DeviceToPlansMap as C "
						"WHERE (C.PlanID=='%q') AND (C.DeviceRowID==a.ID)"
						" AND (B.DeviceRowID==a.ID) "
						"AND (B.SharedUserID==%lu) ORDER BY C.[Order]",
						planID.c_str(), m_users[iUser].ID);
				else if ((!floorID.empty()) && (floorID != "0"))
					result = m_sql.safe_query("SELECT A.ID, A.DeviceID, A.Unit, A.Name, A.Used,"
						" A.Type, A.SubType, A.SignalLevel, A.BatteryLevel,"
						" A.nValue, A.sValue, A.LastUpdate, B.Favorite,"
						" A.SwitchType, A.HardwareID, A.AddjValue,"
						" A.AddjMulti, A.AddjValue2, A.AddjMulti2,"
						" A.LastLevel, A.CustomImage, A.StrParam1,"
						" A.StrParam2, A.Protected, C.XOffset, C.YOffset,"
						" C.PlanID, A.Description,"
						" A.Options, A.Color "
						"FROM DeviceStatus as A, SharedDevices as B,"
						" DeviceToPlansMap as C, Plans as D "
						"WHERE (D.FloorplanID=='%q') AND (D.ID==C.PlanID)"
						" AND (C.DeviceRowID==a.ID) AND (B.DeviceRowID==a.ID)"
						" AND (B.SharedUserID==%lu) ORDER BY C.[Order]",
						floorID.c_str(), m_users[iUser].ID);
				else
				{
					if (!bDisplayHidden)
					{
						// Build a list of Hidden Devices
						result = m_sql.safe_query("SELECT ID FROM Plans WHERE (Name=='$Hidden Devices')");
						if (!result.empty())
						{
							std::string pID = result[0][0];
							result = m_sql.safe_query("SELECT DeviceRowID FROM DeviceToPlansMap WHERE (PlanID=='%q')  AND (DevSceneType==0)", pID.c_str());
							if (!result.empty())
							{
								for (const auto& r : result)
								{
									_HiddenDevices.insert(r[0]);
								}
							}
						}
						bAllowDeviceToBeHidden = true;
					}

					if (order.empty() || (!isAlpha))
					{
						strcpy(szOrderBy, "B.[Order],A.LastUpdate DESC");
					}
					else
					{
						sprintf(szOrderBy, "B.[Order],A.%s ASC", order.c_str());
					}
					// _log.Log(LOG_STATUS, "Getting all devices for user %lu", m_users[iUser].ID);
					szQuery = ("SELECT A.ID, A.DeviceID, A.Unit, A.Name, A.Used,"
						" A.Type, A.SubType, A.SignalLevel, A.BatteryLevel,"
						" A.nValue, A.sValue, A.LastUpdate, B.Favorite,"
						" A.SwitchType, A.HardwareID, A.AddjValue,"
						" A.AddjMulti, A.AddjValue2, A.AddjMulti2,"
						" A.LastLevel, A.CustomImage, A.StrParam1,"
						" A.StrParam2, A.Protected, IFNULL(C.XOffset,0),"
						" IFNULL(C.YOffset,0), IFNULL(C.PlanID,0), A.Description,"
						" A.Options, A.Color "
						"FROM DeviceStatus as A, SharedDevices as B "
						"LEFT OUTER JOIN DeviceToPlansMap as C  ON (C.DeviceRowID==A.ID)"
						"WHERE (B.DeviceRowID==A.ID)"
						" AND (B.SharedUserID==%lu) ORDER BY ");
					szQuery += szOrderBy;
					result = m_sql.safe_query(szQuery.c_str(), m_users[iUser].ID, order.c_str());
				}
			}

			if (result.empty())
				return;

			for (const auto& sd : result)
			{
				try
				{
					unsigned char favorite = atoi(sd[12].c_str());
					bool bIsInPlan = !planID.empty() && (planID != "0");

					// Check if we only want favorite devices
					if (!bIsInPlan)
					{
						if ((bFetchFavorites) && (!favorite))
							continue;
					}

					std::string sDeviceName = sd[3];

					if (!bDisplayHidden)
					{
						if (_HiddenDevices.find(sd[0]) != _HiddenDevices.end())
							continue;
						if (sDeviceName[0] == '$')
						{
							if (bAllowDeviceToBeHidden)
								continue;
							if (!planID.empty())
								sDeviceName = sDeviceName.substr(1);
						}
					}
					int hardwareID = atoi(sd[14].c_str());
					auto hItt = _hardwareNames.find(hardwareID);
					bool bIsHardwareDisabled = true;
					if (hItt != _hardwareNames.end())
					{
						// ignore sensors where the hardware is disabled
						if ((!bDisplayDisabled) && (!(*hItt).second.Enabled))
							continue;
						bIsHardwareDisabled = !(*hItt).second.Enabled;
					}

					unsigned int dType = atoi(sd[5].c_str());
					unsigned int dSubType = atoi(sd[6].c_str());
					unsigned int used = atoi(sd[4].c_str());
					int nValue = atoi(sd[9].c_str());
					std::string sValue = sd[10];
					std::string sLastUpdate = sd[11];
					if (sLastUpdate.size() > 19)
						sLastUpdate = sLastUpdate.substr(0, 19);

					if (iLastUpdate != 0)
					{
						time_t cLastUpdate;
						ParseSQLdatetime(cLastUpdate, tLastUpdate, sLastUpdate, tm1.tm_isdst);
						if (cLastUpdate <= iLastUpdate)
							continue;
					}

					_eSwitchType switchtype = (_eSwitchType)atoi(sd[13].c_str());
					_eMeterType metertype = (_eMeterType)switchtype;
					double AddjValue = atof(sd[15].c_str());
					double AddjMulti = atof(sd[16].c_str());
					double AddjValue2 = atof(sd[17].c_str());
					double AddjMulti2 = atof(sd[18].c_str());
					int LastLevel = atoi(sd[19].c_str());
					int CustomImage = atoi(sd[20].c_str());
					std::string strParam1 = base64_encode(sd[21]);
					std::string strParam2 = base64_encode(sd[22]);
					int iProtected = atoi(sd[23].c_str());

					std::string Description = sd[27];
					std::string sOptions = sd[28];
					std::string sColor = sd[29];
					std::map<std::string, std::string> options = m_sql.BuildDeviceOptions(sOptions);

					struct tm ntime;
					time_t checktime;
					ParseSQLdatetime(checktime, ntime, sLastUpdate, tm1.tm_isdst);
					bool bHaveTimeout = (now - checktime >= SensorTimeOut * 60);

					if (dType == pTypeTEMP_RAIN)
						continue; // dont want you for now

					if ((rused == "true") && (!used))
						continue;

					if ((rused == "false") && (used))
						continue;
					if (!rfilter.empty())
					{
						if (rfilter == "light")
						{
							if (!
								(
								IsLightOrSwitch(dType, dSubType)
								|| (dType == pTypeEvohome)
								|| (dType == pTypeEvohomeRelay)
								|| ((dType == pTypeRego6XXValue) && (dSubType == sTypeRego6XXStatus))
								)
								)
								continue;
						}
						else if (rfilter == "temp")
						{
							if (!IsTemp(dType, dSubType))
								continue;
						}
						else if (rfilter == "weather")
						{
							if (!IsWeather(dType, dSubType))
								continue;
						}
						else if (rfilter == "utility")
						{
							if (!IsUtility(dType, dSubType))
								continue;
						}
						else if (rfilter == "wind")
						{
							if ((dType != pTypeWIND))
								continue;
						}
						else if (rfilter == "rain")
						{
							if ((dType != pTypeRAIN))
								continue;
						}
						else if (rfilter == "uv")
						{
							if ((dType != pTypeUV))
								continue;
						}
						else if (rfilter == "baro")
						{
							if ((dType != pTypeTEMP_HUM_BARO) && (dType != pTypeTEMP_BARO))
								continue;
						}
					}

					// has this device already been seen, now with different plan?
					// assume results are ordered such that same device is adjacent
					// if the idx and the Type are equal (type to prevent matching against Scene with same idx)
					std::string thisIdx = sd[0];
					const int devIdx = atoi(thisIdx.c_str());

					if ((ii > 0) && thisIdx == root["result"][ii - 1]["idx"].asString())
					{
						std::string typeOfThisOne = RFX_Type_Desc(dType, 1);
						if (typeOfThisOne == root["result"][ii - 1]["Type"].asString())
						{
							root["result"][ii - 1]["PlanIDs"].append(atoi(sd[26].c_str()));
							continue;
						}
					}

					root["result"][ii]["HardwareID"] = hardwareID;
					if (_hardwareNames.find(hardwareID) == _hardwareNames.end())
					{
						root["result"][ii]["HardwareName"] = "Unknown?";
						root["result"][ii]["HardwareTypeVal"] = 0;
						root["result"][ii]["HardwareType"] = "Unknown?";
					}
					else
					{
						root["result"][ii]["HardwareName"] = _hardwareNames[hardwareID].Name;
						root["result"][ii]["HardwareTypeVal"] = _hardwareNames[hardwareID].HardwareTypeVal;
						root["result"][ii]["HardwareType"] = _hardwareNames[hardwareID].HardwareType;
					}
					root["result"][ii]["HardwareDisabled"] = bIsHardwareDisabled;

					root["result"][ii]["idx"] = sd[0];
					root["result"][ii]["Protected"] = (iProtected != 0);

					CDomoticzHardwareBase* pHardware = m_mainworker.GetHardware(hardwareID);
					if (pHardware != nullptr)
					{
						if (pHardware->HwdType == HTYPE_SolarEdgeAPI)
						{
							int seSensorTimeOut = 60 * 24 * 60;
							bHaveTimeout = (now - checktime >= seSensorTimeOut * 60);
						}
						else if (pHardware->HwdType == HTYPE_Wunderground)
						{
							CWunderground* pWHardware = dynamic_cast<CWunderground*>(pHardware);
							std::string forecast_url = pWHardware->GetForecastURL();
							if (!forecast_url.empty())
							{
								root["result"][ii]["forecast_url"] = base64_encode(forecast_url);
							}
						}
						else if (pHardware->HwdType == HTYPE_DarkSky)
						{
							CDarkSky* pWHardware = dynamic_cast<CDarkSky*>(pHardware);
							std::string forecast_url = pWHardware->GetForecastURL();
							if (!forecast_url.empty())
							{
								root["result"][ii]["forecast_url"] = base64_encode(forecast_url);
							}
						}
						else if (pHardware->HwdType == HTYPE_VisualCrossing)
						{
							CVisualCrossing* pWHardware = dynamic_cast<CVisualCrossing*>(pHardware);
							std::string forecast_url = pWHardware->GetForecastURL();
							if (!forecast_url.empty())
							{
								root["result"][ii]["forecast_url"] = base64_encode(forecast_url);
							}
						}
						else if (pHardware->HwdType == HTYPE_AccuWeather)
						{
							CAccuWeather* pWHardware = dynamic_cast<CAccuWeather*>(pHardware);
							std::string forecast_url = pWHardware->GetForecastURL();
							if (!forecast_url.empty())
							{
								root["result"][ii]["forecast_url"] = base64_encode(forecast_url);
							}
						}
						else if (pHardware->HwdType == HTYPE_OpenWeatherMap)
						{
							COpenWeatherMap* pWHardware = dynamic_cast<COpenWeatherMap*>(pHardware);
							std::string forecast_url = pWHardware->GetForecastURL();
							if (!forecast_url.empty())
							{
								root["result"][ii]["forecast_url"] = base64_encode(forecast_url);
							}
						}
						else if (pHardware->HwdType == HTYPE_BuienRadar)
						{
							CBuienRadar* pWHardware = dynamic_cast<CBuienRadar*>(pHardware);
							std::string forecast_url = pWHardware->GetForecastURL();
							if (!forecast_url.empty())
							{
								root["result"][ii]["forecast_url"] = base64_encode(forecast_url);
							}
						}
						else if (pHardware->HwdType == HTYPE_Meteorologisk)
						{
							CMeteorologisk* pWHardware = dynamic_cast<CMeteorologisk*>(pHardware);
							std::string forecast_url = pWHardware->GetForecastURL();
							if (!forecast_url.empty())
							{
								root["result"][ii]["forecast_url"] = base64_encode(forecast_url);
							}
						}
					}

					if ((pHardware != nullptr) && (pHardware->HwdType == HTYPE_PythonPlugin))
					{
						// Device ID special formatting should not be applied to Python plugins
						root["result"][ii]["ID"] = sd[1];
					}
					else
					{
						if ((dType == pTypeTEMP) || (dType == pTypeTEMP_BARO) || (dType == pTypeTEMP_HUM) || (dType == pTypeTEMP_HUM_BARO) || (dType == pTypeBARO) ||
							(dType == pTypeHUM) || (dType == pTypeWIND) || (dType == pTypeRAIN) || (dType == pTypeUV) || (dType == pTypeCURRENT) ||
							(dType == pTypeCURRENTENERGY) || (dType == pTypeENERGY) || (dType == pTypeRFXMeter) || (dType == pTypeAirQuality) || (dType == pTypeRFXSensor) ||
							(dType == pTypeP1Power) || (dType == pTypeP1Gas))
						{
							root["result"][ii]["ID"] = is_number(sd[1]) ? std_format("%04X", (unsigned int)atoi(sd[1].c_str())) : sd[1];
						}
						else
						{
							root["result"][ii]["ID"] = sd[1];
						}
					}

					root["result"][ii]["Unit"] = atoi(sd[2].c_str());
					root["result"][ii]["Type"] = RFX_Type_Desc(dType, 1);
					root["result"][ii]["SubType"] = RFX_Type_SubType_Desc(dType, dSubType);
					root["result"][ii]["TypeImg"] = RFX_Type_Desc(dType, 2);
					root["result"][ii]["Name"] = sDeviceName;
					root["result"][ii]["Description"] = Description;
					root["result"][ii]["Used"] = used;
					root["result"][ii]["Favorite"] = favorite;

					int iSignalLevel = atoi(sd[7].c_str());
					if (iSignalLevel < 12)
						root["result"][ii]["SignalLevel"] = iSignalLevel;
					else
						root["result"][ii]["SignalLevel"] = "-";
					root["result"][ii]["BatteryLevel"] = atoi(sd[8].c_str());
					root["result"][ii]["LastUpdate"] = sLastUpdate;

					root["result"][ii]["CustomImage"] = CustomImage;

					if (CustomImage != 0)
					{
						auto ittIcon = m_custom_light_icons_lookup.find(CustomImage);
						if (ittIcon != m_custom_light_icons_lookup.end())
						{
							root["result"][ii]["CustomImage"] = CustomImage;
							root["result"][ii]["Image"] = m_custom_light_icons[ittIcon->second].RootFile;
						}
						else
						{
							CustomImage = 0;
							root["result"][ii]["CustomImage"] = CustomImage;
						}
					}

					root["result"][ii]["XOffset"] = sd[24].c_str();
					root["result"][ii]["YOffset"] = sd[25].c_str();
					root["result"][ii]["PlanID"] = sd[26].c_str();
					Json::Value jsonArray;
					jsonArray.append(atoi(sd[26].c_str()));
					root["result"][ii]["PlanIDs"] = jsonArray;
					root["result"][ii]["AddjValue"] = AddjValue;
					root["result"][ii]["AddjMulti"] = AddjMulti;
					root["result"][ii]["AddjValue2"] = AddjValue2;
					root["result"][ii]["AddjMulti2"] = AddjMulti2;

					std::stringstream s_data;
					s_data << int(nValue) << ", " << sValue;
					root["result"][ii]["Data"] = s_data.str();

					root["result"][ii]["Notifications"] = (m_notifications.HasNotifications(sd[0]) == true) ? "true" : "false";
					root["result"][ii]["ShowNotifications"] = true;

					bool bHasTimers = false;

					if (
						(dType == pTypeLighting1)
						|| (dType == pTypeLighting2)
						|| (dType == pTypeLighting3)
						|| (dType == pTypeLighting4)
						|| (dType == pTypeLighting5)
						|| (dType == pTypeLighting6)
						|| (dType == pTypeFan)
						|| (dType == pTypeColorSwitch)
						|| (dType == pTypeCurtain)
						|| (dType == pTypeBlinds)
						|| (dType == pTypeRFY)
						|| (dType == pTypeChime)
						|| (dType == pTypeThermostat2)
						|| (dType == pTypeThermostat3)
						|| (dType == pTypeThermostat4)
						|| (dType == pTypeRemote)
						|| (dType == pTypeGeneralSwitch)
						|| (dType == pTypeHomeConfort)
						|| (dType == pTypeFS20)
						|| ((dType == pTypeRadiator1) && (dSubType == sTypeSmartwaresSwitchRadiator))
						|| ((dType == pTypeRego6XXValue) && (dSubType == sTypeRego6XXStatus))
						|| (dType == pTypeHunter)
						|| (dType == pTypeDDxxxx)
						)
					{
						// add light details
						bHasTimers = m_sql.HasTimers(sd[0]);

						bHaveTimeout = false;
						root["result"][ii]["HaveTimeout"] = bHaveTimeout;

						std::string lstatus;
						int llevel = 0;
						bool bHaveDimmer = false;
						bool bHaveGroupCmd = false;
						int maxDimLevel = 0;

						GetLightStatus(dType, dSubType, switchtype, nValue, sValue, lstatus, llevel, bHaveDimmer, maxDimLevel, bHaveGroupCmd);

						root["result"][ii]["Status"] = lstatus;
						root["result"][ii]["StrParam1"] = strParam1;
						root["result"][ii]["StrParam2"] = strParam2;

						if (!CustomImage)
							root["result"][ii]["Image"] = "Light";

						if (switchtype == STYPE_Dimmer)
						{
							root["result"][ii]["Level"] = LastLevel;
							int iLevel = round((float(maxDimLevel) / 100.0F) * LastLevel);
							root["result"][ii]["LevelInt"] = iLevel;
							if ((dType == pTypeColorSwitch) || (dType == pTypeLighting5 && dSubType == sTypeTRC02) ||
								(dType == pTypeLighting5 && dSubType == sTypeTRC02_2) || (dType == pTypeGeneralSwitch && dSubType == sSwitchTypeTRC02) ||
								(dType == pTypeGeneralSwitch && dSubType == sSwitchTypeTRC02_2))
							{
								_tColor color(sColor);
								std::string jsonColor = color.toJSONString();
								root["result"][ii]["Color"] = jsonColor;
								llevel = LastLevel;
								if (lstatus == "Set Level" || lstatus == "Set Color")
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
						std::string DimmerType = "none";
						if (switchtype == STYPE_Dimmer)
						{
							DimmerType = "abs";
							if (_hardwareNames.find(hardwareID) != _hardwareNames.end())
							{
								// Milight V4/V5 bridges do not support absolute dimming for RGB or CW_WW lights
								if (_hardwareNames[hardwareID].HardwareTypeVal == HTYPE_LimitlessLights &&
									atoi(_hardwareNames[hardwareID].Mode2.c_str()) != CLimitLess::LBTYPE_V6 &&
									(atoi(_hardwareNames[hardwareID].Mode1.c_str()) == sTypeColor_RGB ||
										atoi(_hardwareNames[hardwareID].Mode1.c_str()) == sTypeColor_White ||
										atoi(_hardwareNames[hardwareID].Mode1.c_str()) == sTypeColor_CW_WW))
								{
									DimmerType = "rel";
								}
							}
						}
						root["result"][ii]["DimmerType"] = DimmerType;
						root["result"][ii]["MaxDimLevel"] = maxDimLevel;
						root["result"][ii]["HaveGroupCmd"] = bHaveGroupCmd;
						root["result"][ii]["SwitchType"] = Switch_Type_Desc(switchtype);
						root["result"][ii]["SwitchTypeVal"] = switchtype;
						uint64_t camIDX = m_mainworker.m_cameras.IsDevSceneInCamera(0, sd[0]);
						root["result"][ii]["UsedByCamera"] = (camIDX != 0) ? true : false;
						if (camIDX != 0)
						{
							std::stringstream scidx;
							scidx << camIDX;
							root["result"][ii]["CameraIdx"] = scidx.str();
							root["result"][ii]["CameraAspect"] = m_mainworker.m_cameras.GetCameraAspectRatio(scidx.str());
						}

						bool bIsSubDevice = false;
						std::vector<std::vector<std::string>> resultSD;
						resultSD = m_sql.safe_query("SELECT ID FROM LightSubDevices WHERE (DeviceRowID=='%q')", sd[0].c_str());
						bIsSubDevice = (!resultSD.empty());

						root["result"][ii]["IsSubDevice"] = bIsSubDevice;

						std::string openStatus = "Open";
						std::string closedStatus = "Closed";
						if (switchtype == STYPE_Doorbell)
						{
							root["result"][ii]["TypeImg"] = "doorbell";
							root["result"][ii]["Status"] = ""; //"Pressed";
						}
						else if (switchtype == STYPE_DoorContact)
						{
							if (!CustomImage)
								root["result"][ii]["Image"] = "Door";
							root["result"][ii]["TypeImg"] = "door";
							bool bIsOn = IsLightSwitchOn(lstatus);
							root["result"][ii]["InternalState"] = (bIsOn == true) ? "Open" : "Closed";
							if (bIsOn)
							{
								lstatus = "Open";
							}
							else
							{
								lstatus = "Closed";
							}
							root["result"][ii]["Status"] = lstatus;
						}
						else if (switchtype == STYPE_DoorLock)
						{
							if (!CustomImage)
								root["result"][ii]["Image"] = "Door";
							root["result"][ii]["TypeImg"] = "door";
							bool bIsOn = IsLightSwitchOn(lstatus);
							root["result"][ii]["InternalState"] = (bIsOn == true) ? "Locked" : "Unlocked";
							if (bIsOn)
							{
								lstatus = "Locked";
							}
							else
							{
								lstatus = "Unlocked";
							}
							root["result"][ii]["Status"] = lstatus;
						}
						else if (switchtype == STYPE_DoorLockInverted)
						{
							if (!CustomImage)
								root["result"][ii]["Image"] = "Door";
							root["result"][ii]["TypeImg"] = "door";
							bool bIsOn = IsLightSwitchOn(lstatus);
							root["result"][ii]["InternalState"] = (bIsOn == true) ? "Unlocked" : "Locked";
							if (bIsOn)
							{
								lstatus = "Unlocked";
							}
							else
							{
								lstatus = "Locked";
							}
							root["result"][ii]["Status"] = lstatus;
						}
						else if (switchtype == STYPE_PushOn)
						{
							if (!CustomImage)
								root["result"][ii]["Image"] = "Push";
							root["result"][ii]["TypeImg"] = "push";
							root["result"][ii]["Status"] = "";
							root["result"][ii]["InternalState"] = (IsLightSwitchOn(lstatus) == true) ? "On" : "Off";
						}
						else if (switchtype == STYPE_PushOff)
						{
							if (!CustomImage)
								root["result"][ii]["Image"] = "Push";
							root["result"][ii]["TypeImg"] = "push";
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
							if (!CustomImage)
								root["result"][ii]["Image"] = "Contact";
							root["result"][ii]["TypeImg"] = "contact";
							bool bIsOn = IsLightSwitchOn(lstatus);
							if (bIsOn)
							{
								lstatus = "Open";
							}
							else
							{
								lstatus = "Closed";
							}
							root["result"][ii]["Status"] = lstatus;
						}
						else if (switchtype == STYPE_Media)
						{
							if ((pHardware != nullptr) && (pHardware->HwdType == HTYPE_LogitechMediaServer))
								root["result"][ii]["TypeImg"] = "LogitechMediaServer";
							else
								root["result"][ii]["TypeImg"] = "Media";
							root["result"][ii]["Status"] = Media_Player_States((_eMediaStatus)nValue);
							lstatus = sValue;
						}
						else if (
							(switchtype == STYPE_Blinds)
							|| (switchtype == STYPE_BlindsPercentage)
							|| (switchtype == STYPE_BlindsPercentageWithStop)
							|| (switchtype == STYPE_VenetianBlindsUS)
							|| (switchtype == STYPE_VenetianBlindsEU)
							)
						{
							root["result"][ii]["Image"] = "blinds";
							root["result"][ii]["TypeImg"] = "blinds";

							if (lstatus == "Close inline relay")
							{
								lstatus = "Close";
							}
							else if (lstatus == "Open inline relay")
							{
								lstatus = "Open";
							}
							else if (lstatus == "Stop inline relay")
							{
								lstatus = "Stop";
							}

							bool bReverseState = false;
							bool bReversePosition = false;

							auto itt = options.find("ReverseState");
							if (itt != options.end())
								bReverseState = (itt->second == "true");
							itt = options.find("ReversePosition");
							if (itt != options.end())
								bReversePosition = (itt->second == "true");

							if (bReversePosition)
							{
								LastLevel = 100 - LastLevel;
								if (lstatus.find("Set Level") == 0)
									lstatus = std_format("Set Level: %d %%", LastLevel);
							}

							if (bReverseState)
							{
								if (lstatus == "Open")
									lstatus = "Close";
								else if (lstatus == "Close")
									lstatus = "Open";
							}


							if (lstatus == "Close")
							{
								lstatus = closedStatus;
							}
							else if (lstatus == "Open")
							{
								lstatus = openStatus;
							}
							else if (lstatus == "Stop")
							{
								lstatus = "Stopped";
							}
							root["result"][ii]["Status"] = lstatus;

							root["result"][ii]["Level"] = LastLevel;
							int iLevel = round((float(maxDimLevel) / 100.0F) * LastLevel);
							root["result"][ii]["LevelInt"] = iLevel;

							root["result"][ii]["ReverseState"] = bReverseState;
							root["result"][ii]["ReversePosition"] = bReversePosition;
						}
						else if (switchtype == STYPE_Dimmer)
						{
							root["result"][ii]["TypeImg"] = "dimmer";
						}
						else if (switchtype == STYPE_Motion)
						{
							root["result"][ii]["TypeImg"] = "motion";
						}
						else if (switchtype == STYPE_Selector)
						{
							std::string selectorStyle = options["SelectorStyle"];
							std::string levelOffHidden = options["LevelOffHidden"];
							std::string levelNames = options["LevelNames"];
							std::string levelActions = options["LevelActions"];
							if (selectorStyle.empty())
							{
								selectorStyle.assign("0"); // default is 'button set'
							}
							if (levelOffHidden.empty())
							{
								levelOffHidden.assign("false"); // default is 'not hidden'
							}
							if (levelNames.empty())
							{
								levelNames.assign("Off"); // default is Off only
							}
							root["result"][ii]["TypeImg"] = "Light";
							root["result"][ii]["SelectorStyle"] = atoi(selectorStyle.c_str());
							root["result"][ii]["LevelOffHidden"] = (levelOffHidden == "true");
							root["result"][ii]["LevelNames"] = base64_encode(levelNames);
							root["result"][ii]["LevelActions"] = base64_encode(levelActions);
						}
						root["result"][ii]["Data"] = lstatus;
					}
					else if (dType == pTypeSecurity1)
					{
						std::string lstatus;
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
						root["result"][ii]["SwitchTypeVal"] = switchtype; // was 0?;
						root["result"][ii]["TypeImg"] = "security";
						root["result"][ii]["StrParam1"] = strParam1;
						root["result"][ii]["StrParam2"] = strParam2;
						root["result"][ii]["Protected"] = (iProtected != 0);

						if ((dSubType == sTypeKD101) || (dSubType == sTypeSA30) || (dSubType == sTypeRM174RF) || (switchtype == STYPE_SMOKEDETECTOR))
						{
							root["result"][ii]["SwitchTypeVal"] = STYPE_SMOKEDETECTOR;
							root["result"][ii]["TypeImg"] = "smoke";
							root["result"][ii]["SwitchType"] = Switch_Type_Desc(STYPE_SMOKEDETECTOR);
						}
						root["result"][ii]["Data"] = lstatus;
						root["result"][ii]["HaveTimeout"] = false;
					}
					else if (dType == pTypeSecurity2)
					{
						std::string lstatus;
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
						root["result"][ii]["SwitchTypeVal"] = switchtype; // was 0?;
						root["result"][ii]["TypeImg"] = "security";
						root["result"][ii]["StrParam1"] = strParam1;
						root["result"][ii]["StrParam2"] = strParam2;
						root["result"][ii]["Protected"] = (iProtected != 0);
						root["result"][ii]["Data"] = lstatus;
						root["result"][ii]["HaveTimeout"] = false;
					}
					else if (dType == pTypeEvohome || dType == pTypeEvohomeRelay)
					{
						std::string lstatus;
						int llevel = 0;
						bool bHaveDimmer = false;
						bool bHaveGroupCmd = false;
						int maxDimLevel = 0;

						GetLightStatus(dType, dSubType, switchtype, nValue, sValue, lstatus, llevel, bHaveDimmer, maxDimLevel, bHaveGroupCmd);

						root["result"][ii]["Status"] = lstatus;
						root["result"][ii]["HaveDimmer"] = bHaveDimmer;
						root["result"][ii]["MaxDimLevel"] = maxDimLevel;
						root["result"][ii]["HaveGroupCmd"] = bHaveGroupCmd;
						root["result"][ii]["SwitchType"] = "evohome";
						root["result"][ii]["SwitchTypeVal"] = switchtype; // was 0?;
						root["result"][ii]["TypeImg"] = "override_mini";
						root["result"][ii]["StrParam1"] = strParam1;
						root["result"][ii]["StrParam2"] = strParam2;
						root["result"][ii]["Protected"] = (iProtected != 0);

						root["result"][ii]["Data"] = lstatus;
						root["result"][ii]["HaveTimeout"] = false;

						if (dType == pTypeEvohomeRelay)
						{
							root["result"][ii]["SwitchType"] = "TPI";
							root["result"][ii]["Level"] = llevel;
							root["result"][ii]["LevelInt"] = atoi(sValue.c_str());
							if (root["result"][ii]["Unit"].asInt() > 100)
								root["result"][ii]["Protected"] = true;

							sprintf(szData, "%s: %d", lstatus.c_str(), atoi(sValue.c_str()));
							root["result"][ii]["Data"] = szData;
						}
					}
					else if ((dType == pTypeEvohomeZone) || (dType == pTypeEvohomeWater))
					{
						root["result"][ii]["HaveTimeout"] = bHaveTimeout;
						root["result"][ii]["TypeImg"] = "override_mini";

						std::vector<std::string> strarray;
						StringSplit(sValue, ";", strarray);
						if (strarray.size() >= 3)
						{
							int i = 0;
							double tempCelcius = atof(strarray[i++].c_str());
							double temp = ConvertTemperature(tempCelcius, tempsign);
							double tempSetPoint;
							root["result"][ii]["Temp"] = temp;
							if (dType == pTypeEvohomeWater && (strarray[i] == "Off" || strarray[i] == "On"))
							{
								root["result"][ii]["State"] = strarray[i++];
							}
							else
							{
								tempCelcius = atof(strarray[i++].c_str());
								tempSetPoint = ConvertTemperature(tempCelcius, tempsign);
								root["result"][ii]["SetPoint"] = tempSetPoint;
							}

							std::string strstatus = strarray[i++];
							root["result"][ii]["Status"] = strstatus;

							if ((dType == pTypeEvohomeZone || dType == pTypeEvohomeWater) && strarray.size() >= 4)
							{
								root["result"][ii]["Until"] = strarray[i++];
							}
							if (dType == pTypeEvohomeZone)
							{
								if (tempCelcius == 325.1)
									sprintf(szTmp, "Off");
								else
									sprintf(szTmp, "%.1f %c", tempSetPoint, tempsign);
								if (strarray.size() >= 4)
									sprintf(szData, "%.1f %c, (%s), %s until %s", temp, tempsign, szTmp, strstatus.c_str(), strarray[3].c_str());
								else
									sprintf(szData, "%.1f %c, (%s), %s", temp, tempsign, szTmp, strstatus.c_str());
							}
							else if (strarray.size() >= 4)
								sprintf(szData, "%.1f %c, %s, %s until %s", temp, tempsign, strarray[1].c_str(), strstatus.c_str(), strarray[3].c_str());
							else
								sprintf(szData, "%.1f %c, %s, %s", temp, tempsign, strarray[1].c_str(), strstatus.c_str());
							root["result"][ii]["Data"] = szData;
							root["result"][ii]["HaveTimeout"] = bHaveTimeout;
						}
					}
					else if ((dType == pTypeTEMP) || (dType == pTypeRego6XXTemp))
					{
						double tvalue = ConvertTemperature(atof(sValue.c_str()), tempsign);
						root["result"][ii]["Temp"] = tvalue;
						sprintf(szData, "%.1f %c", tvalue, tempsign);
						root["result"][ii]["Data"] = szData;
						root["result"][ii]["HaveTimeout"] = bHaveTimeout;

						_tTrendCalculator::_eTendencyType tstate = _tTrendCalculator::_eTendencyType::TENDENCY_UNKNOWN;
						uint64_t tID = ((uint64_t)(hardwareID & 0x7FFFFFFF) << 32) | (devIdx & 0x7FFFFFFF);
						if (m_mainworker.m_trend_calculator.find(tID) != m_mainworker.m_trend_calculator.end())
						{
							tstate = m_mainworker.m_trend_calculator[tID].m_state;
						}
						root["result"][ii]["trend"] = (int)tstate;
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
						_tTrendCalculator::_eTendencyType tstate = _tTrendCalculator::_eTendencyType::TENDENCY_UNKNOWN;
						uint64_t tID = ((uint64_t)(hardwareID & 0x7FFFFFFF) << 32) | (devIdx & 0x7FFFFFFF);
						if (m_mainworker.m_trend_calculator.find(tID) != m_mainworker.m_trend_calculator.end())
						{
							tstate = m_mainworker.m_trend_calculator[tID].m_state;
						}
						root["result"][ii]["trend"] = (int)tstate;
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

							// Calculate dew point

							sprintf(szTmp, "%.2f", ConvertTemperature(CalculateDewPoint(tempCelcius, humidity), tempsign));
							root["result"][ii]["DewPoint"] = szTmp;

							_tTrendCalculator::_eTendencyType tstate = _tTrendCalculator::_eTendencyType::TENDENCY_UNKNOWN;
							uint64_t tID = ((uint64_t)(hardwareID & 0x7FFFFFFF) << 32) | (devIdx & 0x7FFFFFFF);
							if (m_mainworker.m_trend_calculator.find(tID) != m_mainworker.m_trend_calculator.end())
							{
								tstate = m_mainworker.m_trend_calculator[tID].m_state;
							}
							root["result"][ii]["trend"] = (int)tstate;
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
								sprintf(szData, "%.1f %c, %d %%, %.1f hPa", temp, tempsign, atoi(strarray[1].c_str()), atof(strarray[3].c_str()));
							}
							else
							{
								sprintf(szData, "%.1f %c, %d %%, %d hPa", temp, tempsign, atoi(strarray[1].c_str()), atoi(strarray[3].c_str()));
							}
							root["result"][ii]["Data"] = szData;
							root["result"][ii]["HaveTimeout"] = bHaveTimeout;

							_tTrendCalculator::_eTendencyType tstate = _tTrendCalculator::_eTendencyType::TENDENCY_UNKNOWN;
							uint64_t tID = ((uint64_t)(hardwareID & 0x7FFFFFFF) << 32) | (devIdx & 0x7FFFFFFF);
							if (m_mainworker.m_trend_calculator.find(tID) != m_mainworker.m_trend_calculator.end())
							{
								tstate = m_mainworker.m_trend_calculator[tID].m_state;
							}
							root["result"][ii]["trend"] = (int)tstate;
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

							sprintf(szData, "%.1f %c, %.1f hPa", tvalue, tempsign, atof(strarray[1].c_str()));
							root["result"][ii]["Data"] = szData;
							root["result"][ii]["HaveTimeout"] = bHaveTimeout;

							_tTrendCalculator::_eTendencyType tstate = _tTrendCalculator::_eTendencyType::TENDENCY_UNKNOWN;
							uint64_t tID = ((uint64_t)(hardwareID & 0x7FFFFFFF) << 32) | (devIdx & 0x7FFFFFFF);
							if (m_mainworker.m_trend_calculator.find(tID) != m_mainworker.m_trend_calculator.end())
							{
								tstate = m_mainworker.m_trend_calculator[tID].m_state;
							}
							root["result"][ii]["trend"] = (int)tstate;
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

								_tTrendCalculator::_eTendencyType tstate = _tTrendCalculator::_eTendencyType::TENDENCY_UNKNOWN;
								uint64_t tID = ((uint64_t)(hardwareID & 0x7FFFFFFF) << 32) | (devIdx & 0x7FFFFFFF);
								if (m_mainworker.m_trend_calculator.find(tID) != m_mainworker.m_trend_calculator.end())
								{
									tstate = m_mainworker.m_trend_calculator[tID].m_state;
								}
								root["result"][ii]["trend"] = (int)tstate;
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
								if (m_sql.m_windunit != WINDUNIT_Beaufort)
								{
									sprintf(szTmp, "%.1f", float(intSpeed) * m_sql.m_windscale);
								}
								else
								{
									float windms = float(intSpeed) * 0.1F;
									sprintf(szTmp, "%d", MStoBeaufort(windms));
								}
								root["result"][ii]["Speed"] = szTmp;
							}

							// if (dSubType!=sTypeWIND6) //problem in RFXCOM firmware? gust=speed?
							{
								int intGust = atoi(strarray[3].c_str());
								if (m_sql.m_windunit != WINDUNIT_Beaufort)
								{
									sprintf(szTmp, "%.1f", float(intGust) * m_sql.m_windscale);
								}
								else
								{
									float gustms = float(intGust) * 0.1F;
									sprintf(szTmp, "%d", MStoBeaufort(gustms));
								}
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

								_tTrendCalculator::_eTendencyType tstate = _tTrendCalculator::_eTendencyType::TENDENCY_UNKNOWN;
								uint64_t tID = ((uint64_t)(hardwareID & 0x7FFFFFFF) << 32) | (devIdx & 0x7FFFFFFF);
								if (m_mainworker.m_trend_calculator.find(tID) != m_mainworker.m_trend_calculator.end())
								{
									tstate = m_mainworker.m_trend_calculator[tID].m_state;
								}
								root["result"][ii]["trend"] = (int)tstate;
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
							// get lowest value of today, and max rate
							time_t now = mytime(nullptr);
							struct tm ltime;
							localtime_r(&now, &ltime);
							char szDate[40];
							sprintf(szDate, "%04d-%02d-%02d", ltime.tm_year + 1900, ltime.tm_mon + 1, ltime.tm_mday);

							std::vector<std::vector<std::string>> result2;

							if (dSubType == sTypeRAINWU || dSubType == sTypeRAINByRate)
							{
								result2 = m_sql.safe_query("SELECT Total, Rate FROM Rain WHERE (DeviceRowID='%q' AND Date>='%q') ORDER BY ROWID DESC LIMIT 1",
									sd[0].c_str(), szDate);
							}
							else
							{
								result2 = m_sql.safe_query("SELECT MIN(Total), MAX(Total) FROM Rain WHERE (DeviceRowID='%q' AND Date>='%q')", sd[0].c_str(), szDate);
							}

							if (!result2.empty())
							{
								double total_real = 0;
								float rate = 0;
								std::vector<std::string> sd2 = result2[0];

								if (dSubType == sTypeRAINWU || dSubType == sTypeRAINByRate)
								{
									total_real = atof(sd2[0].c_str());
								}
								else
								{
									double total_min = atof(sd2[0].c_str());
									double total_max = atof(strarray[1].c_str());
									total_real = total_max - total_min;
								}

								total_real *= AddjMulti;
								if (dSubType == sTypeRAINByRate)
								{
									rate = static_cast<float>(atof(sd2[1].c_str()) / 10000.0F);
								}
								else
								{
									rate = (static_cast<float>(atof(strarray[0].c_str())) / 100.0F) * float(AddjMulti);
								}

								sprintf(szTmp, "%.1f", total_real);
								root["result"][ii]["Rain"] = szTmp;
								sprintf(szTmp, "%g", rate);
								root["result"][ii]["RainRate"] = szTmp;
								root["result"][ii]["Data"] = sValue;
								root["result"][ii]["HaveTimeout"] = bHaveTimeout;
							}
							else
							{
								root["result"][ii]["Rain"] = "0";
								root["result"][ii]["RainRate"] = "0";
								root["result"][ii]["Data"] = "0";
								root["result"][ii]["HaveTimeout"] = bHaveTimeout;
							}
						}
					}
					else if (dType == pTypeRFXMeter)
					{
						std::string ValueQuantity = options["ValueQuantity"];
						std::string ValueUnits = options["ValueUnits"];
						float divider = m_sql.GetCounterDivider(int(metertype), int(dType), float(AddjValue2));

						if (ValueQuantity.empty())
						{
							ValueQuantity = "Custom";
						}

						// get value of today
						time_t now = mytime(nullptr);
						struct tm ltime;
						localtime_r(&now, &ltime);
						char szDate[40];
						sprintf(szDate, "%04d-%02d-%02d", ltime.tm_year + 1900, ltime.tm_mon + 1, ltime.tm_mday);

						std::vector<std::vector<std::string>> result2;
						strcpy(szTmp, "0");
						result2 = m_sql.safe_query("SELECT Value FROM Meter WHERE (DeviceRowID='%q' AND Date>='%q') ORDER BY Date LIMIT 1", sd[0].c_str(), szDate);
						if (!result2.empty())
						{
							std::vector<std::string> sd2 = result2[0];

							int64_t total_first = std::stoll(sd2[0]);
							int64_t total_last = std::stoll(sValue);
							int64_t total_real = total_last - total_first;

							sprintf(szTmp, "%" PRId64, total_real);

							double musage = 0.0F;
							switch (metertype)
							{
							case MTYPE_ENERGY:
							case MTYPE_ENERGY_GENERATED:
								musage = double(total_real) / divider;
								sprintf(szTmp, "%.3f kWh", musage);
								break;
							case MTYPE_GAS:
								musage = double(total_real) / divider;
								sprintf(szTmp, "%.3f m3", musage);
								break;
							case MTYPE_WATER:
								musage = double(total_real) / (divider / 1000.0F);
								sprintf(szTmp, "%d Liter", round(musage));
								break;
							case MTYPE_COUNTER:
								musage = double(total_real) / divider;
								sprintf(szTmp, "%.10g", musage);
								if (!ValueUnits.empty())
								{
									strcat(szTmp, " ");
									strcat(szTmp, ValueUnits.c_str());
								}
								break;
							default:
								strcpy(szTmp, "?");
								break;
							}
						}
						root["result"][ii]["CounterToday"] = szTmp;

						root["result"][ii]["SwitchTypeVal"] = metertype;
						root["result"][ii]["HaveTimeout"] = bHaveTimeout;
						root["result"][ii]["ValueQuantity"] = ValueQuantity;
						root["result"][ii]["ValueUnits"] = ValueUnits;
						root["result"][ii]["Divider"] = divider;

						double meteroffset = AddjValue;

						double dvalue = static_cast<double>(atof(sValue.c_str()));

						switch (metertype)
						{
						case MTYPE_ENERGY:
						case MTYPE_ENERGY_GENERATED:
							sprintf(szTmp, "%.3f kWh", meteroffset + (dvalue / divider));
							root["result"][ii]["Data"] = szTmp;
							root["result"][ii]["Counter"] = szTmp;
							break;
						case MTYPE_GAS:
							sprintf(szTmp, "%.3f m3", meteroffset + (dvalue / divider));
							root["result"][ii]["Data"] = szTmp;
							root["result"][ii]["Counter"] = szTmp;
							break;
						case MTYPE_WATER:
							sprintf(szTmp, "%.3f m3", meteroffset + (dvalue / divider));
							root["result"][ii]["Data"] = szTmp;
							root["result"][ii]["Counter"] = szTmp;
							break;
						case MTYPE_COUNTER:
							sprintf(szTmp, "%.10g", meteroffset + (dvalue / divider));
							if (!ValueUnits.empty())
							{
								strcat(szTmp, " ");
								strcat(szTmp, ValueUnits.c_str());
							}
							root["result"][ii]["Data"] = szTmp;
							root["result"][ii]["Counter"] = szTmp;
							break;
						default:
							root["result"][ii]["Data"] = "?";
							root["result"][ii]["Counter"] = "?";
							break;
						}
					}
					else if (dType == pTypeYouLess)
					{
						std::string ValueQuantity = options["ValueQuantity"];
						std::string ValueUnits = options["ValueUnits"];
						if (ValueQuantity.empty())
						{
							ValueQuantity = "Custom";
						}

						double musage = 0;
						double divider = m_sql.GetCounterDivider(int(metertype), int(dType), float(AddjValue2));

						// get value of today
						time_t now = mytime(nullptr);
						struct tm ltime;
						localtime_r(&now, &ltime);
						char szDate[40];
						sprintf(szDate, "%04d-%02d-%02d", ltime.tm_year + 1900, ltime.tm_mon + 1, ltime.tm_mday);

						std::vector<std::vector<std::string>> result2;
						strcpy(szTmp, "0");
						result2 = m_sql.safe_query("SELECT MIN(Value), MAX(Value) FROM Meter WHERE (DeviceRowID='%q' AND Date>='%q')", sd[0].c_str(), szDate);
						if (!result2.empty())
						{
							std::vector<std::string> sd2 = result2[0];

							uint64_t total_min = std::stoull(sd2[0]);
							uint64_t total_max = std::stoull(sd2[1]);
							uint64_t total_real = total_max - total_min;

							sprintf(szTmp, "%" PRIu64, total_real);

							musage = 0;
							switch (metertype)
							{
							case MTYPE_ENERGY:
							case MTYPE_ENERGY_GENERATED:
								musage = double(total_real) / divider;
								sprintf(szTmp, "%.3f kWh", musage);
								break;
							case MTYPE_GAS:
								musage = double(total_real) / divider;
								sprintf(szTmp, "%.3f m3", musage);
								break;
							case MTYPE_WATER:
								musage = double(total_real) / divider;
								sprintf(szTmp, "%.3f m3", musage);
								break;
							case MTYPE_COUNTER:
								sprintf(szTmp, "%.10g", double(total_real) / divider);
								if (!ValueUnits.empty())
								{
									strcat(szTmp, " ");
									strcat(szTmp, ValueUnits.c_str());
								}
								break;
							default:
								strcpy(szTmp, "0");
								break;
							}
						}
						root["result"][ii]["CounterToday"] = szTmp;

						std::vector<std::string> splitresults;
						StringSplit(sValue, ";", splitresults);
						if (splitresults.size() < 2)
							continue;

						uint64_t total_actual = std::stoull(splitresults[0]);
						musage = 0;
						switch (metertype)
						{
						case MTYPE_ENERGY:
						case MTYPE_ENERGY_GENERATED:
							musage = double(total_actual) / divider;
							sprintf(szTmp, "%.03f", musage);
							break;
						case MTYPE_GAS:
						case MTYPE_WATER:
							musage = double(total_actual) / divider;
							sprintf(szTmp, "%.03f", musage);
							break;
						case MTYPE_COUNTER:
							sprintf(szTmp, "%.10g", double(total_actual) / divider);
							break;
						default:
							strcpy(szTmp, "0");
							break;
						}
						root["result"][ii]["Counter"] = szTmp;

						root["result"][ii]["SwitchTypeVal"] = metertype;

						uint64_t acounter = std::stoull(sValue);
						musage = 0;
						switch (metertype)
						{
						case MTYPE_ENERGY:
						case MTYPE_ENERGY_GENERATED:
							musage = double(acounter) / divider;
							sprintf(szTmp, "%.3f kWh %s Watt", musage, splitresults[1].c_str());
							break;
						case MTYPE_GAS:
							musage = double(acounter) / divider;
							sprintf(szTmp, "%.3f m3", musage);
							break;
						case MTYPE_WATER:
							musage = double(acounter) / divider;
							sprintf(szTmp, "%.3f m3", musage);
							break;
						case MTYPE_COUNTER:
							sprintf(szTmp, "%.10g", double(acounter) / divider);
							if (!ValueUnits.empty())
							{
								strcat(szTmp, " ");
								strcat(szTmp, ValueUnits.c_str());
							}
							break;
						default:
							strcpy(szTmp, "0");
							break;
						}
						root["result"][ii]["Data"] = szTmp;
						root["result"][ii]["ValueQuantity"] = ValueQuantity;
						root["result"][ii]["ValueUnits"] = ValueUnits;
						root["result"][ii]["Divider"] = divider;

						switch (metertype)
						{
						case MTYPE_ENERGY:
						case MTYPE_ENERGY_GENERATED:
							sprintf(szTmp, "%s Watt", splitresults[1].c_str());
							break;
						case MTYPE_GAS:
							sprintf(szTmp, "%s m3", splitresults[1].c_str());
							break;
						case MTYPE_WATER:
							sprintf(szTmp, "%s m3", splitresults[1].c_str());
							break;
						case MTYPE_COUNTER:
							sprintf(szTmp, "%s", splitresults[1].c_str());
							break;
						default:
							strcpy(szTmp, "0");
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
						{
							root["result"][ii]["SwitchTypeVal"] = MTYPE_ENERGY;
							root["result"][ii]["Counter"] = "0";
							root["result"][ii]["CounterDeliv"] = "0";
							root["result"][ii]["Usage"] = "Invalid";
							root["result"][ii]["UsageDeliv"] = "Invalid";
							root["result"][ii]["Data"] = "Invalid!: " + sValue;
							root["result"][ii]["HaveTimeout"] = true;
							root["result"][ii]["CounterToday"] = "Invalid";
							root["result"][ii]["CounterDelivToday"] = "Invalid";
						}
						else
						{
							float EnergyDivider = 1000.0F;
							int tValue;
							if (m_sql.GetPreferencesVar("MeterDividerEnergy", tValue))
							{
								EnergyDivider = float(tValue);
							}

							uint64_t powerusage1 = std::stoull(splitresults[0]);
							uint64_t powerusage2 = std::stoull(splitresults[1]);
							uint64_t powerdeliv1 = std::stoull(splitresults[2]);
							uint64_t powerdeliv2 = std::stoull(splitresults[3]);
							uint64_t usagecurrent = std::stoull(splitresults[4]);
							uint64_t delivcurrent = std::stoull(splitresults[5]);

							powerdeliv1 = (powerdeliv1 < 10) ? 0 : powerdeliv1;
							powerdeliv2 = (powerdeliv2 < 10) ? 0 : powerdeliv2;

							uint64_t powerusage = powerusage1 + powerusage2;
							uint64_t powerdeliv = powerdeliv1 + powerdeliv2;
							if (powerdeliv < 2)
								powerdeliv = 0;

							double musage = 0;

							root["result"][ii]["SwitchTypeVal"] = MTYPE_ENERGY;
							musage = double(powerusage) / EnergyDivider;
							sprintf(szTmp, "%.03f", musage);
							root["result"][ii]["Counter"] = szTmp;
							musage = double(powerdeliv) / EnergyDivider;
							sprintf(szTmp, "%.03f", musage);
							root["result"][ii]["CounterDeliv"] = szTmp;

							if (bHaveTimeout)
							{
								usagecurrent = 0;
								delivcurrent = 0;
							}
							sprintf(szTmp, "%" PRIu64 " Watt", usagecurrent);
							root["result"][ii]["Usage"] = szTmp;
							sprintf(szTmp, "%" PRIu64 " Watt", delivcurrent);
							root["result"][ii]["UsageDeliv"] = szTmp;
							root["result"][ii]["Data"] = sValue;
							root["result"][ii]["HaveTimeout"] = bHaveTimeout;

							// get value of today
							time_t now = mytime(nullptr);
							struct tm ltime;
							localtime_r(&now, &ltime);
							char szDate[40];
							sprintf(szDate, "%04d-%02d-%02d", ltime.tm_year + 1900, ltime.tm_mon + 1, ltime.tm_mday);

							std::vector<std::vector<std::string>> result2;
							strcpy(szTmp, "0");
							result2 = m_sql.safe_query("SELECT MIN(Value1), MIN(Value2), MIN(Value5), MIN(Value6) FROM MultiMeter WHERE (DeviceRowID='%q' AND Date>='%q')",
								sd[0].c_str(), szDate);
							if (!result2.empty())
							{
								std::vector<std::string> sd2 = result2[0];

								uint64_t total_min_usage_1 = std::stoull(sd2[0]);
								uint64_t total_min_deliv_1 = std::stoull(sd2[1]);
								uint64_t total_min_usage_2 = std::stoull(sd2[2]);
								uint64_t total_min_deliv_2 = std::stoull(sd2[3]);
								uint64_t total_real_usage, total_real_deliv;

								total_min_deliv_1 = (total_min_deliv_1 < 10) ? 0 : total_min_deliv_1;
								total_min_deliv_2 = (total_min_deliv_2 < 10) ? 0 : total_min_deliv_2;

								total_real_usage = powerusage - (total_min_usage_1 + total_min_usage_2);
								total_real_deliv = powerdeliv - (total_min_deliv_1 + total_min_deliv_2);

								if (total_real_deliv < 2)
									total_real_deliv = 0;

								musage = double(total_real_usage) / EnergyDivider;
								sprintf(szTmp, "%.3f kWh", musage);
								root["result"][ii]["CounterToday"] = szTmp;
								musage = double(total_real_deliv) / EnergyDivider;
								sprintf(szTmp, "%.3f kWh", musage);
								root["result"][ii]["CounterDelivToday"] = szTmp;
							}
							else
							{
								sprintf(szTmp, "%.3f kWh", 0.0F);
								root["result"][ii]["CounterToday"] = szTmp;
								root["result"][ii]["CounterDelivToday"] = szTmp;
							}
						}
					}
					else if (dType == pTypeP1Gas)
					{
						root["result"][ii]["SwitchTypeVal"] = MTYPE_GAS;

						// get lowest value of today
						time_t now = mytime(nullptr);
						struct tm ltime;
						localtime_r(&now, &ltime);
						char szDate[40];
						sprintf(szDate, "%04d-%02d-%02d", ltime.tm_year + 1900, ltime.tm_mon + 1, ltime.tm_mday);

						std::vector<std::vector<std::string>> result2;

						float divider = m_sql.GetCounterDivider(int(metertype), int(dType), float(AddjValue2));

						strcpy(szTmp, "0");
						result2 = m_sql.safe_query("SELECT MIN(Value) FROM Meter WHERE (DeviceRowID='%q' AND Date>='%q')", sd[0].c_str(), szDate);
						if (!result2.empty())
						{
							std::vector<std::string> sd2 = result2[0];

							uint64_t total_min_gas = std::stoull(sd2[0]);
							uint64_t gasactual;
							try
							{
								gasactual = std::stoull(sValue);
							}
							catch (std::invalid_argument e)
							{
								_log.Log(LOG_ERROR, "Gas - invalid value: '%s'", sValue.c_str());
								continue;
							}
							uint64_t total_real_gas = gasactual - total_min_gas;

							double musage = double(gasactual) / divider;
							sprintf(szTmp, "%.03f", musage);
							root["result"][ii]["Counter"] = szTmp;
							musage = double(total_real_gas) / divider;
							sprintf(szTmp, "%.03f m3", musage);
							root["result"][ii]["CounterToday"] = szTmp;
							root["result"][ii]["HaveTimeout"] = bHaveTimeout;
							sprintf(szTmp, "%.03f", atof(sValue.c_str()) / divider);
							root["result"][ii]["Data"] = szTmp;
						}
						else
						{
							sprintf(szTmp, "%.03f", 0.0F);
							root["result"][ii]["Counter"] = szTmp;
							sprintf(szTmp, "%.03f m3", 0.0F);
							root["result"][ii]["CounterToday"] = szTmp;
							sprintf(szTmp, "%.03f", atof(sValue.c_str()) / divider);
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
							// CM113
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
									sprintf(szData, "%d Watt", int(val1 * voltage));
								else
									sprintf(szData, "%d Watt, %d Watt, %d Watt", int(val1 * voltage), int(val2 * voltage), int(val3 * voltage));
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
							// CM180i
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
								sprintf(szData, "%d Watt, %d Watt, %d Watt", int(atof(strarray[0].c_str()) * voltage), int(atof(strarray[1].c_str()) * voltage),
									int(atof(strarray[2].c_str()) * voltage));
							}
							if (total > 0)
							{
								sprintf(szTmp, ", Total: %.3f kWh", total / 1000.0F);
								strcat(szData, szTmp);
							}
							root["result"][ii]["Data"] = szData;
							root["result"][ii]["displaytype"] = displaytype;
							root["result"][ii]["HaveTimeout"] = bHaveTimeout;
						}
					}
					else if (((dType == pTypeENERGY) || (dType == pTypePOWER)) || ((dType == pTypeGeneral) && (dSubType == sTypeKwh)))
					{
						std::vector<std::string> strarray;
						StringSplit(sValue, ";", strarray);
						if (strarray.size() == 2)
						{
							double total = atof(strarray[1].c_str()) / 1000;

							time_t now = mytime(nullptr);
							struct tm ltime;
							localtime_r(&now, &ltime);
							char szDate[40];
							sprintf(szDate, "%04d-%02d-%02d", ltime.tm_year + 1900, ltime.tm_mon + 1, ltime.tm_mday);

							std::vector<std::vector<std::string>> result2;
							strcpy(szTmp, "0");
							// get the first value of the day instead of the minimum value, because counter can also decrease
							// result2 = m_sql.safe_query("SELECT MIN(Value) FROM Meter WHERE (DeviceRowID='%q' AND Date>='%q')",
							result2 = m_sql.safe_query("SELECT Value FROM Meter WHERE (DeviceRowID='%q' AND Date>='%q') ORDER BY Date LIMIT 1", sd[0].c_str(), szDate);
							if (!result2.empty())
							{
								float divider = m_sql.GetCounterDivider(int(metertype), int(dType), float(AddjValue2));

								std::vector<std::string> sd2 = result2[0];
								double minimum = atof(sd2[0].c_str()) / divider;

								sprintf(szData, "%.3f kWh", total);
								root["result"][ii]["Data"] = szData;
								if ((dType == pTypeENERGY) || (dType == pTypePOWER))
								{
									sprintf(szData, "%ld Watt", atol(strarray[0].c_str()));
								}
								else
								{
									sprintf(szData, "%g Watt", atof(strarray[0].c_str()));
								}
								root["result"][ii]["Usage"] = szData;
								root["result"][ii]["HaveTimeout"] = bHaveTimeout;
								sprintf(szTmp, "%.3f kWh", total - minimum);
								root["result"][ii]["CounterToday"] = szTmp;
							}
							else
							{
								sprintf(szData, "%.3f kWh", total);
								root["result"][ii]["Data"] = szData;
								if ((dType == pTypeENERGY) || (dType == pTypePOWER))
								{
									sprintf(szData, "%ld Watt", atol(strarray[0].c_str()));
								}
								else
								{
									sprintf(szData, "%g Watt", atof(strarray[0].c_str()));
								}
								root["result"][ii]["Usage"] = szData;
								root["result"][ii]["HaveTimeout"] = bHaveTimeout;
								sprintf(szTmp, "%d kWh", 0);
								root["result"][ii]["CounterToday"] = szTmp;
							}
						}
						root["result"][ii]["TypeImg"] = "current";
						root["result"][ii]["SwitchTypeVal"] = switchtype;		    // MTYPE_ENERGY
						root["result"][ii]["EnergyMeterMode"] = options["EnergyMeterMode"]; // for alternate Energy Reading
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
					else if (dType == pTypeSetpoint)
					{
						if (dSubType == sTypeSetpoint)
						{
							bHasTimers = m_sql.HasTimers(sd[0]);

							std::string value_step = options["ValueStep"];
							std::string value_min = options["ValueMin"];
							std::string value_max = options["ValueMax"];
							std::string value_unit = options["ValueUnit"];

							double valuestep = (!value_step.empty()) ? atof(value_step.c_str()) : 0.5;
							double valuemin = (!value_min.empty()) ? atof(value_min.c_str()) : -200.0;
							double valuemax = (!value_max.empty()) ? atof(value_max.c_str()) : 200.0;

							double value = atof(sValue.c_str());

							if (
								(value_unit.empty())
								|| (value_unit == "°C")
								|| (value_unit == "°F")
								|| (value_unit == "C")
								|| (value_unit == "F")
								)
							{
								if (tempsign == 'C')
									value_unit = "°C";
								else
									value_unit = "°F";

								double tempCelcius = value;
								double temp = ConvertTemperature(tempCelcius, tempsign);

								sprintf(szTmp, "%.1f", temp);
							}
							else
								sprintf(szTmp, "%g", value);

							root["result"][ii]["Data"] = szTmp;
							root["result"][ii]["SetPoint"] = szTmp;
							root["result"][ii]["HaveTimeout"] = bHaveTimeout;
							root["result"][ii]["step"] = valuestep;
							root["result"][ii]["min"] = valuemin;
							root["result"][ii]["max"] = valuemax;
							root["result"][ii]["vunit"] = value_unit;
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
							root["result"][ii]["HaveTimeout"] = false; // this device does not provide feedback, so no timeout!
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
								// km
								sprintf(szTmp, "%.1f km", vis);
							}
							else
							{
								// miles
								sprintf(szTmp, "%.1f mi", vis * 0.6214F);
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
								// Metric
								sprintf(szTmp, "%.1f cm", vis);
							}
							else
							{
								// Imperial
								sprintf(szTmp, "%.1f in", vis * 0.3937007874015748F);
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
							if (!CustomImage)
								root["result"][ii]["Image"] = "Computer";
							root["result"][ii]["TypeImg"] = "temperature";
							root["result"][ii]["Type"] = "temperature";
							_tTrendCalculator::_eTendencyType tstate = _tTrendCalculator::_eTendencyType::TENDENCY_UNKNOWN;
							uint64_t tID = ((uint64_t)(hardwareID & 0x7FFFFFFF) << 32) | (devIdx & 0x7FFFFFFF);
							if (m_mainworker.m_trend_calculator.find(tID) != m_mainworker.m_trend_calculator.end())
							{
								tstate = m_mainworker.m_trend_calculator[tID].m_state;
							}
							root["result"][ii]["trend"] = (int)tstate;
						}
						else if (dSubType == sTypePercentage)
						{
							sprintf(szData, "%g%%", atof(sValue.c_str()));
							root["result"][ii]["Data"] = szData;
							root["result"][ii]["HaveTimeout"] = bHaveTimeout;
							root["result"][ii]["TypeImg"] = "hardware";
						}
						else if (dSubType == sTypeWaterflow)
						{
							sprintf(szData, "%g l/min", atof(sValue.c_str()));
							root["result"][ii]["Data"] = szData;
							root["result"][ii]["HaveTimeout"] = bHaveTimeout;
							if (!CustomImage)
								root["result"][ii]["Image"] = "Moisture";
							root["result"][ii]["TypeImg"] = "moisture";
						}
						else if (dSubType == sTypeCustom)
						{
							std::string szAxesLabel;
							int SensorType = 1;
							std::vector<std::string> sResults;
							StringSplit(sOptions, ";", sResults);

							if (sResults.size() == 2)
							{
								SensorType = atoi(sResults[0].c_str());
								szAxesLabel = sResults[1];
							}
							sprintf(szData, "%g %s", atof(sValue.c_str()), szAxesLabel.c_str());
							root["result"][ii]["Data"] = szData;
							root["result"][ii]["SensorType"] = SensorType;
							root["result"][ii]["SensorUnit"] = szAxesLabel;
							root["result"][ii]["HaveTimeout"] = bHaveTimeout;

							if (!CustomImage)
								root["result"][ii]["Image"] = "Custom";
							root["result"][ii]["TypeImg"] = "Custom";
						}
						else if (dSubType == sTypeFan)
						{
							sprintf(szData, "%d RPM", atoi(sValue.c_str()));
							root["result"][ii]["Data"] = szData;
							root["result"][ii]["HaveTimeout"] = bHaveTimeout;
							if (!CustomImage)
								root["result"][ii]["Image"] = "Fan";
							root["result"][ii]["TypeImg"] = "Fan";
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
							sprintf(szData, "%g V", atof(sValue.c_str()));
							root["result"][ii]["Data"] = szData;
							root["result"][ii]["TypeImg"] = "current";
							root["result"][ii]["HaveTimeout"] = bHaveTimeout;
							root["result"][ii]["Voltage"] = atof(sValue.c_str());
						}
						else if (dSubType == sTypeCurrent)
						{
							sprintf(szData, "%g A", atof(sValue.c_str()));
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
							if (nValue > 4)
								nValue = 4;
							sprintf(szData, "Level: %d", nValue);
							root["result"][ii]["Data"] = szData;
							if (!sValue.empty())
								root["result"][ii]["Data"] = sValue;
							else
								root["result"][ii]["Data"] = Get_Alert_Desc(nValue);
							root["result"][ii]["TypeImg"] = "Alert";
							root["result"][ii]["Level"] = nValue;
							root["result"][ii]["HaveTimeout"] = false;
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
							std::vector<std::string> tstrarray;
							StringSplit(sValue, ";", tstrarray);
							if (tstrarray.empty())
								continue;
							sprintf(szData, "%g hPa", atof(tstrarray[0].c_str()));
							root["result"][ii]["Data"] = szData;
							root["result"][ii]["TypeImg"] = "gauge";
							root["result"][ii]["HaveTimeout"] = bHaveTimeout;
							if (tstrarray.size() > 1)
							{
								root["result"][ii]["Barometer"] = atof(tstrarray[0].c_str());
								int forecast = atoi(tstrarray[1].c_str());
								root["result"][ii]["Forecast"] = forecast;
								root["result"][ii]["ForecastStr"] = BMP_Forecast_Desc(forecast);
							}
						}
						else if (dSubType == sTypeCounterIncremental)
						{
							std::string ValueQuantity = options["ValueQuantity"];
							std::string ValueUnits = options["ValueUnits"];
							if (ValueQuantity.empty())
							{
								ValueQuantity = "Custom";
							}

							double divider = m_sql.GetCounterDivider(int(metertype), int(dType), float(AddjValue2));

							// get value of today
							time_t now = mytime(nullptr);
							struct tm ltime;
							localtime_r(&now, &ltime);
							char szDate[40];
							sprintf(szDate, "%04d-%02d-%02d", ltime.tm_year + 1900, ltime.tm_mon + 1, ltime.tm_mday);

							std::vector<std::vector<std::string>> result2;
							strcpy(szTmp, "0");
							result2 = m_sql.safe_query("SELECT Value FROM Meter WHERE (DeviceRowID='%q' AND Date>='%q') ORDER BY Date LIMIT 1", sd[0].c_str(), szDate);
							if (!result2.empty())
							{
								std::vector<std::string> sd2 = result2[0];

								int64_t total_first = std::stoll(sd2[0]);
								int64_t total_last = std::stoll(sValue);
								int64_t total_real = total_last - total_first;

								double musage = 0;
								switch (metertype)
								{
								case MTYPE_ENERGY:
								case MTYPE_ENERGY_GENERATED:
									musage = double(total_real) / divider;
									sprintf(szTmp, "%.3f kWh", musage);
									break;
								case MTYPE_GAS:
									musage = double(total_real) / divider;
									sprintf(szTmp, "%.3f m3", musage);
									break;
								case MTYPE_WATER:
									musage = double(total_real) / divider;
									sprintf(szTmp, "%.3f m3", musage);
									break;
								case MTYPE_COUNTER:
									sprintf(szTmp, "%.10g", double(total_real) / divider);
									if (!ValueUnits.empty())
									{
										strcat(szTmp, " ");
										strcat(szTmp, ValueUnits.c_str());
									}
									break;
								default:
									strcpy(szTmp, "0");
									break;
								}
							}
							root["result"][ii]["Counter"] = sValue;
							root["result"][ii]["CounterToday"] = szTmp;
							root["result"][ii]["SwitchTypeVal"] = metertype;
							root["result"][ii]["HaveTimeout"] = bHaveTimeout;
							root["result"][ii]["TypeImg"] = "counter";
							root["result"][ii]["ValueQuantity"] = ValueQuantity;
							root["result"][ii]["ValueUnits"] = ValueUnits;
							root["result"][ii]["Divider"] = divider;

							double dvalue = static_cast<double>(atof(sValue.c_str()));
							double meteroffset = AddjValue;

							switch (metertype)
							{
							case MTYPE_ENERGY:
							case MTYPE_ENERGY_GENERATED:
								sprintf(szTmp, "%.3f kWh", meteroffset + (dvalue / divider));
								root["result"][ii]["Data"] = szTmp;
								root["result"][ii]["Counter"] = szTmp;
								break;
							case MTYPE_GAS:
								sprintf(szTmp, "%.3f m3", meteroffset + (dvalue / divider));
								root["result"][ii]["Data"] = szTmp;
								root["result"][ii]["Counter"] = szTmp;
								break;
							case MTYPE_WATER:
								sprintf(szTmp, "%.3f m3", meteroffset + (dvalue / divider));
								root["result"][ii]["Data"] = szTmp;
								root["result"][ii]["Counter"] = szTmp;
								break;
							case MTYPE_COUNTER:
								sprintf(szTmp, "%.10g", meteroffset + (dvalue / divider));
								if (!ValueUnits.empty())
								{
									strcat(szTmp, " ");
									strcat(szTmp, ValueUnits.c_str());
								}
								root["result"][ii]["Data"] = szTmp;
								root["result"][ii]["Counter"] = szTmp;
								break;
							default:
								root["result"][ii]["Data"] = "?";
								root["result"][ii]["Counter"] = "?";
								break;
							}
						}
						else if (dSubType == sTypeManagedCounter)
						{
							std::string ValueQuantity = options["ValueQuantity"];
							std::string ValueUnits = options["ValueUnits"];
							if (ValueQuantity.empty())
							{
								ValueQuantity = "Custom";
							}

							float divider = m_sql.GetCounterDivider(int(metertype), int(dType), float(AddjValue2));

							std::vector<std::string> splitresults;
							StringSplit(sValue, ";", splitresults);
							double dvalue;
							if (splitresults.size() < 2)
							{
								dvalue = static_cast<double>(atof(sValue.c_str()));
							}
							else
							{
								dvalue = static_cast<double>(atof(splitresults[1].c_str()));
								if (dvalue < 0.0)
								{
									dvalue = static_cast<double>(atof(splitresults[0].c_str()));
								}
							}
							root["result"][ii]["SwitchTypeVal"] = metertype;
							root["result"][ii]["HaveTimeout"] = bHaveTimeout;
							root["result"][ii]["TypeImg"] = "counter";
							root["result"][ii]["ValueQuantity"] = ValueQuantity;
							root["result"][ii]["ValueUnits"] = ValueUnits;
							root["result"][ii]["Divider"] = divider;
							root["result"][ii]["ShowNotifications"] = false;
							double meteroffset = AddjValue;

							switch (metertype)
							{
							case MTYPE_ENERGY:
							case MTYPE_ENERGY_GENERATED:
								sprintf(szTmp, "%.3f kWh", meteroffset + (dvalue / divider));
								root["result"][ii]["Data"] = szTmp;
								root["result"][ii]["Counter"] = szTmp;
								break;
							case MTYPE_GAS:
								sprintf(szTmp, "%.3f m3", meteroffset + (dvalue / divider));
								root["result"][ii]["Data"] = szTmp;
								root["result"][ii]["Counter"] = szTmp;
								break;
							case MTYPE_WATER:
								sprintf(szTmp, "%.3f m3", meteroffset + (dvalue / divider));
								root["result"][ii]["Data"] = szTmp;
								root["result"][ii]["Counter"] = szTmp;
								break;
							case MTYPE_COUNTER:
								sprintf(szTmp, "%.10g", meteroffset + (dvalue / divider));
								if (!ValueUnits.empty())
								{
									strcat(szTmp, " ");
									strcat(szTmp, ValueUnits.c_str());
								}
								root["result"][ii]["Data"] = szTmp;
								root["result"][ii]["Counter"] = szTmp;
								break;
							default:
								root["result"][ii]["Data"] = "?";
								root["result"][ii]["Counter"] = "?";
								break;
							}
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
						sprintf(szTmp, "%g %s", m_sql.m_weightscale * atof(sValue.c_str()), m_sql.m_weightsign.c_str());
						root["result"][ii]["Data"] = szTmp;
						root["result"][ii]["HaveTimeout"] = false;
						root["result"][ii]["SwitchTypeVal"] = (m_sql.m_weightsign == "kg") ? 0 : 1;
					}
					else if (dType == pTypeUsage)
					{
						if (dSubType == sTypeElectric)
						{
							sprintf(szData, "%g Watt", atof(sValue.c_str()));
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
							root["result"][ii]["SwitchTypeVal"] = STYPE_OnOff;
							root["result"][ii]["SwitchType"] = Switch_Type_Desc(STYPE_OnOff);
							sprintf(szData, "%d", atoi(sValue.c_str()));
							root["result"][ii]["Data"] = szData;
							root["result"][ii]["HaveTimeout"] = bHaveTimeout;
							root["result"][ii]["StrParam1"] = strParam1;
							root["result"][ii]["StrParam2"] = strParam2;
							root["result"][ii]["Protected"] = (iProtected != 0);

							if (!CustomImage)
								root["result"][ii]["Image"] = "Light";
							root["result"][ii]["TypeImg"] = "utility";

							uint64_t camIDX = m_mainworker.m_cameras.IsDevSceneInCamera(0, sd[0]);
							root["result"][ii]["UsedByCamera"] = (camIDX != 0) ? true : false;
							if (camIDX != 0)
							{
								std::stringstream scidx;
								scidx << camIDX;
								root["result"][ii]["CameraIdx"] = scidx.str();
								root["result"][ii]["CameraAspect"] = m_mainworker.m_cameras.GetCameraAspectRatio(scidx.str());
							}

							root["result"][ii]["Level"] = 0;
							root["result"][ii]["LevelInt"] = atoi(sValue.c_str());
						}
						break;
						case sTypeRego6XXCounter:
						{
							// get value of today
							time_t now = mytime(nullptr);
							struct tm ltime;
							localtime_r(&now, &ltime);
							char szDate[40];
							sprintf(szDate, "%04d-%02d-%02d", ltime.tm_year + 1900, ltime.tm_mon + 1, ltime.tm_mday);

							std::vector<std::vector<std::string>> result2;
							strcpy(szTmp, "0");
							result2 = m_sql.safe_query("SELECT MIN(Value), MAX(Value) FROM Meter WHERE (DeviceRowID='%q' AND Date>='%q')", sd[0].c_str(), szDate);
							if (!result2.empty())
							{
								std::vector<std::string> sd2 = result2[0];

								uint64_t total_min = std::stoull(sd2[0]);
								uint64_t total_max = std::stoull(sd2[1]);
								uint64_t total_real = total_max - total_min;

								sprintf(szTmp, "%" PRIu64, total_real);
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
#ifdef ENABLE_PYTHON
					if (pHardware != nullptr)
					{
						if (pHardware->HwdType == HTYPE_PythonPlugin)
						{
							Plugins::CPlugin* pPlugin = (Plugins::CPlugin*)pHardware;
							bHaveTimeout = pPlugin->HasNodeFailed(sd[1].c_str(), atoi(sd[2].c_str()));
							root["result"][ii]["HaveTimeout"] = bHaveTimeout;
						}
					}
#endif
					root["result"][ii]["Timers"] = (bHasTimers == true) ? "true" : "false";
					ii++;
				}
				catch (const std::exception& e)
				{
					_log.Log(LOG_ERROR, "GetJSonDevices: exception occurred : '%s'", e.what());
					continue;
				}
			}
		}

		/*
		 * Takes root["result"] and groups all items according to sgroupby, summing all values for each category, then creating new items in root["result"]
		 * for each combination year/category.
		 */
		void CWebServer::GroupBy(Json::Value& root, std::string dbasetable, uint64_t idx, std::string sgroupby, bool bUseValuesOrCounter, std::function<std::string(std::string)> counter,
			std::function<std::string(std::string)> value, std::function<std::string(double)> sumToResult)
		{
			/*
			 * This query selects all records (in mc0) that belong to DeviceRowID, each with the record before it (in mc1), and calculates for each record
			 * the "usage" by subtracting the previous counter from its counter.
			 * - It does not take into account records that have a 0-valued counter, to prevent one falling between two categories, which would cause the
			 *   value for one category to be extremely low and the value for the other extremely high.
			 * - When the previous counter is greater than its counter, assumed is that a meter change has taken place; the previous counter is ignored
			 *   and the value of the record is taken as the "usage" (hoping for the best as the value is not always reliable.)
			 * - The reason why not simply the record values are summed, but instead the differences between all the individual counters are summed, is that
			 *   records for some days are not recorded or sometimes disappear, hence values would be missing and that would result in an incomplete total.
			 *   Plus it seems that the value is not always the same as the difference between the counters. Counters are more often reliable.
			 */
			std::string queryString;
			std::vector<std::vector<std::string>> result;
			bool bUseValues = false;

			/* if bUseValuesOrCounter is true, then find out if there are any Counter values in the table, if not: use Value instead of Counter */
			if (bUseValuesOrCounter)
			{
				queryString = "select count(*) from " + dbasetable + " where DeviceRowID = " + std::to_string(idx) + " and " + counter("") + " != 0 ";
				result = m_sql.safe_query(queryString.c_str(), idx, idx, idx, idx, idx);
				if (atoi(result[0][0].c_str()) == 0)
				{
					bUseValues = true;
				}
			}

			/* get the data */
			queryString = "";
			queryString.append(" select");
			queryString.append("  strftime('%%Y',Date) as Year,");

			if (!bUseValues)
			{
				queryString.append("  sum(Difference) as Sum");
			}
			else
			{
				queryString.append("  sum(Value) as Sum");
			}

			if (sgroupby == "quarter")
			{
				queryString.append(",case");
				queryString.append("   when cast(strftime('%%m',Date) as integer) between 1 and 3 then 'Q1'");
				queryString.append("   when cast(strftime('%%m',Date) as integer) between 4 and 6 then 'Q2'");
				queryString.append("   when cast(strftime('%%m',Date) as integer) between 7 and 9 then 'Q3'");
				queryString.append("                                                              else 'Q4'");
				queryString.append("   end as Quarter");
			}
			else if (sgroupby == "month")
			{
				queryString.append(",strftime('%%m',Date) as Month");
			}

			if (!bUseValues)
			{
				queryString.append(" from (");
				queryString.append(" 	select");
				queryString.append("         mc0.DeviceRowID,");
				queryString.append("         date(mc0.Date) as Date,");
				queryString.append("         case");
				queryString.append("            when (" + counter("mc1") + ") <= (" + counter("mc0") + ")");
				queryString.append("            then (" + counter("mc0") + ") - (" + counter("mc1") + ")");
				queryString.append("            else (" + value("mc0") + ")");
				queryString.append("         end as Difference");
				queryString.append(" 	from " + dbasetable + " mc0");
				queryString.append(" 	inner join " + dbasetable + " mc1 on mc1.DeviceRowID = mc0.DeviceRowID");
				queryString.append("         and mc1.Date = (");
				queryString.append("             select max(mcm.Date)");
				queryString.append("             from " + dbasetable + " mcm");
				queryString.append("             where mcm.DeviceRowID = mc0.DeviceRowID and mcm.Date < mc0.Date and (" + counter("mcm") + ") > 0");
				queryString.append("         )");
				queryString.append(" 	where");
				queryString.append("         mc0.DeviceRowID = %" PRIu64 "");
				queryString.append("         and (" + counter("mc0") + ") > 0");
				queryString.append("         and (select min(Date) from " + dbasetable + " where DeviceRowID = %" PRIu64 " and (" + counter("") + ") > 0) <= mc1.Date");
				queryString.append("         and mc0.Date <= (select max(Date) from " + dbasetable + " where DeviceRowID = %" PRIu64 " and (" + counter("") + ") > 0)");
				queryString.append("    union all");
				queryString.append("    select");
				queryString.append("         DeviceRowID,");
				queryString.append("         date(Date) as Date,");
				queryString.append("         " + value(""));
			}

			queryString.append(" 	from " + dbasetable);
			queryString.append(" 	where");
			queryString.append("         DeviceRowID = %" PRIu64 "");
			if (!bUseValues)
			{
				queryString.append("         and (select min(Date) from " + dbasetable + " where DeviceRowID = %" PRIu64 " and (" + counter("") + ") > 0) = Date");
				queryString.append(" )");
			}
			
			queryString.append(" group by strftime('%%Y',Date)");
			if (sgroupby == "quarter")
			{
				queryString.append(",case");
				queryString.append("   when cast(strftime('%%m',Date) as integer) between 1 and 3 then 'Q1'");
				queryString.append("   when cast(strftime('%%m',Date) as integer) between 4 and 6 then 'Q2'");
				queryString.append("   when cast(strftime('%%m',Date) as integer) between 7 and 9 then 'Q3'");
				queryString.append("                                                              else 'Q4'");
				queryString.append("   end");
			}
			else if (sgroupby == "month")
			{
				queryString.append(",strftime('%%m',Date)");
			}
			result = m_sql.safe_query(queryString.c_str(), idx, idx, idx, idx, idx);
			if (!result.empty())
			{
				int firstYearCounting = 0;
				double yearSumPrevious[12];
				int yearPrevious[12];
				for (const auto& sd : result)
				{
					const int year = atoi(sd[0].c_str());
					const double fsum = atof(sd[1].c_str());
					const int previousIndex = sgroupby == "year" ? 0 : sgroupby == "quarter" ? sd[2][1] - '0' - 1 : atoi(sd[2].c_str()) - 1;
					const double* sumPrevious = year - 1 != yearPrevious[previousIndex] ? NULL : &yearSumPrevious[previousIndex];
					const char* trend = !sumPrevious ? "" : *sumPrevious < fsum ? "up" : *sumPrevious > fsum ? "down" : "equal";
					const int ii = root["result"].size();
					if (firstYearCounting == 0 || year < firstYearCounting)
					{
						firstYearCounting = year;
					}
					root["result"][ii]["y"] = sd[0];
					root["result"][ii]["c"] = sgroupby == "year" ? sd[0] : sd[2];
					root["result"][ii]["s"] = sumToResult(fsum);
					root["result"][ii]["t"] = trend;
					yearSumPrevious[previousIndex] = fsum;
					yearPrevious[previousIndex] = year;
				}
				root["firstYear"] = firstYearCounting;
			}
		}

		/*
		 * Adds todayValue to root["result"], either by adding it to the value of the item with the corresponding category or by adding a new item with the
		 * respective category with todayValue. If root["firstYear"] is missing, the today's year is set in it's place.
		 */
		void CWebServer::AddTodayValueToResult(Json::Value& root, const std::string& sgroupby, const std::string& today, const double todayValue, const std::string& formatString)
		{
			std::string todayYear = today.substr(0, 4);
			std::string todayCategory;
			if (sgroupby == "quarter")
			{
				int todayMonth = atoi(today.substr(5, 2).c_str());
				if (todayMonth < 4)
					todayCategory = "Q1";
				else if (todayMonth < 7)
					todayCategory = "Q2";
				else if (todayMonth < 10)
					todayCategory = "Q3";
				else
					todayCategory = "Q4";
			}
			else if (sgroupby == "month")
			{
				todayCategory = today.substr(5, 2);
			}
			else
			{
				todayCategory = todayYear;
			}
			int todayResultIndex = -1;
			for (int resultIndex = 0; resultIndex < static_cast<int>(root["result"].size()) && todayResultIndex == -1; resultIndex++)
			{
				std::string resultYear = root["result"][resultIndex]["y"].asString();
				std::string resultCategory = root["result"][resultIndex]["c"].asString();
				if (resultYear == todayYear && todayCategory == resultCategory)
				{
					todayResultIndex = resultIndex;
				}
			}
			double resultPlusTodayValue = 0;
			if (todayResultIndex == -1)
			{
				todayResultIndex = root["result"].size();
				resultPlusTodayValue = todayValue;
				root["result"][todayResultIndex]["y"] = todayYear.c_str();
				root["result"][todayResultIndex]["c"] = todayCategory.c_str();
			}
			else
			{
				resultPlusTodayValue = atof(root["result"][todayResultIndex]["s"].asString().c_str()) + todayValue;
			}
			char szTmp[30];
			sprintf(szTmp, formatString.c_str(), resultPlusTodayValue);
			root["result"][todayResultIndex]["s"] = szTmp;

			if (!root.isMember("firstYear")) {
				root["firstYear"] = todayYear.c_str();
			}
		}

		void CWebServer::ReloadCustomSwitchIcons()
		{
			m_custom_light_icons.clear();
			m_custom_light_icons_lookup.clear();
			std::string sLine;

			// First get them from the switch_icons.txt file
			std::ifstream infile;
			std::string switchlightsfile = szWWWFolder + "/switch_icons.txt";
			infile.open(switchlightsfile.c_str());
			if (infile.is_open())
			{
				int index = 0;
				while (!infile.eof())
				{
					getline(infile, sLine);
					if (!sLine.empty())
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
							m_custom_light_icons_lookup[cImage.idx] = (int)m_custom_light_icons.size() - 1;
						}
					}
				}
				infile.close();
			}
			// Now get them from the database (idx 100+)
			std::vector<std::vector<std::string>> result;
			result = m_sql.safe_query("SELECT ID,Base,Name,Description FROM CustomImages");
			if (!result.empty())
			{
				int ii = 0;
				for (const auto& sd : result)
				{
					int ID = atoi(sd[0].c_str());

					_tCustomIcon cImage;
					cImage.idx = 100 + ID;
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

					// Check if files are on disk, else add them
					for (const auto& db : _dbImageFiles)
					{
						std::string TableField = db.first;
						std::string IconFile = db.second;

						if (!file_exist(IconFile.c_str()))
						{
							// Does not exists, extract it from the database and add it
							std::vector<std::vector<std::string>> result2;
							result2 = m_sql.safe_queryBlob("SELECT %s FROM CustomImages WHERE ID=%d", TableField.c_str(), ID);
							if (!result2.empty())
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
					m_custom_light_icons_lookup[cImage.idx] = (int)m_custom_light_icons.size() - 1;
					ii++;
				}
			}
		}

		// Primary API (v1) entry point
		void CWebServer::GetJSonPage(WebEmSession& session, const request& req, reply& rep)
		{
			Json::Value root;
			root["status"] = "ERR";

			std::string rtype = request::findValue(&req, "type");
			if (rtype == "command")
			{
				std::string cparam = request::findValue(&req, "param");
				if (!cparam.empty())
				{
					_log.Debug(DEBUG_WEBSERVER, "CWebServer::GetJSonPage :%s :%s ", cparam.c_str(), req.uri.c_str());

					auto pf = m_webcommands.find(cparam);
					if (pf != m_webcommands.end())
					{
						pf->second(session, req, root);
					}
					else
					{	// See if we still have a Param based version not converted to a proper command
						// TODO: remove this once all param based code has been converted to proper commands
						if (!HandleCommandParam(cparam, session, req, root))
						{
							_log.Debug(DEBUG_WEBSERVER, "CWebServer::GetJSonPage(param)(%s) returned an error!", cparam.c_str());
						}
					}
				}
			} //(rtype=="command")
			else
			{	// TODO: remove this after next stable
				// Could be a call to an old style RType, try to handle it and alert the user to update
				_log.Debug(DEBUG_WEBSERVER, "CWebServer::GetJSonPage(rtype) :%s :%s ", rtype.c_str(), req.uri.c_str());

				std::string altrtype;
				if (rtype.compare("settings") == 0)
				{
					altrtype = "getsettings";
				}
				else if (rtype.compare("users") == 0)
				{
					altrtype = "getusers";
				}
				else if (rtype.compare("devices") == 0)
				{
					altrtype = "getdevices";
				}
				else if (rtype.compare("hardware") == 0)
				{
					altrtype = "gethardware";
				}
				else if (rtype.compare("scenes") == 0)
				{
					altrtype = "getscenes";
				}
				else if (rtype.compare("notifications") == 0)
				{
					altrtype = "getnotifications";
				}
				else if (rtype.compare("scenelog") == 0)
				{
					altrtype = "getscenelog";
				}
				else if (rtype.compare("mobiles") == 0)
				{
					altrtype = "getmobiles";
				}
				else if (rtype.compare("cameras") == 0)
				{
					altrtype = "getcameras";
				}
				else if (rtype.compare("cameras_user") == 0)
				{
					altrtype = "getcameras_user";
				}
				else if (rtype.compare("schedules") == 0)
				{
					altrtype = "getschedules";
				}
				else if (rtype.compare("timers") == 0)
				{
					altrtype = "gettimers";
				}
				else if (rtype.compare("scenetimers") == 0)
				{
					altrtype = "getscenetimers";
				}
				else if (rtype.compare("setpointtimers") == 0)
				{
					altrtype = "getsetpointtimers";
				}
				else if (rtype.compare("plans") == 0)
				{
					altrtype = "getplans";
				}
				else if (rtype.compare("floorplans") == 0)
				{
					altrtype = "getfloorplans";
				}
				else if (rtype.compare("lightlog") == 0)
				{
					altrtype = "getlightlog";
				}
				else if (rtype.compare("textlog") == 0)
				{
					altrtype = "gettextlog";
				}
				else if (rtype.compare("graph") == 0)
				{
					altrtype = "graph";
				}
				else if (rtype.compare("createdevice") == 0)
				{
					altrtype = "createdevice";
				}
				else if (rtype.compare("setused") == 0)
				{
					altrtype = "setused";
				}

				if (!altrtype.empty())
				{
					auto pf = m_webcommands.find(altrtype);
					if (pf != m_webcommands.end())
					{
						_log.Log(LOG_NORM, "[WebServer] Deprecated RType (%s) for API request. Handled via fallback (%s), please use correct API Command! (%s)", rtype.c_str(), altrtype.c_str(), req.host_remote_address.c_str());
						pf->second(session, req, root);
					}
				}
				else
				{
					_log.Log(LOG_STATUS, "[WebServer] Deprecated RType (%s) for API request. Call ignored, please use correct API Command! (%s)", rtype.c_str(), req.host_remote_address.c_str());
				}

			}

			reply::set_content(&rep, root.toStyledString());
		}

		void CWebServer::UploadFloorplanImage(WebEmSession& session, const request& req, std::string& redirect_uri)
		{
			Json::Value root;
			root["title"] = "UploadFloorplanImage";
			root["status"] = "ERR";

			if (session.rights != 2)
			{
				session.reply_status = reply::forbidden;
				return; // Only admin user allowed
			}

			std::string planname = request::findValue(&req, "planname");
			std::string scalefactor = request::findValue(&req, "scalefactor");
			std::string imagefile = request::findValue(&req, "imagefile");

			std::vector<std::vector<std::string>> result;
			m_sql.safe_query("INSERT INTO Floorplans ([Name],[ScaleFactor]) VALUES('%s','%s')", planname.c_str(), scalefactor.c_str());
			result = m_sql.safe_query("SELECT MAX(ID) FROM Floorplans");
			if (!result.empty())
			{
				if (!m_sql.safe_UpdateBlobInTableWithID("Floorplans", "Image", result[0][0], imagefile))
				{
					_log.Log(LOG_ERROR, "SQL: Problem inserting floorplan image into database! ");
				}
				else
					root["status"] = "OK";
			}
			redirect_uri = root.toStyledString();
		}

		void CWebServer::GetServiceWorker(WebEmSession& session, const request& req, reply& rep)
		{
			// Return the appcache file (dynamically generated)
			std::string sLine;
			std::string filename = szWWWFolder + "/service-worker.js";

			std::string response;

			std::ifstream is(filename.c_str());
			if (is)
			{
				while (!is.eof())
				{
					getline(is, sLine);
					if (!sLine.empty())
					{
						if (sLine.find("#BuildHash") != std::string::npos)
						{
							stdreplace(sLine, "#BuildHash", szAppHash);
						}
					}
					response += sLine + '\n';
				}
			}
			reply::set_content(&rep, response);
		}

		void CWebServer::GetFloorplanImage(WebEmSession& session, const request& req, reply& rep)
		{
			std::string idx = request::findValue(&req, "idx");
			if (idx.empty())
			{
				return;
			}
			std::vector<std::vector<std::string>> result;
			result = m_sql.safe_queryBlob("SELECT Image FROM Floorplans WHERE ID=%d", atol(idx.c_str()));
			if (result.empty())
				return;
			reply::set_content(&rep, result[0][0].begin(), result[0][0].end());
			std::string oname = "floorplan";
			if (result[0][0].size() > 10)
			{
				if (result[0][0][0] == 'P')
					oname += ".png";
				else if (result[0][0][0] == -1)
					oname += ".jpg";
				else if (result[0][0][0] == 'B')
					oname += ".bmp";
				else if (result[0][0][0] == 'G')
					oname += ".gif";
				else if ((result[0][0][0] == '<') && (result[0][0][1] == 's') && (result[0][0][2] == 'v') && (result[0][0][3] == 'g'))
					oname += ".svg";
				else if (result[0][0].find("<svg") != std::string::npos) // some SVG's start with <xml
					oname += ".svg";
			}
			reply::add_header_attachment(&rep, oname);
		}

		void CWebServer::GetDatabaseBackup(WebEmSession& session, const request& req, reply& rep)
		{
			if (session.rights != 2)
			{
				session.reply_status = reply::forbidden;
				return; // Only admin user allowed
			}
			time_t now = mytime(nullptr);
			Json::Value backupInfo;

			backupInfo["type"] = "Web";
#ifdef WIN32
			backupInfo["location"] = szUserDataFolder + "backup.db";
#else
			backupInfo["location"] = "/tmp/backup.db";
#endif
			if (m_sql.BackupDatabase(backupInfo["location"].asString()))
			{
				std::string szAttachmentName = "domoticz.db";
				std::string szVar;
				if (m_sql.GetPreferencesVar("Title", szVar))
				{
					stdreplace(szVar, " ", "_");
					stdreplace(szVar, "/", "_");
					stdreplace(szVar, "\\", "_");
					if (!szVar.empty())
					{
						szAttachmentName = szVar + ".db";
					}
				}
				reply::set_download_file(&rep, backupInfo["location"].asString(), szAttachmentName);
				backupInfo["duration"] = difftime(mytime(nullptr), now);
				m_mainworker.m_notificationsystem.Notify(Notification::DZ_BACKUP_DONE, Notification::STATUS_INFO, JSonToRawString(backupInfo));
			}
		}

		void CWebServer::RestoreDatabase(WebEmSession& session, const request& req, std::string& redirect_uri)
		{
			redirect_uri = "/index.html";
			if (session.rights != 2)
			{
				session.reply_status = reply::forbidden;
				return; // Only admin user allowed
			}

			std::string dbasefile = request::findValue(&req, "dbasefile");
			if (dbasefile.empty())
			{
				return;
			}

			m_mainworker.StopDomoticzHardware();

			m_sql.RestoreDatabase(dbasefile);
			m_mainworker.AddAllDomoticzHardware();
		}

		/**
		 * Retrieve user session from store, without remote host.
		 */
		WebEmStoredSession CWebServer::GetSession(const std::string& sessionId)
		{
			_log.Debug(DEBUG_AUTH, "SessionStore : get...(%s)", sessionId.c_str());
			WebEmStoredSession session;

			if (sessionId.empty())
			{
				_log.Log(LOG_ERROR, "SessionStore : cannot get session without id.");
			}
			else
			{
				std::vector<std::vector<std::string>> result;
				result = m_sql.safe_query("SELECT SessionID, Username, AuthToken, ExpirationDate FROM UserSessions WHERE SessionID = '%q'", sessionId.c_str());
				if (!result.empty())
				{
					session.id = result[0][0];
					session.username = base64_decode(result[0][1]);
					session.auth_token = result[0][2];

					std::string sExpirationDate = result[0][3];
					// time_t now = mytime(NULL);
					struct tm tExpirationDate;
					ParseSQLdatetime(session.expires, tExpirationDate, sExpirationDate);
					// RemoteHost is not used to restore the session
					// LastUpdate is not used to restore the session
				}
				else
				{
					_log.Debug(DEBUG_AUTH, "SessionStore : session not Found! (%s)", sessionId.c_str());
				}
			}

			return session;
		}

		/**
		 * Save user session.
		 */
		void CWebServer::StoreSession(const WebEmStoredSession& session)
		{
			_log.Debug(DEBUG_AUTH, "SessionStore : store...(%s)", session.id.c_str());
			if (session.id.empty())
			{
				_log.Log(LOG_ERROR, "SessionStore : cannot store session without id.");
				return;
			}

			char szExpires[30];
			struct tm ltime;
			localtime_r(&session.expires, &ltime);
			strftime(szExpires, sizeof(szExpires), "%Y-%m-%d %H:%M:%S", &ltime);

			std::string remote_host = (session.remote_host.size() <= 50) ? // IPv4 : 15, IPv6 : (39|45)
				session.remote_host
				: session.remote_host.substr(0, 50);

			WebEmStoredSession storedSession = GetSession(session.id);
			if (storedSession.id.empty())
			{
				m_sql.safe_query("INSERT INTO UserSessions (SessionID, Username, AuthToken, ExpirationDate, RemoteHost) VALUES ('%q', '%q', '%q', '%q', '%q')", session.id.c_str(),
					base64_encode(session.username).c_str(), session.auth_token.c_str(), szExpires, remote_host.c_str());
			}
			else
			{
				m_sql.safe_query("UPDATE UserSessions set AuthToken = '%q', ExpirationDate = '%q', RemoteHost = '%q', LastUpdate = datetime('now', 'localtime') WHERE SessionID = '%q'",
					session.auth_token.c_str(), szExpires, remote_host.c_str(), session.id.c_str());
			}
		}

		/**
		 * Remove user session and expired sessions.
		 */
		void CWebServer::RemoveSession(const std::string& sessionId)
		{
			_log.Debug(DEBUG_AUTH, "SessionStore : remove... (%s)", sessionId.c_str());
			if (sessionId.empty())
			{
				return;
			}
			m_sql.safe_query("DELETE FROM UserSessions WHERE SessionID = '%q'", sessionId.c_str());
		}

		/**
		 * Remove all expired user sessions.
		 */
		void CWebServer::CleanSessions()
		{
			_log.Debug(DEBUG_AUTH, "SessionStore : clean...");
			m_sql.safe_query("DELETE FROM UserSessions WHERE ExpirationDate < datetime('now', 'localtime')");
		}

		/**
		 * Delete all user's session, except the session used to modify the username or password.
		 * username must have been hashed
		 */
		void CWebServer::RemoveUsersSessions(const std::string& username, const WebEmSession& exceptSession)
		{
			_log.Debug(DEBUG_AUTH, "SessionStore : remove all sessions for User... (%s)", exceptSession.id.c_str());
			m_sql.safe_query("DELETE FROM UserSessions WHERE (Username=='%q') and (SessionID!='%q')", username.c_str(), exceptSession.id.c_str());
		}

	} // namespace server
} // namespace http
