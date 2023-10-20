/*
 * WebServerCmds.cpp
 *
 *  Created on: 12 August 2023
 *
 * This file is NOT a separate class but is part of 'main/WebServer.cpp'
 * It contains the code from non-hardware specific 'CommandCodes' functions that are part of the WebServer class,
 * but for sourcecode management reasons separated out into its own file.
 * The definitions of the methods here are still in 'main/Webserver.h'
*/

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

#include "../hardware/RFXBase.h"
#include "../hardware/MySensorsBase.h"
#include "../hardware/OTGWBase.h"
#include "../hardware/EnphaseAPI.h"
#include "../hardware/AlfenEve.h"
#include "../hardware/RFLinkBase.h"

#define round(a) (int)(a + .5)

extern std::string szStartupFolder;
extern std::string szUserDataFolder;
extern std::string szWWWFolder;

extern std::string szAppVersion;
extern int iAppRevision;
extern std::string szAppHash;
extern std::string szAppDate;
extern std::string szPyVersion;

extern bool g_bUseUpdater;

extern time_t m_StartTime;

extern http::server::CWebServerHelper m_webservers;

namespace http
{
	namespace server
	{
		extern std::map<std::string, http::server::connection::_tRemoteClients> m_remote_web_clients;

		struct _tGuiLanguage
		{
			const char* szShort;
			const char* szLong;
		};

		constexpr std::array<std::pair<const char*, const char*>, 36> guiLanguage{ {
			{ "en", "English" }, { "sq", "Albanian" }, { "ar", "Arabic" }, { "bs", "Bosnian" }, { "bg", "Bulgarian" }, { "ca", "Catalan" },
			{ "zh", "Chinese" }, { "cs", "Czech" }, { "da", "Danish" }, { "nl", "Dutch" }, { "et", "Estonian" }, { "de", "German" },
			{ "el", "Greek" }, { "fr", "French" }, { "fi", "Finnish" }, { "he", "Hebrew" }, { "hu", "Hungarian" }, { "is", "Icelandic" },
			{ "it", "Italian" }, { "lt", "Lithuanian" }, { "lv", "Latvian" }, { "mk", "Macedonian" }, { "no", "Norwegian" }, { "fa", "Persian" },
			{ "pl", "Polish" }, { "pt", "Portuguese" }, { "ro", "Romanian" }, { "ru", "Russian" }, { "sr", "Serbian" }, { "sk", "Slovak" },
			{ "sl", "Slovenian" }, { "es", "Spanish" }, { "sv", "Swedish" }, { "zh_TW", "Taiwanese" }, { "tr", "Turkish" }, { "uk", "Ukrainian" },
			} };

		void CWebServer::Cmd_GetTimerTypes(WebEmSession& session, const request& req, Json::Value& root)
		{
			root["title"] = "GetTimerTypes";
			for (int ii = 0; ii < TTYPE_END; ii++)
			{
				std::string sTimerTypeDesc = Timer_Type_Desc(_eTimerType(ii));
				root["result"][ii] = sTimerTypeDesc;
			}
			root["status"] = "OK";
		}

		void CWebServer::Cmd_GetLanguages(WebEmSession& session, const request& req, Json::Value& root)
		{
			root["title"] = "GetLanguages";
			std::string sValue;
			if (m_sql.GetPreferencesVar("Language", sValue))
			{
				root["language"] = sValue;
			}
			for (auto& lang : guiLanguage)
			{
				root["result"][lang.second] = lang.first;
			}
			root["status"] = "OK";
		}

		void CWebServer::Cmd_GetSwitchTypes(WebEmSession& session, const request& req, Json::Value& root)
		{
			root["title"] = "GetSwitchTypes";

			std::map<std::string, int> _switchtypes;

			for (int ii = 0; ii < STYPE_END; ii++)
			{
				std::string sTypeName = Switch_Type_Desc((_eSwitchType)ii);
				if (sTypeName != "Unknown")
				{
					_switchtypes[sTypeName] = ii;
				}
			}
			// return a sorted list
			for (const auto& type : _switchtypes)
			{
				root["result"][type.second] = type.first;
			}
			root["status"] = "OK";
		}

		void CWebServer::Cmd_GetMeterTypes(WebEmSession& session, const request& req, Json::Value& root)
		{
			root["title"] = "GetMeterTypes";

			for (int ii = 0; ii < MTYPE_END; ii++)
			{
				std::string sTypeName = Meter_Type_Desc((_eMeterType)ii);
				root["result"][ii] = sTypeName;
			}
			root["status"] = "OK";
		}

		void CWebServer::Cmd_GetThemes(WebEmSession& session, const request& req, Json::Value& root)
		{
			root["status"] = "OK";
			root["title"] = "GetThemes";
			m_mainworker.GetAvailableWebThemes();
			int ii = 0;
			for (const auto& theme : m_mainworker.m_webthemes)
			{
				root["result"][ii]["theme"] = theme;
				ii++;
			}
		}

		void CWebServer::Cmd_GetTitle(WebEmSession& session, const request& req, Json::Value& root)
		{
			std::string sValue;
			root["status"] = "OK";
			root["title"] = "GetTitle";
			if (m_sql.GetPreferencesVar("Title", sValue))
				root["Title"] = sValue;
			else
				root["Title"] = "Domoticz";
		}

		void CWebServer::Cmd_LoginCheck(WebEmSession& session, const request& req, Json::Value& root)
		{
			std::string tmpusrname = request::findValue(&req, "username");
			std::string tmpusrpass = request::findValue(&req, "password");
			if ((tmpusrname.empty()) || (tmpusrpass.empty()))
				return;

			std::string rememberme = request::findValue(&req, "rememberme");

			std::string usrname;
			std::string usrpass;
			if (request_handler::url_decode(tmpusrname, usrname))
			{
				if (request_handler::url_decode(tmpusrpass, usrpass))
				{
					usrname = base64_decode(usrname);
					int iUser = FindUser(usrname.c_str());
					if (iUser == -1)
					{
						// log brute force attack
						_log.Log(LOG_ERROR, "Failed login attempt from %s for user '%s' !", session.remote_host.c_str(), usrname.c_str());
						return;
					}
					if (m_users[iUser].Password != usrpass)
					{
						// log brute force attack
						_log.Log(LOG_ERROR, "Failed login attempt from %s for '%s' !", session.remote_host.c_str(), m_users[iUser].Username.c_str());
						return;
					}
					if (m_users[iUser].userrights == URIGHTS_CLIENTID) {
						// Not a right for users to login with
						_log.Log(LOG_ERROR, "Failed login attempt from %s for '%s' !", session.remote_host.c_str(), m_users[iUser].Username.c_str());
						return;
					}
					if (!m_users[iUser].Mfatoken.empty())
					{
						// 2FA enabled for this user
						std::string tmp2fa = request::findValue(&req, "2fatotp");
						std::string sTotpKey = "";
						if(!base32_decode(m_users[iUser].Mfatoken, sTotpKey))
						{
							// Unable to decode the 2FA token
							_log.Log(LOG_ERROR, "Failed login attempt from %s for '%s' !", session.remote_host.c_str(), m_users[iUser].Username.c_str());
							_log.Debug(DEBUG_AUTH, "Failed to base32_decode the Users 2FA token: %s", m_users[iUser].Mfatoken.c_str());
							return;
						}
						if (tmp2fa.empty())
						{
							// No 2FA token given (yet), request one
							root["status"] = "OK";
							root["title"] = "logincheck";
							root["require2fa"] = "true";
							return;
						}
						if (!VerifySHA1TOTP(tmp2fa, sTotpKey))
						{
							// Not a match for the given 2FA token
							_log.Log(LOG_ERROR, "Failed login attempt from %s for '%s' !", session.remote_host.c_str(), m_users[iUser].Username.c_str());
							_log.Debug(DEBUG_AUTH, "Failed login attempt with 2FA token: %s", tmp2fa.c_str());
							return;
						}
					}
					_log.Log(LOG_STATUS, "Login successful from %s for user '%s'", session.remote_host.c_str(), m_users[iUser].Username.c_str());
					root["status"] = "OK";
					root["version"] = szAppVersion;
					root["title"] = "logincheck";
					session.isnew = true;
					session.username = m_users[iUser].Username;
					session.rights = m_users[iUser].userrights;
					session.rememberme = (rememberme == "true");
					root["user"] = session.username;
					root["rights"] = session.rights;
				}
			}
		}

		void CWebServer::Cmd_GetHardwareTypes(WebEmSession& session, const request& req, Json::Value& root)
		{
			if (session.rights != 2)
			{
				session.reply_status = reply::forbidden;
				return; // Only admin user allowed
			}

			root["status"] = "OK";
			root["title"] = "GetHardwareTypes";
			std::map<std::string, int> _htypes;
			for (int ii = 0; ii < HTYPE_END; ii++)
			{
				bool bDoAdd = true;
#ifndef _DEBUG
#ifdef WIN32
				if ((ii == HTYPE_RaspberryBMP085) || (ii == HTYPE_RaspberryHTU21D) || (ii == HTYPE_RaspberryTSL2561) || (ii == HTYPE_RaspberryPCF8574) ||
					(ii == HTYPE_RaspberryBME280) || (ii == HTYPE_RaspberryMCP23017))
				{
					bDoAdd = false;
				}
				else
				{
#ifndef WITH_LIBUSB
					if ((ii == HTYPE_VOLCRAFTCO20) || (ii == HTYPE_TE923))
					{
						bDoAdd = false;
					}
#endif
				}
#endif
#endif
#ifndef WITH_GPIO
				if (ii == HTYPE_RaspberryGPIO)
				{
					bDoAdd = false;
				}

				if (ii == HTYPE_SysfsGpio)
				{
					bDoAdd = false;
				}
#endif
				if (ii == HTYPE_PythonPlugin)
					bDoAdd = false;

				if (bDoAdd)
				{
					std::string description = Hardware_Type_Desc(ii);
					if (!description.empty())
						_htypes[description] = ii;
				}
			}

			// return a sorted hardware list
			int ii = 0;
			for (const auto& type : _htypes)
			{
				root["result"][ii]["idx"] = type.second;
				root["result"][ii]["name"] = type.first;
				ii++;
			}

#ifdef ENABLE_PYTHON
			// Append Plugin list as well
			PluginList(root["result"]);
#endif
		}

		static bool ValidateHardware(const _eHardwareTypes &htype, const std::string &sport, const std::string &address, const int port, const std::string &username, const std::string &password,
									const std::string &smode1, const std::string &smode2, const std::string &smode3, const std::string &smode4, const std::string &smode5, const std::string &smode6,
									const std::string &extra, const std::string idx = "")
		{
			int mode1 = !smode1.empty() ? atoi(smode1.c_str()) : -1;
			int mode2 = !smode2.empty() ? atoi(smode2.c_str()) : -1;
			int mode3 = !smode3.empty() ? atoi(smode3.c_str()) : -1;
			int mode4 = !smode4.empty() ? atoi(smode4.c_str()) : -1;
			int mode5 = !smode5.empty() ? atoi(smode5.c_str()) : -1;
			int mode6 = !smode6.empty() ? atoi(smode6.c_str()) : -1;

			if (htype == HTYPE_DomoticzInternal)
			{
				// DomoticzInternal cannot be added manually
				return false;
			}
			else if (IsSerialDevice(htype))
			{
				// USB/System
				if (sport.empty())
					return false; // need to have a serial port

				if (htype == HTYPE_TeleinfoMeter)
				{
					// Teleinfo always has decimals. Chances to have a P1 and a Teleinfo device on the same
					// Domoticz instance are very low as both are national standards (NL and FR)
					m_sql.UpdatePreferencesVar("SmartMeterType", 0);
				}
			}
			else if (IsNetworkDevice(htype))
			{
				// Lan
				if (address.empty())
					return false;

				if ((htype == HTYPE_Domoticz) || (htype == HTYPE_HARMONY_HUB))
				{
					if (port == 0)
						return false;
				}
				else if ((htype == HTYPE_MySensorsMQTT) || (htype == HTYPE_MQTT) || (htype == HTYPE_MQTTAutoDiscovery))
				{
					if (smode1.empty())
						return false;
				}
				else if ((htype == HTYPE_ECODEVICES) || (htype == HTYPE_TeleinfoMeterTCP))
				{
					// EcoDevices and Teleinfo always have decimals. Chances to have a P1 and a EcoDevice/Teleinfo
					//  device on the same Domoticz instance are very low as both are national standards (NL and FR)
					m_sql.UpdatePreferencesVar("SmartMeterType", 0);
				}
				else if (htype == HTYPE_AlfenEveCharger)
				{
					if ((password.empty()))
						return false;
				}
				else if (htype == HTYPE_Philips_Hue)
				{
					if ((username.empty()) || port == 0)
						return false;
				}
			}
			else if (htype == HTYPE_System)
			{
				// There should be only one
				std::vector<std::vector<std::string>> result;
				result = m_sql.safe_query("SELECT ID FROM Hardware WHERE (Type==%d)", HTYPE_System);
				if (!result.empty() && (idx.empty() || (!idx.empty() && idx != result[0][0])))
					return false;
			}
			else if ((htype == HTYPE_Wunderground) || (htype == HTYPE_DarkSky) || (htype == HTYPE_VisualCrossing) || (htype == HTYPE_AccuWeather) || (htype == HTYPE_OpenWeatherMap) || (htype == HTYPE_ICYTHERMOSTAT) ||
				(htype == HTYPE_TOONTHERMOSTAT) || (htype == HTYPE_AtagOne) || (htype == HTYPE_PVOUTPUT_INPUT) || (htype == HTYPE_NEST) || (htype == HTYPE_ANNATHERMOSTAT) ||
				(htype == HTYPE_THERMOSMART) || (htype == HTYPE_Tado) || (htype == HTYPE_Tesla) || (htype == HTYPE_Mercedes) || (htype == HTYPE_Netatmo))
			{
				if ((username.empty()) || (password.empty()))
					return false;
			}
			else if (htype == HTYPE_SolarEdgeAPI)
			{
				if ((username.empty()))
					return false;
			}
			else if (htype == HTYPE_Nest_OAuthAPI)
			{
				if ((username.empty()) && (extra == "||"))
					return false;
			}
			else if (htype == HTYPE_SBFSpot)
			{
				if (username.empty())
					return false;
			}
			else if (htype == HTYPE_WINDDELEN)
			{
				// sPort here is neither a Network Port or a Serial Port but a variable that should have been passed as mode parameter
				if ((smode1.empty()) || (sport.empty()))
					return false;
			}
			// Below Hardware Types do not have any input checks at the moment
			else if ( (htype == HTYPE_TE923) || (htype == HTYPE_VOLCRAFTCO20) ||  (htype == HTYPE_1WIRE) || (htype == HTYPE_Rtl433) || (htype == HTYPE_Pinger) || (htype == HTYPE_Kodi)
					|| (htype == HTYPE_PanasonicTV) || (htype == HTYPE_LogitechMediaServer) || (htype == HTYPE_RaspberryBMP085) || (htype == HTYPE_RaspberryHTU21D) || (htype == HTYPE_RaspberryTSL2561)
					|| (htype == HTYPE_RaspberryBME280) || (htype == HTYPE_RaspberryMCP23017) || (htype == HTYPE_Dummy) || (htype == HTYPE_Tellstick)
					|| (htype == HTYPE_EVOHOME_SCRIPT) || (htype == HTYPE_EVOHOME_SERIAL) || (htype == HTYPE_EVOHOME_WEB) || (htype == HTYPE_EVOHOME_TCP)
					|| (htype == HTYPE_PiFace) || (htype == HTYPE_HTTPPOLLER) || (htype == HTYPE_BleBox) || (htype == HTYPE_HEOS) || (htype == HTYPE_Yeelight) || (htype == HTYPE_XiaomiGateway)
					|| (htype == HTYPE_Arilux) || (htype == HTYPE_USBtinGateway) || (htype == HTYPE_BuienRadar) || (htype == HTYPE_Honeywell) ||(htype == HTYPE_RaspberryGPIO)
					|| (htype == HTYPE_SysfsGpio) || (htype == HTYPE_OpenWebNetTCP) || (htype == HTYPE_Daikin) || (htype == HTYPE_PythonPlugin) || (htype == HTYPE_RaspberryPCF8574)
					|| (htype == HTYPE_OpenWebNetUSB) || (htype == HTYPE_IntergasInComfortLAN2RF) || (htype == HTYPE_EnphaseAPI) || (htype == HTYPE_EcoCompteur) || (htype == HTYPE_Meteorologisk)
					|| (htype == HTYPE_AirconWithMe) || (htype == HTYPE_EneverPriceFeeds) )
			{
				return true;
			}
			else
			{
				_log.Debug(DEBUG_HARDWARE, "ValidateHardware: No checks for Hardware type (%d)", htype);
				return false;
			}

			return true;
		}

