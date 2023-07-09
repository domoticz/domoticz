#include "stdafx.h"
#include "EnphaseAPI.h"
#include "../main/Helper.h"
#include "../main/Logger.h"
#include "../httpclient/HTTPClient.h"
#include "../httpclient/UrlEncode.h"
#include "../main/json_helper.h"
#include "../main/mainworker.h"
#include "../main/RFXtrx.h"
#include "../main/SQLHelper.h"
#include "../notifications/NotificationHelper.h"
#include "../tinyxpath/tinyxml.h"
#include "hardwaretypes.h"
#include <iostream>

//https://enphase.com/en-us/products-and-services/envoy

#ifdef _DEBUG
//#define DEBUG_EnphaseAPI_R
//#define DEBUG_EnphaseAPI_W
#endif

/*
Example
{"production":[{"type":"inverters","activeCount":9,"readingTime":1568991780,"wNow":712,"whLifetime":1448651},{"type":"eim","activeCount":1,"measurementType":"production","readingTime":1568991966,"wNow":624.315,"whLifetime":1455843.527,"varhLeadLifetime":0.001,"varhLagLifetime":311039.158,"vahLifetime":1619431.681,"rmsCurrent":2.803,"rmsVoltage":233.289,"reactPwr":137.092,"apprntPwr":654.245,"pwrFactor":0.95,"whToday":4295.527,"whLastSevenDays":74561.527,"vahToday":5854.681,"varhLeadToday":0.001,"varhLagToday":2350.158}],"consumption":[{"type":"eim","activeCount":1,"measurementType":"total-consumption","readingTime":1568991966,"wNow":1260.785,"whLifetime":2743860.336,"varhLeadLifetime":132372.858,"varhLagLifetime":273043.125,"vahLifetime":3033001.948,"rmsCurrent":5.995,"rmsVoltage":233.464,"reactPwr":437.269,"apprntPwr":1399.886,"pwrFactor":0.9,"whToday":11109.336,"whLastSevenDays":129007.336,"vahToday":13323.948,"varhLeadToday":895.858,"varhLagToday":3700.125},{"type":"eim","activeCount":1,"measurementType":"net-consumption","readingTime":1568991966,"wNow":636.47,"whLifetime":0.0,"varhLeadLifetime":132372.857,"varhLagLifetime":-37996.033,"vahLifetime":3033001.948,"rmsCurrent":3.191,"rmsVoltage":233.376,"reactPwr":574.361,"apprntPwr":744.807,"pwrFactor":0.85,"whToday":0,"whLastSevenDays":0,"vahToday":0,"varhLeadToday":0,"varhLagToday":0}],"storage":[{"type":"acb","activeCount":0,"readingTime":0,"wNow":0,"whNow":0,"state":"idle"}]}
*/

#ifdef DEBUG_EnphaseAPI_W
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

#ifdef DEBUG_EnphaseAPI_R
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

EnphaseAPI::EnphaseAPI(const int ID, const std::string& IPAddress, const unsigned short usIPPort, int PollInterval, const bool bPollInverters, const std::string& szUsername, const std::string& szPassword) :
	m_szIPAddress(IPAddress),
	m_szUsername(szUsername),
	m_szPassword(CURLEncode::URLEncode(szPassword))
{
	m_bGetInverterDetails = bPollInverters;

	m_HwdID = ID;

	if (PollInterval < 5)
		PollInterval = 30;
	if (PollInterval > 120)
		PollInterval = 120;
	m_poll_interval = PollInterval;

	std::vector<std::vector<std::string> > result;
	std::string szName = "EnphaseToken_" + std::to_string(m_HwdID);
	result = m_sql.safe_query("SELECT ID, Value FROM UserVariables WHERE (Name=='%q')", szName.c_str());
	if (result.empty())
	{
		m_sql.safe_query("INSERT INTO UserVariables (Name, ValueType, Value) VALUES ('%q',%d,'%q')", szName.c_str(), USERVARTYPE_STRING, "");
		result = m_sql.safe_query("SELECT ID, Value FROM UserVariables WHERE (Name=='%q')", szName.c_str());
		if (result.empty())
			return;
	}
	if (!result.empty())
	{
		m_szToken = result[0][1];
	}
}

