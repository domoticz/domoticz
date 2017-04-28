#include "stdafx.h"
#include "P1MeterBase.h"
#include "hardwaretypes.h"
#include "../main/localtime_r.h"
#include "../main/Logger.h"

#define CRC16_ARC	0x8005
#define CRC16_ARC_REFL	0xA001

typedef enum {
	ID=0,
	STD,
	LINE17,
	LINE18,
	EXCLMARK
} MatchType;

#define P1_SMID			"/" // Smart Meter ID. Used to detect start of telegram.
#define P1PU1			"1-0:1.8.1" // total power usage tariff 1
#define P1PU2			"1-0:1.8.2" // total power usage tariff 2
#define P1PD1			"1-0:2.8.1" // total delivered power tariff 1
#define P1PD2			"1-0:2.8.2" // total delivered power tariff 2
#define P1TIP			"0-0:96.14.0" // tariff indicator power
#define P1PUC			"1-0:1.7.0" // current power usage
#define P1PDC			"1-0:2.7.0" // current power delivery
#define P1VOLTL1		"1-0:32.7.0" // voltage L1 (DSMRv5)
#define P1VOLTL2		"1-0:52.7.0" // voltage L2 (DSMRv5)
#define P1VOLTL3		"1-0:72.7.0" // voltage L3 (DSMRv5)
#define P1GTS			"0-1:24.3.0" // timestamp gas usage sample
#define P1GTSME382		"0-2:24.3.0" // timestamp gas usage sample
#define P1GUDSMR4C1		"0-1:24.2.1" // DSMR4 gas usage sample - channel 1
#define P1GUDSMR4C2		"0-2:24.2.1" // DSMR4 gas usage sample - channel 2
#define P1GUDSMR4C3		"0-3:24.2.1" // DSMR4 gas usage sample - channel 3
#define P1GUDSMR4C4		"0-4:24.2.1" // DSMR4 gas usage sample - channel 4

typedef enum {
	P1TYPE_SMID=0,
	P1TYPE_POWERUSAGE1,
	P1TYPE_POWERUSAGE2,
	P1TYPE_POWERDELIV1,
	P1TYPE_POWERDELIV2,
	P1TYPE_TARIFF,
	P1TYPE_USAGECURRENT,
	P1TYPE_DELIVCURRENT,
	P1TYPE_VOLTAGEL1,
	P1TYPE_VOLTAGEL2,
	P1TYPE_VOLTAGEL3,
	P1TYPE_GASTIMESTAMP,
	P1TYPE_GASTIMESTAMPME382,
	P1TYPE_GASUSAGE,
	P1TYPE_GASUSAGEDSMR4_CHANNEL1,
	P1TYPE_GASUSAGEDSMR4_CHANNEL2,
	P1TYPE_GASUSAGEDSMR4_CHANNEL3,
	P1TYPE_GASUSAGEDSMR4_CHANNEL4,
	P1TYPE_END,
} P1Type;

typedef struct _tMatch {
	MatchType matchtype;
	P1Type type;
	const char* key;
	const char* topic;
	int start;
	int width;
} Match;

Match matchlist[] = {
	{ID,		P1TYPE_SMID,				P1_SMID,		"", 			 0,  0},
	{STD,		P1TYPE_POWERUSAGE1,			P1PU1,			"powerusage1",		10,  9},
	{STD,		P1TYPE_POWERUSAGE2,			P1PU2,			"powerusage2",		10,  9},
	{STD,		P1TYPE_POWERDELIV1,			P1PD1,			"powerdeliv1",		10,  9},
	{STD,		P1TYPE_POWERDELIV2,			P1PD2,			"powerdeliv2",		10,  9},
	{STD,		P1TYPE_TARIFF,				P1TIP,			"tariff",		12,  4},
	{STD,		P1TYPE_USAGECURRENT,			P1PUC,			"powerusagec",		10,  7},
	{STD,		P1TYPE_DELIVCURRENT,			P1PDC,			"powerdelivc",		10,  7},
	{STD,		P1TYPE_VOLTAGEL1,			P1VOLTL1,		"voltagel1",		11,  5},
	{STD,		P1TYPE_VOLTAGEL2,			P1VOLTL2,		"voltagel2",		11,  5},
	{STD,		P1TYPE_VOLTAGEL3,			P1VOLTL3,		"voltagel3",		11,  5},
	{LINE17,	P1TYPE_GASTIMESTAMP,			P1GTS,			"gastimestamp",		11, 12},
	{LINE17,	P1TYPE_GASTIMESTAMPME382,		P1GTSME382,		"gastimestamp",		11, 12},
	{LINE18,	P1TYPE_GASUSAGE,			"(",			"gasusage",		 1,  9},
	{STD,		P1TYPE_GASUSAGEDSMR4_CHANNEL1,		P1GUDSMR4C1,		"gasusage",	 	26,  8},
	{STD,		P1TYPE_GASUSAGEDSMR4_CHANNEL2,		P1GUDSMR4C2,		"gasusage",	 	26,  8},
	{STD,		P1TYPE_GASUSAGEDSMR4_CHANNEL3,		P1GUDSMR4C3,		"gasusage",	 	26,  8},
	{STD,		P1TYPE_GASUSAGEDSMR4_CHANNEL4,		P1GUDSMR4C4,		"gasusage",	 	26,  8},
	{EXCLMARK,	P1TYPE_END,				"!",			"",			 0,  0}
};

