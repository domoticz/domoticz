#include "stdafx.h"
#include "Enigma2.h"
#include "../main/Helper.h"
#include "../main/Logger.h"
#include "../main/RFXNames.h"
#include "../main/RFXtrx.h"
#include "../main/SQLHelper.h"
#include "../main/mainworker.h"
#include "../httpclient/HTTPClient.h"
#include "../tinyxpath/tinyxml.h"
#include "hardwaretypes.h"
#include <libwebem/Base64.h>

// Enigma2 WebInterface (https://dream.reichholf.net/wiki/Enigma2:WebInterface), served by OpenWebif.
//
// Unit 1 Status - Media Player: Off (box does not answer ping), On (channel name or "Enigma2 ON"), Sleeping (standby).
// Unit 2 Source - selector Off|Standby|Reboot|RestartE2|On, levels 0..40 map to /web/powerstate?newstate=:
//   Off 1 (deep standby, or 5 when deep standby is not supported), Standby 5, Reboot 2, RestartE2 3, On 4.
//
// A box whose web interface is silent but which still answers ping (Enigma2 restarting) keeps its last state.
//
// Mode1: poll interval in seconds, Mode2: deep standby supported (0/1), Mode3: show channel name (0/1)

#define ENIGMA2_UNIT_STATUS 1
#define ENIGMA2_UNIT_SOURCE 2

#define ENIGMA2_POWER_DEEPSTANDBY 1
#define ENIGMA2_POWER_REBOOT 2
#define ENIGMA2_POWER_RESTARTE2 3
#define ENIGMA2_POWER_WAKEUP 4
#define ENIGMA2_POWER_STANDBY 5

#define ENIGMA2_HTTP_TIMEOUT 4

CEnigma2::CEnigma2(const int ID, const std::string &IPAddress, const unsigned short usIPPort, const std::string &username, const std::string &password, const int pollInterval,
		   const int deepStandby, const int channelName)
	: m_szIPAddress(IPAddress)
	, m_usIPPort(usIPPort ? usIPPort : 80)
	, m_iPollInterval(pollInterval)
	, m_bDeepStandby(deepStandby != 0)
	, m_bChannelName(channelName != 0)
{
	m_HwdID = ID;
	if (m_iPollInterval < 10)
		m_iPollInterval = 60;
	if (!username.empty())
		m_ExtraHeaders.push_back("Authorization: Basic " + base64_encode(username + ":" + password));
}

bool CEnigma2::StartHardware()
{
	RequestStart();

	CreateDevices();

	m_thread = std::make_shared<std::thread>([this] { Do_Work(); });
	SetThreadNameInt(m_thread->native_handle());
	m_bIsStarted = true;
	sOnConnected(this);
	return (m_thread != nullptr);
}

bool CEnigma2::StopHardware()
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

void CEnigma2::Do_Work()
{
	int sec_counter = 0;
	Log(LOG_STATUS, "Worker started, box %s:%d, poll every %d s", m_szIPAddress.c_str(), m_usIPPort, m_iPollInterval);
	while (!IsStopRequested(1000))
	{
		sec_counter++;
		if (sec_counter % 12 == 0)
			m_LastHeartbeat = mytime(nullptr);

		if (--m_iSecondsToPoll <= 0)
		{
			m_iSecondsToPoll = m_iPollInterval;
			Poll();
		}
	}
	Log(LOG_STATUS, "Worker stopped");
}

// Status and Source are created when missing. Existing devices (also those created by the
// Enigma2 Python plugin, which uses the same units) are kept with their history.
void CEnigma2::CreateDevices()
{
	char szID[20];
	std::vector<std::vector<std::string>> result;

	// The ID a Lighting2 command echo uses; the Python plugin stored it as %04X%04X
	snprintf(szID, sizeof(szID), "%X%02X%02X%02X", 0, 0, (m_HwdID & 0xFF00) >> 8, m_HwdID & 0xFF);
	result = m_sql.safe_query("SELECT ID, DeviceID FROM DeviceStatus WHERE (HardwareID==%d) AND (Unit==%d)", m_HwdID, ENIGMA2_UNIT_STATUS);
	if (!result.empty())
	{
		if (result[0][1] != szID)
		{
			Log(LOG_STATUS, "Status device ID %s changed to %s", result[0][1].c_str(), szID);
			m_sql.safe_query("UPDATE DeviceStatus SET DeviceID='%q' WHERE (ID==%s)", szID, result[0][0].c_str());
		}
	}
	else
	{
		uint64_t idx = m_sql.InsertDevice(m_HwdID, 0, szID, ENIGMA2_UNIT_STATUS, pTypeLighting2, sTypeAC, STYPE_Media, 0, "Enigma2 OFF", m_Name + " - Status", 12, 255, 1);
		if (idx != (uint64_t)-1)
			m_sql.safe_query("UPDATE DeviceStatus SET CustomImage=2 WHERE (ID==%" PRIu64 ")", idx);
	}

	result = m_sql.safe_query("SELECT ID FROM DeviceStatus WHERE (HardwareID==%d) AND (Unit==%d)", m_HwdID, ENIGMA2_UNIT_SOURCE);
	if (!result.empty())
	{
		// Off is shown only when it sends deep standby; follow the setting after it changes
		std::map<std::string, std::string> options = m_sql.GetDeviceOptions(result[0][0]);
		const std::string offHidden = m_bDeepStandby ? "false" : "true";
		if (options["LevelOffHidden"] != offHidden)
		{
			options["LevelOffHidden"] = offHidden;
			m_sql.SetDeviceOptions(std::stoull(result[0][0]), options);
		}
	}
	else
	{
		snprintf(szID, sizeof(szID), "%08X", m_HwdID);
		uint64_t idx = m_sql.InsertDevice(m_HwdID, 0, szID, ENIGMA2_UNIT_SOURCE, pTypeGeneralSwitch, sSwitchTypeSelector, STYPE_Selector, 0, "0", m_Name + " - Source", 12, 255, 1);
		if (idx != (uint64_t)-1)
		{
			std::string options = "LevelActions:||||;LevelNames:Off|Standby|Reboot|RestartE2|On;LevelOffHidden:";
			options += m_bDeepStandby ? "false" : "true";
			options += ";SelectorStyle:0";
			m_sql.SetDeviceOptions(idx, m_sql.BuildDeviceOptions(options, false));
			m_sql.safe_query("UPDATE DeviceStatus SET CustomImage=12 WHERE (ID==%" PRIu64 ")", idx);
		}
	}
}

