#include "stdafx.h"
#include "OnkyoAVTCP.h"
#include "../main/Logger.h"
#include "../main/Helper.h"
#include "../main/SQLHelper.h"
#include <iostream>
#include "../main/localtime_r.h"
#include "../main/mainworker.h"
#include "../hardware/hardwaretypes.h"
#include <json/json.h>
#include "../tinyxpath/tinyxml.h"
#include "../main/WebServer.h"

#include <sstream>

#define RETRY_DELAY 30

namespace
{
	struct eiscp_packet
	{
		uint32_t magic;
		uint32_t hdr_size;
		uint32_t data_size;
		uint8_t version;
		uint8_t reserved[3];
		char start;
		char dest;
		char message[2];
	};

	// Do not change except to append to these, or you break compatibility.
	enum DevNr
	{
		ID_MVL = 0,
		ID_ZVL,
		ID_VL3,
		ID_VL4,
		ID_PWR,
		ID_ZPW,
		ID_PW3,
		ID_PW4,
		ID_SLI,
		ID_SLZ,
		ID_SL3,
		ID_SL4,
		ID_HDO,
	};

	struct selector_name
	{
		int val;
		const char *name;
	};

	const struct selector_name input_names[] = {
		{ 0x00, "STB/DVR" },
		{ 0x01, "CBL/SAT" },
		{ 0x02, "GAME/TV" },
		{ 0x03, "AUX1" },
		{ 0x04, "AUX2/GAME2" },
		{ 0x05, "PC" },
		{ 0x06, "VIDEO7" },
		{ 0x07, "Hidden1" },
		{ 0x08, "Hidden2" },
		{ 0x09, "Hidden3" },
		{ 0x10, "BD/DVD" },
		{ 0x11, "STRM BOX" },
		{ 0x12, "TV" },
		{ 0x20, "TV/TAPE" },
		{ 0x21, "TAPE2" },
		{ 0x22, "PHONO" },
		{ 0x23, "TV/CD" },
		{ 0x24, "FM" },
		{ 0x25, "AM" },
		{ 0x26, "TUNER" },
		{ 0x27, "Music Server" },
		{ 0x28, "Internet Radio" },
		{ 0x29, "USB/USB(Front)" },
		{ 0x2a, "USB(Rear)" },
		{ 0x2c, "USB(Toggle)" },
		{ 0x2d, "Airplay" },
		{ 0x2e, "Bluetooth" },
		{ 0x30, "MULTI CH" },
		{ 0x31, "XM" },
		{ 0x32, "SIRIUS" },
		{ 0x33, "DAB" },
		{ 0x40, "Universal PORT" },
		{ 0x55, "HDMI5" },
		{ 0x56, "HDMI6" },
		{ 0x57, "HDMI7" },
		{ 0x80, "Source" },
		{ 0, nullptr },
	};

	using _switch_types = struct
	{
		const char *iscpCmd;
		const char *iscpMute;
		const char *name;
		uint8_t switchType;
		uint8_t subtype;
		int customImage;
		const char *options;
		const struct selector_name *default_names;
	};

