/**
 * SolarEdge API & Web Portal Hardware Driver v2.0
 *
 * Author: GizMoCuz
 *
 * Credits for Web API:
 *   - AndrewTapp / solaredgeoptimizers
 *   - Claude (Anthropic) - AI-assisted development
 */

#include "stdafx.h"
#include "SolarEdgeAPI.h"
#include "../main/Helper.h"
#include "../main/Logger.h"
#include "../main/SQLHelper.h"
#include "../httpclient/UrlEncode.h"
#include "hardwaretypes.h"
#include "../httpclient/HTTPClient.h"
#include "../main/json_helper.h"
#include "../main/RFXtrx.h"
#include "../main/mainworker.h"
#include <libwebem/Base64.h>
#include <openssl/rand.h>
#include <fstream>
#include <regex>

#ifdef _WIN32
#define gmtime_r(timep, result) gmtime_s(result, timep)
#endif

#define SE_VOLT_DC 20
#define SE_AC_CURRENT 24

#define SE_GRID 30
#define SE_LOAD 31
#define SE_PV 32
#define SE_STORAGE_STATUS 33
#define SE_STORAGE_POWER 34
#define SE_STORAGE_CHARGELEVEL 35

#define SE_OVERVIEW_CURRENT 40
#define SE_OVERVIEW_TODAY 41
#define SE_OVERVIEW_MONTH 42
#define SE_OVERVIEW_YEAR 43
#define SE_OVERVIEW_LIFETIME 44

#define SE_ENERGY_PRODUCTION 50
#define SE_ENERGY_CONSUMPTION 51
#define SE_ENERGY_SELFCONSUMPTION 52
#define SE_ENERGY_FEEDIN 53
#define SE_ENERGY_PURCHASED 54

// Optimizer web-portal sensor sub-IDs (per optimizer node, base node 300+)
#define SE_OPT_POWER 1

// Web portal node ID bases for inverters and strings
#define SE_WEB_INVERTER_BASE 250
#define SE_WEB_STRING_BASE 260

// Web portal power sub-ID (aggregated from child optimizers), for inverter/string nodes
#define SE_WEB_POWER 1

// SolarEdge web portal OAuth2 (PKCE) endpoints
#define SE_WEB_CLIENT_ID "ugfnsujd3384sshcjehaphlh3"
#define SE_WEB_REDIRECT_URI "https://monitoring.solaredge.com/mfe/auth/callback"
#define SE_WEB_AUTHORIZE_URL "https://login.solaredge.com/oauth2/authorize"
#define SE_WEB_TOKEN_URL "https://login.solaredge.com/oauth2/token"
#define SE_WEB_EXCHANGE_URL "https://monitoring.solaredge.com/services/auth/token?legacy=false"

// SolarEdge Basic Monitoring API v2 (X-API-Key header, no more ?api_key= query param)
#define SE_API_BASE_URL "https://monitoringapi.solaredge.com/v2"

// Monitoring API v2 OAuth2 Site Access (used for homeowner-only accounts with no Fleet API key;
// see developer.solaredge.com - Fleet Access is not offered to accounts without an installer profile)
#define SE_OAUTH_TOKEN_URL "https://monitoringapi.solaredge.com/v2/oauth2/token"

// SolarEdge Connect (the developer console's consent UI) - undocumented, reverse-engineered endpoints
// used to fully automate the one-time Site Access authorization using the account's SSO login,
// the same way the Web Portal login below already automates login.solaredge.com.
#define SE_CONNECT_LOGIN_REQUEST_URL "https://connect.solaredge.com/services/key-manager/connect-consent/login-request"
#define SE_CONNECT_SESSION_TOKEN_URL "https://connect.solaredge.com/services/key-manager/connect-consent/token"
#define SE_CONNECT_APPROVE_URL_PREFIX "https://connect.solaredge.com/services/key-manager/applications/"


#ifdef _DEBUG
//	#define DEBUG_SolarEdgeAPIR
//	#define DEBUG_SolarEdgeAPIW
#endif

#ifdef DEBUG_SolarEdgeAPIW
void SaveString2Disk(std::string str, std::string filename)
{
	FILE* fOut = fopen(filename.c_str(), "wb+");
	if (fOut)
	{
		fwrite(str.c_str(), 1, str.size(), fOut);
		fclose(fOut);
	}
}
#endif
#ifdef DEBUG_SolarEdgeAPIR
std::string ReadFile(std::string filename)
{
	std::ifstream file;
	std::string sResult = "";
	file.open(filename.c_str());
	if (!file.is_open())
		return "";
	std::string sLine;
	while (!file.eof())
	{
		getline(file, sLine);
		sResult += sLine;
	}
	file.close();
	return sResult;
}
#endif

namespace
{
	std::string ExtractNodeIdentity(const Json::Value& node)
	{
		if (!node["serial"].empty())
			return node["serial"].asString();
		const Json::Value& props = node["properties"];
		if (!props.empty() && !props["identifier"].empty())
			return props["identifier"].asString();
		if (!node["uuid"].empty())
			return node["uuid"].asString();
		return "";
	}

	bool IsInactiveNode(const Json::Value& node)
	{
		const Json::Value& props = node["properties"];
		if (props.empty())
			return false;
		return props.get("status", "").asString() == "INACTIVE";
	}

	int LastHttpStatusCode(const std::vector<std::string>& vHeaderData)
	{
		int iStatusCode = 0;
		for (const auto& line : vHeaderData)
		{
			if (line.compare(0, 5, "HTTP/") != 0)
				continue;
			size_t pos = line.find(' ');
			if (pos != std::string::npos)
				iStatusCode = atoi(line.c_str() + pos + 1);
		}
		return iStatusCode;
	}

	bool HeaderNameIs(const std::string& line, const std::string& headerName)
	{
		if (line.size() < headerName.size())
			return false;
		for (size_t i = 0; i < headerName.size(); i++)
		{
			if (tolower((unsigned char)line[i]) != tolower((unsigned char)headerName[i]))
				return false;
		}
		return true;
	}

	// Scans every captured redirect hop for a Location header carrying the named query parameter
	std::string ExtractLocationParam(const std::vector<std::string>& vHeaderData, const std::string& paramName)
	{
		for (const auto& line : vHeaderData)
		{
			if (!HeaderNameIs(line, "location:"))
				continue;
			size_t paramPos = line.find(paramName);
			if (paramPos == std::string::npos)
				continue;
			size_t valueStart = paramPos + paramName.size();
			size_t valueEnd = line.find_first_of("&\r\n", valueStart);
			if (valueEnd == std::string::npos)
				return line.substr(valueStart);
			return line.substr(valueStart, valueEnd - valueStart);
		}
		return "";
	}

	// Scans every captured redirect hop for a Location header carrying an OAuth "code" parameter
	std::string ExtractLocationCode(const std::vector<std::string>& vHeaderData)
	{
		return ExtractLocationParam(vHeaderData, "code=");
	}

	// V2 monitoring API date-time parameters are UTC, formatted as e.g. "2026-01-01T00:00:00Z"
	std::string FormatApiUtcTime(time_t t)
	{
		struct tm tmv;
		gmtime_r(&t, &tmv);
		char szBuf[40];
		snprintf(szBuf, sizeof(szBuf), "%04d-%02d-%02dT%02d:%02d:%02dZ", tmv.tm_year + 1900, tmv.tm_mon + 1, tmv.tm_mday, tmv.tm_hour, tmv.tm_min, tmv.tm_sec);
		return szBuf;
	}

	// V2 energy responses carry an explicit "unit" (WH/KWH/MWH/GWH); normalize everything to Wh
	double ToWattHours(double value, const std::string& unit)
	{
		if (unit == "KWH")
			return value * 1000.0;
		if (unit == "MWH")
			return value * 1000000.0;
		if (unit == "GWH")
			return value * 1000000000.0;
		return value;
	}

	// V2 telemetry metrics are { unit, values: [ { timestamp, value } ] }; we only ever want the latest sample
	double LastMetricValue(const Json::Value& metric)
	{
		const Json::Value& values = metric["values"];
		if (!values.isArray() || values.empty())
			return 0;
		return values[values.size() - 1]["value"].asDouble();
	}
}

SolarEdgeAPI::SolarEdgeAPI(const int ID, const std::string& APIKey, const std::string& Password, const std::string& Extra, const int Mode1) :
	m_APIKey(APIKey)
{
	m_SiteID = 0;
	m_HwdID = ID;
	m_totalActivePower = 0;
	m_totalEnergy = 0;

	// Parse Extra: "web_username|site_id|client_id_b64|client_secret_b64|auth_code_b64"
	// The last three fields are only used for OAuth2 Site Access, when no Fleet API Key is configured.
	// web_username/site_id stay plain text (not base64) to keep reading already-deployed Hardware
	// rows from before OAuth2 support existed; neither value can contain '|' in practice (an email
	// address and a numeric site ID), so the split is unambiguous without encoding them too.
	std::vector<std::string> vExtra;
	StringSplit(Extra, "|", vExtra);
	if (vExtra.size() > 0)
		m_WebUsername = vExtra[0];
	if (vExtra.size() > 1)
		m_WebSiteID = vExtra[1];
	if (vExtra.size() > 2)
		m_ApiClientId = base64_decode(vExtra[2]);
	if (vExtra.size() > 3)
		m_ApiClientSecret = base64_decode(vExtra[3]);
	if (vExtra.size() > 4)
		m_ApiAuthCode = base64_decode(vExtra[4]);

	m_WebPassword = Password;
	m_bPollOptimizers = (Mode1 != 0);

	LoadWebRefreshToken();
	LoadApiRefreshToken();
}

bool SolarEdgeAPI::StartHardware()
{
	RequestStart();

	//Start worker thread
	m_thread = std::make_shared<std::thread>([this] { Do_Work(); });
	SetThreadNameInt(m_thread->native_handle());
	m_bIsStarted = true;
	sOnConnected(this);
	return (m_thread != nullptr);
}

bool SolarEdgeAPI::StopHardware()
{
	if (m_thread)
	{
		RequestStop();
		m_thread->join();
		m_thread.reset();
	}
	m_bIsStarted = false;
	return true;
}