bool CEnigma2::GetXML(const std::string &endpoint, const long timeoutSec, TiXmlDocument &doc)
{
	std::stringstream sURL;
	sURL << "http://" << m_szIPAddress << ":" << m_usIPPort << "/web/" << endpoint;

	std::vector<unsigned char> vResponse;
	if (!HTTPClient::GETBinary(sURL.str(), m_ExtraHeaders, vResponse, timeoutSec))
	{
		Debug(DEBUG_HARDWARE, "No answer from %s", sURL.str().c_str());
		return false;
	}
	std::string sResult(vResponse.begin(), vResponse.end());
	doc.Parse(sResult.c_str(), nullptr, TIXML_ENCODING_UTF8);
	if (doc.Error() || (doc.RootElement() == nullptr))
	{
		Log(LOG_ERROR, "Invalid XML from %s", sURL.str().c_str());
		return false;
	}
	return true;
}

// Text of root/child, trimmed; empty when missing
static std::string ChildText(const TiXmlElement *parent, const char *name)
{
	if (parent == nullptr)
		return "";
	const TiXmlElement *pElem = parent->FirstChildElement(name);
	if ((pElem == nullptr) || (pElem->GetText() == nullptr))
		return "";
	std::string text = pElem->GetText();
	return stdstring_trimws(text);
}

void CEnigma2::LogBoxInfo()
{
	TiXmlDocument doc;
	if (!GetXML("about", ENIGMA2_HTTP_TIMEOUT, doc))
		return;
	const TiXmlElement *pAbout = doc.RootElement()->FirstChildElement("e2about");
	std::string model = ChildText(pAbout, "e2model");
	std::string version = ChildText(pAbout, "e2enigmaversion");
	if (!model.empty())
		Log(LOG_STATUS, "Connected to %s, Enigma2 %s", model.c_str(), version.c_str());
	m_bInfoLogged = true;
}

std::string CEnigma2::GetChannelName()
{
	TiXmlDocument doc;
	if (!GetXML("subservices", ENIGMA2_HTTP_TIMEOUT, doc))
		return "";
	return ChildText(doc.RootElement()->FirstChildElement("e2service"), "e2servicename");
}

bool CEnigma2::IsPinging()
{
	// The address goes to a shell - accept host names and IP addresses only
	for (const char c : m_szIPAddress)
	{
		if (!isalnum(static_cast<unsigned char>(c)) && (c != '.') && (c != '-') && (c != ':'))
			return false;
	}
#ifdef WIN32
	std::string sCmd = "ping -n 1 -w 1000 " + m_szIPAddress;
#else
	std::string sCmd = "ping -c 1 -W 1 " + m_szIPAddress + " 2>/dev/null";
#endif
	int returncode = -1;
	ExecuteCommandAndReturn(sCmd, returncode);
	return (returncode == 0);
}