	constexpr std::array<_switch_types, 13> switch_types{
		{
			{ "MVL", "AMT", "Master volume", STYPE_Dimmer, sSwitchGeneralSwitch, 8, nullptr, nullptr },										    //
			{ "ZVL", "ZMT", "Zone 2 volume", STYPE_Dimmer, sSwitchGeneralSwitch, 8, nullptr, nullptr },										    //
			{ "VL3", "MT3", "Zone 3 volume", STYPE_Dimmer, sSwitchGeneralSwitch, 8, nullptr, nullptr },										    //
			{ "VL4", "MT4", "Zone 4 volume", STYPE_Dimmer, sSwitchGeneralSwitch, 8, nullptr, nullptr },										    //
			{ "PWR", nullptr, "Master power", STYPE_OnOff, sSwitchGeneralSwitch, 5, nullptr, nullptr },										    //
			{ "ZPW", nullptr, "Zone 2 power", STYPE_OnOff, sSwitchGeneralSwitch, 5, nullptr, nullptr },										    //
			{ "PW3", nullptr, "Zone 3 power", STYPE_OnOff, sSwitchGeneralSwitch, 5, nullptr, nullptr },										    //
			{ "PW4", nullptr, "Zone 4 power", STYPE_OnOff, sSwitchGeneralSwitch, 5, nullptr, nullptr },										    //
			{ "SLI", nullptr, "Master selector", STYPE_Selector, sSwitchTypeSelector, 5, "SelectorStyle:1;LevelNames:Off;LevelOffHidden:true;LevelActions:00", input_names },	    //
			{ "SLZ", nullptr, "Zone 2 selector", STYPE_Selector, sSwitchTypeSelector, 5, "SelectorStyle:1;LevelNames:Off;LevelOffHidden:true;LevelActions:00", input_names },	    //
			{ "SL2", nullptr, "Zone 3 selector", STYPE_Selector, sSwitchTypeSelector, 5, "SelectorStyle:1;LevelNames:Off;LevelOffHidden:true;LevelActions:00", input_names },	    //
			{ "SL3", nullptr, "Zone 4 selector", STYPE_Selector, sSwitchTypeSelector, 5, "SelectorStyle:1;LevelNames:Off;LevelOffHidden:true;LevelActions:00", input_names },	    //
			{ "HDO", nullptr, "HDMI Output", STYPE_Selector, sSwitchTypeSelector, 5, "SelectorStyle:0;LevelNames:Off|Main|Sub|Main+Sub;LevelOffHidden:true;LevelActions:00|01|02|03" }, //
		}																						    //
	};

	constexpr std::array<std::pair<const char *, const char *>, 5> text_types{
		{
			{ "NAL", "Album name" },    //
			{ "NAT", "Artist name" },   //
			{ "NTI", "Title name" },    //
			{ "NTM", "Playback time" }, //
			{ "NTR", "Track info" },    //
		}				    //
	};

#define IS_END_CHAR(x) ((x) == 0x1a)
}; // namespace

OnkyoAVTCP::OnkyoAVTCP(const int ID, const std::string &IPAddress, const unsigned short usIPPort) :
	m_szIPAddress(IPAddress)
{
	m_HwdID=ID;
	m_usIPPort=usIPPort;
	m_retrycntr = RETRY_DELAY;
	m_pPartialPkt = nullptr;
	m_PPktLen = 0;

	// Ooops, changing Device ID was a mistake. Fix up migration for Main/Z2 power switches from
	// the 3.8153 release. Don't change IDs again!
	for (int i = ID_PWR; i < ID_PW3; i++)
		m_sql.safe_query("UPDATE DeviceStatus SET DeviceID='%08X' WHERE (HardwareID==%d) AND (DeviceID=='%08X') AND (SwitchType==%d) AND (SubType==%d)",
				 i, m_HwdID, i - 2, switch_types[i].switchType, switch_types[i].subtype);
}

OnkyoAVTCP::~OnkyoAVTCP()
{
	free(m_pPartialPkt);
}

bool OnkyoAVTCP::StartHardware()
{
	RequestStart();

	//force connect the next first time
	m_retrycntr=RETRY_DELAY;
	m_bIsStarted=true;

	//Start worker thread
	m_thread = std::make_shared<std::thread>([this] { Do_Work(); });
	SetThreadNameInt(m_thread->native_handle());
	return (m_thread != nullptr);
}

bool OnkyoAVTCP::StopHardware()
{
	if (m_thread)
	{
		RequestStop();
		m_thread->join();
		m_thread.reset();
	}
	m_bIsStarted=false;
	return true;
}