bool EnphaseAPI::StartHardware()
{
	RequestStart();

	//Start worker thread
	m_thread = std::make_shared<std::thread>([this] { Do_Work(); });
	SetThreadNameInt(m_thread->native_handle());
	m_bIsStarted = true;
	sOnConnected(this);
	return (m_thread != nullptr);
}

bool EnphaseAPI::StopHardware()
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

void EnphaseAPI::Do_Work()
{
	Log(LOG_STATUS, "Worker started...");
	int sec_counter = m_poll_interval - 4;

	bool bHaveRunOnce = false;

	while (!IsStopRequested(1000))
	{
		sec_counter++;

		if (sec_counter % 12 == 0) {
			m_LastHeartbeat = mytime(nullptr);
		}

		if (sec_counter % m_poll_interval == 0)
		{
			bool bInsideSunHours = IsItSunny();
			if ((bHaveRunOnce) && (!bInsideSunHours))
			{
				if (
					(!m_bHaveConsumption)
					&& (!m_bHaveeNetConsumption)
					&& (!m_bHaveStorage)
					)
				{
					//no need to poll outside sun hours
					m_sql.safe_query(
						"UPDATE DeviceStatus SET LastUpdate='%s' WHERE (HardwareID==%d)", TimeToString(nullptr, TF_DateTime).c_str(), m_HwdID);
					continue;
				}
			}

			try
			{
				if (m_szSoftwareVersion.empty())
				{
					if (!GetSerialSoftwareVersion())
						continue;
				}

				Json::Value result;
				if (getProductionDetails(result))
				{
					parseProduction(result);
					parseConsumption(result);
					parseStorage(result);
				}
				if (m_bGetInverterDetails)
				{
					getInverterDetails();
				}
				bHaveRunOnce = true;
			}
			catch (const std::exception& e)
			{
				Log(LOG_ERROR, "Exception: %s", e.what());
			}
		}
	}
	Log(LOG_STATUS, "Worker stopped...");
}

bool EnphaseAPI::WriteToHardware(const char* /*pdata*/, const unsigned char /*length*/)
{
	return false;
}

bool EnphaseAPI::IsItSunny()
{
	time_t atime = mytime(nullptr);
	struct tm ltime;
	localtime_r(&atime, &ltime);

	int ActHourMin = (ltime.tm_hour * 60) + ltime.tm_min;

	int sunRise = getSunRiseSunSetMinutes(true);
	int sunSet = getSunRiseSunSetMinutes(false);

	if (sunRise != 0 && sunSet != 0)
	{
		if (
			(ActHourMin + 60 < sunRise)
			|| (ActHourMin - 60 > sunSet)
			)
		{
			return false;
		}
	}
	return true;
}

