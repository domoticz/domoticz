#include "stdafx.h"
#include "MochadTCP.h"
#include "../main/Logger.h"
#include "../main/Helper.h"
#include "../main/localtime_r.h"
#include "../main/RFXtrx.h"
#include <iostream>

#include "hardwaretypes.h"
#include "../main/Logger.h"

#define RETRY_DELAY 30

enum _eMochadMatchType {
	ID=0,
	STD,
	LINE17,
	LINE18,
	EXCLMARK
};

enum _eMochadType {
	MOCHAD_STATUS=0,
	MOCHAD_UNIT,
	MOCHAD_ACTION,
	MOCHAD_RFSEC
};

typedef struct {
	_eMochadMatchType matchtype;
	_eMochadType type;
	const char* key;
	int start;
	int width;
} MochadMatch;

static MochadMatch matchlist[] = {
	{STD,	MOCHAD_STATUS,	"House ",	6, 255},
	{STD,	MOCHAD_UNIT,	"Tx PL HouseUnit: ",	17, 9},
	{STD,	MOCHAD_UNIT,	"Rx PL HouseUnit: ",	17, 9},
	{STD,	MOCHAD_UNIT,	"Tx RF HouseUnit: ",	17, 9},
	{STD,	MOCHAD_UNIT,	"Rx RF HouseUnit: ",	17, 9},
	{STD,	MOCHAD_ACTION,	"Tx PL House: ",	13, 9},
	{STD,	MOCHAD_ACTION,	"Rx PL House: ",	13, 9},
	{STD,	MOCHAD_ACTION,	"Tx RF House: ",	13, 9},
	{STD,	MOCHAD_ACTION,	"Rx RF House: ",	13, 9},
	{STD,	MOCHAD_RFSEC,	"Rx RFSEC Addr: ",	15, 8 }
};

//end

MochadTCP::MochadTCP(const int ID, const std::string &IPAddress, const unsigned short usIPPort):
m_szIPAddress(IPAddress)
{
	m_HwdID=ID;
	m_stoprequested=false;
	m_bDoRestart = false;
	m_usIPPort=usIPPort;
	m_linecount=0;
	m_exclmarkfound=0;
	m_bufferpos=0;

	memset(&m_mochadbuffer,0,sizeof(m_mochadbuffer));
	memset(&m_mochadsec, 0, sizeof(m_mochadsec));
	memset(&m_mochad,0,sizeof(m_mochad));

	m_mochad.LIGHTING1.packetlength = sizeof(m_mochad) - 1;
	m_mochad.LIGHTING1.packettype = pTypeLighting1;
	m_mochad.LIGHTING1.subtype = sTypeX10;
	m_mochad.LIGHTING1.housecode = 0;
	m_mochad.LIGHTING1.unitcode = 0;
	m_mochad.LIGHTING1.cmnd = 0;

	m_mochadsec.SECURITY1.packetlength = sizeof(m_mochadsec) - 1;
	m_mochadsec.SECURITY1.packettype = pTypeSecurity1;
	m_mochadsec.SECURITY1.subtype = 0;
	m_mochadsec.SECURITY1.id1 = 0;
	m_mochadsec.SECURITY1.id2 = 0;
	m_mochadsec.SECURITY1.id3 = 0;
	m_mochadsec.SECURITY1.status = sStatusNormal;
	m_mochadsec.SECURITY1.rssi = 12;
	m_mochadsec.SECURITY1.battery_level = 0;

	memset(&selected, 0, sizeof(selected));
	currentHouse=0;
	currentUnit=0;
}

MochadTCP::~MochadTCP(void)
{
}

bool MochadTCP::StartHardware()
{
	m_stoprequested=false;
	m_bDoRestart = false;

	//force connect the next first time
//	m_retrycntr=RETRY_DELAY;
//	m_bIsStarted=true;

	//Start worker thread
	m_thread = std::make_shared<std::thread>(&MochadTCP::Do_Work, this);
	return (m_thread != nullptr);
}