void OnkyoAVTCP::OnConnect()
{
	_log.Log(LOG_STATUS,"OnkyoAVTCP: connected to: %s:%d", m_szIPAddress.c_str(), m_usIPPort);
	m_bIsStarted=true;

	SendPacket("NRIQSTN");
	sOnConnected(this);
}

void OnkyoAVTCP::OnDisconnect()
{
	_log.Log(LOG_STATUS,"OnkyoAVTCP: disconnected");
}

void OnkyoAVTCP::Do_Work()
{
	int sec_counter = 0;
	connect(m_szIPAddress, m_usIPPort);
	while (!IsStopRequested(1000))
	{
		sec_counter++;

		if (sec_counter  % 12 == 0) {
			m_LastHeartbeat = mytime(nullptr);
		}
	}
	terminate();

	_log.Log(LOG_STATUS,"OnkyoAVTCP: TCP/IP Worker stopped...");
}

void OnkyoAVTCP::OnData(const unsigned char *pData, size_t length)
{
	ParseData(pData,length);
}

void OnkyoAVTCP::OnError(const boost::system::error_code& error)
{
	if (
		(error == boost::asio::error::address_in_use) ||
		(error == boost::asio::error::connection_refused) ||
		(error == boost::asio::error::access_denied) ||
		(error == boost::asio::error::host_unreachable) ||
		(error == boost::asio::error::timed_out)
		)
	{
		_log.Log(LOG_ERROR, "OnkyoAVTCP: Can not connect to: %s:%d", m_szIPAddress.c_str(), m_usIPPort);
	}
	else if (
		(error == boost::asio::error::eof) ||
		(error == boost::asio::error::connection_reset)
		)
	{
		_log.Log(LOG_STATUS, "OnkyoAVTCP: Connection reset!");
	}
	else
		_log.Log(LOG_ERROR, "OnkyoAVTCP: %s", error.message().c_str());
}

bool OnkyoAVTCP::WriteToHardware(const char *pdata, const unsigned char /*length*/)
{
	if (!isConnected() || !pdata)
	{
		return false;
	}
	const tRBUF *pCmd = reinterpret_cast<const tRBUF *>(pdata);
	unsigned char packettype = pCmd->ICMND.packettype;
	std::string message;

	if (packettype == pTypeGeneralSwitch) {
		const _tGeneralSwitch *xcmd = reinterpret_cast<const _tGeneralSwitch*>(pdata);
		int ID = xcmd->id;
		int level = xcmd->level;
		char buf[9];
		if (ID >= int(switch_types.size()))
			return false;
		switch(xcmd->cmnd) {
		case gswitch_sSetLevel:
			if (switch_types[ID].subtype == sSwitchTypeSelector) {
				std::vector<std::vector<std::string> > result;
				result = m_sql.safe_query("SELECT Options FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='%08X')", m_HwdID, ID);
				if (result.empty()) {
					_log.Log(LOG_ERROR, "OnkyoAVTCP: No device with ID %08X", ID);
					return false;
				}
				std::string val = GetSelectorSwitchLevelAction(m_sql.BuildDeviceOptions(result[0][0]), level);
				if (val.empty())
					snprintf(buf, 6, "%s%02X", switch_types[ID].iscpCmd, level / 10);
				else {
					// Seriously. They *send* us the ID with lower case, and then refuse to accept it like that.
					boost::to_upper(val);
					snprintf(buf, 6, "%s%s", switch_types[ID].iscpCmd, val.c_str());
				}
			} else {
				snprintf(buf, 6, "%s%02X", switch_types[ID].iscpCmd, level);
			}
			SendPacket(buf);
			break;
		case gswitch_sOn:
			if (switch_types[ID].iscpMute)
				sprintf(buf, "%s00", switch_types[ID].iscpMute);
			else
				sprintf(buf, "%s01", switch_types[ID].iscpCmd);
			SendPacket(buf);
			break;
		case gswitch_sOff:
			if (switch_types[ID].iscpMute)
				sprintf(buf, "%s01", switch_types[ID].iscpMute);
			else
				sprintf(buf, "%s00", switch_types[ID].iscpCmd);
			SendPacket(buf);
			break;
		}
	}
	return true;
}

