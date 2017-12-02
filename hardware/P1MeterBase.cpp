#include "stdafx.h"
#include "P1MeterBase.h"
#include "hardwaretypes.h"
#include "../main/SQLHelper.h"
#include "../main/localtime_r.h"
#include "../main/Logger.h"

#define CRC16_ARC	0x8005
#define CRC16_ARC_REFL	0xA001

typedef enum {
	ID=0,
	EXCLMARK,
	STD,
	DEVTYPE,
	GAS,
	LINE17,
	LINE18
} MatchType;

#define P1SMID		"/"		// Smart Meter ID. Used to detect start of telegram.
#define P1VER		"1-3:0.2.8"	// P1 version
#define P1TS		"0-0:1.0.0"	// Timestamp
#define P1PU1		"1-0:1.8.1"	// total power usage tariff 1
#define P1PU2		"1-0:1.8.2"	// total power usage tariff 2
#define P1PD1		"1-0:2.8.1"	// total delivered power tariff 1
#define P1PD2		"1-0:2.8.2"	// total delivered power tariff 2
#define P1TIP		"0-0:96.14.0"	// tariff indicator power
#define P1PUC		"1-0:1.7.0"	// current power usage
#define P1PDC		"1-0:2.7.0"	// current power delivery
#define P1VOLTL1	"1-0:32.7.0"	// voltage L1 (DSMRv5)
#define P1VOLTL2	"1-0:52.7.0"	// voltage L2 (DSMRv5)
#define P1VOLTL3	"1-0:72.7.0"	// voltage L3 (DSMRv5)
#define P1GTS		"0-n:24.3.0"	// DSMR2 timestamp gas usage sample
#define P1GUDSMR2	"("		// DSMR2 gas usage sample
#define P1GUDSMR4	"0-n:24.2.1"	// DSMR4 gas usage sample
#define P1MBTYPE	"0-n:24.1.0"	// M-Bus device type
#define P1EOT		"!"		// End of telegram.

typedef enum {
	P1TYPE_SMID=0,
	P1TYPE_END,
	P1TYPE_VERSION,
	P1TYPE_POWERUSAGE1,
	P1TYPE_POWERUSAGE2,
	P1TYPE_POWERDELIV1,
	P1TYPE_POWERDELIV2,
	P1TYPE_USAGECURRENT,
	P1TYPE_DELIVCURRENT,
	P1TYPE_VOLTAGEL1,
	P1TYPE_VOLTAGEL2,
	P1TYPE_VOLTAGEL3,
	P1TYPE_MBUSDEVICETYPE,
	P1TYPE_GASUSAGEDSMR4,
	P1TYPE_GASTIMESTAMP,
	P1TYPE_GASUSAGE
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
	{ID,		P1TYPE_SMID,			P1SMID,		"",			 0,  0},
	{EXCLMARK,	P1TYPE_END,			P1EOT,		"",			 0,  0},
	{STD,		P1TYPE_VERSION,			P1VER,		"version",		10,  2},
	{STD,		P1TYPE_POWERUSAGE1,		P1PU1,		"powerusage1",		10,  9},
	{STD,		P1TYPE_POWERUSAGE2,		P1PU2,		"powerusage2",		10,  9},
	{STD,		P1TYPE_POWERDELIV1,		P1PD1,		"powerdeliv1",		10,  9},
	{STD,		P1TYPE_POWERDELIV2,		P1PD2,		"powerdeliv2",		10,  9},
	{STD,		P1TYPE_USAGECURRENT,		P1PUC,		"powerusagec",		10,  7},
	{STD,		P1TYPE_DELIVCURRENT,		P1PDC,		"powerdelivc",		10,  7},
	{STD,		P1TYPE_VOLTAGEL1,		P1VOLTL1,	"voltagel1",		11,  5},
	{STD,		P1TYPE_VOLTAGEL2,		P1VOLTL2,	"voltagel2",		11,  5},
	{STD,		P1TYPE_VOLTAGEL3,		P1VOLTL3,	"voltagel3",		11,  5},
	{DEVTYPE,	P1TYPE_MBUSDEVICETYPE,		P1MBTYPE,	"mbusdevicetype",	11,  3},
	{GAS,		P1TYPE_GASUSAGEDSMR4,		P1GUDSMR4,	"gasusage",	 	26,  8},
	{LINE17,	P1TYPE_GASTIMESTAMP,		P1GTS,		"gastimestamp",		11, 12},
	{LINE18,	P1TYPE_GASUSAGE,		P1GUDSMR2,	"gasusage",		 1,  9}
}; // must keep DEVTYPE, GAS, LINE17 and LINE18 in this order at end of matchlist