bool MochadTCP::StopHardware()
{
	m_stoprequested = true;
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


void MochadTCP::OnConnect()
{
	_log.Log(LOG_STATUS, "Mochad: connected to: %s:%d", m_szIPAddress.c_str(), m_usIPPort);
	m_bIsStarted = true;
	m_bDoRestart = false;

	sOnConnected(this);
}

void MochadTCP::OnDisconnect()
{
	_log.Log(LOG_STATUS, "Mochad: disconnected");
	m_bDoRestart = true;
}

void MochadTCP::OnData(const unsigned char *pData, size_t length)
{
	std::lock_guard<std::mutex> l(readQueueMutex);
	ParseData(pData, length);
}

void MochadTCP::Do_Work()
{
	bool bFirstTime = true;

	while (!m_stoprequested)
	{

		time_t atime = mytime(NULL);
		struct tm ltime;
		localtime_r(&atime, &ltime);


		if (ltime.tm_sec % 12 == 0) {
			mytime(&m_LastHeartbeat);
		}
		if (bFirstTime)
		{
			bFirstTime = false;
			if (!mIsConnected)
			{
				m_rxbufferpos = 0;
				connect(m_szIPAddress, m_usIPPort);
			}
		}
		else
		{
			if ((m_bDoRestart) && (ltime.tm_sec % 30 == 0))
			{
				_log.Log(LOG_STATUS, "Mochad: trying to connect to %s:%d", m_szIPAddress.c_str(), m_usIPPort);
				connect(m_szIPAddress, m_usIPPort);
			}
			sleep_milliseconds(40);
			update();
		}
	}
	_log.Log(LOG_STATUS,"Mochad: TCP/IP Worker stopped...");
}

void MochadTCP::OnError(const std::exception e)
{
	_log.Log(LOG_ERROR, "Mochad: Error: %s", e.what());
}

void MochadTCP::OnError(const boost::system::error_code& error)
{
	if (
		(error == boost::asio::error::address_in_use) ||
		(error == boost::asio::error::connection_refused) ||
		(error == boost::asio::error::access_denied) ||
		(error == boost::asio::error::host_unreachable) ||
		(error == boost::asio::error::timed_out)
		)
	{
		_log.Log(LOG_ERROR, "Mochad: Can not connect to: %s:%d", m_szIPAddress.c_str(), m_usIPPort);
	}
	else if (
		(error == boost::asio::error::eof) ||
		(error == boost::asio::error::connection_reset)
		)
	{
		_log.Log(LOG_STATUS, "Mochad: Connection reset!");
	}
	else
		_log.Log(LOG_ERROR, "Mochad: %s", error.message().c_str());
}

bool MochadTCP::WriteToHardware(const char *pdata, const unsigned char length)
{
	//RBUF *m_mochad = (RBUF *)pdata;
	if (!mIsConnected)
		return false;
	if (pdata[1] == pTypeInterfaceControl && pdata[2] == sTypeInterfaceCommand && pdata[4] == cmdSTATUS) {
		sprintf (s_buffer,"ST\n");
	} else if (pdata[1] == pTypeLighting1 && pdata[2] == sTypeX10 && pdata[6] == light1_sOn) {
		sprintf (s_buffer,"RF %c%d on\n",(char)(pdata[4]), pdata[5]);
	} else if (pdata[1] == pTypeLighting1 && pdata[2] == sTypeX10 && pdata[6] == light1_sOff) {
		sprintf (s_buffer,"RF %c%d off\n",(char)(pdata[4]), pdata[5]);
	} else {
//			case light1_sDim:
//			case light1_sBright:
//			case light1_sAllOn:
//			case light1_sAllOff:
		_log.Log(LOG_STATUS, "Mochad: Unknown command %d:%d:%d", pdata[1],pdata[2],pdata[6]);
		return false;
	}
//	_log.Log(LOG_STATUS, "Mochad: send '%s'", s_buffer);
	write((const unsigned char *)s_buffer, strlen(s_buffer));
	return true;
}

void MochadTCP::MatchLine()
{
	if ((strlen((const char*)&m_mochadbuffer)<1)||(m_mochadbuffer[0]==0x0a))
		return; //null value (startup)
	uint8_t i;
	int j,k;
	uint8_t found=0;
	MochadMatch t;
	char value[20]="";
	std::string vString;



	for(i=0;(i<sizeof(matchlist)/sizeof(MochadMatch))&(!found);i++)
	{
		t = matchlist[i];
		switch(t.matchtype)
		{
		case ID:
			if(strncmp(t.key, (const char*)&m_mochadbuffer, strlen(t.key)) == 0) {
				m_linecount=1;
				found=1;
			}
			else
				continue;
			break;
		case STD:
			if(strncmp(t.key, (const char*)&m_mochadbuffer, strlen(t.key)) == 0) {
				found=1;
			}
			else
				continue;
			break;
		case LINE17:
			if(strncmp(t.key, (const char*)&m_mochadbuffer, strlen(t.key)) == 0) {
				m_linecount = 17;
				found=1;
			}
			else
				continue;
			break;
		case LINE18:
			if((m_linecount == 18)&&(strncmp(t.key, (const char*)&m_mochadbuffer, strlen(t.key)) == 0)) {
				found=1;
			}
			break;
		case EXCLMARK:
			if(strncmp(t.key, (const char*)&m_mochadbuffer, strlen(t.key)) == 0) {
				m_exclmarkfound=1;
				found=1;
			}
			else
				continue;
			break;
		default:
			continue;
		} //switch
	}
	if(!found)
		goto onError;

	switch (t.type)
	{
	case MOCHAD_STATUS:
		j = t.start;
		if (!('A'<=  m_mochadbuffer[j] &&  m_mochadbuffer[j] <='Z'))
			goto onError;
		m_mochad.LIGHTING1.housecode = m_mochadbuffer[j++];
		if (!(':'==  m_mochadbuffer[j++])) goto onError;
		if (!(' '==  m_mochadbuffer[j++])) goto onError;
		while ('1' <= m_mochadbuffer[j] && m_mochadbuffer[j] <= '9') {
			m_mochad.LIGHTING1.unitcode = m_mochadbuffer[j++] - '0';
			if ('0' <= m_mochadbuffer[j] && m_mochadbuffer[j] <= '9') {
				m_mochad.LIGHTING1.unitcode = m_mochad.LIGHTING1.unitcode*10 + m_mochadbuffer[j++] - '0';
			}
			if (!('='==  m_mochadbuffer[j++]))
				return;
			if (!('0' <= m_mochadbuffer[j] && m_mochadbuffer[j] <= '1')) goto onError;
			m_mochad.LIGHTING1.cmnd = m_mochadbuffer[j++] - '0';
			sDecodeRXMessage(this, (const unsigned char *)&m_mochad, NULL, 255);
			if (!(','==  m_mochadbuffer[j++])) return;
		}
		break;
	case MOCHAD_UNIT:
		j = t.start;
		if (!('A'<=  m_mochadbuffer[j] &&  m_mochadbuffer[j] <='Z')) goto onError;
		currentHouse = m_mochadbuffer[j++]-'A';
		if (!('0' <= m_mochadbuffer[j] && m_mochadbuffer[j] <= '9')) goto onError;
		currentUnit = m_mochadbuffer[j++] - '0';
		if (('0' <= m_mochadbuffer[j] && m_mochadbuffer[j] <= '9'))
			currentUnit = currentUnit*10 + m_mochadbuffer[j++] - '0';
		selected[currentHouse][currentUnit] = 1;
		if (!(' '==  m_mochadbuffer[j++])) return;
		goto checkFunc;
		break;
	case MOCHAD_ACTION:
		j = t.start;
		if (!('A'<=  m_mochadbuffer[j] &&  m_mochadbuffer[j] <='Z'))
			goto onError;
		currentHouse = m_mochadbuffer[j++]-'A';
		if (!(' '==  m_mochadbuffer[j++])) goto onError;
checkFunc:
		if (!('F'==  m_mochadbuffer[j++])) goto onError;
		if (!('u'==  m_mochadbuffer[j++])) goto onError;
		if (!('n'==  m_mochadbuffer[j++])) goto onError;
		if (!('c'==  m_mochadbuffer[j++])) goto onError;
		if (!(':'==  m_mochadbuffer[j++])) goto onError;
		if (!(' '==  m_mochadbuffer[j++])) goto onError;
		if (!('O'==  m_mochadbuffer[j++])) goto onError;
		if ('f'==  m_mochadbuffer[j]) m_mochad.LIGHTING1.cmnd = 0;
		else
		if ('n'==  m_mochadbuffer[j]) m_mochad.LIGHTING1.cmnd = 1;
		else goto onError;
		for (k=1;k<=16;k++) {
			if (selected[currentHouse][k] >0) {
				m_mochad.LIGHTING1.housecode = currentHouse+'A';
				m_mochad.LIGHTING1.unitcode = k;
				sDecodeRXMessage(this, (const unsigned char *)&m_mochad, NULL, 255);
				selected[currentHouse][k] = 0;
			}
		}
		break;
	case MOCHAD_RFSEC:
		j = t.start;
		char *pchar;
		char tempRFSECbuf[50];

		if (strstr((const char*)&m_mochadbuffer[j], "DS10A"))
		{
			m_mochadsec.SECURITY1.subtype = sTypeSecX10;
			setSecID(&m_mochadbuffer[t.start]);
			m_mochadsec.SECURITY1.battery_level = 0x0f;

			// parse sensor conditions, e.g. "Contact_alert_min_DS10A" or "'Contact_normal_max_low_DS10A"
			strcpy(tempRFSECbuf, (const char *)&m_mochadbuffer[t.start + t.width + 7]);
			pchar = strtok(tempRFSECbuf, " _");
			while (pchar != NULL)
			{
				if (strcmp(pchar, "alert") == 0)
					m_mochadsec.SECURITY1.status = sStatusAlarm;
				else if (strcmp(pchar, "normal") == 0)
					m_mochadsec.SECURITY1.status = sStatusNormal;
				else if (strcmp(pchar, "max") == 0)
				{
					if (m_mochadsec.SECURITY1.status == sStatusAlarm)
						m_mochadsec.SECURITY1.status = sStatusAlarmDelayed;
					else if (m_mochadsec.SECURITY1.status == sStatusNormal)
						m_mochadsec.SECURITY1.status = sStatusNormalDelayed;
				}
				else if (strcmp(pchar, "low") == 0)
					m_mochadsec.SECURITY1.battery_level = 1;
				pchar = strtok(NULL, " _");
			}
			m_mochadsec.SECURITY1.rssi = 12; // signal strength ?? 12 = no signal strength
		}
		else if (strstr((const char *)&m_mochadbuffer[j], "KR10A"))
		{
			m_mochadsec.SECURITY1.subtype = sTypeSecX10R;
			setSecID(&m_mochadbuffer[t.start]);
			m_mochadsec.SECURITY1.battery_level = 0x0f;

			// parse remote conditions, e.g. "Panic_KR10A" "Lights_On_KR10A" "Lights_Off_KR10A" "Disarm_KR10A" "Arm_KR10A"
			strcpy(tempRFSECbuf, (const char *)&m_mochadbuffer[t.start + t.width + 7]);
			pchar = strtok(tempRFSECbuf, " _");
			while (pchar != NULL)
			{
				if (strcmp(pchar, "Panic") == 0)
					m_mochadsec.SECURITY1.status = sStatusPanic;
				else if (strcmp(pchar, "Disarm") == 0)
					m_mochadsec.SECURITY1.status = sStatusDisarm;
				else if (strcmp(pchar, "Arm") == 0)
					m_mochadsec.SECURITY1.status = sStatusArmAway;
				else if (strcmp(pchar, "On") == 0)
					m_mochadsec.SECURITY1.status = sStatusLightOn;
				else if (strcmp(pchar, "Off") == 0)
					m_mochadsec.SECURITY1.status = sStatusLightOff;
				pchar = strtok(NULL, " _");
			}
			m_mochadsec.SECURITY1.rssi = 12;
		}
		else if (strstr((const char *)&m_mochadbuffer[j], "MS10A"))
		{
			m_mochadsec.SECURITY1.subtype = sTypeSecX10M;
			setSecID(&m_mochadbuffer[t.start]);
			m_mochadsec.SECURITY1.battery_level = 0x0f;

			// parse remote conditions, "Motion_alert_MS10A" and "Motion_normal_MS10A"
			strcpy(tempRFSECbuf, (const char *)&m_mochadbuffer[t.start + t.width + 7]);
			pchar = strtok(tempRFSECbuf, " _");
			while (pchar != NULL)
			{
				if (strcmp(pchar, "alert") == 0)
					m_mochadsec.SECURITY1.status = sStatusMotion;
				else if (strcmp(pchar, "normal") == 0)
					m_mochadsec.SECURITY1.status = sStatusNoMotion;
				else if (strcmp(pchar, "low") == 0)
					m_mochadsec.SECURITY1.battery_level = 1;
				pchar = strtok(NULL, " _");
			}
			m_mochadsec.SECURITY1.rssi = 12;
		}
		else
			goto onError;

		sDecodeRXMessage(this, (const unsigned char *)&m_mochadsec, NULL, 255);
		break;
	}
	return;
onError:
	_log.Log(LOG_ERROR, "Mochad: Cannot decode '%s'", m_mochadbuffer);

}

void MochadTCP::ParseData(const unsigned char *pData, int Len)
{
	int ii=0;
	while (ii<Len)
	{
		const unsigned char c = pData[ii];
		if(c == 0x0d)
		{
			ii++;
			continue;
		}

		m_mochadbuffer[m_bufferpos] = c;
		if(c == 0x0a || m_bufferpos == sizeof(m_mochadbuffer) - 1)
		{
			// discard newline, close string, parse line and clear it.
			if(m_bufferpos > 0) m_mochadbuffer[m_bufferpos] = 0;
			m_linecount++;
			if (strlen((const char *)m_mochadbuffer) > 14) {
				int i = 0;
				while (m_mochadbuffer[i+15] != 0) {
					m_mochadbuffer[i] = m_mochadbuffer[i+15];
					i++;
				}
				m_mochadbuffer[i] = 0;
//				_log.Log(LOG_STATUS, "Mochad: recv '%s'", m_mochadbuffer);
				MatchLine();
			}
			m_bufferpos = 0;
		}
		else
		{
			m_bufferpos++;
		}
		ii++;
	}
}

void MochadTCP::setSecID(unsigned char *p)
{
	int j = 0;
	m_mochadsec.SECURITY1.id1 = (hex2bin(p[j++]) << 4);
	m_mochadsec.SECURITY1.id1 |= hex2bin(p[j++]);
	j++; // skip the ":"
	m_mochadsec.SECURITY1.id2 = (hex2bin(p[j++]) << 4);
	m_mochadsec.SECURITY1.id2 |= hex2bin(p[j++]);
	j++; // skip the ":"
	m_mochadsec.SECURITY1.id3 = (hex2bin(p[j++]) << 4);
	m_mochadsec.SECURITY1.id3 |= hex2bin(p[j]);
}

unsigned char MochadTCP::hex2bin(char h)
{
	if (h >= '0' && h <= '9')
		return h - '0';
	if (h >= 'A' && h <= 'F')
		return h - 'A' + 10;
	// handle lower-case hex letter
	return h - 'a' + 10;
}
