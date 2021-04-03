//
#include "stdafx.h"
#include "AirconWithMe.h"

#include <regex>

#include "../httpclient/HTTPClient.h"
#include "../main/json_helper.h"
#include "hardwaretypes.h"
#include "../main/SQLHelper.h"
#include "../main/mainworker.h"
#include "../main/Logger.h"

#define AIRCONWITHME_POLL_INTERVAL 30

CAirconWithMe::CAirconWithMe(const int id, const std::string& ipaddress, const unsigned short ipport, const std::string& users, const std::string& password)
	: mIpAddress(ipaddress)
	, mIpPort(ipport)
	, mUsername(users)
	, mPassword(password)
    , mPollInterval(AIRCONWITHME_POLL_INTERVAL)
{
	m_HwdID = id;
    
    _UIDMap =
    {
        {1,   {1,   "Airco On/Off", NDT_SWITCH, true}},
        {2,   {2,   "User mode", NDT_SELECTORSWITCH, true, {{"Auto", 0},{"Heat", 1},{"Dry", 2},{"Fan", 3},{"Cool", 4}}}},
        {4,   {4,   "Fan Speed", NDT_SELECTORSWITCH, true, {{"Speed 1", 1},{"Speed 2", 2},{"Speed 3", 3},{"Speed 4", 4},{"Speed 5", 5},{"Speed 6", 6}}}},
        {5,   {5,   "Vane Up/Down", NDT_SELECTORSWITCH, true, {{"Position 1", 1},{"Position 2", 2},{"Position 3", 3},{"Position 4", 4},{"Position 5", 5},{"Position 6", 6},{"Swing", 10}}}},
        {9,   {9,   "User Setpoint", NDT_THERMOSTAT, true}},
        {10,  {10,  "Return Path Temperature", NDT_THERMOMETER, false}},
        {12,  {12,  "Remote Disable", NDT_SELECTORSWITCH, true, {{"Remote Enabled", 0},{"Remote Disabled", 1}}}},
        {13,  {13,  "On Time (Hour)", NDT_HOUR, false}},
        {14,  {14,  "Alarm Status", NDT_SWITCH, false}},
        {15,  {15,  "Error Code", NDT_NUMBER, false}},
        {37,  {37 , "Outdoor temperature", NDT_THERMOMETER, false}},
    };
    
    _DeviceInfo =
    {
        {0xFF0001, "lastError", "LastError", NDT_NUMBER},
        {0xFF0002, "rssi", "Wifi rssi", NDT_NUMBER},
        {0xFF0003, "ssid", "Wifi ssid", NDT_STRING},
        {0xFF0004, "acStatus", "Communication Error", NDT_SWITCH},
        {0xFF0005, "wlanLNK", "Wifi Connected", NDT_SWITCH},
        {0xFF0006, "powerStatus", "Power Status", NDT_NUMBER},
        {0xFF0007, "wifiTxPower", "Wifi TX power", NDT_NUMBER},
    };
}

bool CAirconWithMe::StartHardware()
{
	RequestStart();

	//Start worker thread
	m_thread = std::make_shared<std::thread>([this] { Do_Work(); });
	SetThreadNameInt(m_thread->native_handle());
	m_bIsStarted = true;
	sOnConnected(this);
	return (m_thread != nullptr);
}


bool CAirconWithMe::StopHardware()
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


void CAirconWithMe::Do_Work()
{
	int countdown = mPollInterval;
	_log.Log(LOG_STATUS, "AirconWithMe: Worker started...");
	GetValues();
	while (!IsStopRequested(1000))
	{
		countdown--;
		if (countdown % 12 == 0) 
		{
			m_LastHeartbeat = mytime(nullptr);
		}
		if (countdown == 0) 
		{
			GetValues();
			countdown = mPollInterval;
		}
	}
	_log.Log(LOG_STATUS, "AirconWithMe: Worker stopped...");
}