void SolarEdgeAPI::Do_Work()
{
	Log(LOG_STATUS, "Worker started...");

	// Polling intervals (seconds)
	// Layout: every 2 hours (also provides site/inverter/string/optimizer structure)
	// Optimizer playback: every 10 minutes (1 call for all optimizers, daylight only)
	// Energy totals (month/year/lifetime): every hour, these barely move within a 5-minute window
	constexpr int LAYOUT_INTERVAL = 7200;         // 2 hours
	constexpr int OPTIMIZER_DATA_INTERVAL = 600;   // 10 minutes
	constexpr int ENERGY_TOTALS_INTERVAL = 3600;   // 1 hour

	// Start counters so layout runs ~5s after startup, optimizer data ~15s after
	int sec_counter = 295;
	int layout_timer = LAYOUT_INTERVAL - 5;
	int optimizer_data_timer = OPTIMIZER_DATA_INTERVAL - 15;
	int energy_totals_timer = ENERGY_TOTALS_INTERVAL - 20;

	while (!IsStopRequested(1000))
	{
		sec_counter++;
		layout_timer++;
		optimizer_data_timer++;
		energy_totals_timer++;

		if (sec_counter % 12 == 0)
			m_LastHeartbeat = mytime(nullptr);

		// API-key polling (site overview, inverter telemetry, grid/load/pv/battery)
		if (sec_counter % 300 == 0)
		{
			if (m_SiteID == 0)
			{
				if (!GetSite())
					continue;
				GetBatteryFromInventory();
				GetInverters();
			}

			if (!m_inverters.empty())
				GetInverterTelemetry();
			GetOverview();
			GetBatteryDetails();
		}

		// API-key polling: month/year/lifetime energy totals, once an hour
		if (m_SiteID != 0 && energy_totals_timer >= ENERGY_TOTALS_INTERVAL)
		{
			energy_totals_timer = 0;
			GetSiteEnergyTotals();
		}

		bool bNowInDaylight = isDaylightWindow();
		if (m_bWasInDaylightWindow && !bNowInDaylight)
			ResetPowerValues();
		m_bWasInDaylightWindow = bNowInDaylight;

		// Web portal polling (requires web credentials)
		bool bWebCredentials = !m_WebUsername.empty() && !m_WebPassword.empty();

		// Web portal: refresh site layout every 2 hours (site/inverter/string/optimizer structure)
		// Need either a configured Web Site ID or an API-discovered m_SiteID
		if (bWebCredentials && !m_WebSiteID.empty() && layout_timer >= LAYOUT_INTERVAL)
		{
			layout_timer = 0;
			GetSiteLayout();
		}

		// Web portal: poll optimizer power data every 10 minutes
		// This makes 1 HTTP call for all optimizers, so gated by the Poll Optimizers setting
		if (bWebCredentials && m_bPollOptimizers && !m_optimizers.empty() && optimizer_data_timer >= OPTIMIZER_DATA_INTERVAL)
		{
			optimizer_data_timer = 0;
			GetOptimizerData();
		}
	}
	Log(LOG_STATUS, "Worker stopped...");
}

bool SolarEdgeAPI::WriteToHardware(const char* pdata, const unsigned char length)
{
	return false;
}

int SolarEdgeAPI::getSunRiseSunSetMinutes(const bool bGetSunRise)
{
	std::vector<std::string> strarray;
	std::vector<std::string> sunRisearray;
	std::vector<std::string> sunSetarray;

	if (!m_mainworker.m_LastSunriseSet.empty())
	{
		StringSplit(m_mainworker.m_LastSunriseSet, ";", strarray);
		StringSplit(strarray[0], ":", sunRisearray);
		StringSplit(strarray[1], ":", sunSetarray);

		int sunRiseInMinutes = (atoi(sunRisearray[0].c_str()) * 60) + atoi(sunRisearray[1].c_str());
		int sunSetInMinutes = (atoi(sunSetarray[0].c_str()) * 60) + atoi(sunSetarray[1].c_str());

		if (bGetSunRise) {
			return sunRiseInMinutes;
		}
		return sunSetInMinutes;
	}
	return 0;
}

bool SolarEdgeAPI::isDaylightWindow()
{
	// Check daylight window
	time_t atime = mytime(nullptr);
	struct tm ltime;
	localtime_r(&atime, &ltime);
	int ActHourMin = (ltime.tm_hour * 60) + ltime.tm_min;
	int sunRise = getSunRiseSunSetMinutes(true);
	int sunSet = getSunRiseSunSetMinutes(false);
	if (ActHourMin + 60 < sunRise)
		return false;
	if (ActHourMin - 60 > sunSet)
		return false;
	return true;
}

void SolarEdgeAPI::ResetPowerValues()
{
	Log(LOG_STATUS, "Daylight window ended, resetting power/current values to zero");
	char szTmp[100];

	SendKwhMeter(1, 1, 255, 0, m_totalEnergy / 1000.0, "kWh Meter Total");

	for (int i = 0; i < (int)m_inverters.size(); i++)
	{
		sprintf(szTmp, "kWh Meter %s", m_inverters[i].name.c_str());
		double energy = (i < (int)m_lastInverterEnergy.size()) ? m_lastInverterEnergy[i] : 0.0;
		SendKwhMeter(0, 1 + i, 255, 0, energy / 1000.0, szTmp);

		// No need to reset the next values, API returns 0 before daylight window ends
		//for (int ii = 0; ii < 3; ii++)
		//{
		//	int iPhase = ii + 1;
		//	sprintf(szTmp, "acCurrent L%d %s", iPhase, m_inverters[i].name.c_str());
		//	SendCustomSensor(i, SE_AC_CURRENT + ii, 255, 0, szTmp, "A");
		//	sprintf(szTmp, "Power L%d %s", iPhase, m_inverters[i].name.c_str());
		//	SendWattMeter(1 + i, iPhase, 255, 0, szTmp);
		//}
	}

	SendWattMeter(200, SE_OVERVIEW_CURRENT, 255, 0, "Site Current Power");
	SendWattMeter(200, SE_GRID, 255, 0, "Grid Power");
	SendWattMeter(200, SE_LOAD, 255, 0, "Load Power");
	SendWattMeter(200, SE_PV, 255, 0, "PV Power");
	if (m_bPollBattery)
		SendWattMeter(200, SE_STORAGE_POWER, 255, 0, "Battery Power");

	if (m_bPollOptimizers) // only reset optimizer values if we are polling them, otherwise they may create unnecessary devices.
	{
		for (const auto& opt : m_optimizers)
		{
			sprintf(szTmp, "%s Power", opt.displayName.c_str());
			SendWattMeter(opt.nodeId, SE_OPT_POWER, 255, 0, szTmp);
		}
		for (const auto& str : m_webStrings)
		{
			sprintf(szTmp, "%s Power", str.displayName.c_str());
			SendWattMeter(str.nodeId, SE_WEB_POWER, 255, 0, szTmp);
		}
		for (const auto& inv : m_webInverters)
		{
			sprintf(szTmp, "%s Power", inv.displayName.c_str());
			SendWattMeter(inv.nodeId, SE_WEB_POWER, 255, 0, szTmp);
		}
	}
}

bool SolarEdgeAPI::ApiUsesOAuth() const
{
	return m_APIKey.empty();
}

std::string SolarEdgeAPI::BuildApiAuthHeader() const
{
	if (!ApiUsesOAuth())
		return "X-API-Key: " + m_APIKey;
	return "Authorization: Bearer " + m_ApiAccessToken;
}

std::string SolarEdgeAPI::GetApiTokenPrefKey() const
{
	return "SolarEdgeApiToken_" + std::to_string(m_HwdID);
}

bool SolarEdgeAPI::LoadApiRefreshToken()
{
	int nValue = 0;
	std::string sValue;
	if (!m_sql.GetPreferencesVar(GetApiTokenPrefKey(), nValue, sValue))
		return false;
	m_ApiRefreshToken = sValue;
	m_ApiNextRefreshTs = nValue;
	return !m_ApiRefreshToken.empty();
}

void SolarEdgeAPI::StoreApiRefreshToken()
{
	if (m_ApiRefreshToken.empty())
		return;
	m_sql.UpdatePreferencesVar(GetApiTokenPrefKey(), (int)m_ApiNextRefreshTs, m_ApiRefreshToken);
}

bool SolarEdgeAPI::ApiExchangeAuthCode()
{
	if (m_ApiClientId.empty() || m_ApiClientSecret.empty() || m_ApiAuthCode.empty())
		return false;

	// Accept either a bare authorization code, or the full (possibly unreachable) callback URL
	// the user copied from their browser's address bar after approving access.
	std::string code = m_ApiAuthCode;
	size_t codePos = code.find("code=");
	if (codePos != std::string::npos)
	{
		size_t valueStart = codePos + 5;
		size_t valueEnd = code.find_first_of("&#", valueStart);
		code = (valueEnd == std::string::npos) ? code.substr(valueStart) : code.substr(valueStart, valueEnd - valueStart);
	}
	if (m_WebSiteID.empty())
	{
		size_t sitePos = m_ApiAuthCode.find("site_id=");
		if (sitePos != std::string::npos)
		{
			size_t valueStart = sitePos + 8;
			size_t valueEnd = m_ApiAuthCode.find_first_of("&#", valueStart);
			m_WebSiteID = (valueEnd == std::string::npos) ? m_ApiAuthCode.substr(valueStart) : m_ApiAuthCode.substr(valueStart, valueEnd - valueStart);
		}
	}

	Json::Value body;
	body["grant_type"] = "authorization_code";
	body["code"] = code;
	body["client_id"] = m_ApiClientId;
	body["client_secret"] = m_ApiClientSecret;

	std::vector<std::string> ExtraHeaders;
	ExtraHeaders.push_back("Content-Type: application/json");

	std::string sResult;
	std::vector<std::string> vHeaderData;
	HTTPClient::POST(SE_OAUTH_TOKEN_URL, JSonToRawString(body), ExtraHeaders, sResult, vHeaderData, true, true);

	int iStatusCode = LastHttpStatusCode(vHeaderData);
	Debug(DEBUG_HARDWARE, "Monitoring API: OAuth code exchange status %d", iStatusCode);

	Json::Value root;
	if (iStatusCode != 200 || !ParseJSon(sResult, root) || !root.isObject() || root["access_token"].empty() || root["refresh_token"].empty())
	{
		Log(LOG_ERROR, "Monitoring API: Failed to exchange the authorization code for a token! Check the Client ID/Secret, and that the code hasn't already been used or expired.");
		return false;
	}

	m_ApiAccessToken = root["access_token"].asString();
	m_ApiRefreshToken = root["refresh_token"].asString();
	int expiresIn = root.get("expires_in", 7200).asInt();
	int refreshIn = (expiresIn * 2) / 3;
	if (refreshIn < 30)
		refreshIn = 30; // never schedule the next refresh in the past
	m_ApiNextRefreshTs = mytime(nullptr) + refreshIn;
	StoreApiRefreshToken();

	// The authorization code is single-use; drop it from the Hardware row so we don't try to
	// redeem it again on the next restart (it would just fail, but there's no reason to try).
	m_ApiAuthCode.clear();
	m_sql.safe_query("UPDATE Hardware SET Extra='%q|%q|%q|%q|' WHERE (ID == %d)", m_WebUsername.c_str(), m_WebSiteID.c_str(), base64_encode(m_ApiClientId).c_str(),
			  base64_encode(m_ApiClientSecret).c_str(), m_HwdID);

	Log(LOG_STATUS, "Monitoring API: OAuth authorization succeeded");
	return true;
}

