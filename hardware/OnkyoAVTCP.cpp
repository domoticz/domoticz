#include "stdafx.h"
#include "OnkyoAVTCP.h"
#include "../main/Logger.h"
#include "../main/Helper.h"
#include "../main/SQLHelper.h"
#include <iostream>
#include "../main/localtime_r.h"
#include "../main/mainworker.h"
#include "../hardware/hardwaretypes.h"
#include "../json/json.h"
#include "../tinyxpath/tinyxml.h"

#include <sstream>


#define RETRY_DELAY 30

OnkyoAVTCP::OnkyoAVTCP(const int ID, const std::string &IPAddress, const unsigned short usIPPort) :
m_szIPAddress(IPAddress)
{
	m_HwdID=ID;
	m_bDoRestart=false;
	m_stoprequested=false;
	m_usIPPort=usIPPort;
	m_retrycntr = RETRY_DELAY;
	m_pPartialPkt = NULL;
	m_PPktLen = 0;
}

OnkyoAVTCP::~OnkyoAVTCP(void)
{
	free(m_pPartialPkt);
}

bool OnkyoAVTCP::StartHardware()
{
	m_stoprequested=false;
	m_bDoRestart=false;

	//force connect the next first time
	m_retrycntr=RETRY_DELAY;
	m_bIsStarted=true;

	//Start worker thread
	m_thread = boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&OnkyoAVTCP::Do_Work, this)));
	return (m_thread!=NULL);
}

bool OnkyoAVTCP::StopHardware()
{
	m_stoprequested=true;
	try {
		if (m_thread)
		{
			m_thread->join();
		}
	}
	catch (...)
	{
		//Don't throw from a Stop command
	}
	if (isConnected())
	{
		try {
			disconnect();
		} catch(...)
		{
			//Don't throw from a Stop command
		}
	}

	m_bIsStarted=false;
	return true;
}

void OnkyoAVTCP::OnConnect()
{
	_log.Log(LOG_STATUS,"OnkyoAVTCP: connected to: %s:%ld", m_szIPAddress.c_str(), m_usIPPort);
	m_bDoRestart=false;
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
	bool bFirstTime=true;
	int sec_counter = 0;
	while (!m_stoprequested)
	{
		sleep_seconds(1);
		sec_counter++;

		if (sec_counter  % 12 == 0) {
			m_LastHeartbeat=mytime(NULL);
		}

		if (bFirstTime)
		{
			bFirstTime=false;
			connect(m_szIPAddress,m_usIPPort);
		}
		else
		{
			if ((m_bDoRestart) && (sec_counter % 30 == 0))
			{
				connect(m_szIPAddress,m_usIPPort);
			}
			update();
		}
	}
	_log.Log(LOG_STATUS,"OnkyoAVTCP: TCP/IP Worker stopped...");
}

void OnkyoAVTCP::OnData(const unsigned char *pData, size_t length)
{
	boost::lock_guard<boost::mutex> l(readQueueMutex);
	ParseData(pData,length);
}