P1MeterBase::P1MeterBase(void)
{
	Init();
}


P1MeterBase::~P1MeterBase(void)
{
}

void P1MeterBase::Init()
{
	m_linecount=0;
	m_exclmarkfound=0;
	l_exclmarkfound=0;
	m_CRfound=0;
	m_bufferpos=0;
	l_bufferpos=0;
	m_lastgasusage=0;
	m_lastSharedSendGas=0;
	m_lastUpdateTime=0;
	m_voltagel1=0;
	m_voltagel2=0;
	m_voltagel3=0;

	memset(&m_buffer,0,sizeof(m_buffer));
	memset(&l_buffer,0,sizeof(l_buffer));

	memset(&m_p1power,0,sizeof(m_p1power));
	memset(&m_p1gas,0,sizeof(m_p1gas));

	m_p1power.len=sizeof(P1Power)-1;
	m_p1power.type=pTypeP1Power;
	m_p1power.subtype=sTypeP1Power;
	m_p1power.ID=1;

	m_p1gas.len=sizeof(P1Gas)-1;
	m_p1gas.type=pTypeP1Gas;
	m_p1gas.subtype=sTypeP1Gas;
	m_p1gas.ID=1;
}

bool P1MeterBase::MatchLine()
{
	if ((strlen((const char*)&l_buffer)<1)||(l_buffer[0]==0x0a))
		return true; //null value (startup)
	uint8_t i;
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
			if(strncmp(t.key, (const char*)&l_buffer, strlen(t.key)) == 0) {
				m_linecount=1;
				found=1;
			}
			else
				continue;
			break;
		case STD:
			if(strncmp(t.key, (const char*)&l_buffer, strlen(t.key)) == 0) {
				found=1;
			}
			else
				continue;
			break;
		case LINE17:
			if(strncmp(t.key, (const char*)&l_buffer, strlen(t.key)) == 0) {
				m_linecount = 17;
				found=1;
			}
			else
				continue;
			break;
		case LINE18:
			if((m_linecount == 18)&&(strncmp(t.key, (const char*)&l_buffer, strlen(t.key)) == 0)) {
				found=1;
			}
			break;
		case EXCLMARK:
			if(strncmp(t.key, (const char*)&l_buffer, strlen(t.key)) == 0) {
				l_exclmarkfound=1;
				found=1;
			}
			else
				continue;
			break;
		default:
			continue;
		} //switch

		if(!found)
			continue;

		if (l_exclmarkfound) {
			time_t atime=mytime(NULL);
			if (difftime(atime,m_lastUpdateTime)>=m_ratelimit) {
				m_lastUpdateTime=atime;
				sDecodeRXMessage(this, (const unsigned char *)&m_p1power, "Power", 255);
				if (m_voltagel1) {
					SendVoltageSensor(0, 1, 255, m_voltagel1, "Voltage L1");
					if (m_voltagel2)
						SendVoltageSensor(0, 2, 255, m_voltagel2, "Voltage L2");
					if (m_voltagel3)
						SendVoltageSensor(0, 3, 255, m_voltagel3, "Voltage L3");
				}
				if (
					(m_p1gas.gasusage!=m_lastgasusage)||
					(difftime(atime,m_lastSharedSendGas)>=300)
					)
				{
					//only update gas when there is a new value, or 5 minutes are passed
					m_lastSharedSendGas=atime;
					m_lastgasusage=m_p1gas.gasusage;
					sDecodeRXMessage(this, (const unsigned char *)&m_p1gas, "Gas", 255);
				}
			}
			m_linecount=0;
			l_exclmarkfound=0;
		}
		else
		{
			vString=(const char*)&l_buffer+t.start;
			int ePos=t.width;
			if (t.matchtype==STD)
			{
				ePos=vString.find_first_of("*)");
			}
			else if (t.matchtype==LINE18)
			{
				ePos=vString.find_first_of(")");
			}

			if (ePos==std::string::npos)
			{
				// invalid message: value not delimited
				_log.Log(LOG_STATUS,"P1: dismiss incoming - value is not delimited in line \"%s\"",l_buffer);
				return false;
			}

			if (ePos>19)
			{
				// invalid message: line too long
				_log.Log(LOG_STATUS,"P1: dismiss incoming - value in line \"%s\" is oversized",l_buffer);
				return false;
			}

			if (ePos>0)
			{
				strcpy(value,vString.substr(0,ePos).c_str());
				//_log.Log(LOG_NORM,"P1: Key: %s, Value: %s", t.topic,value);
			}

			unsigned long temp_usage = 0;
			float temp_volt = 0;
			char *validate=value+ePos;

			switch (t.type)
			{
			case P1TYPE_POWERUSAGE1:
				temp_usage = (unsigned long)(strtod(value,&validate)*1000.0f);
				m_p1power.powerusage1 = temp_usage;
				break;
			case P1TYPE_POWERUSAGE2:
				temp_usage = (unsigned long)(strtod(value,&validate)*1000.0f);
				m_p1power.powerusage2=temp_usage;
				break;
			case P1TYPE_POWERDELIV1:
				temp_usage = (unsigned long)(strtod(value,&validate)*1000.0f);
				m_p1power.powerdeliv1=temp_usage;
				break;
			case P1TYPE_POWERDELIV2:
				temp_usage = (unsigned long)(strtod(value,&validate)*1000.0f);
				m_p1power.powerdeliv2=temp_usage;
				break;
			case P1TYPE_TARIFF:
				break;
			case P1TYPE_USAGECURRENT:
				temp_usage = (unsigned long)(strtod(value,&validate)*1000.0f);	//Watt
				if (temp_usage < 17250)
				{
					m_p1power.usagecurrent = temp_usage;
				}
				break;
			case P1TYPE_DELIVCURRENT:
				temp_usage = (unsigned long)(strtod(value,&validate)*1000.0f);	//Watt;
				if (temp_usage < 17250)
				{
					m_p1power.delivcurrent = temp_usage;
				}
				break;
			case P1TYPE_VOLTAGEL1:
				temp_volt = strtof(value,&validate);
				if (temp_volt < 300)
					m_voltagel1 = temp_volt; //Voltage L1;
				break;
			case P1TYPE_VOLTAGEL2:
				temp_volt = strtof(value,&validate);
				if (temp_volt < 300)
					m_voltagel2 = temp_volt; //Voltage L2;
				break;
			case P1TYPE_VOLTAGEL3:
				temp_volt = strtof(value,&validate);
				if (temp_volt < 300)
					m_voltagel3 = temp_volt; //Voltage L3;
				break;
			case P1TYPE_GASTIMESTAMP:
			case P1TYPE_GASTIMESTAMPME382:
				break;
			case P1TYPE_GASUSAGE:
			case P1TYPE_GASUSAGEDSMR4_CHANNEL1:
			case P1TYPE_GASUSAGEDSMR4_CHANNEL2:
			case P1TYPE_GASUSAGEDSMR4_CHANNEL3:
			case P1TYPE_GASUSAGEDSMR4_CHANNEL4:
				temp_usage = (unsigned long)(strtod(value,&validate)*1000.0f);
				m_p1gas.gasusage = temp_usage;
				break;
			}

			if (ePos>0 && ((validate - value) != ePos)) {
				// invalid message: value is not a number
				_log.Log(LOG_STATUS,"P1: dismiss incoming - value in line \"%s\" is not a number", l_buffer);
				return false;
			}

			return true;
		}
	}
	return true;
}


