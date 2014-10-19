#include "stdafx.h"
#include "MochadTCP.h"
#include "../main/Logger.h"
#include "../main/Helper.h"
#include "../main/localtime_r.h"
#include "../main/RFXtrx.h"
#include <iostream>

#include "hardwaretypes.h"
#include "../main/localtime_r.h"
#include "../main/Logger.h"

#define RETRY_DELAY 30

typedef enum { 
	ID=0, 
	STD, 
	LINE17, 
	LINE18, 
	EXCLMARK 
} MatchType;

typedef enum {
	MOCHAD_STATUS=0,
	MOCHAD_UNIT,
	MOCHAD_ACTION
} MochadType;

typedef struct _tMatch {
	MatchType matchtype;
	MochadType type;
	const char* key;
	int start;
	int width;
} Match;

static Match matchlist[] = {
	{STD,	MOCHAD_STATUS,	"House ",	6, 255},
	{STD,	MOCHAD_UNIT,	"Tx PL HouseUnit: ",	17, 9},
	{STD,	MOCHAD_UNIT,	"Rx PL HouseUnit: ",	17, 9},
	{STD,	MOCHAD_UNIT,	"Tx RF HouseUnit: ",	17, 9},
	{STD,	MOCHAD_UNIT,	"Rx RF HouseUnit: ",	17, 9},
	{STD,	MOCHAD_ACTION,	"Tx PL House: ",	13, 9},
	{STD,	MOCHAD_ACTION,	"Rx PL House: ",	13, 9},
	{STD,	MOCHAD_ACTION,	"Tx RF House: ",	13, 9},
	{STD,	MOCHAD_ACTION,	"Rx RF House: ",	13, 9}
};

static int selected[17][17];
//end


MochadTCP::MochadTCP(const int ID, const std::string IPAddress, const unsigned short usIPPort)
{
	m_HwdID=ID;
#if defined WIN32
	int ret;
	//Init winsock
	WSADATA data;
	WORD version; 

	version = (MAKEWORD(2, 2)); 
	ret = WSAStartup(version, &data); 
	if (ret != 0) 
	{  
		ret = WSAGetLastError(); 

		if (ret == WSANOTINITIALISED) 
		{  
			_log.Log(LOG_ERROR,"Mochad: Winsock could not be initialized!");
		}
	}
#endif
	m_socket=INVALID_SOCKET;
	m_stoprequested=false;
	m_szIPAddress=IPAddress;
	m_usIPPort=usIPPort;
	m_linecount=0;
	m_exclmarkfound=0;
	m_bufferpos=0;

	memset(&m_buffer,0,sizeof(m_buffer));
	memset(&m_mochad,0,sizeof(m_mochad));

	m_mochad.LIGHTING1.packetlength = sizeof(m_mochad) - 1;
	m_mochad.LIGHTING1.packettype = pTypeLighting1;
	m_mochad.LIGHTING1.subtype = sTypeX10;
	m_mochad.LIGHTING1.housecode = 0;
	m_mochad.LIGHTING1.unitcode = 0;
	m_mochad.LIGHTING1.cmnd = 0;
}

MochadTCP::~MochadTCP(void)
{
#if defined WIN32
	//
	// Release WinSock
	//
	WSACleanup();
#endif
}

bool MochadTCP::StartHardware()
{
	m_stoprequested=false;

	memset(&m_addr,0,sizeof(sockaddr_in));
	m_addr.sin_family = AF_INET;
	m_addr.sin_port = htons(m_usIPPort);

	unsigned long ip;
	ip=inet_addr(m_szIPAddress.c_str());

	// if we have a error in the ip, it means we have entered a string
	if(ip!=INADDR_NONE)
	{
		m_addr.sin_addr.s_addr=ip;
	}
	else
	{
		// change Hostname in serveraddr
		hostent *he=gethostbyname(m_szIPAddress.c_str());
		if(he==NULL)
		{
			return false;
		}
		else
		{
			memcpy(&(m_addr.sin_addr),he->h_addr_list[0],4);
		}
	}

	//force connect the next first time
//	m_retrycntr=RETRY_DELAY;
//	m_bIsStarted=true;

	//Start worker thread
	m_thread = boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&MochadTCP::Do_Work, this)));
	return (m_thread!=NULL);
}

