#include "stdafx.h"
#include "DomoticzTCP.h"
#include "../main/json_helper.h"
#include "../main/Logger.h"
#include "../main/Helper.h"
#include "../main/mainworker.h"
#include "../main/SQLHelper.h"
#include "../main/WebServerHelper.h"

#define RETRY_DELAY 30

extern http::server::CWebServerHelper m_webservers;

DomoticzTCP::DomoticzTCP(const int ID, const std::string& IPAddress, const unsigned short usIPPort, const std::string& username, const std::string& password)
	: m_szIPAddress(IPAddress)
	, m_username(username)
	, m_password(password)
{
	m_HwdID = ID;
	m_usIPPort = usIPPort;
	m_bIsStarted = false;
}

bool DomoticzTCP::StartHardware()
{
	RequestStart();

	//Start worker thread
	m_thread = std::make_shared<std::thread>([this] { Do_Work(); });
	SetThreadNameInt(m_thread->native_handle());

	return (m_thread != nullptr);
}

bool DomoticzTCP::StopHardware()
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

void DomoticzTCP::OnConnect()
{
	Log(LOG_STATUS, "connected to: %s:%d", m_szIPAddress.c_str(), m_usIPPort);
	if (!m_username.empty())
	{
		std::string sAuth = std_format("SIGNv2;%s;%s", m_username.c_str(), m_password.c_str());
		WriteToHardware(sAuth);
	}
	sOnConnected(this);
}

void DomoticzTCP::OnDisconnect()
{
	Log(LOG_STATUS, "disconnected from: %s:%d", m_szIPAddress.c_str(), m_usIPPort);
}

void DomoticzTCP::OnData(const uint8_t* pData, size_t length)
{
	if (length == 6 && strstr(reinterpret_cast<const char*>(pData), "NOAUTH") != nullptr)
	{
		Log(LOG_ERROR, "Authentication failed for user %s on %s:%d", m_username.c_str(), m_szIPAddress.c_str(), m_usIPPort);
		return;
	}

	std::lock_guard<std::mutex> l(m_readMutex);

	std::vector<char> uhash = HexToBytes(m_password);

	std::string szEncoded = std::string((const char*)pData, length);
	std::string szDecoded;

	AESDecryptData(szEncoded, szDecoded, (const uint8_t*)uhash.data());

	Json::Value root;

	bool ret = ParseJSon(szDecoded, root);
	if ((!ret) || (!root.isObject()))
	{
		Log(LOG_ERROR, "Invalid data received!");
		return;
	}

	if (root["OrgHardwareID"].empty() == true)
	{
		Log(LOG_ERROR, "Invalid data received, or no data returned!");
		return;
	}

	try
	{
		int OrgHardwareID = root["OrgHardwareID"].asInt();
		int OrgDeviceRowID = root["OrgDeviceRowID"].asInt();
		std::string DeviceID = root["DeviceID"].asString();
		int Unit = root["Unit"].asInt();
		std::string Name = root["Name"].asString();
		int Type = root["Type"].asInt();
		int SubType = root["SubType"].asInt();
		int SwitchType = root["SwitchType"].asInt();
		int SignalLevel = root["SignalLevel"].asInt();
		int BatteryLevel = root["BatteryLevel"].asInt();
		int nValue = root["nValue"].asInt();
		std::string sValue = root["sValue"].asString();
		std::string LastUpdate = root["LastUpdate"].asString();
		int LastLevel = root["LastLevel"].asInt();
		std::string Options = root["Options"].asString();
		std::string Color = root["Color"].asString();

		uint64_t idx = m_sql.UpdateValue(m_HwdID, OrgHardwareID, DeviceID.c_str(), Unit, Type, SubType, SignalLevel, BatteryLevel, nValue, sValue.c_str(), Name, true, m_Name.c_str());
		if (idx == (uint64_t)-1)
		{
			Log(LOG_ERROR, "Failed to update device %s", DeviceID.c_str());
			return;
		}

		auto result = m_sql.safe_query("SELECT SwitchType, Options, Color FROM DeviceStatus WHERE (ID==%q)", std::to_string(idx).c_str());

		int oldSwitchType = atoi(result[0][0].c_str());
		std::string oldOptions = result[0][1];
		std::string oldColor = result[0][2];

		if (SwitchType != oldSwitchType)
			m_sql.UpdateDeviceValue("SwitchType", SwitchType, std::to_string(idx));
		if (Options != oldOptions)
			m_sql.UpdateDeviceValue("Options", Options, std::to_string(idx));
		if (Color != oldColor)
			m_sql.UpdateDeviceValue("Color", Color, std::to_string(idx));

		m_sql.UpdateDeviceValue("LastUpdate", LastUpdate, std::to_string(idx));

	}
	catch (const std::exception&)
	{
		Log(LOG_ERROR, "Invalid data received!");
	}
}

