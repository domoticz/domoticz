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
#ifdef WIN32
#include <windows.h>
#endif
#include <openssl/sha.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/bn.h>
#include <openssl/param_build.h>
#include <openssl/core_names.h>
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
#include "KWHStats.h"
#include "ThemeSettings.h"
#include "WebAssets.h"
#include "WebAssetFetch.h"
#include "../httpclient/HTTPClient.h"
#include "../hardware/hardwaretypes.h"
#include <libwebem/Base64.h>
#include "../smtpclient/SMTPClient.h"
#include "../push/BasePush.h"
#include "../push/McpPush.h"
#include "../notifications/NotificationHelper.h"

#ifdef ENABLE_PYTHON
#include "../hardware/plugins/Plugins.h"
#include "../hardware/plugins/PluginManager.h"
#include "../tinyxpath/tinyxml.h"
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
#include <set>

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
#include "../hardware/Matter.h"
#include "../hardware/RFLinkBase.h"
#ifdef WITH_OPENZWAVE
#include "../hardware/OpenZWave.h"
#endif

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
				// Time counters are deprecated and migrated to Custom counters since DB version 99, hide from selection
				if (ii == MTYPE_TIME)
					continue;
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

		void CWebServer::Cmd_FetchUrl(WebEmSession& session, const request& req, Json::Value& root)
		{
			root["status"] = "ERR";
			root["title"] = "FetchUrl";
			std::string sUrl = request::findValue(&req, "url");
			if (sUrl.empty())
			{
				session.reply_status = reply::bad_request;
				root["message"] = "url parameter missing";
				return;
			}
			// Only allow http/https URLs
			if (sUrl.substr(0, 7) != "http://" && sUrl.substr(0, 8) != "https://")
			{
				session.reply_status = reply::bad_request;
				root["message"] = "Only http/https URLs are allowed";
				return;
			}
			std::string sResult;
			if (!HTTPClient::GET(sUrl, sResult))
			{
				session.reply_status = reply::bad_request;
				root["message"] = "Fetch failed";
				return;
			}
			root["status"] = "OK";
			root["data"] = sResult;
		}

		void CWebServer::Cmd_LoginCheck(WebEmSession& session, const request& req, Json::Value& root)
		{
			root["status"] = "ERR";
			root["title"] = "logincheck";
			std::string tmpusrname = request::findValue(&req, "username");
			std::string tmpusrpass = request::findValue(&req, "password");
			if ((tmpusrname.empty()) || (tmpusrpass.empty()))
			{
				session.reply_status = reply::bad_request;
				return;
			}

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
						session.reply_status = reply::unauthorized;
						return;
					}
					if (m_users[iUser].Password != usrpass)
					{
						// log brute force attack
						_log.Log(LOG_ERROR, "Failed login attempt from %s for '%s' !", session.remote_host.c_str(), m_users[iUser].Username.c_str());
						session.reply_status = reply::unauthorized;
						return;
					}
					if (m_users[iUser].userrights == URIGHTS_CLIENTID) {
						// Not a right for users to login with
						_log.Log(LOG_ERROR, "Failed login attempt from %s for '%s' !", session.remote_host.c_str(), m_users[iUser].Username.c_str());
						session.reply_status = reply::unauthorized;
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
							session.reply_status = reply::internal_server_error;
							return;
						}
						if (tmp2fa.empty())
						{
							// No 2FA token given (yet), request one
							root["status"] = "OK";
							root["require2fa"] = "true";
							return;
						}
						if (!VerifySHA1TOTP(tmp2fa, sTotpKey))
						{
							// Not a match for the given 2FA token
							_log.Log(LOG_ERROR, "Failed login attempt from %s for '%s' !", session.remote_host.c_str(), m_users[iUser].Username.c_str());
							_log.Debug(DEBUG_AUTH, "Failed login attempt with 2FA token: %s", tmp2fa.c_str());
							session.reply_status = reply::unauthorized;
							return;
						}
					}
					_log.Log(LOG_STATUS, "Login successful from %s for user '%s'", session.remote_host.c_str(), m_users[iUser].Username.c_str());
					root["status"] = "OK";
					root["version"] = szAppVersion;
					session.isnew = true;
					session.username = m_users[iUser].Username;
					session.rights = m_users[iUser].userrights;
					session.rememberme = (rememberme == "true");
					root["user"] = session.username;
					root["rights"] = session.rights;
				}
			}
		}

		// ---------------------------------------------------------------------------
		// WebAuthn / Passkey helper: minimal CBOR value reader
		// Returns false on parse error; advances 'pos' past the item.
		// When the item is a byte-string the raw bytes are written to 'out_bytes'.
		// ---------------------------------------------------------------------------
		namespace {

			// Skip a single CBOR item (any type), advancing pos.  Returns false on error.
			static bool cbor_skip(const std::vector<uint8_t>& buf, size_t& pos);

			static bool cbor_read_length(const std::vector<uint8_t>& buf, size_t& pos, uint8_t addl, uint64_t& out_len)
			{
				if (addl <= 23) { out_len = addl; return true; }
				if (addl == 24) {
					if (pos >= buf.size()) return false;
					out_len = buf[pos++]; return true;
				}
				if (addl == 25) {
					if (pos + 2 > buf.size()) return false;
					out_len = (uint64_t(buf[pos]) << 8) | buf[pos + 1]; pos += 2; return true;
				}
				if (addl == 26) {
					if (pos + 4 > buf.size()) return false;
					out_len = (uint64_t(buf[pos]) << 24) | (uint64_t(buf[pos+1]) << 16) |
					          (uint64_t(buf[pos+2]) << 8)  | buf[pos+3]; pos += 4; return true;
				}
				if (addl == 27) {
					if (pos + 8 > buf.size()) return false;
					out_len = (uint64_t(buf[pos])   << 56) | (uint64_t(buf[pos+1]) << 48) |
					          (uint64_t(buf[pos+2]) << 40) | (uint64_t(buf[pos+3]) << 32) |
					          (uint64_t(buf[pos+4]) << 24) | (uint64_t(buf[pos+5]) << 16) |
					          (uint64_t(buf[pos+6]) << 8)  |  uint64_t(buf[pos+7]);
					pos += 8; return true;
				}
				return false;
			}

			static bool cbor_skip(const std::vector<uint8_t>& buf, size_t& pos)
			{
				if (pos >= buf.size()) return false;
				uint8_t first = buf[pos++];
				uint8_t major = first >> 5;
				uint8_t addl  = first & 0x1f;
				uint64_t len = 0;

				if (major == 0) { // unsigned int – already consumed
					if (!cbor_read_length(buf, pos, addl, len)) return false;
					// integer itself is encoded in the additional-info bytes; nothing more to skip
					return true;
				}
				if (major == 1) { // negative int
					if (!cbor_read_length(buf, pos, addl, len)) return false;
					return true;
				}
				if (major == 2 || major == 3) { // byte string or text string
					if (!cbor_read_length(buf, pos, addl, len)) return false;
					if (pos + len > buf.size()) return false;
					pos += (size_t)len;
					return true;
				}
				if (major == 4) { // array
					if (!cbor_read_length(buf, pos, addl, len)) return false;
					for (uint64_t i = 0; i < len; i++)
						if (!cbor_skip(buf, pos)) return false;
					return true;
				}
				if (major == 5) { // map
					if (!cbor_read_length(buf, pos, addl, len)) return false;
					for (uint64_t i = 0; i < len * 2; i++)
						if (!cbor_skip(buf, pos)) return false;
					return true;
				}
				if (major == 7) { // simple / float
					if (addl <= 23) return true;
					if (addl == 24) { pos++; return pos <= buf.size(); }
					if (addl == 25) { pos += 2; return pos <= buf.size(); }
					if (addl == 26) { pos += 4; return pos <= buf.size(); }
					if (addl == 27) { pos += 8; return pos <= buf.size(); }
					return true;
				}
				return false;
			}

			// Read a CBOR text string into out_str; advances pos.
			static bool cbor_read_tstr(const std::vector<uint8_t>& buf, size_t& pos, std::string& out_str)
			{
				if (pos >= buf.size()) return false;
				uint8_t first = buf[pos++];
				if ((first >> 5) != 3) return false;
				uint64_t len = 0;
				if (!cbor_read_length(buf, pos, first & 0x1f, len)) return false;
				if (pos + len > buf.size()) return false;
				out_str.assign(reinterpret_cast<const char*>(buf.data() + pos), (size_t)len);
				pos += (size_t)len;
				return true;
			}

			// Read a CBOR byte string into out; advances pos.
			static bool cbor_read_bstr(const std::vector<uint8_t>& buf, size_t& pos, std::vector<uint8_t>& out)
			{
				if (pos >= buf.size()) return false;
				uint8_t first = buf[pos++];
				if ((first >> 5) != 2) return false;
				uint64_t len = 0;
				if (!cbor_read_length(buf, pos, first & 0x1f, len)) return false;
				if (pos + len > buf.size()) return false;
				out.assign(buf.data() + pos, buf.data() + pos + (size_t)len);
				pos += (size_t)len;
				return true;
			}

			// Read a CBOR integer (positive or negative) into out_val; advances pos.
			static bool cbor_read_int(const std::vector<uint8_t>& buf, size_t& pos, int64_t& out_val)
			{
				if (pos >= buf.size()) return false;
				uint8_t first = buf[pos++];
				uint8_t major = first >> 5;
				uint8_t addl  = first & 0x1f;
				if (major != 0 && major != 1) return false;
				uint64_t len = 0;
				if (!cbor_read_length(buf, pos, addl, len)) return false;
				if (major == 0)
					out_val = static_cast<int64_t>(len);
				else
					out_val = -1 - static_cast<int64_t>(len);
				return true;
			}

			// Extract authData byte-string from a "none"-attestation CBOR map.
			// The map has keys "fmt", "attStmt", "authData".
			static bool cbor_extract_authdata(const std::vector<uint8_t>& buf, std::vector<uint8_t>& authData)
			{
				size_t pos = 0;
				if (pos >= buf.size()) return false;
				uint8_t first = buf[pos++];
				if ((first >> 5) != 5) return false; // must be a map
				uint64_t mapLen = 0;
				if (!cbor_read_length(buf, pos, first & 0x1f, mapLen)) return false;

				for (uint64_t i = 0; i < mapLen; i++) {
					std::string key;
					if (!cbor_read_tstr(buf, pos, key)) return false;
					if (key == "authData") {
						if (!cbor_read_bstr(buf, pos, authData)) return false;
						return true;
					}
					// skip the value
					if (!cbor_skip(buf, pos)) return false;
				}
				return false;
			}

			// Parse a COSE_Key (CBOR map) and extract ES256 or RS256 key material.
			struct CoseKey {
				int64_t kty  = 0; // 1
				int64_t alg  = 0; // 3
				int64_t crv  = 0; // -1 (EC only)
				std::vector<uint8_t> x; // -2
				std::vector<uint8_t> y; // -3
				std::vector<uint8_t> n; // -1 (RSA)
				std::vector<uint8_t> e; // -2 (RSA)
			};

			static bool cbor_parse_cose_key(const std::vector<uint8_t>& buf, CoseKey& key)
			{
				size_t pos = 0;
				if (pos >= buf.size()) return false;
				uint8_t first = buf[pos++];
				if ((first >> 5) != 5) return false;
				uint64_t mapLen = 0;
				if (!cbor_read_length(buf, pos, first & 0x1f, mapLen)) return false;

				for (uint64_t i = 0; i < mapLen; i++) {
					int64_t k = 0;
					if (!cbor_read_int(buf, pos, k)) return false;
					if (k == 1 || k == 3) {
						int64_t v = 0;
						if (!cbor_read_int(buf, pos, v)) return false;
						if (k == 1) key.kty = v;
						else        key.alg = v;
					} else if (k == -1) {
						// EC: crv (int) or RSA: n (bstr)
						if (pos >= buf.size()) return false;
						uint8_t peek = buf[pos];
						uint8_t major = peek >> 5;
						if (major == 0 || major == 1) {
							if (!cbor_read_int(buf, pos, key.crv)) return false;
						} else {
							if (!cbor_read_bstr(buf, pos, key.n)) return false;
						}
					} else if (k == -2) {
						// EC: x (bstr) or RSA: e (bstr)
						std::vector<uint8_t> tmp;
						if (!cbor_read_bstr(buf, pos, tmp)) return false;
						if (key.kty == 2 || key.x.empty()) key.x = tmp;
						key.e = tmp;
					} else if (k == -3) {
						// EC: y (bstr)
						if (!cbor_read_bstr(buf, pos, key.y)) return false;
					} else {
						if (!cbor_skip(buf, pos)) return false;
					}
				}
				return true;
			}

		// Extract the WebAuthn RP ID from the HTTP Host header.
		// WebAuthn requires a domain name as RP ID; the server's local socket address
		// (session.local_host) may be a raw IP which browsers reject.
		// The Host header contains exactly what the browser used to connect.
		static std::string GetWebAuthnRpId(const WebEmSession& session, const request& req)
		{
			const char* hostHeader = request::get_req_header(&req, "Host");
			if (hostHeader != nullptr)
			{
				std::string host(hostHeader);
				// Strip port number if present (e.g., "localhost:8080" -> "localhost")
				auto colonPos = host.find(':');
				if (colonPos != std::string::npos)
					host = host.substr(0, colonPos);
				// For IPv6 addresses in brackets like [::1], strip brackets
				if (!host.empty() && host.front() == '[')
				{
					auto bracketEnd = host.find(']');
					if (bracketEnd != std::string::npos)
						host = host.substr(1, bracketEnd - 1);
				}
				if (!host.empty())
					return host;
			}
			return session.local_host;
		}

		// Parse a User-Agent string into a concise "OS / Browser" description.
		static std::string ParseUserAgent(const std::string& ua)
		{
			if (ua.empty())
				return "Unknown device";

			// Detect OS
			std::string os;
			if (ua.find("iPhone") != std::string::npos || ua.find("iPad") != std::string::npos)
				os = "iOS";
			else if (ua.find("Android") != std::string::npos)
				os = "Android";
			else if (ua.find("Windows NT") != std::string::npos)
				os = "Windows";
			else if (ua.find("Macintosh") != std::string::npos || ua.find("Mac OS X") != std::string::npos)
				os = "macOS";
			else if (ua.find("X11") != std::string::npos || ua.find("Linux") != std::string::npos)
				os = "Linux";
			else
				os = "Unknown OS";

			// Detect browser — check Edge before Chrome since Edge UA also contains "Chrome"
			std::string browser;
			if (ua.find("Edg/") != std::string::npos)
				browser = "Edge";
			else if (ua.find("Firefox/") != std::string::npos)
				browser = "Firefox";
			else if (ua.find("Chrome/") != std::string::npos)
				browser = "Chrome";
			else if (ua.find("Safari/") != std::string::npos)
				browser = "Safari";
			else
				browser = "Unknown browser";

			return os + " / " + browser;
		}

		} // anonymous namespace

		// ---------------------------------------------------------------------------
		// Cmd_HasPasskeys
		// ---------------------------------------------------------------------------
		void CWebServer::Cmd_HasPasskeys(WebEmSession& session, const request& req, Json::Value& root)
		{
			root["status"] = "OK";
			root["title"] = "haspasskeys";
			root["hasPasskeys"] = HasAnyPasskeys();
		}

		// ---------------------------------------------------------------------------
		// Cmd_GetMyPasskeys
		// ---------------------------------------------------------------------------
		void CWebServer::Cmd_GetMyPasskeys(WebEmSession& session, const request& req, Json::Value& root)
		{
			root["status"] = "ERR";
			root["title"] = "getmypasskeys";

			if (session.username.empty())
			{
				session.reply_status = reply::forbidden;
				return;
			}

			int iUser = FindUser(session.username.c_str());
			if (iUser == -1)
			{
				session.reply_status = reply::forbidden;
				return;
			}

			Json::Value passkeys = ParsePasskeys(m_users[iUser].Passkeys);
			Json::Value result(Json::arrayValue);
			for (Json::ArrayIndex i = 0; i < passkeys.size(); i++)
			{
				Json::Value entry;
				entry["id"]      = passkeys[i]["id"];
				entry["name"]    = passkeys[i]["name"];
				entry["created"] = passkeys[i]["created"];
				if (passkeys[i].isMember("device"))
					entry["device"] = passkeys[i]["device"].asString();
				result.append(entry);
			}
			root["result"] = result;
			root["status"] = "OK";
		}

		// ---------------------------------------------------------------------------
		// Cmd_DeletePasskey
		// ---------------------------------------------------------------------------
		void CWebServer::Cmd_DeletePasskey(WebEmSession& session, const request& req, Json::Value& root)
		{
			root["status"] = "ERR";
			root["title"] = "deletepasskey";
			if (session.username.empty())
			{
				session.reply_status = reply::forbidden;
				return;
			}

			std::string credentialId = request::findValue(&req, "credentialId");
			if (credentialId.empty())
			{
				session.reply_status = reply::bad_request;
				root["message"] = "Missing credentialId";
				return;
			}

			int iUser = FindUser(session.username.c_str());
			if (iUser == -1)
			{
				session.reply_status = reply::forbidden;
				return;
			}

			// Verify the credential belongs to this user
			Json::Value passkeys = ParsePasskeys(m_users[iUser].Passkeys);
			bool found = false;
			for (Json::ArrayIndex i = 0; i < passkeys.size(); i++)
			{
				if (passkeys[i]["id"].asString() == credentialId)
				{
					found = true;
					break;
				}
			}
			if (!found)
			{
				session.reply_status = reply::bad_request;
				root["message"] = "Credential not found for this user";
				return;
			}

			if (!RemovePasskeyFromUser(m_users[iUser].ID, credentialId))
			{
				session.reply_status = reply::internal_server_error;
				root["message"] = "Failed to delete passkey";
				return;
			}

			_log.Log(LOG_STATUS, "Passkey deleted for user '%s' (credentialId: %.16s...)", session.username.c_str(), credentialId.c_str());
			root["status"] = "OK";
		}

		// ---------------------------------------------------------------------------
		// Cmd_RegisterPasskeyBegin
		// ---------------------------------------------------------------------------
		void CWebServer::Cmd_RegisterPasskeyBegin(WebEmSession& session, const request& req, Json::Value& root)
		{
			root["title"] = "registerpasskey-begin";
			root["status"] = "ERR";
			if (session.username.empty())
			{
				session.reply_status = reply::forbidden;
				return;
			}

			int iUser = FindUser(session.username.c_str());
			if (iUser == -1)
			{
				session.reply_status = reply::forbidden;
				return;
			}

			// Generate 32-byte random challenge
			uint8_t challengeBytes[32];
			if (RAND_bytes(challengeBytes, sizeof(challengeBytes)) != 1)
			{
				session.reply_status = reply::internal_server_error;
				root["message"] = "Failed to generate challenge";
				return;
			}
			std::string challengeB64 = base64url_encode_buf(challengeBytes, sizeof(challengeBytes));

			// Store challenge in map
			{
				std::lock_guard<std::mutex> lock(m_webauthn_mutex);
				WebAuthnChallenge wac;
				wac.challenge = challengeB64;
				wac.userID    = std::to_string(m_users[iUser].ID);
				wac.created   = mytime(nullptr);
				m_webauthn_challenges[session.id] = wac;
			}

			// Build excludeCredentials from existing passkeys
			Json::Value excludeCredentials(Json::arrayValue);
			Json::Value existingPasskeys = ParsePasskeys(m_users[iUser].Passkeys);
			for (Json::ArrayIndex i = 0; i < existingPasskeys.size(); i++)
			{
				Json::Value cred;
				cred["type"] = "public-key";
				cred["id"]   = existingPasskeys[i]["id"];
				excludeCredentials.append(cred);
			}

			// User ID for WebAuthn = base64url of the numeric user ID as string
			std::string userIdStr = std::to_string(m_users[iUser].ID);
			std::string userIdB64 = base64url_encode(userIdStr);

			root["status"]              = "OK";
			root["challenge"]           = challengeB64;
			root["timeout"]             = 300000;
			root["attestation"]         = "none";
			root["excludeCredentials"]  = excludeCredentials;

			Json::Value rp;
			rp["name"] = "Domoticz";
			rp["id"]   = GetWebAuthnRpId(session, req);
			root["rp"] = rp;

			Json::Value user;
			user["id"]          = userIdB64;
			user["name"]        = m_users[iUser].Username;
			user["displayName"] = m_users[iUser].Username;
			root["user"] = user;

			Json::Value pubKeyCredParams(Json::arrayValue);
			Json::Value alg1, alg2;
			alg1["type"] = "public-key"; alg1["alg"] = -7;    // ES256
			alg2["type"] = "public-key"; alg2["alg"] = -257;  // RS256
			pubKeyCredParams.append(alg1);
			pubKeyCredParams.append(alg2);
			root["pubKeyCredParams"] = pubKeyCredParams;

			Json::Value authenticatorSelection;
			authenticatorSelection["residentKey"]       = "preferred";
			authenticatorSelection["userVerification"]  = "required";
			root["authenticatorSelection"] = authenticatorSelection;
		}

		// ---------------------------------------------------------------------------
		// Cmd_RegisterPasskeyComplete
		// ---------------------------------------------------------------------------
		void CWebServer::Cmd_RegisterPasskeyComplete(WebEmSession& session, const request& req, Json::Value& root)
		{
			root["status"]  = "ERR";
			root["title"]  = "registerpasskey-complete";
			if (session.username.empty())
			{
				session.reply_status = reply::forbidden;
				return;
			}

			int iUser = FindUser(session.username.c_str());
			if (iUser == -1)
			{
				session.reply_status = reply::forbidden;
				return;
			}

			// Retrieve and validate the stored challenge
			std::string storedChallenge;
			{
				std::lock_guard<std::mutex> lock(m_webauthn_mutex);
				auto it = m_webauthn_challenges.find(session.id);
				if (it == m_webauthn_challenges.end())
				{
					root["message"] = "No pending challenge for this session";
					session.reply_status = reply::bad_request;
					return;
				}
				time_t now = mytime(nullptr);
				if (now - it->second.created > 300)
				{
					m_webauthn_challenges.erase(it);
					root["message"] = "Challenge expired";
					session.reply_status = reply::unauthorized;
					return;
				}
				storedChallenge = it->second.challenge;
				m_webauthn_challenges.erase(it);
			}

			// Read request parameters
			std::string clientDataJSONb64  = request::findValue(&req, "clientDataJSON");
			std::string attestationObjb64  = request::findValue(&req, "attestationObject");
			std::string credentialId       = request::findValue(&req, "credentialId");
			std::string credentialName     = request::findValue(&req, "credentialName");

			if (clientDataJSONb64.empty() || attestationObjb64.empty() || credentialId.empty())
			{
				root["status"]  = "ERR";
				root["message"] = "Missing required parameters";
				session.reply_status = reply::bad_request;
				return;
			}
			if (credentialName.empty())
				credentialName = "Passkey";

			// Decode and verify clientDataJSON
			std::string clientDataJSONraw = base64url_decode(clientDataJSONb64);
			if (clientDataJSONraw.empty())
			{
				root["status"]  = "ERR";
				root["message"] = "Invalid clientDataJSON encoding";
				session.reply_status = reply::internal_server_error;
				return;
			}
			Json::Value clientData;
			if (!ParseJSon(clientDataJSONraw, clientData))
			{
				root["status"]  = "ERR";
				root["message"] = "Failed to parse clientDataJSON";
				session.reply_status = reply::internal_server_error;
				return;
			}
			if (clientData["type"].asString() != "webauthn.create")
			{
				root["status"]  = "ERR";
				root["message"] = "Invalid clientDataJSON type";
				session.reply_status = reply::bad_request;
				return;
			}
			if (clientData["challenge"].asString() != storedChallenge)
			{
				root["status"]  = "ERR";
				root["message"] = "Challenge mismatch";
				session.reply_status = reply::bad_request;
				return;
			}
			// Origin check – log mismatch but don't reject (users may access via different URLs)
			{
				std::string rpId = GetWebAuthnRpId(session, req);
				std::string origin = clientData["origin"].asString();
				if (origin.find(rpId) == std::string::npos)
				{
					_log.Log(LOG_STATUS, "WebAuthn registration: origin '%s' does not contain expected host '%s' (non-fatal)",
					         origin.c_str(), rpId.c_str());
				}
			}

			// Decode attestationObject (CBOR)
			std::string attObjRaw = base64url_decode(attestationObjb64);
			if (attObjRaw.empty())
			{
				root["status"]  = "ERR";
				root["message"] = "Invalid attestationObject encoding";
				session.reply_status = reply::internal_server_error;
				return;
			}
			std::vector<uint8_t> attObjBuf(attObjRaw.begin(), attObjRaw.end());

			// Extract authData from CBOR attestation object
			std::vector<uint8_t> authData;
			if (!cbor_extract_authdata(attObjBuf, authData))
			{
				root["status"]  = "ERR";
				root["message"] = "Failed to parse attestationObject CBOR";
				session.reply_status = reply::internal_server_error;
				return;
			}

			// Parse authData binary structure
			// [rpIdHash(32)] [flags(1)] [signCount(4)] [aaguid(16)] [credIdLen(2)] [credId(credIdLen)] [credPublicKey(...)]
			if (authData.size() < 37)
			{
				root["status"]  = "ERR";
				root["message"] = "authData too short";
				session.reply_status = reply::internal_server_error;
				return;
			}

			// Verify rpIdHash = SHA-256(rpId)
			{
				uint8_t expectedHash[SHA256_DIGEST_LENGTH];
				std::string rpId = GetWebAuthnRpId(session, req);
				SHA256(reinterpret_cast<const uint8_t*>(rpId.data()), rpId.size(), expectedHash);
				if (memcmp(authData.data(), expectedHash, SHA256_DIGEST_LENGTH) != 0)
				{
					_log.Log(LOG_STATUS, "WebAuthn registration: rpIdHash mismatch (non-fatal, host may differ)");
				}
			}

			// Verify UP flag (bit 0 of byte 32)
			uint8_t flags = authData[32];
			if (!(flags & 0x01))
			{
				root["status"]  = "ERR";
				root["message"] = "User presence flag not set";
				session.reply_status = reply::bad_request;
				return;
			}

			// Check AT flag (bit 6) – attested credential data present
			if (!(flags & 0x40))
			{
				root["status"]  = "ERR";
				root["message"] = "Attested credential data not present";
				session.reply_status = reply::bad_request;
				return;
			}

			size_t adPos = 37; // after rpIdHash(32) + flags(1) + signCount(4)

			// aaguid: 16 bytes
			if (adPos + 16 + 2 > authData.size())
			{
				root["status"]  = "ERR";
				root["message"] = "authData truncated before credIdLen";
				session.reply_status = reply::internal_server_error;
				return;
			}
			adPos += 16;

			// credIdLen: 2 bytes big-endian
			uint16_t credIdLen = (uint16_t(authData[adPos]) << 8) | authData[adPos + 1];
			adPos += 2;

			if (adPos + credIdLen > authData.size())
			{
				root["status"]  = "ERR";
				root["message"] = "authData truncated in credId";
				session.reply_status = reply::internal_server_error;
				return;
			}
			adPos += credIdLen;

			// Remaining bytes = credentialPublicKey (COSE)
			if (adPos >= authData.size())
			{
				root["status"]  = "ERR";
				root["message"] = "No public key data in authData";
				session.reply_status = reply::bad_request;
				return;
			}
			std::vector<uint8_t> credPubKeyBytes(authData.begin() + adPos, authData.end());

			// Base64-encode the COSE public key for storage
			std::string pubKeyB64 = base64_encode_buf(credPubKeyBytes.data(), (unsigned int)credPubKeyBytes.size());

			// Capture device/browser info from the User-Agent header
			std::string deviceInfo;
			const char* ua = request::get_req_header(&req, "User-Agent");
			if (ua != nullptr)
				deviceInfo = ParseUserAgent(std::string(ua));

			// Store the passkey
			if (!AddPasskeyToUser(m_users[iUser].ID, credentialId, pubKeyB64, credentialName, deviceInfo))
			{
				root["status"]  = "ERR";
				root["message"] = "Failed to store passkey";
				session.reply_status = reply::internal_server_error;
				return;
			}

			_log.Log(LOG_STATUS, "Passkey registered for user '%s' (credentialId: %.16s...)", session.username.c_str(), credentialId.c_str());
			root["status"] = "OK";
		}

		// ---------------------------------------------------------------------------
		// Cmd_PasskeyLoginBegin
		// ---------------------------------------------------------------------------
		void CWebServer::Cmd_PasskeyLoginBegin(WebEmSession& session, const request& req, Json::Value& root)
		{
			// Generate 32-byte random challenge
			uint8_t challengeBytes[32];
			if (RAND_bytes(challengeBytes, sizeof(challengeBytes)) != 1)
			{
				root["status"]  = "ERR";
				root["message"] = "Failed to generate challenge";
				return;
			}
			std::string challengeB64 = base64url_encode_buf(challengeBytes, sizeof(challengeBytes));

			// Store challenge (no userID at this point – we find the user during complete)
			{
				std::lock_guard<std::mutex> lock(m_webauthn_mutex);
				WebAuthnChallenge wac;
				wac.challenge = challengeB64;
				wac.userID    = "";
				wac.created   = mytime(nullptr);
				m_webauthn_challenges[session.id] = wac;
			}

			// Collect all credential IDs from all users
			Json::Value allowCredentials(Json::arrayValue);
			for (const auto& user : m_users)
			{
				if (user.Passkeys.empty()) continue;
				Json::Value passkeys = ParsePasskeys(user.Passkeys);
				for (Json::ArrayIndex i = 0; i < passkeys.size(); i++)
				{
					Json::Value cred;
					cred["type"] = "public-key";
					cred["id"]   = passkeys[i]["id"];
					allowCredentials.append(cred);
				}
			}

			root["status"]           = "OK";
			root["title"]            = "passkeylogin-begin";
			root["challenge"]        = challengeB64;
			root["timeout"]          = 300000;
			root["rpId"]             = GetWebAuthnRpId(session, req);
			root["userVerification"] = "required";
			root["allowCredentials"] = allowCredentials;
		}

		// ---------------------------------------------------------------------------
		// Cmd_PasskeyLoginComplete
		// ---------------------------------------------------------------------------
		void CWebServer::Cmd_PasskeyLoginComplete(WebEmSession& session, const request& req, Json::Value& root)
		{
			// Retrieve and validate the stored challenge
			std::string storedChallenge;
			{
				std::lock_guard<std::mutex> lock(m_webauthn_mutex);
				auto it = m_webauthn_challenges.find(session.id);
				if (it == m_webauthn_challenges.end())
				{
					root["status"]  = "ERR";
					root["message"] = "No pending challenge for this session";
					return;
				}
				time_t now = mytime(nullptr);
				if (now - it->second.created > 300)
				{
					m_webauthn_challenges.erase(it);
					root["status"]  = "ERR";
					root["message"] = "Challenge expired";
					return;
				}
				storedChallenge = it->second.challenge;
				m_webauthn_challenges.erase(it);
			}

			// Read request parameters
			std::string credentialId      = request::findValue(&req, "credentialId");
			std::string authenticatorDatab64 = request::findValue(&req, "authenticatorData");
			std::string clientDataJSONb64    = request::findValue(&req, "clientDataJSON");
			std::string signatureb64         = request::findValue(&req, "signature");
			std::string rememberme           = request::findValue(&req, "rememberme");

			if (credentialId.empty() || authenticatorDatab64.empty() || clientDataJSONb64.empty() || signatureb64.empty())
			{
				root["status"]  = "ERR";
				root["message"] = "Missing required parameters";
				return;
			}

			// Find user by credential ID
			int iUser = FindUserByPasskeyCredentialID(credentialId);
			if (iUser == -1)
			{
				_log.Log(LOG_ERROR, "Passkey login failed from %s: unknown credentialId", session.remote_host.c_str());
				root["status"]  = "ERR";
				root["message"] = "Unknown credential";
				return;
			}

			// Find the specific passkey entry
			Json::Value passkeys = ParsePasskeys(m_users[iUser].Passkeys);
			Json::Value passkeyEntry;
			bool found = false;
			for (Json::ArrayIndex i = 0; i < passkeys.size(); i++)
			{
				if (passkeys[i]["id"].asString() == credentialId)
				{
					passkeyEntry = passkeys[i];
					found = true;
					break;
				}
			}
			if (!found)
			{
				root["status"]  = "ERR";
				root["message"] = "Credential not found";
				return;
			}

			// Decode clientDataJSON
			std::string clientDataJSONraw = base64url_decode(clientDataJSONb64);
			if (clientDataJSONraw.empty())
			{
				root["status"]  = "ERR";
				root["message"] = "Invalid clientDataJSON encoding";
				return;
			}
			Json::Value clientData;
			if (!ParseJSon(clientDataJSONraw, clientData))
			{
				root["status"]  = "ERR";
				root["message"] = "Failed to parse clientDataJSON";
				return;
			}
			if (clientData["type"].asString() != "webauthn.get")
			{
				root["status"]  = "ERR";
				root["message"] = "Invalid clientDataJSON type";
				return;
			}
			if (clientData["challenge"].asString() != storedChallenge)
			{
				root["status"]  = "ERR";
				root["message"] = "Challenge mismatch";
				return;
			}
			// Origin check – log but don't reject
			{
				std::string rpId = GetWebAuthnRpId(session, req);
				std::string origin = clientData["origin"].asString();
				if (origin.find(rpId) == std::string::npos)
				{
					_log.Log(LOG_STATUS, "WebAuthn login: origin '%s' does not contain expected host '%s' (non-fatal)",
					         origin.c_str(), rpId.c_str());
				}
			}

			// Decode authenticatorData
			std::string authDataRaw = base64url_decode(authenticatorDatab64);
			if (authDataRaw.size() < 37)
			{
				root["status"]  = "ERR";
				root["message"] = "authenticatorData too short";
				return;
			}
			std::vector<uint8_t> authDataBytes(authDataRaw.begin(), authDataRaw.end());

			// Verify rpIdHash
			{
				uint8_t expectedHash[SHA256_DIGEST_LENGTH];
				std::string rpId = GetWebAuthnRpId(session, req);
				SHA256(reinterpret_cast<const uint8_t*>(rpId.data()), rpId.size(), expectedHash);
				if (memcmp(authDataBytes.data(), expectedHash, SHA256_DIGEST_LENGTH) != 0)
				{
					_log.Log(LOG_STATUS, "WebAuthn login: rpIdHash mismatch (non-fatal, host may differ)");
				}
			}

			// Verify UP flag
			if (!(authDataBytes[32] & 0x01))
			{
				root["status"]  = "ERR";
				root["message"] = "User presence flag not set";
				return;
			}

			// Extract signCount (bytes 33-36, big-endian)
			uint32_t newSignCount = (uint32_t(authDataBytes[33]) << 24) |
			                        (uint32_t(authDataBytes[34]) << 16) |
			                        (uint32_t(authDataBytes[35]) << 8)  |
			                         uint32_t(authDataBytes[36]);

			// Compute hash of clientDataJSON
			uint8_t clientDataHash[SHA256_DIGEST_LENGTH];
			SHA256(reinterpret_cast<const uint8_t*>(clientDataJSONraw.data()), clientDataJSONraw.size(), clientDataHash);

			// signedData = authenticatorData || hash
			std::vector<uint8_t> signedData(authDataBytes);
			signedData.insert(signedData.end(), clientDataHash, clientDataHash + SHA256_DIGEST_LENGTH);

			// Decode the stored COSE public key (base64-encoded in passkeyEntry["key"])
			std::string pubKeyB64 = passkeyEntry["key"].asString();
			std::string pubKeyRaw = base64_decode(pubKeyB64);
			if (pubKeyRaw.empty())
			{
				root["status"]  = "ERR";
				root["message"] = "Invalid stored public key";
				return;
			}
			std::vector<uint8_t> coseBuf(pubKeyRaw.begin(), pubKeyRaw.end());

			CoseKey coseKey;
			if (!cbor_parse_cose_key(coseBuf, coseKey))
			{
				root["status"]  = "ERR";
				root["message"] = "Failed to parse COSE public key";
				return;
			}

			// Decode the signature
			std::string sigRaw = base64url_decode(signatureb64);
			if (sigRaw.empty())
			{
				root["status"]  = "ERR";
				root["message"] = "Invalid signature encoding";
				return;
			}

			bool sigValid = false;

			if (coseKey.alg == -7) // ES256 – ECDSA-P256-SHA256
			{
				if (coseKey.x.size() != 32 || coseKey.y.size() != 32)
				{
					root["status"]  = "ERR";
					root["message"] = "Invalid EC key coordinates";
					return;
				}

				// Build uncompressed EC point: 0x04 || X || Y
				std::vector<uint8_t> pubPoint;
				pubPoint.reserve(65);
				pubPoint.push_back(0x04);
				pubPoint.insert(pubPoint.end(), coseKey.x.begin(), coseKey.x.end());
				pubPoint.insert(pubPoint.end(), coseKey.y.begin(), coseKey.y.end());

				OSSL_PARAM_BLD* bld = OSSL_PARAM_BLD_new();
				if (!bld)
				{
					root["status"]  = "ERR";
					root["message"] = "Failed to create EC key";
					return;
				}
				OSSL_PARAM_BLD_push_utf8_string(bld, OSSL_PKEY_PARAM_GROUP_NAME, "prime256v1", 0);
				OSSL_PARAM_BLD_push_octet_string(bld, OSSL_PKEY_PARAM_PUB_KEY, pubPoint.data(), pubPoint.size());
				OSSL_PARAM* params = OSSL_PARAM_BLD_to_param(bld);
				OSSL_PARAM_BLD_free(bld);

				EVP_PKEY_CTX* pctx = EVP_PKEY_CTX_new_from_name(nullptr, "EC", nullptr);
				EVP_PKEY* pkey = nullptr;
				if (!pctx || EVP_PKEY_fromdata_init(pctx) != 1 || EVP_PKEY_fromdata(pctx, &pkey, EVP_PKEY_PUBLIC_KEY, params) != 1)
				{
					EVP_PKEY_CTX_free(pctx);
					OSSL_PARAM_free(params);
					root["status"]  = "ERR";
					root["message"] = "Failed to set EC public key";
					return;
				}
				EVP_PKEY_CTX_free(pctx);
				OSSL_PARAM_free(params);

				EVP_MD_CTX* mdCtx = EVP_MD_CTX_new();
				if (mdCtx &&
				    EVP_DigestVerifyInit(mdCtx, nullptr, EVP_sha256(), nullptr, pkey) == 1 &&
				    EVP_DigestVerifyUpdate(mdCtx, signedData.data(), signedData.size()) == 1 &&
				    EVP_DigestVerifyFinal(mdCtx, reinterpret_cast<const uint8_t*>(sigRaw.data()), sigRaw.size()) == 1)
				{
					sigValid = true;
				}
				EVP_MD_CTX_free(mdCtx);
				EVP_PKEY_free(pkey);
			}
			else if (coseKey.alg == -257) // RS256 – RSASSA-PKCS1-v1_5-SHA256
			{
				if (coseKey.n.empty() || coseKey.e.empty())
				{
					root["status"]  = "ERR";
					root["message"] = "Invalid RSA key parameters";
					return;
				}

				BIGNUM* bnN = BN_bin2bn(coseKey.n.data(), (int)coseKey.n.size(), nullptr);
				BIGNUM* bnE = BN_bin2bn(coseKey.e.data(), (int)coseKey.e.size(), nullptr);
				if (!bnN || !bnE)
				{
					BN_free(bnN);
					BN_free(bnE);
					root["status"]  = "ERR";
					root["message"] = "Failed to create RSA BIGNUMs";
					return;
				}

				OSSL_PARAM_BLD* bld = OSSL_PARAM_BLD_new();
				if (!bld)
				{
					BN_free(bnN);
					BN_free(bnE);
					root["status"]  = "ERR";
					root["message"] = "Failed to create RSA key";
					return;
				}
				OSSL_PARAM_BLD_push_BN(bld, OSSL_PKEY_PARAM_RSA_N, bnN);
				OSSL_PARAM_BLD_push_BN(bld, OSSL_PKEY_PARAM_RSA_E, bnE);
				OSSL_PARAM* params = OSSL_PARAM_BLD_to_param(bld);
				OSSL_PARAM_BLD_free(bld);
				BN_free(bnN);
				BN_free(bnE);

				EVP_PKEY_CTX* pctx = EVP_PKEY_CTX_new_from_name(nullptr, "RSA", nullptr);
				EVP_PKEY* pkey = nullptr;
				if (!pctx || EVP_PKEY_fromdata_init(pctx) != 1 || EVP_PKEY_fromdata(pctx, &pkey, EVP_PKEY_PUBLIC_KEY, params) != 1)
				{
					EVP_PKEY_CTX_free(pctx);
					OSSL_PARAM_free(params);
					root["status"]  = "ERR";
					root["message"] = "Failed to create RSA key";
					return;
				}
				EVP_PKEY_CTX_free(pctx);
				OSSL_PARAM_free(params);

				EVP_MD_CTX* mdCtx = EVP_MD_CTX_new();
				if (!mdCtx)
				{
					EVP_PKEY_free(pkey);
					root["status"]  = "ERR";
					root["message"] = "Failed to create MD context";
					return;
				}
				if (EVP_DigestVerifyInit(mdCtx, nullptr, EVP_sha256(), nullptr, pkey) == 1 &&
				    EVP_DigestVerifyUpdate(mdCtx, signedData.data(), signedData.size()) == 1 &&
				    EVP_DigestVerifyFinal(mdCtx, reinterpret_cast<const uint8_t*>(sigRaw.data()), sigRaw.size()) == 1)
				{
					sigValid = true;
				}
				EVP_MD_CTX_free(mdCtx);
				EVP_PKEY_free(pkey);
			}
			else
			{
				root["status"]  = "ERR";
				root["message"] = "Unsupported key algorithm";
				return;
			}

			if (!sigValid)
			{
				_log.Log(LOG_ERROR, "Passkey login: signature verification failed from %s for user '%s'",
				         session.remote_host.c_str(), m_users[iUser].Username.c_str());
				root["status"]  = "ERR";
				root["message"] = "Signature verification failed";
				return;
			}

			// Verify sign count (reject replay if counter is not zero and hasn't advanced)
			uint32_t storedCount = passkeyEntry["cnt"].asUInt();
			if (storedCount > 0 && newSignCount <= storedCount)
			{
				_log.Log(LOG_ERROR, "Passkey login: sign count replay detected for user '%s' (stored=%u, new=%u)",
				         m_users[iUser].Username.c_str(), storedCount, newSignCount);
				root["status"]  = "ERR";
				root["message"] = "Sign count replay detected";
				return;
			}

			// Update sign count
			UpdatePasskeySignCount(m_users[iUser].ID, credentialId, newSignCount);

			// Create session
			_log.Log(LOG_STATUS, "Passkey login successful from %s for user '%s'",
			         session.remote_host.c_str(), m_users[iUser].Username.c_str());
			root["status"]  = "OK";
			root["version"] = szAppVersion;
			root["title"]   = "passkeylogin-complete";
			session.isnew     = true;
			session.username  = m_users[iUser].Username;
			session.rights    = m_users[iUser].userrights;
			session.rememberme = (rememberme == "true");
			root["user"]   = session.username;
			root["rights"] = session.rights;
		}

		void CWebServer::Cmd_GetHardwareTypes(WebEmSession& session, const request& req, Json::Value& root)
		{
			if (session.rights != URIGHTS_ADMIN)
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
#ifndef WITH_OPENZWAVE
				if (ii == HTYPE_OpenZWave)
					bDoAdd = false;
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
				(htype == HTYPE_Tesla) || (htype == HTYPE_Mercedes) || (htype == HTYPE_Netatmo))
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
					|| (htype == HTYPE_SysfsGpio) || (htype == HTYPE_OpenWebNetTCP) || (htype == HTYPE_Daikin) || (htype == HTYPE_DaikinModbus) || (htype == HTYPE_PythonPlugin) || (htype == HTYPE_RaspberryPCF8574)
					|| (htype == HTYPE_OpenWebNetUSB) || (htype == HTYPE_IntergasInComfortLAN2RF) || (htype == HTYPE_EnphaseAPI) || (htype == HTYPE_EcoCompteur) || (htype == HTYPE_Meteorologisk) || (htype == HTYPE_OpenMeteo)
					|| (htype == HTYPE_AirconWithMe) || (htype == HTYPE_EneverPriceFeeds) || (htype == HTYPE_Tado) || (htype == HTYPE_Matter))
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

		static bool ValidateSettingsJSON(const std::string &settings)
		{
			if (settings.empty())
				return true;
			if (settings.size() > 65536)
			{
				_log.Log(LOG_ERROR, "WebServer: Settings JSON exceeds 64KB limit");
				return false;
			}
			Json::Value settingsJson;
			if (!ParseJSon(settings, settingsJson) || !settingsJson.isObject())
			{
				_log.Log(LOG_ERROR, "WebServer: Settings is not valid JSON");
				return false;
			}
			for (const auto &key : settingsJson.getMemberNames())
			{
				if (settingsJson[key].asString().size() > 4096)
				{
					_log.Log(LOG_ERROR, "WebServer: Settings value for '%s' exceeds 4KB limit", key.c_str());
					return false;
				}
			}
			return true;
		}