bool SolarEdgeAPI::ApiRefreshToken()
{
	if (m_ApiRefreshToken.empty() || m_ApiClientId.empty() || m_ApiClientSecret.empty())
		return false;

	Json::Value body;
	body["grant_type"] = "refresh_token";
	body["refresh_token"] = m_ApiRefreshToken;
	body["client_id"] = m_ApiClientId;
	body["client_secret"] = m_ApiClientSecret;

	std::vector<std::string> ExtraHeaders;
	ExtraHeaders.push_back("Content-Type: application/json");

	std::string sResult;
	std::vector<std::string> vHeaderData;
	HTTPClient::POST(SE_OAUTH_TOKEN_URL, JSonToRawString(body), ExtraHeaders, sResult, vHeaderData, true, true);

	int iStatusCode = LastHttpStatusCode(vHeaderData);
	Debug(DEBUG_HARDWARE, "Monitoring API: OAuth token refresh status %d", iStatusCode);

	Json::Value root;
	// Each refresh returns a fresh access+refresh token pair; the old refresh token is invalidated
	if (iStatusCode != 200 || !ParseJSon(sResult, root) || !root.isObject() || root["access_token"].empty() || root["refresh_token"].empty())
	{
		Log(LOG_ERROR, "Monitoring API: Failed to refresh the OAuth access token! A new authorization code will be needed (see hardware settings).");
		m_ApiAccessToken.clear();
		m_ApiRefreshToken.clear();
		return false;
	}

	m_ApiAccessToken = root["access_token"].asString();
	m_ApiRefreshToken = root["refresh_token"].asString();
	int expiresIn = root.get("expires_in", 7200).asInt();
	int refreshIn = (expiresIn * 2) / 3;
	if (refreshIn < 30)
		refreshIn = 30;
	m_ApiNextRefreshTs = mytime(nullptr) + refreshIn;
	StoreApiRefreshToken();

	Debug(DEBUG_HARDWARE, "Monitoring API: OAuth token refresh succeeded");
	return true;
}

bool SolarEdgeAPI::ApiEnsureLoggedIn()
{
	if (!ApiUsesOAuth())
		return true; // static Fleet API Key, nothing to refresh

	if (!m_ApiAccessToken.empty() && (mytime(nullptr) - 15) < m_ApiNextRefreshTs)
		return true;

	if (!m_ApiRefreshToken.empty())
	{
		if (ApiRefreshToken())
			return true;
	}

	if (!m_ApiAuthCode.empty())
		return ApiExchangeAuthCode();

	// No manually-pasted code: attempt a fully automatic authorization using the SolarEdge SSO
	// login already configured for the Web Portal (same account, same credentials).
	if (!m_WebUsername.empty() && !m_WebPassword.empty())
		return ApiAutoAuthorize();

	Log(LOG_ERROR, "Monitoring API: No Fleet API Key, no valid OAuth token, and no Web Username/Password configured for automatic authorization!");
	return false;
}

bool SolarEdgeAPI::ApiAutoAuthorize()
{
	if (m_ApiClientId.empty() || m_ApiClientSecret.empty())
		return false;

	std::vector<std::string> jsonHeaders;
	jsonHeaders.push_back("Content-Type: application/json");
	jsonHeaders.push_back("Accept: application/json");

	// Step 1: ask SolarEdge Connect for the (internal) SSO authorize URL and an opaque transaction blob
	Json::Value reqBody;
	reqBody["clientId"] = m_ApiClientId;

	std::string sResult;
	std::vector<std::string> vHeaderData;
	HTTPClient::POST(SE_CONNECT_LOGIN_REQUEST_URL, JSonToRawString(reqBody), jsonHeaders, sResult, vHeaderData, true, true);
	int iStatusCode = LastHttpStatusCode(vHeaderData);
	Debug(DEBUG_HARDWARE, "Monitoring API: OAuth auto-authorize step 1 (login-request) status %d", iStatusCode);

	Json::Value root;
	if (iStatusCode != 200 || !ParseJSon(sResult, root) || !root.isObject() || root["authorizeUrl"].empty() || root["transaction"].empty())
	{
		Log(LOG_ERROR, "Monitoring API: OAuth auto-authorize failed requesting a login session!");
		return false;
	}
	std::string authorizeUrl = root["authorizeUrl"].asString();
	std::string transaction = root["transaction"].asString();

	// Step 2: follow it through to the SolarEdge SSO login form (same identity provider the Web Portal uses)
	std::vector<std::string> getHeaders;
	getHeaders.push_back("Accept: text/html");

	std::string sLoginPage;
	if (!HTTPClient::GET(authorizeUrl, getHeaders, sLoginPage) || sLoginPage.empty())
	{
		Log(LOG_ERROR, "Monitoring API: OAuth auto-authorize failed loading the SolarEdge login form!");
		return false;
	}

	std::string formAction;
	std::map<std::string, std::string> formFields;
	if (!ParseLoginForm(sLoginPage, formAction, formFields))
	{
		Log(LOG_ERROR, "Monitoring API: OAuth auto-authorize failed, could not find the login form!");
		return false;
	}

	if (formAction.compare(0, 4, "http") != 0)
	{
		if (formAction.empty() || formAction[0] != '/')
			formAction = "/" + formAction;
		formAction = "https://login.solaredge.com" + formAction;
	}

	static const std::regex hostRegex("^https?://([^/:]+)", std::regex::icase);
	std::smatch hostMatch;
	std::string host;
	if (std::regex_search(formAction, hostMatch, hostRegex))
		host = hostMatch[1].str();
	std::transform(host.begin(), host.end(), host.begin(), ::tolower);
	bool bHostOk = (host == "solaredge.com") || (host.size() > 14 && host.compare(host.size() - 14, 14, ".solaredge.com") == 0);
	if (!bHostOk)
	{
		Log(LOG_ERROR, "Monitoring API: OAuth auto-authorize login form does not point to a solaredge.com host, aborting for safety!");
		return false;
	}

	// Step 3: submit the SolarEdge SSO credentials (the same account as the Web Portal login).
	// The form ships a "cognitoAsfData" field as empty; that's a client-side device-fingerprint
	// signal used only for risk scoring, submitting it empty is accepted like any other login.
	formFields["username"] = m_WebUsername;
	formFields["password"] = m_WebPassword;

	std::string postData;
	for (const auto& field : formFields)
	{
		if (!postData.empty())
			postData += "&";
		postData += CURLEncode::URLEncode(field.first) + "=" + CURLEncode::URLEncode(field.second);
	}

	std::vector<std::string> loginHeaders;
	loginHeaders.push_back("Content-Type: application/x-www-form-urlencoded");

	std::string sLoginResult;
	std::vector<std::string> vLoginHeaderData;
	HTTPClient::POST(formAction, postData, loginHeaders, sLoginResult, vLoginHeaderData, true, true);

	std::string ssoCode = ExtractLocationParam(vLoginHeaderData, "code=");
	std::string ssoState = ExtractLocationParam(vLoginHeaderData, "state=");
	Debug(DEBUG_HARDWARE, "Monitoring API: OAuth auto-authorize step 3 (credentials) %s", ssoCode.empty() ? "failed" : "succeeded");
	if (ssoCode.empty() || ssoState.empty())
	{
		Log(LOG_ERROR, "Monitoring API: OAuth auto-authorize failed submitting credentials! Check the Web Username/Password (2FA-protected accounts aren't supported here; use a manually pasted authorization code instead).");
		return false;
	}

	// Step 4: exchange that SSO code for a SolarEdge Connect session
	Json::Value tokenBody;
	tokenBody["code"] = ssoCode;
	tokenBody["state"] = ssoState;
	tokenBody["transaction"] = transaction;

	std::string sSessionResult;
	std::vector<std::string> vSessionHeaderData;
	HTTPClient::POST(SE_CONNECT_SESSION_TOKEN_URL, JSonToRawString(tokenBody), jsonHeaders, sSessionResult, vSessionHeaderData, true, true);
	int iSessionStatus = LastHttpStatusCode(vSessionHeaderData);
	Debug(DEBUG_HARDWARE, "Monitoring API: OAuth auto-authorize step 4 (session) status %d", iSessionStatus);

	Json::Value sessionRoot;
	if (iSessionStatus != 200 || !ParseJSon(sSessionResult, sessionRoot) || !sessionRoot.isObject() || sessionRoot["accessToken"].empty())
	{
		Log(LOG_ERROR, "Monitoring API: OAuth auto-authorize failed establishing a Connect session!");
		return false;
	}
	std::string sessionToken = sessionRoot["accessToken"].asString();

	// Step 5: approve the application's access request on the user's behalf
	Json::Value approveBody;
	approveBody["transaction"] = transaction;

	std::vector<std::string> approveHeaders;
	approveHeaders.push_back("Content-Type: application/json");
	approveHeaders.push_back("Accept: application/json");
	approveHeaders.push_back("Authorization: Bearer " + sessionToken);

	std::string sApproveResult;
	std::vector<std::string> vApproveHeaderData;
	HTTPClient::POST(SE_CONNECT_APPROVE_URL_PREFIX + m_ApiClientId + "/approve", JSonToRawString(approveBody), approveHeaders, sApproveResult, vApproveHeaderData, true, true);
	int iApproveStatus = LastHttpStatusCode(vApproveHeaderData);
	Debug(DEBUG_HARDWARE, "Monitoring API: OAuth auto-authorize step 5 (approve) status %d", iApproveStatus);

	Json::Value approveRoot;
	if (iApproveStatus != 200 || !ParseJSon(sApproveResult, approveRoot) || !approveRoot.isObject() || approveRoot["uri"].empty())
	{
		Log(LOG_ERROR, "Monitoring API: OAuth auto-authorize failed approving access!");
		return false;
	}

	// The approved "uri" carries our application's own authorization code and Site ID, e.g.
	// "http://localhost?code=...&site_id=...". Feed it through the same parser used for a
	// manually pasted callback URL, then exchange it for an access/refresh token pair.
	m_ApiAuthCode = approveRoot["uri"].asString();
	Log(LOG_STATUS, "Monitoring API: OAuth auto-authorize succeeded, exchanging the authorization code");
	return ApiExchangeAuthCode();
}