bool OnkyoAVTCP::SendPacket(const char *pdata)
{
	if (!isConnected() || !pdata)
	{
		return false;
	}

	_log.Log(LOG_NORM, "OnkyoAVTCP: Send %s", pdata);
	size_t length = strlen(pdata);

	struct eiscp_packet *pPkt = (struct eiscp_packet *)malloc(sizeof(*pPkt) + length);
	if (!pPkt)
	{
		return false;
	}

	pPkt->magic = htonl(0x49534350); // "ISCP"
	pPkt->hdr_size = htonl(16);
	pPkt->data_size = htonl(length + 3);
	pPkt->version = 1;
	memset(pPkt->reserved, 0, sizeof(pPkt->reserved));
	pPkt->start = '!';
	pPkt->dest = '1';
	memcpy(pPkt->message, pdata, length);
	pPkt->message[length] = 0x0d;
	pPkt->message[length + 1] = 0x0a;

	write((unsigned char *)pPkt, length + sizeof(*pPkt));
	free(pPkt);
	return true;
}

bool OnkyoAVTCP::SendPacket(const char *pCmd, const char *pArg)
{
	std::string str(pCmd);
	str += pArg;

	return SendPacket(str.c_str());
}

void OnkyoAVTCP::ReceiveSwitchMsg(const char *pData, int Len, bool muting, int ID)
{
	char *endp;
	int level = strtol(pData + 3, &endp, 16);
	int action = gswitch_sSetLevel;

	if (!IS_END_CHAR(*endp))
		return;

	if (muting)
		action = level ? gswitch_sOff : gswitch_sOn;
	else if (switch_types[ID].switchType == STYPE_OnOff)
		action = level ? gswitch_sOn : gswitch_sOff;


	std::vector<std::vector<std::string> > result;
	result = m_sql.safe_query("SELECT Name,nValue,sValue,Options,ID FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='%08X') AND (Unit == %d)",
				  m_HwdID, ID, 0);
	if (result.empty()) {
		EnsureSwitchDevice(ID, nullptr);
		result = m_sql.safe_query("SELECT Name,nValue,sValue,Options,ID FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='%08X') AND (Unit == %d)",
					  m_HwdID, ID, 0);
		if (result.empty())
			return;
	}

	if (switch_types[ID].subtype == sSwitchTypeSelector) {
		std::map<std::string, std::string> options = m_sql.BuildDeviceOptions(result[0][3]);
		std::map<std::string, std::string>::const_iterator itt = options.find("LevelActions");
		if (itt == options.end())
			return;
		std::string sOptions = itt->second;
		std::vector<std::string> strarray;
		boost::split(strarray, sOptions, boost::is_any_of("|"), boost::token_compress_off);
		std::vector<std::string>::iterator itt2;
		int i = 0;
		for (itt2 = strarray.begin(); itt2 != strarray.end(); ++itt2) {
			if (strtoul(itt2->c_str(), nullptr, 16) == (unsigned long)level)
				break;
			i += 10;
		}
		if (itt2 == strarray.end()) {
			// Add previously unknown value to a selector
			std::string str(reinterpret_cast<const char*>(pData + 3), Len - 4);
			std::string level_name = str;
			const struct selector_name *n = switch_types[ID].default_names;
			while (n && n->name) {
				if (n->val == level) {
					level_name = n->name;
					break;
				}
				n++;
			}
			options["LevelActions"] = options["LevelActions"] + "|" + str;
			options["LevelNames"] = options["LevelNames"] + "|" + level_name;
			m_sql.SetDeviceOptions(atoi(result[0][4].c_str()), options);
		}
		level = i;
	}
	if (!result.empty() && action == gswitch_sSetLevel) {
		//check if we have a change, if not do not update it
		int nvalue = atoi(result[0][1].c_str());
		if ((!level) && (nvalue == gswitch_sOff))
			return;
		if ((level && (nvalue != gswitch_sOff))) {
			//Check Level
			int slevel = atoi(result[0][2].c_str());
			if (slevel == level)
				return;
		}
	}

	_tGeneralSwitch gswitch;
	gswitch.subtype = switch_types[ID].subtype;
	gswitch.id = ID;
	gswitch.unitcode = 0;
	gswitch.cmnd = (uint8_t)action;
	gswitch.level = (uint8_t)level;
	gswitch.battery_level = 255;
	gswitch.rssi = 12;
	gswitch.seqnbr = 0;

	sDecodeRXMessage(this, (const unsigned char *)&gswitch, switch_types[ID].name, 255, m_Name.c_str());
}