void DomoticzTCP::OnError(const boost::system::error_code& error)
{
	if (
		(error == boost::asio::error::address_in_use) ||
		(error == boost::asio::error::connection_refused) ||
		(error == boost::asio::error::access_denied) ||
		(error == boost::asio::error::host_unreachable) ||
		(error == boost::asio::error::timed_out) ||
		(error == boost::asio::error::host_not_found)
		)
	{
		Log(LOG_ERROR, "Can not connect to: %s:%d (%s)", m_szIPAddress.c_str(), m_usIPPort, error.message().c_str());
	}
	else if (error != boost::asio::error::eof)
	{
		Log(LOG_ERROR, "%s", error.message().c_str());
	}
}

void DomoticzTCP::Do_Work()
{
	connect(m_szIPAddress, m_usIPPort);
	int sec_counter = 0;
	while (!IsStopRequested(1000))
	{
		sec_counter++;
		if (sec_counter % 12 == 0)
			mytime(&m_LastHeartbeat);
	}
	terminate();

	Log(LOG_STATUS, "Worker stopped...");
}

bool DomoticzTCP::WriteToHardware(const char* pdata, unsigned char length)
{
	if (!ASyncTCP::isConnected())
		return false;
	write(std::string(pdata, length));
	return true;
}

bool DomoticzTCP::WriteToHardware(const std::string& szData)
{
	if (!ASyncTCP::isConnected())
		return false;
	write(szData);
	return true;
}

bool AssambleDeviceInfo(const std::string& idx, Json::Value& root)
{
	auto result = m_sql.safe_query("SELECT OrgHardwareID, DeviceID, Unit, Type, SubType FROM DeviceStatus WHERE (ID==%q)", idx.c_str());
	if (result.empty())
		return false;
	int iIndex = 0;
	root["HardwareID"] = atoi(result[0][iIndex++].c_str());
	root["DeviceID"] = result[0][iIndex++];
	root["Unit"] = atoi(result[0][iIndex++].c_str());
	root["Type"] = atoi(result[0][iIndex++].c_str());
	root["SubType"] = atoi(result[0][iIndex++].c_str());
	return true;
}

bool DomoticzTCP::SwitchLight(const uint64_t idx, const std::string& switchcmd, const int level, _tColor color, const bool ooc, const std::string& User)
{
	Json::Value root;
	if (!AssambleDeviceInfo(std::to_string(idx), root))
		return false;
	root["action"] = "SwitchLight";
	root["switchcmd"] = switchcmd;
	root["level"] = level;
	root["color"] = color.toJSONString();
	root["ooc"] = ooc;
	root["User"] = User;

	std::string szSend = JSonToRawString(root);
	std::vector<char> uhash = HexToBytes(m_password);
	std::string szEncrypted;
	AESEncryptData(szSend, szEncrypted, (const uint8_t*)uhash.data());
	return WriteToHardware(szEncrypted);
}

bool DomoticzTCP::SetSetPoint(const std::string& idx, const float TempValue)
{
	Json::Value root;
	if (!AssambleDeviceInfo(idx, root))
		return false;
	root["action"] = "SetSetpoint";
	root["TempValue"] = TempValue;

	std::string szSend = JSonToRawString(root);
	std::vector<char> uhash = HexToBytes(m_password);
	std::string szEncrypted;
	AESEncryptData(szSend, szEncrypted, (const uint8_t*)uhash.data());
	return WriteToHardware(szEncrypted);
}

bool DomoticzTCP::SetSetPointEvo(const std::string& idx, float TempValue, const std::string& newMode, const std::string& until)
{
	Json::Value root;
	if (!AssambleDeviceInfo(idx, root))
		return false;
	root["action"] = "SetSetpointEvo";
	root["TempValue"] = TempValue;
	root["newMode"] = newMode;
	root["until"] = until;

	std::string szSend = JSonToRawString(root);
	std::vector<char> uhash = HexToBytes(m_password);
	std::string szEncrypted;
	AESEncryptData(szSend, szEncrypted, (const uint8_t*)uhash.data());
	return WriteToHardware(szEncrypted);
}

bool DomoticzTCP::SetThermostatState(const std::string& idx, int newState)
{
	Json::Value root;
	if (!AssambleDeviceInfo(idx, root))
		return false;
	root["action"] = "SetThermostatState";
	root["newState"] = newState;

	std::string szSend = JSonToRawString(root);
	std::vector<char> uhash = HexToBytes(m_password);
	std::string szEncrypted;
	AESEncryptData(szSend, szEncrypted, (const uint8_t*)uhash.data());
	return WriteToHardware(szEncrypted);
}

bool DomoticzTCP::SwitchEvoModal(const std::string& idx, const std::string& status, const std::string& action, const std::string& ooc, const std::string& until)
{
	Json::Value root;
	if (!AssambleDeviceInfo(idx, root))
		return false;
	root["action"] = "SwitchEvoModal";
	root["status"] = status;
	root["evo_action"] = action;
	root["ooc"] = ooc;
	root["until"] = until;

	std::string szSend = JSonToRawString(root);
	std::vector<char> uhash = HexToBytes(m_password);
	std::string szEncrypted;
	AESEncryptData(szSend, szEncrypted, (const uint8_t*)uhash.data());
	return WriteToHardware(szEncrypted);
}