#ifdef ENABLE_PYTHON
		// Parse a plugin manifest XML once: extract the <plugin key="..."> attribute and the set of
		// field names declared as password fields. The password attribute is matched case-insensitively
		// ("true"/"TRUE"/"1") so a manifest typo does not silently expose a secret. Pure: operates on
		// the manifest XML string, no I/O. keyOut is empty when the manifest has no key attribute.
		static void ParsePluginManifest(const std::string &manifestXml, std::string &keyOut, std::set<std::string> &passwordFieldsOut)
		{
			keyOut.clear();
			passwordFieldsOut.clear();
			TiXmlDocument xmlDoc;
			xmlDoc.Parse(manifestXml.c_str());
			if (xmlDoc.Error())
				return;
			TiXmlNode *pPluginNode = xmlDoc.FirstChild("plugin");
			if (!pPluginNode)
				return;
			TiXmlElement *pPluginEle = pPluginNode->ToElement();
			if (!pPluginEle)
				return;
			const char *pKey = pPluginEle->Attribute("key");
			if (pKey)
				keyOut = pKey;
			TiXmlNode *pParamsNode = pPluginNode->FirstChild("params");
			if (!pParamsNode)
				return;
			auto isPasswordAttr = [](const char *pPassword) -> bool {
				if (!pPassword)
					return false;
				std::string v(pPassword);
				for (auto &c : v)
					c = (char)tolower((unsigned char)c);
				return (v == "true" || v == "1");
			};
			auto checkParam = [&](TiXmlElement *pEle) {
				const char *pField = pEle->Attribute("field");
				if (pField && isPasswordAttr(pEle->Attribute("password")))
					passwordFieldsOut.insert(pField);
			};
			for (TiXmlNode *pChild = pParamsNode->FirstChild(); pChild; pChild = pChild->NextSibling())
			{
				TiXmlElement *pEle = pChild->ToElement();
				if (!pEle)
					continue;
				std::string tagName = pEle->Value();
				if (tagName == "param")
					checkParam(pEle);
				else if (tagName == "group")
				{
					for (TiXmlNode *pGroupChild = pEle->FirstChild("param"); pGroupChild; pGroupChild = pGroupChild->NextSibling("param"))
					{
						TiXmlElement *pGroupEle = pGroupChild->ToElement();
						if (pGroupEle)
							checkParam(pGroupEle);
					}
				}
			}
		}

		// Build a map from each plugin's manifest "key" attribute (which matches the Hardware.Extra
		// column) to its set of password field names. GetManifest() is keyed by plugin DIRECTORY, not by
		// the key attribute, so it must be walked and re-keyed. Only plugins that declare at least one
		// password field appear.
		static std::map<std::string, std::set<std::string>> BuildPluginPasswordFieldsByKey()
		{
			std::map<std::string, std::set<std::string>> byKey;
			Plugins::CPluginSystem pluginSystem;
			for (const auto &manifest : *pluginSystem.GetManifest())
			{
				std::string key;
				std::set<std::string> fields;
				ParsePluginManifest(manifest.second, key, fields);
				if (!key.empty() && !fields.empty())
					byKey[key] = fields;
			}
			return byKey;
		}

		// Pure merge for the plugin Settings write path. Given the incoming Settings JSON (from the edit
		// form, where password fields were stripped to empty on the read side), the currently stored
		// Settings JSON, and the plugin's password field names, return the JSON to persist. A password
		// field that arrives empty or missing keeps its stored value ("leave blank to keep"). Fails
		// graceful: never wipes a readable stored secret, never throws.
		static std::string MergePluginSettingsPreservePasswords(const std::string &incomingSettings, const std::string &storedSettings, const std::set<std::string> &passwordFields)
		{
			if (passwordFields.empty())
				return incomingSettings;

			Json::Value incoming;
			bool incomingOk = !incomingSettings.empty() && ParseJSon(incomingSettings, incoming) && incoming.isObject();
			Json::Value stored;
			bool storedOk = !storedSettings.empty() && ParseJSon(storedSettings, stored) && stored.isObject();

			// Incoming unusable (blank/invalid): preserve everything rather than wipe stored secrets.
			if (!incomingOk)
			{
				if (!incomingSettings.empty())
					_log.Log(LOG_ERROR, "WebServer: incoming plugin Settings not parseable on save; preserving stored values");
				return storedOk ? storedSettings : incomingSettings;
			}
			// Stored unreadable: nothing recoverable to preserve; keep the incoming values as-is. A
			// corrupt stored blob is already lost; warn so it is not silent.
			if (!storedOk)
			{
				if (!storedSettings.empty())
					_log.Log(LOG_ERROR, "WebServer: stored plugin Settings not parseable on save; cannot preserve existing secrets");
				return incomingSettings;
			}

			for (const auto &field : passwordFields)
			{
				bool blank = !incoming.isMember(field) || incoming[field].asString().empty();
				if (blank && stored.isMember(field))
					incoming[field] = stored[field];
			}
			return JSonToRawString(incoming);
		}