bool CAirconWithMe::DoWebRequest(const std::string& postdata, Json::Value& root, std::string& errorMessage, bool loginWhenFailed)
{
	if (loginWhenFailed && mSessionId.size() < 5)
	{
		if (!Login())
			return false;
	}

	std::string sURL = "http://" + mIpAddress + "/api.cgi";
	std::vector<std::string> ExtraHeaders;
	std::string sResult;

	ExtraHeaders.push_back("Content-type: application/json");
	ExtraHeaders.push_back("Accept: application/json");

	std::string postDataWithSessionId = std::regex_replace(postdata, std::regex("%SESSION_ID%"), mSessionId);

	if (!HTTPClient::POST(sURL, postDataWithSessionId, ExtraHeaders, sResult))
	{
		errorMessage = "Error getting data from url \"" + sURL + "\"";
		return false;
	}

	bool ret = ParseJSon(sResult, root);
	if (!ret || root.empty())
	{
		errorMessage = "Invalid data received!";
		return false;
	}

	if (root["success"] == false)
	{
		if (loginWhenFailed)
		{
			Login();
			return DoWebRequest(postdata, root, errorMessage, false);
		}
		errorMessage = "request failed!";
		return false;
	}

	return true;
}


bool CAirconWithMe::Login()
{
	mSessionId = ""; 
	
	Json::Value root;
	std::string errorMessage;
	std::string postdata = R"({"command":"login","data":{"username":")" + mUsername + R"(","password":")" + mPassword + "\"}}";

	if (!DoWebRequest(postdata, root, errorMessage, false))
	{
		Log(LOG_ERROR, "Login failed : " + errorMessage);
		return false;
	}

	if (root["data"]["id"]["sessionID"].empty())
	{
		Log(LOG_ERROR, "No session ID!");
		return false;
	}

	mSessionId = root["data"]["id"]["sessionID"].asString();
	return true;
}


bool CAirconWithMe::GetAvailableDataPoints()
{
	Json::Value root;
	std::string errorMessage;
	std::string postdata = R"({"command":"getavailabledatapoints","data":{"sessionID":"%SESSION_ID%"}})";
	if (!DoWebRequest(postdata, root, errorMessage, true))
	{
		Log(LOG_ERROR, "GetAvailableDataPoints failed : " + errorMessage);
		return false;
	}

	if (!root["data"]["dp"]["datapoints"].isArray())
	{
		Log(LOG_ERROR, "GetValues failed : No Values returned.");
		return false;
	}

	for (const auto& it : root["data"]["dp"]["datapoints"])
	{
		int32_t uid = it["uid"].asInt();
		mDeviceInfo[uid].mUID = uid;

		int32_t type = it["type"].asInt();
		mDeviceInfo[uid].mType = type;
		
		std::string RW = it["rw"].asString();
		if (RW.find('w') != std::string::npos)
			mDeviceInfo[uid].mWritable = true;

		if (type == 1 && it["descr"]["states"].isArray())
		{
			for (const auto& it2 : it["descr"]["states"])
			{
				mDeviceInfo[uid].mAvailableStates.push_back(it2.asInt());
			}
		}

		if (type == 2 && it["descr"]["maxValue"].isInt())
			mDeviceInfo[uid].mMaxValue = it["descr"]["maxValue"].asInt();

		if (type == 2 && it["descr"]["minValue"].isInt())
			mDeviceInfo[uid].mMinValue = it["descr"]["minValue"].asInt();
	}

	ComputerSwitchLevelValues();

	return true;
}


void CAirconWithMe::ComputerSwitchLevelValues()
{
	for (auto& it1 : _UIDMap)
	{
		if (it1.second.mDomoticzType == NDT_SELECTORSWITCH)
		{
			auto it2 = mDeviceInfo.find(it1.second.mUID);
			if (it2 != mDeviceInfo.end())
			{
				UIDDeviceInfo& deviceInfo = it2->second;
				deviceInfo.mSelectorStates = "Off";
				int32_t SelectorSwitchValue = 0;
				for (auto& it3 : it1.second.mAllStates)
				{
					for (const auto& it4 : deviceInfo.mAvailableStates)
					{
						if (it3.mStateId == it4)
						{
							SelectorSwitchValue += 10;
							deviceInfo.mSelectorToState[SelectorSwitchValue] = it4;
							deviceInfo.mStateToSelector[it4] = SelectorSwitchValue;
							deviceInfo.mSelectorStates += "|" + it3.mOptionName;
						}
					}
				}
			}
		}
	}
}