		void CWebServer::Cmd_AddHardware(WebEmSession& session, const request& req, Json::Value& root)
		{
			if (session.rights != URIGHTS_ADMIN)
			{
				session.reply_status = reply::forbidden;
				return; // Only admin user allowed
			}

			std::string name = HTMLSanitizer::Sanitize(CURLEncode::URLDecode(request::findValue(&req, "name")));
			std::string senabled = request::findValue(&req, "enabled");
			std::string shtype = request::findValue(&req, "htype");
			std::string loglevel = request::findValue(&req, "loglevel");
			std::string address = HTMLSanitizer::Sanitize(request::findValue(&req, "address"));
			std::string sport = request::findValue(&req, "port");
			std::string username = HTMLSanitizer::Sanitize(request::findValue(&req, "username"));
			std::string password = request::findValue(&req, "password");
			std::string extra = CURLEncode::URLDecode(request::findValue(&req, "extra"));
			std::string sdatatimeout = request::findValue(&req, "datatimeout");
			if ((name.empty()) || (senabled.empty()) || (shtype.empty()))
				return;
			_eHardwareTypes htype = (_eHardwareTypes)atoi(shtype.c_str());

			stdstring_trim(username);
			stdstring_trim(password);
			int iDataTimeout = atoi(sdatatimeout.c_str());
			int mode1 = 0;
			int mode2 = 0;
			int mode3 = 0;
			int mode4 = 0;
			int mode5 = 0;
			int mode6 = 0;
			int port = atoi(sport.c_str());
			uint32_t iLogLevelEnabled = (uint32_t)atoi(loglevel.c_str());
			std::string mode1Str = request::findValue(&req, "Mode1");
			if (!mode1Str.empty())
			{
				mode1 = atoi(mode1Str.c_str());
			}
			std::string mode2Str = request::findValue(&req, "Mode2");
			if (!mode2Str.empty())
			{
				mode2 = atoi(mode2Str.c_str());
			}
			std::string mode3Str = request::findValue(&req, "Mode3");
			if (!mode3Str.empty())
			{
				mode3 = atoi(mode3Str.c_str());
			}
			std::string mode4Str = request::findValue(&req, "Mode4");
			if (!mode4Str.empty())
			{
				mode4 = atoi(mode4Str.c_str());
			}
			std::string mode5Str = request::findValue(&req, "Mode5");
			if (!mode5Str.empty())
			{
				mode5 = atoi(mode5Str.c_str());
			}
			std::string mode6Str = request::findValue(&req, "Mode6");
			if (!mode6Str.empty())
			{
				mode6 = atoi(mode6Str.c_str());
			}

			if (!ValidateHardware(htype, sport, address, port, username, password, mode1Str, mode2Str, mode3Str, mode4Str, mode5Str, mode6Str, extra))
				return;

			root["status"] = "OK";
			root["title"] = "AddHardware";

			std::vector<std::vector<std::string>> result;

			if (htype == HTYPE_Domoticz)
			{
				if (password.size() != 32)
				{
					password = GenerateMD5Hash(password);
				}
			}
			else if ((htype == HTYPE_S0SmartMeterUSB) || (htype == HTYPE_S0SmartMeterTCP))
			{
				extra = "0;1000;0;1000;0;1000;0;1000;0;1000";
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
			else if (htype == HTYPE_PanasonicTV)
			{
				mode1 = 30;
				mode2 = 1000;
			}
			else if (htype == HTYPE_LogitechMediaServer)
			{
				mode1 = 30;
				mode2 = 1000;
			}
			else if (htype == HTYPE_HEOS)
			{
				mode1 = 30;
				mode2 = 1000;
			}
			else if (htype == HTYPE_Tellstick)
			{
				mode1 = 4;
				mode2 = 500;
			}

			if (htype == HTYPE_HTTPPOLLER)
			{
				m_sql.safe_query("INSERT INTO Hardware (Name, Enabled, Type, LogLevel, Address, Port, SerialPort, Username, Password, Extra, Mode1, Mode2, Mode3, Mode4, Mode5, Mode6, "
					"DataTimeout) VALUES ('%q',%d, %d, %d,'%q',%d,'%q','%q','%q','%q','%q','%q', '%q', '%q', '%q', '%q', %d)",
					name.c_str(), (senabled == "true") ? 1 : 0, htype, iLogLevelEnabled, address.c_str(), port, sport.c_str(), username.c_str(), password.c_str(),
					extra.c_str(), mode1Str.c_str(), mode2Str.c_str(), mode3Str.c_str(), mode4Str.c_str(), mode5Str.c_str(), mode6Str.c_str(), iDataTimeout);
			}
			else if (htype == HTYPE_PythonPlugin)
			{
				sport = request::findValue(&req, "serialport");
				m_sql.safe_query("INSERT INTO Hardware (Name, Enabled, Type, LogLevel, Address, Port, SerialPort, Username, Password, Extra, Mode1, Mode2, Mode3, Mode4, Mode5, Mode6, "
					"DataTimeout) VALUES ('%q',%d, %d, %d,'%q',%d,'%q','%q','%q','%q','%q','%q', '%q', '%q', '%q', '%q', %d)",
					name.c_str(), (senabled == "true") ? 1 : 0, htype, iLogLevelEnabled, address.c_str(), port, sport.c_str(), username.c_str(), password.c_str(),
					extra.c_str(), mode1Str.c_str(), mode2Str.c_str(), mode3Str.c_str(), mode4Str.c_str(), mode5Str.c_str(), mode6Str.c_str(), iDataTimeout);
			}
			else if ((htype == HTYPE_RFXtrx433) || (htype == HTYPE_RFXtrx868))
			{
				// No Extra field here, handled in CWebServer::SetRFXCOMMode
				m_sql.safe_query("INSERT INTO Hardware (Name, Enabled, Type, LogLevel, Address, Port, SerialPort, Username, Password, Mode1, Mode2, Mode3, Mode4, Mode5, Mode6, "
					"DataTimeout) VALUES ('%q',%d, %d, %d,'%q',%d,'%q','%q','%q',%d,%d,%d,%d,%d,%d,%d)",
					name.c_str(), (senabled == "true") ? 1 : 0, htype, iLogLevelEnabled, address.c_str(), port, sport.c_str(), username.c_str(), password.c_str(), mode1,
					mode2, mode3, mode4, mode5, mode6, iDataTimeout);
				extra = "0";
			}
			else
			{
				m_sql.safe_query("INSERT INTO Hardware (Name, Enabled, Type, LogLevel, Address, Port, SerialPort, Username, Password, Extra, Mode1, Mode2, Mode3, Mode4, Mode5, Mode6, "
					"DataTimeout) VALUES ('%q',%d, %d, %d,'%q',%d,'%q','%q','%q','%q',%d,%d,%d,%d,%d,%d,%d)",
					name.c_str(), (senabled == "true") ? 1 : 0, htype, iLogLevelEnabled, address.c_str(), port, sport.c_str(), username.c_str(), password.c_str(),
					extra.c_str(), mode1, mode2, mode3, mode4, mode5, mode6, iDataTimeout);
			}

			// add the device for real in our system
			result = m_sql.safe_query("SELECT MAX(ID) FROM Hardware");
			if (!result.empty())
			{
				std::vector<std::string> sd = result[0];
				int ID = atoi(sd[0].c_str());

				root["idx"] = sd[0].c_str(); // OTO output the created ID for easier management on the caller side (if automated)

				m_mainworker.AddHardwareFromParams(ID, name, (senabled == "true") ? true : false, htype, iLogLevelEnabled, address, port, sport, username, password, extra, mode1,
					mode2, mode3, mode4, mode5, mode6, iDataTimeout, true);
			}
		}

		void CWebServer::Cmd_UpdateHardware(WebEmSession& session, const request& req, Json::Value& root)
		{
			if (session.rights != URIGHTS_ADMIN)
			{
				session.reply_status = reply::forbidden;
				return; // Only admin user allowed
			}

			std::string idx = request::findValue(&req, "idx");
			if (idx.empty())
				return;
			std::string name = HTMLSanitizer::Sanitize(CURLEncode::URLDecode(request::findValue(&req, "name")));
			std::string senabled = request::findValue(&req, "enabled");
			std::string shtype = request::findValue(&req, "htype");
			std::string loglevel = request::findValue(&req, "loglevel");
			std::string address = HTMLSanitizer::Sanitize(request::findValue(&req, "address"));
			std::string sport = request::findValue(&req, "port");
			std::string username = HTMLSanitizer::Sanitize(request::findValue(&req, "username"));
			std::string password = request::findValue(&req, "password");
			std::string extra = HTMLSanitizer::Sanitize(CURLEncode::URLDecode(request::findValue(&req, "extra")));
			std::string sdatatimeout = request::findValue(&req, "datatimeout");

			if ((name.empty()) || (senabled.empty()) || (shtype.empty()))
				return;

			stdstring_trim(username);
			stdstring_trim(password);

			std::string mode1Str = request::findValue(&req, "Mode1");
			std::string mode2Str = request::findValue(&req, "Mode2");
			std::string mode3Str = request::findValue(&req, "Mode3");
			std::string mode4Str = request::findValue(&req, "Mode4");
			std::string mode5Str = request::findValue(&req, "Mode5");
			std::string mode6Str = request::findValue(&req, "Mode6");

			int mode1 = atoi(mode1Str.c_str());
			int mode2 = atoi(mode2Str.c_str());
			int mode3 = atoi(mode3Str.c_str());
			int mode4 = atoi(mode4Str.c_str());
			int mode5 = atoi(mode5Str.c_str());
			int mode6 = atoi(mode6Str.c_str());

			bool bEnabled = (senabled == "true") ? true : false;

			_eHardwareTypes htype = (_eHardwareTypes)atoi(shtype.c_str());
			int iDataTimeout = atoi(sdatatimeout.c_str());

			int port = atoi(sport.c_str());
			uint32_t iLogLevelEnabled = (uint32_t)atoi(loglevel.c_str());

			bool bIsSerial = false;

			if (!ValidateHardware(htype, sport, address, port, username, password, mode1Str, mode2Str, mode3Str, mode4Str, mode5Str, mode6Str, extra, idx))
				return;

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
				// just disable the device
				m_sql.safe_query("UPDATE Hardware SET Enabled=%d WHERE (ID == '%q')", (bEnabled == true) ? 1 : 0, idx.c_str());
			}
			else
			{
				if (htype == HTYPE_HTTPPOLLER)
				{
					m_sql.safe_query("UPDATE Hardware SET Name='%q', Enabled=%d, Type=%d, LogLevel=%d, Address='%q', Port=%d, SerialPort='%q', Username='%q', Password='%q', "
						"Extra='%q', DataTimeout=%d WHERE (ID == '%q')",
						name.c_str(), (senabled == "true") ? 1 : 0, htype, iLogLevelEnabled, address.c_str(), port, sport.c_str(), username.c_str(), password.c_str(),
						extra.c_str(), iDataTimeout, idx.c_str());
				}
				else if (htype == HTYPE_PythonPlugin)
				{
					sport = request::findValue(&req, "serialport");
					m_sql.safe_query("UPDATE Hardware SET Name='%q', Enabled=%d, Type=%d, LogLevel=%d, Address='%q', Port=%d, SerialPort='%q', Username='%q', Password='%q', "
						"Extra='%q', Mode1='%q', Mode2='%q', Mode3='%q', Mode4='%q', Mode5='%q', Mode6='%q', DataTimeout=%d WHERE (ID == '%q')",
						name.c_str(), (senabled == "true") ? 1 : 0, htype, iLogLevelEnabled, address.c_str(), port, sport.c_str(), username.c_str(), password.c_str(),
						extra.c_str(), mode1Str.c_str(), mode2Str.c_str(), mode3Str.c_str(), mode4Str.c_str(), mode5Str.c_str(), mode6Str.c_str(), iDataTimeout,
						idx.c_str());
				}
				else if ((htype == HTYPE_RFXtrx433) || (htype == HTYPE_RFXtrx868))
				{
					// No Extra field here, handled in CWebServer::SetRFXCOMMode
					m_sql.safe_query("UPDATE Hardware SET Name='%q', Enabled=%d, Type=%d, LogLevel=%d, Address='%q', Port=%d, SerialPort='%q', Username='%q', Password='%q', "
						"Mode1=%d, Mode2=%d, Mode3=%d, Mode4=%d, Mode5=%d, Mode6=%d, DataTimeout=%d WHERE (ID == '%q')",
						name.c_str(), (bEnabled == true) ? 1 : 0, htype, iLogLevelEnabled, address.c_str(), port, sport.c_str(), username.c_str(), password.c_str(),
						mode1, mode2, mode3, mode4, mode5, mode6, iDataTimeout, idx.c_str());
					std::vector<std::vector<std::string>> result;
					result = m_sql.safe_query("SELECT Extra FROM Hardware WHERE ID=%q", idx.c_str());
					if (!result.empty())
						extra = result[0][0];
				}
				else
				{
					m_sql.safe_query("UPDATE Hardware SET Name='%q', Enabled=%d, Type=%d, LogLevel=%d, Address='%q', Port=%d, SerialPort='%q', Username='%q', Password='%q', "
						"Extra='%q', Mode1=%d, Mode2=%d, Mode3=%d, Mode4=%d, Mode5=%d, Mode6=%d, DataTimeout=%d WHERE (ID == '%q')",
						name.c_str(), (bEnabled == true) ? 1 : 0, htype, iLogLevelEnabled, address.c_str(), port, sport.c_str(), username.c_str(), password.c_str(),
						extra.c_str(), mode1, mode2, mode3, mode4, mode5, mode6, iDataTimeout, idx.c_str());
				}
			}

			// re-add the device in our system
			int ID = atoi(idx.c_str());
			m_mainworker.AddHardwareFromParams(ID, name, bEnabled, htype, iLogLevelEnabled, address, port, sport, username, password, extra, mode1, mode2, mode3, mode4, mode5, mode6,
				iDataTimeout, true);
		}

		void CWebServer::Cmd_GetDeviceValueOptions(WebEmSession& session, const request& req, Json::Value& root)
		{
			if (session.rights != 2)
			{
				session.reply_status = reply::forbidden;
				return; // Only admin user allowed
			}
			std::string idx = request::findValue(&req, "idx");
			if (idx.empty())
				return;
			std::vector<std::vector<std::string>> devresult;
			devresult = m_sql.safe_query("SELECT Type, SubType FROM DeviceStatus WHERE (ID=='%q')", idx.c_str());
			if (!devresult.empty())
			{
				int devType = std::stoi(devresult[0][0]);
				int devSubType = std::stoi(devresult[0][1]);
				std::vector<std::string> result;
				result = CBasePush::DropdownOptions(devType, devSubType);
				int ii = 0;
				for (const auto& ddOption : result)
				{
					root["result"][ii]["Value"] = ii + 1;
					root["result"][ii]["Wording"] = ddOption.c_str();
					ii++;
				}
			}
			root["status"] = "OK";
			root["title"] = "GetDeviceValueOptions";
		}

		void CWebServer::Cmd_GetDeviceValueOptionWording(WebEmSession& session, const request& req, Json::Value& root)
		{
			if (session.rights != 2)
			{
				session.reply_status = reply::forbidden;
				return; // Only admin user allowed
			}
			std::string idx = request::findValue(&req, "idx");
			std::string pos = request::findValue(&req, "pos");
			if ((idx.empty()) || (pos.empty()))
				return;
			std::string wording;
			std::vector<std::vector<std::string>> devresult;
			devresult = m_sql.safe_query("SELECT Type, SubType FROM DeviceStatus WHERE (ID=='%q')", idx.c_str());
			if (!devresult.empty())
			{
				int devType = std::stoi(devresult[0][0]);
				int devSubType = std::stoi(devresult[0][1]);
				wording = CBasePush::DropdownOptionsValue(devType, devSubType, std::stoi(pos));
			}
			root["wording"] = wording;
			root["status"] = "OK";
			root["title"] = "GetDeviceValueOptions";
		}

		void CWebServer::Cmd_AddUserVariable(WebEmSession& session, const request& req, Json::Value& root)
		{
			if (session.rights != 2)
			{
				session.reply_status = reply::forbidden;
				_log.Log(LOG_ERROR, "User: %s tried to add a uservariable!", session.username.c_str());
				return; // Only admin user allowed
			}
			std::string variablename = HTMLSanitizer::Sanitize(request::findValue(&req, "vname"));
			std::string variablevalue = request::findValue(&req, "vvalue");
			std::string variabletype = request::findValue(&req, "vtype");

			root["title"] = "AddUserVariable";
			root["status"] = "ERR";

			if (!std::isdigit(variabletype[0]))
			{
				stdlower(variabletype);
				if (variabletype == "integer")
					variabletype = "0";
				else if (variabletype == "float")
					variabletype = "1";
				else if (variabletype == "string")
					variabletype = "2";
				else if (variabletype == "date")
					variabletype = "3";
				else if (variabletype == "time")
					variabletype = "4";
				else
				{
					root["message"] = "Invalid variabletype " + variabletype;
					return;
				}
			}

			if ((variablename.empty()) || (variabletype.empty()) ||
				((variabletype != "0") && (variabletype != "1") && (variabletype != "2") && (variabletype != "3") && (variabletype != "4")) ||
				((variablevalue.empty()) && (variabletype != "2")))
			{
				root["message"] = "Invalid variabletype " + variabletype;
				return;
			}

			std::string errorMessage;
			if (!m_sql.AddUserVariable(variablename, (const _eUsrVariableType)atoi(variabletype.c_str()), variablevalue, errorMessage))
			{
				root["message"] = errorMessage;
			}
			else
			{
				root["status"] = "OK";
			}
		}

		void CWebServer::Cmd_DeleteUserVariable(WebEmSession& session, const request& req, Json::Value& root)
		{
			if (session.rights != 2)
			{
				_log.Log(LOG_ERROR, "User: %s tried to delete a uservariable!", session.username.c_str());
				session.reply_status = reply::forbidden;
				return; // Only admin user allowed
			}
			std::string idx = request::findValue(&req, "idx");
			if (idx.empty())
				return;

			m_sql.DeleteUserVariable(idx);
			root["status"] = "OK";
			root["title"] = "DeleteUserVariable";
		}

		void CWebServer::Cmd_UpdateUserVariable(WebEmSession& session, const request& req, Json::Value& root)
		{
			if (session.rights != 2)
			{
				_log.Log(LOG_ERROR, "User: %s tried to update a uservariable!", session.username.c_str());
				session.reply_status = reply::forbidden;
				return; // Only admin user allowed
			}

			std::string idx = request::findValue(&req, "idx");
			std::string variablename = HTMLSanitizer::Sanitize(request::findValue(&req, "vname"));
			std::string variablevalue = request::findValue(&req, "vvalue");
			std::string variabletype = request::findValue(&req, "vtype");

			root["title"] = "UpdateUserVariable";
			root["status"] = "ERR";

			if (!std::isdigit(variabletype[0]))
			{
				stdlower(variabletype);
				if (variabletype == "integer")
					variabletype = "0";
				else if (variabletype == "float")
					variabletype = "1";
				else if (variabletype == "string")
					variabletype = "2";
				else if (variabletype == "date")
					variabletype = "3";
				else if (variabletype == "time")
					variabletype = "4";
				else
				{
					root["message"] = "Invalid variabletype " + variabletype;
					return;
				}
			}

			if ((variablename.empty()) || (variabletype.empty()) ||
				((variabletype != "0") && (variabletype != "1") && (variabletype != "2") && (variabletype != "3") && (variabletype != "4")) ||
				((variablevalue.empty()) && (variabletype != "2")))
			{
				root["message"] = "Invalid variabletype " + variabletype;
				return;
			}

			std::vector<std::vector<std::string>> result;
			if (idx.empty())
			{
				result = m_sql.safe_query("SELECT ID FROM UserVariables WHERE Name='%q'", variablename.c_str());
				if (result.empty())
				{
					root["message"] = "Uservariable " + variablename + " does not exist";
					return;
				}
				idx = result[0][0];
			}

			result = m_sql.safe_query("SELECT Name, ValueType FROM UserVariables WHERE ID='%q'", idx.c_str());
			if (result.empty())
			{
				root["message"] = "Uservariable " + variablename + " does not exist";
				return;
			}

			bool bTypeNameChanged = false;
			if (variablename != result[0][0])
				bTypeNameChanged = true; // new name
			else if (variabletype != result[0][1])
				bTypeNameChanged = true; // new type

			std::string errorMessage;
			if (!m_sql.UpdateUserVariable(idx, variablename, (const _eUsrVariableType)atoi(variabletype.c_str()), variablevalue, !bTypeNameChanged, errorMessage))
			{
				root["message"] = errorMessage;
			}
			else
			{
				root["status"] = "OK";
				if (bTypeNameChanged)
				{
					if (m_sql.m_bEnableEventSystem)
						m_mainworker.m_eventsystem.GetCurrentUserVariables();
				}
			}
		}

		void CWebServer::Cmd_GetUserVariables(WebEmSession& session, const request& req, Json::Value& root)
		{
			std::vector<std::vector<std::string>> result;
			result = m_sql.safe_query("SELECT ID, Name, ValueType, Value, LastUpdate FROM UserVariables");
			int ii = 0;
			for (const auto& sd : result)
			{
				root["result"][ii]["idx"] = sd[0];
				root["result"][ii]["Name"] = sd[1];
				root["result"][ii]["Type"] = sd[2];
				root["result"][ii]["Value"] = sd[3];
				root["result"][ii]["LastUpdate"] = sd[4];
				ii++;
			}
			root["status"] = "OK";
			root["title"] = "GetUserVariables";
		}

		void CWebServer::Cmd_GetUserVariable(WebEmSession& session, const request& req, Json::Value& root)
		{
			std::string idx = request::findValue(&req, "idx");
			if (idx.empty())
				return;

			int iVarID = atoi(idx.c_str());

			std::vector<std::vector<std::string>> result;
			result = m_sql.safe_query("SELECT ID, Name, ValueType, Value, LastUpdate FROM UserVariables WHERE (ID==%d)", iVarID);
			int ii = 0;
			for (const auto& sd : result)
			{
				root["result"][ii]["idx"] = sd[0];
				root["result"][ii]["Name"] = sd[1];
				root["result"][ii]["Type"] = sd[2];
				root["result"][ii]["Value"] = sd[3];
				root["result"][ii]["LastUpdate"] = sd[4];
				ii++;
			}
			root["status"] = "OK";
			root["title"] = "GetUserVariable";
		}

		void CWebServer::Cmd_AllowNewHardware(WebEmSession& session, const request& req, Json::Value& root)
		{
			if (session.rights != 2)
			{
				session.reply_status = reply::forbidden;
				return; // Only admin user allowed
			}
			std::string sTimeout = request::findValue(&req, "timeout");
			if (sTimeout.empty())
				return;
			root["status"] = "OK";
			root["title"] = "AllowNewHardware";

			m_sql.AllowNewHardwareTimer(atoi(sTimeout.c_str()));
		}

		void CWebServer::Cmd_DeleteHardware(WebEmSession& session, const request& req, Json::Value& root)
		{
			if (session.rights != 2)
			{
				session.reply_status = reply::forbidden;
				return; // Only admin user allowed
			}

			std::string idx = request::findValue(&req, "idx");
			if (idx.empty())
				return;
			int hwID = atoi(idx.c_str());

			CDomoticzHardwareBase* pBaseHardware = m_mainworker.GetHardware(hwID);
			if ((pBaseHardware != nullptr) && (pBaseHardware->HwdType == HTYPE_DomoticzInternal))
			{
				// DomoticzInternal cannot be removed
				return;
			}

			root["status"] = "OK";
			root["title"] = "DeleteHardware";

			m_mainworker.RemoveDomoticzHardware(hwID);
			m_sql.DeleteHardware(idx);
		}

		void CWebServer::Cmd_GetLog(WebEmSession& session, const request& req, Json::Value& root)
		{
			root["status"] = "OK";
			root["title"] = "GetLog";

			time_t lastlogtime = 0;
			std::string slastlogtime = request::findValue(&req, "lastlogtime");
			if (!slastlogtime.empty())
			{
				std::stringstream s_str(slastlogtime);
				s_str >> lastlogtime;
			}

			_eLogLevel lLevel = LOG_NORM;
			std::string sloglevel = request::findValue(&req, "loglevel");
			if (!sloglevel.empty())
			{
				lLevel = (_eLogLevel)atoi(sloglevel.c_str());
			}

			std::list<CLogger::_tLogLineStruct> logmessages = _log.GetLog(lLevel);
			int ii = 0;
			for (const auto& msg : logmessages)
			{
				if (msg.logtime > lastlogtime)
				{
					std::stringstream szLogTime;
					szLogTime << msg.logtime;
					root["LastLogTime"] = szLogTime.str();
					root["result"][ii]["level"] = static_cast<int>(msg.level);
					root["result"][ii]["message"] = msg.logmessage;
					ii++;
				}
			}
		}

		void CWebServer::Cmd_ClearLog(WebEmSession& session, const request& req, Json::Value& root)
		{
			root["status"] = "OK";
			root["title"] = "ClearLog";
			_log.ClearLog();
		}

		// Plan Functions
		void CWebServer::Cmd_AddPlan(WebEmSession& session, const request& req, Json::Value& root)
		{
			if (session.rights != 2)
			{
				session.reply_status = reply::forbidden;
				return; // Only admin user allowed
			}

			std::string name = HTMLSanitizer::Sanitize(request::findValue(&req, "name"));
			if (name.empty())
			{
				session.reply_status = reply::bad_request;
			}

			root["status"] = "OK";
			root["title"] = "AddPlan";
			m_sql.safe_query("INSERT INTO Plans (Name) VALUES ('%q')", name.c_str());
			std::vector<std::vector<std::string>> result;
			result = m_sql.safe_query("SELECT MAX(ID) FROM Plans");
			if (!result.empty())
			{
				std::vector<std::string> sd = result[0];
				int ID = atoi(sd[0].c_str());

				root["idx"] = ID; // OTO output the created ID for easier management on the caller side (if automated)
			}
		}

		void CWebServer::Cmd_UpdatePlan(WebEmSession& session, const request& req, Json::Value& root)
		{
			if (session.rights != 2)
			{
				session.reply_status = reply::forbidden;
				return; // Only admin user allowed
			}

			std::string idx = request::findValue(&req, "idx");
			if (idx.empty())
				return;
			std::string name = HTMLSanitizer::Sanitize(request::findValue(&req, "name"));
			if (name.empty())
			{
				session.reply_status = reply::bad_request;
				return;
			}

			root["status"] = "OK";
			root["title"] = "UpdatePlan";

			m_sql.safe_query("UPDATE Plans SET Name='%q' WHERE (ID == '%q')", name.c_str(), idx.c_str());
		}

		void CWebServer::Cmd_DeletePlan(WebEmSession& session, const request& req, Json::Value& root)
		{
			if (session.rights != 2)
			{
				session.reply_status = reply::forbidden;
				return; // Only admin user allowed
			}

			std::string idx = request::findValue(&req, "idx");
			if (idx.empty())
				return;
			root["status"] = "OK";
			root["title"] = "DeletePlan";
			m_sql.safe_query("DELETE FROM DeviceToPlansMap WHERE (PlanID == '%q')", idx.c_str());
			m_sql.safe_query("DELETE FROM Plans WHERE (ID == '%q')", idx.c_str());
		}