P1MeterBase::P1MeterBase(void)
{
}


P1MeterBase::~P1MeterBase(void)
{
}

void P1MeterBase::Init()
{
	m_p1version=0;
	m_linecount=0;
	m_exclmarkfound=0;
	m_CRfound=0;
	m_bufferpos=0;
	m_lastgasusage=0;
	m_lastSharedSendGas=0;
	m_lastUpdateTime=0;

	l_exclmarkfound=0;
	l_bufferpos=0;

	m_voltagel1=0;
	m_voltagel2=0;
	m_voltagel3=0;

	memset(&m_buffer,0,sizeof(m_buffer));
	memset(&l_buffer,0,sizeof(l_buffer));

	memset(&m_power,0,sizeof(m_power));
	memset(&m_gas,0,sizeof(m_gas));

	m_power.len=sizeof(P1Power)-1;
	m_power.type=pTypeP1Power;
	m_power.subtype=sTypeP1Power;
	m_power.ID=1;

	m_gas.len=sizeof(P1Gas)-1;
	m_gas.type=pTypeP1Gas;
	m_gas.subtype=sTypeP1Gas;
	m_gas.ID=1;
	m_gas.gasusage=0;

	m_gasmbuschannel=0;
	m_gasprefix="0-n";
	m_gastimestamp="";
	m_gasclockskew=0;
	m_gasoktime=0;

	std::vector<std::vector<std::string> > result;
	result = m_sql.safe_query("SELECT Value FROM UserVariables WHERE (Name='P1GasMeterChannel')");
	if (!result.empty())
	{
		std::string s_gasmbuschannel = result[0][0];
		if ((s_gasmbuschannel.length()==1) && (s_gasmbuschannel[0]>0x30) && (s_gasmbuschannel[0]<0x35)) // value must be a single digit number between 1 and 4
		{
			m_gasmbuschannel=(char)s_gasmbuschannel[0];
			m_gasprefix[2]=m_gasmbuschannel;
			_log.Log(LOG_STATUS,"P1 Smart Meter: Gas meter M-Bus channel %c enforced by 'P1GasMeterChannel' user variable", m_gasmbuschannel);
		}
	}
}