bool SolarEdgeAPI::GetSite()
{
	m_SiteID = 0;

	if (ApiUsesOAuth())
	{
		// Fleet-wide Site List (GET /sites) is X-API-Key only and rejects OAuth bearer tokens entirely,
		// so under Site Access we rely on the Site ID captured from the OAuth callback (or set manually).
		if (!ApiEnsureLoggedIn())
			return false;
		if (m_WebSiteID.empty())
		{
			Log(LOG_ERROR, "Monitoring API: OAuth Site Access requires a Site ID! It's normally captured automatically from the authorization callback URL; otherwise set it manually in the hardware settings.");
			return false;
		}
		m_SiteID = atoi(m_WebSiteID.c_str());
		return (m_SiteID != 0);
	}

	std::string sResult;
#ifdef DEBUG_SolarEdgeAPIR
	sResult = ReadFile("E:\\SolarEdge_sites.json");
#else

	std::vector<std::string> ExtraHeaders;
	ExtraHeaders.push_back("Accept: application/json");
	ExtraHeaders.push_back(BuildApiAuthHeader());

	std::stringstream sURL;
	sURL << SE_API_BASE_URL << "/sites?page=1&sites-in-page=1";

	std::vector<std::string> vHeaderData;
	if (!HTTPClient::GET(sURL.str(), ExtraHeaders, sResult, vHeaderData))
	{
		Log(LOG_ERROR, "Error getting http data (Sites)!");
		return false;
	}
	Debug(DEBUG_HARDWARE, "API: Sites status %d", LastHttpStatusCode(vHeaderData));
#ifdef DEBUG_SolarEdgeAPIW
	SaveString2Disk(sResult, "E:\\SolarEdge_sites.json");
#endif
#endif
	Json::Value root;

	bool ret = ParseJSon(sResult, root);
	if ((!ret) || (!root.isObject()))
	{
		Log(LOG_ERROR, "Invalid data received!");
		return false;
	}
	if (root["sites"].empty() == true)
	{
		Log(LOG_ERROR, "Invalid data received, or invalid APIKey");
		return false;
	}
	if (root["sites"]["count"].empty() == true)
	{
		Log(LOG_ERROR, "Invalid data received, or invalid APIKey");
		return false;
	}
	int tot_results = root["sites"]["count"].asInt();
	if (tot_results < 1)
		return false;
	Json::Value reading = root["sites"]["site"][0];

	if (reading["siteId"].empty() == true)
	{
		Log(LOG_ERROR, "Invalid data received, or invalid APIKey");
		return false;
	}
	m_SiteID = reading["siteId"].asInt();
	if (m_WebSiteID.empty())
		m_WebSiteID = std::to_string(m_SiteID);
	return true;
}

void SolarEdgeAPI::GetBatteryFromInventory()
{
	if (!ApiEnsureLoggedIn())
		return;

	std::string sResult;

	std::vector<std::string> ExtraHeaders;
	ExtraHeaders.push_back("Accept: application/json");
	ExtraHeaders.push_back(BuildApiAuthHeader());

	std::stringstream sURL;
	sURL << SE_API_BASE_URL << "/sites/" << m_SiteID << "/devices";

	std::vector<std::string> vHeaderData;
	if (!HTTPClient::GET(sURL.str(), ExtraHeaders, sResult, vHeaderData))
	{
		Log(LOG_ERROR, "Error getting http data (Inventory)!");
		return;
	}
	Debug(DEBUG_HARDWARE, "API: Inventory status %d", LastHttpStatusCode(vHeaderData));

	Json::Value root;

	bool ret = ParseJSon(sResult, root);
	if ((!ret) || (!root.isArray()))
	{
		Log(LOG_ERROR, "Invalid data received!");
		return;
	}

	m_bPollBattery = false;
	int inverterCount = 0;
	std::string szModel, szFirmware;
	for (const auto& device : root)
	{
		std::string type = device.get("type", "").asString();
		if (type == "BATTERY")
			m_bPollBattery = true;
		else if (type == "INVERTER")
		{
			inverterCount++;
			if (szModel.empty())
			{
				szModel = device.get("partNumber", device.get("model", "")).asString();
				szFirmware = device.get("firmwareVersion", "").asString();
			}
		}
	}

	// Model and firmware of the first inverter, shown in the hardware overview
	if (!szModel.empty())
	{
		std::string szVersion = szModel;
		if (!szFirmware.empty())
			szVersion += " (fw " + szFirmware + ")";
		if (inverterCount > 1)
			szVersion += " +" + std::to_string(inverterCount - 1) + " more";
		if (szVersion != m_szSoftwareVersion)
		{
			m_szSoftwareVersion = szVersion;
			Log(LOG_STATUS, "Inverter: %s", m_szSoftwareVersion.c_str());
		}
	}
}

void SolarEdgeAPI::GetInverters()
{
	m_inverters.clear();
	if (!ApiEnsureLoggedIn())
		return;

	std::string sResult;
#ifdef DEBUG_SolarEdgeAPIR
	sResult = ReadFile("E:\\SolarEdge_inverters.json");
#else

	std::vector<std::string> ExtraHeaders;
	ExtraHeaders.push_back("Accept: application/json");
	ExtraHeaders.push_back(BuildApiAuthHeader());

	std::stringstream sURL;
	sURL << SE_API_BASE_URL << "/sites/" << m_SiteID << "/devices?types=INVERTER";

	std::vector<std::string> vHeaderData;
	if (!HTTPClient::GET(sURL.str(), ExtraHeaders, sResult, vHeaderData))
	{
		Log(LOG_ERROR, "Error getting http data (Equipment)!");
		return;
	}
	Debug(DEBUG_HARDWARE, "API: Equipment status %d", LastHttpStatusCode(vHeaderData));
#ifdef DEBUG_SolarEdgeAPIW
	SaveString2Disk(sResult, "E:\\SolarEdge_inverters.json");
#endif
#endif
	Json::Value root;

	bool ret = ParseJSon(sResult, root);
	if ((!ret) || (!root.isArray()))
	{
		Log(LOG_ERROR, "Invalid data received!");
		return;
	}

	for (const auto& reading : root)
	{
		if (reading["serialNumber"].empty() == true)
			continue;
		_tInverterSettings iSettings;
		iSettings.name = reading.get("name", "").asString();
		iSettings.manufacturer = reading.get("manufacturer", "").asString();
		iSettings.model = reading.get("model", "").asString();
		iSettings.SN = reading["serialNumber"].asString();
		if (iSettings.name.empty())
			iSettings.name = iSettings.SN;
		m_inverters.push_back(iSettings);
	}
	m_lastInverterEnergy.assign(m_inverters.size(), 0.0);
}