bool MochadTCP::StopHardware()
{
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
	_log.Log(LOG_STATUS, "Mochad: connected to: %s:%ld", m_szIPAddress.c_str(), m_usIPPort);
	m_bIsStarted = true;

	sOnConnected(this);
}

void MochadTCP::OnDisconnect()
{
	_log.Log(LOG_STATUS, "Mochad: disconnected");
}



bool MochadTCP::ConnectInternal()
{

// connect to the server
	connect((const std::string &)m_addr, sizeof(m_addr));
	_log.Log(LOG_STATUS,"Mochad: connected to: %s:%ld", m_szIPAddress.c_str(), m_usIPPort);

//	Init();

	sOnConnected(this);
	return true;
}


void MochadTCP::OnData(const unsigned char *pData, size_t length)
{
	boost::lock_guard<boost::mutex> l(readQueueMutex);
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
	_log.Log(LOG_ERROR, "Mochad: Error: %s", error.message().c_str());
}

void MochadTCP::WriteToHardware(const char *pdata, const unsigned char length)
{
	RBUF *m_mochad = (RBUF *)pdata;
	if (!mIsConnected)
		return;
	if (pdata[1] == pTypeInterfaceControl && pdata[2] == sTypeInterfaceCommand && pdata[4] == cmdSTATUS) {
		sprintf (s_buffer,"ST\n");
	} else if (pdata[1] == pTypeLighting1 && pdata[2] == sTypeX10 && pdata[6] == light1_sOn) {
		sprintf (s_buffer,"PL %c%d on\n",(char)(pdata[4]), pdata[5]);
	} else if (pdata[1] == pTypeLighting1 && pdata[2] == sTypeX10 && pdata[6] == light1_sOff) {
		sprintf (s_buffer,"PL %c%d off\n",(char)(pdata[4]), pdata[5]);
	} else {
//			case light1_sDim:
//			case light1_sBright:
//			case light1_sAllOn:
//			case light1_sAllOff:
		_log.Log(LOG_STATUS, "Mochad: Unknown command %d:%d:%d:%d", pdata[1],pdata[2],pdata[6]);
	}
//	_log.Log(LOG_STATUS, "Mochad: send '%s'", s_buffer);
	write((const unsigned char *)s_buffer, strlen(s_buffer));
}