void OnkyoAVTCP::EnsureSwitchDevice(int ID, const char *options)
{
	std::vector<std::vector<std::string> > result;
	std::string options_str;
	result = m_sql.safe_query("SELECT ID FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='%08X')", m_HwdID, ID);
	if (result.empty()) {
		if (!options && switch_types[ID].options) {
			options_str = m_sql.FormatDeviceOptions(m_sql.BuildDeviceOptions(switch_types[ID].options, false));
			options = options_str.c_str();
			_log.Log(LOG_ERROR, "Options from '%s': '%s'\n", switch_types[ID].options, options);
		}
		m_sql.safe_query(
				 "INSERT INTO DeviceStatus (HardwareID, DeviceID, Unit, Type, SubType, SwitchType, Used, SignalLevel, BatteryLevel, Name, nValue, sValue, CustomImage, Options) "
				 "VALUES (%d, '%08X', %d, %d, %d, %d, %d, 12, 255, '%q', 0, '%q', %d, '%q')",
				 m_HwdID, ID, 0, pTypeGeneralSwitch, switch_types[ID].subtype, switch_types[ID].switchType, 1, switch_types[ID].name, "", switch_types[ID].customImage, options?options:"");

	}
}


std::string OnkyoAVTCP::BuildSelectorOptions(const std::string & names, const std::string & ids)
{
	std::map<std::string, std::string> optionsMap;
	optionsMap.insert(std::pair<std::string, std::string>("LevelOffHidden", "true"));
	optionsMap.insert(std::pair<std::string, std::string>("SelectorStyle", "1"));
	optionsMap.insert(std::pair<std::string, std::string>("LevelNames", names));
	optionsMap.insert(std::pair<std::string, std::string>("LevelActions", ids));
	return m_sql.FormatDeviceOptions(optionsMap);
}