void SolarEdgeAPI::GetInverterTelemetry()
{
	m_totalActivePower = 0;
	m_totalEnergy = 0;

	if (m_inverters.empty())
		return;

	//We only poll one hour before sunrise till one hour after sunset
	if (!isDaylightWindow())
		return;

	if (!ApiEnsureLoggedIn())
		return;

	std::vector<std::string> ExtraHeaders;
	ExtraHeaders.push_back("Accept: application/json");
	ExtraHeaders.push_back(BuildApiAuthHeader());

	time_t now = mytime(nullptr);
	std::string szNow = FormatApiUtcTime(now);

	std::map<std::string, float> powerBySN, voltageBySN, currentBySN, frequencyBySN;
	std::map<std::string, double> energyBySN;

	// Current power/voltage/current/frequency, one bulk call for every inverter on the site
	{
		std::string szFrom = FormatApiUtcTime(now - 900); // last 15 minutes

		std::stringstream sURL;
		sURL << SE_API_BASE_URL << "/sites/" << m_SiteID << "/inverters/telemetry"
			<< "?resolution=QUARTER_HOUR&from=" << CURLEncode::URLEncode(szFrom) << "&to=" << CURLEncode::URLEncode(szNow);

		std::string sResult;
		std::vector<std::string> vHeaderData;
		if (!HTTPClient::GET(sURL.str(), ExtraHeaders, sResult, vHeaderData))
			Log(LOG_ERROR, "Error getting http data (Inverter telemetry)!");
		else
		{
			Debug(DEBUG_HARDWARE, "API: Inverter telemetry status %d", LastHttpStatusCode(vHeaderData));
			Json::Value root;
			if (!ParseJSon(sResult, root) || !root.isObject() || root["inverters"].empty())
				Log(LOG_ERROR, "Invalid data received, or invalid APIKey (Inverter telemetry)!");
			else
			{
				const Json::Value& inverters = root["inverters"];
				for (const auto& sn : inverters.getMemberNames())
				{
					const Json::Value& node = inverters[sn];
					if (!node["power"].empty())
						powerBySN[sn] = (float)LastMetricValue(node["power"]);
					if (!node["voltage"].empty())
						voltageBySN[sn] = (float)LastMetricValue(node["voltage"]);
					if (!node["current"].empty())
						currentBySN[sn] = (float)LastMetricValue(node["current"]);
					if (!node["frequency"].empty())
						frequencyBySN[sn] = (float)LastMetricValue(node["frequency"]);
				}
			}
		}
	}

	// Lifetime energy (TOTAL resolution), the closest V2 equivalent of the old cumulative inverter counter
	{
		std::stringstream sURL;
		sURL << SE_API_BASE_URL << "/sites/" << m_SiteID << "/inverters/telemetry"
			<< "?resolution=TOTAL&from=" << CURLEncode::URLEncode(std::string("2000-01-01T00:00:00Z")) << "&to=" << CURLEncode::URLEncode(szNow);

		std::string sResult;
		std::vector<std::string> vHeaderData;
		if (!HTTPClient::GET(sURL.str(), ExtraHeaders, sResult, vHeaderData))
			Log(LOG_ERROR, "Error getting http data (Inverter lifetime energy)!");
		else
		{
			Debug(DEBUG_HARDWARE, "API: Inverter lifetime energy status %d", LastHttpStatusCode(vHeaderData));
			Json::Value root;
			if (!ParseJSon(sResult, root) || !root.isObject() || root["inverters"].empty())
				Log(LOG_ERROR, "Invalid data received, or invalid APIKey (Inverter lifetime energy)!");
			else
			{
				const Json::Value& inverters = root["inverters"];
				for (const auto& sn : inverters.getMemberNames())
				{
					const Json::Value& node = inverters[sn];
					if (!node["energy"].empty())
						energyBySN[sn] = LastMetricValue(node["energy"]);
				}
			}
		}
	}

	char szTmp[200];
	for (int i = 0; i < (int)m_inverters.size(); i++)
	{
		const _tInverterSettings& inv = m_inverters[i];
		float curActivePower = 0;

		auto itP = powerBySN.find(inv.SN);
		if (itP != powerBySN.end())
		{
			curActivePower = itP->second;
			m_totalActivePower += curActivePower;
			sprintf(szTmp, "Power %s", inv.name.c_str());
			SendWattMeter(1 + i, 1, 255, curActivePower, szTmp);
		}

		auto itE = energyBySN.find(inv.SN);
		if (itE != energyBySN.end())
		{
			double curEnergy = itE->second;
			if (curEnergy != 0)
			{
				sprintf(szTmp, "kWh Meter %s", inv.name.c_str());
				SendKwhMeter(0, 1 + i, 255, curActivePower, curEnergy / 1000.0, szTmp);
			}
			if (i < (int)m_lastInverterEnergy.size())
				m_lastInverterEnergy[i] = curEnergy;
			m_totalEnergy += curEnergy;
		}

		// V2 no longer exposes DC voltage; this is now the AC voltage averaged across active phases
		auto itV = voltageBySN.find(inv.SN);
		if (itV != voltageBySN.end())
		{
			sprintf(szTmp, "AC %s", inv.name.c_str());
			SendVoltageSensor(i, SE_VOLT_DC, 255, itV->second, szTmp);
		}
		// V2 no longer exposes a per-phase breakdown; this is a derived value (power / voltage), averaged across active phases
		auto itC = currentBySN.find(inv.SN);
		if (itC != currentBySN.end())
		{
			sprintf(szTmp, "acCurrent %s", inv.name.c_str());
			SendCustomSensor(i, SE_AC_CURRENT, 255, itC->second, szTmp, "A");
		}
		auto itF = frequencyBySN.find(inv.SN);
		if (itF != frequencyBySN.end())
		{
			sprintf(szTmp, "Hz %s", inv.name.c_str());
			SendCustomSensor(1 + i, 1, 255, itF->second, szTmp, "Hz");
		}
	}

	if ((m_inverters.size() > 1) && (m_totalEnergy > 0))
	{
		//Send total kWh
		SendKwhMeter(1, 1, 255, m_totalActivePower, m_totalEnergy / 1000.0, "kWh Meter Total");
	}
}

void SolarEdgeAPI::GetBatteryDetails()
{
	if (!ApiEnsureLoggedIn())
		return;

	// V1's currentPowerFlow endpoint moved to the paid-tier-only Advanced Monitoring API in V2.
	// Grid/Load/PV power is available on every tier via the site-wide meter telemetry bulk endpoint instead.
	std::vector<std::string> ExtraHeaders;
	ExtraHeaders.push_back("Accept: application/json");
	ExtraHeaders.push_back(BuildApiAuthHeader());

	time_t now = mytime(nullptr);
	std::string szFrom = FormatApiUtcTime(now - 900); // last 15 minutes
	std::string szTo = FormatApiUtcTime(now);

	{
		std::stringstream sURL;
		sURL << SE_API_BASE_URL << "/sites/" << m_SiteID << "/meters/telemetry"
			<< "?resolution=QUARTER_HOUR&from=" << CURLEncode::URLEncode(szFrom) << "&to=" << CURLEncode::URLEncode(szTo);

		std::string sResult;
		std::vector<std::string> vHeaderData;
		if (!HTTPClient::GET(sURL.str(), ExtraHeaders, sResult, vHeaderData))
			Log(LOG_ERROR, "Error getting http data (Meter telemetry)!");
		else
		{
			Debug(DEBUG_HARDWARE, "API: Meter telemetry status %d", LastHttpStatusCode(vHeaderData));
			Json::Value root;
			// An empty "meters" object is a normal response for a site with no separate physical
			// meter device (inverter-only systems, common without a net-metering CT clamp) - not an error.
			if (!ParseJSon(sResult, root) || !root.isObject())
				Log(LOG_ERROR, "Invalid data received (Meter telemetry)!");
			else
			{
				double production = 0, consumption = 0, imported = 0, exported = 0;
				bool bHaveProduction = false, bHaveConsumption = false, bHaveGrid = false;
				const Json::Value& meters = root["meters"];
				for (const auto& sn : meters.getMemberNames())
				{
					const Json::Value& meter = meters[sn];
					if (!meter["productionPower"].empty())
					{
						production += LastMetricValue(meter["productionPower"]);
						bHaveProduction = true;
					}
					if (!meter["consumptionPower"].empty())
					{
						consumption += LastMetricValue(meter["consumptionPower"]);
						bHaveConsumption = true;
					}
					if (!meter["importPower"].empty())
					{
						imported += LastMetricValue(meter["importPower"]);
						bHaveGrid = true;
					}
					if (!meter["exportPower"].empty())
					{
						exported += LastMetricValue(meter["exportPower"]);
						bHaveGrid = true;
					}
				}
				if (bHaveProduction)
					SendWattMeter(200, SE_PV, 255, (float)production, "PV Power");
				if (bHaveConsumption)
					SendWattMeter(200, SE_LOAD, 255, (float)consumption, "Load Power");
				if (bHaveGrid)
				{
					// positive = importing from grid, negative = exporting to grid (matches the old currentPowerFlow convention)
					SendWattMeter(200, SE_GRID, 255, (float)(imported - exported), "Grid Power");
				}
			}
		}
	}

	if (!m_bPollBattery)
		return;

	std::stringstream sURL;
	sURL << SE_API_BASE_URL << "/sites/" << m_SiteID << "/storage/telemetry"
		<< "?resolution=QUARTER_HOUR&from=" << CURLEncode::URLEncode(szFrom) << "&to=" << CURLEncode::URLEncode(szTo);

	std::string sResult;
	std::vector<std::string> vHeaderData;
	if (!HTTPClient::GET(sURL.str(), ExtraHeaders, sResult, vHeaderData))
	{
		Log(LOG_ERROR, "Error getting http data (Storage telemetry)!");
		return;
	}
	Debug(DEBUG_HARDWARE, "API: Storage telemetry status %d", LastHttpStatusCode(vHeaderData));

	Json::Value root;
	if (!ParseJSon(sResult, root) || !root.isObject() || root["storage"].empty())
	{
		Log(LOG_ERROR, "Invalid data received (Storage telemetry)!");
		return;
	}

	double chargePower = 0, dischargePower = 0, socSum = 0;
	int socCount = 0;
	const Json::Value& storage = root["storage"];
	for (const auto& sn : storage.getMemberNames())
	{
		const Json::Value& batt = storage[sn];
		if (!batt["chargePower"].empty())
			chargePower += LastMetricValue(batt["chargePower"]);
		if (!batt["dischargePower"].empty())
			dischargePower += LastMetricValue(batt["dischargePower"]);
		if (!batt["stateOfEnergy"].empty())
		{
			socSum += LastMetricValue(batt["stateOfEnergy"]);
			socCount++;
		}
	}

	// positive = discharging, negative = charging (matches the old currentPowerFlow convention)
	float batteryPower = (float)(dischargePower - chargePower);
	SendWattMeter(200, SE_STORAGE_POWER, 255, batteryPower, "Battery Power");

	// V2 has no direct "status" string; derive it from the power flow instead
	std::string status = (chargePower > 0) ? "Charging" : (dischargePower > 0) ? "Discharging" : "Idle";
	SendTextSensor(200, SE_STORAGE_STATUS, 255, status, "Battery Status");

	if (socCount > 0)
		SendPercentageSensor(200, SE_STORAGE_CHARGELEVEL, 255, (float)(socSum / socCount), "Battery Charge Level");

	// Note: V1's "critical" battery flag has no V2 equivalent, so it is no longer sent.
}

