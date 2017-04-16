#include "stdafx.h"
#include "P1MeterBase.h"
#include "hardwaretypes.h"
#include "../main/localtime_r.h"

typedef enum { 
	ID=0, 
	STD, 
	LINE17, 
	LINE18, 
	EXCLMARK 
} MatchType;

#define P1_SMID			"/" // Smart Meter ID. Used to detect start of telegram.
#define P1PU1			"1-0:1.8.1" // total power usage normal tariff
#define P1PU2			"1-0:1.8.2" // total power usage low tariff
#define P1PD1			"1-0:2.8.1" // total delivered power normal tariff
#define P1PD2			"1-0:2.8.2" // total delivered power low tariff
#define P1TIP			"0-0:96.14.0" // tariff indicator power
#define P1PUC			"1-0:1.7.0" // current power usage
#define P1PDC			"1-0:2.7.0" // current power delivery
#define P1GTS			"0-1:24.3.0" // timestamp gas usage sample
#define P1GTSDSMRv4		"0-1:24.2.1" // timestamp gas usage sample
#define P1GTSGyrE350	"0-2:24.2.1" // timestamp gas usage sample
#define P1GTSME382		"0-2:24.3.0" // timestamp gas usage sample

typedef enum {
	P1TYPE_SMID=0,
	P1TYPE_POWERUSAGE1,
	P1TYPE_POWERUSAGE2,
	P1TYPE_POWERDELIV1,
	P1TYPE_POWERDELIV2,
	P1TYPE_TARIFF,
	P1TYPE_USAGECURRENT,
	P1TYPE_DELIVCURRENT,
	P1TYPE_GASTIMESTAMP,
	P1TYPE_GASTIMESTAMPME382,
	P1TYPE_GASUSAGE,
	P1TYPE_GASUSAGEDSMRv4,
	P1TYPE_GASUSAGEGyrE350,
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
	{ID,	P1TYPE_SMID,				P1_SMID,		"", 0, 0},
	{STD,	P1TYPE_POWERUSAGE1,			P1PU1,			"powerusage1",	10, 9},
	{STD,	P1TYPE_POWERUSAGE2,			P1PU2,			"powerusage2",	10, 9},
	{STD,	P1TYPE_POWERDELIV1,			P1PD1,			"powerdeliv1",	10, 9},
	{STD,	P1TYPE_POWERDELIV2,			P1PD2,			"powerdeliv2",	10, 9},
	{STD,	P1TYPE_TARIFF,				P1TIP,			"tariff",		12, 4},
	{STD,	P1TYPE_USAGECURRENT,		P1PUC,			"powerusagec",	10, 7},
	{STD,	P1TYPE_DELIVCURRENT,		P1PDC,			"powerdelivc",	10, 7},
	{ LINE17,P1TYPE_GASTIMESTAMP,		P1GTS,			"gastimestamp",	11, 12 },
	{ LINE17,P1TYPE_GASTIMESTAMPME382,	P1GTSME382,		"gastimestamp",	11, 12 },
	{LINE18,P1TYPE_GASUSAGE,			"(",			"gasusage",		1, 9},
	{ STD, P1TYPE_GASUSAGEDSMRv4,		P1GTSDSMRv4,	"gasusage", 26, 8 },
	{ STD, P1TYPE_GASUSAGEGyrE350,		P1GTSGyrE350,	"gasusage", 26, 8 },
	{ EXCLMARK, P1TYPE_END, "!", "", 0, 0 }
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
	m_bufferpos=0;
	m_lastgasusage=0;
	m_lastelectrausage=0;
	m_lastSharedSendElectra=0;
	m_lastSharedSendGas=0;

	memset(&m_buffer,0,sizeof(m_buffer));

	memset(&m_p1power,0,sizeof(m_p1power));
	memset(&m_p1gas,0,sizeof(m_p1gas));

	m_p1power.len=sizeof(P1Power)-1;
	m_p1power.type=pTypeP1Power;
	m_p1power.subtype=sTypeP1Power;
	m_p1power.ID = 1;

	m_p1gas.len=sizeof(P1Gas)-1;
	m_p1gas.type=pTypeP1Gas;
	m_p1gas.subtype=sTypeP1Gas;
	m_p1gas.ID = 1;
}