void MochadTCP::MatchLine()
{
	if ((strlen((const char*)&m_buffer)<1)||(m_buffer[0]==0x0a))
		return; //null value (startup)
	uint8_t i;
	int j,k;
	uint8_t found=0;
	Match t;
	char value[20]="";
	std::string vString;



	for(i=0;(i<sizeof(matchlist)/sizeof(Match))&(!found);i++)
	{
		t = matchlist[i];
		switch(t.matchtype)
		{
		case ID:
			if(strncmp(t.key, (const char*)&m_buffer, strlen(t.key)) == 0) {
				m_linecount=1;
				found=1;
			}
			else 
				continue;
			break;
		case STD:
			if(strncmp(t.key, (const char*)&m_buffer, strlen(t.key)) == 0) {
				found=1;
			}
			else 
				continue;
			break;
		case LINE17:
			if(strncmp(t.key, (const char*)&m_buffer, strlen(t.key)) == 0) {
				m_linecount = 17;
				found=1;
			}
			else 
				continue;
			break;
		case LINE18:
			if((m_linecount == 18)&&(strncmp(t.key, (const char*)&m_buffer, strlen(t.key)) == 0)) {
				found=1;
			}
			break;
		case EXCLMARK:
			if(strncmp(t.key, (const char*)&m_buffer, strlen(t.key)) == 0) {
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
		if (!('A'<=  m_buffer[j] &&  m_buffer[j] <='Z'))
			goto onError;
		m_mochad.LIGHTING1.housecode = m_buffer[j++];
		if (!(':'==  m_buffer[j++])) goto onError;
		if (!(' '==  m_buffer[j++])) goto onError;
		while ('1' <= m_buffer[j] && m_buffer[j] <= '9') {
			m_mochad.LIGHTING1.unitcode = m_buffer[j++] - '0'; 
			if ('0' <= m_buffer[j] && m_buffer[j] <= '9') {
				m_mochad.LIGHTING1.unitcode = m_mochad.LIGHTING1.unitcode*10 + m_buffer[j++] - '0';
			}
			if (!('='==  m_buffer[j++]))
				return;
			if (!('0' <= m_buffer[j] && m_buffer[j] <= '1')) goto onError;
			m_mochad.LIGHTING1.cmnd = m_buffer[j++] - '0';
			sDecodeRXMessage(this, (const unsigned char *)&m_mochad);//decode message
			if (!(','==  m_buffer[j++])) return;
		}
		break;
	case MOCHAD_UNIT:
		j = t.start;
		if (!('A'<=  m_buffer[j] &&  m_buffer[j] <='Z')) goto onError;
		currentHouse = m_buffer[j++]-'A';
		if (!('0' <= m_buffer[j] && m_buffer[j] <= '9')) goto onError;
		currentUnit = m_buffer[j++] - '0';
		if (('0' <= m_buffer[j] && m_buffer[j] <= '9')) 
			currentUnit = currentUnit*10 + m_buffer[j++] - '0';
		selected[currentHouse][currentUnit] = 1;
		if (!(' '==  m_buffer[j++])) return;
		goto checkFunc;
		break;
	case MOCHAD_ACTION:
		j = t.start;
		if (!('A'<=  m_buffer[j] &&  m_buffer[j] <='Z'))
			goto onError;
		currentHouse = m_buffer[j++]-'A';
		if (!(' '==  m_buffer[j++])) goto onError;
checkFunc:
		if (!('F'==  m_buffer[j++])) goto onError;
		if (!('u'==  m_buffer[j++])) goto onError;
		if (!('n'==  m_buffer[j++])) goto onError;
		if (!('c'==  m_buffer[j++])) goto onError;
		if (!(':'==  m_buffer[j++])) goto onError;
		if (!(' '==  m_buffer[j++])) goto onError;
		if (!('O'==  m_buffer[j++])) goto onError;
		if ('f'==  m_buffer[j]) m_mochad.LIGHTING1.cmnd = 0;
		else
		if ('n'==  m_buffer[j]) m_mochad.LIGHTING1.cmnd = 1;
		else goto onError;
		for (k=1;k<=16;k++) {
			if (selected[currentHouse][k] >0) {
				m_mochad.LIGHTING1.housecode = currentHouse+'A'; 
				m_mochad.LIGHTING1.unitcode = k; 
				sDecodeRXMessage(this, (const unsigned char *)&m_mochad);//decode message			
				selected[currentHouse][k] = 0;
			}
		}
		break;
	}
	return;
onError:
	_log.Log(LOG_ERROR, "Mochad: Cannot decode '%s'", m_buffer);

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

		m_buffer[m_bufferpos] = c;
		if(c == 0x0a || m_bufferpos == sizeof(m_buffer) - 1)
		{
			// discard newline, close string, parse line and clear it.
			if(m_bufferpos > 0) m_buffer[m_bufferpos] = 0;
			m_linecount++;
			if (strlen((const char *)m_buffer) > 14) {
				int i = 0;
				while (m_buffer[i+15] != 0) {
					m_buffer[i] = m_buffer[i+15];
					i++;
				}
				m_buffer[i] = 0;
//				_log.Log(LOG_STATUS, "Mochad: recv '%s'", m_buffer);
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