bool CAirconWithMe::GetValues()
{
	if (mDeviceInfo.empty())
	{
		if (false == GetAvailableDataPoints())
			return false;
	}

	Json::Value root;
	std::string errorMessage;
	std::string postdata = R"({"command":"getdatapointvalue","data":{"sessionID":"%SESSION_ID%","uid":"all"}})";

	if (!DoWebRequest(postdata, root, errorMessage, true))
	{
		Log(LOG_ERROR, "GetValues failed : " + errorMessage);
		return false;
	}

	if (!root["data"]["dpval"].isArray())
	{
		Log(LOG_ERROR, "GetValues failed : No Values returned.");
		return false;
	}

	for (const auto& it : root["data"]["dpval"])
	{
		int32_t uid = it["uid"].asInt();
		int32_t value = it["value"].asInt();
		UpdateDomoticzWithValue(uid, value);
	}

	return GetInfo();
}


bool CAirconWithMe::GetInfo()
{
	Json::Value root;
	std::string errorMessage;
	std::string postdata = R"({"command":"getinfo"})";

	if (!DoWebRequest(postdata, root, errorMessage, false))
	{
		Log(LOG_ERROR, "GetValues failed : " + errorMessage);
		return false;
	}

	if (!root["data"]["info"].isObject())
	{
		Log(LOG_ERROR, "GetValues failed : No Values returned.");
		return false;
	}

	const Json::Value& info = root["data"]["info"];
	for (const auto& it : _DeviceInfo)
	{
		if (it.mDomoticzType == NDT_NUMBER)
		{
			if (info[it.mName].isInt())
			{
				int32_t value = info[it.mName].asInt();
				SendCustomSensor(it.mUID / 256, it.mUID, 255, static_cast<float>(value), it.mDescription, "");
			}
		}
		if (it.mDomoticzType == NDT_STRING)
		{
			if (info[it.mName].isString())
			{
				std::string value = info[it.mName].asString();
				SendTextSensor(it.mUID / 256, it.mUID % 256, 255, value, it.mDescription);
			}
		}
		if (it.mDomoticzType == NDT_SWITCH)
		{
			if (info[it.mName].isInt())
			{
				int32_t value = info[it.mName].asInt();
				SendGeneralSwitch(it.mUID, 1, 255, value > 0 ? gswitch_sOn : gswitch_sOff, 1, it.mDescription, m_Name);
			}
		}
	}

	return true;
}

void CAirconWithMe::UpdateDomoticzWithValue(int32_t uid, int32_t value)
{
	if (_UIDMap.find(uid) == _UIDMap.end())
		return;

	const UIDinfo& valueInfo = _UIDMap[uid];
	switch (valueInfo.mDomoticzType)
	{
	case NDT_SWITCH:
		//SendSwitch(uid, 0, 255, value > 0, 1, valueInfo.mDefaultName.c_str(), m_Name);
		SendGeneralSwitch(uid, 1, 255, value > 0 ? gswitch_sOn : gswitch_sOff, 1, valueInfo.mDefaultName, m_Name);
		break;

	case NDT_THERMOSTAT:
		SendSetPointSensor(0, uid / 256, uid % 256, static_cast<float>(value) / 10.0F, valueInfo.mDefaultName);
		break;

	case NDT_SELECTORSWITCH:
		UpdateSelectorSwitch(uid, value, valueInfo);
		break;

	case NDT_THERMOMETER:
		SendTempSensor(uid, 255, static_cast<float>(value) / 10.0F, valueInfo.mDefaultName);
		break;

	case NDT_NUMBER:
		SendCustomSensor(0, uid, 255, static_cast<float>(value), valueInfo.mDefaultName, "");
		break;

	case NDT_HOUR:
		SendCustomSensor(0, uid, 255, static_cast<float>(value), valueInfo.mDefaultName, "Hour");
		break;

	}
}