#endif

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
			std::string settings = CURLEncode::URLDecode(request::findValue(&req, "settings"));
			if ((name.empty()) || (senabled.empty()) || (shtype.empty()))
				return;
			_eHardwareTypes htype = (_eHardwareTypes)atoi(shtype.c_str());

			stdstring_trim(username);
			stdstring_trim(password);
			int iDataTimeout = atoi(sdatatimeout.c_str());

			if (!ValidateSettingsJSON(settings))
				return;

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
			else if (htype == HTYPE_BleBox)
			{
				mode1 = 60;
				mode2 = 0;
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
					"DataTimeout, Settings) VALUES ('%q',%d, %d, %d,'%q',%d,'%q','%q','%q','%q','%q','%q', '%q', '%q', '%q', '%q', %d, '%q')",
					name.c_str(), (senabled == "true") ? 1 : 0, htype, iLogLevelEnabled, address.c_str(), port, sport.c_str(), username.c_str(), password.c_str(),
					extra.c_str(), mode1Str.c_str(), mode2Str.c_str(), mode3Str.c_str(), mode4Str.c_str(), mode5Str.c_str(), mode6Str.c_str(), iDataTimeout,
					settings.c_str());
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
				g_McpPush.onDeviceTableChanged();
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
			std::string settings = CURLEncode::URLDecode(request::findValue(&req, "settings"));

			if ((name.empty()) || (senabled.empty()) || (shtype.empty()))
				return;

			stdstring_trim(username);
			stdstring_trim(password);

			if (!ValidateSettingsJSON(settings))
				return;

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

			if (htype == HTYPE_Netatmo && extra == "") {
				//Extra contains  private data (client sectret), and is not sent to the front-end because of security reason
				//Avoid overwriting existing datas
				std::vector<std::vector<std::string>> result;
				result = m_sql.safe_query("SELECT Extra FROM Hardware WHERE ID=%q", idx.c_str());
				if (!result.empty())
					extra = result[0][0];
			}

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
#ifdef ENABLE_PYTHON
					{
						// Preserve custom password fields left blank on save ("leave blank to keep").
						// Extra holds the plugin key; a password field submitted empty keeps its stored value.
						std::map<std::string, std::set<std::string>> pluginPasswordFields = BuildPluginPasswordFieldsByKey();
						auto itPwd = pluginPasswordFields.find(extra);
						if (itPwd != pluginPasswordFields.end() && !itPwd->second.empty())
						{
							std::vector<std::vector<std::string>> storedRes = m_sql.safe_query("SELECT Settings FROM Hardware WHERE ID=%q", idx.c_str());
							std::string storedSettings = storedRes.empty() ? "" : storedRes[0][0];
							settings = MergePluginSettingsPreservePasswords(settings, storedSettings, itPwd->second);
						}
					}
