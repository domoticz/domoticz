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
	Log(LOG_STATUS, "Connected to: %s:%d", m_szIPAddress.c_str(), m_usIPPort);
	m_bDataReceived = false;
	m_recvBuffer.clear();
	if (!m_username.empty())
	{
		std::string sAuth = std_format("SIGNv%d;%s;%s", REMOTE_PROTOCOL_VERSION, m_username.c_str(), m_password.c_str());
		WriteFramed(sAuth);
		mytime(&m_tAuthSent);
	}
	sOnConnected(this);
}

void DomoticzTCP::OnDisconnect()
{
	Log(LOG_STATUS, "Disconnected from: %s:%d", m_szIPAddress.c_str(), m_usIPPort);
}

void DomoticzTCP::OnData(const uint8_t* pData, size_t length)
{
	std::lock_guard<std::mutex> l(m_readMutex);

	m_bDataReceived = true;

	// NOAUTH is sent unframed during authentication (before any device data)
	if (m_recvBuffer.empty() && length == 6 && strncmp(reinterpret_cast<const char*>(pData), "NOAUTH", 6) == 0)
	{
		Log(LOG_ERROR, "Authentication failed for user %s on %s:%d", m_username.c_str(), m_szIPAddress.c_str(), m_usIPPort);
		return;
	}

	m_recvBuffer.append(reinterpret_cast<const char*>(pData), length);

	// Overflow guard: once we have a 4-byte header, validate the declared length
	// to prevent the buffer growing beyond 1 MB before the frame is processed.
	if (m_recvBuffer.size() >= 4)
	{
		uint32_t msgLen;
		memcpy(&msgLen, m_recvBuffer.data(), 4);
		msgLen = ntohl(msgLen);
		if (msgLen == 0 || msgLen > 1048576)
		{
			Log(LOG_ERROR, "Invalid frame length (%u) from %s:%d, resetting connection", msgLen, m_szIPAddress.c_str(), m_usIPPort);
			m_recvBuffer.clear();
			return;
		}
	}

	while (m_recvBuffer.size() >= 4)
	{
		uint32_t msgLen;
		memcpy(&msgLen, m_recvBuffer.data(), 4);
		msgLen = ntohl(msgLen);

		if (msgLen == 0 || msgLen > 1048576)
		{
			Log(LOG_ERROR, "Invalid frame length received (%u), resetting buffer", msgLen);
			m_recvBuffer.clear();
			return;
		}

		if (m_recvBuffer.size() < 4 + (size_t)msgLen)
			return; // incomplete frame — wait for more data

		std::string szEncoded(m_recvBuffer.data() + 4, msgLen);
		m_recvBuffer.erase(0, 4 + msgLen);

		std::vector<char> uhash = HexToBytes(m_password);
		std::string szDecoded;
		AESDecryptData(szEncoded, szDecoded, (const uint8_t*)uhash.data());

		Json::Value root;
		bool ret = ParseJSon(szDecoded, root);
		if ((!ret) || (!root.isObject()))
		{
			Log(LOG_ERROR, "Invalid data received!");
			continue;
		}

		if (root["OrgHardwareID"].empty())
		{
			Log(LOG_ERROR, "Invalid data received, or no data returned!");
			continue;
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
				if (!m_sql.m_bAcceptNewHardware)
				{
					Log(LOG_STATUS, "Device creation failed, Domoticz settings prevent accepting new devices. (device ID %s)", DeviceID.c_str());
					continue;
				}
				Log(LOG_ERROR, "Failed to update device %s", DeviceID.c_str());
				continue;
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

			if (IsLightOrSwitch(Type, SubType))
			{
				m_mainworker.CheckSceneCode(idx, Type, SubType, nValue, sValue.c_str(), m_Name);
			}
		}
		catch (const std::exception& e)
		{
			Log(LOG_ERROR, "Exception: Invalid data received! (%s)", e.what());
		}
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
		{
			mytime(&m_LastHeartbeat);
		}
		if (m_tAuthSent != 0 && !m_bDataReceived)
		{
			time_t now;
			mytime(&now);
			if (now - m_tAuthSent >= 12)
			{
				Log(LOG_ERROR, "No data received from %s:%d after 12 seconds. The remote Domoticz may be running an older version that does not support the SIGNv%d protocol. Please update the remote Domoticz instance.",
					m_szIPAddress.c_str(), m_usIPPort, REMOTE_PROTOCOL_VERSION);
				m_tAuthSent = 0; // log only once per connection
			}
		}
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

void DomoticzTCP::WriteFramed(const std::string& szData)
{
	uint32_t len = htonl((uint32_t)szData.size());
	std::string szFramed(reinterpret_cast<const char*>(&len), 4);
	szFramed += szData;
	write(szFramed);
}

bool DomoticzTCP::SendEncrypted(const std::string& szPlaintext)
{
	if (!ASyncTCP::isConnected())
		return false;
	std::vector<char> uhash = HexToBytes(m_password);
	std::string szEncrypted;
	AESEncryptData(szPlaintext, szEncrypted, (const uint8_t*)uhash.data());
	WriteFramed(szEncrypted);
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

	return SendEncrypted(JSonToRawString(root));
}

bool DomoticzTCP::SetSetPoint(const std::string& idx, const float TempValue, const std::string& User)
{
	Json::Value root;
	if (!AssambleDeviceInfo(idx, root))
		return false;
	root["action"] = "SetSetpoint";
	root["TempValue"] = TempValue;
	root["User"] = User;

	return SendEncrypted(JSonToRawString(root));
}

bool DomoticzTCP::SetSetPointEvo(const std::string& idx, float TempValue, const std::string& newMode, const std::string& until, const std::string& User)
{
	Json::Value root;
	if (!AssambleDeviceInfo(idx, root))
		return false;
	root["action"] = "SetSetpointEvo";
	root["TempValue"] = TempValue;
	root["newMode"] = newMode;
	root["until"] = until;
	root["User"] = User;

	return SendEncrypted(JSonToRawString(root));
}

bool DomoticzTCP::SetThermostatState(const std::string& idx, int newState)
{
	Json::Value root;
	if (!AssambleDeviceInfo(idx, root))
		return false;
	root["action"] = "SetThermostatState";
	root["newState"] = newState;

	return SendEncrypted(JSonToRawString(root));
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

	return SendEncrypted(JSonToRawString(root));
}

bool DomoticzTCP::SetTextDevice(const std::string& idx, const std::string& text)
{
	Json::Value root;
	if (!AssambleDeviceInfo(idx, root))
		return false;
	root["action"] = "SetTextDevice";
	root["text"] = text;

	return SendEncrypted(JSonToRawString(root));
}


#ifdef WITH_OPENZWAVE
bool DomoticzTCP::SetZWaveThermostatMode(const std::string& idx, int tMode)
{
	Json::Value root;
	if (!AssambleDeviceInfo(idx, root))
		return false;
	root["action"] = "SetZWaveThermostatMode";
	root["tMode"] = tMode;

	return SendEncrypted(JSonToRawString(root));
}

bool DomoticzTCP::SetZWaveThermostatFanMode(const std::string& idx, int fMode)
{
	Json::Value root;
	if (!AssambleDeviceInfo(idx, root))
		return false;
	root["action"] = "SetZWaveThermostatFanMode";
	root["fMode"] = fMode;

	return SendEncrypted(JSonToRawString(root));
}
#endif

