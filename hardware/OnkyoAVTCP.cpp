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

	SendPacket("PWRQSTN");
	SendPacket("MVLQSTN");
	SendPacket("ZPWQSTN");
	SendPacket("ZVLQSTN");
	SendPacket("SLIQSTN");
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
#define ARRAY_SIZE(x) (sizeof(x) / sizeof(x[0]))
static struct {
	const char *iscpCmd;
	const char *iscpMute;
	const char *name;
	int switchType;
	int subtype;
	int customImage;
	const char *options;
} switch_types[] = {
	{ "MVL", "AMT", "Master volume", STYPE_Dimmer, sSwitchGeneralSwitch, 8, NULL },
	{ "ZVL", "ZMT", "Zone 2 volume", STYPE_Dimmer, sSwitchGeneralSwitch, 8, NULL },
//	{ "SLI", NULL, "Source selector", STYPE_Selector, sSwitchTypeSelector, 5, "SelectorStyle:0;LevelNames:Off|BD|Sky+HD|Chromecast" },
	{ "PWR", NULL, "Master power", STYPE_OnOff, sSwitchGeneralSwitch, 5, NULL },
	{ "ZPW", NULL, "Zone 2 power", STYPE_OnOff, sSwitchGeneralSwitch, 5, NULL }
};

#define IS_END_CHAR(x) ((x) == 0x1a || (x) == 0x0d || (x) == 0x0a)


bool OnkyoAVTCP::WriteToHardware(const char *pdata, const unsigned char length)
{
	if (!mIsConnected || !pdata)
	{
		return false;
	}
	const tRBUF *pCmd = reinterpret_cast<const tRBUF *>(pdata);
	unsigned char packettype = pCmd->ICMND.packettype;
	unsigned char subtype = pCmd->ICMND.subtype;
	bool result = true;
	std::string message = "";
	bool isctrl = false;

	if (packettype == pTypeGeneralSwitch) {
		_tGeneralSwitch *xcmd = (_tGeneralSwitch*)pdata;
		int ID = xcmd->id;
		int level = xcmd->level;
		char buf[9];
		if (ID >= ARRAY_SIZE(switch_types))
			return false;
		switch(xcmd->cmnd) {
		case gswitch_sSetLevel:
			if (switch_types[ID].subtype == sSwitchTypeSelector)
				level /= 10;
			snprintf(buf, 6, "%s%02X", switch_types[ID].iscpCmd, level);
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
	result = m_sql.safe_query("SELECT Name,nValue,sValue FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='%08x') AND (Unit == %d) AND (Type==%d) AND (Subtype==%d)",
							  m_HwdID, ID, 0, int(pTypeGeneralSwitch), int(sSwitchGeneralSwitch));
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
	if (switch_types[ID].subtype == sSwitchTypeSelector)
		level *= 10;
	gswitch.id = ID;
	gswitch.unitcode = 0;
	gswitch.cmnd = action;
	gswitch.level = level;
	gswitch.battery_level = 255;
	gswitch.rssi = 12;
	gswitch.seqnbr = 0;

	if (result.empty()) {
		m_mainworker.PushAndWaitRxMessage(this, (const unsigned char *)&gswitch, switch_types[ID].name, 255);
		m_sql.safe_query("UPDATE DeviceStatus SET SwitchType=%d, CustomImage=%d WHERE(HardwareID == %d) AND (DeviceID == '%08x')", switch_types[ID].switchType, switch_types[ID].customImage, m_HwdID, ID);
		if (switch_types[ID].options) {
			result = m_sql.safe_query("SELECT ID FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='%08x') AND (Type==%d) ", m_HwdID, ID, pTypeGeneralSwitch);
			if (result.size() > 0) {
				std::string Idx = result[0][0];
				m_sql.SetDeviceOptions(atoi(Idx.c_str()), m_sql.BuildDeviceOptions(switch_types[ID].options, false));
			}
		}
	} else
		sDecodeRXMessage(this, (const unsigned char *)&gswitch, switch_types[ID].name, 255);
}

void OnkyoAVTCP::ReceiveMessage(const char *pData, int Len)
{
	if (Len < 3 || !IS_END_CHAR(pData[Len-1]))
	  return;

	_log.Log(LOG_NORM, "OnkyoAVTCP: Packet received: %d %.*s", Len, Len-2, pData);

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
//		_log.Log(LOG_NORM, "ISCP at len %d size %d", Len, data_size);
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