#endif
					m_sql.safe_query("UPDATE Hardware SET Name='%q', Enabled=%d, Type=%d, LogLevel=%d, Address='%q', Port=%d, SerialPort='%q', Username='%q', Password='%q', "
						"Extra='%q', Mode1='%q', Mode2='%q', Mode3='%q', Mode4='%q', Mode5='%q', Mode6='%q', DataTimeout=%d, Settings='%q' WHERE (ID == '%q')",
						name.c_str(), (senabled == "true") ? 1 : 0, htype, iLogLevelEnabled, address.c_str(), port, sport.c_str(), username.c_str(), password.c_str(),
						extra.c_str(), mode1Str.c_str(), mode2Str.c_str(), mode3Str.c_str(), mode4Str.c_str(), mode5Str.c_str(), mode6Str.c_str(), iDataTimeout,
						settings.c_str(), idx.c_str());
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
			root["status"] = "ERR";
			root["title"] = "GetDeviceValueOptions";
			if (session.rights != URIGHTS_ADMIN)
			{
				session.reply_status = reply::forbidden;
				return; // Only admin user allowed
			}
			std::string idx = request::findValue(&req, "idx");
			if (idx.empty())
			{
				session.reply_status = reply::bad_request;
				return;
			}
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
		}

		void CWebServer::Cmd_GetDeviceValueOptionWording(WebEmSession& session, const request& req, Json::Value& root)
		{
			root["status"] = "ERR";
			root["title"] = "GetDeviceValueOptions";
			if (session.rights != URIGHTS_ADMIN)
			{
				session.reply_status = reply::forbidden;
				return; // Only admin user allowed
			}
			std::string idx = request::findValue(&req, "idx");
			std::string pos = request::findValue(&req, "pos");
			if ((idx.empty()) || (pos.empty()))
			{
				session.reply_status = reply::bad_request;
				return;
			}
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
		}

		static bool ValidateUserVariableParams(const std::string& variablename, std::string& variabletype, const std::string& variablevalue, std::string& errorMessage)
		{
			if (variablename.empty())
			{
				errorMessage = "Missing variable name (vname)";
				return false;
			}
			if (variabletype.empty())
			{
				errorMessage = "Missing variable type (vtype)";
				return false;
			}
			if (!std::isdigit((unsigned char)variabletype[0]))
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
					errorMessage = "Invalid variabletype " + variabletype;
					return false;
				}
			}
			if ((variabletype != "0") && (variabletype != "1") && (variabletype != "2") && (variabletype != "3") && (variabletype != "4"))
			{
				errorMessage = "Invalid variabletype " + variabletype;
				return false;
			}
			if ((variablevalue.empty()) && (variabletype != "2"))
			{
				errorMessage = "Missing variable value (vvalue) for variabletype " + variabletype;
				return false;
			}
			return true;
		}

		void CWebServer::Cmd_AddUserVariable(WebEmSession& session, const request& req, Json::Value& root)
		{
			root["title"] = "AddUserVariable";
			root["status"] = "ERR";
			if (session.rights != URIGHTS_ADMIN)
			{
				session.reply_status = reply::forbidden;
				_log.Log(LOG_ERROR, "User: %s tried to add a uservariable!", session.username.c_str());
				return; // Only admin user allowed
			}
			std::string variablename = HTMLSanitizer::Sanitize(request::findValue(&req, "vname"));
			std::string variablevalue = HTMLSanitizer::Sanitize(request::findValue(&req, "vvalue"));
			std::string variabletype = request::findValue(&req, "vtype");

			std::string errorMessage;
			if (!ValidateUserVariableParams(variablename, variabletype, variablevalue, errorMessage))
			{
				root["message"] = errorMessage;
				session.reply_status = reply::bad_request;
				return;
			}

			if (!m_sql.AddUserVariable(variablename, (const _eUsrVariableType)atoi(variabletype.c_str()), variablevalue, errorMessage))
			{
				root["message"] = errorMessage;
				session.reply_status = reply::internal_server_error;
				return;
			}
			root["status"] = "OK";
		}

		void CWebServer::Cmd_DeleteUserVariable(WebEmSession& session, const request& req, Json::Value& root)
		{
			root["title"] = "DeleteUserVariable";
			root["status"] = "ERR";
			if (session.rights != URIGHTS_ADMIN)
			{
				_log.Log(LOG_ERROR, "User: %s tried to delete a uservariable!", session.username.c_str());
				session.reply_status = reply::forbidden;
				return; // Only admin user allowed
			}
			std::string idx = request::findValue(&req, "idx");
			if (idx.empty())
			{
				session.reply_status = reply::bad_request;
				return;
			}

			m_sql.DeleteUserVariable(idx);
			root["status"] = "OK";
		}

		void CWebServer::Cmd_UpdateUserVariable(WebEmSession& session, const request& req, Json::Value& root)
		{
			root["title"] = "UpdateUserVariable";
			root["status"] = "ERR";
			if (session.rights != URIGHTS_ADMIN)
			{
				_log.Log(LOG_ERROR, "User: %s tried to update a uservariable!", session.username.c_str());
				session.reply_status = reply::forbidden;
				return; // Only admin user allowed
			}

			std::string idx = request::findValue(&req, "idx");
			std::string variablename = HTMLSanitizer::Sanitize(request::findValue(&req, "vname"));
			std::string variablevalue = HTMLSanitizer::Sanitize(request::findValue(&req, "vvalue"));
			std::string variabletype = request::findValue(&req, "vtype");

			std::string errorMessage;
			if (!ValidateUserVariableParams(variablename, variabletype, variablevalue, errorMessage))
			{
				root["message"] = errorMessage;
				session.reply_status = reply::bad_request;
				return;
			}

			std::vector<std::vector<std::string>> result;
			if (idx.empty())
			{
				result = m_sql.safe_query("SELECT ID FROM UserVariables WHERE Name='%q'", variablename.c_str());
				if (result.empty())
				{
					root["message"] = "Uservariable " + variablename + " does not exist";
					session.reply_status = reply::bad_request;
					return;
				}
				idx = result[0][0];
			}

			result = m_sql.safe_query("SELECT Name, ValueType FROM UserVariables WHERE ID='%q'", idx.c_str());
			if (result.empty())
			{
				root["message"] = "Uservariable " + variablename + " does not exist";
				session.reply_status = reply::bad_request;
				return;
			}

			bool bTypeNameChanged = false;
			if (variablename != result[0][0])
				bTypeNameChanged = true; // new name
			else if (variabletype != result[0][1])
				bTypeNameChanged = true; // new type

			if (!m_sql.UpdateUserVariable(idx, variablename, (const _eUsrVariableType)atoi(variabletype.c_str()), variablevalue, !bTypeNameChanged, errorMessage))
			{
				root["message"] = errorMessage;
				session.reply_status = reply::bad_request;
				return;
			}

			if (bTypeNameChanged)
			{
				if (m_sql.m_bEnableEventSystem)
					m_mainworker.m_eventsystem.GetCurrentUserVariables();
			}
			root["status"] = "OK";
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

			const int iVarID = atoi(idx.c_str());

			auto result = m_sql.safe_query("SELECT ID, Name, ValueType, Value, LastUpdate FROM UserVariables WHERE (ID==%d)", iVarID);
			if (!result.empty())
			{
				//gizmocuz, this should now have been an array [0], but maybe some users expect it now
				auto sd = result[0];
				root["result"][0]["idx"] = sd[0];
				root["result"][0]["Name"] = sd[1];
				root["result"][0]["Type"] = sd[2];
				root["result"][0]["Value"] = sd[3];
				root["result"][0]["LastUpdate"] = sd[4];
				root["status"] = "OK";
				root["title"] = "GetUserVariable";
			}
		}

		void CWebServer::Cmd_AllowNewHardware(WebEmSession& session, const request& req, Json::Value& root)
		{
			if (session.rights != URIGHTS_ADMIN)
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
			if (session.rights != URIGHTS_ADMIN)
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
			g_McpPush.onDeviceTableChanged();
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
			if (session.rights != URIGHTS_ADMIN)
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
			if (session.rights != URIGHTS_ADMIN)
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
			if (session.rights != URIGHTS_ADMIN)
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
				"WHERE (T2.[ID]==T1.[HardwareID]) ORDER BY T2.[Name], T1.[Name]");
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
			if (session.rights != URIGHTS_ADMIN)
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
			if (session.rights != URIGHTS_ADMIN)
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
			if (session.rights != URIGHTS_ADMIN)
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
			if (session.rights != URIGHTS_ADMIN)
			{
				session.reply_status = reply::forbidden;
				return;
			}
			std::string planid = request::findValue(&req, "planid");
			std::string sorder = request::findValue(&req, "order");
			if (planid.empty() || sorder.empty())
				return;

			std::stringstream ss(sorder);
			std::string token;
			int pos = 1;
			while (std::getline(ss, token, ','))
			{
				if (!token.empty())
				{
					m_sql.safe_query("UPDATE DeviceToPlansMap SET [Order] = %d WHERE (ID='%q') AND (PlanID='%q')", pos, token.c_str(), planid.c_str());
					++pos;
				}
			}

			root["status"] = "OK";
			root["title"] = "ChangePlanDeviceOrder";
		}

		void CWebServer::Cmd_ChangePlanFullOrder(WebEmSession& session, const request& req, Json::Value& root)
		{
			if (session.rights != URIGHTS_ADMIN)
			{
				session.reply_status = reply::forbidden;
				return;
			}
			std::string sorder = request::findValue(&req, "order");
			if (sorder.empty())
				return;

			std::stringstream ss(sorder);
			std::string token;
			int pos = 1;
			while (std::getline(ss, token, ','))
			{
				if (!token.empty())
				{
					m_sql.safe_query("UPDATE Plans SET [Order] = %d WHERE (ID='%q')", pos, token.c_str());
					++pos;
				}
			}

			root["status"] = "OK";
			root["title"] = "ChangePlanFullOrder";
		}

		void CWebServer::Cmd_GetVersion(WebEmSession& session, const request& req, Json::Value& root)
		{
			root["status"] = "OK";
			root["title"] = "GetVersion";
			if (session.rights != URIGHTS_NONE)
			{
				root["version"] = szAppVersion;
				root["hash"] = szAppHash;
				root["build_time"] = szAppDate;
				CdzVents* dzvents = CdzVents::GetInstance();
				root["dzvents_version"] = dzvents->GetVersion();
				root["python_version"] = szPyVersion;
				root["UseUpdate"] = false;
				root["HaveUpdate"] = m_mainworker.IsUpdateAvailable(false);
				root["ThemeSettingsAPI"] = CThemeSettings::API_VERSION;

				if (session.rights == URIGHTS_ADMIN)
				{
					root["UseUpdate"] = g_bUseUpdater;
					root["DomoticzUpdateURL"] = m_mainworker.m_szDomoticzUpdateURL;
					root["SystemName"] = m_mainworker.m_szSystemName;
					root["Revision"] = m_mainworker.m_iRevision;
					auto dbresult = m_sql.safe_query(
						"SELECT (SELECT page_count FROM pragma_page_count()) * (SELECT page_size FROM pragma_page_size())");
					if (!dbresult.empty())
						root["db_size"] = (Json::Int64)atoll(dbresult[0][0].c_str());
				}
			}
		}

		void CWebServer::Cmd_GetAuth(WebEmSession& session, const request& req, Json::Value& root)
		{
			root["status"] = "OK";
			root["title"] = "GetAuth";
			root["canlogout"] = !session.istrustednetwork || !session.id.empty();
			if (session.rights != URIGHTS_NONE)
			{
				root["user"] = session.username;
				root["rights"] = session.rights;
				root["version"] = szAppVersion;
			}
		}

		void CWebServer::Cmd_GetSetupRequired(WebEmSession& session, const request& req, Json::Value& root)
		{
			root["status"] = "OK";
			root["title"] = "GetSetupRequired";
			root["SetupRequired"] = !FindAdminUser();
		}

		void CWebServer::Cmd_SetupWizardCreateAdmin(WebEmSession& session, const request& req, Json::Value& root)
		{
			root["title"] = "SetupWizardCreateAdmin";
			root["status"] = "ERR";

			static std::mutex setupMutex;
			std::lock_guard<std::mutex> lock(setupMutex);

			// Security: only allow when no admin user exists
			if (FindAdminUser())
			{
				_log.Log(LOG_ERROR, "Setup wizard attempt blocked: admin account already exists (IP: %s)", session.remote_host.c_str());
				session.reply_status = reply::bad_request;
				root["message"] = "Setup has already been completed";
				return;
			}

			std::string username = CURLEncode::URLDecode(request::findValue(&req, "username"));
			std::string password = CURLEncode::URLDecode(request::findValue(&req, "password"));

			if (username.empty() || password.empty())
			{
				session.reply_status = reply::bad_request;
				root["message"] = "Username and password are required";
				return;
			}

			if (username.length() > 128)
			{
				session.reply_status = reply::bad_request;
				root["message"] = "Username is too long";
				return;
			}

			// Username is sent as plaintext, we base64 encode for storage
			// Password is sent as MD5 hash from the frontend (same as login flow)
			m_sql.safe_query(
				"INSERT INTO Users (Active, Username, Password, Rights, TabsEnabled) VALUES (1, '%q', '%q', %d, 0x1F)",
				base64_encode(username).c_str(), password.c_str(), http::server::URIGHTS_ADMIN);

			_log.Log(LOG_STATUS, "Admin user '%s' created via setup wizard", username.c_str());

			// Reload users so the new admin is immediately available for login
			LoadUsers();

			root["status"] = "OK";
		}

		void CWebServer::Cmd_GetMyProfile(WebEmSession& session, const request& req, Json::Value& root)
		{
			root["status"] = "ERR";
			root["title"] = "GetMyProfile";
			if (session.rights == URIGHTS_NONE)	// Viewer cannot change his profile
			{
				session.reply_status = reply::forbidden;
				return;
			}

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

		void CWebServer::Cmd_UpdateMyProfile(WebEmSession& session, const request& req, Json::Value& root)
		{
			root["status"] = "ERR";
			root["title"] = "UpdateMyProfile";

			if (req.method != "POST" || session.rights == URIGHTS_NONE)	// Viewer cannot change his profile
			{
				session.reply_status = reply::forbidden;
				return;
			}

			std::string sUsername = request::findValue(&req, "username");
			int iUser = FindUser(session.username.c_str());
			if (iUser == -1)
			{
				root["error"] = "User not found!";
				session.reply_status = reply::bad_request;
				return;
			}
			if (m_users[iUser].Username != sUsername)
			{
				root["error"] = "User mismatch!";
				session.reply_status = reply::bad_request;
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
					session.reply_status = reply::unauthorized;
					return;
				}
			}

			std::string sTotpsecret = request::findValue(&req, "totpsecret");
			std::string sTotpCode = request::findValue(&req, "totpcode");
			bool bEnablemfa = (request::findValue(&req, "enablemfa") == "true" ? true : false);
			if (bEnablemfa && sTotpsecret.empty())
			{
				root["error"] = "Not a valid TOTP secret!";
				session.reply_status = reply::unauthorized;
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
							session.reply_status = reply::unauthorized;
							return;
						}
					}
				}
			}
			m_users[iUser].Mfatoken = sTotpsecret;
			m_sql.safe_query("UPDATE Users SET MFAsecret='%q' WHERE (ID=%d)", sTotpsecret.c_str(), m_users[iUser].ID);

			// Update dashboard type preference (bit 7 of TabsEnabled)
			std::string sUseDynamicDashboard = request::findValue(&req, "usedynamicdashboard");
			if (!sUseDynamicDashboard.empty())
			{
				auto result2 = m_sql.safe_query("SELECT TabsEnabled FROM Users WHERE (ID=%d)", m_users[iUser].ID);
				if (!result2.empty())
				{
					int tabsEnabled = atoi(result2[0][0].c_str());
					if (sUseDynamicDashboard == "true")
						tabsEnabled |= (1 << 7);
					else
						tabsEnabled &= ~(1 << 7);
					m_sql.safe_query("UPDATE Users SET TabsEnabled=%d WHERE (ID=%d)", tabsEnabled, m_users[iUser].ID);
				}
			}

			LoadUsers();
			root["status"] = "OK";
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
			if (szStartupFolder != szUserDataFolder)
			{
				historyfile = szStartupFolder + "History.txt";
			}

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
			ExtraHeaders.push_back("Type: " + std::string(!bIsBetaChannel ? "Stable" : "Beta"));

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
			root["CurrencySign"] = m_sql.m_currencysign;
			root["PriceResolution"] = m_sql.m_PriceResolution.load();

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
				int bEnableTabDashboardDynamic = 0;

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
					bEnableTabDashboardDynamic = (TabsEnabled & (1 << 7));
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
				root["result"]["EnableTabDashboardDynamic"] = bEnableTabDashboardDynamic != 0;

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

		void CWebServer::Cmd_GetForecastConfig(WebEmSession& session, const request& req, Json::Value& root)
		{
			if (session.rights == URIGHTS_NONE)
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

			if (session.rights == URIGHTS_VIEWER || session.rights == URIGHTS_NONE)
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
			std::string ohid = request::findValue(&req, "ohid");
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
				result = m_sql.safe_query("SELECT HardwareID, OrgHardwareID, DeviceID, Unit, Type, SubType FROM DeviceStatus WHERE (ID=='%q')", idx.c_str());
				if (result.empty())
					return;
				hid = result[0][0];
				ohid = result[0][1];
				did = result[0][2];
				dunit = result[0][3];
				dtype = result[0][4];
				dsubtype = result[0][5];
			}

			int HardwareID = atoi(hid.c_str());
			int OrgHardwareID = atoi(ohid.c_str());
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
			if (m_mainworker.UpdateDevice(HardwareID, OrgHardwareID, DeviceID, unit, devType, subType, invalue, svalue, szUpdateUser, signallevel, batterylevel, parseTrigger))
			{
				root["status"] = "OK";
				root["title"] = "Update Device";
			}
		}

		void CWebServer::Cmd_UpdateDevices(WebEmSession& session, const request& req, Json::Value& root)
		{
			if (session.rights == URIGHTS_VIEWER || session.rights == URIGHTS_NONE)
			{
				session.reply_status = reply::forbidden;
				return; // only user or higher allowed
			}

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
			if (session.rights == URIGHTS_VIEWER || session.rights == URIGHTS_NONE)
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
			if (session.rights != URIGHTS_ADMIN)
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
			if (session.rights != URIGHTS_ADMIN)
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
			if (session.rights != URIGHTS_ADMIN)
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
			if (session.rights != URIGHTS_ADMIN)
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
			// run script in new session with setsid + nohup for complete detachment from parent
			// Use fixed log filename for frontend display (both scripts write to same file)
			// Remove any existing log first: a root-owned log from a prior run would be
			// unwritable if domoticz is now running as a non-root user, silently preventing
			// the script from starting.
			std::string logfile = std::string(szStartupFolder) + "update.log";
			std::string lscript = "rm -f " + logfile + " 2>/dev/null; setsid nohup " + scriptname + " > " + logfile + " 2>&1 &";
			int ret = system(lscript.c_str());
			_log.Log(LOG_STATUS, "Update script started: %s (log: update.log)", scriptname.c_str());
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

			int iP1Hardware = m_mainworker.FindDomoticzHardwareByType(HTYPE_P1SmartMeter);
			if (iP1Hardware == -1)
				iP1Hardware = m_mainworker.FindDomoticzHardwareByType(HTYPE_P1SmartMeterLAN);
			if (iP1Hardware != -1)
			{
				P1MeterBase* pP1Meter = dynamic_cast<P1MeterBase*>(m_mainworker.GetHardware(iP1Hardware));
				if (pP1Meter != nullptr)
				{
					root["P1_Tariff"] = (pP1Meter->m_current_tariff == 1) ? "Low" : "High";
				}
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

			if (session.rights != URIGHTS_ADMIN)
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

		void CWebServer::Cmd_DeleteDateRange(WebEmSession& session, const request& req, Json::Value& root)
		{
			if (session.rights != URIGHTS_ADMIN)
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
			if (session.rights != URIGHTS_ADMIN)
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
			if (session.rights != URIGHTS_ADMIN)
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

				m_sql.UpdatePreferencesVar("HourIdxElectricityDevice", atoi(request::findValue(&req, "HourIdxElectricityDevice").c_str())); cntSettings++;
				m_sql.UpdatePreferencesVar("HourIdxGasDevice", atoi(request::findValue(&req, "HourIdxGasDevice").c_str())); cntSettings++;
				m_sql.UpdatePreferencesVar("P1DisplayType", atoi(request::findValue(&req, "P1DisplayType").c_str())); cntSettings++;
			int iPriceResolution = atoi(request::findValue(&req, "PriceResolution").c_str());
			if (iPriceResolution != 15 && iPriceResolution != 30 && iPriceResolution != 60)
				iPriceResolution = 60;
			m_sql.m_PriceResolution = iPriceResolution;
			m_sql.UpdatePreferencesVar("PriceResolution", iPriceResolution); cntSettings++;


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

				m_mainworker.HandleHourPrice();

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

				m_sql.m_bLogUnusedSensors = (request::findValue(&req, "LogUnusedSensors") == "on" ? 1 : 0);
				m_sql.UpdatePreferencesVar("LogUnusedSensors", m_sql.m_bLogUnusedSensors); cntSettings++;

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

				std::string sCurrency = request::findValue(&req, "CurrencySymbol");
				m_sql.UpdatePreferencesVar("Currency", sCurrency); cntSettings++;

				bool AllowPlainBasicAuth = (request::findValue(&req, "AllowPlainBasicAuth") == "on" ? 1 : 0);
				m_sql.UpdatePreferencesVar("AllowPlainBasicAuth", AllowPlainBasicAuth);

				m_webservers.SetAllowPlainBasicAuth(AllowPlainBasicAuth);
				cntSettings++;

				std::string WebLocalNetworks = CURLEncode::URLDecode(request::findValue(&req, "WebLocalNetworks"));
				m_sql.UpdatePreferencesVar("WebLocalNetworks", WebLocalNetworks);
				m_webservers.ReloadTrustedNetworks();
				cntSettings++;

				std::string sProxyHeaderFamily = request::findValue(&req, "WebProxyHeaderFamily");
				if (!sProxyHeaderFamily.empty())
				{
					int iProxyHeaderFamily = atoi(sProxyHeaderFamily.c_str());
					if ((iProxyHeaderFamily >= static_cast<int>(ProxyHeaderFamily::None)) && (iProxyHeaderFamily <= static_cast<int>(ProxyHeaderFamily::XRealIP)))
					{
						int iCurrentFamily = static_cast<int>(ProxyHeaderFamily::XForwardedFor);
						m_sql.GetPreferencesVar("WebProxyHeaderFamily", iCurrentFamily);
						m_sql.UpdatePreferencesVar("WebProxyHeaderFamily", iProxyHeaderFamily);
						// Applied when the server is constructed; the running servers read
						// this from their own settings copy on the io threads, so it is not
						// safe to mutate it underneath them here.
						if (iCurrentFamily != iProxyHeaderFamily)
							_log.Log(LOG_STATUS, "Proxy forwarded-header setting changed, restart Domoticz to apply it");
						cntSettings++;
					}
				}

				std::string WebAllowedCORSOrigins = CURLEncode::URLDecode(request::findValue(&req, "WebAllowedCORSOrigins"));
				m_sql.UpdatePreferencesVar("WebAllowedCORSOrigins", WebAllowedCORSOrigins);
				cntSettings++;
				int WebCORSAllowTrustedNetworks = (request::findValue(&req, "WebCORSAllowTrustedNetworks") == "on" ? 1 : 0);
				m_sql.UpdatePreferencesVar("WebCORSAllowTrustedNetworks", WebCORSAllowTrustedNetworks);
				cntSettings++;
				m_webservers.ReloadCorsPolicy();
				if (WebAllowedCORSOrigins.find('*') != std::string::npos)
					_log.Log(LOG_STATUS, "SECURITY RISK! CORS origin '*' is configured: every website can call the API from a browser on a trusted network! Restrict 'Allowed CORS origins' in Settings/Security to specific origins.");

				if (session.username.empty())
				{
					// Local network could be changed so lets force a check here
					session.rights = URIGHTS_NONE;
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
					m_mainworker.m_scheduler.HandleTimerPlanSwitch();
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

				//Energy Dashboard Settings
				int EP1 = atoi(request::findValue(&req, "EP1").c_str());
				int EGas = atoi(request::findValue(&req, "EGas").c_str());
				int EWater = atoi(request::findValue(&req, "EWater").c_str());
				int ESolar = atoi(request::findValue(&req, "ESolar").c_str());
				int EBatteryWatt = atoi(request::findValue(&req, "EBatteryWatt").c_str());
				int EBatterySoc = atoi(request::findValue(&req, "EBatterySoc").c_str());
				int EBatteryVolt = atoi(request::findValue(&req, "EBatteryVolt").c_str());
				int EBatteryEnergyIn = atoi(request::findValue(&req, "EBatteryEnergyIn").c_str());
				int EBatteryEnergyOut = atoi(request::findValue(&req, "EBatteryEnergyOut").c_str());
				int ETextSensor = atoi(request::findValue(&req, "ETextSensor").c_str());
				int EOutsideTempSensor = atoi(request::findValue(&req, "EOutsideTempSensor").c_str());
				int EExtra1 = atoi(request::findValue(&req, "EExtra1").c_str());
				int EExtra2 = atoi(request::findValue(&req, "EExtra2").c_str());
				int EExtra3 = atoi(request::findValue(&req, "EExtra3").c_str());
				std::string EExtra1Field = request::findValue(&req, "EExtra1Field");
				std::string EExtra2Field = request::findValue(&req, "EExtra2Field");
				std::string EExtra3Field = request::findValue(&req, "EExtra3Field");
				std::string EExtra1Icon = request::findValue(&req, "EExtra1Icon");
				std::string EExtra2Icon = request::findValue(&req, "EExtra2Icon");
				std::string EExtra3Icon = request::findValue(&req, "EExtra3Icon");

				bool bConvertWaterM3ToLiter = (request::findValue(&req, "EConvertWaterM3ToLiter") == "on" ? 1 : 0);
				bool bDisplayTime = (request::findValue(&req, "EDisplayTime") == "on" ? 1 : 0);
				bool bDisplayOutsideTemp = (request::findValue(&req, "EDisplayOutsideTemp") == "on" ? 1 : 0);
				bool bDisplayFlowWithLines = (request::findValue(&req, "EDisplayFlowWithLines") == "on" ? 1 : 0);
				bool bUseCustomIcons = (request::findValue(&req, "EUseCustomIcons") == "on" ? 1 : 0);

				Json::Value ESettings;
				ESettings["idP1"] = EP1;
				ESettings["idGas"] = EGas;
				ESettings["idWater"] = EWater;
				ESettings["idSolar"] = ESolar;
				ESettings["idBatteryWatt"] = EBatteryWatt;
				ESettings["idBatterySoc"] = EBatterySoc;
				ESettings["idBatteryVolt"] = EBatteryVolt;
				ESettings["idBatteryEnergyIn"] = EBatteryEnergyIn;
				ESettings["idBatteryEnergyOut"] = EBatteryEnergyOut;
				ESettings["idTextSensor"] = ETextSensor;
				ESettings["idOutsideTempSensor"] = EOutsideTempSensor;
				ESettings["idExtra1"] = EExtra1;
				ESettings["idExtra2"] = EExtra2;
				ESettings["idExtra3"] = EExtra3;
				ESettings["Extra1Field"] = EExtra1Field;
				ESettings["Extra2Field"] = EExtra2Field;
				ESettings["Extra3Field"] = EExtra3Field;
				ESettings["Extra1Icon"] = EExtra1Icon;
				ESettings["Extra2Icon"] = EExtra2Icon;
				ESettings["Extra3Icon"] = EExtra3Icon;

				ESettings["ConvertWaterM3ToLiter"] = bConvertWaterM3ToLiter;
				ESettings["DisplayTime"] = bDisplayTime;
				ESettings["DisplayOutsideTemp"] = bDisplayOutsideTemp;
				ESettings["DisplayFlowWithLines"] = bDisplayFlowWithLines;
				ESettings["UseCustomIcons"] = bUseCustomIcons;

				std::string szESettings = JSonToRawString(ESettings);
				m_sql.UpdatePreferencesVar("ESettings", szESettings);

				m_sql.SetUnitsAndScale();

				/* To wrap up everything */
				m_notifications.ConfigFromGetvars(req, true);
				m_notifications.LoadConfig();

#ifdef ENABLE_PYTHON
				// Signal plugins to update Settings dictionary
				PluginLoadConfig();
#endif

				std::string sDebugLevel = request::findValue(&req, "DebugLevel");
				if (!sDebugLevel.empty())
				{
					uint32_t iDebugLevel = static_cast<uint32_t>(atoi(sDebugLevel.c_str()));
					_log.SetDebugFlags(iDebugLevel);
					if (iDebugLevel != 0)
					{
						// Enable debug log level when any debug flags are set
						_log.SetLogFlags(_log.GetLogFlags() | LOG_DEBUG_INT);
					}
					else
					{
						// Disable debug log level when no debug flags are set
						_log.SetLogFlags(_log.GetLogFlags() & ~LOG_DEBUG_INT);
					}
					cntSettings++;
				}

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
			if (session.rights != URIGHTS_ADMIN)
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
			g_McpPush.onDeviceTableChanged();
		}

		void CWebServer::Cmd_AddScene(WebEmSession& session, const request& req, Json::Value& root)
		{
			root["title"] = "AddScene";
			root["status"] = "ERR";
			if (session.rights != URIGHTS_ADMIN)
			{
				session.reply_status = reply::forbidden;
				return; // Only admin user allowed
			}

			std::string name = HTMLSanitizer::Sanitize(request::findValue(&req, "name"));
			name = HTMLSanitizer::Sanitize(name);
			if (name.empty())
			{
				session.reply_status = reply::bad_request;
				root["message"] = "No Scene Name specified!";
				return;
			}
			std::string stype = request::findValue(&req, "scenetype");
			if (stype.empty())
			{
				session.reply_status = reply::bad_request;
				root["message"] = "No Scene Type specified!";
				return;
			}
			if (m_sql.DoesSceneByNameExits(name) == true)
			{
				session.reply_status = reply::bad_request;
				root["message"] = "A Scene with this Name already Exits!";
				return;
			}
			m_sql.safe_query("INSERT INTO Scenes (Name,SceneType) VALUES ('%q',%d)", name.c_str(), atoi(stype.c_str()));
			if (m_sql.m_bEnableEventSystem)
			{
				m_mainworker.m_eventsystem.GetCurrentScenesGroups();
			}
			root["status"] = "OK";
		}

		void CWebServer::Cmd_DeleteScene(WebEmSession& session, const request& req, Json::Value& root)
		{
			root["title"] = "DeleteScene";
			root["status"] = "ERR";
			if (session.rights != URIGHTS_ADMIN)
			{
				session.reply_status = reply::forbidden;
				return; // Only admin user allowed
			}

			std::string idx = CURLEncode::URLDecode(request::findValue(&req, "idx"));
			if (idx.empty())
			{
				session.reply_status = reply::bad_request;
				return;
			}
			m_sql.DeleteScenes(idx);
			root["status"] = "OK";
		}

		void CWebServer::Cmd_UpdateScene(WebEmSession& session, const request& req, Json::Value& root)
		{
			root["title"] = "UpdateScene";
			root["status"] = "ERR";
			if (session.rights != URIGHTS_ADMIN)
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
			{
				session.reply_status = reply::bad_request;
				return;
			}
			std::string stype = request::findValue(&req, "scenetype");
			if (stype.empty())
			{
				session.reply_status = reply::bad_request;
				root["message"] = "No Scene Type specified!";
				return;
			}
			std::string tmpstr = request::findValue(&req, "protected");
			int iProtected = (tmpstr == "true") ? 1 : 0;

			std::string onaction = base64_decode(request::findValue(&req, "onaction"));
			std::string offaction = base64_decode(request::findValue(&req, "offaction"));

			m_sql.safe_query("UPDATE Scenes SET Name='%q', Description='%q', SceneType=%d, Protected=%d, OnAction='%q', OffAction='%q' WHERE (ID == '%q')", name.c_str(),
				description.c_str(), atoi(stype.c_str()), iProtected, onaction.c_str(), offaction.c_str(), idx.c_str());
			uint64_t ullidx = std::stoull(idx);
			m_mainworker.m_eventsystem.WWWUpdateSingleState(ullidx, name, m_mainworker.m_eventsystem.REASON_SCENEGROUP);
			root["status"] = "OK";
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
				root["result"][ii]["FaClass"] = icon.FaClass;
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
			if (!rid.empty() && rid.find_first_not_of("0123456789") != std::string::npos)
				rid.clear();
			std::string planid = request::findValue(&req, "plan");
			if (!planid.empty() && planid.find_first_not_of("0123456789") != std::string::npos)
				planid.clear();

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
			bool bFilterByPlan = (!planid.empty() && planid != "0");
			if (!rid.empty())
			{
				result = m_sql.safe_query("SELECT ID, Name, Activators, Favorite, nValue, SceneType, LastUpdate, Protected, OnAction, OffAction, Description FROM Scenes WHERE (ID == %q) ORDER BY [Order]", rid.c_str());
			}
			else if (bFilterByPlan)
			{
				result = m_sql.safe_query("SELECT ID, Name, Activators, Favorite, nValue, SceneType, LastUpdate, Protected, OnAction, OffAction, Description FROM Scenes WHERE ID IN (SELECT DeviceRowID FROM DeviceToPlansMap WHERE DevSceneType=1 AND PlanID=%q) ORDER BY [Order]", planid.c_str());
			}
			else
			{
				result = m_sql.safe_query("SELECT ID, Name, Activators, Favorite, nValue, SceneType, LastUpdate, Protected, OnAction, OffAction, Description FROM Scenes ORDER BY [Order]");
			}
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
#ifdef WITH_OPENZWAVE
			m_ZW_Hwidx = -1;
#endif
			std::vector<std::vector<std::string>> result;
			result = m_sql.safe_query("SELECT ID, Name, Enabled, Type, Address, Port, SerialPort, Username, Password, Extra, Mode1, Mode2, Mode3, Mode4, Mode5, Mode6, DataTimeout, "
				"LogLevel, Settings FROM Hardware ORDER BY ID ASC");
			if (!result.empty())
			{
#ifdef ENABLE_PYTHON
				// Map plugin key -> password field names, built once, but only when the result actually
				// contains a plugin row (avoids parsing manifests for non-plugin queries).
				std::map<std::string, std::set<std::string>> pluginPasswordFields;
				{
					bool hasPlugin = false;
					for (const auto &sd : result)
						if ((_eHardwareTypes)atoi(sd[3].c_str()) == HTYPE_PythonPlugin) { hasPlugin = true; break; }
					if (hasPlugin)
						pluginPasswordFields = BuildPluginPasswordFieldsByKey();
				}
#endif
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
					if (hType == HTYPE_Netatmo) {
						root["result"][ii]["Extra"] = "";	//Don't pass the refresh token to the front-end because of security reasons
					}
					else {
						root["result"][ii]["Extra"] = sd[9];
					}

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

					if (hType == HTYPE_PythonPlugin && !sd[18].empty())
					{
						Json::Value settingsJson;
						if (ParseJSon(sd[18], settingsJson) && settingsJson.isObject())
						{
#ifdef ENABLE_PYTHON
							// Strip password-type field values so secrets never reach the browser, and
							// report which ones are set so the UI can show "leave blank to keep".
							std::string pluginKey = sd[9]; // Extra holds the plugin key
							auto itPwdFields = pluginPasswordFields.find(pluginKey);
							if (itPwdFields != pluginPasswordFields.end())
							{
								Json::Value pwdSet(Json::objectValue);
								for (const auto &field : itPwdFields->second)
								{
									if (settingsJson.isMember(field))
									{
										if (!settingsJson[field].asString().empty())
											pwdSet[field] = true;
										settingsJson[field] = "";
									}
								}
								if (!pwdSet.empty())
									root["result"][ii]["SettingsPwdSet"] = pwdSet;
							}
#else
							// Without Python support the plugin manifest is unavailable, so password
							// fields cannot be identified and stripped; do not send stored plugin
							// settings of lingering plugin rows at all.
							settingsJson = Json::objectValue;
#endif
							root["result"][ii]["Settings"] = settingsJson;
						}
						else
						{
							root["result"][ii]["Settings"] = Json::objectValue;
						}
					}
					else
					{
						root["result"][ii]["Settings"] = Json::objectValue;
					}

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
						else if (pHardware->HwdType == HTYPE_Matter)
						{
							CMatter* pMyHardware = dynamic_cast<CMatter*>(pHardware);
							root["result"][ii]["version"]   = pMyHardware->m_szSoftwareVersion;
							root["result"][ii]["Connected"] = pMyHardware->m_bConnected.load();
						}
#ifdef WITH_OPENZWAVE
						else if (pHardware->HwdType == HTYPE_OpenZWave)
						{ // Special case for openzwave (status for nodes queried)
							COpenZWave* pOZWHardware = dynamic_cast<COpenZWave*>(pHardware);
							root["result"][ii]["version"] = pOZWHardware->GetVersionLong();
							root["result"][ii]["NodesQueried"] = (pOZWHardware->m_awakeNodesQueried || pOZWHardware->m_allNodesQueried);
						}
#endif
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
			{
				session.reply_status = reply::forbidden;
				return;
			}

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
			}
			// having no users defined is a normal situation, not an error
			root["status"] = "OK";
		}

		void CWebServer::Cmd_GetApplications(WebEmSession & session, const request& req, Json::Value &root)
		{
			root["title"] = "GetApplications";
			if (session.rights != URIGHTS_ADMIN)
			{
				session.reply_status = reply::forbidden;
			}
			else
			{
				std::vector<std::vector<std::string>> result;
				result = m_sql.safe_query("SELECT ID, Active, Public, Applicationname, Secret, Pemfile, RefreshExpire, SigningSecret, LastSeen, RedirectUris FROM Applications ORDER BY ID ASC");
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
						root["result"][ii]["RefreshExpire"] = atoi(sd[6].c_str());
						root["result"][ii]["SigningSecret"] = sd[7];
						root["result"][ii]["LastSeen"] = sd[8];
						root["result"][ii]["RedirectUris"] = sd[9];
						ii++;
					}
				}
				root["status"] = "OK";
			}
		}

		void CWebServer::Cmd_AddApplication(WebEmSession & session, const request& req, Json::Value &root)
		{
			root["title"] = "AddApplication";
			if (session.rights != URIGHTS_ADMIN)
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
				std::string srefreshexpire = request::findValue(&req, "refreshexpire");
				uint32_t refreshexpire = (srefreshexpire.empty()) ? 0 : static_cast<uint32_t>(atol(srefreshexpire.c_str()));
				std::string signingsecret = request::findValue(&req, "signingsecret");
				std::string redirecturis = request::findValue(&req, "redirecturis");
				// Auto-generate signing secret if not provided
				if (signingsecret.empty())
					signingsecret = GenerateUUID();
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
				m_sql.safe_query("INSERT INTO Applications (Active, Public, Applicationname, Secret, Pemfile, RefreshExpire, SigningSecret, RedirectUris) VALUES (%d,%d,'%q','%q','%q',%u,'%q','%q')",
					(senabled == "true") ? 1 : 0, (spublic == "true") ? 1 : 0, applicationname.c_str(), secret.c_str(), pemfile.c_str(), refreshexpire, signingsecret.c_str(), redirecturis.c_str());

				// Reload the applications (and users)
				LoadUsers();
				root["status"] = "OK";
			}
		}

		void CWebServer::Cmd_UpdateApplication(WebEmSession & session, const request& req, Json::Value &root)
		{
			root["status"] = "ERR";
			root["title"] = "UpdateApplication";
			if (session.rights != URIGHTS_ADMIN)
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
				std::string srefreshexpire = request::findValue(&req, "refreshexpire");
				uint32_t refreshexpire = (srefreshexpire.empty()) ? 0 : static_cast<uint32_t>(atol(srefreshexpire.c_str()));
				std::string signingsecret = request::findValue(&req, "signingsecret");
				std::string redirecturis = request::findValue(&req, "redirecturis");
				// Auto-generate signing secret if not provided
				if (signingsecret.empty())
					signingsecret = GenerateUUID();
				if (idx.empty() || senabled.empty() || applicationname.empty() || spublic.empty())
				{
					session.reply_status = reply::bad_request;
					return;
				}
				if ((spublic != "true") && secret.empty())
				{
					root["statustext"] = "Secret's can only be empty for Public Clients!";
					session.reply_status = reply::bad_request;
					return;
				}
				if ((spublic == "true") && pemfile.empty())
				{
					root["statustext"] = "A PEM file containing private and public key must be given for Public Clients!";
					session.reply_status = reply::bad_request;
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
						session.reply_status = reply::bad_request;
						return;
					}
				}

				// Update the application
				m_sql.safe_query("UPDATE Applications SET Active=%d, Public=%d, Applicationname='%q', Secret='%q', Pemfile='%q', RefreshExpire=%u, SigningSecret='%q', RedirectUris='%q' WHERE (ID == '%q')",
					(senabled == "true") ? 1 : 0, (spublic == "true") ? 1 : 0, applicationname.c_str(), secret.c_str(), pemfile.c_str(), refreshexpire, signingsecret.c_str(), redirecturis.c_str(), idx.c_str());

				// Reload the applications (and users)
				LoadUsers();
				root["status"] = "OK";
			}
		}

		void CWebServer::Cmd_DeleteApplication(WebEmSession & session, const request& req, Json::Value &root)
		{
			root["title"] = "DeleteApplication";
			root["status"] = "ERR";

			if (session.rights != URIGHTS_ADMIN)
			{
				session.reply_status = reply::forbidden;
				return;
			}
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

		void CWebServer::Cmd_GetAccessTokens(WebEmSession& session, const request& req, Json::Value& root)
		{
			root["title"] = "GetAccessTokens";
			if (session.rights != URIGHTS_ADMIN)
			{
				session.reply_status = reply::forbidden;
				return;
			}
			auto tokens = m_sql.GetAccessTokens();
			int ii = 0;
			for (const auto& t : tokens)
			{
				root["result"][ii]["idx"] = static_cast<Json::UInt64>(t.ID);
				root["result"][ii]["Name"] = t.Name;
				root["result"][ii]["Rights"] = t.Rights;
				root["result"][ii]["Expiry"] = static_cast<Json::Int64>(t.Expiry);
				root["result"][ii]["CreatedAt"] = t.CreatedAt;
				root["result"][ii]["LastUpdate"] = t.LastUpdate;
				ii++;
			}
			root["status"] = "OK";
		}

		void CWebServer::Cmd_CreateAccessToken(WebEmSession& session, const request& req, Json::Value& root)
		{
			root["title"] = "CreateAccessToken";
			if (session.rights != URIGHTS_ADMIN)
			{
				session.reply_status = reply::forbidden;
				return;
			}
			std::string name = request::findValue(&req, "name");
			std::string srights = request::findValue(&req, "rights");
			std::string sexpiry = request::findValue(&req, "expiry"); // days: 0=never, 30, 90, 365

			if (name.empty())
			{
				root["statustext"] = "Name is required";
				return;
			}

			int rights = srights.empty() ? 0 : atoi(srights.c_str());
			if (rights < 0 || rights > 2)
				rights = 0;

			int expiryDays = sexpiry.empty() ? 0 : atoi(sexpiry.c_str());
			time_t expiry = 0;
			uint32_t exptime = 315360000; // ~10 years for "never"
			if (expiryDays > 0)
			{
				expiry = time(nullptr) + static_cast<time_t>(expiryDays) * 86400;
				exptime = static_cast<uint32_t>(expiryDays) * 86400;
			}

			// Ensure domoticzUI has a signing secret; it may be empty on fresh installs
			{
				auto signingRows = m_sql.safe_query("SELECT SigningSecret FROM Applications WHERE Applicationname='domoticzUI' AND Active=1");
				if (signingRows.empty())
				{
					root["statustext"] = "domoticzUI Application not found or inactive";
					return;
				}
				if (signingRows[0][0].empty())
				{
					m_sql.safe_query("UPDATE Applications SET SigningSecret='%q' WHERE Applicationname='domoticzUI'", GenerateUUID().c_str());
					LoadUsers();
				}
			}

			// Insert placeholder row first; JWT generation needs the assigned token ID for the subject claim
			unsigned long tokenID = 0;
			if (!m_sql.CreateAccessToken(name, rights, expiry, "placeholder", tokenID))
			{
				root["statustext"] = "Failed to create access token";
				return;
			}

			std::string jwttoken;
			std::string subject = "at:" + std::to_string(tokenID);
			Json::Value payload;
			payload["roles"][0] = rights;

			// Use a stable issuer not tied to a specific hostname; access tokens must work from any network address.
			std::string issuer = m_pWebEm->m_DigistRealm;

			try
			{
				if (!m_pWebEm->GenerateJwtToken(jwttoken, "domoticzUI", subject, exptime, payload, issuer))
				{
					m_sql.DeleteAccessToken(tokenID);
					root["statustext"] = "Failed to generate token (is domoticzUI Application active?)";
					return;
				}
			}
			catch (const std::exception&)
			{
				m_sql.DeleteAccessToken(tokenID);
				root["statustext"] = "Failed to generate token";
				return;
			}

			// Store SHA-256 hash of the raw JWT — the raw token is never persisted
			unsigned char hash[SHA256_DIGEST_LENGTH];
			SHA256(reinterpret_cast<const unsigned char*>(jwttoken.c_str()), jwttoken.size(), hash);
			char hashHex[SHA256_DIGEST_LENGTH * 2 + 1];
			for (int i = 0; i < SHA256_DIGEST_LENGTH; i++)
				snprintf(hashHex + i * 2, 3, "%02x", hash[i]);
			std::string tokenHash(hashHex);

			m_sql.safe_query("UPDATE AccessTokens SET TokenHash='%q' WHERE ID=%lu", tokenHash.c_str(), tokenID);

			// Register the new token as a synthetic user without calling LoadUsers() (which would wipe all active sessions)
			m_pWebEm->AddUserPassword(40000UL + tokenID, "at:" + std::to_string(tokenID), "", "", "",
				static_cast<_eUserRights>(rights), 0, "", "", 0, "", 0);

			root["status"] = "OK";
			root["idx"] = static_cast<Json::UInt64>(tokenID);
			root["token"] = jwttoken; // shown exactly once
		}

		void CWebServer::Cmd_DeleteAccessToken(WebEmSession& session, const request& req, Json::Value& root)
		{
			root["title"] = "DeleteAccessToken";
			if (session.rights != URIGHTS_ADMIN)
			{
				session.reply_status = reply::forbidden;
				return;
			}
			std::string sidx = request::findValue(&req, "idx");
			if (sidx.empty())
			{
				session.reply_status = reply::bad_request;
				return;
			}
			unsigned long tokenID = static_cast<unsigned long>(atol(sidx.c_str()));
			m_sql.DeleteAccessToken(tokenID);
			m_pWebEm->RemoveUserPassword(40000UL + tokenID);
			root["status"] = "OK";
		}

		void CWebServer::Cmd_GetMobiles(WebEmSession& session, const request& req, Json::Value& root)
		{
			root["status"] = "ERR";
			root["title"] = "Mobiles";

			if (session.rights != URIGHTS_ADMIN)
			{
				session.reply_status = reply::forbidden;
				return;
			}

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
			std::string szSwitchUser;
			if (iUser != -1)
			{
				szSwitchUser = m_users[iUser].Username + " (IP: " + session.remote_host + ")";
				_log.Log(LOG_STATUS, "User: %s initiated a SetPoint command", m_users[iUser].Username.c_str());
			}
			m_mainworker.SetSetPoint(idx, static_cast<float>(atof(setpoint.c_str())), szSwitchUser);
		}

		void CWebServer::Cmd_GetSceneActivations(WebEmSession& session, const request& req, Json::Value& root)
		{
			if (session.rights != URIGHTS_ADMIN)
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
			if (session.rights != URIGHTS_ADMIN)
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
			if (session.rights != URIGHTS_ADMIN)
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
			if (session.rights != URIGHTS_ADMIN)
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
			if (session.rights != URIGHTS_ADMIN)
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
					root["result"][ii]["FaClass"] = icon.FaClass;
					ii++;
				}
			}
		}

		void CWebServer::Cmd_DeleteCustomIcon(WebEmSession& session, const request& req, Json::Value& root)
		{
			if (session.rights != URIGHTS_ADMIN)
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
			if (session.rights != URIGHTS_ADMIN)
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

		void CWebServer::Cmd_UploadWebAsset(WebEmSession& session, const request& req, Json::Value& root)
		{
			root["title"] = "UploadWebAsset";
			if (session.rights != URIGHTS_ADMIN)
			{
				session.reply_status = reply::forbidden;
				return; // Only admin user allowed
			}

			std::string szName = request::findValue(&req, "name");
			std::string szData = request::findValue(&req, "data"); // base64 encoded
			std::string szURL = request::findValue(&req, "url");
			std::string szTitle = request::findValue(&req, "title"); // optional, display only

			if (szName.empty() || (szData.empty() && szURL.empty()))
			{
				root["error"] = "Missing name, and data or url";
				return;
			}
			if (!IsSafeWebAssetName(szName))
			{
				root["error"] = "Invalid asset name";
				return;
			}
			if (!IsAllowedWebAssetType(szName))
			{
				root["error"] = "Unsupported asset type";
				return;
			}

			if (szData.empty())
			{
				// The download runs in the background; the caller polls getwebassetjob
				// with the returned job id until it reports done.
				std::string szError;
				const std::string szJobID = WebAssetFetch::StartInstall(szName, szURL, szTitle, szError);
				if (szJobID.empty())
				{
					root["error"] = szError;
					return;
				}
				root["status"] = "OK";
				root["job"] = szJobID;
				root["path"] = "assets/" + szName;
				return;
			}

			if (WebAssetFetch::IsInstallRunning(szName))
			{
				root["error"] = "This library is currently being installed";
				return;
			}

			std::string szContent = base64_decode(szData);
			if (szContent.empty())
			{
				root["error"] = "Could not decode asset data";
				return;
			}
			if (szContent.size() > WEB_ASSET_MAX_SIZE)
			{
				root["error"] = "Asset too large";
				return;
			}

			if (WebAssetFetch::IsNameOwnedByOther(szName, szName))
			{
				root["error"] = "Asset file name '" + szName + "' is already used by another installed library";
				return;
			}

			if (!EnsureWebAssetFolder())
			{
				root["error"] = "Could not create assets folder";
				return;
			}

			if (!WriteWebAssetFile(szName, szContent, "UploadWebAsset"))
			{
				root["error"] = "Could not write asset";
				return;
			}
			WebAssetFetch::SetTitle(szName, szTitle);

			root["status"] = "OK";
			root["path"] = "assets/" + szName;
			root["size"] = static_cast<int>(szContent.size());   // capped well below INT_MAX
		}

		void CWebServer::Cmd_GetWebAssetJob(WebEmSession& session, const request& req, Json::Value& root)
		{
			root["title"] = "GetWebAssetJob";
			if (session.rights != URIGHTS_ADMIN)
			{
				session.reply_status = reply::forbidden;
				return; // Only admin user allowed
			}

			const std::string szJobID = request::findValue(&req, "job");
			WebAssetFetch::JobStatus status;
			if (szJobID.empty() || (szJobID.size() > 64) || !WebAssetFetch::GetJobStatus(szJobID, status))
			{
				root["error"] = "Unknown job";
				return;
			}

			root["status"] = "OK";
			root["name"] = status.szName;
			if (status.bRunning)
				root["state"] = "running";
			else if (status.bSuccess)
			{
				root["state"] = "done";
				root["path"] = "assets/" + status.szName;
			}
			else
			{
				root["state"] = "failed";
				root["error"] = status.szError.empty() ? "Could not install the library" : status.szError;
			}
		}

		void CWebServer::Cmd_GetWebAssets(WebEmSession& session, const request& req, Json::Value& root)
		{
			root["title"] = "GetWebAssets";

			// Not admin-only: every user's browser has to know which stylesheets to load.
			if (session.rights == URIGHTS_NONE)
			{
				session.reply_status = reply::forbidden;
				return;
			}

			root["status"] = "OK";

			DIR* lDir = opendir(WebAssetFolder().c_str());
			if (lDir == nullptr)
				return; // no assets stored yet — an empty result is not an error

			struct _tAssetMeta
			{
				std::string szSourceURL;
				std::string szLastUpdate;
				std::string szTitle;
			};
			std::map<std::string, _tAssetMeta> metadata;
			auto result = m_sql.safe_query("SELECT Name, SourceURL, LastUpdate, Title FROM WebAssets");
			for (const auto& sd : result)
				metadata[sd[0]] = _tAssetMeta{ sd[1], sd[2], sd[3] };

			int ii = 0;
			struct dirent* ent;
			while ((ent = readdir(lDir)) != nullptr)
			{
				const std::string szFileName = ent->d_name;
				if ((szFileName == ".") || (szFileName == ".."))
					continue;
				if (!IsAllowedWebAssetType(szFileName))
					continue;
				std::string szSourceURL;
				std::string szLastUpdate;
				std::string szTitle;
				auto itt = metadata.find(szFileName);
				if (itt != metadata.end())
				{
					szSourceURL = itt->second.szSourceURL;
					szLastUpdate = itt->second.szLastUpdate;
					szTitle = itt->second.szTitle;
				}

				root["result"][ii]["name"] = szFileName;
				root["result"][ii]["path"] = "assets/" + szFileName;
				root["result"][ii]["LastUpdate"] = szLastUpdate;
				root["result"][ii]["Title"] = szTitle;
				// Withheld from viewers: a source URL can name a host on the local network.
				if (session.rights == URIGHTS_ADMIN)
					root["result"][ii]["SourceURL"] = szSourceURL;
				ii++;
			}
			closedir(lDir);
		}

		void CWebServer::Cmd_DeleteWebAsset(WebEmSession& session, const request& req, Json::Value& root)
		{
			root["title"] = "DeleteWebAsset";
			if (session.rights != URIGHTS_ADMIN)
			{
				session.reply_status = reply::forbidden;
				return; // Only admin user allowed
			}

			std::string szName = request::findValue(&req, "name");
			if (szName.empty() || !IsSafeWebAssetName(szName) || !IsAllowedWebAssetType(szName))
			{
				root["error"] = "Invalid asset name";
				return;
			}
			if (WebAssetFetch::IsInstallRunning(szName))
			{
				root["error"] = "This library is currently being installed";
				return;
			}

			const std::string szFile = WebAssetFolder() + "/" + szName;
			if (!file_exist(szFile.c_str()))
			{
				root["error"] = "Asset not found";
				return;
			}
			if (std::remove(szFile.c_str()) != 0)
			{
				root["error"] = "Could not remove asset";
				return;
			}
			WebAssetFetch::Forget(szName);
			root["status"] = "OK";
		}

		void CWebServer::Cmd_RenameDevice(WebEmSession& session, const request& req, Json::Value& root)
		{
			if (session.rights != URIGHTS_ADMIN)
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
			if (session.rights != URIGHTS_ADMIN)
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
			if (session.rights != URIGHTS_ADMIN)
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
					session.reply_status = reply::bad_request;
					root["status"] = "ERR";
					return;
				}
			}
			root["status"] = "OK";

			_log.Log(logLevel, "%s", smessage.c_str());
		}

		void CWebServer::Cmd_FixKwhCounterSpikes(WebEmSession& session, const request& req, Json::Value& root)
		{
			root["status"] = "ERR";
			root["title"] = "FixKwhCounterSpikes";
			if (session.rights != URIGHTS_ADMIN)
			{
				session.reply_status = reply::forbidden;
				return; // Only admin user allowed
			}

			std::string sidx = request::findValue(&req, "idx");
			if (sidx.empty())
			{
				session.reply_status = reply::bad_request;
				root["message"] = "idx parameter missing";
				return;
			}
			uint64_t idx = 0;
			try
			{
				idx = std::stoull(sidx);
			}
			catch (const std::exception&)
			{
				session.reply_status = reply::bad_request;
				root["message"] = "Invalid idx format";
				return;
			}

			std::string sthreshold = request::findValue(&req, "threshold");
			double max_daily_kwh = 0.0; // 0 = auto-detect from device history
			if (!sthreshold.empty())
			{
				char* endptr = nullptr;
				double parsed = strtod(sthreshold.c_str(), &endptr);
				if (endptr != sthreshold.c_str() && parsed > 0 && parsed <= 1e6)
					max_daily_kwh = parsed;
			}

			bool dry_run = (request::findValue(&req, "dryrun") == "1");

			std::vector<std::string> results;
			bool ok = m_sql.FixKwhCounterSpikes(idx, max_daily_kwh, dry_run, results);

			root["status"] = ok ? "OK" : "ERR";
			root["dryrun"] = dry_run;
			for (int i = 0; i < static_cast<int>(results.size()); i++)
				root["result"][i] = results[i];
		}

		void CWebServer::Cmd_SpreadCounterSpike(WebEmSession& session, const request& req, Json::Value& root)
		{
			root["title"] = "SpreadCounterSpike";
			root["status"] = "ERR";
			if (session.rights != URIGHTS_ADMIN)
			{
				session.reply_status = reply::forbidden;
				return;
			}

			std::string sidx = request::findValue(&req, "idx");
			std::string sdate = request::findValue(&req, "date");
			if (sidx.empty() || sdate.empty())
			{
				session.reply_status = reply::bad_request;
				root["message"] = "idx and date parameters required";
				return;
			}

			uint64_t idx = 0;
			try { idx = std::stoull(sidx); }
			catch (const std::exception&)
			{
				session.reply_status = reply::bad_request;
				root["message"] = "Invalid idx format";
				return;
			}

			std::vector<std::string> results;
			bool ok = m_sql.SpreadCounterSpike(idx, sdate, results);
			root["status"] = ok ? "OK" : "ERR";
			for (int i = 0; i < static_cast<int>(results.size()); i++)
				root["result"][i] = results[i];
		}

		void CWebServer::Cmd_ClearShortLog(WebEmSession& session, const request& req, Json::Value& root)
		{
			if (session.rights != URIGHTS_ADMIN)
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

		void CWebServer::Cmd_PruneUnusedSensorLogs(WebEmSession& session, const request& req, Json::Value& root)
		{
			if (session.rights != URIGHTS_ADMIN)
			{
				session.reply_status = reply::forbidden;
				return;
			}
			root["status"] = "OK";
			root["title"] = "PruneUnusedSensorLogs";

			auto result = m_sql.safe_query(
				"SELECT COUNT(DISTINCT ds.ID) FROM DeviceStatus ds WHERE ds.Used=0 AND ("
				"EXISTS(SELECT 1 FROM Temperature WHERE DeviceRowID=ds.ID) OR "
				"EXISTS(SELECT 1 FROM Rain WHERE DeviceRowID=ds.ID) OR "
				"EXISTS(SELECT 1 FROM Wind WHERE DeviceRowID=ds.ID) OR "
				"EXISTS(SELECT 1 FROM UV WHERE DeviceRowID=ds.ID) OR "
				"EXISTS(SELECT 1 FROM Meter WHERE DeviceRowID=ds.ID) OR "
				"EXISTS(SELECT 1 FROM MultiMeter WHERE DeviceRowID=ds.ID) OR "
				"EXISTS(SELECT 1 FROM Percentage WHERE DeviceRowID=ds.ID) OR "
				"EXISTS(SELECT 1 FROM Fan WHERE DeviceRowID=ds.ID) OR "
				"EXISTS(SELECT 1 FROM LightingLog WHERE DeviceRowID=ds.ID) OR "
				"EXISTS(SELECT 1 FROM Temperature_Calendar WHERE DeviceRowID=ds.ID) OR "
				"EXISTS(SELECT 1 FROM Rain_Calendar WHERE DeviceRowID=ds.ID) OR "
				"EXISTS(SELECT 1 FROM Wind_Calendar WHERE DeviceRowID=ds.ID) OR "
				"EXISTS(SELECT 1 FROM UV_Calendar WHERE DeviceRowID=ds.ID) OR "
				"EXISTS(SELECT 1 FROM Meter_Calendar WHERE DeviceRowID=ds.ID) OR "
				"EXISTS(SELECT 1 FROM MultiMeter_Calendar WHERE DeviceRowID=ds.ID) OR "
				"EXISTS(SELECT 1 FROM Percentage_Calendar WHERE DeviceRowID=ds.ID) OR "
				"EXISTS(SELECT 1 FROM Fan_Calendar WHERE DeviceRowID=ds.ID)"
				")");

			int iDeviceCount = (!result.empty() && !result[0].empty()) ? atoi(result[0][0].c_str()) : 0;

			_log.Log(LOG_STATUS, "Pruning log data for unused sensors...");
			int iTotalDeleted = m_sql.PruneUnusedSensorLogs();
			_log.Log(LOG_STATUS, "Pruned %d log records for %d unused sensors", iTotalDeleted, iDeviceCount);

			root["rowsdeleted"] = iTotalDeleted;
			root["devicesaffected"] = iDeviceCount;
		}

		void CWebServer::Cmd_VacuumDatabase(WebEmSession& session, const request& req, Json::Value& root)
		{
			if (session.rights != URIGHTS_ADMIN)
			{
				session.reply_status = reply::forbidden;
				return; // Only admin user allowed
			}
			root["status"] = "OK";
			root["title"] = "VacuumDatabase";

			m_sql.VacuumDatabase();
		}

		void CWebServer::Cmd_GetDbStats(WebEmSession& session, const request& req, Json::Value& root)
		{
			if (session.rights != URIGHTS_ADMIN)
			{
				session.reply_status = reply::forbidden;
				return;
			}
			root["status"] = "OK";
			root["title"] = "GetDbStats";

			auto result = m_sql.safe_query(
				"SELECT (SELECT page_count FROM pragma_page_count()) * (SELECT page_size FROM pragma_page_size()), "
				"       (SELECT freelist_count FROM pragma_freelist_count()) * (SELECT page_size FROM pragma_page_size())");
			if (!result.empty() && result[0].size() >= 2)
			{
				root["dbsize"] = (Json::Int64)atoll(result[0][0].c_str());
				root["freesize"] = (Json::Int64)atoll(result[0][1].c_str());
			}

			auto unused = m_sql.safe_query(
				"SELECT COUNT(DISTINCT ds.ID),"
				"  (SELECT COUNT(*) FROM Temperature WHERE DeviceRowID IN (SELECT ID FROM DeviceStatus WHERE Used=0)) +"
				"  (SELECT COUNT(*) FROM Rain WHERE DeviceRowID IN (SELECT ID FROM DeviceStatus WHERE Used=0)) +"
				"  (SELECT COUNT(*) FROM Wind WHERE DeviceRowID IN (SELECT ID FROM DeviceStatus WHERE Used=0)) +"
				"  (SELECT COUNT(*) FROM UV WHERE DeviceRowID IN (SELECT ID FROM DeviceStatus WHERE Used=0)) +"
				"  (SELECT COUNT(*) FROM Meter WHERE DeviceRowID IN (SELECT ID FROM DeviceStatus WHERE Used=0)) +"
				"  (SELECT COUNT(*) FROM MultiMeter WHERE DeviceRowID IN (SELECT ID FROM DeviceStatus WHERE Used=0)) +"
				"  (SELECT COUNT(*) FROM Percentage WHERE DeviceRowID IN (SELECT ID FROM DeviceStatus WHERE Used=0)) +"
				"  (SELECT COUNT(*) FROM Fan WHERE DeviceRowID IN (SELECT ID FROM DeviceStatus WHERE Used=0)) +"
				"  (SELECT COUNT(*) FROM LightingLog WHERE DeviceRowID IN (SELECT ID FROM DeviceStatus WHERE Used=0)) +"
				"  (SELECT COUNT(*) FROM Temperature_Calendar WHERE DeviceRowID IN (SELECT ID FROM DeviceStatus WHERE Used=0)) +"
				"  (SELECT COUNT(*) FROM Rain_Calendar WHERE DeviceRowID IN (SELECT ID FROM DeviceStatus WHERE Used=0)) +"
				"  (SELECT COUNT(*) FROM Wind_Calendar WHERE DeviceRowID IN (SELECT ID FROM DeviceStatus WHERE Used=0)) +"
				"  (SELECT COUNT(*) FROM UV_Calendar WHERE DeviceRowID IN (SELECT ID FROM DeviceStatus WHERE Used=0)) +"
				"  (SELECT COUNT(*) FROM Meter_Calendar WHERE DeviceRowID IN (SELECT ID FROM DeviceStatus WHERE Used=0)) +"
				"  (SELECT COUNT(*) FROM MultiMeter_Calendar WHERE DeviceRowID IN (SELECT ID FROM DeviceStatus WHERE Used=0)) +"
				"  (SELECT COUNT(*) FROM Percentage_Calendar WHERE DeviceRowID IN (SELECT ID FROM DeviceStatus WHERE Used=0)) +"
				"  (SELECT COUNT(*) FROM Fan_Calendar WHERE DeviceRowID IN (SELECT ID FROM DeviceStatus WHERE Used=0))"
				" FROM DeviceStatus ds WHERE ds.Used=0 AND ("
				"  EXISTS(SELECT 1 FROM Temperature WHERE DeviceRowID=ds.ID) OR"
				"  EXISTS(SELECT 1 FROM Rain WHERE DeviceRowID=ds.ID) OR"
				"  EXISTS(SELECT 1 FROM Wind WHERE DeviceRowID=ds.ID) OR"
				"  EXISTS(SELECT 1 FROM UV WHERE DeviceRowID=ds.ID) OR"
				"  EXISTS(SELECT 1 FROM Meter WHERE DeviceRowID=ds.ID) OR"
				"  EXISTS(SELECT 1 FROM MultiMeter WHERE DeviceRowID=ds.ID) OR"
				"  EXISTS(SELECT 1 FROM Percentage WHERE DeviceRowID=ds.ID) OR"
				"  EXISTS(SELECT 1 FROM Fan WHERE DeviceRowID=ds.ID) OR"
				"  EXISTS(SELECT 1 FROM LightingLog WHERE DeviceRowID=ds.ID) OR"
				"  EXISTS(SELECT 1 FROM Temperature_Calendar WHERE DeviceRowID=ds.ID) OR"
				"  EXISTS(SELECT 1 FROM Rain_Calendar WHERE DeviceRowID=ds.ID) OR"
				"  EXISTS(SELECT 1 FROM Wind_Calendar WHERE DeviceRowID=ds.ID) OR"
				"  EXISTS(SELECT 1 FROM UV_Calendar WHERE DeviceRowID=ds.ID) OR"
				"  EXISTS(SELECT 1 FROM Meter_Calendar WHERE DeviceRowID=ds.ID) OR"
				"  EXISTS(SELECT 1 FROM MultiMeter_Calendar WHERE DeviceRowID=ds.ID) OR"
				"  EXISTS(SELECT 1 FROM Percentage_Calendar WHERE DeviceRowID=ds.ID) OR"
				"  EXISTS(SELECT 1 FROM Fan_Calendar WHERE DeviceRowID=ds.ID)"
				" )");
			if (!unused.empty() && unused[0].size() >= 2)
			{
				root["unuseddevices"] = atoi(unused[0][0].c_str());
				root["unusedrecords"] = (Json::Int64)atoll(unused[0][1].c_str());
			}
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
			if (session.rights != URIGHTS_ADMIN)
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
			if (session.rights != URIGHTS_ADMIN)
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
				int sType = atoi(result[0][1].c_str());
				if ((dType == pTypeTEMP) || (dType == pTypeTEMP_HUM) || (dType == pTypeTEMP_HUM_BARO))
				{
					//Allow old Temp or Temp+Hum or Temp+Hum+Baro devices to be replaced by new Temp or Temp+Hum or Temp+Hum+Baro
					result = m_sql.safe_query("SELECT ID, Name, Type FROM DeviceStatus WHERE (Type=='%d') || (Type=='%d') || (Type=='%d') AND (ID!=%" PRIu64 ")", pTypeTEMP, pTypeTEMP_HUM, pTypeTEMP_HUM_BARO, idx);
				}
				else if (dType == pTypeRAIN)
				{
					result = m_sql.safe_query("SELECT ID, Name, Type FROM DeviceStatus WHERE (Type=='%d') AND (ID!=%" PRIu64 ")", pTypeRAIN, idx);
				}
				else
				{
					result = m_sql.safe_query("SELECT ID, Name FROM DeviceStatus WHERE (Type=='%q') AND (SubType=='%q') AND (ID!=%" PRIu64 ")", result[0][0].c_str(),
						result[0][1].c_str(), idx);

					if ((dType == pTypeAirQuality) && (sType == sTypeVoc))
					{
						//Allow VOC sensors to be replaced by custom sensor
						auto result2 = m_sql.safe_query("SELECT ID, Name FROM DeviceStatus WHERE (Type==%d) AND (SubType==%d) AND (ID!=%" PRIu64 ")", pTypeGeneral, sTypeCustom);
						result.insert(result.end(), result2.begin(), result2.end());
					}
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
			std::string sOldIdx = request::findValue(&req, "idx");
			if (sOldIdx.empty())
				return;

			std::string sNewIdx = request::findValue(&req, "newidx");
			if (sNewIdx.empty())
				return;

			root["status"] = "OK";
			root["title"] = "DoTransferDevice";

			m_sql.TransferDevice(sOldIdx, sNewIdx);	// Function body moved to main helper
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
			if (session.rights != URIGHTS_ADMIN)
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

		void CWebServer::Cmd_ClearSharedUserDevices(WebEmSession& session, const request& req, Json::Value& root)
		{
			if (session.rights != URIGHTS_ADMIN)
			{
				session.reply_status = reply::forbidden;
				return; // Only admin user allowed
			}
			std::string idx = request::findValue(&req, "idx");
			if (idx.empty())
				return;
			root["status"] = "OK";
			root["title"] = "ClearSharedUserDevices";
			m_sql.safe_query("DELETE FROM SharedDevices WHERE SharedUserID == '%q'", idx.c_str());
			LoadUsers();
		}

		static bool IsIconToken(const std::string& szToken, size_t maxLen, bool bAllowSpaces)
		{
			if (szToken.empty() || (szToken.size() > maxLen))
				return false;
			for (const char c : szToken)
			{
				if ((c >= '0') && (c <= '9'))
					continue;
				if ((c >= 'a') && (c <= 'z'))
					continue;
				if ((c >= 'A') && (c <= 'Z'))
					continue;
				if ((c == '-') || (c == '_'))
					continue;
				if (bAllowSpaces && (c == ' '))
					continue;
				return false;
			}
			return true;
		}

		static bool NormaliseDeviceIcon(const std::string& szIn, std::string& szOut)
		{
			szOut.clear();
			if (szIn.empty())
				return true;
			if (szIn.size() > 512)
				return false;

			Json::Value jIn;
			if (!ParseJSon(szIn, jIn) || !jIn.isObject())
				return false;

			const std::string szType = jIn["t"].isString() ? jIn["t"].asString() : "";
			const std::string szOn = jIn["on"].isString() ? jIn["on"].asString() : "";
			const std::string szOff = jIn["off"].isString() ? jIn["off"].asString() : "";

			if (!IsIconToken(szType, 32, false))
				return false;
			if (!IsIconToken(szOn, 128, true))
				return false;
			if (!szOff.empty() && !IsIconToken(szOff, 128, true))
				return false;

			Json::Value jOut;
			jOut["t"] = szType;
			jOut["on"] = szOn;
			if (!szOff.empty())
				jOut["off"] = szOff;
			szOut = JSonToRawString(jOut);
			return true;
		}

		void CWebServer::Cmd_SetUsed(WebEmSession& session, const request& req, Json::Value& root)
		{
			if (session.rights != URIGHTS_ADMIN)
			{
				session.reply_status = reply::forbidden;
				return; // Only admin user allowed
			}

			std::string idx = request::findValue(&req, "idx");
			std::string sused = request::findValue(&req, "used");
			if ((idx.empty()) || (sused.empty()))
				return;
			std::vector<std::vector<std::string>> result;
			result = m_sql.safe_query("SELECT Type,SubType,HardwareID,CustomImage,Description FROM DeviceStatus WHERE (ID == '%q')", idx.c_str());
			if (result.empty())
				return;

			std::string deviceid = request::findValue(&req, "deviceid");
			std::string name = HTMLSanitizer::Sanitize(request::findValue(&req, "name")); stdstring_trim(name);

			bool bHaveText = request::hasValue(&req, "text");
			std::string text = request::findValue(&req, "text"); stdstring_trim(text);

			bool bHaveDescription = request::hasValue(&req, "description");
			std::string description = HTMLSanitizer::Sanitize(request::findValue(&req, "description")); stdstring_trim(description);

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
			bool bHasIcon = request::hasValue(&req, "icon");
			std::string sIcon = request::findValue(&req, "icon");

			std::string strunit = request::findValue(&req, "unit");
			std::string strParam1 = HTMLSanitizer::Sanitize(base64_decode(request::findValue(&req, "strparam1")));
			std::string strParam2 = HTMLSanitizer::Sanitize(base64_decode(request::findValue(&req, "strparam2")));
			std::string tmpstr = request::findValue(&req, "protected");
			bool bHasstrParam1 = request::hasValue(&req, "strparam1");
			int iProtected = (tmpstr == "true") ? 1 : 0;

			std::string sOptions = HTMLSanitizer::Sanitize(base64_decode(request::findValue(&req, "options")));
			std::string devoptions = HTMLSanitizer::Sanitize(CURLEncode::URLDecode(request::findValue(&req, "devoptions")));
			std::string EnergyMeterMode = CURLEncode::URLDecode(request::findValue(&req, "EnergyMeterMode"));
			std::string sShowIcon = request::findValue(&req, "ShowIcon");

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
			// Text sensor devices hold user-authored HTML; preserve tags but strip dangerous attributes
			text = (dType == pTypeGeneral && dSubType == sTypeTextStatus) ? HTMLSanitizer::SanitizeHTML(text) : HTMLSanitizer::Sanitize(text);
			std::string sHwdID = sd[2];
			int OldCustomImage = atoi(sd[3].c_str());
			std::string OldDescription = sd[4];
			if (!bHaveDescription)
				description = OldDescription;

			int CustomImage = (!sCustomImage.empty()) ? std::stoi(sCustomImage) : OldCustomImage;

			if (!setPoint.empty() || !state.empty())
			{
				double tempcelcius = atof(setPoint.c_str());
				if (m_sql.m_tempunit == TEMPUNIT_F)
				{
					// Convert back to Celsius
					tempcelcius = ConvertToCelsius(tempcelcius);
				}
				sprintf(szTmp, "%.2f", tempcelcius);

				if (dType == pTypeThermostat6)
				{
					// For Thermostat6, preserve existing temperature and mode, only update setpoint
					std::vector<std::vector<std::string>> currentResult;
					currentResult = m_sql.safe_query("SELECT sValue FROM DeviceStatus WHERE (ID == '%q')", idx.c_str());
					if (!currentResult.empty())
					{
						std::vector<std::string> strarray;
						StringSplit(currentResult[0][0], ";", strarray);
						if (dSubType == sTypeThermostat6Temp && strarray.size() >= 2)
						{
							sprintf(szTmp, "%s;%.2f", strarray[0].c_str(), tempcelcius);
						}
						else if (dSubType == sTypeThermostat6TempHum && strarray.size() >= 4)
						{
							sprintf(szTmp, "%s;%.2f;%s;%s", strarray[0].c_str(), tempcelcius, strarray[2].c_str(), strarray[3].c_str());
						}
						else if (dSubType == sTypeThermostat6TempBaro && strarray.size() >= 4)
						{
							sprintf(szTmp, "%s;%.2f;%s;%s", strarray[0].c_str(), tempcelcius, strarray[2].c_str(), strarray[3].c_str());
						}
						else if (dSubType == sTypeThermostat6TempHumBaro && strarray.size() >= 6)
						{
							sprintf(szTmp, "%s;%.2f;%s;%s;%s;%s", strarray[0].c_str(), tempcelcius, strarray[2].c_str(), strarray[3].c_str(), strarray[4].c_str(), strarray[5].c_str());
						}
						m_sql.safe_query("UPDATE DeviceStatus SET Used=%d, sValue='%q' WHERE (ID == '%q')", used, szTmp, idx.c_str());
					}
				}
				else if (dType != pTypeEvohomeZone && dType != pTypeEvohomeWater) // sql update now done in setsetpoint for evohome devices
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

			if (bHasIcon)
			{
				std::string szIconNormalised;
				if (NormaliseDeviceIcon(sIcon, szIconNormalised))
				{
					m_sql.safe_query("UPDATE DeviceStatus SET Icon='%q' WHERE (ID == '%q')", szIconNormalised.c_str(), idx.c_str());
				}
				else
				{
					_log.Log(LOG_ERROR, "SetUsed: rejected invalid icon reference for device %s", idx.c_str());
					root["error"] = "Invalid icon";
					return;
				}
			}

			if ((dType == pTypeGeneral) && (dSubType == sTypeTextStatus))
			{
				if (bHaveText)
				{
					m_sql.safe_query("UPDATE DeviceStatus SET sValue='%q' WHERE (ID == '%q')", text.c_str(), idx.c_str());
					m_mainworker.SetTextDevice(idx, text);
					m_sql.UpdateLastUpdate(idx);
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
				std::string szSwitchUser;
				if (bHaveUser)
				{
					int iUser = FindUser(session.username.c_str());
					if (iUser != -1)
					{
						urights = static_cast<int>(m_users[iUser].userrights);
						szSwitchUser = m_users[iUser].Username + " (IP: " + session.remote_host + ")";
						_log.Log(LOG_STATUS, "User: %s initiated a SetPoint command", m_users[iUser].Username.c_str());
					}
				}
				if (urights < 1)
					return;
				if (dType == pTypeEvohomeWater)
					m_mainworker.SetSetPointEvo(idx, (state == "On") ? 1.0F : 0.0F, mode, until, szSwitchUser); // FIXME float not guaranteed precise?
				else if (dType == pTypeEvohomeZone)
					m_mainworker.SetSetPointEvo(idx, static_cast<float>(atof(setPoint.c_str())), mode, until, szSwitchUser);
				else
					m_mainworker.SetSetPoint(idx, static_cast<float>(atof(setPoint.c_str())), szSwitchUser);
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
			bool bNeedShowIcon = (!sShowIcon.empty() && (sShowIcon == "0" || sShowIcon == "1") &&
				atoi(result[0][0].c_str()) == pTypeGeneral && atoi(result[0][1].c_str()) == sTypeTextStatus);
			if (!EnergyMeterMode.empty() || bNeedShowIcon)
			{
				auto options = m_sql.GetDeviceOptions(idx);
				if (!EnergyMeterMode.empty())
					options["EnergyMeterMode"] = EnergyMeterMode;
				if (bNeedShowIcon)
					options["ShowIcon"] = sShowIcon;
				uint64_t ullidx = std::stoull(idx);
				m_sql.SetDeviceOptions(ullidx, options);
			}

			if (!devoptions.empty())
			{
				m_sql.safe_query("UPDATE DeviceStatus SET Options='%q' WHERE (ID == '%q')", devoptions.c_str(), idx.c_str());
			}

			std::string sColorParam = request::findValue(&req, "color");
			if (request::hasValue(&req, "color"))
			{
				m_sql.safe_query("UPDATE DeviceStatus SET Color='%q' WHERE (ID == '%q')", sColorParam.c_str(), idx.c_str());
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
			// the device was already validated above, 'result' can have been reused by the
			// sub device lookup in between, so it says nothing about the outcome here
			root["status"] = "OK";
			root["title"] = "SetUsed";

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
				else if (Key == "LogUnusedSensors")
				{
					root["LogUnusedSensors"] = nValue;
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
				else if (Key == "WebProxyHeaderFamily")
				{
					root["WebProxyHeaderFamily"] = nValue;
				}
				else if (Key == "WebAllowedCORSOrigins")
				{
					root["WebAllowedCORSOrigins"] = sValue;
				}
				else if (Key == "WebCORSAllowTrustedNetworks")
				{
					root["WebCORSAllowTrustedNetworks"] = nValue;
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
				else if (Key == "HourIdxElectricityDevice")
				{
					root["HourIdxElectricityDevice"] = nValue;
				}
				else if (Key == "HourIdxGasDevice")
				{
					root["HourIdxGasDevice"] = nValue;
				}
				else if (Key == "Currency")
				{
					root["Currency"] = sValue;
				}
				else if (Key == "ESettings")
				{
					Json::Value jesettings;
					bool ret = ParseJSon(sValue, jesettings);
					if (ret)
					{
						root["ESettings"] = jesettings;
					}
				}
				else if (Key == "P1DisplayType")
				{
					root["P1DisplayType"] = nValue;
				}
				else if (Key == "PriceResolution")
				{
					root["PriceResolution"] = nValue;
				}
			}
			// ThemeSettings is served from the ThemeSettings table as the merge of the
			// instance defaults with the calling user's overlay (user rows win per
			// theme), so existing themes reading data.ThemeSettings keep working and
			// get per-user values for free. The legacy Preferences row is not read.
			Json::Value jThemeSettings;
			const int iUser = session.username.empty() ? -1 : FindUser(session.username.c_str());
			const unsigned long userID = (iUser != -1) ? m_users[iUser].ID : 0;
			if (CThemeSettings::GetMerged(iUser != -1, userID, jThemeSettings))
				root["ThemeSettings"] = jThemeSettings;
			root["DebugLevel"] = static_cast<int>(_log.GetDebugFlags());
		}

		void CWebServer::Cmd_ThemeSettingsGet(WebEmSession& session, const request& req, Json::Value& root)
		{
			root["status"] = "ERR";
			root["title"] = "ThemeSettingsGet";

			if ((session.rights != URIGHTS_VIEWER) && (session.rights != URIGHTS_SWITCHER) && (session.rights != URIGHTS_ADMIN))
			{
				session.reply_status = reply::forbidden;
				return;
			}

			const std::string themeName = request::findValue(&req, "theme");
			if (!CThemeSettings::IsValidThemeName(themeName))
			{
				root["error"] = CThemeSettings::ErrorCode(CThemeSettings::eResult::InvalidTheme);
				root["message"] = CThemeSettings::ErrorMessage(CThemeSettings::eResult::InvalidTheme);
				return;
			}

			// Per-user rows cannot work when the session identity is shared, which is the
			// case for trusted-network / -nowwwpwd requests without an explicit login:
			// CheckAuthentication assigns the first admin to every anonymous client.
			root["PerUser"] = !session.istrustednetwork || !session.id.empty();
			root["theme"] = themeName;

			root["instance"]["present"] = false;
			{
				Json::Value jValue;
				std::string lastUpdate;
				if (CThemeSettings::Get(CThemeSettings::eScope::Instance, 0, themeName, jValue, lastUpdate))
				{
					root["instance"]["present"] = true;
					root["instance"]["value"] = jValue;
					root["instance"]["lastupdate"] = lastUpdate;
				}
			}

			root["user"]["present"] = false;
			const int iUser = session.username.empty() ? -1 : FindUser(session.username.c_str());
			if (iUser != -1)
			{
				Json::Value jValue;
				std::string lastUpdate;
				if (CThemeSettings::Get(CThemeSettings::eScope::User, m_users[iUser].ID, themeName, jValue, lastUpdate))
				{
					root["user"]["present"] = true;
					root["user"]["value"] = jValue;
					root["user"]["lastupdate"] = lastUpdate;
				}
			}
			root["status"] = "OK";
		}

		void CWebServer::Cmd_ThemeSettingsSet(WebEmSession& session, const request& req, Json::Value& root)
		{
			root["status"] = "ERR";
			root["title"] = "ThemeSettingsSet";

			if (req.method != "POST")
			{
				root["error"] = "post_required";
				root["message"] = "Only POST is allowed";
				return;
			}
			if ((session.rights != URIGHTS_VIEWER) && (session.rights != URIGHTS_SWITCHER) && (session.rights != URIGHTS_ADMIN))
			{
				session.reply_status = reply::forbidden;
				return;
			}
			const int iUser = session.username.empty() ? -1 : FindUser(session.username.c_str());
			if (iUser == -1)
			{
				// OAuth clients, access tokens and synthetic sessions have no Users row to
				// attach an overlay to; refuse explicitly instead of guessing an owner.
				root["error"] = "no_identity";
				root["message"] = "Session does not resolve to a user account";
				session.reply_status = reply::forbidden;
				return;
			}

			const unsigned long userID = m_users[iUser].ID;
			const std::string szReset = request::findValue(&req, "reset");
			std::string newLastUpdate;
			CThemeSettings::eResult res;

			if (szReset == "all")
			{
				// Drops every overlay this user holds, the only way to free rows of a
				// theme that was renamed or uninstalled and whose name a client can no
				// longer produce. Deliberately has no instance-scope counterpart.
				res = CThemeSettings::DeleteForUser(userID);
			}
			else if (szReset == "true")
			{
				res = CThemeSettings::Reset(CThemeSettings::eScope::User, userID, request::findValue(&req, "theme"));
			}
			else
			{
				res = CThemeSettings::Set(CThemeSettings::eScope::User, userID, request::findValue(&req, "theme"), request::findValue(&req, "value"),
							  request::findValue(&req, "lastupdate"), newLastUpdate);
			}
			if (res != CThemeSettings::eResult::Ok)
			{
				root["error"] = CThemeSettings::ErrorCode(res);
				root["message"] = CThemeSettings::ErrorMessage(res);
				return;
			}
			// Only a stored value has a token to hand back; a reset leaves no row
			if (!newLastUpdate.empty())
				root["lastupdate"] = newLastUpdate;
			root["status"] = "OK";
		}

		void CWebServer::Cmd_ThemeSettingsSetDefault(WebEmSession& session, const request& req, Json::Value& root)
		{
			root["status"] = "ERR";
			root["title"] = "ThemeSettingsSetDefault";

			if (req.method != "POST")
			{
				root["error"] = "post_required";
				root["message"] = "Only POST is allowed";
				return;
			}
			if (session.rights != URIGHTS_ADMIN)
			{
				session.reply_status = reply::forbidden;
				return;
			}

			std::string newLastUpdate;
			CThemeSettings::eResult res;

			// Instance defaults are reset one theme at a time; there is no reset=all here
			if (request::findValue(&req, "reset") == "true")
			{
				res = CThemeSettings::Reset(CThemeSettings::eScope::Instance, 0, request::findValue(&req, "theme"));
			}
			else
			{
				res = CThemeSettings::Set(CThemeSettings::eScope::Instance, 0, request::findValue(&req, "theme"), request::findValue(&req, "value"),
							  request::findValue(&req, "lastupdate"), newLastUpdate);
			}
			if (res != CThemeSettings::eResult::Ok)
			{
				root["error"] = CThemeSettings::ErrorCode(res);
				root["message"] = CThemeSettings::ErrorMessage(res);
				return;
			}
			// Keep the legacy Preferences blob in step with the instance rows
			CThemeSettings::MirrorDefaults();
			if (!newLastUpdate.empty())
				root["lastupdate"] = newLastUpdate;
			root["status"] = "OK";
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

			if (!(IsLightOrSwitch(dType, dSubType) || (dType == pTypeEvohome) || (dType == pTypeEvohomeRelay) || (dType == pTypeRego6XXValue)))
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
			if (session.rights != URIGHTS_ADMIN)
			{
				session.reply_status = reply::forbidden;
				return; // Only admin user allowed
			}

			int ii = 0;
			root["title"] = "rclientslog";
			// m_webservers aggregates across every running server (plain and
			// secure), since the tracked-clients map is now per-cWebem-instance
			// rather than one process-wide map shared by all of them.
			for (const auto& rc : m_webservers.GetRemoteClients())
			{
				char timestring[128];
				timestring[0] = 0;
				struct tm timeinfo;
				localtime_r(&rc.last_seen, &timeinfo);

				strftime(timestring, sizeof(timestring), "%a, %d %b %Y %H:%M:%S %z", &timeinfo);

				root["result"][ii]["date"] = timestring;
				root["result"][ii]["address"] = rc.host_remote_endpoint_address_;
				root["result"][ii]["port"] = rc.host_local_endpoint_port_;
				root["result"][ii]["req"] = rc.host_last_request_uri_;
				ii++;
			}
			root["status"] = "OK";
		}

		void CWebServer::Cmd_GetDynamicPriceDevices(WebEmSession& session, const request& req, Json::Value& root)
		{
			if (session.rights != URIGHTS_ADMIN)
			{
				session.reply_status = reply::forbidden;
				return; //Only admin user allowed
			}
			root["status"] = "OK";
			root["title"] = "GetDynamicPriceDevices";
			std::vector<std::vector<std::string> > result;
			result = m_sql.safe_query("SELECT ID, Name FROM DeviceStatus WHERE( (Type==243 AND SubType==31) OR (Type==243 AND SubType==33) ) ORDER BY Name");
			if (!result.empty())
			{
				int ii = 0;

				root["result"][ii]["idx"] = 0x98765;
				root["result"][ii]["Name"] = "Internal (Meter Settings)";
				ii++;

				for (const auto& sd : result)
				{
					root["result"][ii]["idx"] = atoi(sd[0].c_str());
					root["result"][ii]["Name"] = sd[1];
					ii++;
				}
			}
		}
		void CWebServer::Cmd_GetEnergyDashboardDevices(WebEmSession& session, const request& req, Json::Value& root)
		{
			root["title"] = "GetEnergyDashboardDevices";

			std::string szESettings;
			if (m_sql.GetPreferencesVar("ESettings", szESettings))
			{
				Json::Value jesettings;
				std::string sError;
				bool ret = ParseJSon(szESettings, jesettings, &sError);
				if (ret)
				{
					root["status"] = "OK";
					root["result"]["ESettings"] = jesettings;
				}
			}
		}

		void CWebServer::Cmd_GetkWhStats(WebEmSession& session, const request& req, Json::Value& root)
		{
			if (request::findValue(&req, "idx").empty())
				return;
			uint64_t idx = std::stoull(request::findValue(&req, "idx"));

			Json::Value result;
			CKWHStats::GetJSONStats(idx, result);
			root["result"] = result;
			root["status"] = "OK";
			root["title"] = "GetkWhStats";
		}

		void CWebServer::Cmd_ResetkWhStats(WebEmSession& session, const request& req, Json::Value& root)
		{
			if (session.rights != URIGHTS_ADMIN)
			{
				session.reply_status = reply::forbidden;
				return; //Only admin user allowed
			}
			if (request::findValue(&req, "idx").empty())
				return;
			uint64_t idx = std::stoull(request::findValue(&req, "idx"));

			CKWHStats::ResetJSONStats(idx);
			root["status"] = "OK";
			root["title"] = "ResetkWhStats";
		}

		void CWebServer::Cmd_FixkWhStats(WebEmSession& session, const request& req, Json::Value& root)
		{
			if (session.rights != URIGHTS_ADMIN)
			{
				session.reply_status = reply::forbidden;
				return; //Only admin user allowed
			}
			if (request::findValue(&req, "idx").empty())
				return;
			uint64_t idx = std::stoull(request::findValue(&req, "idx"));

			bool changed = CKWHStats::RemoveSpikeStats(idx);
			root["changed"] = changed;
			root["status"] = "OK";
			root["title"] = "FixkWhStats";
		}

		void CWebServer::Cmd_FixCounterPrices(WebEmSession& session, const request& req, Json::Value& root)
		{
			if (session.rights != URIGHTS_ADMIN)
			{
				session.reply_status = reply::forbidden;
				return; //Only admin user allowed
			}
			if (request::findValue(&req, "idx").empty())
				return;
			uint64_t idx = std::stoull(request::findValue(&req, "idx"));

			int hwID = -1;
			bool bIsKwhCounter = false;
			{
				auto devRes = m_sql.safe_query(
					"SELECT HardwareID, Type, SubType FROM DeviceStatus WHERE ID='%" PRIu64 "'", idx);
				if (!devRes.empty())
				{
					hwID        = atoi(devRes[0][0].c_str());
					int devType = atoi(devRes[0][1].c_str());
					int devSub  = atoi(devRes[0][2].c_str());
					bIsKwhCounter = (devType == pTypeGeneral && devSub == sTypeKwh);
				}
			}

			// Fix actual counter spikes in Meter_Calendar / Meter (kWh devices only).
			// Uses auto-detected threshold (0.0 = 100x median daily usage).
			// Non-kWh devices are silently skipped by FixKwhCounterSpikes.
			std::vector<std::string> spikeResults;
			m_sql.FixKwhCounterSpikes(idx, 0.0, false, spikeResults);
			int spikesFixed = 0;
			for (const auto& line : spikeResults)
			{
				// Count both kWh spikes ("Positive/Negative spike") and P1 calendar spikes ("Calendar spike")
				if (line.rfind("Positive spike", 0) == 0 || line.rfind("Negative spike", 0) == 0 ||
				    line.rfind("Calendar spike", 0) == 0 || line.rfind("Shortlog corrupt", 0) == 0)
					spikesFixed++;
			}

			bool changed = CKWHStats::RemoveSpikeStats(idx);
			int pricesFixed = m_sql.SanitizeCalendarData(idx);

			// Only stop/restart hardware when spikes were actually corrected in the DB.
			// The CounterHelper (used by MQTT-AD and similar plugins) caches the cumulative
			// counter in memory and must reload from the corrected DB values.
			// EnphaseAPI also benefits from a restart: FixKwhCounterSpikes corrects
			// DeviceStatus.sValue to the lower baseline, and the upward-spike detection
			// in ProcessEnphaseCounter will then catch the gap between that baseline and
			// the Envoy's still-high whLifetime, applying the correct negative offset so
			// the tracker continues from the corrected value.  Without the restart the
			// corrected sValue is overwritten on the very next Envoy poll.
			bool bNeedsRestart = bIsKwhCounter && hwID != -1 && spikesFixed > 0;
			if (bNeedsRestart)
			{
				m_mainworker.RemoveDomoticzHardware(hwID);
				auto hwRes = m_sql.safe_query(
					"SELECT ID, Name, Enabled, Type, LogLevel, Address, Port, SerialPort, Username, Password, "
					"Extra, Mode1, Mode2, Mode3, Mode4, Mode5, Mode6, DataTimeout FROM Hardware WHERE ID=%d", hwID);
				if (!hwRes.empty())
				{
					const auto& hw = hwRes[0];
					m_mainworker.AddHardwareFromParams(
						atoi(hw[0].c_str()), hw[1], atoi(hw[2].c_str()) != 0,
						(_eHardwareTypes)atoi(hw[3].c_str()), (uint32_t)atoi(hw[4].c_str()),
						hw[5], (uint16_t)atoi(hw[6].c_str()), hw[7], hw[8], hw[9], hw[10],
						atoi(hw[11].c_str()), atoi(hw[12].c_str()), atoi(hw[13].c_str()),
						atoi(hw[14].c_str()), atoi(hw[15].c_str()), atoi(hw[16].c_str()),
						atoi(hw[17].c_str()), true);
					spikeResults.push_back("Hardware restarted to reload CounterHelper from corrected values");
				}
			}

			root["changed"] = changed || (pricesFixed > 0) || (spikesFixed > 0);
			root["kwhStatsFixed"] = changed;
			root["pricesFixed"] = pricesFixed;
			root["spikesFixed"] = spikesFixed;
			// Include detail lines so the frontend can show exactly what was corrected
			for (int i = 0; i < static_cast<int>(spikeResults.size()); i++)
				root["result"][i] = spikeResults[i];
			root["status"] = "OK";
			root["title"] = "FixCounterPrices";
		}

		// Helper function to convert ANSI color codes to HTML spans
		// Also handles progress indicators (dots, ===>, percentages) from tar/wget
		static std::string ConvertAnsiToHtml(const std::string& input)
		{
			// First, handle carriage returns - only keep content after last \r per segment
			std::string processed;
			size_t lastCR = 0;
			bool hasCR = false;
			for (size_t j = 0; j < input.size(); j++)
			{
				if (input[j] == '\r')
				{
					lastCR = j + 1;
					hasCR = true;
				}
			}
			if (hasCR && lastCR < input.size())
				processed = input.substr(lastCR);
			else
				processed = input;

			std::string result;
			result.reserve(processed.size() * 2);

			size_t i = 0;
			bool inSpan = false;
			int progressCharCount = 0;
			const int maxProgressCharsPerLine = 50;

			while (i < processed.size())
			{
				// Check for ANSI escape sequence: \033[ or \x1b[
				if (i + 1 < processed.size() && processed[i] == '\033' && processed[i + 1] == '[')
				{
					// Find the end of the escape sequence (ends with 'm')
					size_t start = i + 2;
					size_t end = start;
					while (end < processed.size() && processed[end] != 'm')
						end++;

					if (end < processed.size())
					{
						std::string code = processed.substr(start, end - start);

						// Close previous span if open
						if (inSpan)
						{
							result += "</span>";
							inSpan = false;
						}

						// Map ANSI codes to CSS classes
						if (code == "0;31" || code == "31")  // Red
						{
							result += "<span class=\"log-red\">";
							inSpan = true;
						}
						else if (code == "0;32" || code == "32")  // Green
						{
							result += "<span class=\"log-green\">";
							inSpan = true;
						}
						else if (code == "1;33" || code == "33")  // Yellow
						{
							result += "<span class=\"log-yellow\">";
							inSpan = true;
						}
						// code == "0" is reset, just close span (already done above)

						i = end + 1;
						progressCharCount = 0;
						continue;
					}
				}

				// Handle progress characters (dots, equals signs) - break into lines
				if (processed[i] == '.' || processed[i] == '=')
				{
					progressCharCount++;
					result += processed[i];
					if (progressCharCount >= maxProgressCharsPerLine)
					{
						result += "<br>";
						progressCharCount = 0;
					}
				}
				// HTML escape special characters
				else if (processed[i] == '<')
				{
					result += "&lt;";
					progressCharCount = 0;
				}
				else if (processed[i] == '>')
				{
					result += "&gt;";
					progressCharCount = 0;
				}
				else if (processed[i] == '&')
				{
					result += "&amp;";
					progressCharCount = 0;
				}
				else if (processed[i] != '\r')  // Skip any remaining CR
				{
					result += processed[i];
					progressCharCount = 0;
				}

				i++;
			}

			// Close any remaining open span
			if (inSpan)
				result += "</span>";

			return result;
		}

		void CWebServer::Cmd_GetUpdateLog(WebEmSession& session, const request& req, Json::Value& root)
		{
			if (session.rights != URIGHTS_ADMIN)
			{
				session.reply_status = reply::forbidden;
				return; // Only admin user allowed
			}

			root["status"] = "OK";
			root["title"] = "GetUpdateLog";
			root["version"] = szAppVersion;

			std::string logfile = szStartupFolder + "update.log";
			std::ifstream infile;
			infile.open(logfile.c_str());

			bool hasError = false;
			bool hasCompleted = false;
			std::string lastErrorLine;

			if (infile.is_open())
			{
				std::string sLine;
				int ii = 0;
				while (!infile.eof())
				{
					std::getline(infile, sLine);
					if (!sLine.empty())
					{
						// Check for error (red color code \033[0;31m)
						if (sLine.find("\033[0;31m") != std::string::npos)
						{
							hasError = true;
							// Extract error message (strip ANSI codes for the message)
							lastErrorLine = sLine;
							// Remove ANSI codes for plain text error message
							size_t pos;
							while ((pos = lastErrorLine.find("\033[")) != std::string::npos)
							{
								size_t endPos = lastErrorLine.find('m', pos);
								if (endPos != std::string::npos)
									lastErrorLine.erase(pos, endPos - pos + 1);
								else
									break;
							}
							// Remove the >> prefix if present
							if (lastErrorLine.substr(0, 3) == ">> ")
								lastErrorLine = lastErrorLine.substr(3);
						}

						// Check for successful completion
						if (sLine.find("Update completed successfully") != std::string::npos)
							hasCompleted = true;

						// Convert ANSI to HTML and store
						root["result"][ii] = ConvertAnsiToHtml(sLine);
						ii++;
					}
				}
				infile.close();
			}

			// Derive status from log content
			if (hasError)
			{
				root["updatestatus"] = "error";
				root["errormessage"] = lastErrorLine;
			}
			else if (hasCompleted)
			{
				root["updatestatus"] = "complete";
			}
			else
			{
				root["updatestatus"] = "running";
			}
		}

		// ---------------------------------------------------------------------------
		// Dashboard 2.0 layout management commands
		// ---------------------------------------------------------------------------

		void CWebServer::Cmd_GetDashboardLayouts(WebEmSession& session, const request& req, Json::Value& root)
		{
			root["status"] = "ERR";
			root["title"] = "GetDashboardLayouts";

			if (session.username.empty())
			{
				session.reply_status = reply::forbidden;
				return;
			}

			int iUser = FindUser(session.username.c_str());
			if (iUser == -1)
			{
				session.reply_status = reply::forbidden;
				return;
			}

			Json::Value layouts;
			if (!m_sql.GetDashboardLayouts((int)m_users[iUser].ID, layouts))
			{
				root["message"] = "Failed to retrieve dashboard layouts";
				return;
			}
			root["status"] = "OK";
			root["result"] = layouts;
		}

		void CWebServer::Cmd_GetDashboardLayout(WebEmSession& session, const request& req, Json::Value& root)
		{
			root["status"] = "ERR";
			root["title"] = "GetDashboardLayout";

			if (session.username.empty())
			{
				session.reply_status = reply::forbidden;
				return;
			}

			int iUser = FindUser(session.username.c_str());
			if (iUser == -1)
			{
				session.reply_status = reply::forbidden;
				return;
			}

			std::string layoutid = request::findValue(&req, "id");
			if (layoutid.empty())
			{
				root["message"] = "Missing parameter: id";
				return;
			}

			Json::Value layout;
			if (!m_sql.GetDashboardLayout((int)m_users[iUser].ID, layoutid, layout))
			{
				root["message"] = "Layout not found";
				return;
			}
			root["status"] = "OK";
			root["result"] = layout;
		}

		void CWebServer::Cmd_SaveDashboardLayout(WebEmSession& session, const request& req, Json::Value& root)
		{
			root["status"] = "ERR";
			root["title"] = "SaveDashboardLayout";

			if (session.username.empty())
			{
				session.reply_status = reply::forbidden;
				return;
			}

			int iUser = FindUser(session.username.c_str());
			if (iUser == -1)
			{
				session.reply_status = reply::forbidden;
				return;
			}

			std::string id         = request::findValue(&req, "id");
			std::string name       = request::findValue(&req, "name");
			std::string isdefstr   = request::findValue(&req, "isDefault");
			std::string layoutjson = request::findValue(&req, "layout");

			if (id.empty() || name.empty())
			{
				root["message"] = "Missing required parameters: id, name";
				return;
			}

			if (id.size() > 64)
			{
				root["message"] = "Invalid id: too long";
				return;
			}
			for (char c : id)
			{
				if (!isalnum((unsigned char)c) && c != '-' && c != '_')
				{
					root["message"] = "Invalid id format";
					return;
				}
			}

			if (name.size() > 100)
			{
				root["message"] = "Invalid name: too long";
				return;
			}

			bool bUpdateLayout = !layoutjson.empty();

			if (bUpdateLayout)
			{
				const size_t MAX_LAYOUT_SIZE = 1048576; // 1 MB
				if (layoutjson.size() > MAX_LAYOUT_SIZE)
				{
					root["message"] = "Layout JSON exceeds maximum allowed size";
					return;
				}

				Json::Value parsedLayout;
				std::string parseError;
				if (!ParseJSon(layoutjson, parsedLayout, &parseError))
				{
					root["message"] = "Invalid layout JSON: " + parseError;
					return;
				}
			}

			bool isDefault = (isdefstr == "true" || isdefstr == "1");
			if (!m_sql.SaveDashboardLayout((int)m_users[iUser].ID, id, name, isDefault, bUpdateLayout ? layoutjson : std::string()))
			{
				root["message"] = "Failed to save layout";
				return;
			}
			root["status"] = "OK";
		}

		void CWebServer::Cmd_DeleteDashboardLayout(WebEmSession& session, const request& req, Json::Value& root)
		{
			root["status"] = "ERR";
			root["title"] = "DeleteDashboardLayout";

			if (session.username.empty())
			{
				session.reply_status = reply::forbidden;
				return;
			}

			int iUser = FindUser(session.username.c_str());
			if (iUser == -1)
			{
				session.reply_status = reply::forbidden;
				return;
			}

			std::string id = request::findValue(&req, "id");
			if (id.empty())
			{
				root["message"] = "Missing parameter: id";
				return;
			}

			m_sql.DeleteDashboardLayout((int)m_users[iUser].ID, id);
			root["status"] = "OK";
		}

		void CWebServer::Cmd_CopyDashboardLayout(WebEmSession& session, const request& req, Json::Value& root)
		{
			root["status"] = "ERR";
			root["title"] = "CopyDashboardLayout";

			if (session.username.empty())
			{
				session.reply_status = reply::forbidden;
				return;
			}

			int iUser = FindUser(session.username.c_str());
			if (iUser == -1)
			{
				session.reply_status = reply::forbidden;
				return;
			}

			std::string srcid   = request::findValue(&req, "id");
			std::string newname = request::findValue(&req, "newname");
			if (srcid.empty() || newname.empty())
			{
				root["message"] = "Missing parameters: id, newname";
				return;
			}

			if (newname.size() > 100)
			{
				root["message"] = "Invalid newname: too long";
				return;
			}

			std::string newid = GenerateUUID();
			if (!m_sql.CopyDashboardLayout((int)m_users[iUser].ID, srcid, newid, newname))
			{
				root["message"] = "Failed to copy layout (source not found?)";
				return;
			}
			root["status"] = "OK";
			root["id"] = newid;
		}

	} // namespace server
} // namespace http