void SolarEdgeAPI::GetOverview()
{
	if (!ApiEnsureLoggedIn())
		return;

	std::vector<std::string> ExtraHeaders;
	ExtraHeaders.push_back("Accept: application/json");
	ExtraHeaders.push_back(BuildApiAuthHeader());

	// Current site power (Site Overview no longer carries an instantaneous power figure in V2)
	{
		time_t now = mytime(nullptr);
		std::stringstream sURL;
		sURL << SE_API_BASE_URL << "/sites/" << m_SiteID << "/power"
			<< "?resolution=QUARTER_HOUR&from=" << CURLEncode::URLEncode(FormatApiUtcTime(now - 900)) << "&to=" << CURLEncode::URLEncode(FormatApiUtcTime(now));

		std::string sResult;
		std::vector<std::string> vHeaderData;
		if (!HTTPClient::GET(sURL.str(), ExtraHeaders, sResult, vHeaderData))
			Log(LOG_ERROR, "Error getting http data (Site Power)!");
		else
		{
			Debug(DEBUG_HARDWARE, "API: Site Power status %d", LastHttpStatusCode(vHeaderData));
			Json::Value root;
			if (ParseJSon(sResult, root) && root.isObject() && !root["values"].empty())
				SendWattMeter(200, SE_OVERVIEW_CURRENT, 255, (float)LastMetricValue(root), "Site Current Power");
		}
	}

	// Today's production/consumption breakdown (defaults to midnight-today .. now when from/to are omitted).
	// This also replaces the old, separate EnergyDetails call: V2's per-category energy is already in this response.
	std::string sResult;
#ifdef DEBUG_SolarEdgeAPIR
	sResult = ReadFile("E:\\SolarEdge_overview.json");
#else
	std::stringstream sURL;
	sURL << SE_API_BASE_URL << "/sites/" << m_SiteID << "/overview";

	std::vector<std::string> vHeaderData;
	if (!HTTPClient::GET(sURL.str(), ExtraHeaders, sResult, vHeaderData))
	{
		Log(LOG_ERROR, "Error getting http data (Overview)!");
		return;
	}
	Debug(DEBUG_HARDWARE, "API: Overview status %d", LastHttpStatusCode(vHeaderData));
#ifdef DEBUG_SolarEdgeAPIW
	SaveString2Disk(sResult, "E:\\SolarEdge_overview.json");
#endif
#endif
	Json::Value root;

	bool ret = ParseJSon(sResult, root);
	if ((!ret) || (!root.isObject()))
	{
		Log(LOG_ERROR, "Invalid data received!");
		return;
	}

	const Json::Value& production = root["production"];
	const Json::Value& consumption = root["consumption"];

	if (!production.empty() && !production["total"].empty())
	{
		double energyWh = ToWattHours(production["total"].asDouble(), production.get("unit", "WH").asString());
		SendCustomSensor(200, SE_OVERVIEW_TODAY, 255, (float)(energyWh / 1000.0), "Energy Today", "kWh");
		SendCustomSensor(201, SE_ENERGY_PRODUCTION, 255, (float)(energyWh / 1000.0), "Energy Production", "kWh");
	}
	if (!production.empty() && !production["toSelfConsumption"].empty())
	{
		double energyWh = ToWattHours(production["toSelfConsumption"].asDouble(), production.get("unit", "WH").asString());
		SendCustomSensor(201, SE_ENERGY_SELFCONSUMPTION, 255, (float)(energyWh / 1000.0), "Energy Self Consumption", "kWh");
	}
	if (!production.empty() && !production["toGrid"].empty())
	{
		double energyWh = ToWattHours(production["toGrid"].asDouble(), production.get("unit", "WH").asString());
		SendCustomSensor(201, SE_ENERGY_FEEDIN, 255, (float)(energyWh / 1000.0), "Energy Feed In", "kWh");
	}
	if (!consumption.empty() && !consumption["total"].empty())
	{
		double energyWh = ToWattHours(consumption["total"].asDouble(), consumption.get("unit", "WH").asString());
		SendCustomSensor(201, SE_ENERGY_CONSUMPTION, 255, (float)(energyWh / 1000.0), "Energy Consumption", "kWh");
	}
	if (!consumption.empty() && !consumption["fromGrid"].empty())
	{
		double energyWh = ToWattHours(consumption["fromGrid"].asDouble(), consumption.get("unit", "WH").asString());
		SendCustomSensor(201, SE_ENERGY_PURCHASED, 255, (float)(energyWh / 1000.0), "Energy Purchased", "kWh");
	}
}

void SolarEdgeAPI::GetSiteEnergyTotals()
{
	if (!ApiEnsureLoggedIn())
		return;

	std::vector<std::string> ExtraHeaders;
	ExtraHeaders.push_back("Accept: application/json");
	ExtraHeaders.push_back(BuildApiAuthHeader());

	time_t now = mytime(nullptr);
	struct tm ltime;
	localtime_r(&now, &ltime);

	char szMonthFrom[40], szYearFrom[40];
	snprintf(szMonthFrom, sizeof(szMonthFrom), "%04d-%02d-01T00:00:00Z", ltime.tm_year + 1900, ltime.tm_mon + 1);
	snprintf(szYearFrom, sizeof(szYearFrom), "%04d-01-01T00:00:00Z", ltime.tm_year + 1900);
	std::string szNow = FormatApiUtcTime(now);

	struct _tEnergyQuery
	{
		std::string from;
		int sensorId;
		const char* label;
	};
	const _tEnergyQuery queries[] = {
		{ szMonthFrom, SE_OVERVIEW_MONTH, "Energy This Month" },
		{ szYearFrom, SE_OVERVIEW_YEAR, "Energy This Year" },
		{ "2000-01-01T00:00:00Z", SE_OVERVIEW_LIFETIME, "Lifetime Energy" },
	};

	for (const auto& query : queries)
	{
		std::stringstream sURL;
		sURL << SE_API_BASE_URL << "/sites/" << m_SiteID << "/energy"
			<< "?resolution=TOTAL&from=" << CURLEncode::URLEncode(query.from) << "&to=" << CURLEncode::URLEncode(szNow);

		std::string sResult;
		std::vector<std::string> vHeaderData;
		if (!HTTPClient::GET(sURL.str(), ExtraHeaders, sResult, vHeaderData))
		{
			Log(LOG_ERROR, "Error getting http data (Site Energy - %s)!", query.label);
			continue;
		}
		Debug(DEBUG_HARDWARE, "API: Site Energy (%s) status %d", query.label, LastHttpStatusCode(vHeaderData));

		Json::Value root;
		if (!ParseJSon(sResult, root) || !root.isObject() || root["values"].empty())
		{
			Log(LOG_ERROR, "Invalid data received (Site Energy - %s)!", query.label);
			continue;
		}
		double energyWh = ToWattHours(LastMetricValue(root), root.get("unit", "WH").asString());
		SendCustomSensor(200, query.sensorId, 255, (float)(energyWh / 1000.0), query.label, "kWh");
	}
}

std::string SolarEdgeAPI::GetWebTokenPrefKey() const
{
	return "SolarEdgeWebToken_" + std::to_string(m_HwdID);
}

bool SolarEdgeAPI::LoadWebRefreshToken()
{
	// Earlier builds kept the token in the Hardware row, where the web interface displays it
	auto result = m_sql.safe_query("SELECT Address, SerialPort FROM Hardware WHERE (ID==%d)", m_HwdID);
	if (!result.empty() && !result[0][0].empty())
	{
		m_WebRefreshToken = result[0][0];
		if (!result[0][1].empty())
			m_WebNextRefreshTs = std::stol(result[0][1]);
		m_sql.safe_query("UPDATE Hardware SET Address='', SerialPort='' WHERE (ID == %d)", m_HwdID);
		StoreWebRefreshToken();
		return true;
	}

	int nValue = 0;
	std::string sValue;
	if (!m_sql.GetPreferencesVar(GetWebTokenPrefKey(), nValue, sValue))
		return false;
	m_WebRefreshToken = sValue;
	m_WebNextRefreshTs = nValue;
	return !m_WebRefreshToken.empty();
}

void SolarEdgeAPI::StoreWebRefreshToken()
{
	if (m_WebRefreshToken.empty())
		return;
	// Kept out of the Hardware row on purpose, the web interface returns every column of it
	m_sql.UpdatePreferencesVar(GetWebTokenPrefKey(), (int)m_WebNextRefreshTs, m_WebRefreshToken);
}

std::string SolarEdgeAPI::GetCookieValue(const std::string& name) const
{
	extern std::string szUserDataFolder;
	std::string sPath = szUserDataFolder + "domocookie.txt";
	std::ifstream file(sPath);
	if (!file.is_open())
		return "";
	std::string sLine;
	while (std::getline(file, sLine))
	{
		if (sLine.empty() || sLine[0] == '#')
			continue;
		std::vector<std::string> fields;
		StringSplit(sLine, "\t", fields);
		if (fields.size() < 7)
			continue;
		if (fields[5] == name)
			return fields[6];
	}
	return "";
}

bool SolarEdgeAPI::ParseLoginForm(const std::string& html, std::string& formAction, std::map<std::string, std::string>& fields) const
{
	static const std::regex formRegex("<form[^>]*action=\"([^\"]*)\"", std::regex::icase);
	std::smatch formMatch;
	if (!std::regex_search(html, formMatch, formRegex))
		return false;
	formAction = formMatch[1].str();

	// Unescape the only HTML entity that realistically shows up in a form action URL
	size_t pos = 0;
	while ((pos = formAction.find("&amp;", pos)) != std::string::npos)
	{
		formAction.replace(pos, 5, "&");
		pos += 1;
	}

	static const std::regex inputRegex("<input[^>]*>", std::regex::icase);
	static const std::regex nameRegex("name=\"([^\"]*)\"", std::regex::icase);
	static const std::regex valueRegex("value=\"([^\"]*)\"", std::regex::icase);

	auto inputsBegin = std::sregex_iterator(html.begin(), html.end(), inputRegex);
	auto inputsEnd = std::sregex_iterator();
	for (auto it = inputsBegin; it != inputsEnd; ++it)
	{
		std::string tag = it->str();
		std::smatch nameMatch;
		if (!std::regex_search(tag, nameMatch, nameRegex))
			continue;
		std::string value;
		std::smatch valueMatch;
		if (std::regex_search(tag, valueMatch, valueRegex))
			value = valueMatch[1].str();
		fields[nameMatch[1].str()] = value;
	}

	return !formAction.empty();
}