bool P1MeterBase::MatchLine()
{
	if ((strlen((const char*)&l_buffer)<1)||(l_buffer[0]==0x0a))
		return true; //null value (startup)
	uint8_t i;
	uint8_t found=0;
	Match *t;
	char value[20]="";
	std::string vString;

	for(i=0;(i<sizeof(matchlist)/sizeof(Match))&(!found);i++)
	{
		t = &matchlist[i];
		switch(t->matchtype)
		{
		case ID:
			// start of data
			if(strncmp(t->key, (const char*)&l_buffer, strlen(t->key)) == 0) {
				m_linecount=1;
				found=1;
			}
			continue; // we do not process anything else on this line
			break;
		case EXCLMARK:
			// end of data
			if(strncmp(t->key, (const char*)&l_buffer, strlen(t->key)) == 0) {
				l_exclmarkfound=1;
				found=1;
			}
			break;
		case STD:
			if(strncmp(t->key, (const char*)&l_buffer, strlen(t->key)) == 0)
				found=1;
			break;
		case DEVTYPE:
			if(m_gasmbuschannel==0){
				vString=(const char*)t->key+3;
				if (strncmp(vString.c_str(), (const char*)&l_buffer+3, strlen(t->key)-3) == 0)
					found=1;
				else
					i+=100; // skip matches with any other gas lines - we need to find the M0-Bus channel first
			}
			break;
		case GAS:
			if(strncmp((m_gasprefix+(t->key+3)).c_str(), (const char*)&l_buffer, strlen(t->key)) == 0){
				found=1;
			}
			if (m_p1version>=4)
				i+=100; // skip matches with any DSMR v2 gas lines
			break;
		case LINE17:
			if(strncmp((m_gasprefix+(t->key+3)).c_str(), (const char*)&l_buffer, strlen(t->key)) == 0){
				m_linecount = 17;
				found=1;
			}
			break;
		case LINE18:
			if((m_linecount == 18)&&(strncmp(t->key, (const char*)&l_buffer, strlen(t->key)) == 0))
				found=1;
			break;
		} //switch

		if(!found)
			continue;

		if (l_exclmarkfound) {
			if (m_p1version==0)
			{
				_log.Log(LOG_STATUS,"P1 Smart Meter: Meter is pre DSMR 4.0 - using DSMR 2.2 compatibility");
				m_p1version=2;
			}
			time_t atime=mytime(NULL);
			if (difftime(atime,m_lastUpdateTime)>=m_ratelimit) {
				m_lastUpdateTime=atime;
				sDecodeRXMessage(this, (const unsigned char *)&m_power, "Power", 255);
				if (m_voltagel1) {
					SendVoltageSensor(0, 1, 255, m_voltagel1, "Voltage L1");
					if (m_voltagel2)
						SendVoltageSensor(0, 2, 255, m_voltagel2, "Voltage L2");
					if (m_voltagel3)
						SendVoltageSensor(0, 3, 255, m_voltagel3, "Voltage L3");
				}
				if ( (m_gas.gasusage>0)&&( (m_gas.gasusage!=m_lastgasusage)||(difftime(atime,m_lastSharedSendGas)>=300) ) ){
					//only update gas when there is a new value, or 5 minutes are passed
					if (m_gasclockskew>=300){ // just accept it - we cannot sync to our clock
						m_lastSharedSendGas=atime;
						m_lastgasusage=m_gas.gasusage;
						sDecodeRXMessage(this, (const unsigned char *)&m_gas, "Gas", 255);
					}
					else if (atime>=m_gasoktime){
						struct tm ltime;
						localtime_r(&atime, &ltime);
						char myts[16];
						sprintf(myts,"%02d%02d%02d%02d%02d%02dW",ltime.tm_year%100,ltime.tm_mon+1,ltime.tm_mday,ltime.tm_hour,ltime.tm_min,ltime.tm_sec);
						if (ltime.tm_isdst)
						myts[12]='S';
						if ( (m_gastimestamp.length()>13) || (strncmp((const char*)&myts,m_gastimestamp.c_str(),m_gastimestamp.length())>=0) )
						{
							m_lastSharedSendGas=atime;
							m_lastgasusage=m_gas.gasusage;
							m_gasoktime+=300;
							sDecodeRXMessage(this, (const unsigned char *)&m_gas, "Gas", 255);
						}
						else // gas clock is ahead
						{
							struct tm gastm;
							gastm.tm_year = atoi(m_gastimestamp.substr(0, 2).c_str()) + 100;
							gastm.tm_mon = atoi(m_gastimestamp.substr(2, 2).c_str()) - 1;
							gastm.tm_mday = atoi(m_gastimestamp.substr(4, 2).c_str());
							gastm.tm_hour = atoi(m_gastimestamp.substr(6, 2).c_str());
							gastm.tm_min = atoi(m_gastimestamp.substr(8, 2).c_str());
							gastm.tm_sec = atoi(m_gastimestamp.substr(10, 2).c_str());
							if (m_gastimestamp.length()==12)
								gastm.tm_isdst = -1;
							else if (m_gastimestamp[12]=='W')
								gastm.tm_isdst = 0;
							else
								gastm.tm_isdst = 1;

							time_t gtime=mktime(&gastm);
							m_gasclockskew=difftime(gtime,atime);
							if (m_gasclockskew>=300){
								_log.Log(LOG_ERROR, "P1 Smart Meter: Unable to synchronize to the gas meter clock because it is more than 5 minutes ahead of my time");
							}
							else {
								m_gasoktime=gtime;
								_log.Log(LOG_STATUS, "P1 Smart Meter: Gas meter clock is %i seconds ahead - wait for my clock to catch up", (int)m_gasclockskew);
							}
						}
					}
				}
			}
			m_linecount=0;
			l_exclmarkfound=0;
		}
		else
		{
			vString=(const char*)&l_buffer+t->start;
			int ePos=t->width;
			ePos=vString.find_first_of("*)");

			if (ePos==std::string::npos)
			{
				// invalid message: value not delimited
				_log.Log(LOG_NORM,"P1 Smart Meter: Dismiss incoming - value is not delimited in line \"%s\"", l_buffer);
				return false;
			}

			if (ePos>19)
			{
				// invalid message: line too long
				_log.Log(LOG_NORM,"P1 Smart Meter: Dismiss incoming - value in line \"%s\" is oversized", l_buffer);
				return false;
			}

			if (ePos>0)
			{
				strcpy(value,vString.substr(0,ePos).c_str());
#ifdef _DEBUG
				_log.Log(LOG_NORM,"P1 Smart Meter: Key: %s, Value: %s", t->topic,value);
#endif
			}

			unsigned long temp_usage = 0;
			float temp_volt = 0;
			char *validate=value+ePos;

			switch (t->type)
			{
			case P1TYPE_VERSION:
				if (m_p1version==0)
					_log.Log(LOG_STATUS,"P1 Smart Meter: Meter reports as DSMR %c.%c", value[0], value[1]);
				m_p1version=value[0]-0x30;
				break;
			case P1TYPE_MBUSDEVICETYPE:
				temp_usage = (unsigned long)(strtod(value,&validate));
				if (temp_usage == 3) {
					m_gasmbuschannel = (char)l_buffer[2];
					m_gasprefix[2]=m_gasmbuschannel;
					_log.Log(LOG_STATUS,"P1 Smart Meter: Found gas meter on M-Bus channel %c", m_gasmbuschannel);
				}
				break;
			case P1TYPE_POWERUSAGE1:
				temp_usage = (unsigned long)(strtod(value,&validate)*1000.0f);
				if (!m_power.powerusage1 || m_p1version >= 4)
					m_power.powerusage1 = temp_usage;
				else if (temp_usage - m_power.powerusage1 < 10000)
					m_power.powerusage1 = temp_usage;
				break;
			case P1TYPE_POWERUSAGE2:
				temp_usage = (unsigned long)(strtod(value,&validate)*1000.0f);
				if (!m_power.powerusage2 || m_p1version >= 4)
					m_power.powerusage2 = temp_usage;
				else if (temp_usage - m_power.powerusage2 < 10000)
					m_power.powerusage2 = temp_usage;
				break;
			case P1TYPE_POWERDELIV1:
				temp_usage = (unsigned long)(strtod(value,&validate)*1000.0f);
				if (!m_power.powerdeliv1 || m_p1version >= 4)
					m_power.powerdeliv1 = temp_usage;
				else if (temp_usage - m_power.powerdeliv1 < 10000)
					m_power.powerdeliv1 = temp_usage;
				break;
			case P1TYPE_POWERDELIV2:
				temp_usage = (unsigned long)(strtod(value,&validate)*1000.0f);
				if (!m_power.powerdeliv2 || m_p1version >= 4)
					m_power.powerdeliv2 = temp_usage;
				else if (temp_usage - m_power.powerdeliv2 < 10000)
					m_power.powerdeliv2 = temp_usage;
				break;
			case P1TYPE_USAGECURRENT:
				temp_usage = (unsigned long)(strtod(value,&validate)*1000.0f);	//Watt
				if (temp_usage < 17250)
					m_power.usagecurrent = temp_usage;
				break;
			case P1TYPE_DELIVCURRENT:
				temp_usage = (unsigned long)(strtod(value,&validate)*1000.0f);	//Watt;
				if (temp_usage < 17250)
					m_power.delivcurrent = temp_usage;
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
				m_gastimestamp = std::string(value);
				break;
			case P1TYPE_GASUSAGE:
			case P1TYPE_GASUSAGEDSMR4:
				temp_usage = (unsigned long)(strtod(value,&validate)*1000.0f);
				if (!m_gas.gasusage || m_p1version >= 4)
					m_gas.gasusage = temp_usage;
				else if (temp_usage - m_gas.gasusage < 20000)
					m_gas.gasusage = temp_usage;
				break;
			}

			if (ePos>0 && ((validate - value) != ePos)) {
				// invalid message: value is not a number
				_log.Log(LOG_NORM,"P1 Smart Meter: Dismiss incoming - value in line \"%s\" is not a number", l_buffer);
				return false;
			}

			if (t->type == P1TYPE_GASUSAGEDSMR4){ // need to get timestamp from this line as well
				vString=(const char*)&l_buffer+11;
				m_gastimestamp=vString.substr(0,13);
#ifdef _DEBUG
				_log.Log(LOG_NORM,"P1 Smart Meter: Key: gastimestamp, Value: %s", m_gastimestamp);
#endif
			}
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
		if (m_p1version==0)
		{
			_log.Log(LOG_STATUS,"P1 Smart Meter: Meter is pre DSMR 4.0 and does not send a CRC checksum - using DSMR 2.2 compatibility");
			m_p1version=2;
		}
		// always return true with pre DSMRv4 format message
		return true;
	}

	if (l_buffer[5]!=0){
		// trailing characters after CRC
		_log.Log(LOG_NORM,"P1 Smart Meter: Dismiss incoming - CRC value in message has trailing characters");
		return false;
	}

	if (!m_CRfound){
		_log.Log(LOG_NORM,"P1 Smart Meter: You appear to have middle ware that changes the message content - skipping CRC validation");
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
		_log.Log(LOG_NORM,"P1 Smart Meter: Dismiss incoming - CRC failed");
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
			_log.Log(LOG_STATUS,"P1 Smart Meter: WARNING: got new message but buffer still contains unprocessed data from previous message.");
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
			_log.Log(LOG_NORM,"P1 Smart Meter: Dismiss incoming - message oversized");
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