void OnkyoAVTCP::OnError(const std::exception e)
{
	_log.Log(LOG_ERROR,"OnkyoAVTCP: Error: %s",e.what());
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
		_log.Log(LOG_ERROR, "OnkyoAVTCP: Can not connect to: %s:%ld", m_szIPAddress.c_str(), m_usIPPort);
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

struct eiscp_packet {
	uint32_t magic;
	uint32_t hdr_size;
	uint32_t data_size;
	uint8_t version;
	uint8_t reserved[3];
	char start;
	char dest;
	char message[2];
};

enum DevNr {
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

#define ARRAY_SIZE(x) ((ssize_t)(sizeof(x) / sizeof(x[0])))
static struct {
	const char *iscpCmd;
	const char *iscpMute;
	const char *name;
	int switchType;
	int subtype;
	int customImage;
	const char *options;
} switch_types[] = {
	[ID_MVL] = { "MVL", "AMT", "Master volume", STYPE_Dimmer, sSwitchGeneralSwitch, 8, NULL },
	[ID_ZVL] = { "ZVL", "ZMT", "Zone 2 volume", STYPE_Dimmer, sSwitchGeneralSwitch, 8, NULL },
	[ID_VL3] = { "VL3", "MT3", "Zone 3 volume", STYPE_Dimmer, sSwitchGeneralSwitch, 8, NULL },
	[ID_VL4] = { "VL4", "MT4", "Zone 4 volume", STYPE_Dimmer, sSwitchGeneralSwitch, 8, NULL },
	[ID_PWR] = { "PWR", NULL, "Master power", STYPE_OnOff, sSwitchGeneralSwitch, 5, NULL },
	[ID_ZPW] = { "ZPW", NULL, "Zone 2 power", STYPE_OnOff, sSwitchGeneralSwitch, 5, NULL },
	[ID_PW3] = { "PW3", NULL, "Zone 3 power", STYPE_OnOff, sSwitchGeneralSwitch, 5, NULL },
	[ID_PW4] = { "PW4", NULL, "Zone 4 power", STYPE_OnOff, sSwitchGeneralSwitch, 5, NULL },
	[ID_SLI] = { "SLI", NULL, "Master selector", STYPE_Selector, sSwitchTypeSelector, 5, NULL },
	[ID_SLZ] = { "SLZ", NULL, "Zone 2 selector", STYPE_Selector, sSwitchTypeSelector, 5, NULL },
	[ID_SL3] = { "SL2", NULL, "Zone 3 selector", STYPE_Selector, sSwitchTypeSelector, 5, NULL },
	[ID_SL4] = { "SL3", NULL, "Zone 4 selector", STYPE_Selector, sSwitchTypeSelector, 5, NULL },
	[ID_HDO] = { "HDO", NULL, "HDMI Output", STYPE_Selector, sSwitchTypeSelector, 5, "SelectorStyle:0;LevelNames:Off|Main|Sub|Main+Sub;LevelOffHidden:true;LevelActions:00|01|02|03" },
};

#define IS_END_CHAR(x) ((x) == 0x1a)


bool OnkyoAVTCP::WriteToHardware(const char *pdata, const unsigned char length)
{
	if (!mIsConnected || !pdata)
	{
		return false;
	}
	const tRBUF *pCmd = reinterpret_cast<const tRBUF *>(pdata);
	unsigned char packettype = pCmd->ICMND.packettype;
	std::string message = "";

	if (packettype == pTypeGeneralSwitch) {
		_tGeneralSwitch *xcmd = (_tGeneralSwitch*)pdata;
		int ID = xcmd->id;
		int level = xcmd->level;
		char buf[9];
		if (ID >= ARRAY_SIZE(switch_types))
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
				else
					snprintf(buf, 6, "%s%s", switch_types[ID].iscpCmd, val.c_str());
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
	if (!mIsConnected || !pdata)
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
	result = m_sql.safe_query("SELECT Name,nValue,sValue,Options FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='%08X') AND (Unit == %d)",
				  m_HwdID, ID, 0);
	if (switch_types[ID].subtype == sSwitchTypeSelector) {
		if (result.empty())
			return;
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
			if (strtoul(itt2->c_str(), NULL, 16) == (unsigned long)level)
				break;
			i += 10;
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
	gswitch.cmnd = action;
	gswitch.level = level;
	gswitch.battery_level = 255;
	gswitch.rssi = 12;
	gswitch.seqnbr = 0;

	sDecodeRXMessage(this, (const unsigned char *)&gswitch, switch_types[ID].name, 255);
}

void OnkyoAVTCP::EnsureDevice(int ID, const char *options)
{
	std::vector<std::vector<std::string> > result;
	std::string options_str;
	result = m_sql.safe_query("SELECT ID FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='%08X')", m_HwdID, ID);
	if (result.size() == 0) {
		if (!options && switch_types[ID].options) {
			options_str = m_sql.FormatDeviceOptions(m_sql.BuildDeviceOptions(switch_types[ID].options, false));
			options = options_str.c_str();
			_log.Log(LOG_ERROR, "Options from '%s': '%s'\n", switch_types[ID].options, options);
		}
		m_sql.safe_query(
				 "INSERT INTO DeviceStatus (HardwareID, DeviceID, Unit, Type, SubType, SwitchType, Used, SignalLevel, BatteryLevel, Name, nValue, sValue, CustomImage, Options) "
				 "VALUES (%d, '%08X', %d, %d, %d, %d, %d, 12, 255, '%q', 0, '%q', %d, '%q')",
				 m_HwdID, ID, 0, pTypeGeneralSwitch, switch_types[ID].subtype, switch_types[ID].switchType, 1, switch_types[ID].name, "", switch_types[ID].customImage, options?:"");

	}

	char query[8];
	sprintf(query, "%sQSTN", switch_types[ID].iscpCmd);
	SendPacket(query);
}


std::string OnkyoAVTCP::BuildSelectorOptions(std::string & names, std::string & ids)
{
	std::map<std::string, std::string> optionsMap;

	std::vector<std::string>::iterator n_itt, id_itt;
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

			int nlen = strlen(name);
			while (nlen && name[nlen - 1] == ' ')
				nlen--;
			if (!nlen)
				continue;

			char *escaped_name = strndup(name, nlen);
			while (nlen) {
				if (escaped_name[nlen - 1] == '|')
					 escaped_name[nlen - 1] = '!';
				nlen--;
			}
			for (int i = 0; i < 3; i++) {
				int zone_nr = strtoul(zone, NULL, 16);
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
			EnsureDevice(ID_PWR + id - 1);
			EnsureDevice(ID_MVL + id - 1);
			std::string options = BuildSelectorOptions(InputNames[id - 1], InputIDs[id - 1]);;
			EnsureDevice(ID_SLI + id - 1, options.c_str());
		}
	}
	EnsureDevice(ID_HDO);
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

	int i;
	for (i = 0; i < ARRAY_SIZE(switch_types); i++) {
	  if (!memcmp(pData, switch_types[i].iscpCmd, 3)) {
		  ReceiveSwitchMsg(pData, Len, false, i);
		  break;
	  } else if (switch_types[i].iscpMute && !memcmp(pData, switch_types[i].iscpMute, 3)) {
		  ReceiveSwitchMsg(pData, Len, true, i);
		  break;
	  }
	}
}

void OnkyoAVTCP::ParseData(const unsigned char *pData, int Len)
{
	if (m_pPartialPkt) {
		unsigned char *new_data = (unsigned char *)realloc(m_pPartialPkt, m_PPktLen + Len);
		if (!new_data) {
			free(m_pPartialPkt);
			m_pPartialPkt = NULL;
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
		_log.Log(LOG_NORM, "ISCP at len %d size %d", Len, data_size);
		if (Len < 16 + data_size)
			break;

		ReceiveMessage(pkt->message, data_size - 2);
		Len -= 16 + data_size;
		pData += 16 + data_size;
	}
	unsigned char *new_partial = NULL;
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