bool SolarEdgeAPI::WebExchangeSession(const std::string& tokenJsonBody)
{
	std::vector<std::string> ExtraHeaders;
	ExtraHeaders.push_back("Authorization: Bearer " + m_WebAccessToken);
	ExtraHeaders.push_back("Content-Type: application/json");

	std::string sResult;
	std::vector<std::string> vHeaderData;
	HTTPClient::POST(SE_WEB_EXCHANGE_URL, tokenJsonBody, ExtraHeaders, sResult, vHeaderData, true, true);

	int iStatusCode = LastHttpStatusCode(vHeaderData);
	Debug(DEBUG_HARDWARE, "Web portal: Session exchange status %d", iStatusCode);
	return (iStatusCode >= 200 && iStatusCode < 300);
}

bool SolarEdgeAPI::WebRefreshToken()
{
	if (m_WebRefreshToken.empty())
		return false;

	std::string httpData = "grant_type=refresh_token";
	httpData += "&client_id=" SE_WEB_CLIENT_ID;
	httpData += "&refresh_token=" + CURLEncode::URLEncode(m_WebRefreshToken);

	std::vector<std::string> ExtraHeaders;
	ExtraHeaders.push_back("Content-Type: application/x-www-form-urlencoded");

	std::string sResult;
	std::vector<std::string> vHeaderData;
	HTTPClient::POST(SE_WEB_TOKEN_URL, httpData, ExtraHeaders, sResult, vHeaderData);

	int iStatusCode = LastHttpStatusCode(vHeaderData);
	Debug(DEBUG_HARDWARE, "Web portal: Token refresh status %d", iStatusCode);

	Json::Value root;
	// The refresh response only carries a new access_token, the existing refresh token stays valid
	if (iStatusCode != 200 || !ParseJSon(sResult, root) || !root.isObject() || root["access_token"].empty() || root["expires_in"].empty())
	{
		Log(LOG_ERROR, "Web portal: Failed to refresh access token, a new login will be attempted");
		m_WebAccessToken.clear();
		return false;
	}

	m_WebAccessToken = root["access_token"].asString();
	if (!root["refresh_token"].empty())
		m_WebRefreshToken = root["refresh_token"].asString();
	int expiresIn = root["expires_in"].asInt();
	int refreshIn = (expiresIn * 2) / 3;
	if (refreshIn < 30)
		refreshIn = 30; // never schedule the next refresh in the past
	m_WebNextRefreshTs = mytime(nullptr) + refreshIn;
	StoreWebRefreshToken();

	// The session exchange rejects a body without a refresh token, and the refresh response omits it
	root["refresh_token"] = m_WebRefreshToken;

	if (!WebExchangeSession(JSonToRawString(root)))
	{
		Log(LOG_ERROR, "Web portal: Session exchange failed after token refresh!");
		return false;
	}

	Debug(DEBUG_HARDWARE, "Web portal: Token refresh succeeded");
	return true;
}

bool SolarEdgeAPI::WebLogin()
{
	if (m_WebUsername.empty() || m_WebPassword.empty())
	{
		Log(LOG_ERROR, "Web portal: No web username/password configured!");
		return false;
	}

	unsigned char verifierBytes[32];
	if (RAND_bytes(verifierBytes, sizeof(verifierBytes)) != 1)
	{
		Log(LOG_ERROR, "Web portal: Failed to generate PKCE code verifier!");
		return false;
	}
	std::string codeVerifier = base64url_encode_buf(verifierBytes, sizeof(verifierBytes));
	std::string codeChallenge = base64url_encode(sha256raw(codeVerifier));

	std::stringstream sAuthorizeURL;
	sAuthorizeURL << SE_WEB_AUTHORIZE_URL
		<< "?client_id=" SE_WEB_CLIENT_ID
		<< "&response_type=code"
		<< "&redirect_uri=" << CURLEncode::URLEncode(SE_WEB_REDIRECT_URI)
		<< "&code_challenge=" << CURLEncode::URLEncode(codeChallenge)
		<< "&code_challenge_method=S256";

	std::vector<std::string> authorizeHeaders;
	authorizeHeaders.push_back("Accept: text/html");

	std::string sFormResult;
	std::vector<std::string> vAuthorizeHeaderData;
	// Rely on the process-wide cookie jar so the login session carries through to the form POST below
	HTTPClient::GET(sAuthorizeURL.str(), authorizeHeaders, sFormResult, vAuthorizeHeaderData);

	int iAuthorizeStatus = LastHttpStatusCode(vAuthorizeHeaderData);
	Debug(DEBUG_HARDWARE, "Web portal: Authorize status %d", iAuthorizeStatus);
	if (iAuthorizeStatus != 200 || sFormResult.empty())
	{
		Log(LOG_ERROR, "Web portal: Error requesting login form!");
		return false;
	}

	// With a valid SSO session the authorize call redirects straight to the callback and there
	// is no login form to submit, so take the code from the redirect chain when it is already there
	std::string authCode = ExtractLocationCode(vAuthorizeHeaderData);
	if (!authCode.empty())
		Debug(DEBUG_HARDWARE, "Web portal: Reused existing session, authorization code received");

	if (authCode.empty())
	{
		std::string formAction;
		std::map<std::string, std::string> formFields;
		if (!ParseLoginForm(sFormResult, formAction, formFields))
		{
			Log(LOG_ERROR, "Web portal: Could not find login form in authorize response!");
			return false;
		}

		if (formAction.compare(0, 4, "http") != 0)
		{
			if (formAction.empty() || formAction[0] != '/')
				formAction = "/" + formAction;
			formAction = "https://login.solaredge.com" + formAction;
		}

		static const std::regex hostRegex("^https?://([^/:]+)", std::regex::icase);
		std::smatch hostMatch;
		std::string host;
		if (std::regex_search(formAction, hostMatch, hostRegex))
			host = hostMatch[1].str();
		std::transform(host.begin(), host.end(), host.begin(), ::tolower);
		bool bHostOk = (host == "solaredge.com") || (host.size() > 14 && host.compare(host.size() - 14, 14, ".solaredge.com") == 0);
		if (!bHostOk)
		{
			Log(LOG_ERROR, "Web portal: Login form action does not point to a solaredge.com host, aborting for safety!");
			return false;
		}

		formFields["username"] = m_WebUsername;
		formFields["password"] = m_WebPassword;

		std::string postData;
		for (const auto& field : formFields)
		{
			if (!postData.empty())
				postData += "&";
			postData += CURLEncode::URLEncode(field.first) + "=" + CURLEncode::URLEncode(field.second);
		}

		std::vector<std::string> loginHeaders;
		loginHeaders.push_back("Content-Type: application/x-www-form-urlencoded");

		std::string sLoginResult;
		std::vector<std::string> vLoginHeaderData;
		HTTPClient::POST(formAction, postData, loginHeaders, sLoginResult, vLoginHeaderData, true, true);

		authCode = ExtractLocationCode(vLoginHeaderData);
		Debug(DEBUG_HARDWARE, "Web portal: Login form submitted, authorization code %s", authCode.empty() ? "not found" : "received");
		if (authCode.empty())
		{
			Log(LOG_ERROR, "Web portal: Could not extract authorization code, check web username/password!");
			return false;
		}
	}

	std::string tokenData = "grant_type=authorization_code";
	tokenData += "&client_id=" SE_WEB_CLIENT_ID;
	tokenData += "&redirect_uri=" + CURLEncode::URLEncode(SE_WEB_REDIRECT_URI);
	tokenData += "&code=" + CURLEncode::URLEncode(authCode);
	tokenData += "&code_verifier=" + CURLEncode::URLEncode(codeVerifier);

	std::vector<std::string> tokenHeaders;
	tokenHeaders.push_back("Content-Type: application/x-www-form-urlencoded");

	std::string sTokenResult;
	std::vector<std::string> vTokenHeaderData;
	HTTPClient::POST(SE_WEB_TOKEN_URL, tokenData, tokenHeaders, sTokenResult, vTokenHeaderData);

	int iTokenStatus = LastHttpStatusCode(vTokenHeaderData);
	Debug(DEBUG_HARDWARE, "Web portal: Token exchange status %d", iTokenStatus);

	Json::Value tokenRoot;
	if (iTokenStatus != 200 || !ParseJSon(sTokenResult, tokenRoot) || !tokenRoot.isObject()
		|| tokenRoot["access_token"].empty() || tokenRoot["refresh_token"].empty() || tokenRoot["expires_in"].empty())
	{
		Log(LOG_ERROR, "Web portal: Failed to obtain an access token!");
		return false;
	}

	m_WebAccessToken = tokenRoot["access_token"].asString();
	m_WebRefreshToken = tokenRoot["refresh_token"].asString();
	int expiresIn = tokenRoot["expires_in"].asInt();
	int refreshIn = (expiresIn * 2) / 3;
	if (refreshIn < 30)
		refreshIn = 30; // never schedule the next refresh in the past
	m_WebNextRefreshTs = mytime(nullptr) + refreshIn;
	StoreWebRefreshToken();

	if (!WebExchangeSession(sTokenResult))
	{
		Log(LOG_ERROR, "Web portal: Session exchange failed after login!");
		return false;
	}

	Log(LOG_STATUS, "Web portal: Login succeeded");
	return true;
}

bool SolarEdgeAPI::WebEnsureLoggedIn()
{
	if (!m_WebAccessToken.empty() && (mytime(nullptr) - 15) < m_WebNextRefreshTs)
		return true;

	if (!m_WebRefreshToken.empty())
	{
		if (WebRefreshToken())
			return true;
		Log(LOG_ERROR, "Web portal: Refresh token failed, a full login will be attempted");
	}

	return WebLogin();
}