void P1MeterBase::MatchLine()
{
	if ((strlen((const char*)&m_buffer)<1)||(m_buffer[0]==0x0a))
		return; //null value (startup)
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
		
		if(!found)
			continue;

		if (m_exclmarkfound) {
			time_t atime=mytime(NULL);
			sDecodeRXMessage(this, (const unsigned char *)&m_p1power, "Power", 255);
			bool bSend2Shared=(atime-m_lastSharedSendElectra>59);
			if (abs(double(m_lastelectrausage)-double(m_p1power.usagecurrent))>40)
				bSend2Shared=true;
			if (bSend2Shared)
			{
				m_lastelectrausage=m_p1power.usagecurrent;
				m_lastSharedSendElectra=atime;
			}
			if (
				(m_p1gas.gasusage!=m_lastgasusage)||
				(atime-m_lastSharedSendGas>=300)
				)
			{
				//only update gas when there is a new value, or 5 minutes are passed
				m_lastSharedSendGas=atime;
				m_lastgasusage=m_p1gas.gasusage;
				sDecodeRXMessage(this, (const unsigned char *)&m_p1gas, "Gas", 255);
			}
			m_exclmarkfound=0;
		}
		else
		{
			if (t.matchtype==STD)
			{
				vString=(const char*)&m_buffer+t.start;
				int ePos=vString.find_first_of("*");
				if (ePos==std::string::npos)
				{
					ePos=vString.find_first_of(")");
					if (ePos==std::string::npos)
					{
						strncpy(value, (const char*)&m_buffer+t.start, t.width);
						value[t.width] = 0;
					}
					else
					{
						strcpy(value,vString.substr(0,ePos).c_str());
					}
				}
				else
				{
					strcpy(value,vString.substr(0,ePos).c_str());
				}
			}
			else if (t.matchtype==LINE18)
			{
				vString=(const char*)&m_buffer+t.start;
				int ePos=vString.find_first_of(")");
				if (ePos==std::string::npos)
				{
					strncpy(value, (const char*)&m_buffer+t.start, t.width);
					value[t.width] = 0;
				}
				else
				{
					strcpy(value,vString.substr(0,ePos).c_str());
				}
			}
			else if (t.matchtype==ID)
			{
				//
			}
			else
			{
				strncpy(value, (const char*)&m_buffer + t.start, t.width);
				value[t.width] = 0;
			}

			unsigned long temp_usage = 0;

			switch (t.type)
			{
			case P1TYPE_POWERUSAGE1:
				temp_usage = (unsigned long)(atof(value)*1000.0f);
				m_p1power.powerusage1 = temp_usage;
				break;
			case P1TYPE_POWERUSAGE2:
				temp_usage = (unsigned long)(atof(value)*1000.0f);
				m_p1power.powerusage2=temp_usage;
				break;
			case P1TYPE_POWERDELIV1:
				temp_usage = (unsigned long)(atof(value)*1000.0f);
				m_p1power.powerdeliv1=temp_usage;
				break;
			case P1TYPE_POWERDELIV2:
				temp_usage = (unsigned long)(atof(value)*1000.0f);
				m_p1power.powerdeliv2=temp_usage;
				break;
			case P1TYPE_TARIFF:
				break;
			case P1TYPE_USAGECURRENT:
				temp_usage = (unsigned long)(atof(value)*1000.0f);	//Watt
				if (temp_usage < 17250)
				{
					m_p1power.usagecurrent = temp_usage;
				}
				break;
			case P1TYPE_DELIVCURRENT:
				temp_usage = (unsigned long)(atof(value)*1000.0f);	//Watt;
				if (temp_usage < 17250)
				{
					m_p1power.delivcurrent = temp_usage;
				}
				break;
			case P1TYPE_GASTIMESTAMP:
			case P1TYPE_GASTIMESTAMPME382:
				break;
			case P1TYPE_GASUSAGE:
			case P1TYPE_GASUSAGEDSMRv4:
			case P1TYPE_GASUSAGEGyrE350:
				temp_usage = (unsigned long)(atof(value)*1000.0f);
				m_p1gas.gasusage = temp_usage;
				break;
			}

			//_log.Log(LOG_NORM,"Key: %s, Value: %s", t.topic,value);

			return;
		}
	}
}

void P1MeterBase::ParseData(const unsigned char *pData, int Len)
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
			m_linecount++;
			if (m_bufferpos > 0) {
				m_buffer[m_bufferpos] = 0;
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