		void CWebServer::Cmd_GetUnusedPlanDevices(WebEmSession& session, const request& req, Json::Value& root)
		{
			root["status"] = "OK";
			root["title"] = "GetUnusedPlanDevices";
			std::string sunique = request::findValue(&req, "unique");
			std::string sactplan = request::findValue(&req, "actplan");
			if (
				sunique.empty()
				|| sactplan.empty()
				)
				return;
			const int iActPlan = atoi(sactplan.c_str());
			const bool iUnique = (sunique == "true") ? true : false;
			int ii = 0;

			std::vector<std::vector<std::string>> result;
			std::vector<std::vector<std::string>> result2;
			result = m_sql.safe_query("SELECT T1.[ID], T1.[Name], T1.[Type], T1.[SubType], T2.[Name] AS HardwareName FROM DeviceStatus as T1, Hardware as T2 "
				"WHERE (T1.[Used]==1) AND (T2.[ID]==T1.[HardwareID]) ORDER BY T2.[Name], T1.[Name]");
			if (!result.empty())
			{
				for (const auto& sd : result)
				{
					bool bDoAdd = true;
					if (iUnique)
					{
						result2 = m_sql.safe_query("SELECT ID FROM DeviceToPlansMap WHERE (DeviceRowID=='%q') AND (DevSceneType==0) AND (PlanID==%d)", sd[0].c_str(), iActPlan);
						bDoAdd = result2.empty();
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
			// Add Scenes
			result = m_sql.safe_query("SELECT ID, Name FROM Scenes ORDER BY Name COLLATE NOCASE ASC");
			if (!result.empty())
			{
				for (const auto& sd : result)
				{
					bool bDoAdd = true;
					if (iUnique)
					{
						result2 = m_sql.safe_query("SELECT ID FROM DeviceToPlansMap WHERE (DeviceRowID=='%q') AND (DevSceneType==1) AND (PlanID==%d)", sd[0].c_str(), iActPlan);
						bDoAdd = (result2.empty());
					}
					if (bDoAdd)
					{
						root["result"][ii]["type"] = 1;
						root["result"][ii]["idx"] = sd[0];
						std::string sname = "[Scene] " + sd[1];
						root["result"][ii]["Name"] = sname;
						ii++;
					}
				}
			}
		}

		void CWebServer::Cmd_AddPlanActiveDevice(WebEmSession& session, const request& req, Json::Value& root)
		{
			if (session.rights != 2)
			{
				session.reply_status = reply::forbidden;
				return; // Only admin user allowed
			}

			std::string idx = request::findValue(&req, "idx");
			std::string sactivetype = request::findValue(&req, "activetype");
			std::string activeidx = request::findValue(&req, "activeidx");
			if ((idx.empty()) || (sactivetype.empty()) || (activeidx.empty()))
				return;
			root["status"] = "OK";
			root["title"] = "AddPlanActiveDevice";

			int activetype = atoi(sactivetype.c_str());

			// check if it is not already there
			std::vector<std::vector<std::string>> result;
			result = m_sql.safe_query("SELECT ID FROM DeviceToPlansMap WHERE (DeviceRowID=='%q') AND (DevSceneType==%d) AND (PlanID=='%q')", activeidx.c_str(), activetype, idx.c_str());
			if (result.empty())
			{
				m_sql.safe_query("INSERT INTO DeviceToPlansMap (DevSceneType,DeviceRowID, PlanID) VALUES (%d,'%q','%q')", activetype, activeidx.c_str(), idx.c_str());
			}
		}

		void CWebServer::Cmd_GetPlanDevices(WebEmSession& session, const request& req, Json::Value& root)
		{
			std::string idx = request::findValue(&req, "idx");
			if (idx.empty())
				return;
			root["status"] = "OK";
			root["title"] = "GetPlanDevices";

			std::vector<std::vector<std::string>> result;
			result = m_sql.safe_query("SELECT ID, DevSceneType, DeviceRowID, [Order] FROM DeviceToPlansMap WHERE (PlanID=='%q') ORDER BY [Order]", idx.c_str());
			if (!result.empty())
			{
				int ii = 0;
				for (const auto& sd : result)
				{
					std::string ID = sd[0];
					int DevSceneType = atoi(sd[1].c_str());
					std::string DevSceneRowID = sd[2];

					std::string Name;
					if (DevSceneType == 0)
					{
						std::vector<std::vector<std::string>> result2;
						result2 = m_sql.safe_query("SELECT Name FROM DeviceStatus WHERE (ID=='%q')", DevSceneRowID.c_str());
						if (!result2.empty())
						{
							Name = result2[0][0];
						}
					}
					else
					{
						std::vector<std::vector<std::string>> result2;
						result2 = m_sql.safe_query("SELECT Name FROM Scenes WHERE (ID=='%q')", DevSceneRowID.c_str());
						if (!result2.empty())
						{
							Name = "[Scene] " + result2[0][0];
						}
					}
					if (!Name.empty())
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

		void CWebServer::Cmd_DeletePlanDevice(WebEmSession& session, const request& req, Json::Value& root)
		{
			if (session.rights != 2)
			{
				session.reply_status = reply::forbidden;
				return; // Only admin user allowed
			}
			std::string idx = request::findValue(&req, "idx");
			if (idx.empty())
				return;
			root["status"] = "OK";
			root["title"] = "DeletePlanDevice";
			m_sql.safe_query("DELETE FROM DeviceToPlansMap WHERE (ID == '%q')", idx.c_str());
		}

		void CWebServer::Cmd_SetPlanDeviceCoords(WebEmSession& session, const request& req, Json::Value& root)
		{
			std::string idx = request::findValue(&req, "idx");
			std::string planidx = request::findValue(&req, "planidx");
			std::string xoffset = request::findValue(&req, "xoffset");
			std::string yoffset = request::findValue(&req, "yoffset");
			std::string type = request::findValue(&req, "DevSceneType");
			if ((idx.empty()) || (planidx.empty()) || (xoffset.empty()) || (yoffset.empty()))
				return;
			if (type != "1")
				type = "0"; // 0 = Device, 1 = Scene/Group
			root["status"] = "OK";
			root["title"] = "SetPlanDeviceCoords";
			m_sql.safe_query("UPDATE DeviceToPlansMap SET [XOffset] = '%q', [YOffset] = '%q' WHERE (DeviceRowID='%q') and (PlanID='%q') and (DevSceneType='%q')", xoffset.c_str(),
				yoffset.c_str(), idx.c_str(), planidx.c_str(), type.c_str());
			_log.Log(LOG_STATUS, "(Floorplan) Device '%s' coordinates set to '%s,%s' in plan '%s'.", idx.c_str(), xoffset.c_str(), yoffset.c_str(), planidx.c_str());
		}

		void CWebServer::Cmd_DeleteAllPlanDevices(WebEmSession& session, const request& req, Json::Value& root)
		{
			if (session.rights != 2)
			{
				session.reply_status = reply::forbidden;
				return; // Only admin user allowed
			}
			std::string idx = request::findValue(&req, "idx");
			if (idx.empty())
				return;
			root["status"] = "OK";
			root["title"] = "DeleteAllPlanDevices";
			m_sql.safe_query("DELETE FROM DeviceToPlansMap WHERE (PlanID == '%q')", idx.c_str());
		}

		void CWebServer::Cmd_ChangePlanOrder(WebEmSession& session, const request& req, Json::Value& root)
		{
			std::string idx = request::findValue(&req, "idx");
			if (idx.empty())
				return;
			std::string sway = request::findValue(&req, "way");
			if (sway.empty())
				return;
			bool bGoUp = (sway == "0");

			std::string aOrder, oID, oOrder;

			std::vector<std::vector<std::string>> result;
			result = m_sql.safe_query("SELECT [Order] FROM Plans WHERE (ID=='%q')", idx.c_str());
			if (result.empty())
				return;
			aOrder = result[0][0];

			if (!bGoUp)
			{
				// Get next device order
				result = m_sql.safe_query("SELECT ID, [Order] FROM Plans WHERE ([Order]>'%q') ORDER BY [Order] ASC", aOrder.c_str());
				if (result.empty())
					return;
				oID = result[0][0];
				oOrder = result[0][1];
			}
			else
			{
				// Get previous device order
				result = m_sql.safe_query("SELECT ID, [Order] FROM Plans WHERE ([Order]<'%q') ORDER BY [Order] DESC", aOrder.c_str());
				if (result.empty())
					return;
				oID = result[0][0];
				oOrder = result[0][1];
			}
			// Swap them
			root["status"] = "OK";
			root["title"] = "ChangePlanOrder";

			m_sql.safe_query("UPDATE Plans SET [Order] = '%q' WHERE (ID='%q')", oOrder.c_str(), idx.c_str());
			m_sql.safe_query("UPDATE Plans SET [Order] = '%q' WHERE (ID='%q')", aOrder.c_str(), oID.c_str());
		}

		void CWebServer::Cmd_ChangePlanDeviceOrder(WebEmSession& session, const request& req, Json::Value& root)
		{
			std::string planid = request::findValue(&req, "planid");
			std::string idx = request::findValue(&req, "idx");
			std::string sway = request::findValue(&req, "way");
			if ((planid.empty()) || (idx.empty()) || (sway.empty()))
				return;
			bool bGoUp = (sway == "0");

			std::string aOrder, oID, oOrder;

			std::vector<std::vector<std::string>> result;
			result = m_sql.safe_query("SELECT [Order] FROM DeviceToPlansMap WHERE ((ID=='%q') AND (PlanID=='%q'))", idx.c_str(), planid.c_str());
			if (result.empty())
				return;
			aOrder = result[0][0];

			if (!bGoUp)
			{
				// Get next device order
				result = m_sql.safe_query("SELECT ID, [Order] FROM DeviceToPlansMap WHERE (([Order]>'%q') AND (PlanID=='%q')) ORDER BY [Order] ASC", aOrder.c_str(), planid.c_str());
				if (result.empty())
					return;
				oID = result[0][0];
				oOrder = result[0][1];
			}
			else
			{
				// Get previous device order
				result = m_sql.safe_query("SELECT ID, [Order] FROM DeviceToPlansMap WHERE (([Order]<'%q') AND (PlanID=='%q')) ORDER BY [Order] DESC", aOrder.c_str(), planid.c_str());
				if (result.empty())
					return;
				oID = result[0][0];
				oOrder = result[0][1];
			}
			// Swap them
			root["status"] = "OK";
			root["title"] = "ChangePlanOrder";

			m_sql.safe_query("UPDATE DeviceToPlansMap SET [Order] = '%q' WHERE (ID='%q')", oOrder.c_str(), idx.c_str());
			m_sql.safe_query("UPDATE DeviceToPlansMap SET [Order] = '%q' WHERE (ID='%q')", aOrder.c_str(), oID.c_str());
		}

		void CWebServer::Cmd_GetVersion(WebEmSession& session, const request& req, Json::Value& root)
		{
			root["status"] = "OK";
			root["title"] = "GetVersion";
			if (session.rights != -1 )
			{
				root["version"] = szAppVersion;
				root["hash"] = szAppHash;
				root["build_time"] = szAppDate;
				CdzVents* dzvents = CdzVents::GetInstance();
				root["dzvents_version"] = dzvents->GetVersion();
				root["python_version"] = szPyVersion;
				root["UseUpdate"] = false;
				root["HaveUpdate"] = m_mainworker.IsUpdateAvailable(false);

				if (session.rights == URIGHTS_ADMIN)
				{
					root["UseUpdate"] = g_bUseUpdater;
					root["DomoticzUpdateURL"] = m_mainworker.m_szDomoticzUpdateURL;
					root["SystemName"] = m_mainworker.m_szSystemName;
					root["Revision"] = m_mainworker.m_iRevision;
				}
			}
		}

		void CWebServer::Cmd_GetAuth(WebEmSession& session, const request& req, Json::Value& root)
		{
			root["status"] = "OK";
			root["title"] = "GetAuth";
			if (session.rights != -1)
			{
				root["user"] = session.username;
				root["rights"] = session.rights;
				root["version"] = szAppVersion;
			}
		}

		void CWebServer::Cmd_GetMyProfile(WebEmSession& session, const request& req, Json::Value& root)
		{
			root["status"] = "ERR";
			root["title"] = "GetMyProfile";
			if (session.rights > 0)	// Viewer cannot change his profile
			{
				int iUser = FindUser(session.username.c_str());
				if (iUser != -1)
				{
					root["user"] = session.username;
					root["rights"] = session.rights;
					if (!m_users[iUser].Mfatoken.empty())
						root["mfasecret"] = m_users[iUser].Mfatoken;
					root["status"] = "OK";
				}
			}
		}

		void CWebServer::Cmd_UpdateMyProfile(WebEmSession& session, const request& req, Json::Value& root)
		{
			root["status"] = "ERR";
			root["title"] = "UpdateMyProfile";

			if (req.method == "POST" && session.rights > 0)	// Viewer cannot change his profile
			{
				std::string sUsername = request::findValue(&req, "username");
				int iUser = FindUser(session.username.c_str());
				if (iUser == -1)
				{
					root["error"] = "User not found!";
					return;
				}
				if (m_users[iUser].Username != sUsername)
				{
					root["error"] = "User mismatch!";
					return;
				}

				std::string sOldPwd = request::findValue(&req, "oldpwd");
				std::string sNewPwd = request::findValue(&req, "newpwd");
				if (!sOldPwd.empty() && !sNewPwd.empty())
				{
					if (m_users[iUser].Password == sOldPwd)
					{
						m_users[iUser].Password = sNewPwd;
						m_sql.safe_query("UPDATE Users SET Password='%q' WHERE (ID=%d)", sNewPwd.c_str(), m_users[iUser].ID);
						LoadUsers();	// Make sure the new password is loaded in memory
						root["status"] = "OK";
					}
					else
					{
						root["error"] = "Old password mismatch!";
						return;
					}
				}

				std::string sTotpsecret = request::findValue(&req, "totpsecret");
				std::string sTotpCode = request::findValue(&req, "totpcode");
				bool bEnablemfa = (request::findValue(&req, "enablemfa") == "true" ? true : false);
				if (bEnablemfa && sTotpsecret.empty())
				{
					root["error"] = "Not a valid TOTP secret!";
					return;
				}
				// Update the User Profile
				if (!bEnablemfa)
				{
					sTotpsecret = "";
				}
				else
				{
					//verify code
					if (!sTotpCode.empty())
					{
						std::string sTotpKey = "";
						if (base32_decode(sTotpsecret, sTotpKey))
						{
							if (!VerifySHA1TOTP(sTotpCode, sTotpKey))
							{
								root["error"] = "Incorrect/expired 6 digit code!";
								return;
							}
						}
					}
				}
				m_users[iUser].Mfatoken = sTotpsecret;
				m_sql.safe_query("UPDATE Users SET MFAsecret='%q' WHERE (ID=%d)", sTotpsecret.c_str(), m_users[iUser].ID);

				LoadUsers();
				root["status"] = "OK";
			}
		}

		void CWebServer::Cmd_GetUptime(WebEmSession& session, const request& req, Json::Value& root)
		{
			// this is used in the about page, we are going to round the seconds a bit to display nicer
			time_t atime = mytime(nullptr);
			time_t tuptime = atime - m_StartTime;
			// round to 5 seconds (nicer in about page)
			tuptime = ((tuptime / 5) * 5) + 5;
			int days, hours, minutes, seconds;
			days = (int)(tuptime / 86400);
			tuptime -= (days * 86400);
			hours = (int)(tuptime / 3600);
			tuptime -= (hours * 3600);
			minutes = (int)(tuptime / 60);
			tuptime -= (minutes * 60);
			seconds = (int)tuptime;
			root["status"] = "OK";
			root["title"] = "GetUptime";
			root["days"] = days;
			root["hours"] = hours;
			root["minutes"] = minutes;
			root["seconds"] = seconds;
		}

		void CWebServer::Cmd_GetActualHistory(WebEmSession& session, const request& req, Json::Value& root)
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

		void CWebServer::Cmd_GetNewHistory(WebEmSession& session, const request& req, Json::Value& root)
		{
			root["status"] = "OK";
			root["title"] = "GetNewHistory";

			std::string historyfile;
			int nValue;
			m_sql.GetPreferencesVar("ReleaseChannel", nValue);
			bool bIsBetaChannel = (nValue != 0);

			utsname my_uname;
			if (uname(&my_uname) < 0)
				return;

			std::string systemname = my_uname.sysname;
			std::string machine = my_uname.machine;
			std::transform(systemname.begin(), systemname.end(), systemname.begin(), ::tolower);

			if (machine == "armv6l" || (machine == "aarch64" && sizeof(void*) == 4))
			{
				// Seems like old arm systems can also use the new arm build
				machine = "armv7l";
			}

			std::string szHistoryURL = "https://www.domoticz.com/download.php?channel=stable&type=history";
			if (bIsBetaChannel)
			{
				if (((machine != "armv6l") && (machine != "armv7l") && (systemname != "windows") && (machine != "x86_64") && (machine != "aarch64")) ||
					(strstr(my_uname.release, "ARCH+") != nullptr))
					szHistoryURL = "https://www.domoticz.com/download.php?channel=beta&type=history";
				else
					szHistoryURL = "https://www.domoticz.com/download.php?channel=beta&type=history&system=" + systemname + "&machine=" + machine;
			}
			std::vector<std::string> ExtraHeaders;
			ExtraHeaders.push_back("Unique_ID: " + m_sql.m_UniqueID);
			ExtraHeaders.push_back("App_Version: " + szAppVersion);
			ExtraHeaders.push_back("App_Revision: " + std::to_string(iAppRevision));
			ExtraHeaders.push_back("System_Name: " + systemname);
			ExtraHeaders.push_back("Machine: " + machine);
			ExtraHeaders.push_back("Type: " + (!bIsBetaChannel) ? "Stable" : "Beta");

			if (!HTTPClient::GET(szHistoryURL, ExtraHeaders, historyfile))
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

		void CWebServer::Cmd_GetConfig(WebEmSession& session, const request& req, Json::Value& root)
		{
			Cmd_GetVersion(session, req, root);
			root["status"] = "ERR";
			root["title"] = "GetConfig";

			std::string sValue;
			int nValue = 0;
			int iDashboardType = 0;

			if (m_sql.GetPreferencesVar("Language", sValue))
			{
				root["language"] = sValue;
			}
			if (m_sql.GetPreferencesVar("DegreeDaysBaseTemperature", sValue))
			{
				root["DegreeDaysBaseTemperature"] = atof(sValue.c_str());
			}
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

			root["WindScale"] = m_sql.m_windscale * 10.0F;
			root["WindSign"] = m_sql.m_windsign;
			root["TempScale"] = m_sql.m_tempscale;
			root["TempSign"] = m_sql.m_tempsign;

			int iUser = -1;
			if (!session.username.empty() && (iUser = FindUser(session.username.c_str())) != -1)
			{
				unsigned long UserID = m_users[iUser].ID;
				root["UserName"] = m_users[iUser].Username;

				int bEnableTabDashboard = 1;
				int bEnableTabFloorplans = 0;
				int bEnableTabLight = 1;
				int bEnableTabScenes = 1;
				int bEnableTabTemp = 1;
				int bEnableTabWeather = 1;
				int bEnableTabUtility = 1;
				int bEnableTabCustom = 0;

				std::vector<std::vector<std::string>> result;
				result = m_sql.safe_query("SELECT TabsEnabled FROM Users WHERE (ID==%lu)", UserID);
				if (!result.empty())
				{
					int TabsEnabled = atoi(result[0][0].c_str());
					bEnableTabLight = (TabsEnabled & (1 << 0));
					bEnableTabScenes = (TabsEnabled & (1 << 1));
					bEnableTabTemp = (TabsEnabled & (1 << 2));
					bEnableTabWeather = (TabsEnabled & (1 << 3));
					bEnableTabUtility = (TabsEnabled & (1 << 4));
					bEnableTabCustom = (TabsEnabled & (1 << 5));
					bEnableTabFloorplans = (TabsEnabled & (1 << 6));
				}

				if (iDashboardType == 3)
				{
					// Floorplan , no need to show a tab floorplan
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
					// Add custom templates
					DIR* lDir;
					struct dirent* ent;
					std::string templatesFolder = szWWWFolder + "/templates";
					int iFile = 0;
					if ((lDir = opendir(templatesFolder.c_str())) != nullptr)
					{
						while ((ent = readdir(lDir)) != nullptr)
						{
							std::string filename = ent->d_name;
							size_t pos = filename.find(".htm");
							if (pos != std::string::npos)
							{
								std::string shortfile = filename.substr(0, pos);
								root["result"]["templates"][iFile]["file"] = shortfile;
								stdreplace(shortfile, "_", " ");
								root["result"]["templates"][iFile]["name"] = shortfile;
								iFile++;
								continue;
							}
							// Same thing for URLs
							pos = filename.find(".url");
							if (pos != std::string::npos)
							{
								std::string url;
								std::string shortfile = filename.substr(0, pos);
								// First get the URL from the file
								std::ifstream urlfile;
								urlfile.open((templatesFolder + "/" + filename).c_str());
								if (urlfile.is_open())
								{
									getline(urlfile, url);
									urlfile.close();
									// Pass URL in results
									stdreplace(shortfile, "_", " ");
									root["result"]["urls"][shortfile] = url;
								}
							}
						}
						closedir(lDir);
					}
				}
			}
			root["status"] = "OK";
		}

		// Could now be obsolete as only 1 usage was found in Forecast screen, which now uses other command
		void CWebServer::Cmd_GetLocation(WebEmSession& session, const request& req, Json::Value& root)
		{
			if (session.rights == -1)
			{
				session.reply_status = reply::forbidden;
				return; // Only auth user allowed
			}
			root["title"] = "GetLocation";
			std::string Latitude = "1";
			std::string Longitude = "1";
			std::string sValue;
			if (m_sql.GetPreferencesVar("Location", sValue))
			{
				std::vector<std::string> strarray;
				StringSplit(sValue, ";", strarray);

				if (strarray.size() == 2)
				{
					Latitude = strarray[0];
					Longitude = strarray[1];
					root["status"] = "OK";
				}
			}
			root["Latitude"] = Latitude;
			root["Longitude"] = Longitude;
		}

		void CWebServer::Cmd_GetForecastConfig(WebEmSession& session, const request& req, Json::Value& root)
		{
			if (session.rights == -1)
			{
				session.reply_status = reply::forbidden;
				return; // Only auth user allowed
			}

			std::string Latitude = "1";
			std::string Longitude = "1";
			std::string sValue, sFURL, forecast_url;
			std::stringstream ss, sURL;
			uint8_t iSucces = 0;
			bool iFrame = true;

			root["title"] = "GetForecastConfig";
			root["status"] = "Error";

			if (m_sql.GetPreferencesVar("Location", sValue))
			{
				std::vector<std::string> strarray;
				StringSplit(sValue, ";", strarray);

				if (strarray.size() == 2)
				{
					Latitude = strarray[0];
					Longitude = strarray[1];
					iSucces++;
				}
				root["Latitude"] = Latitude;
				root["Longitude"] = Longitude;
				sValue = "";
				sValue.clear();
			}

			root["Forecasthardware"] = 0;
			int iValue = 0;
			if (m_sql.GetPreferencesVar("ForecastHardwareID", iValue))
			{
				root["Forecasthardware"] = iValue;
			}

			if (root["Forecasthardware"] > 0)
			{
				int iHardwareID = root["Forecasthardware"].asInt();
				CDomoticzHardwareBase* pHardware = m_mainworker.GetHardware(iHardwareID);
				if (pHardware != nullptr)
				{
					if (pHardware->HwdType == HTYPE_OpenWeatherMap)
					{
						root["Forecasthardwaretype"] = HTYPE_OpenWeatherMap;
						COpenWeatherMap* pWHardware = dynamic_cast<COpenWeatherMap*>(pHardware);
						forecast_url = pWHardware->GetForecastURL();
						if (!forecast_url.empty())
						{
							sFURL = forecast_url;
							iFrame = false;
						}
						Json::Value forecast_data = pWHardware->GetForecastData();
						if (!forecast_data.empty())
						{
							root["Forecastdata"] = forecast_data;
						}
					}
					else if (pHardware->HwdType == HTYPE_BuienRadar)
					{
						root["Forecasthardwaretype"] = HTYPE_BuienRadar;
						CBuienRadar* pWHardware = dynamic_cast<CBuienRadar*>(pHardware);
						forecast_url = pWHardware->GetForecastURL();
						if (!forecast_url.empty())
						{
							sFURL = forecast_url;
						}
					}
					else if (pHardware->HwdType == HTYPE_VisualCrossing)
					{
						root["Forecasthardwaretype"] = HTYPE_VisualCrossing;
						CVisualCrossing* pWHardware = dynamic_cast<CVisualCrossing*>(pHardware);
						forecast_url = pWHardware->GetForecastURL();
						if (!forecast_url.empty())
						{
							sFURL = forecast_url;
						}
					}
					else
					{
						root["Forecasthardware"] = 0; // reset to 0
					}
				}
				else
				{
					_log.Debug(DEBUG_WEBSERVER, "CWebServer::GetForecastConfig() : Could not find hardware (not active?) for ID %s!", root["Forecasthardware"].asString().c_str());
					root["Forecasthardware"] = 0; // reset to 0
				}
			}

			if (root["Forecasthardware"] == 0 && iSucces == 1)
			{
				// No forecast device, but we have geo coords, so enough for fallback
				iSucces++;
			}
			else if (!sFURL.empty())
			{
				root["Forecasturl"] = sFURL;
				iSucces++;
			}

			if (iSucces == 2)
			{
				root["status"] = "OK";
			}
		}

		void CWebServer::Cmd_SendNotification(WebEmSession& session, const request& req, Json::Value& root)
		{
			std::string subject = request::findValue(&req, "subject");
			std::string body = request::findValue(&req, "body");
			std::string subsystem = request::findValue(&req, "subsystem");
			std::string extradata = request::findValue(&req, "extradata");
			if ((subject.empty()) || (body.empty()))
				return;
			if (subsystem.empty())
				subsystem = NOTIFYALL;
			// Add to queue
			if (m_notifications.SendMessage(0, std::string(""), subsystem, std::string(""), subject, body, extradata, 1, std::string(""), false))
			{
				root["status"] = "OK";
			}
			root["title"] = "SendNotification";
		}

		void CWebServer::Cmd_EmailCameraSnapshot(WebEmSession& session, const request& req, Json::Value& root)
		{
			std::string camidx = request::findValue(&req, "camidx");
			std::string subject = request::findValue(&req, "subject");
			if ((camidx.empty()) || (subject.empty()))
				return;
			// Add to queue
			m_sql.AddTaskItem(_tTaskItem::EmailCameraSnapshot(1, camidx, subject));
			root["status"] = "OK";
			root["title"] = "Email Camera Snapshot";
		}

		void CWebServer::Cmd_UpdateDevice(WebEmSession& session, const request& req, Json::Value& root)
		{
			std::string Username = "Admin";
			if (!session.username.empty())
				Username = session.username;

			if (session.rights < 1)
			{
				session.reply_status = reply::forbidden;
				return; // only user or higher allowed
			}

			std::string idx = request::findValue(&req, "idx");

			if (!IsIdxForUser(&session, atoi(idx.c_str())))
			{
				_log.Log(LOG_ERROR, "User: %s tried to update an Unauthorized device!", session.username.c_str());
				session.reply_status = reply::forbidden;
				return;
			}

			std::string hid = request::findValue(&req, "hid");
			std::string did = request::findValue(&req, "did");
			std::string dunit = request::findValue(&req, "dunit");
			std::string dtype = request::findValue(&req, "dtype");
			std::string dsubtype = request::findValue(&req, "dsubtype");

			std::string nvalue = request::findValue(&req, "nvalue");
			std::string svalue = request::findValue(&req, "svalue");
			std::string ptrigger = request::findValue(&req, "parsetrigger");

			bool parseTrigger = (ptrigger != "false");

			if ((nvalue.empty() && svalue.empty()))
			{
				return;
			}

			int signallevel = 12;
			int batterylevel = 255;

			if (idx.empty())
			{
				// No index supplied, check if raw parameters where supplied
				if ((hid.empty()) || (did.empty()) || (dunit.empty()) || (dtype.empty()) || (dsubtype.empty()))
					return;
			}
			else
			{
				// Get the raw device parameters
				std::vector<std::vector<std::string>> result;
				result = m_sql.safe_query("SELECT HardwareID, DeviceID, Unit, Type, SubType FROM DeviceStatus WHERE (ID=='%q')", idx.c_str());
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

			// uint64_t ulIdx = std::stoull(idx);

			int invalue = atoi(nvalue.c_str());

			std::string sSignalLevel = request::findValue(&req, "rssi");
			if (!sSignalLevel.empty())
			{
				signallevel = atoi(sSignalLevel.c_str());
			}
			std::string sBatteryLevel = request::findValue(&req, "battery");
			if (!sBatteryLevel.empty())
			{
				batterylevel = atoi(sBatteryLevel.c_str());
			}
			std::string szUpdateUser = Username + " (IP: " + session.remote_host + ")";
			if (m_mainworker.UpdateDevice(HardwareID, DeviceID, unit, devType, subType, invalue, svalue, szUpdateUser, signallevel, batterylevel, parseTrigger))
			{
				root["status"] = "OK";
				root["title"] = "Update Device";
			}
		}

		void CWebServer::Cmd_UpdateDevices(WebEmSession& session, const request& req, Json::Value& root)
		{
			std::string script = request::findValue(&req, "script");
			if (script.empty())
			{
				return;
			}
			std::string content = req.content;

			std::vector<std::string> allParameters;

			// Keep the url content on the right of the '?'
			std::vector<std::string> allParts;
			StringSplit(req.uri, "?", allParts);
			if (!allParts.empty())
			{
				// Split all url parts separated by a '&'
				StringSplit(allParts[1], "&", allParameters);
			}

			CLuaHandler luaScript;
			bool ret = luaScript.executeLuaScript(script, content, allParameters);
			if (ret)
			{
				root["status"] = "OK";
				root["title"] = "Update Device";
			}
		}

		void CWebServer::Cmd_CustomEvent(WebEmSession& session, const request& req, Json::Value& root)
		{
			if (session.rights < 1)
			{
				session.reply_status = reply::forbidden;
				return; // only user or higher allowed
			}
			Json::Value eventInfo;
			eventInfo["name"] = request::findValue(&req, "event");
			if (!req.content.empty())
				eventInfo["data"] = req.content.c_str(); // data from POST
			else
				eventInfo["data"] = request::findValue(&req, "data"); // data in URL

			if (eventInfo["name"].empty())
			{
				return;
			}

			m_mainworker.m_notificationsystem.Notify(Notification::DZ_CUSTOM, Notification::STATUS_INFO, JSonToRawString(eventInfo));

			root["status"] = "OK";
			root["title"] = "Custom Event";
		}

		void CWebServer::Cmd_SetThermostatState(WebEmSession& session, const request& req, Json::Value& root)
		{
			std::string sstate = request::findValue(&req, "state");
			std::string idx = request::findValue(&req, "idx");
			std::string name = HTMLSanitizer::Sanitize(request::findValue(&req, "name"));

			if ((idx.empty()) || (sstate.empty()))
				return;
			int iState = atoi(sstate.c_str());

			int urights = 3;
			bool bHaveUser = (!session.username.empty());
			if (bHaveUser)
			{
				int iUser = FindUser(session.username.c_str());
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

		void CWebServer::Cmd_SystemShutdown(WebEmSession& session, const request& req, Json::Value& root)
		{
			if (session.rights != 2)
			{
				session.reply_status = reply::forbidden;
				return; // Only admin user allowed
			}
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

		void CWebServer::Cmd_SystemReboot(WebEmSession& session, const request& req, Json::Value& root)
		{
			if (session.rights != 2)
			{
				session.reply_status = reply::forbidden;
				return; // Only admin user allowed
			}
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

		void CWebServer::Cmd_ExcecuteScript(WebEmSession& session, const request& req, Json::Value& root)
		{
			if (session.rights != 2)
			{
				session.reply_status = reply::forbidden;
				return; // Only admin user allowed
			}
			std::string scriptname = request::findValue(&req, "scriptname");
			if (scriptname.empty())
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
			std::string script_params = request::findValue(&req, "scriptparams");
			std::string strparm = szUserDataFolder;
			if (!script_params.empty())
			{
				if (!strparm.empty())
					strparm += " " + script_params;
				else
					strparm = script_params;
			}
			std::string sdirect = request::findValue(&req, "direct");
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
				// add script to background worker
				m_sql.AddTaskItem(_tTaskItem::ExecuteScript(0.2F, scriptname, strparm));
			}
			root["title"] = "ExecuteScript";
			root["status"] = "OK";
		}

		// Only for Unix systems
		void CWebServer::Cmd_ApplicationUpdate(WebEmSession& session, const request& req, Json::Value& root)
		{
			if (session.rights != 2)
			{
				session.reply_status = reply::forbidden;
				return; // Only admin user allowed
			}
#ifdef WIN32
#ifndef _DEBUG
			return;
#endif
#endif
			int nValue;
			m_sql.GetPreferencesVar("ReleaseChannel", nValue);
			bool bIsBetaChannel = (nValue != 0);

			std::string scriptname(szStartupFolder);
			scriptname += (bIsBetaChannel) ? "updatebeta" : "updaterelease";
			// run script in background
			std::string lscript = scriptname + " &";
			int ret = system(lscript.c_str());
			root["title"] = "UpdateApplication";
			root["status"] = "OK";
		}

		void CWebServer::Cmd_GetCosts(WebEmSession& session, const request& req, Json::Value& root)
		{
			int nValue = 0;
			m_sql.GetPreferencesVar("CostEnergy", nValue);
			root["CostEnergy"] = nValue;
			m_sql.GetPreferencesVar("CostEnergyT2", nValue);
			root["CostEnergyT2"] = nValue;
			m_sql.GetPreferencesVar("CostEnergyR1", nValue);
			root["CostEnergyR1"] = nValue;
			m_sql.GetPreferencesVar("CostEnergyR2", nValue);
			root["CostEnergyR2"] = nValue;
			m_sql.GetPreferencesVar("CostGas", nValue);
			root["CostGas"] = nValue;
			m_sql.GetPreferencesVar("CostWater", nValue);
			root["CostWater"] = nValue;

			int tValue = 1000;
			if (m_sql.GetPreferencesVar("MeterDividerWater", tValue))
			{
				root["DividerWater"] = float(tValue);
			}
			float EnergyDivider = 1000.0F;
			if (m_sql.GetPreferencesVar("MeterDividerEnergy", tValue))
			{
				EnergyDivider = float(tValue);
				root["DividerEnergy"] = EnergyDivider;
			}

			std::string idx = request::findValue(&req, "idx");
			if (idx.empty())
				return;

			char szTmp[100];
			std::vector<std::vector<std::string>> result;
			result = m_sql.safe_query("SELECT Type, SubType, nValue, sValue FROM DeviceStatus WHERE (ID=='%q')", idx.c_str());
			if (!result.empty())
			{
				root["status"] = "OK";
				root["title"] = "GetCosts";

				std::vector<std::string> sd = result[0];
				unsigned char dType = atoi(sd[0].c_str());
				// unsigned char subType = atoi(sd[1].c_str());
				// nValue = (unsigned char)atoi(sd[2].c_str());
				std::string sValue = sd[3];

				if (dType == pTypeP1Power)
				{
					// also provide the counter values

					std::vector<std::string> splitresults;
					StringSplit(sValue, ";", splitresults);
					if (splitresults.size() != 6)
						return;

					uint64_t powerusage1 = std::stoull(splitresults[0]);
					uint64_t powerusage2 = std::stoull(splitresults[1]);
					uint64_t powerdeliv1 = std::stoull(splitresults[2]);
					uint64_t powerdeliv2 = std::stoull(splitresults[3]);
					// uint64_t usagecurrent = std::stoull(splitresults[4]);
					// uint64_t delivcurrent = std::stoull(splitresults[5]);

					powerdeliv1 = (powerdeliv1 < 10) ? 0 : powerdeliv1;
					powerdeliv2 = (powerdeliv2 < 10) ? 0 : powerdeliv2;

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

		void CWebServer::Cmd_CheckForUpdate(WebEmSession& session, const request& req, Json::Value& root)
		{
			bool bHaveUser = (!session.username.empty());
			int urights = 3;
			if (bHaveUser)
			{
				int iUser = FindUser(session.username.c_str());
				if (iUser != -1)
					urights = static_cast<int>(m_users[iUser].userrights);
			}
			root["statuscode"] = urights;

			root["status"] = "OK";
			root["title"] = "CheckForUpdate";
			root["HaveUpdate"] = false;
			root["Revision"] = m_mainworker.m_iRevision;

			if (session.rights != 2)
			{
				session.reply_status = reply::forbidden;
				return; // Only admin users may update
			}

			bool bIsForced = (request::findValue(&req, "forced") == "true");

			if (!bIsForced)
			{
				int nValue = 0;
				m_sql.GetPreferencesVar("UseAutoUpdate", nValue);
				if (nValue != 1)
				{
					return;
				}
			}

			root["HaveUpdate"] = m_mainworker.IsUpdateAvailable(bIsForced);
			root["DomoticzUpdateURL"] = m_mainworker.m_szDomoticzUpdateURL;
			root["SystemName"] = m_mainworker.m_szSystemName;
			root["Revision"] = m_mainworker.m_iRevision;
		}

		void CWebServer::Cmd_DownloadUpdate(WebEmSession& session, const request& req, Json::Value& root)
		{
			if (!m_mainworker.StartDownloadUpdate())
				return;
			root["status"] = "OK";
			root["title"] = "DownloadUpdate";
		}

		void CWebServer::Cmd_DownloadReady(WebEmSession& session, const request& req, Json::Value& root)
		{
			if (!m_mainworker.m_bHaveDownloadedDomoticzUpdate)
				return;
			root["status"] = "OK";
			root["title"] = "DownloadReady";
			root["downloadok"] = (m_mainworker.m_bHaveDownloadedDomoticzUpdateSuccessFull) ? true : false;
		}

		void CWebServer::Cmd_DeleteDateRange(WebEmSession& session, const request& req, Json::Value& root)
		{
			if (session.rights != 2)
			{
				session.reply_status = reply::forbidden;
				return; // Only admin user allowed
			}
			const std::string idx = request::findValue(&req, "idx");
			const std::string fromDate = request::findValue(&req, "fromdate");
			const std::string toDate = request::findValue(&req, "todate");
			if ((idx.empty()) || (fromDate.empty() || toDate.empty()))
				return;
			root["status"] = "OK";
			root["title"] = "deletedaterange";
			m_sql.DeleteDateRange(idx.c_str(), fromDate, toDate);
		}

		void CWebServer::Cmd_DeleteDataPoint(WebEmSession& session, const request& req, Json::Value& root)
		{
			if (session.rights != 2)
			{
				session.reply_status = reply::forbidden;
				return; // Only admin user allowed
			}
			const std::string idx = request::findValue(&req, "idx");
			const std::string Date = request::findValue(&req, "date");

			if ((idx.empty()) || (Date.empty()))
				return;

			root["status"] = "OK";
			root["title"] = "deletedatapoint";
			m_sql.DeleteDataPoint(idx.c_str(), Date);
		}

		// PostSettings
		void CWebServer::Cmd_PostSettings(WebEmSession& session, const request& req, Json::Value& root)
		{
			if (session.rights != 2)
			{
				session.reply_status = reply::forbidden;
				return; // Only admin user allowed
			}

			root["title"] = "StoreSettings";
			root["status"] = "ERR";

			uint16_t cntSettings = 0;

			try {

				/* Start processing the simple ones */
				/* -------------------------------- */

				m_sql.UpdatePreferencesVar("DashboardType", atoi(request::findValue(&req, "DashboardType").c_str())); cntSettings++;
				m_sql.UpdatePreferencesVar("MobileType", atoi(request::findValue(&req, "MobileType").c_str())); cntSettings++;
				m_sql.UpdatePreferencesVar("ReleaseChannel", atoi(request::findValue(&req, "ReleaseChannel").c_str())); cntSettings++;
				m_sql.UpdatePreferencesVar("LightHistoryDays", atoi(request::findValue(&req, "LightHistoryDays").c_str())); cntSettings++;
				m_sql.UpdatePreferencesVar("5MinuteHistoryDays", atoi(request::findValue(&req, "ShortLogDays").c_str())); cntSettings++;
				m_sql.UpdatePreferencesVar("ElectricVoltage", atoi(request::findValue(&req, "ElectricVoltage").c_str())); cntSettings++;
				m_sql.UpdatePreferencesVar("CM113DisplayType", atoi(request::findValue(&req, "CM113DisplayType").c_str())); cntSettings++;
				m_sql.UpdatePreferencesVar("MaxElectricPower", atoi(request::findValue(&req, "MaxElectricPower").c_str())); cntSettings++;
				m_sql.UpdatePreferencesVar("DoorbellCommand", atoi(request::findValue(&req, "DoorbellCommand").c_str())); cntSettings++;
				m_sql.UpdatePreferencesVar("SmartMeterType", atoi(request::findValue(&req, "SmartMeterType").c_str())); cntSettings++;
				m_sql.UpdatePreferencesVar("SecOnDelay", atoi(request::findValue(&req, "SecOnDelay").c_str())); cntSettings++;
				m_sql.UpdatePreferencesVar("FloorplanPopupDelay", atoi(request::findValue(&req, "FloorplanPopupDelay").c_str())); cntSettings++;
				m_sql.UpdatePreferencesVar("FloorplanActiveOpacity", atoi(request::findValue(&req, "FloorplanActiveOpacity").c_str())); cntSettings++;
				m_sql.UpdatePreferencesVar("FloorplanInactiveOpacity", atoi(request::findValue(&req, "FloorplanInactiveOpacity").c_str())); cntSettings++;
				m_sql.UpdatePreferencesVar("OneWireSensorPollPeriod", atoi(request::findValue(&req, "OneWireSensorPollPeriod").c_str())); cntSettings++;
				m_sql.UpdatePreferencesVar("OneWireSwitchPollPeriod", atoi(request::findValue(&req, "OneWireSwitchPollPeriod").c_str())); cntSettings++;

				m_sql.UpdatePreferencesVar("UseAutoUpdate", (request::findValue(&req, "checkforupdates") == "on" ? 1 : 0)); cntSettings++;
				m_sql.UpdatePreferencesVar("UseAutoBackup", (request::findValue(&req, "enableautobackup") == "on" ? 1 : 0)); cntSettings++;
				m_sql.UpdatePreferencesVar("HideDisabledHardwareSensors", (request::findValue(&req, "HideDisabledHardwareSensors") == "on" ? 1 : 0)); cntSettings++;
				m_sql.UpdatePreferencesVar("ShowUpdateEffect", (request::findValue(&req, "ShowUpdateEffect") == "on" ? 1 : 0)); cntSettings++;
				m_sql.UpdatePreferencesVar("FloorplanFullscreenMode", (request::findValue(&req, "FloorplanFullscreenMode") == "on" ? 1 : 0)); cntSettings++;
				m_sql.UpdatePreferencesVar("FloorplanAnimateZoom", (request::findValue(&req, "FloorplanAnimateZoom") == "on" ? 1 : 0)); cntSettings++;
				m_sql.UpdatePreferencesVar("FloorplanShowSensorValues", (request::findValue(&req, "FloorplanShowSensorValues") == "on" ? 1 : 0)); cntSettings++;
				m_sql.UpdatePreferencesVar("FloorplanShowSwitchValues", (request::findValue(&req, "FloorplanShowSwitchValues") == "on" ? 1 : 0)); cntSettings++;
				m_sql.UpdatePreferencesVar("FloorplanShowSceneNames", (request::findValue(&req, "FloorplanShowSceneNames") == "on" ? 1 : 0)); cntSettings++;
				m_sql.UpdatePreferencesVar("IFTTTEnabled", (request::findValue(&req, "IFTTTEnabled") == "on" ? 1 : 0)); cntSettings++;

				m_sql.UpdatePreferencesVar("Language", request::findValue(&req, "Language")); cntSettings++;
				m_sql.UpdatePreferencesVar("DegreeDaysBaseTemperature", request::findValue(&req, "DegreeDaysBaseTemperature")); cntSettings++;

				m_sql.UpdatePreferencesVar("FloorplanRoomColour", CURLEncode::URLDecode(request::findValue(&req, "FloorplanRoomColour"))); cntSettings++;
				m_sql.UpdatePreferencesVar("IFTTTAPI", base64_encode(request::findValue(&req, "IFTTTAPI"))); cntSettings++;

				m_sql.UpdatePreferencesVar("Title", (request::findValue(&req, "Title").empty()) ? "Domoticz" : request::findValue(&req, "Title")); cntSettings++;

				/* More complex ones that need additional processing */
				/* ------------------------------------------------- */

				float CostEnergy = static_cast<float>(atof(request::findValue(&req, "CostEnergy").c_str()));
				m_sql.UpdatePreferencesVar("CostEnergy", int(CostEnergy * 10000.0F)); cntSettings++;
				float CostEnergyT2 = static_cast<float>(atof(request::findValue(&req, "CostEnergyT2").c_str()));
				m_sql.UpdatePreferencesVar("CostEnergyT2", int(CostEnergyT2 * 10000.0F)); cntSettings++;
				float CostEnergyR1 = static_cast<float>(atof(request::findValue(&req, "CostEnergyR1").c_str()));
				m_sql.UpdatePreferencesVar("CostEnergyR1", int(CostEnergyR1 * 10000.0F)); cntSettings++;
				float CostEnergyR2 = static_cast<float>(atof(request::findValue(&req, "CostEnergyR2").c_str()));
				m_sql.UpdatePreferencesVar("CostEnergyR2", int(CostEnergyR2 * 10000.0F)); cntSettings++;
				float CostGas = static_cast<float>(atof(request::findValue(&req, "CostGas").c_str()));
				m_sql.UpdatePreferencesVar("CostGas", int(CostGas * 10000.0F)); cntSettings++;
				float CostWater = static_cast<float>(atof(request::findValue(&req, "CostWater").c_str()));
				m_sql.UpdatePreferencesVar("CostWater", int(CostWater * 10000.0F)); cntSettings++;

				int EnergyDivider = atoi(request::findValue(&req, "EnergyDivider").c_str());
				if (EnergyDivider < 1)
					EnergyDivider = 1000;
				m_sql.UpdatePreferencesVar("MeterDividerEnergy", EnergyDivider); cntSettings++;
				int GasDivider = atoi(request::findValue(&req, "GasDivider").c_str());
				if (GasDivider < 1)
					GasDivider = 100;
				m_sql.UpdatePreferencesVar("MeterDividerGas", GasDivider); cntSettings++;
				int WaterDivider = atoi(request::findValue(&req, "WaterDivider").c_str());
				if (WaterDivider < 1)
					WaterDivider = 100;
				m_sql.UpdatePreferencesVar("MeterDividerWater", WaterDivider); cntSettings++;

				int sensortimeout = atoi(request::findValue(&req, "SensorTimeout").c_str());
				if (sensortimeout < 10)
					sensortimeout = 10;
				m_sql.UpdatePreferencesVar("SensorTimeout", sensortimeout); cntSettings++;

				std::string RaspCamParams = request::findValue(&req, "RaspCamParams");
				if ((!RaspCamParams.empty()) && (IsArgumentSecure(RaspCamParams)))
					m_sql.UpdatePreferencesVar("RaspCamParams", RaspCamParams);
				cntSettings++;

				std::string UVCParams = request::findValue(&req, "UVCParams");
				if ((!UVCParams.empty()) && (IsArgumentSecure(UVCParams)))
					m_sql.UpdatePreferencesVar("UVCParams", UVCParams);
				cntSettings++;

				/* Also update m_sql.variables */
				/* --------------------------- */

				int iShortLogInterval = atoi(request::findValue(&req, "ShortLogInterval").c_str());
				if (iShortLogInterval < 1)
					iShortLogInterval = 5;
				m_sql.m_ShortLogInterval = iShortLogInterval;
				m_sql.UpdatePreferencesVar("ShortLogInterval", m_sql.m_ShortLogInterval); cntSettings++;

				m_sql.m_bShortLogAddOnlyNewValues = (request::findValue(&req, "ShortLogAddOnlyNewValues") == "on" ? 1 : 0);
				m_sql.UpdatePreferencesVar("ShortLogAddOnlyNewValues", m_sql.m_bShortLogAddOnlyNewValues); cntSettings++;

				m_sql.m_bLogEventScriptTrigger = (request::findValue(&req, "LogEventScriptTrigger") == "on" ? 1 : 0);
				m_sql.UpdatePreferencesVar("LogEventScriptTrigger", m_sql.m_bLogEventScriptTrigger); cntSettings++;

				m_sql.m_bAllowWidgetOrdering = (request::findValue(&req, "AllowWidgetOrdering") == "on" ? 1 : 0);
				m_sql.UpdatePreferencesVar("AllowWidgetOrdering", m_sql.m_bAllowWidgetOrdering); cntSettings++;

				int iEnableNewHardware = (request::findValue(&req, "AcceptNewHardware") == "on" ? 1 : 0);
				m_sql.m_bAcceptNewHardware = (iEnableNewHardware == 1);
				m_sql.UpdatePreferencesVar("AcceptNewHardware", m_sql.m_bAcceptNewHardware); cntSettings++;

				int nUnit = atoi(request::findValue(&req, "WindUnit").c_str());
				m_sql.m_windunit = (_eWindUnit)nUnit;
				m_sql.UpdatePreferencesVar("WindUnit", m_sql.m_windunit); cntSettings++;

				nUnit = atoi(request::findValue(&req, "TempUnit").c_str());
				m_sql.m_tempunit = (_eTempUnit)nUnit;
				m_sql.UpdatePreferencesVar("TempUnit", m_sql.m_tempunit); cntSettings++;

				nUnit = atoi(request::findValue(&req, "WeightUnit").c_str());
				m_sql.m_weightunit = (_eWeightUnit)nUnit;
				m_sql.UpdatePreferencesVar("WeightUnit", m_sql.m_weightunit); cntSettings++;

				m_sql.SetUnitsAndScale();

				/* Update Preferences and call other functions as well due to changes */
				/* ------------------------------------------------------------------ */

				std::string Latitude = request::findValue(&req, "Latitude");
				std::string Longitude = request::findValue(&req, "Longitude");
				if ((!Latitude.empty()) && (!Longitude.empty()))
				{
					std::string LatLong = Latitude + ";" + Longitude;
					m_sql.UpdatePreferencesVar("Location", LatLong);
					m_mainworker.GetSunSettings();
				}
				cntSettings++;

				bool AllowPlainBasicAuth = (request::findValue(&req, "AllowPlainBasicAuth") == "on" ? 1 : 0);
				m_sql.UpdatePreferencesVar("AllowPlainBasicAuth", AllowPlainBasicAuth);

				m_pWebEm->SetAllowPlainBasicAuth(AllowPlainBasicAuth);
				cntSettings++;

				std::string WebLocalNetworks = CURLEncode::URLDecode(request::findValue(&req, "WebLocalNetworks"));
				m_sql.UpdatePreferencesVar("WebLocalNetworks", WebLocalNetworks);
				m_webservers.ReloadTrustedNetworks();
				cntSettings++;

				if (session.username.empty())
				{
					// Local network could be changed so lets force a check here
					session.rights = -1;
				}

				std::string SecPassword = request::findValue(&req, "SecPassword");
				SecPassword = CURLEncode::URLDecode(SecPassword);
				if (SecPassword.size() != 32)
				{
					SecPassword = GenerateMD5Hash(SecPassword);
				}
				m_sql.UpdatePreferencesVar("SecPassword", SecPassword);
				cntSettings++;

				std::string ProtectionPassword = request::findValue(&req, "ProtectionPassword");
				ProtectionPassword = CURLEncode::URLDecode(ProtectionPassword);
				if (ProtectionPassword.size() != 32)
				{
					ProtectionPassword = GenerateMD5Hash(ProtectionPassword);
				}
				m_sql.UpdatePreferencesVar("ProtectionPassword", ProtectionPassword);
				cntSettings++;

				std::string SendErrorsAsNotification = request::findValue(&req, "SendErrorsAsNotification");
				int iSendErrorsAsNotification = (SendErrorsAsNotification == "on" ? 1 : 0);
				m_sql.UpdatePreferencesVar("SendErrorsAsNotification", iSendErrorsAsNotification);
				_log.ForwardErrorsToNotificationSystem(iSendErrorsAsNotification != 0);
				cntSettings++;

				int rnOldvalue = 0;
				int rnvalue = 0;

				m_sql.GetPreferencesVar("ActiveTimerPlan", rnOldvalue);
				rnvalue = atoi(request::findValue(&req, "ActiveTimerPlan").c_str());
				if (rnOldvalue != rnvalue)
				{
					m_sql.UpdatePreferencesVar("ActiveTimerPlan", rnvalue);
					m_sql.m_ActiveTimerPlan = rnvalue;
					m_mainworker.m_scheduler.ReloadSchedules();
				}
				cntSettings++;

				rnOldvalue = 0;
				m_sql.GetPreferencesVar("NotificationSensorInterval", rnOldvalue);
				rnvalue = atoi(request::findValue(&req, "NotificationSensorInterval").c_str());
				if (rnOldvalue != rnvalue)
				{
					m_sql.UpdatePreferencesVar("NotificationSensorInterval", rnvalue);
					m_notifications.ReloadNotifications();
				}
				cntSettings++;

				rnOldvalue = 0;
				m_sql.GetPreferencesVar("NotificationSwitchInterval", rnOldvalue);
				rnvalue = atoi(request::findValue(&req, "NotificationSwitchInterval").c_str());
				if (rnOldvalue != rnvalue)
				{
					m_sql.UpdatePreferencesVar("NotificationSwitchInterval", rnvalue);
					m_notifications.ReloadNotifications();
				}
				cntSettings++;

				rnOldvalue = 0;
				m_sql.GetPreferencesVar("EnableEventScriptSystem", rnOldvalue);
				std::string EnableEventScriptSystem = request::findValue(&req, "EnableEventScriptSystem");
				int iEnableEventScriptSystem = (EnableEventScriptSystem == "on" ? 1 : 0);
				m_sql.UpdatePreferencesVar("EnableEventScriptSystem", iEnableEventScriptSystem);
				m_sql.m_bEnableEventSystem = (iEnableEventScriptSystem == 1);
				if (iEnableEventScriptSystem != rnOldvalue)
				{
					m_mainworker.m_eventsystem.SetEnabled(m_sql.m_bEnableEventSystem);
					m_mainworker.m_eventsystem.StartEventSystem();
				}
				cntSettings++;

				std::string EnableEventSystemFullURLLog = request::findValue(&req, "EventSystemLogFullURL");
				m_sql.m_bEnableEventSystemFullURLLog = EnableEventSystemFullURLLog == "on" ? true : false;
				m_sql.UpdatePreferencesVar("EventSystemLogFullURL", (int)m_sql.m_bEnableEventSystemFullURLLog);
				cntSettings++;

				rnOldvalue = 0;
				m_sql.GetPreferencesVar("DisableDzVentsSystem", rnOldvalue);
				std::string DisableDzVentsSystem = request::findValue(&req, "DisableDzVentsSystem");
				int iDisableDzVentsSystem = (DisableDzVentsSystem == "on" ? 0 : 1);
				m_sql.UpdatePreferencesVar("DisableDzVentsSystem", iDisableDzVentsSystem);
				m_sql.m_bDisableDzVentsSystem = (iDisableDzVentsSystem == 1);
				if (m_sql.m_bEnableEventSystem && !iDisableDzVentsSystem && iDisableDzVentsSystem != rnOldvalue)
				{
					m_mainworker.m_eventsystem.LoadEvents();
					m_mainworker.m_eventsystem.GetCurrentStates();
				}
				cntSettings++;
				m_sql.UpdatePreferencesVar("DzVentsLogLevel", atoi(request::findValue(&req, "DzVentsLogLevel").c_str()));
				cntSettings++;

				rnOldvalue = 0;
				m_sql.GetPreferencesVar("RemoteSharedPort", rnOldvalue);
				m_sql.UpdatePreferencesVar("RemoteSharedPort", atoi(request::findValue(&req, "RemoteSharedPort").c_str()));
				m_sql.GetPreferencesVar("RemoteSharedPort", rnvalue);

				if (rnvalue != rnOldvalue)
				{
					m_mainworker.m_sharedserver.StopServer();
					if (rnvalue != 0)
					{
						char szPort[100];
						sprintf(szPort, "%d", rnvalue);
						m_mainworker.m_sharedserver.StartServer("::", szPort);
						m_mainworker.LoadSharedUsers();
					}
				}
				cntSettings++;

				rnOldvalue = 0;
				m_sql.GetPreferencesVar("RandomTimerFrame", rnOldvalue);
				rnvalue = atoi(request::findValue(&req, "RandomSpread").c_str());
				if (rnOldvalue != rnvalue)
				{
					m_sql.UpdatePreferencesVar("RandomTimerFrame", rnvalue);
					m_mainworker.m_scheduler.ReloadSchedules();
				}
				cntSettings++;

				rnOldvalue = 0;
				int batterylowlevel = atoi(request::findValue(&req, "BatterLowLevel").c_str());
				if (batterylowlevel > 100)
					batterylowlevel = 100;
				m_sql.GetPreferencesVar("BatteryLowNotification", rnOldvalue);
				m_sql.UpdatePreferencesVar("BatteryLowNotification", batterylowlevel);
				if ((rnOldvalue != batterylowlevel) && (batterylowlevel != 0))
					m_sql.CheckBatteryLow();
				cntSettings++;

				/* Update the Theme */

				std::string SelectedTheme = request::findValue(&req, "Themes");
				m_sql.UpdatePreferencesVar("WebTheme", SelectedTheme);
				m_pWebEm->SetWebTheme(SelectedTheme);
				cntSettings++;

				//Update the Max kWh value
				rnvalue = 6000;
				if (m_sql.GetPreferencesVar("MaxElectricPower", rnvalue))
				{
					if (rnvalue < 1)
						rnvalue = 6000;
					m_sql.m_max_kwh_usage = rnvalue;
				}

				/* To wrap up everything */
				m_notifications.ConfigFromGetvars(req, true);
				m_notifications.LoadConfig();

#ifdef ENABLE_PYTHON
				// Signal plugins to update Settings dictionary
				PluginLoadConfig();
#endif
				root["status"] = "OK";
			}
			catch (const std::exception& e)
			{
				std::stringstream errmsg;
				errmsg << "Error occured during processing of POSTed settings (" << e.what() << ") after processing " << cntSettings << " settings!";
				root["errmsg"] = errmsg.str();
				_log.Log(LOG_ERROR, errmsg.str());
			}
			catch (...)
			{
				std::stringstream errmsg;
				errmsg << "Error occured during processing of POSTed settings after processing " << cntSettings << " settings!";
				root["errmsg"] = errmsg.str();
				_log.Log(LOG_ERROR, errmsg.str());
			}
			std::string msg = "Processed " + std::to_string(cntSettings) + " settings!";
			root["message"] = msg;
		}

		void CWebServer::Cmd_DeleteDevice(WebEmSession& session, const request& req, Json::Value& root)
		{
			if (session.rights != 2)
			{
				session.reply_status = reply::forbidden;
				return; // Only admin user allowed
			}

			std::string idx = CURLEncode::URLDecode(request::findValue(&req, "idx"));
			if (idx.empty())
				return;

			root["status"] = "OK";
			root["title"] = "DeleteDevice";
			m_sql.DeleteDevices(idx);
			m_mainworker.m_scheduler.ReloadSchedules();
		}

		void CWebServer::Cmd_AddScene(WebEmSession& session, const request& req, Json::Value& root)
		{
			if (session.rights != 2)
			{
				session.reply_status = reply::forbidden;
				return; // Only admin user allowed
			}

			std::string name = HTMLSanitizer::Sanitize(request::findValue(&req, "name"));
			name = HTMLSanitizer::Sanitize(name);
			if (name.empty())
			{
				root["status"] = "ERR";
				root["message"] = "No Scene Name specified!";
				return;
			}
			std::string stype = request::findValue(&req, "scenetype");
			if (stype.empty())
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
			m_sql.safe_query("INSERT INTO Scenes (Name,SceneType) VALUES ('%q',%d)", name.c_str(), atoi(stype.c_str()));
			if (m_sql.m_bEnableEventSystem)
			{
				m_mainworker.m_eventsystem.GetCurrentScenesGroups();
			}
		}

		void CWebServer::Cmd_DeleteScene(WebEmSession& session, const request& req, Json::Value& root)
		{
			if (session.rights != 2)
			{
				session.reply_status = reply::forbidden;
				return; // Only admin user allowed
			}

			std::string idx = CURLEncode::URLDecode(request::findValue(&req, "idx"));
			if (idx.empty())
				return;
			root["status"] = "OK";
			root["title"] = "DeleteScene";
			m_sql.DeleteScenes(idx);
		}

		void CWebServer::Cmd_UpdateScene(WebEmSession& session, const request& req, Json::Value& root)
		{
			if (session.rights != 2)
			{
				session.reply_status = reply::forbidden;
				return; // Only admin user allowed
			}

			std::string idx = request::findValue(&req, "idx");
			std::string name = HTMLSanitizer::Sanitize(request::findValue(&req, "name"));
			std::string description = HTMLSanitizer::Sanitize(request::findValue(&req, "description"));

			name = HTMLSanitizer::Sanitize(name);
			description = HTMLSanitizer::Sanitize(description);

			if ((idx.empty()) || (name.empty()))
				return;
			std::string stype = request::findValue(&req, "scenetype");
			if (stype.empty())
			{
				root["status"] = "ERR";
				root["message"] = "No Scene Type specified!";
				return;
			}
			std::string tmpstr = request::findValue(&req, "protected");
			int iProtected = (tmpstr == "true") ? 1 : 0;

			std::string onaction = base64_decode(request::findValue(&req, "onaction"));
			std::string offaction = base64_decode(request::findValue(&req, "offaction"));

			root["status"] = "OK";
			root["title"] = "UpdateScene";
			m_sql.safe_query("UPDATE Scenes SET Name='%q', Description='%q', SceneType=%d, Protected=%d, OnAction='%q', OffAction='%q' WHERE (ID == '%q')", name.c_str(),
				description.c_str(), atoi(stype.c_str()), iProtected, onaction.c_str(), offaction.c_str(), idx.c_str());
			uint64_t ullidx = std::stoull(idx);
			m_mainworker.m_eventsystem.WWWUpdateSingleState(ullidx, name, m_mainworker.m_eventsystem.REASON_SCENEGROUP);
		}

		// Helper function for sorting in Cmd_CustomLightIcons
		bool compareIconsByName(const http::server::CWebServer::_tCustomIcon& a, const http::server::CWebServer::_tCustomIcon& b)
		{
			return a.Title < b.Title;
		}

		void CWebServer::Cmd_CustomLightIcons(WebEmSession& session, const request& req, Json::Value& root)
		{
			int ii = 0;

			std::vector<_tCustomIcon> temp_custom_light_icons = m_custom_light_icons;
			// Sort by name
			std::sort(temp_custom_light_icons.begin(), temp_custom_light_icons.end(), compareIconsByName);

			root["title"] = "CustomLightIcons";
			for (const auto& icon : temp_custom_light_icons)
			{
				root["result"][ii]["idx"] = icon.idx;
				root["result"][ii]["imageSrc"] = icon.RootFile;
				root["result"][ii]["text"] = icon.Title;
				root["result"][ii]["description"] = icon.Description;
				ii++;
			}
			root["status"] = "OK";
		}

		void CWebServer::Cmd_GetPlans(WebEmSession& session, const request& req, Json::Value& root)
		{
			root["status"] = "OK";
			root["title"] = "getplans";

			std::string sDisplayHidden = request::findValue(&req, "displayhidden");
			bool bDisplayHidden = (sDisplayHidden == "1");

			std::vector<std::vector<std::string>> result, result2;
			result = m_sql.safe_query("SELECT ID, Name, [Order] FROM Plans ORDER BY [Order]");
			if (!result.empty())
			{
				int ii = 0;
				for (const auto& sd : result)
				{
					std::string Name = sd[1];
					bool bIsHidden = (Name[0] == '$');

					if ((bDisplayHidden) || (!bIsHidden))
					{
						root["result"][ii]["idx"] = sd[0];
						root["result"][ii]["Name"] = Name;
						root["result"][ii]["Order"] = sd[2];

						unsigned int totDevices = 0;

						result2 = m_sql.safe_query("SELECT COUNT(*) FROM DeviceToPlansMap WHERE (PlanID=='%q')", sd[0].c_str());
						if (!result2.empty())
						{
							totDevices = (unsigned int)atoi(result2[0][0].c_str());
						}
						root["result"][ii]["Devices"] = totDevices;

						ii++;
					}
				}
			}
		}

		void CWebServer::Cmd_GetFloorPlans(WebEmSession& session, const request& req, Json::Value& root)
		{
			root["status"] = "OK";
			root["title"] = "getfloorplans";

			std::vector<std::vector<std::string>> result, result2, result3;

			result = m_sql.safe_query("SELECT Key, nValue, sValue FROM Preferences WHERE Key LIKE 'Floorplan%%'");
			if (result.empty())
				return;

			for (const auto& sd : result)
			{
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

			result2 = m_sql.safe_query("SELECT ID, Name, ScaleFactor, [Order] FROM Floorplans ORDER BY [Order]");
			if (!result2.empty())
			{
				int ii = 0;
				for (const auto& sd : result2)
				{
					root["result"][ii]["idx"] = sd[0];
					root["result"][ii]["Name"] = sd[1];
					std::string ImageURL = "images/floorplans/plan?idx=" + sd[0];
					root["result"][ii]["Image"] = ImageURL;
					root["result"][ii]["ScaleFactor"] = sd[2];
					root["result"][ii]["Order"] = sd[3];

					unsigned int totPlans = 0;

					result3 = m_sql.safe_query("SELECT COUNT(*) FROM Plans WHERE (FloorplanID=='%q')", sd[0].c_str());
					if (!result3.empty())
					{
						totPlans = (unsigned int)atoi(result3[0][0].c_str());
					}
					root["result"][ii]["Plans"] = totPlans;

					ii++;
				}
			}
		}

		void CWebServer::Cmd_GetScenes(WebEmSession& session, const request& req, Json::Value& root)
		{
			root["status"] = "OK";
			root["title"] = "getscenes";
			root["AllowWidgetOrdering"] = m_sql.m_bAllowWidgetOrdering;

			std::string sDisplayHidden = request::findValue(&req, "displayhidden");
			bool bDisplayHidden = (sDisplayHidden == "1");

			std::string sLastUpdate = request::findValue(&req, "lastupdate");

			std::string rid = request::findValue(&req, "rid");

			time_t LastUpdate = 0;
			if (!sLastUpdate.empty())
			{
				std::stringstream sstr;
				sstr << sLastUpdate;
				sstr >> LastUpdate;
			}

			time_t now = mytime(nullptr);
			struct tm tm1;
			localtime_r(&now, &tm1);
			struct tm tLastUpdate;
			localtime_r(&now, &tLastUpdate);

			root["ActTime"] = static_cast<int>(now);

			std::vector<std::vector<std::string>> result, result2;
			std::string szQuery = "SELECT ID, Name, Activators, Favorite, nValue, SceneType, LastUpdate, Protected, OnAction, OffAction, Description FROM Scenes";
			if (!rid.empty())
				szQuery += " WHERE (ID == " + rid + ")";
			szQuery += " ORDER BY [Order]";
			result = m_sql.safe_query(szQuery.c_str());
			if (!result.empty())
			{
				int ii = 0;
				for (const auto& sd : result)
				{
					std::string sName = sd[1];
					if ((bDisplayHidden == false) && (sName[0] == '$'))
						continue;

					std::string sLastUpdate = sd[6];
					if (LastUpdate != 0)
					{
						time_t cLastUpdate;
						ParseSQLdatetime(cLastUpdate, tLastUpdate, sLastUpdate, tm1.tm_isdst);
						if (cLastUpdate <= LastUpdate)
							continue;
					}

					unsigned char nValue = atoi(sd[4].c_str());
					unsigned char scenetype = atoi(sd[5].c_str());
					int iProtected = atoi(sd[7].c_str());

					std::string onaction = base64_encode(sd[8]);
					std::string offaction = base64_encode(sd[9]);

					root["result"][ii]["idx"] = sd[0];
					root["result"][ii]["Name"] = sName;
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
					uint64_t camIDX = m_mainworker.m_cameras.IsDevSceneInCamera(1, sd[0]);
					root["result"][ii]["UsedByCamera"] = (camIDX != 0) ? true : false;
					if (camIDX != 0)
					{
						std::stringstream scidx;
						scidx << camIDX;
						root["result"][ii]["CameraIdx"] = scidx.str();
						root["result"][ii]["CameraAspect"] = m_mainworker.m_cameras.GetCameraAspectRatio(scidx.str());
					}
					ii++;
				}
			}
			if (!m_mainworker.m_LastSunriseSet.empty())
			{
				std::vector<std::string> strarray;
				StringSplit(m_mainworker.m_LastSunriseSet, ";", strarray);
				if (strarray.size() == 10)
				{
					char szTmp[100];
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
		}

		void CWebServer::Cmd_GetHardware(WebEmSession& session, const request& req, Json::Value& root)
		{
			root["title"] = "gethardware";

			std::vector<std::vector<std::string>> result;
			result = m_sql.safe_query("SELECT ID, Name, Enabled, Type, Address, Port, SerialPort, Username, Password, Extra, Mode1, Mode2, Mode3, Mode4, Mode5, Mode6, DataTimeout, "
				"LogLevel FROM Hardware ORDER BY ID ASC");
			if (!result.empty())
			{
				int ii = 0;
				for (const auto& sd : result)
				{
					_eHardwareTypes hType = (_eHardwareTypes)atoi(sd[3].c_str());
					if (hType == HTYPE_DomoticzInternal)
						continue;
					root["result"][ii]["idx"] = sd[0];
					root["result"][ii]["Name"] = sd[1];
					root["result"][ii]["Enabled"] = (sd[2] == "1") ? "true" : "false";
					root["result"][ii]["Type"] = hType;
					root["result"][ii]["Address"] = sd[4];
					root["result"][ii]["Port"] = atoi(sd[5].c_str());
					root["result"][ii]["SerialPort"] = sd[6];
					root["result"][ii]["Username"] = sd[7];
					root["result"][ii]["Password"] = sd[8];
					root["result"][ii]["Extra"] = sd[9];

					if (hType == HTYPE_PythonPlugin)
					{
						root["result"][ii]["Mode1"] = sd[10]; // Plugins can have non-numeric values in the Mode fields
						root["result"][ii]["Mode2"] = sd[11];
						root["result"][ii]["Mode3"] = sd[12];
						root["result"][ii]["Mode4"] = sd[13];
						root["result"][ii]["Mode5"] = sd[14];
						root["result"][ii]["Mode6"] = sd[15];
					}
					else
					{
						root["result"][ii]["Mode1"] = atoi(sd[10].c_str());
						root["result"][ii]["Mode2"] = atoi(sd[11].c_str());
						root["result"][ii]["Mode3"] = atoi(sd[12].c_str());
						root["result"][ii]["Mode4"] = atoi(sd[13].c_str());
						root["result"][ii]["Mode5"] = atoi(sd[14].c_str());
						root["result"][ii]["Mode6"] = atoi(sd[15].c_str());
					}
					root["result"][ii]["DataTimeout"] = atoi(sd[16].c_str());
					root["result"][ii]["LogLevel"] = atoi(sd[17].c_str());

					CDomoticzHardwareBase* pHardware = m_mainworker.GetHardware(atoi(sd[0].c_str()));
					if (pHardware != nullptr)
					{
						if ((pHardware->HwdType == HTYPE_RFXtrx315) || (pHardware->HwdType == HTYPE_RFXtrx433) || (pHardware->HwdType == HTYPE_RFXtrx868) ||
							(pHardware->HwdType == HTYPE_RFXLAN))
						{
							CRFXBase* pMyHardware = dynamic_cast<CRFXBase*>(pHardware);
							if (!pMyHardware->m_Version.empty())
								root["result"][ii]["version"] = pMyHardware->m_Version;
							else
								root["result"][ii]["version"] = sd[11];
							root["result"][ii]["noiselvl"] = pMyHardware->m_NoiseLevel;
						}
						else if ((pHardware->HwdType == HTYPE_MySensorsUSB) || (pHardware->HwdType == HTYPE_MySensorsTCP) || (pHardware->HwdType == HTYPE_MySensorsMQTT))
						{
							MySensorsBase* pMyHardware = dynamic_cast<MySensorsBase*>(pHardware);
							root["result"][ii]["version"] = pMyHardware->GetGatewayVersion();
						}
						else if ((pHardware->HwdType == HTYPE_OpenThermGateway) || (pHardware->HwdType == HTYPE_OpenThermGatewayTCP))
						{
							OTGWBase* pMyHardware = dynamic_cast<OTGWBase*>(pHardware);
							root["result"][ii]["version"] = pMyHardware->m_Version;
						}
						else if ((pHardware->HwdType == HTYPE_RFLINKUSB) || (pHardware->HwdType == HTYPE_RFLINKTCP))
						{
							CRFLinkBase* pMyHardware = dynamic_cast<CRFLinkBase*>(pHardware);
							root["result"][ii]["version"] = pMyHardware->m_Version;
						}
						else if (pHardware->HwdType == HTYPE_EnphaseAPI)
						{
							EnphaseAPI* pMyHardware = dynamic_cast<EnphaseAPI*>(pHardware);
							root["result"][ii]["version"] = pMyHardware->m_szSoftwareVersion;
						}
						else if (pHardware->HwdType == HTYPE_AlfenEveCharger)
						{
							AlfenEve* pMyHardware = dynamic_cast<AlfenEve*>(pHardware);
							root["result"][ii]["version"] = pMyHardware->m_szSoftwareVersion;
						}
					}
					ii++;
				}
			}
			root["status"] = "OK";
		}

		void CWebServer::Cmd_GetDevices(WebEmSession& session, const request& req, Json::Value& root)
		{
			std::string rfilter = request::findValue(&req, "filter");
			std::string order = request::findValue(&req, "order");
			std::string rused = request::findValue(&req, "used");
			std::string rid = request::findValue(&req, "rid");
			std::string planid = request::findValue(&req, "plan");
			std::string floorid = request::findValue(&req, "floor");
			std::string sDisplayHidden = request::findValue(&req, "displayhidden");
			std::string sFetchFavorites = request::findValue(&req, "favorite");
			std::string sDisplayDisabled = request::findValue(&req, "displaydisabled");
			bool bDisplayHidden = (sDisplayHidden == "1");
			bool bFetchFavorites = (sFetchFavorites == "1");

			int HideDisabledHardwareSensors = 0;
			m_sql.GetPreferencesVar("HideDisabledHardwareSensors", HideDisabledHardwareSensors);
			bool bDisabledDisabled = (HideDisabledHardwareSensors == 0);
			if (sDisplayDisabled == "1")
				bDisabledDisabled = true;

			std::string sLastUpdate = request::findValue(&req, "lastupdate");
			std::string hwidx = request::findValue(&req, "hwidx"); // OTO

			time_t LastUpdate = 0;
			if (!sLastUpdate.empty())
			{
				std::stringstream sstr;
				sstr << sLastUpdate;
				sstr >> LastUpdate;
			}

			root["status"] = "OK";
			root["title"] = "Devices";
			root["app_version"] = szAppVersion;
			GetJSonDevices(root, rused, rfilter, order, rid, planid, floorid, bDisplayHidden, bDisabledDisabled, bFetchFavorites, LastUpdate, session.username, hwidx);
		}

		void CWebServer::Cmd_GetUsers(WebEmSession& session, const request& req, Json::Value& root)
		{
			root["status"] = "ERR";
			root["title"] = "Users";

			if (session.rights != URIGHTS_ADMIN)
				return;

			std::vector<std::vector<std::string>> result;
			result = m_sql.safe_query("SELECT ID, Active, Username, Password, Rights, RemoteSharing, TabsEnabled FROM USERS ORDER BY ID ASC");
			if (!result.empty())
			{
				int ii = 0;
				for (const auto& sd : result)
				{
					root["result"][ii]["idx"] = sd[0];
					root["result"][ii]["Enabled"] = (sd[1] == "1") ? "true" : "false";
					root["result"][ii]["Username"] = base64_decode(sd[2]);
					root["result"][ii]["Password"] = sd[3];
					root["result"][ii]["Rights"] = atoi(sd[4].c_str());
					root["result"][ii]["RemoteSharing"] = atoi(sd[5].c_str());
					root["result"][ii]["TabsEnabled"] = atoi(sd[6].c_str());
					ii++;
				}
				root["status"] = "OK";
			}
		}

		void CWebServer::Cmd_GetApplications(WebEmSession & session, const request& req, Json::Value &root)
		{
			root["title"] = "GetApplications";
			if (session.rights < URIGHTS_ADMIN)
			{
				session.reply_status = reply::forbidden;
			}
			else
			{
				std::vector<std::vector<std::string>> result;
				result = m_sql.safe_query("SELECT ID, Active, Public, Applicationname, Secret, Pemfile, LastSeen FROM Applications ORDER BY ID ASC");
				if (!result.empty())
				{
					int ii = 0;
					for (const auto& sd : result)
					{
						root["result"][ii]["idx"] = sd[0];
						root["result"][ii]["Enabled"] = (sd[1] == "1") ? "true" : "false";
						root["result"][ii]["Public"] = (sd[2] == "1") ? "true" : "false";
						root["result"][ii]["Applicationname"] = sd[3];
						root["result"][ii]["Secret"] = sd[4];
						root["result"][ii]["Pemfile"] = sd[5];
						root["result"][ii]["LastSeen"] = sd[6];
						ii++;
					}
				}
				root["status"] = "OK";
			}
		}

		void CWebServer::Cmd_AddApplication(WebEmSession & session, const request& req, Json::Value &root)
		{
			root["title"] = "AddApplication";
			if (session.rights < URIGHTS_ADMIN)
			{
				session.reply_status = reply::forbidden;
			}
			else
			{
				std::string senabled = request::findValue(&req, "enabled");
				std::string spublic = request::findValue(&req, "public");
				std::string applicationname = request::findValue(&req, "applicationname");
				std::string secret = request::findValue(&req, "secret");
				std::string pemfile = request::findValue(&req, "pemfile");
				if (senabled.empty() || applicationname.empty() || spublic.empty())
				{
					session.reply_status = reply::bad_request;
					return;
				}
				if ((spublic != "true") && secret.empty())
				{
					root["statustext"] = "Secret's can only be empty for Public Clients!";
					return;
				}
				if ((spublic == "true") && pemfile.empty())
				{
					root["statustext"] = "A PEM file containing private and public key must be given for Public Clients!";
					return;
				}
				// Check for duplicate application name
				std::vector<std::vector<std::string>> result;
				result = m_sql.safe_query("SELECT ID FROM Applications WHERE (Applicationname == '%q')", applicationname.c_str());
				if (!result.empty())
				{
					root["statustext"] = "Duplicate Applicationname!";
					return;
				}

				// Insert the new application
				m_sql.safe_query("INSERT INTO Applications (Active, Public, Applicationname, Secret, Pemfile) VALUES (%d,%d,'%q','%q','%q')",
					(senabled == "true") ? 1 : 0, (spublic == "true") ? 1 : 0, applicationname.c_str(), secret.c_str(), pemfile.c_str());

				// Reload the applications (and users)
				LoadUsers();
				root["status"] = "OK";
			}
		}

		void CWebServer::Cmd_UpdateApplication(WebEmSession & session, const request& req, Json::Value &root)
		{
			root["title"] = "UpdateApplication";
			if (session.rights < URIGHTS_ADMIN)
			{
				session.reply_status = reply::forbidden;
			}
			else
			{
				std::string senabled = request::findValue(&req, "enabled");
				std::string spublic = request::findValue(&req, "public");
				std::string applicationname = request::findValue(&req, "applicationname");
				std::string secret = request::findValue(&req, "secret");
				std::string pemfile = request::findValue(&req, "pemfile");
				std::string idx = request::findValue(&req, "idx");
				if (idx.empty() || senabled.empty() || applicationname.empty() || spublic.empty())
				{
					session.reply_status = reply::bad_request;
					return;
				}
				if ((spublic != "true") && secret.empty())
				{
					root["statustext"] = "Secret's can only be empty for Public Clients!";
					return;
				}
				if ((spublic == "true") && pemfile.empty())
				{
					root["statustext"] = "A PEM file containing private and public key must be given for Public Clients!";
					return;
				}
				// Check for duplicate application name
				std::vector<std::vector<std::string>> result;
				result = m_sql.safe_query("SELECT ID FROM Applications WHERE (Applicationname == '%q')", applicationname.c_str());
				if (!result.empty())
				{
					std::string oidx = result[0][0];
					if (oidx != idx)
					{
						root["statustext"] = "Duplicate Applicationname!";
						return;
					}
				}

				// Update the application
				m_sql.safe_query("UPDATE Applications SET Active=%d, Public=%d, Applicationname='%q', Secret='%q', Pemfile='%q' WHERE (ID == '%q')",
					(senabled == "true") ? 1 : 0, (spublic == "true") ? 1 : 0, applicationname.c_str(), secret.c_str(), pemfile.c_str(), idx.c_str());

				// Reload the applications (and users)
				LoadUsers();
				root["status"] = "OK";
			}
		}

		void CWebServer::Cmd_DeleteApplication(WebEmSession & session, const request& req, Json::Value &root)
		{
			root["title"] = "DeleteApplication";
			if (session.rights < URIGHTS_ADMIN)
			{
				session.reply_status = reply::forbidden;
			}
			else
			{
				std::string idx = request::findValue(&req, "idx");
				if (idx.empty())
				{
					session.reply_status = reply::bad_request;
					return;
				}

				// Remove Application
				std::vector<std::vector<std::string>> result;
				result = m_sql.safe_query("SELECT ID FROM Applications WHERE (ID == '%q')", idx.c_str());
				if (result.size() != 1)
				{
					session.reply_status = reply::bad_request;
					return;
				}
				m_sql.safe_query("DELETE FROM Applications WHERE (ID == '%q')", idx.c_str());

				// Reload the applications (and users)
				LoadUsers();
				root["status"] = "OK";
			}
		}

		void CWebServer::Cmd_GetMobiles(WebEmSession& session, const request& req, Json::Value& root)
		{
			root["status"] = "ERR";
			root["title"] = "Mobiles";

			if (session.rights != URIGHTS_ADMIN)
				return;

			std::vector<std::vector<std::string>> result;
			result = m_sql.safe_query("SELECT ID, Active, Name, UUID, LastUpdate, DeviceType FROM MobileDevices ORDER BY Name COLLATE NOCASE ASC");
			if (!result.empty())
			{
				int ii = 0;
				for (const auto& sd : result)
				{
					root["result"][ii]["idx"] = sd[0];
					root["result"][ii]["Enabled"] = (sd[1] == "1") ? "true" : "false";
					root["result"][ii]["Name"] = sd[2];
					root["result"][ii]["UUID"] = sd[3];
					root["result"][ii]["LastUpdate"] = sd[4];
					root["result"][ii]["DeviceType"] = sd[5];
					ii++;
				}
			}
			root["status"] = "OK";
		}

		void CWebServer::Cmd_SetSetpoint(WebEmSession& session, const request& req, Json::Value& root)
		{
			bool bHaveUser = (!session.username.empty());
			int iUser = -1;
			int urights = 3;
			if (bHaveUser)
			{
				iUser = FindUser(session.username.c_str());
				if (iUser != -1)
				{
					urights = static_cast<int>(m_users[iUser].userrights);
				}
			}
			if (urights < 1)
				return;

			std::string idx = request::findValue(&req, "idx");
			std::string setpoint = request::findValue(&req, "setpoint");
			if ((idx.empty()) || (setpoint.empty()))
				return;
			root["status"] = "OK";
			root["title"] = "SetSetpoint";
			if (iUser != -1)
			{
				_log.Log(LOG_STATUS, "User: %s initiated a SetPoint command", m_users[iUser].Username.c_str());
			}
			m_mainworker.SetSetPoint(idx, static_cast<float>(atof(setpoint.c_str())));
		}

		void CWebServer::Cmd_GetSceneActivations(WebEmSession& session, const request& req, Json::Value& root)
		{
			if (session.rights != 2)
			{
				session.reply_status = reply::forbidden;
				return; // Only admin user allowed
			}

			std::string idx = request::findValue(&req, "idx");
			if (idx.empty())
				return;

			root["status"] = "OK";
			root["title"] = "GetSceneActivations";

			std::vector<std::vector<std::string>> result, result2;
			result = m_sql.safe_query("SELECT Activators, SceneType FROM Scenes WHERE (ID==%q)", idx.c_str());
			if (result.empty())
				return;
			int ii = 0;
			std::string Activators = result[0][0];
			int SceneType = atoi(result[0][1].c_str());
			if (!Activators.empty())
			{
				// Get Activator device names
				std::vector<std::string> arrayActivators;
				StringSplit(Activators, ";", arrayActivators);
				for (const auto& sCodeCmd : arrayActivators)
				{
					std::vector<std::string> arrayCode;
					StringSplit(sCodeCmd, ":", arrayCode);

					std::string sID = arrayCode[0];
					int sCode = 0;
					if (arrayCode.size() == 2)
					{
						sCode = atoi(arrayCode[1].c_str());
					}

					result2 = m_sql.safe_query("SELECT Name, [Type], SubType, SwitchType FROM DeviceStatus WHERE (ID==%q)", sID.c_str());
					if (!result2.empty())
					{
						std::vector<std::string> sd = result2[0];
						std::string lstatus = "-";
						if ((SceneType == 0) && (arrayCode.size() == 2))
						{
							unsigned char devType = (unsigned char)atoi(sd[1].c_str());
							unsigned char subType = (unsigned char)atoi(sd[2].c_str());
							_eSwitchType switchtype = (_eSwitchType)atoi(sd[3].c_str());
							int nValue = sCode;
							std::string sValue;
							int llevel = 0;
							bool bHaveDimmer = false;
							bool bHaveGroupCmd = false;
							int maxDimLevel = 0;
							GetLightStatus(devType, subType, switchtype, nValue, sValue, lstatus, llevel, bHaveDimmer, maxDimLevel, bHaveGroupCmd);
						}
						uint64_t dID = std::stoull(sID);
						root["result"][ii]["idx"] = Json::Value::UInt64(dID);
						root["result"][ii]["name"] = sd[0];
						root["result"][ii]["code"] = sCode;
						root["result"][ii]["codestr"] = lstatus;
						ii++;
					}
				}
			}
		}

		void CWebServer::Cmd_AddSceneCode(WebEmSession& session, const request& req, Json::Value& root)
		{
			if (session.rights != 2)
			{
				session.reply_status = reply::forbidden;
				return; // Only admin user allowed
			}

			std::string sceneidx = request::findValue(&req, "sceneidx");
			std::string idx = request::findValue(&req, "idx");
			std::string cmnd = request::findValue(&req, "cmnd");
			if ((sceneidx.empty()) || (idx.empty()) || (cmnd.empty()))
				return;
			root["status"] = "OK";
			root["title"] = "AddSceneCode";

			// First check if we do not already have this device as activation code
			std::vector<std::vector<std::string>> result;
			result = m_sql.safe_query("SELECT Activators, SceneType FROM Scenes WHERE (ID==%q)", sceneidx.c_str());
			if (result.empty())
				return;
			std::string Activators = result[0][0];
			unsigned char scenetype = atoi(result[0][1].c_str());

			if (!Activators.empty())
			{
				// Get Activator device names
				std::vector<std::string> arrayActivators;
				StringSplit(Activators, ";", arrayActivators);
				for (const auto& sCodeCmd : arrayActivators)
				{
					std::vector<std::string> arrayCode;
					StringSplit(sCodeCmd, ":", arrayCode);

					std::string sID = arrayCode[0];
					std::string sCode;
					if (arrayCode.size() == 2)
					{
						sCode = arrayCode[1];
					}

					if (sID == idx)
					{
						if (scenetype == 1)
							return; // Group does not work with separate codes, so already there
						if (sCode == cmnd)
							return; // same code, already there!
					}
				}
			}
			if (!Activators.empty())
				Activators += ";";
			Activators += idx;
			if (scenetype == 0)
			{
				Activators += ":" + cmnd;
			}
			m_sql.safe_query("UPDATE Scenes SET Activators='%q' WHERE (ID==%q)", Activators.c_str(), sceneidx.c_str());
		}

		void CWebServer::Cmd_RemoveSceneCode(WebEmSession& session, const request& req, Json::Value& root)
		{
			if (session.rights != 2)
			{
				session.reply_status = reply::forbidden;
				return; // Only admin user allowed
			}

			std::string sceneidx = request::findValue(&req, "sceneidx");
			std::string idx = request::findValue(&req, "idx");
			std::string code = request::findValue(&req, "code");
			if ((idx.empty()) || (sceneidx.empty()) || (code.empty()))
				return;
			root["status"] = "OK";
			root["title"] = "RemoveSceneCode";

			std::vector<std::vector<std::string>> result;
			result = m_sql.safe_query("SELECT Activators, SceneType FROM Scenes WHERE (ID==%q)", sceneidx.c_str());
			if (result.empty())
				return;
			std::string Activators = result[0][0];
			int SceneType = atoi(result[0][1].c_str());
			if (!Activators.empty())
			{
				// Get Activator device names
				std::vector<std::string> arrayActivators;
				StringSplit(Activators, ";", arrayActivators);
				std::string newActivation;
				for (const auto& sCodeCmd : arrayActivators)
				{
					std::vector<std::string> arrayCode;
					StringSplit(sCodeCmd, ":", arrayCode);

					std::string sID = arrayCode[0];
					std::string sCode;
					if (arrayCode.size() == 2)
					{
						sCode = arrayCode[1];
					}
					bool bFound = false;
					if (sID == idx)
					{
						if ((SceneType == 1) || (sCode.empty()))
						{
							bFound = true;
						}
						else
						{
							// Also check the code
							bFound = (sCode == code);
						}
					}
					if (!bFound)
					{
						if (!newActivation.empty())
							newActivation += ";";
						newActivation += sID;
						if ((SceneType == 0) && (!sCode.empty()))
						{
							newActivation += ":" + sCode;
						}
					}
				}
				if (Activators != newActivation)
				{
					m_sql.safe_query("UPDATE Scenes SET Activators='%q' WHERE (ID==%q)", newActivation.c_str(), sceneidx.c_str());
				}
			}
		}

		void CWebServer::Cmd_ClearSceneCodes(WebEmSession& session, const request& req, Json::Value& root)
		{
			if (session.rights != 2)
			{
				session.reply_status = reply::forbidden;
				return; // Only admin user allowed
			}

			std::string sceneidx = request::findValue(&req, "sceneidx");
			if (sceneidx.empty())
				return;
			root["status"] = "OK";
			root["title"] = "ClearSceneCode";

			m_sql.safe_query("UPDATE Scenes SET Activators='' WHERE (ID==%q)", sceneidx.c_str());
		}

		void CWebServer::Cmd_GetSerialDevices(WebEmSession& session, const request& req, Json::Value& root)
		{
			root["status"] = "OK";
			root["title"] = "GetSerialDevices";

			bool bUseDirectPath = false;
			std::vector<std::string> serialports = GetSerialPorts(bUseDirectPath);
			int ii = 0;
			for (const auto& port : serialports)
			{
				root["result"][ii]["name"] = port;
				root["result"][ii]["value"] = ii;
				ii++;
			}
		}

		void CWebServer::Cmd_GetDevicesList(WebEmSession& session, const request& req, Json::Value& root)
		{
			root["status"] = "OK";
			root["title"] = "GetDevicesList";
			int ii = 0;
			std::vector<std::vector<std::string>> result;
			result = m_sql.safe_query("SELECT ID, Name, Type, SubType FROM DeviceStatus WHERE (Used == 1) ORDER BY Name COLLATE NOCASE ASC");
			if (!result.empty())
			{
				for (const auto& sd : result)
				{
					root["result"][ii]["idx"] = sd[0];
					root["result"][ii]["name"] = sd[1];
					root["result"][ii]["name_type"] = std_format("%s (%s/%s)",
						sd[1].c_str(),
						RFX_Type_Desc(std::stoi(sd[2]), 1),
						RFX_Type_SubType_Desc(std::stoi(sd[2]), std::stoi(sd[3]))
					);
					//root["result"][ii]["Type"] = RFX_Type_Desc(std::stoi(sd[2]), 1);
					//root["result"][ii]["SubType"] = RFX_Type_SubType_Desc(std::stoi(sd[2]), std::stoi(sd[3]));
					ii++;
				}
			}
		}

		void CWebServer::Cmd_UploadCustomIcon(WebEmSession& session, const request& req, Json::Value& root)
		{
			root["title"] = "UploadCustomIcon";
			// Only admin user allowed
			if (session.rights != 2)
			{
				session.reply_status = reply::forbidden;
				return; // Only admin user allowed
			}
			std::string zipfile = request::findValue(&req, "file");
			if (!zipfile.empty())
			{
				std::string ErrorMessage;
				bool bOK = m_sql.InsertCustomIconFromZip(zipfile, ErrorMessage);
				if (bOK)
				{
					root["status"] = "OK";
				}
				else
				{
					root["error"] = ErrorMessage;
				}
			}
		}

		void CWebServer::Cmd_GetCustomIconSet(WebEmSession& session, const request& req, Json::Value& root)
		{
			root["status"] = "OK";
			root["title"] = "GetCustomIconSet";
			int ii = 0;
			for (const auto& icon : m_custom_light_icons)
			{
				if (icon.idx >= 100)
				{
					std::string IconFile16 = "images/" + icon.RootFile + ".png";
					std::string IconFile48On = "images/" + icon.RootFile + "48_On.png";
					std::string IconFile48Off = "images/" + icon.RootFile + "48_Off.png";

					root["result"][ii]["idx"] = icon.idx - 100;
					root["result"][ii]["Title"] = icon.Title;
					root["result"][ii]["Description"] = icon.Description;
					root["result"][ii]["IconFile16"] = IconFile16;
					root["result"][ii]["IconFile48On"] = IconFile48On;
					root["result"][ii]["IconFile48Off"] = IconFile48Off;
					ii++;
				}
			}
		}

		void CWebServer::Cmd_DeleteCustomIcon(WebEmSession& session, const request& req, Json::Value& root)
		{
			if (session.rights != 2)
			{
				session.reply_status = reply::forbidden;
				return; // Only admin user allowed
			}

			std::string sidx = request::findValue(&req, "idx");
			if (sidx.empty())
				return;
			int idx = atoi(sidx.c_str());
			root["status"] = "OK";
			root["title"] = "DeleteCustomIcon";

			m_sql.safe_query("DELETE FROM CustomImages WHERE (ID == %d)", idx);

			// Delete icons file from disk
			for (const auto& icon : m_custom_light_icons)
			{
				if (icon.idx == idx + 100)
				{
					std::string IconFile16 = szWWWFolder + "/images/" + icon.RootFile + ".png";
					std::string IconFile48On = szWWWFolder + "/images/" + icon.RootFile + "48_On.png";
					std::string IconFile48Off = szWWWFolder + "/images/" + icon.RootFile + "48_Off.png";
					std::remove(IconFile16.c_str());
					std::remove(IconFile48On.c_str());
					std::remove(IconFile48Off.c_str());
					break;
				}
			}
			ReloadCustomSwitchIcons();
		}

		void CWebServer::Cmd_UpdateCustomIcon(WebEmSession& session, const request& req, Json::Value& root)
		{
			if (session.rights != 2)
			{
				session.reply_status = reply::forbidden;
				return; // Only admin user allowed
			}

			std::string sidx = request::findValue(&req, "idx");
			std::string sname = HTMLSanitizer::Sanitize(request::findValue(&req, "name"));
			std::string sdescription = HTMLSanitizer::Sanitize(request::findValue(&req, "description"));
			if ((sidx.empty()) || (sname.empty()) || (sdescription.empty()))
				return;

			int idx = atoi(sidx.c_str());
			root["status"] = "OK";
			root["title"] = "UpdateCustomIcon";

			m_sql.safe_query("UPDATE CustomImages SET Name='%q', Description='%q' WHERE (ID == %d)", sname.c_str(), sdescription.c_str(), idx);
			ReloadCustomSwitchIcons();
		}

		void CWebServer::Cmd_RenameDevice(WebEmSession& session, const request& req, Json::Value& root)
		{
			if (session.rights != 2)
			{
				session.reply_status = reply::forbidden;
				return; // Only admin user allowed
			}

			std::string sidx = request::findValue(&req, "idx");
			std::string sname = HTMLSanitizer::Sanitize(request::findValue(&req, "name"));
			if ((sidx.empty()) || (sname.empty()))
				return;
			int idx = atoi(sidx.c_str());
			root["status"] = "OK";
			root["title"] = "RenameDevice";

			m_sql.safe_query("UPDATE DeviceStatus SET Name='%q' WHERE (ID == %d)", sname.c_str(), idx);
			uint64_t ullidx = std::stoull(sidx);
			m_mainworker.m_eventsystem.WWWUpdateSingleState(ullidx, sname, m_mainworker.m_eventsystem.REASON_DEVICE);

#ifdef ENABLE_PYTHON
			// Notify plugin framework about the change
			m_mainworker.m_pluginsystem.DeviceModified(idx);
#endif
		}

		void CWebServer::Cmd_RenameScene(WebEmSession& session, const request& req, Json::Value& root)
		{
			if (session.rights != 2)
			{
				session.reply_status = reply::forbidden;
				return; // Only admin user allowed
			}

			std::string sidx = request::findValue(&req, "idx");
			std::string sname = HTMLSanitizer::Sanitize(request::findValue(&req, "name"));
			if ((sidx.empty()) || (sname.empty()))
				return;
			int idx = atoi(sidx.c_str());
			root["status"] = "OK";
			root["title"] = "RenameScene";

			m_sql.safe_query("UPDATE Scenes SET Name='%q' WHERE (ID == %d)", sname.c_str(), idx);
			uint64_t ullidx = std::stoull(sidx);
			m_mainworker.m_eventsystem.WWWUpdateSingleState(ullidx, sname, m_mainworker.m_eventsystem.REASON_SCENEGROUP);
		}

		void CWebServer::Cmd_SetDeviceUsed(WebEmSession& session, const request& req, Json::Value& root)
		{
			if (session.rights != 2)
			{
				session.reply_status = reply::forbidden;
				return; // Only admin user allowed
			}

			std::string sIdx = request::findValue(&req, "idx");
			std::string sUsed = request::findValue(&req, "used");
			std::string sName = request::findValue(&req, "name");
			std::string sMainDeviceIdx = request::findValue(&req, "maindeviceidx");
			if (sIdx.empty() || sUsed.empty())
				return;
			const int idx = atoi(sIdx.c_str());
			bool bIsUsed = (sUsed == "true");

			if (!sName.empty())
				m_sql.safe_query("UPDATE DeviceStatus SET Used=%d, Name='%q' WHERE (ID == %d)", bIsUsed ? 1 : 0, sName.c_str(), idx);
			else
				m_sql.safe_query("UPDATE DeviceStatus SET Used=%d WHERE (ID == %d)", bIsUsed ? 1 : 0, idx);

			root["status"] = "OK";
			root["title"] = "SetDeviceUsed";

			if ((!sMainDeviceIdx.empty()) && (sMainDeviceIdx != sIdx))
			{
				// this is a sub device for another light/switch
				// first check if it is not already a sub device
				auto result = m_sql.safe_query("SELECT ID FROM LightSubDevices WHERE (DeviceRowID=='%q') AND (ParentID =='%q')", sIdx.c_str(), sMainDeviceIdx.c_str());
				if (result.empty())
				{
					// no it is not, add it
					m_sql.safe_query("INSERT INTO LightSubDevices (DeviceRowID, ParentID) VALUES ('%q','%q')", sIdx.c_str(), sMainDeviceIdx.c_str());
				}
			}

			if (m_sql.m_bEnableEventSystem)
			{
				if (!bIsUsed)
					m_mainworker.m_eventsystem.RemoveSingleState(idx, m_mainworker.m_eventsystem.REASON_DEVICE);
				else
					m_mainworker.m_eventsystem.WWWUpdateSingleState(idx, sName, m_mainworker.m_eventsystem.REASON_DEVICE);
			}
#ifdef ENABLE_PYTHON
			// Notify plugin framework about the change
			m_mainworker.m_pluginsystem.DeviceModified(idx);
#endif
		}

		void CWebServer::Cmd_AddLogMessage(WebEmSession& session, const request& req, Json::Value& root)
		{
			std::string smessage = request::findValue(&req, "message");
			if (smessage.empty())
				return;
			root["title"] = "AddLogMessage";

			_eLogLevel logLevel = LOG_STATUS;
			std::string slevel = request::findValue(&req, "level");
			if (!slevel.empty())
			{
				if ((slevel == "1") || (slevel == "normal"))
					logLevel = LOG_NORM;
				else if ((slevel == "2") || (slevel == "status"))
					logLevel = LOG_STATUS;
				else if ((slevel == "4") || (slevel == "error"))
					logLevel = LOG_ERROR;
				else
				{
					root["status"] = "ERR";
					return;
				}
			}
			root["status"] = "OK";

			_log.Log(logLevel, "%s", smessage.c_str());
		}

		void CWebServer::Cmd_ClearShortLog(WebEmSession& session, const request& req, Json::Value& root)
		{
			if (session.rights != 2)
			{
				session.reply_status = reply::forbidden;
				return; // Only admin user allowed
			}
			root["status"] = "OK";
			root["title"] = "ClearShortLog";

			_log.Log(LOG_STATUS, "Clearing Short Log...");

			m_sql.ClearShortLog();

			_log.Log(LOG_STATUS, "Short Log Cleared!");
		}

		void CWebServer::Cmd_VacuumDatabase(WebEmSession& session, const request& req, Json::Value& root)
		{
			if (session.rights != 2)
			{
				session.reply_status = reply::forbidden;
				return; // Only admin user allowed
			}
			root["status"] = "OK";
			root["title"] = "VacuumDatabase";

			m_sql.VacuumDatabase();
		}

		void CWebServer::Cmd_AddMobileDevice(WebEmSession& session, const request& req, Json::Value& root)
		{
			std::string suuid = HTMLSanitizer::Sanitize(request::findValue(&req, "uuid"));
			std::string ssenderid = HTMLSanitizer::Sanitize(request::findValue(&req, "senderid"));
			std::string sname = HTMLSanitizer::Sanitize(request::findValue(&req, "name"));
			std::string sdevtype = HTMLSanitizer::Sanitize(request::findValue(&req, "devicetype"));
			std::string sactive = request::findValue(&req, "active");
			if ((suuid.empty()) || (ssenderid.empty()))
				return;
			root["status"] = "OK";
			root["title"] = "AddMobileDevice";

			if (sactive.empty())
				sactive = "1";
			int iActive = (sactive == "1") ? 1 : 0;

			std::vector<std::vector<std::string>> result;
			result = m_sql.safe_query("SELECT ID, Name, DeviceType FROM MobileDevices WHERE (UUID=='%q')", suuid.c_str());
			if (result.empty())
			{
				// New
				m_sql.safe_query("INSERT INTO MobileDevices (Active,UUID,SenderID,Name,DeviceType) VALUES (%d,'%q','%q','%q','%q')", iActive, suuid.c_str(), ssenderid.c_str(),
					sname.c_str(), sdevtype.c_str());
			}
			else
			{
				// Update
				std::string sLastUpdate = TimeToString(nullptr, TF_DateTime);
				m_sql.safe_query("UPDATE MobileDevices SET Active=%d, SenderID='%q', LastUpdate='%q' WHERE (UUID == '%q')", iActive, ssenderid.c_str(),
					sLastUpdate.c_str(), suuid.c_str());

				std::string dname = result[0][1];
				std::string ddevtype = result[0][2];
				if (dname.empty() || ddevtype.empty())
				{
					m_sql.safe_query("UPDATE MobileDevices SET Name='%q', DeviceType='%q' WHERE (UUID == '%q')", sname.c_str(), sdevtype.c_str(), suuid.c_str());
				}
			}
		}

		void CWebServer::Cmd_UpdateMobileDevice(WebEmSession& session, const request& req, Json::Value& root)
		{
			if (session.rights != 2)
			{
				session.reply_status = reply::forbidden;
				return; // Only admin user allowed
			}
			std::string sidx = request::findValue(&req, "idx");
			std::string enabled = request::findValue(&req, "enabled");
			std::string name = HTMLSanitizer::Sanitize(request::findValue(&req, "name"));

			if ((sidx.empty()) || (enabled.empty()) || (name.empty()))
				return;
			uint64_t idx = std::stoull(sidx);

			m_sql.safe_query("UPDATE MobileDevices SET Name='%q', Active=%d WHERE (ID==%" PRIu64 ")", name.c_str(), (enabled == "true") ? 1 : 0, idx);

			root["status"] = "OK";
			root["title"] = "UpdateMobile";
		}

		void CWebServer::Cmd_DeleteMobileDevice(WebEmSession& session, const request& req, Json::Value& root)
		{
			if (session.rights != 2)
			{
				session.reply_status = reply::forbidden;
				return; // Only admin user allowed
			}
			std::string suuid = request::findValue(&req, "uuid");
			if (suuid.empty())
				return;
			std::vector<std::vector<std::string>> result;
			result = m_sql.safe_query("SELECT ID FROM MobileDevices WHERE (UUID=='%q')", suuid.c_str());
			if (result.empty())
				return;
			m_sql.safe_query("DELETE FROM MobileDevices WHERE (UUID == '%q')", suuid.c_str());
			root["status"] = "OK";
			root["title"] = "DeleteMobileDevice";
		}

		void CWebServer::Cmd_GetTransfers(WebEmSession& session, const request& req, Json::Value& root)
		{
			root["status"] = "OK";
			root["title"] = "GetTransfers";

			uint64_t idx = 0;
			if (!request::findValue(&req, "idx").empty())
			{
				idx = std::stoull(request::findValue(&req, "idx"));
			}

			std::vector<std::vector<std::string>> result;
			result = m_sql.safe_query("SELECT Type, SubType FROM DeviceStatus WHERE (ID==%" PRIu64 ")", idx);
			if (!result.empty())
			{
				int dType = atoi(result[0][0].c_str());
				if ((dType == pTypeTEMP) || (dType == pTypeTEMP_HUM) || (dType == pTypeTEMP_HUM_BARO))
				{
					//Allow old Temp or Temp+Hum or Temp+Hum+Baro devices to be replaced by new Temp or Temp+Hum or Temp+Hum+Baro
					result = m_sql.safe_query("SELECT ID, Name, Type FROM DeviceStatus WHERE (Type=='%d') || (Type=='%d') || (Type=='%d') AND (ID!=%" PRIu64 ")", pTypeTEMP, pTypeTEMP_HUM, pTypeTEMP_HUM_BARO, idx);
				}
				else
				{
					result = m_sql.safe_query("SELECT ID, Name FROM DeviceStatus WHERE (Type=='%q') AND (SubType=='%q') AND (ID!=%" PRIu64 ")", result[0][0].c_str(),
						result[0][1].c_str(), idx);
				}

				std::sort(std::begin(result), std::end(result), [](std::vector<std::string> a, std::vector<std::string> b) { return a[1] < b[1]; });

				int ii = 0;
				for (const auto& sd : result)
				{
					root["result"][ii]["idx"] = sd[0];
					root["result"][ii]["Name"] = sd[1];
					ii++;
				}
			}
		}

		// Will transfer Newest sensor log to OLD sensor,
		// then set the HardwareID/DeviceID/Unit/Name/Type/Subtype/Unit for the OLD sensor to the NEW sensor ID/Type/Subtype/Unit
		// then delete the NEW sensor
		void CWebServer::Cmd_DoTransferDevice(WebEmSession& session, const request& req, Json::Value& root)
		{
			std::string sidx = request::findValue(&req, "idx");
			if (sidx.empty())
				return;

			std::string newidx = request::findValue(&req, "newidx");
			if (newidx.empty())
				return;

			std::vector<std::vector<std::string>> result;

			root["status"] = "OK";
			root["title"] = "DoTransferDevice";

			result = m_sql.safe_query("SELECT HardwareID, DeviceID, Unit, Type, SubType FROM DeviceStatus WHERE (ID == '%q')", newidx.c_str());
			if (result.empty())
				return;

			int newHardwareID = std::stoi(result[0].at(0));
			std::string newDeviceID = result[0].at(1);
			int newUnit = std::stoi(result[0].at(2));
			int devType = std::stoi(result[0].at(3));
			int subType = std::stoi(result[0].at(4));

			//get last update date from old device
			result = m_sql.safe_query("SELECT LastUpdate FROM DeviceStatus WHERE (ID == '%q')", sidx.c_str());
			if (result.empty())
				return;
			std::string szLastOldDate = result[0][0];

			m_sql.safe_query("UPDATE DeviceStatus SET HardwareID = %d, DeviceID = '%q', Unit = %d, Type = %d, SubType = %d WHERE ID == '%q'", newHardwareID, newDeviceID.c_str(), newUnit, devType, subType, sidx.c_str());

			//new device could already have some logging, so let's keep this data
			//Rain
			m_sql.safe_query("UPDATE Rain SET DeviceRowID='%q' WHERE (DeviceRowID == '%q') AND (Date>'%q')", sidx.c_str(), newidx.c_str(), szLastOldDate.c_str());
			m_sql.safe_query("UPDATE Rain_Calendar SET DeviceRowID='%q' WHERE (DeviceRowID == '%q') AND (Date>'%q')", sidx.c_str(), newidx.c_str(), szLastOldDate.c_str());

			//Temperature
			m_sql.safe_query("UPDATE Temperature SET DeviceRowID='%q' WHERE (DeviceRowID == '%q') AND (Date>'%q')", sidx.c_str(), newidx.c_str(), szLastOldDate.c_str());
			m_sql.safe_query("UPDATE Temperature_Calendar SET DeviceRowID='%q' WHERE (DeviceRowID == '%q') AND (Date>'%q')", sidx.c_str(), newidx.c_str(), szLastOldDate.c_str());

			//UV
			m_sql.safe_query("UPDATE UV SET DeviceRowID='%q' WHERE (DeviceRowID == '%q') AND (Date>'%q')", sidx.c_str(), newidx.c_str(), szLastOldDate.c_str());
			m_sql.safe_query("UPDATE UV_Calendar SET DeviceRowID='%q' WHERE (DeviceRowID == '%q') AND (Date>'%q')", sidx.c_str(), newidx.c_str(), szLastOldDate.c_str());

			//Wind
			m_sql.safe_query("UPDATE Wind SET DeviceRowID='%q' WHERE (DeviceRowID == '%q') AND (Date>'%q')", sidx.c_str(), newidx.c_str(), szLastOldDate.c_str());
			m_sql.safe_query("UPDATE Wind_Calendar SET DeviceRowID='%q' WHERE (DeviceRowID == '%q') AND (Date>'%q')", sidx.c_str(), newidx.c_str(), szLastOldDate.c_str());

			//Meter
			m_sql.safe_query("UPDATE Meter SET DeviceRowID='%q' WHERE (DeviceRowID == '%q') AND (Date>'%q')", sidx.c_str(), newidx.c_str(), szLastOldDate.c_str());
			m_sql.safe_query("UPDATE Meter_Calendar SET DeviceRowID='%q' WHERE (DeviceRowID == '%q') AND (Date>'%q')", sidx.c_str(), newidx.c_str(), szLastOldDate.c_str());

			//Multimeter
			m_sql.safe_query("UPDATE MultiMeter SET DeviceRowID='%q' WHERE (DeviceRowID == '%q') AND (Date>'%q')", sidx.c_str(), newidx.c_str(), szLastOldDate.c_str());
			m_sql.safe_query("UPDATE MultiMeter_Calendar SET DeviceRowID='%q' WHERE (DeviceRowID == '%q') AND (Date>'%q')", sidx.c_str(), newidx.c_str(), szLastOldDate.c_str());

			//Fan
			m_sql.safe_query("UPDATE Fan SET DeviceRowID='%q' WHERE (DeviceRowID == '%q') AND (Date>'%q')", sidx.c_str(), newidx.c_str(), szLastOldDate.c_str());
			m_sql.safe_query("UPDATE Fan_Calendar SET DeviceRowID='%q' WHERE (DeviceRowID == '%q') AND (Date>'%q')", sidx.c_str(), newidx.c_str(), szLastOldDate.c_str());

			//Percentage
			m_sql.safe_query("UPDATE Percentage SET DeviceRowID='%q' WHERE (DeviceRowID == '%q') AND (Date>'%q')", sidx.c_str(), newidx.c_str(), szLastOldDate.c_str());
			m_sql.safe_query("UPDATE Percentage_Calendar SET DeviceRowID='%q' WHERE (DeviceRowID == '%q') AND (Date>'%q')", sidx.c_str(), newidx.c_str(), szLastOldDate.c_str());

			m_sql.DeleteDevices(newidx);

			m_mainworker.m_scheduler.ReloadSchedules();
		}

		void CWebServer::Cmd_GetNotifications(WebEmSession& session, const request& req, Json::Value& root)
		{
			root["status"] = "OK";
			root["title"] = "getnotifications";

			int ii = 0;

			// Add known notification systems
			for (const auto& notifier : m_notifications.m_notifiers)
			{
				root["notifiers"][ii]["name"] = notifier.first;
				root["notifiers"][ii]["description"] = notifier.first;
				ii++;
			}

			uint64_t idx = 0;
			if (!request::findValue(&req, "idx").empty())
			{
				idx = std::stoull(request::findValue(&req, "idx"));
			}
			std::vector<_tNotification> notifications = m_notifications.GetNotifications(idx);
			if (!notifications.empty())
			{
				ii = 0;
				for (const auto& n : notifications)
				{
					root["result"][ii]["idx"] = Json::Value::UInt64(n.ID);
					std::string sParams = n.Params;
					if (sParams.empty())
					{
						sParams = "S";
					}
					root["result"][ii]["Params"] = sParams;
					root["result"][ii]["Priority"] = n.Priority;
					root["result"][ii]["SendAlways"] = n.SendAlways;
					root["result"][ii]["CustomMessage"] = n.CustomMessage;
					root["result"][ii]["CustomAction"] = CURLEncode::URLEncode(n.CustomAction);
					root["result"][ii]["ActiveSystems"] = n.ActiveSystems;
					ii++;
				}
			}
		}

		void CWebServer::Cmd_GetSharedUserDevices(WebEmSession& session, const request& req, Json::Value& root)
		{
			std::string idx = request::findValue(&req, "idx");
			if (idx.empty())
				return;
			root["title"] = "GetSharedUserDevices";

			std::vector<std::vector<std::string>> result;
			result = m_sql.safe_query("SELECT DeviceRowID FROM SharedDevices WHERE (SharedUserID == '%q')", idx.c_str());
			if (!result.empty())
			{
				int ii = 0;
				for (const auto& sd : result)
				{
					root["result"][ii]["DeviceRowIdx"] = sd[0];
					ii++;
				}
			}
			root["status"] = "OK";
		}

		void CWebServer::Cmd_SetSharedUserDevices(WebEmSession& session, const request& req, Json::Value& root)
		{
			if (session.rights != 2)
			{
				session.reply_status = reply::forbidden;
				return; // Only admin user allowed
			}
			std::string idx = request::findValue(&req, "idx");
			std::string userdevices = CURLEncode::URLDecode(request::findValue(&req, "devices"));
			if (idx.empty())
				return;
			root["title"] = "SetSharedUserDevices";
			std::vector<std::string> strarray;
			StringSplit(userdevices, ";", strarray);

			// First make a backup of the favorite devices before deleting the devices for this user, then add the (new) onces and restore favorites
			m_sql.safe_query("UPDATE SharedDevices SET SharedUserID = 0 WHERE SharedUserID == '%q' and Favorite == 1", idx.c_str());
			m_sql.safe_query("DELETE FROM SharedDevices WHERE SharedUserID == '%q'", idx.c_str());

			int nDevices = static_cast<int>(strarray.size());
			for (int ii = 0; ii < nDevices; ii++)
			{
				m_sql.safe_query("INSERT INTO SharedDevices (SharedUserID,DeviceRowID) VALUES ('%q','%q')", idx.c_str(), strarray[ii].c_str());
				m_sql.safe_query("UPDATE SharedDevices SET Favorite = 1 WHERE SharedUserid == '%q' AND DeviceRowID IN (SELECT DeviceRowID FROM SharedDevices WHERE SharedUserID == 0)",
					idx.c_str());
			}
			m_sql.safe_query("DELETE FROM SharedDevices WHERE SharedUserID == 0");
			LoadUsers();
			root["status"] = "OK";
		}

		void CWebServer::Cmd_ClearUserDevices(WebEmSession& session, const request& req, Json::Value& root)
		{
			if (session.rights != 2)
			{
				session.reply_status = reply::forbidden;
				return; // Only admin user allowed
			}
			std::string idx = request::findValue(&req, "idx");
			if (idx.empty())
				return;
			root["status"] = "OK";
			root["title"] = "ClearUserDevices";
			m_sql.safe_query("DELETE FROM SharedDevices WHERE SharedUserID == '%q'", idx.c_str());
			LoadUsers();
		}

		void CWebServer::Cmd_SetUsed(WebEmSession& session, const request& req, Json::Value& root)
		{
			if (session.rights != 2)
			{
				session.reply_status = reply::forbidden;
				return; // Only admin user allowed
			}

			std::string idx = request::findValue(&req, "idx");
			std::string sused = request::findValue(&req, "used");
			if ((idx.empty()) || (sused.empty()))
				return;
			std::vector<std::vector<std::string>> result;
			result = m_sql.safe_query("SELECT Type,SubType,HardwareID,CustomImage FROM DeviceStatus WHERE (ID == '%q')", idx.c_str());
			if (result.empty())
				return;

			std::string deviceid = request::findValue(&req, "deviceid");
			std::string name = HTMLSanitizer::Sanitize(request::findValue(&req, "name"));
			std::string description = HTMLSanitizer::Sanitize(request::findValue(&req, "description"));
			std::string sswitchtype = request::findValue(&req, "switchtype");
			std::string maindeviceidx = request::findValue(&req, "maindeviceidx");
			std::string addjvalue = request::findValue(&req, "addjvalue");
			std::string addjmulti = request::findValue(&req, "addjmulti");
			std::string addjvalue2 = request::findValue(&req, "addjvalue2");
			std::string addjmulti2 = request::findValue(&req, "addjmulti2");
			std::string setPoint = request::findValue(&req, "setpoint");
			std::string state = request::findValue(&req, "state");
			std::string mode = request::findValue(&req, "mode");
			std::string until = request::findValue(&req, "until");
			std::string clock = request::findValue(&req, "clock");
			std::string tmode = request::findValue(&req, "tmode");
			std::string fmode = request::findValue(&req, "fmode");
			std::string sCustomImage = request::findValue(&req, "customimage");

			std::string strunit = request::findValue(&req, "unit");
			std::string strParam1 = HTMLSanitizer::Sanitize(base64_decode(request::findValue(&req, "strparam1")));
			std::string strParam2 = HTMLSanitizer::Sanitize(base64_decode(request::findValue(&req, "strparam2")));
			std::string tmpstr = request::findValue(&req, "protected");
			bool bHasstrParam1 = request::hasValue(&req, "strparam1");
			int iProtected = (tmpstr == "true") ? 1 : 0;

			std::string sOptions = HTMLSanitizer::Sanitize(base64_decode(request::findValue(&req, "options")));
			std::string devoptions = HTMLSanitizer::Sanitize(CURLEncode::URLDecode(request::findValue(&req, "devoptions")));
			std::string EnergyMeterMode = CURLEncode::URLDecode(request::findValue(&req, "EnergyMeterMode"));

			char szTmp[200];

			bool bHaveUser = (!session.username.empty());
			// int iUser = -1;
			if (bHaveUser)
			{
				// iUser = FindUser(session.username.c_str());
			}

			int switchtype = -1;
			if (!sswitchtype.empty())
				switchtype = atoi(sswitchtype.c_str());

			int used = (sused == "true") ? 1 : 0;
			if (!maindeviceidx.empty())
				used = 0;

			std::vector<std::string> sd = result[0];
			unsigned char dType = atoi(sd[0].c_str());
			unsigned char dSubType = atoi(sd[1].c_str());
			int HwdID = atoi(sd[2].c_str());
			std::string sHwdID = sd[2];
			int OldCustomImage = atoi(sd[3].c_str());

			int CustomImage = (!sCustomImage.empty()) ? std::stoi(sCustomImage) : OldCustomImage;

			// Strip trailing spaces in 'name'
			name = stdstring_trim(name);

			// Strip trailing spaces in 'description'
			description = stdstring_trim(description);

			if (!setPoint.empty() || !state.empty())
			{
				double tempcelcius = atof(setPoint.c_str());
				if (m_sql.m_tempunit == TEMPUNIT_F)
				{
					// Convert back to Celsius
					tempcelcius = ConvertToCelsius(tempcelcius);
				}
				sprintf(szTmp, "%.2f", tempcelcius);

				if (dType != pTypeEvohomeZone && dType != pTypeEvohomeWater) // sql update now done in setsetpoint for evohome devices
				{
					m_sql.safe_query("UPDATE DeviceStatus SET Used=%d, sValue='%q' WHERE (ID == '%q')", used, szTmp, idx.c_str());
				}
			}
			if (name.empty())
			{
				m_sql.safe_query("UPDATE DeviceStatus SET Used=%d WHERE (ID == '%q')", used, idx.c_str());
			}
			else
			{
				if (switchtype == -1)
				{
					m_sql.safe_query("UPDATE DeviceStatus SET Used=%d, Name='%q', Description='%q', CustomImage=%d WHERE (ID == '%q')", used, name.c_str(), description.c_str(),
						CustomImage, idx.c_str());
				}
				else
				{
					m_sql.safe_query("UPDATE DeviceStatus SET Used=%d, Name='%q', Description='%q', SwitchType=%d, CustomImage=%d WHERE (ID == '%q')", used, name.c_str(),
						description.c_str(), switchtype, CustomImage, idx.c_str());
				}
			}

			if (bHasstrParam1)
			{
				m_sql.safe_query("UPDATE DeviceStatus SET StrParam1='%q', StrParam2='%q' WHERE (ID == '%q')", strParam1.c_str(), strParam2.c_str(), idx.c_str());
			}

			m_sql.safe_query("UPDATE DeviceStatus SET Protected=%d WHERE (ID == '%q')", iProtected, idx.c_str());

			if (!setPoint.empty() || !state.empty())
			{
				int urights = 3;
				if (bHaveUser)
				{
					int iUser = FindUser(session.username.c_str());
					if (iUser != -1)
					{
						urights = static_cast<int>(m_users[iUser].userrights);
						_log.Log(LOG_STATUS, "User: %s initiated a SetPoint command", m_users[iUser].Username.c_str());
					}
				}
				if (urights < 1)
					return;
				if (dType == pTypeEvohomeWater)
					m_mainworker.SetSetPoint(idx, (state == "On") ? 1.0F : 0.0F, mode, until); // FIXME float not guaranteed precise?
				else if (dType == pTypeEvohomeZone)
					m_mainworker.SetSetPoint(idx, static_cast<float>(atof(setPoint.c_str())), mode, until);
				else
					m_mainworker.SetSetPoint(idx, static_cast<float>(atof(setPoint.c_str())));
			}

			if (!strunit.empty())
			{
				bool bUpdateUnit = true;
#ifdef ENABLE_PYTHON
				// check if HW is plugin
				std::vector<std::vector<std::string>> result;
				result = m_sql.safe_query("SELECT Type FROM Hardware WHERE (ID == %d)", HwdID);
				if (!result.empty())
				{
					_eHardwareTypes Type = (_eHardwareTypes)std::stoi(result[0][0]);
					if (Type == HTYPE_PythonPlugin)
					{
						bUpdateUnit = false;
						_log.Log(LOG_ERROR, "Cmd_SetUsed: Not allowed to change unit of device owned by plugin %u!", HwdID);
					}
				}
#endif
				if (bUpdateUnit)
				{
					m_sql.safe_query("UPDATE DeviceStatus SET Unit='%q' WHERE (ID == '%q')", strunit.c_str(), idx.c_str());
				}
			}
			// FIXME evohome ...we need the zone id to update the correct zone...but this should be ok as a generic call?
			if (!deviceid.empty())
			{
				m_sql.safe_query("UPDATE DeviceStatus SET DeviceID='%q' WHERE (ID == '%q')", deviceid.c_str(), idx.c_str());
			}
			if (!addjvalue.empty())
			{
				double faddjvalue = atof(addjvalue.c_str());
				m_sql.safe_query("UPDATE DeviceStatus SET AddjValue=%f WHERE (ID == '%q')", faddjvalue, idx.c_str());
			}
			if (!addjmulti.empty())
			{
				double faddjmulti = atof(addjmulti.c_str());
				if (faddjmulti == 0)
					faddjmulti = 1;
				m_sql.safe_query("UPDATE DeviceStatus SET AddjMulti=%f WHERE (ID == '%q')", faddjmulti, idx.c_str());
			}
			if (!addjvalue2.empty())
			{
				double faddjvalue2 = atof(addjvalue2.c_str());
				m_sql.safe_query("UPDATE DeviceStatus SET AddjValue2=%f WHERE (ID == '%q')", faddjvalue2, idx.c_str());
			}
			if (!addjmulti2.empty())
			{
				double faddjmulti2 = atof(addjmulti2.c_str());
				if (faddjmulti2 == 0)
					faddjmulti2 = 1;
				m_sql.safe_query("UPDATE DeviceStatus SET AddjMulti2=%f WHERE (ID == '%q')", faddjmulti2, idx.c_str());
			}
			if (!EnergyMeterMode.empty())
			{
				auto options = m_sql.GetDeviceOptions(idx);
				options["EnergyMeterMode"] = EnergyMeterMode;
				uint64_t ullidx = std::stoull(idx);
				m_sql.SetDeviceOptions(ullidx, options);
			}

			if (!devoptions.empty())
			{
				m_sql.safe_query("UPDATE DeviceStatus SET Options='%q' WHERE (ID == '%q')", devoptions.c_str(), idx.c_str());
			}

			if (used == 0)
			{
				bool bRemoveSubDevices = (request::findValue(&req, "RemoveSubDevices") == "true");

				if (bRemoveSubDevices)
				{
					// if this device was a slave device, remove it
					m_sql.safe_query("DELETE FROM LightSubDevices WHERE (DeviceRowID == '%q')", idx.c_str());
				}
				m_sql.safe_query("DELETE FROM LightSubDevices WHERE (ParentID == '%q')", idx.c_str());

				m_sql.safe_query("DELETE FROM Timers WHERE (DeviceRowID == '%q')", idx.c_str());
			}

			// Save device options
			if (!sOptions.empty())
			{
				uint64_t ullidx = std::stoull(idx);
				m_sql.SetDeviceOptions(ullidx, m_sql.BuildDeviceOptions(sOptions, false));
			}

			if (!maindeviceidx.empty())
			{
				if (maindeviceidx != idx)
				{
					// this is a sub device for another light/switch
					// first check if it is not already a sub device
					result = m_sql.safe_query("SELECT ID FROM LightSubDevices WHERE (DeviceRowID=='%q') AND (ParentID =='%q')", idx.c_str(), maindeviceidx.c_str());
					if (result.empty())
					{
						// no it is not, add it
						m_sql.safe_query("INSERT INTO LightSubDevices (DeviceRowID, ParentID) VALUES ('%q','%q')", idx.c_str(), maindeviceidx.c_str());
					}
				}
			}
			if ((used == 0) && (maindeviceidx.empty()))
			{
				// really remove it, including log etc
				m_sql.DeleteDevices(idx);
			}
			else
			{
#ifdef ENABLE_PYTHON
				// Notify plugin framework about the change
				m_mainworker.m_pluginsystem.DeviceModified(atoi(idx.c_str()));
#endif
			}
			if (!result.empty())
			{
				root["status"] = "OK";
				root["title"] = "SetUsed";
			}
			if (m_sql.m_bEnableEventSystem)
				m_mainworker.m_eventsystem.GetCurrentStates();
		}

		void CWebServer::Cmd_GetSettings(WebEmSession& session, const request& req, Json::Value& root)
		{
			std::vector<std::vector<std::string>> result;
			char szTmp[100];

			result = m_sql.safe_query("SELECT Key, nValue, sValue FROM Preferences");
			if (result.empty())
				return;
			root["status"] = "OK";
			root["title"] = "settings";
			root["cloudenabled"] = false;

			for (const auto& sd : result)
			{
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
				if (m_notifications.IsInConfig(Key))
				{
					if (sValue.empty() && nValue > 0)
					{
						root[Key] = nValue;
					}
					else
					{
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
				else if (Key == "ShortLogAddOnlyNewValues")
				{
					root["ShortLogAddOnlyNewValues"] = nValue;
				}
				else if (Key == "ShortLogInterval")
				{
					root["ShortLogInterval"] = nValue;
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
				else if (Key == "MaxElectricPower")
				{
					root["MaxElectricPower"] = nValue;
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
					sprintf(szTmp, "%.4f", (float)(nValue) / 10000.0F);
					root["CostEnergy"] = szTmp;
				}
				else if (Key == "CostEnergyT2")
				{
					sprintf(szTmp, "%.4f", (float)(nValue) / 10000.0F);
					root["CostEnergyT2"] = szTmp;
				}
				else if (Key == "CostEnergyR1")
				{
					sprintf(szTmp, "%.4f", (float)(nValue) / 10000.0F);
					root["CostEnergyR1"] = szTmp;
				}
				else if (Key == "CostEnergyR2")
				{
					sprintf(szTmp, "%.4f", (float)(nValue) / 10000.0F);
					root["CostEnergyR2"] = szTmp;
				}
				else if (Key == "CostGas")
				{
					sprintf(szTmp, "%.4f", (float)(nValue) / 10000.0F);
					root["CostGas"] = szTmp;
				}
				else if (Key == "CostWater")
				{
					sprintf(szTmp, "%.4f", (float)(nValue) / 10000.0F);
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
				else if (Key == "Title")
				{
					root["Title"] = sValue;
				}
				else if (Key == "WindUnit")
				{
					root["WindUnit"] = nValue;
				}
				else if (Key == "TempUnit")
				{
					root["TempUnit"] = nValue;
				}
				else if (Key == "WeightUnit")
				{
					root["WeightUnit"] = nValue;
				}
				else if (Key == "AllowPlainBasicAuth")
				{
					root["AllowPlainBasicAuth"] = nValue;
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
				else if (Key == "DegreeDaysBaseTemperature")
				{
					root["DegreeDaysBaseTemperature"] = sValue;
				}
				else if (Key == "EnableEventScriptSystem")
				{
					root["EnableEventScriptSystem"] = nValue;
				}
				else if (Key == "EventSystemLogFullURL")
				{
					root["EventSystemLogFullURL"] = nValue;
				}
				else if (Key == "DisableDzVentsSystem")
				{
					root["DisableDzVentsSystem"] = nValue;
				}
				else if (Key == "DzVentsLogLevel")
				{
					root["DzVentsLogLevel"] = nValue;
				}
				else if (Key == "LogEventScriptTrigger")
				{
					root["LogEventScriptTrigger"] = nValue;
				}
				else if (Key == "(1WireSensorPollPeriod")
				{
					root["1WireSensorPollPeriod"] = nValue;
				}
				else if (Key == "(1WireSwitchPollPeriod")
				{
					root["1WireSwitchPollPeriod"] = nValue;
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
				else if (Key == "MyDomoticzSubsystems")
				{
					root["MyDomoticzSubsystems"] = nValue;
				}
				else if (Key == "SendErrorsAsNotification")
				{
					root["SendErrorsAsNotification"] = nValue;
				}
				else if (Key == "DeltaTemperatureLog")
				{
					root[Key] = sValue;
				}
				else if (Key == "IFTTTEnabled")
				{
					root["IFTTTEnabled"] = nValue;
				}
				else if (Key == "IFTTTAPI")
				{
					root["IFTTTAPI"] = sValue;
				}
			}
		}

		void CWebServer::Cmd_GetLightLog(WebEmSession& session, const request& req, Json::Value& root)
		{
			uint64_t idx = 0;
			if (!request::findValue(&req, "idx").empty())
			{
				idx = std::stoull(request::findValue(&req, "idx"));
			}
			std::vector<std::vector<std::string>> result;
			// First get Device Type/SubType
			result = m_sql.safe_query("SELECT Type, SubType, SwitchType, Options FROM DeviceStatus WHERE (ID == %" PRIu64 ")", idx);
			if (result.empty())
				return;

			unsigned char dType = atoi(result[0][0].c_str());
			unsigned char dSubType = atoi(result[0][1].c_str());
			_eSwitchType switchtype = (_eSwitchType)atoi(result[0][2].c_str());
			std::map<std::string, std::string> options = m_sql.BuildDeviceOptions(result[0][3]);

			if (
				(dType != pTypeLighting1) && (dType != pTypeLighting2) && (dType != pTypeLighting3) && (dType != pTypeLighting4) && (dType != pTypeLighting5) &&
				(dType != pTypeLighting6) && (dType != pTypeFan) && (dType != pTypeColorSwitch) && (dType != pTypeSecurity1) && (dType != pTypeSecurity2) && (dType != pTypeEvohome) &&
				(dType != pTypeEvohomeRelay) && (dType != pTypeCurtain) && (dType != pTypeBlinds) && (dType != pTypeRFY) && (dType != pTypeRego6XXValue) && (dType != pTypeChime) &&
				(dType != pTypeThermostat2) && (dType != pTypeThermostat3) && (dType != pTypeThermostat4) && (dType != pTypeRemote) && (dType != pTypeGeneralSwitch) &&
				(dType != pTypeHomeConfort) && (dType != pTypeFS20) && (!((dType == pTypeRadiator1) && (dSubType == sTypeSmartwaresSwitchRadiator))) && (dType != pTypeHunter) && (dType != pTypeDDxxxx)
				)
				return; // no light device! we should not be here!

			root["status"] = "OK";
			root["title"] = "getlightlog";

			result = m_sql.safe_query("SELECT ROWID, nValue, sValue, User, Date FROM LightingLog WHERE (DeviceRowID==%" PRIu64 ") ORDER BY Date DESC", idx);
			if (!result.empty())
			{
				std::map<std::string, std::string> selectorStatuses;
				if (switchtype == STYPE_Selector)
				{
					GetSelectorSwitchStatuses(options, selectorStatuses);
				}

				int ii = 0;
				for (const auto& sd : result)
				{
					std::string lidx = sd.at(0);
					int nValue = atoi(sd.at(1).c_str());
					std::string sValue = sd.at(2);
					std::string sUser = sd.at(3);
					std::string ldate = sd.at(4);

					// add light details
					std::string lstatus;
					std::string ldata;
					int llevel = 0;
					bool bHaveDimmer = false;
					bool bHaveSelector = false;
					bool bHaveGroupCmd = false;
					int maxDimLevel = 0;

					if (switchtype == STYPE_Media)
					{
						if (sValue == "0")
							continue; // skip 0-values in log for MediaPlayers
						lstatus = sValue;
						ldata = lstatus;
					}
					else if (switchtype == STYPE_Selector)
					{
						if (ii == 0)
						{
							bHaveSelector = true;
							maxDimLevel = (int)selectorStatuses.size();
						}
						if (!selectorStatuses.empty())
						{

							std::string sLevel = selectorStatuses[sValue];
							ldata = sLevel;
							lstatus = "Set Level: " + sLevel;
							llevel = atoi(sValue.c_str());
						}
					}
					else
					{
						GetLightStatus(dType, dSubType, switchtype, nValue, sValue, lstatus, llevel, bHaveDimmer, maxDimLevel, bHaveGroupCmd);
						ldata = lstatus;
					}

					if (ii == 0)
					{
						// Log these parameters once
						root["HaveDimmer"] = bHaveDimmer;
						root["result"][ii]["MaxDimLevel"] = maxDimLevel;
						root["HaveGroupCmd"] = bHaveGroupCmd;
						root["HaveSelector"] = bHaveSelector;
					}

					// Corrent names for certain switch types
					switch (switchtype)
					{
					case STYPE_Contact:
						ldata = (ldata == "On") ? "Open" : "Closed";
						break;
					case STYPE_DoorContact:
						ldata = (ldata == "On") ? "Open" : "Closed";
						break;
					case STYPE_DoorLock:
						ldata = (ldata == "On") ? "Locked" : "Unlocked";
						break;
					case STYPE_DoorLockInverted:
						ldata = (ldata == "On") ? "Unlocked" : "Locked";
						break;
					}

					root["result"][ii]["idx"] = lidx;
					root["result"][ii]["Date"] = ldate;
					root["result"][ii]["Data"] = ldata;
					root["result"][ii]["Status"] = lstatus;
					root["result"][ii]["Level"] = llevel;
					root["result"][ii]["User"] = sUser;
					ii++;
				}
			}
		}

		void CWebServer::Cmd_GetTextLog(WebEmSession& session, const request& req, Json::Value& root)
		{
			uint64_t idx = 0;
			if (!request::findValue(&req, "idx").empty())
			{
				idx = std::stoull(request::findValue(&req, "idx"));
			}
			std::vector<std::vector<std::string>> result;

			root["status"] = "OK";
			root["title"] = "gettextlog";

			result = m_sql.safe_query("SELECT ROWID, sValue, User, Date FROM LightingLog WHERE (DeviceRowID==%" PRIu64 ") ORDER BY Date DESC", idx);
			if (!result.empty())
			{
				int ii = 0;
				for (const auto& sd : result)
				{
					root["result"][ii]["idx"] = sd[0];
					root["result"][ii]["Data"] = sd[1];
					root["result"][ii]["User"] = sd[2];
					root["result"][ii]["Date"] = sd[3];
					ii++;
				}
			}
		}

		void CWebServer::Cmd_GetSceneLog(WebEmSession& session, const request& req, Json::Value& root)
		{
			uint64_t idx = 0;
			if (!request::findValue(&req, "idx").empty())
			{
				idx = std::stoull(request::findValue(&req, "idx"));
			}
			std::vector<std::vector<std::string>> result;

			root["status"] = "OK";
			root["title"] = "getscenelog";

			result = m_sql.safe_query("SELECT ROWID, nValue, User, Date FROM SceneLog WHERE (SceneRowID==%" PRIu64 ") ORDER BY Date DESC", idx);
			if (!result.empty())
			{
				int ii = 0;
				for (const auto& sd : result)
				{
					root["result"][ii]["idx"] = sd[0];
					int nValue = atoi(sd[1].c_str());
					root["result"][ii]["Data"] = (nValue == 0) ? "Off" : "On";
					root["result"][ii]["User"] = sd[2];
					root["result"][ii]["Date"] = sd[3];
					ii++;
				}
			}
		}

		void CWebServer::Cmd_RemoteWebClientsLog(WebEmSession& session, const request& req, Json::Value& root)
		{
			if (session.rights != 2)
			{
				session.reply_status = reply::forbidden;
				return; // Only admin user allowed
			}

			int ii = 0;
			root["title"] = "rclientslog";
			for (const auto& itt_rc : m_remote_web_clients)
			{
				char timestring[128];
				timestring[0] = 0;
				struct tm timeinfo;
				localtime_r(&itt_rc.second.last_seen, &timeinfo);

				strftime(timestring, sizeof(timestring), "%a, %d %b %Y %H:%M:%S %z", &timeinfo);

				root["result"][ii]["date"] = timestring;
				root["result"][ii]["address"] = itt_rc.second.host_remote_endpoint_address_;
				root["result"][ii]["port"] = itt_rc.second.host_local_endpoint_port_;
				root["result"][ii]["req"] = itt_rc.second.host_last_request_uri_;
				ii++;
			}
			root["status"] = "OK";
		}

	} // namespace server
} // namespace http