bool SolarEdgeAPI::GetLayoutFromAPI(Json::Value& json_output)
{
	std::string sResult;

#ifdef DEBUG_SolarEdgeAPIR
	sResult = ReadFile("E:\\SolarEdge_web_layout.json");
#else
	if (m_WebSiteID.empty())
	{
		Log(LOG_ERROR, "Web portal: No Site ID available! Configure Site ID or enable API polling.");
		return false;
	}

	std::vector<std::string> ExtraHeaders;
	ExtraHeaders.push_back("Authorization: Bearer " + m_WebAccessToken);
	ExtraHeaders.push_back("Accept: application/json");

	std::stringstream sURL;
	sURL << "https://monitoring.solaredge.com/services/layout/logical/generic/v2/site/" << m_WebSiteID << "?include-optimizers=true";

	std::vector<std::string> vHeaderData;
	bool bOK = HTTPClient::GET(sURL.str(), ExtraHeaders, sResult, vHeaderData);
	int iStatusCode = LastHttpStatusCode(vHeaderData);
	Debug(DEBUG_HARDWARE, "Web portal: Layout URL %s status %d", sURL.str().c_str(), iStatusCode);
	if (!bOK)
	{
		std::string sStatusLine = !vHeaderData.empty() ? vHeaderData[0] : "no response";
		Log(LOG_ERROR, "Web portal: Error getting site layout! (%s)", sStatusLine.c_str());
		return false;
	}
#ifdef DEBUG_SolarEdgeAPIW
	SaveString2Disk(sResult, "E:\\SolarEdge_web_layout.json");
#endif
#endif

	if (!ParseJSon(sResult, json_output) || !json_output.isObject())
	{
		Log(LOG_ERROR, "Web portal: Invalid JSON in site layout response!");
		return false;
	}
	if (json_output["siteStructure"].empty())
	{
		Log(LOG_ERROR, "Web portal: No siteStructure in site layout response!");
		return false;
	}

	return true;
}

void SolarEdgeAPI::WalkLayoutNode(const Json::Value& node, const std::string& inverterName, int inverterNodeId, int stringNodeId, int& inverterIndex, int& stringIndex, int& optimizerNodeBase)
{
	if (!node.isObject())
		return;
	if (IsInactiveNode(node))
		return;

	std::string type = node.get("type", "").asString();
	std::string name = node.get("name", "").asString();
	std::string identity = ExtractNodeIdentity(node);

	std::string curInverterName = inverterName;
	int curInverterNodeId = inverterNodeId;
	int curStringNodeId = stringNodeId;

	if (type == "INVERTER")
	{
		_tWebNodeInfo info;
		info.reporterId = identity;
		info.displayName = name;
		info.nodeId = SE_WEB_INVERTER_BASE + inverterIndex++;
		m_webInverters.push_back(info);
		curInverterName = name;
		curInverterNodeId = info.nodeId;
	}
	else if (type == "STRING")
	{
		_tWebNodeInfo info;
		info.reporterId = identity;
		// The v2 layout already names strings "String x.y", so only add the prefix when it is missing
		info.displayName = (name.find("String") == 0) ? name : "String " + name;
		info.nodeId = SE_WEB_STRING_BASE + stringIndex++;
		m_webStrings.push_back(info);
		curStringNodeId = info.nodeId;
	}
	else if (type == "OPTIMIZER")
	{
		_tOptimizerInfo info;
		info.reporterId = identity;
		info.serialNumber = node.get("serial", "").asString();
		info.displayName = name;
		info.inverterName = curInverterName;
		info.stringNodeId = curStringNodeId;
		info.inverterNodeId = curInverterNodeId;
		info.nodeId = optimizerNodeBase++;
		m_optimizers.push_back(info);
		return; // optimizers are leaves
	}

	const Json::Value& children = node["children"];
	if (!children.isArray())
		return;
	for (const auto& child : children)
		WalkLayoutNode(child, curInverterName, curInverterNodeId, curStringNodeId, inverterIndex, stringIndex, optimizerNodeBase);
}

bool SolarEdgeAPI::GetSiteLayout()
{
	if (!WebEnsureLoggedIn())
		return false;

	Json::Value root;
	if (!GetLayoutFromAPI(root))
		return false;

	m_optimizers.clear();
	m_webInverters.clear();
	m_webStrings.clear();

	int inverterIndex = 0;
	int stringIndex = 0;
	int optimizerNodeBase = 300;

	const Json::Value& siteNode = root["siteStructure"];
	WalkLayoutNode(siteNode, "", -1, -1, inverterIndex, stringIndex, optimizerNodeBase);

	Log(LOG_STATUS, "Web portal: Discovered %d inverters, %d strings, %d optimizers",
		(int)m_webInverters.size(), (int)m_webStrings.size(), (int)m_optimizers.size());

	return true;
}

void SolarEdgeAPI::GetOptimizerData()
{
	if (m_optimizers.empty())
		return;

	// Check daylight window
	if (!isDaylightWindow())
		return;

	if (!WebEnsureLoggedIn())
		return;

	time_t atime = mytime(nullptr);
	struct tm ltime;
	localtime_r(&atime, &ltime);

	char szStart[40];
	char szEnd[40];
	snprintf(szStart, sizeof(szStart), "%04d-%02d-%02dT00:00:00Z", ltime.tm_year + 1900, ltime.tm_mon + 1, ltime.tm_mday);
	snprintf(szEnd, sizeof(szEnd), "%04d-%02d-%02dT%02d:%02d:%02dZ", ltime.tm_year + 1900, ltime.tm_mon + 1, ltime.tm_mday, ltime.tm_hour, ltime.tm_min, ltime.tm_sec);

	std::string startDate = CURLEncode::URLEncode(szStart);
	std::string endDate = CURLEncode::URLEncode(szEnd);

	std::stringstream sURL;
	sURL << "https://monitoring.solaredge.com/services/layout/playback/site/" << m_WebSiteID << "/optimizers-compact"
		<< "?resolution=hours&start-date=" << startDate << "&end-date=" << endDate;

	std::vector<std::string> ExtraHeaders;
	ExtraHeaders.push_back("Authorization: Bearer " + m_WebAccessToken);
	ExtraHeaders.push_back("Accept: application/json");
	std::string csrfToken = GetCookieValue("CSRF-TOKEN");
	if (!csrfToken.empty())
		ExtraHeaders.push_back("X-CSRF-TOKEN: " + csrfToken);

	std::string sResult;
	std::vector<std::string> vHeaderData;
	bool bOK = HTTPClient::GET(sURL.str(), ExtraHeaders, sResult, vHeaderData);
	int iStatusCode = LastHttpStatusCode(vHeaderData);
	Debug(DEBUG_HARDWARE, "Web portal: Playback URL %s status %d", sURL.str().c_str(), iStatusCode);
	if (!bOK)
	{
		Log(LOG_ERROR, "Web portal: Error getting optimizer playback data! (status %d)", iStatusCode);
		return;
	}

	Json::Value root;
	if (!ParseJSon(sResult, root) || !root.isObject())
	{
		Log(LOG_ERROR, "Web portal: Invalid JSON in optimizer playback response!");
		return;
	}

	const Json::Value& serials = root["optimizerSerials"];
	const Json::Value& compressPowerData = root["compressPowerData"];
	if (!serials.isArray() || !compressPowerData.isArray())
	{
		Log(LOG_ERROR, "Web portal: Missing optimizerSerials/compressPowerData in playback response!");
		return;
	}

	int timeSlotsCount = root["timeSlotsCount"].asInt();
	size_t serialCount = serials.size();
	size_t headerLen = 2 + (2 * serialCount);
	if (compressPowerData.size() <= headerLen || timeSlotsCount <= 0)
	{
		Debug(DEBUG_HARDWARE, "Web portal: Playback response carries no measurements (serials %d, slots %d)", (int)serialCount, timeSlotsCount);
		return;
	}

	int dataStartIdx = compressPowerData[1].asInt();
	if (dataStartIdx < 0 || (size_t)dataStartIdx >= compressPowerData.size())
	{
		Log(LOG_ERROR, "Web portal: Invalid playback data start index %d!", dataStartIdx);
		return;
	}

	std::map<std::string, float> powerByShortSerial;
	for (size_t i = 0; i < serialCount; i++)
	{
		std::string shortSerial = serials[(Json::ArrayIndex)i].asString();
		int offset = compressPowerData[(Json::ArrayIndex)(3 + (i * 2))].asInt();
		if (offset < 0)
			continue;

		float value = 0;
		for (int s = timeSlotsCount - 1; s >= 0; s--)
		{
			size_t idx = (size_t)dataStartIdx + (size_t)offset + (size_t)s;
			if (idx >= compressPowerData.size())
				continue;
			value = compressPowerData[(Json::ArrayIndex)idx].asFloat();
			if (value != 0)
				break;
		}
		powerByShortSerial[shortSerial] = value;
	}

	Debug(DEBUG_HARDWARE, "Web portal: Playback decoded %d optimizer serials, %d slots", (int)serialCount, timeSlotsCount);

	std::map<int, double> stringPower;
	std::map<int, double> inverterPower;
	char szTmp[200];

	for (const auto& opt : m_optimizers)
	{
		std::string shortSerial = opt.serialNumber.substr(0, opt.serialNumber.find('-'));
		auto it = powerByShortSerial.find(shortSerial);
		if (it == powerByShortSerial.end())
			continue;

		float power = it->second;
		snprintf(szTmp, sizeof(szTmp), "%s Power", opt.displayName.c_str());
		SendWattMeter(opt.nodeId, SE_OPT_POWER, 255, power, szTmp);

		if (opt.stringNodeId >= 0)
			stringPower[opt.stringNodeId] += power;
		if (opt.inverterNodeId >= 0)
			inverterPower[opt.inverterNodeId] += power;
	}

	for (const auto& str : m_webStrings)
	{
		auto it = stringPower.find(str.nodeId);
		if (it == stringPower.end())
			continue;
		snprintf(szTmp, sizeof(szTmp), "%s Power", str.displayName.c_str());
		SendWattMeter(str.nodeId, SE_WEB_POWER, 255, (float)it->second, szTmp);
	}

	for (const auto& inv : m_webInverters)
	{
		auto it = inverterPower.find(inv.nodeId);
		if (it == inverterPower.end())
			continue;
		snprintf(szTmp, sizeof(szTmp), "%s Power", inv.displayName.c_str());
		SendWattMeter(inv.nodeId, SE_WEB_POWER, 255, (float)it->second, szTmp);
	}
}