void CAirconWithMe::UpdateSelectorSwitch(const int32_t uid, const int32_t value, const UIDinfo& valueInfo)
{
	const UIDDeviceInfo& deviceInfo = mDeviceInfo[uid];
	auto selector = deviceInfo.mStateToSelector.find(value);
	if (selector == deviceInfo.mStateToSelector.end())
		return;

	int32_t selectorLevel = selector->second;

	_tGeneralSwitch xcmd;
	xcmd.len = sizeof(_tGeneralSwitch) - 1;
	xcmd.id = uid;
	xcmd.unitcode = 1;
	xcmd.type = pTypeGeneralSwitch;
	xcmd.subtype = sSwitchTypeSelector;
	xcmd.battery_level = 255;
	xcmd.rssi = 12;
	xcmd.cmnd = gswitch_sSetLevel;
	xcmd.level = selectorLevel;

	std::vector<std::vector<std::string> > result;
	result = m_sql.safe_query("SELECT nValue FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='%08X') AND (Unit == '%d')", m_HwdID, xcmd.id, xcmd.unitcode);
	m_mainworker.PushAndWaitRxMessage(this, (const unsigned char*)&xcmd, valueInfo.mDefaultName.c_str(), xcmd.battery_level, m_Name.c_str());
	if (result.empty())
	{
		m_sql.safe_query("UPDATE DeviceStatus SET Name='%q', SwitchType=%d, CustomImage=%i WHERE(HardwareID == %d) AND (DeviceID == '%08X') AND (Unit == '%d')", valueInfo.mDefaultName.c_str(), (STYPE_Selector), 0, m_HwdID, xcmd.id, xcmd.unitcode);
		result = m_sql.safe_query("SELECT ID FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='%08X') AND (Unit == '%d')", m_HwdID, xcmd.id, xcmd.unitcode);
		if (!result.empty())
		{
			std::map<std::string, std::string> options;
			options["SelectorStyle"] = "1";
			options["LevelOffHidden"] = "true";
			options["LevelNames"] = deviceInfo.mSelectorStates;

			int sIdx = std::stoi(result[0][0]);
			m_sql.SetDeviceOptions(sIdx, options);
		}
	}
}


bool CAirconWithMe::WriteToHardware(const char* pdata, const unsigned char length)
{ 
	const tRBUF* pSen = reinterpret_cast<const tRBUF*>(pdata);
	unsigned char packettype = pSen->ICMND.packettype;
	if (packettype == pTypeGeneralSwitch)
	{
		const _tGeneralSwitch* pSwitch = reinterpret_cast<const _tGeneralSwitch*>(pSen);
		int32_t uid = pSwitch->id;
		if (_UIDMap.find(uid) == _UIDMap.end())
			return false;
		if (mDeviceInfo.find(uid) == mDeviceInfo.end())
			return false;

		int32_t value = 0;
		if (pSwitch->cmnd == gswitch_sOn)
			value = 1;
		if (pSwitch->cmnd == gswitch_sSetLevel)
		{
			value = mDeviceInfo[uid].mSelectorToState[pSwitch->level];
		}

		SendValueToAirco(uid, value);
	}
	else if (packettype == pTypeThermostat)
	{
		const _tThermostat* pThemostat = reinterpret_cast<const _tThermostat*>(pSen);
		int32_t uid = pThemostat->id3 * 256 + pThemostat->id4;
		if (_UIDMap.find(uid) == _UIDMap.end())
			return false;
		if (mDeviceInfo.find(uid) == mDeviceInfo.end())
			return false;

		int32_t value = static_cast<int32_t>(pThemostat->temp * 10);
		SendValueToAirco(uid, value);
	}


	return true; 
};

void CAirconWithMe::SendValueToAirco(const int32_t uid, const int32_t value)
{
	Json::Value root;
	std::string errorMessage;
	std::string postdata = R"({"command":"setdatapointvalue","data":{"sessionID":"%SESSION_ID%","uid":)" + std::to_string(uid) + ",\"value\":" + std::to_string(value) + "}}";

	if (!DoWebRequest(postdata, root, errorMessage, true))
	{
		Log(LOG_ERROR, "GetValues failed : " + errorMessage);
		return;
	}

	UpdateDomoticzWithValue(uid, value);
}