int EnphaseAPI::getSunRiseSunSetMinutes(const bool bGetSunRise)
{
	std::vector<std::string> strarray;
	std::vector<std::string> sunRisearray;
	std::vector<std::string> sunSetarray;

	if (!m_mainworker.m_LastSunriseSet.empty())
	{
		StringSplit(m_mainworker.m_LastSunriseSet, ";", strarray);
		if (strarray.size() < 2)
			return false;
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

// emupwGetMobilePasswd taken from https://github.com/sarnau/EnphaseEnergy
std::string EnphaseAPI::V5_emupwGetMobilePasswd(const std::string& serialNumber, const std::string& userName, const std::string& realm)
{
	std::string digest = GenerateMD5Hash(std::string("[e]") + userName + "@" + realm + "#" + serialNumber + " EnPhAsE eNeRgY ");
	if (digest.length() <= 8)
		return "";

	int countZero = (int)std::count(digest.begin(), digest.end(), '0');
	int countOne = (int)std::count(digest.begin(), digest.end(), '1');
	std::string szPassword;
	std::string szRight = digest.substr(digest.length() - 8);
	for (auto it = szRight.rbegin(); it != szRight.rend(); it++)
	{
		if (countZero == 3 || countZero == 6 || countZero == 9)
			countZero--;
		if (countZero > 20)
			countZero = 20;
		if (countZero < 0)
			countZero = 0;

		if (countOne == 9 || countOne == 15)
			countOne--;
		if (countOne > 26)
			countOne = 26;
		if (countOne < 0)
			countOne = 0;
		if (*it == '0')
		{
			szPassword += 'f' + countZero;
			countZero = countZero - 1;
		}
		else if (*it == '1')
		{
			szPassword += '@' + countOne;
			countOne = countOne - 1;
		}
		else
			szPassword += *it;
	}
	return szPassword;
}

bool EnphaseAPI::GetSerialSoftwareVersion()
{
	std::string sResult;

#ifdef DEBUG_EnphaseAPI_R
	sResult = ReadFile("E:\\EnphaseAPI_info.xml");
#else
	std::stringstream sURL;
	sURL << "http://" << m_szIPAddress << "/info.xml";

	if (!HTTPClient::GET(sURL.str(), sResult))
	{
		Log(LOG_ERROR, "Error getting http data! (info)");
		return false;
	}
#ifdef DEBUG_EnphaseAPI_W
	SaveString2Disk(sResult, "E:\\EnphaseAPI_info.xml");
#endif
#endif

	Debug(DEBUG_RECEIVED, "info: %s", sResult.c_str());

	TiXmlDocument doc;
	if (doc.Parse(sResult.c_str(), nullptr, TIXML_ENCODING_UTF8) && doc.Error())
	{
		Log(LOG_ERROR, "Invalid data received!");
		return false;
	}

	TiXmlElement* pRoot, * pDevice;
	TiXmlElement* pElem;

	pRoot = doc.FirstChildElement("envoy_info");
	if (!pRoot)
	{
		Log(LOG_ERROR, "Invalid data received! (no xml/envoy_info)");
		return false;
	}

	pDevice = pRoot->FirstChildElement("device");
	if (!pRoot)
	{
		Log(LOG_ERROR, "Invalid data received! (no xml/device)");
		return false;
	}

	pElem = pDevice->FirstChildElement("sn");
	if (pElem == nullptr)
	{
		Log(LOG_ERROR, "Cannot find serial number");
		return false;
	}
	m_szSerial = pElem->GetText();

	pElem = pDevice->FirstChildElement("software");
	if (pElem == nullptr)
	{
		Log(LOG_ERROR, "Cannot find software version");
		return false;
	}
	m_szSoftwareVersion = pElem->GetText();

	Log(LOG_STATUS, "Connected, serial: %s, software: %s", m_szSerial.c_str(), m_szSoftwareVersion.c_str());

	if (
		(m_szSoftwareVersion.size() < 2)
		|| (m_szSoftwareVersion[0] != 'D')
		)
	{
		Log(LOG_STATUS, "Unsupported software version! Please contact us for support!");
	}
	else
	{
		if (m_szSoftwareVersion[1] < '7')
		{
			m_szInstallerPassword = V5_emupwGetMobilePasswd(m_szSerial, "installer", "enphaseenergy.com");
			Log(LOG_STATUS, "Firmware out of date!. Using password generated from SN. Please update your firmware!!");
		}
	}

	return true;
}

bool EnphaseAPI::GetAccessToken()
{
	m_szToken.clear();

	std::string sResult;
#ifdef DEBUG_EnphaseAPI_R
	sResult = ReadFile("E:\\EnphaseAPI_login.json");
#else
	if (
		(m_szUsername.empty())
		|| (m_szPassword.empty())
		)
	{
		Log(LOG_ERROR, "You need to supply a username/password!");
		return false;
	}

	std::string sURL = "https://enlighten.enphaseenergy.com/login/login.json";

	std::stringstream sstr;
	sstr
		<< "user[email]=" << m_szUsername
		<< "&user[password]=" << m_szPassword;
	std::string szPostdata = sstr.str();

	std::vector<std::string> ExtraHeaders;

	if (!HTTPClient::POST(sURL, szPostdata, ExtraHeaders, sResult))
	{
		Log(LOG_ERROR, "Error getting http data! (login)");
		return false;
	}
#ifdef DEBUG_EnphaseAPI_W
	SaveString2Disk(sResult, "E:\\EnphaseAPI_login.json");
#endif
#endif

	Debug(DEBUG_RECEIVED, "login: %s", sResult.c_str());

	Json::Value root;
	bool ret = ParseJSon(sResult, root);
	if ((!ret) || (!root.isObject()))
	{
		Log(LOG_ERROR, "Invalid data received! (login/json)");
		return false;
	}

	if (root["session_id"].empty())
	{
		Log(LOG_ERROR, "Invalid data received! (no session_id)");
		return false;
	}

	std::string session_id = root["session_id"].asString();

#ifdef DEBUG_EnphaseAPI_R
	sResult = ReadFile("E:\\EnphaseAPI_token.json");
#else
	//Now get the Token
	Json::Value reqRoot;
	reqRoot["session_id"] = session_id;
	reqRoot["serial_num"] = m_szSerial;
	reqRoot["username"] = m_szUsername;

	szPostdata = JSonToRawString(reqRoot);

	ExtraHeaders.push_back("Content-type: application/json");

	sURL = "https://entrez.enphaseenergy.com/tokens";

	if (!HTTPClient::POST(sURL, szPostdata, ExtraHeaders, sResult))
	{
		Log(LOG_ERROR, "Error getting http data! (get token)");
		return false;
	}
#ifdef DEBUG_EnphaseAPI_W
	SaveString2Disk(sResult, "E:\\EnphaseAPI_token.json");
#endif
#endif
	if (sResult.find("Failed") != std::string::npos)
	{
		Log(LOG_ERROR, "Error getting http data! (token returned)");
		return false;
	}

	m_szToken = sResult;

#ifdef DEBUG_EnphaseAPI_R
	sResult = ReadFile("E:\\EnphaseAPI_check_jwt.json");
#else
	//Validate token on IQ Gateway
	ExtraHeaders.push_back("Authorization: Bearer " + m_szToken);

	sURL = "http://" + m_szIPAddress + "/auth/check_jwt";

	if (!HTTPClient::GET(sURL, ExtraHeaders, sResult))
	{
		Log(LOG_ERROR, "Error getting http data! (check_jwt)");
		return false;
	}
#ifdef DEBUG_EnphaseAPI_W
	SaveString2Disk(sResult, "E:\\EnphaseAPI_check_jwt.json");
#endif
#endif

	if (sResult.find("Valid token") == std::string::npos)
	{
		Log(LOG_ERROR, "Error getting http data! (invalid token!)");
		return false;
	}

	//Store token for later usage
	std::string szName = "EnphaseToken_" + std::to_string(m_HwdID);
	m_sql.safe_query("UPDATE UserVariables SET Value='%q' WHERE (Name=='%q')", m_szToken.c_str(), szName.c_str());
	return true;
}

bool EnphaseAPI::NeedToken()
{
	if (m_szSoftwareVersion.empty())
		return false;
	int iMainVersion = m_szSoftwareVersion[1] - 0x30;
	return (iMainVersion >= 7);
}

bool EnphaseAPI::getProductionDetails(Json::Value& result)
{
	if (NeedToken())
	{
		if (m_szToken.empty())
		{
			//We need a access token
			if (!GetAccessToken())
				return false;
			if (m_szToken.empty())
				return false;
		}
	}

	std::string sResult;

#ifdef DEBUG_EnphaseAPI_R
	sResult = ReadFile("E:\\EnphaseAPI_production.json");
#else
	std::stringstream sURL;
	sURL << "http://" << m_szIPAddress << "/production.json?details=1";

	std::vector<std::string> ExtraHeaders;
	if (!m_szToken.empty()) {
		ExtraHeaders.push_back("Authorization: Bearer " + m_szToken);
		ExtraHeaders.push_back("Content-Type:application/json");
	}

	if (!HTTPClient::GET(sURL.str(), ExtraHeaders, sResult))
	{
		if (!m_szToken.empty())
		{
			//We might need a new access token
			if (!GetAccessToken())
				return false;
			if (m_szToken.empty())
				return false;

			if (!HTTPClient::GET(sURL.str(), ExtraHeaders, sResult))
			{
				if (sResult.find("401") != std::string::npos)
				{
					Log(LOG_ERROR, "Error getting http data (production/Unauthorized!)");
					m_szToken.clear();
				}
				else
					Log(LOG_ERROR, "Error getting http data! (production)");
				return false;
			}
		}
		else
		{
			Log(LOG_ERROR, "Error getting http data! (production)");
			return false;
		}
	}
#ifdef DEBUG_EnphaseAPI_W
	SaveString2Disk(sResult, "E:\\EnphaseAPI_production.json");
#endif
#endif
	Debug(DEBUG_RECEIVED, "production: %s", sResult.c_str());

	bool ret = ParseJSon(sResult, result);
	if ((!ret) || (!result.isObject()))
	{
		m_szToken.clear();
		Log(LOG_ERROR, "Invalid data received! (production/json)");
		return false;
	}
	if (
		(result["consumption"].empty())
		&& (result["production"].empty())
		)
	{
		m_szToken.clear();
		Log(LOG_ERROR, "Invalid (no) data received (production, objects not found)");
		return false;
	}
	return true;
}

void EnphaseAPI::parseProduction(const Json::Value& root)
{
	if (!IsItSunny())
	{
		m_sql.safe_query(
			"UPDATE DeviceStatus SET LastUpdate='%s' WHERE (HardwareID==%d)", TimeToString(nullptr, TF_DateTime).c_str(), m_HwdID);
		return;
	}
	if (root["production"].empty() == true)
	{
		//No production details available
		return;
	}
	size_t sproduction = root["production"].size();
	bool bIsMeteredVersion = (sproduction > 1);
	if (bIsMeteredVersion)
	{
		bIsMeteredVersion = root["production"][1]["whLifetime"].asInt() != 0;
	}
	Json::Value reading = (bIsMeteredVersion) ? root["production"][1] : root["production"][0];

	int musage = reading["wNow"].asInt();
	int mtotal = reading["whLifetime"].asInt();

	if (mtotal != 0)
	{
		SendKwhMeter(m_HwdID, 1, 255, musage, mtotal / 1000.0, "Enphase kWh Production");
	}
}

void EnphaseAPI::parseConsumption(const Json::Value& root)
{
	if (root["consumption"].empty())
	{
		return;
	}

	int iIndex = 2;
	for (const auto& itt : root["consumption"])
	{
		int activeCount = itt["activeCount"].asInt();
		if (activeCount == 0)
			continue;

		m_bHaveConsumption = true;

		std::string szName = "Enphase " + itt["measurementType"].asString();
		int musage = itt["wNow"].asInt();
		int mtotal = itt["whLifetime"].asInt();
		if (mtotal != 0)
		{
			SendKwhMeter(m_HwdID, iIndex++, 255, musage, mtotal / 1000.0, szName);
		}
	}
}

void EnphaseAPI::parseStorage(const Json::Value& root)
{
	if (root["storage"].empty())
	{
		return;
	}

	if (root["storage"][0].empty())
	{
		Log(LOG_ERROR, "Invalid data received (storage)");
		return;
	}

	Json::Value reading = root["storage"][0];

	int activeCount = reading["activeCount"].asInt();
	if (activeCount == 0)
		return;

	int musage = reading["wNow"].asInt();
	SendWattMeter(m_HwdID, 1, 255, static_cast<float>(musage), "Enphase Storage wNow");
	//int whNow = reading["whNow"].asInt();
	//SendWattMeter(m_HwdID, 2, 255, static_cast<float>(musage), "Enphase Storage whNow");

	int percentageFull = reading["percentFull"].asInt();
	SendPercentageSensor(m_HwdID, 1, 255, static_cast<float>(percentageFull), "Enphase Storage Percent Full");

	std::string szState = reading["state"].asString();
	SendTextSensor(m_HwdID, 1, 255, szState, "Enphase Storage State");

	m_bHaveStorage = true;
}

uint64_t EnphaseAPI::UpdateValueInt(const char* ID, unsigned char unit, unsigned char devType, unsigned char subType, unsigned char signallevel, unsigned char batterylevel, int nValue,
	const char* sValue, std::string& devname, bool bUseOnOffAction, const std::string& user)
{
	uint64_t DeviceRowIdx = m_sql.UpdateValue(m_HwdID, ID, unit, devType, subType, signallevel, batterylevel, nValue, sValue, devname, bUseOnOffAction, (!user.empty()) ? user.c_str() : m_Name.c_str());
	if (DeviceRowIdx == (uint64_t)-1)
		return -1;
	if (m_bOutputLog)
	{
		std::string szLogString = RFX_Type_Desc(devType, 1) + std::string("/") + std::string(RFX_Type_SubType_Desc(devType, subType)) + " (" + devname + ")";
		Log(LOG_NORM, szLogString);
	}
	m_mainworker.sOnDeviceReceived(m_HwdID, DeviceRowIdx, devname, nullptr);
	m_notifications.CheckAndHandleNotification(DeviceRowIdx, m_HwdID, ID, devname, unit, devType, subType, nValue, sValue);
	return DeviceRowIdx;
}

bool EnphaseAPI::getInverterDetails()
{
	std::string sResult;
#ifdef DEBUG_EnphaseAPI_R
	sResult = ReadFile("E:\\EnphaseAPI_inverters.json");
#else
	std::stringstream sURL;
	sURL << "http://";

	if (!NeedToken())
	{
		//Firmware version lower than V7
		sURL << "envoy:" << m_szSerial.substr(m_szSerial.size() - 6) << "@";
	}
	sURL << m_szIPAddress << "/api/v1/production/inverters";

	std::vector<std::string> ExtraHeaders;
	if (!m_szToken.empty()) {
		ExtraHeaders.push_back("Authorization: Bearer " + m_szToken);
		ExtraHeaders.push_back("Content-Type:application/json");
	}

	if (!HTTPClient::GET(sURL.str(), ExtraHeaders, sResult))
	{
		if (!NeedToken() && !m_szInstallerPassword.empty())
		{
			std::stringstream sURLInstallerPwd;
			sURLInstallerPwd << "http://installer:" << m_szInstallerPassword << "@" << m_szIPAddress << "/api/v1/production/inverters";
			if (!HTTPClient::GET(sURLInstallerPwd.str(), ExtraHeaders, sResult))
			{
				Log(LOG_ERROR, "Invalid data received! (inverter details)");
				return false;
			}
		}
		else
		{
			Log(LOG_ERROR, "Invalid data received! (inverter details)");
			return false;
		}
	}
#ifdef DEBUG_EnphaseAPI_W
	SaveString2Disk(sResult, "E:\\EnphaseAPI_inverters.json");
#endif
#endif

	Debug(DEBUG_RECEIVED, "inverters: %s", sResult.c_str());

	Json::Value root;
	bool ret = ParseJSon(sResult, root);
	if ((!ret) || (!root.isArray()))
	{
		Log(LOG_ERROR, "Invalid data received! (inverter details/json)");
		return false;
	}

	for (const auto& itt : root)
	{
		if (itt["serialNumber"].empty())
			return false;
		std::string szSerialNumber = itt["serialNumber"].asString();

		int musage = itt["lastReportWatts"].asInt();
		int mtotal = itt["maxReportWatts"].asInt();

		std::string szDeviceID = szSerialNumber;
		std::string sDeviceName = "Inv " + szSerialNumber;
		int nValue = 0;
		std::string sValue = std::to_string(musage) + ";" + std::to_string(mtotal);
		int devType = pTypeGeneral;
		int subType = sTypeKwh;

		std::vector<std::vector<std::string>> result;
		result = m_sql.safe_query("SELECT Name,nValue,sValue FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='%q') AND (Unit == %d) AND (Type==%d) AND (Subtype==%d)",
			m_HwdID, szDeviceID.c_str(), 1, devType, subType);
		if (result.empty())
		{
			// Insert
			int iUsed = 0;
			m_sql.safe_query("INSERT INTO DeviceStatus (HardwareID, DeviceID, Unit, Type, SubType, SignalLevel, BatteryLevel, Name, Used, nValue, sValue) "
				"VALUES (%d, '%q', 1, %d, %d, %d, %d, '%q', %d, %d, '%q')",
				m_HwdID, szDeviceID.c_str(), devType, subType, 12, 255, sDeviceName.c_str(), iUsed, nValue, sValue.c_str());

			result = m_sql.safe_query("SELECT ID FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='%q') AND (Unit == %d) AND (Type==%d) AND (Subtype==%d)",
				m_HwdID, szDeviceID.c_str(), 1, devType, subType);
			if (!result.empty())
			{
				m_sql.SetDeviceOptions(std::stoull(result[0][0]), m_sql.BuildDeviceOptions("EnergyMeterMode:1", false));
			}
		}
		else
		{
			// Update
			UpdateValueInt(szDeviceID.c_str(), 1, devType, subType, 12, 255, nValue, sValue.c_str(), result[0][0]);
		}
	}

	return true;
}