bool OnkyoAVTCP::ReceiveXML(const char *pData, int Len)
{
	TiXmlDocument doc;

	((char *)pData)[Len - 1] = 0;
	pData += 3;

	doc.Parse(pData);
	if (doc.Error()) {
		_log.Log(LOG_ERROR, "OnkyoAVTCP: Failed to parse NRIQSTN result: %s", doc.ErrorDesc());
		// XX: Fallback, add defaults anyway? */
		return false;
	}
	TiXmlElement *pResponseNode = doc.FirstChildElement("response");
	if (!pResponseNode) {
		_log.Log(LOG_ERROR, "No <response> element in NRIQSTN result");
		return false;
	}
	TiXmlElement *pDeviceNode = pResponseNode->FirstChildElement("device");
	if (!pDeviceNode) {
		_log.Log(LOG_ERROR, "No <device> element in NRIQSTN result");
		return false;
	}
	std::string InputNames[4], InputIDs[4];
	TiXmlElement *pSelectorlistNode = pDeviceNode->FirstChildElement("selectorlist");
	if (pSelectorlistNode) {
		TiXmlElement *pSelector;
		for ( pSelector = pSelectorlistNode->FirstChildElement(); pSelector;
		      pSelector = pSelector->NextSiblingElement() ) {
			const char *id = pSelector->Attribute("id");
			const char *name = pSelector->Attribute("name");
			const char *zone = pSelector->Attribute("zone");
			int value;
			if (!id || !name || !zone ||
			    pSelector->QueryIntAttribute("value", &value) != TIXML_SUCCESS ||
			    !value)
				continue;

			char *escaped_name = strdup(name);
			int nlen = strlen(name);
			while (nlen) {
				if (escaped_name[nlen - 1] == ' ' && escaped_name[nlen] == 0)
					escaped_name[nlen - 1] = 0;
				else if (escaped_name[nlen - 1] == '|')
					escaped_name[nlen - 1] = '!';
				nlen--;
			}
			if (!escaped_name[0])
				continue;

			for (int i = 0; i < 3; i++) {
				int zone_nr = strtoul(zone, nullptr, 16);
				if (zone_nr & (1 << i)) {
					if (InputNames[i].empty())
						InputNames[i] = "Off";
					InputNames[i] += '|';
					InputNames[i] += escaped_name;
					if (InputIDs[i].empty())
						InputIDs[i] = "Off";
					InputIDs[i] += '|';
					InputIDs[i] += id;
				}
			}
			free(escaped_name);
		}
	}
	TiXmlElement *pZonelistNode = pDeviceNode->FirstChildElement("zonelist");
	if (pZonelistNode) {
		TiXmlElement *pZone;
		for ( pZone = pZonelistNode->FirstChildElement(); pZone;
		      pZone = pZone->NextSiblingElement() ) {
			int id, value, volmax, volstep;
			const char *name = pZone->Attribute("name");

			if (!name)
				continue;

			if (pZone->QueryIntAttribute("value", &value) != TIXML_SUCCESS ||
			    pZone->QueryIntAttribute("id", &id) != TIXML_SUCCESS ||
			    pZone->QueryIntAttribute("volmax", &volmax) != TIXML_SUCCESS ||
			    pZone->QueryIntAttribute("volstep", &volstep) != TIXML_SUCCESS)
				continue;

			if (!value)
				continue;

			if (id < 1 || id > 4)
				continue;

			// Create the input selector immediately, with the known options.
			std::string options = BuildSelectorOptions(InputNames[id - 1], InputIDs[id - 1]);;
			EnsureSwitchDevice(ID_SLI + id - 1, options.c_str());

			// Send queries for it, and the power and volume for this zone.
			SendPacket(switch_types[ID_SLI + id - 1].iscpCmd, "QSTN");
			SendPacket(switch_types[ID_PWR + id - 1].iscpCmd, "QSTN");
			SendPacket(switch_types[ID_MVL + id - 1].iscpCmd, "QSTN");
		}
	}

	SendPacket("HDOQSTN");
	return true;
}

void OnkyoAVTCP::ReceiveMessage(const char *pData, int Len)
{
	while (Len && (pData[Len - 1] == '\r' || pData[Len - 1] == '\n'))
		Len--;

	if (Len < 4 || !IS_END_CHAR(pData[Len-1]))
		return;

	// Special-case the startup XML data
	if (!memcmp(pData, "NRI", 3)) {
		ReceiveXML(pData, Len);
		return;
	}
	_log.Log(LOG_NORM, "OnkyoAVTCP: Packet received: %d %.*s", Len, Len-1, pData);

	int i = 0;
	for (const auto &text : text_types)
	{
		if (!memcmp(pData, text.first, 3))
		{
			std::string str(reinterpret_cast<const char*>(pData + 3), Len - 4);
			SendTextSensor(1, i, 255, str, text.second);
			return;
		}
		++i;
	}

	i = 0;
	for (const auto &sw : switch_types)
	{
		if (!memcmp(pData, sw.iscpCmd, 3))
		{
			ReceiveSwitchMsg(pData, Len, false, i);
			return;
		}
		if (sw.iscpMute && !memcmp(pData, sw.iscpMute, 3))
		{
			ReceiveSwitchMsg(pData, Len, true, i);
			return;
		}
		++i;
	}
}