void CEnigma2::Poll()
{
	TiXmlDocument doc;
	if (!GetXML("powerstate", ENIGMA2_HTTP_TIMEOUT, doc))
	{
		if (IsPinging())
		{
			if (!m_bWebIfDown)
			{
				Log(LOG_STATUS, "Box answers ping, web interface does not - status unchanged");
				m_bWebIfDown = true;
			}
			return;
		}
		m_bWebIfDown = false;
		m_szLastChannel.clear();
		UpdateUnit(ENIGMA2_UNIT_STATUS, MSTAT_OFF, "Enigma2 OFF");
		UpdateUnit(ENIGMA2_UNIT_SOURCE, 0, "0");
		return;
	}
	if (m_bWebIfDown)
	{
		Log(LOG_STATUS, "Web interface answers again");
		m_bWebIfDown = false;
	}
	if (!m_bInfoLogged)
		LogBoxInfo();

	// Anything but true/false (an error page that parses as XML) keeps the last state
	std::string inStandby = ChildText(doc.RootElement(), "e2instandby");
	if ((inStandby != "true") && (inStandby != "false"))
	{
		Log(LOG_ERROR, "Unexpected /web/powerstate response");
		return;
	}

	if (inStandby == "false")
	{
		std::string channel;
		if (m_bChannelName)
		{
			channel = GetChannelName();
			if (!channel.empty() && (channel != m_szLastChannel))
				Log(LOG_STATUS, "Current channel: %s", channel.c_str());
			m_szLastChannel = channel;
		}
		UpdateUnit(ENIGMA2_UNIT_SOURCE, 2, "40");
		UpdateUnit(ENIGMA2_UNIT_STATUS, MSTAT_ON, channel.empty() ? "Enigma2 ON" : channel);
	}
	else
	{
		m_szLastChannel.clear();
		UpdateUnit(ENIGMA2_UNIT_SOURCE, 2, "10");
		UpdateUnit(ENIGMA2_UNIT_STATUS, MSTAT_SLEEPING, "Enigma2 Standby");
	}
}

// Writes only a change, like a device update would be shown in the log
void CEnigma2::UpdateUnit(const int unit, const int nValue, const std::string &sValue)
{
	std::vector<std::vector<std::string>> result;
	result = m_sql.safe_query("SELECT DeviceID, Type, SubType, nValue, sValue, Name FROM DeviceStatus WHERE (HardwareID==%d) AND (Unit==%d)", m_HwdID, unit);
	if (result.empty())
		return;
	if ((atoi(result[0][3].c_str()) == nValue) && (result[0][4] == sValue))
		return;

	std::string devname = result[0][5];
	uint64_t DeviceRowIdx = m_sql.UpdateValue(m_HwdID, 0, result[0][0].c_str(), (unsigned char)unit, (unsigned char)atoi(result[0][1].c_str()), (unsigned char)atoi(result[0][2].c_str()), 12, 255,
						  nValue, sValue.c_str(), devname, true, m_Name.c_str());
	if (DeviceRowIdx == (uint64_t)-1)
		return;
	// Live updates for the web interface and push/MQTT exports
	m_mainworker.sOnDeviceReceived(m_HwdID, DeviceRowIdx, devname, nullptr);
}

bool CEnigma2::SendPowerState(const int newState)
{
	TiXmlDocument doc;
	std::string endpoint = "powerstate?newstate=" + std::to_string(newState);
	Log(LOG_STATUS, "Sending %s", endpoint.c_str());
	bool bOK = GetXML(endpoint, ENIGMA2_HTTP_TIMEOUT, doc);
	if (!bOK && (newState == ENIGMA2_POWER_DEEPSTANDBY))
	{
		// The box may drop the connection while it starts shutting down
		Log(LOG_STATUS, "Deep standby: no answer from the box (expected when it shuts down)");
		bOK = true;
	}
	else if (!bOK)
		Log(LOG_ERROR, "Command %s failed", endpoint.c_str());

	// Read the new state a few seconds later
	m_iSecondsToPoll = 5;
	return bOK;
}

bool CEnigma2::WriteToHardware(const char *pdata, const unsigned char /*length*/)
{
	const tRBUF *pCmd = reinterpret_cast<const tRBUF *>(pdata);
	const int offState = m_bDeepStandby ? ENIGMA2_POWER_DEEPSTANDBY : ENIGMA2_POWER_STANDBY;
	int newState = -1;

	if ((pCmd->ICMND.packettype == pTypeLighting2) && (pCmd->LIGHTING2.unitcode == ENIGMA2_UNIT_STATUS))
	{
		switch (pCmd->LIGHTING2.cmnd)
		{
			case light2_sOff:
			case light2_sGroupOff:
				newState = offState;
				break;
			case light2_sOn:
			case light2_sGroupOn:
				newState = ENIGMA2_POWER_WAKEUP;
				break;
			default:
				break;
		}
	}
	else if (pCmd->ICMND.packettype == pTypeGeneralSwitch)
	{
		const _tGeneralSwitch *pSwitch = reinterpret_cast<const _tGeneralSwitch *>(pdata);
		if (pSwitch->unitcode == ENIGMA2_UNIT_SOURCE)
		{
			if (pSwitch->cmnd == gswitch_sOff)
				newState = offState;
			else if (pSwitch->cmnd == gswitch_sSetLevel)
			{
				switch (pSwitch->level)
				{
					case 0:
						newState = offState;
						break;
					case 10:
						newState = ENIGMA2_POWER_STANDBY;
						break;
					case 20:
						newState = ENIGMA2_POWER_REBOOT;
						break;
					case 30:
						newState = ENIGMA2_POWER_RESTARTE2;
						break;
					case 40:
						newState = ENIGMA2_POWER_WAKEUP;
						break;
					default:
						break;
				}
			}
		}
	}

	if (newState < 0)
	{
		Log(LOG_ERROR, "Unsupported command (packet type %d)", pCmd->ICMND.packettype);
		return false;
	}
	return SendPowerState(newState);
}