/*
/ GB3:	DSMR 4.0 defines a CRC checksum at the end of the message, calculated from
/	and including the message starting character '/' upto and including the message
/	end character '!'. According to the specs the CRC is a 16bit checksum using the
/	polynomial x^16 + x^15 + x^2 + 1, however input/output are reflected.
*/

bool P1MeterBase::CheckCRC()
{
	// sanity checks
	if (l_buffer[1]==0){
		// always return true with pre DSMRv4 format message
		return true;
	}

	if (l_buffer[5]!=0){
		// trailing characters after CRC
		_log.Log(LOG_STATUS,"P1: dismiss incoming - CRC value in message has trailing characters");
		return false;
	}

	if (!m_CRfound){
		_log.Log(LOG_STATUS,"P1: you appear to have middleware that changes the message content - skipping CRC validation");
		return true;
	}

	// retrieve CRC from the current line
	char crc_str[5];
	strncpy(crc_str, (const char*)&l_buffer+1, 4);
	crc_str[4]=0;
	uint16_t m_crc16=(uint16_t)strtoul(crc_str,NULL,16);

	// calculate CRC
	const unsigned char* c_buffer=m_buffer;
	uint8_t i;
	uint16_t crc=0;
	int m_size=m_bufferpos;
	while (m_size--) {
		crc = crc ^ *c_buffer++;
		for (i=0;i<8;i++){
			if ((crc & 0x0001)){
				crc = (crc >> 1) ^ CRC16_ARC_REFL;
			} else {
				crc = crc >> 1;
			}
		}
	}
	if (crc != m_crc16){
		_log.Log(LOG_STATUS,"P1: dismiss incoming - CRC failed");
	}
	return (crc==m_crc16);
}