void OnkyoAVTCP::ParseData(const unsigned char *pData, int Len)
{
	if (m_pPartialPkt) {
		unsigned char *new_data = (unsigned char *)realloc(m_pPartialPkt, m_PPktLen + Len);
		if (!new_data) {
			free(m_pPartialPkt);
			m_pPartialPkt = nullptr;
			m_PPktLen = 0;
			_log.Log(LOG_ERROR, "OnkyoAVTCP: Failed to prepend previous data");
			// We'll attempt to resync
		} else {
			memcpy(new_data + m_PPktLen, pData, Len);
			m_pPartialPkt = new_data;
			Len += m_PPktLen;
			pData = new_data;
		}
	}
	while (Len >= 18) {
		const struct eiscp_packet *pkt = (const struct eiscp_packet *)pData;
		if (pkt->magic != htonl(0x49534350) || // "ISCP"
		    pkt->hdr_size != htonl(16) || pkt->version != 1) {
			Len--;
			pData++;
			continue;
		}
		int data_size = static_cast<int>(ntohl(pkt->data_size));
		if (Len < 16 + data_size)
			break;

		ReceiveMessage(pkt->message, data_size - 2);
		Len -= 16 + data_size;
		pData += 16 + data_size;
	}
	unsigned char *new_partial = nullptr;
	if (Len) {
		if (pData == m_pPartialPkt) {
			m_PPktLen = Len;
			return;
		}
		new_partial = (unsigned char *)malloc(Len);
		if (new_partial)
			memcpy(new_partial, pData, Len);
		else Len = 0;
	}
	free(m_pPartialPkt);
	m_PPktLen = Len;
	m_pPartialPkt = new_partial;
}

bool OnkyoAVTCP::CustomCommand(const uint64_t /*idx*/, const std::string &sCommand)
{
	return SendPacket(sCommand.c_str());
}

//Webserver helpers
namespace http {
	namespace server {
		void CWebServer::Cmd_OnkyoEiscpCommand(WebEmSession & session, const request& req, Json::Value &root)
		{
			if (session.rights != 2)
			{
				session.reply_status = reply::forbidden;
				return; //Only admin user allowed
			}

			std::string sIdx = request::findValue(&req, "idx");
			std::string sAction = request::findValue(&req, "action");
			if (sIdx.empty())
				return;
			//int idx = atoi(sIdx.c_str());

			std::vector<std::vector<std::string> > result;
			result = m_sql.safe_query("SELECT DS.SwitchType, DS.DeviceID, H.Type, H.ID FROM DeviceStatus DS, Hardware H WHERE (DS.ID=='%q') AND (DS.HardwareID == H.ID)", sIdx.c_str());

			root["status"] = "ERR";
			if (result.size() == 1)
			{
				_eHardwareTypes	hType = (_eHardwareTypes)atoi(result[0][2].c_str());
				switch (hType) {
				// We allow raw EISCP commands to be sent on *any* of the logical devices
				// associated with the hardware.
				case HTYPE_OnkyoAVTCP:
					CDomoticzHardwareBase *pBaseHardware = m_mainworker.GetHardwareByIDType(result[0][3], HTYPE_OnkyoAVTCP);
					if (pBaseHardware == nullptr)
						return;
					OnkyoAVTCP *pOnkyoAVTCP = reinterpret_cast<OnkyoAVTCP*>(pBaseHardware);

					pOnkyoAVTCP->SendPacket(sAction.c_str());
					root["status"] = "OK";
					root["title"] = "OnkyoEiscpCommand";
					break;
				}
			}
		}

	} // namespace server
} // namespace http