/*
/ GB3:	ParseData() can be called with either a complete message (P1MeterTCP) or individual
/	lines (P1MeterSerial).
/
/	While it is technically possible to do a CRC check line by line, we like to keep
/	things organized and assemble the complete message before running that check. If the
/	message is DSMR 4.0+ of course.
/
/	Because older DSMR standard does not contain a CRC we still need the validation rules
/	in Matchline(). In fact, one of them is essential for keeping Domoticz from crashing
/	in specific cases of bad data. In essence this means that a CRC check will only be
/	done if the message passes all other validation rules
*/

void P1MeterBase::ParseData(const unsigned char *pData, const int Len, const bool disable_crc, int ratelimit)
{
	int ii=0;
	m_ratelimit=ratelimit;
	// a new message should not start with an empty line, but just in case it does (crude check is sufficient here)
	while ((m_linecount==0) && (pData[ii]<0x10)){
		ii++;
	}

	// re enable reading pData when a new message starts, empty buffers
	if (pData[ii]==0x2f)
	{
		if ((l_buffer[0]==0x21) && !l_exclmarkfound && (m_linecount>0)) {
			_log.Log(LOG_STATUS,"P1: WARNING: got new message but buffer still contains unprocessed data from previous message.");
			l_buffer[l_bufferpos] = 0;
			if (disable_crc || CheckCRC()) {
				MatchLine();
			}
		}
		m_linecount = 1;
		l_bufferpos = 0;
		m_bufferpos = 0;
		m_exclmarkfound = 0;
	}

	// assemble complete message in message buffer
	while ((ii<Len) && (m_linecount>0) && (!m_exclmarkfound) && (m_bufferpos<sizeof(m_buffer))){
		const unsigned char c = pData[ii];
		m_buffer[m_bufferpos] = c;
		m_bufferpos++;
		if(c==0x21){
			// stop reading at exclamation mark (do not include CRC)
			ii=Len;
			m_exclmarkfound = 1;
		}else{
			ii++;
		}
	}

	if(m_bufferpos==sizeof(m_buffer)){
		// discard oversized message
		if ((Len > 400) || (pData[0]==0x21)){
			// 400 is an arbitrary chosen number to differentiate between full messages and single line commits
			_log.Log(LOG_STATUS,"P1: dismiss incoming - message oversized");
		}
		m_linecount = 0;
		return;
	}

	// read pData, ignore/stop if there is a message validation failure
	ii=0;
	while ((ii<Len) && (m_linecount>0))
	{
		const unsigned char c = pData[ii];
		ii++;
		if (c==0x0d) {
			m_CRfound=1;
			continue;
		}

		if (c==0x0a) {
			// close string, parse line and clear it.
			m_linecount++;
			if ((l_bufferpos>0) && (l_bufferpos<sizeof(l_buffer))) {
				// don't try to match empty or oversized lines
				l_buffer[l_bufferpos] = 0;
				if(l_buffer[0]==0x21 && !disable_crc){
					if (!CheckCRC()) {
						m_linecount = 0;
						return;
					}
				}
				if (!MatchLine()) {
					// discard message
					m_linecount=0;
				}
			}
			l_bufferpos = 0;
		}
		else if (l_bufferpos<sizeof(l_buffer)) {
			l_buffer[l_bufferpos] = c;
			l_bufferpos++;
		}
	}
}
