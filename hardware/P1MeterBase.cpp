#include "stdafx.h"
#include "P1MeterBase.h"
#include "hardwaretypes.h"
#include "../main/SQLHelper.h"
#include "../main/localtime_r.h"
#include "../main/Logger.h"

#include <openssl/bio.h>
#include <openssl/evp.h>
/*
#include <cryptopp/aes.h>
#include <cryptopp/gcm.h>
#include <cryptopp/modes.h>
#include <cryptopp/filters.h>
*/

#define CRC16_ARC	0x8005
#define CRC16_ARC_REFL	0xA001

#define GCMTagLength 12
const std::string _szDecodeAdd = "3000112233445566778899AABBCCDDEEFF";

enum class _eP1MatchType {
	ID = 0,
	EXCLMARK,
	STD,
	DEVTYPE,
	GAS,
	LINE17,
	LINE18
};

#define P1SMID		"/"				// Smart Meter ID. Used to detect start of telegram.
#define P1VER		"1-3:0.2.8"		// P1 version
#define P1VERBE		"0-0:96.1.4"	// P1 version + e-MUCS version (Belgium)
#define P1TS		"0-0:1.0.0"		// Timestamp
#define P1PUSG		"1-0:1.8."		// total power usage (excluding tariff indicator)
#define P1PDLV		"1-0:2.8."		// total delivered power (excluding tariff indicator)
#define P1TIP		"0-0:96.14.0"	// tariff indicator power
#define P1PUC		"1-0:1.7.0"		// current power usage
#define P1PDC		"1-0:2.7.0"		// current power delivery
#define P1VOLTL1	"1-0:32.7.0"	// voltage L1 (DSMRv5)
#define P1VOLTL2	"1-0:52.7.0"	// voltage L2 (DSMRv5)
#define P1VOLTL3	"1-0:72.7.0"	// voltage L3 (DSMRv5)
#define P1AMPEREL1	"1-0:31.7.0"	// amperage L1 (DSMRv5)
#define P1AMPEREL2	"1-0:51.7.0"	// amperage L2 (DSMRv5)
#define P1AMPEREL3	"1-0:71.7.0"	// amperage L3 (DSMRv5)
#define P1POWUSL1	"1-0:21.7.0"    // Power used L1 (DSMRv5)
#define P1POWUSL2	"1-0:41.7.0"    // Power used L2 (DSMRv5)
#define P1POWUSL3	"1-0:61.7.0"    // Power used L3 (DSMRv5)
#define P1POWDLL1	"1-0:22.7.0"    // Power delivered L1 (DSMRv5)
#define P1POWDLL2	"1-0:42.7.0"    // Power delivered L2 (DSMRv5)
#define P1POWDLL3	"1-0:62.7.0"    // Power delivered L3 (DSMRv5)
#define P1GTS		"0-n:24.3.0"	// DSMR2 timestamp gas usage sample
#define P1GUDSMR2	"("				// DSMR2 gas usage sample
#define P1GUDSMR4	"0-n:24.2."		// DSMR4 gas usage sample (excluding 'tariff' indicator)
#define P1MBTYPE	"0-n:24.1.0"	// M-Bus device type
#define P1EOT		"!"				// End of telegram.

enum _eP1Type {
	P1TYPE_SMID = 0,
	P1TYPE_END,
	P1TYPE_VERSION,
	P1TYPE_POWERUSAGE,
	P1TYPE_POWERDELIV,
	P1TYPE_USAGECURRENT,
	P1TYPE_DELIVCURRENT,
	P1TYPE_VOLTAGEL1,
	P1TYPE_VOLTAGEL2,
	P1TYPE_VOLTAGEL3,
	P1TYPE_AMPERAGEL1,
	P1TYPE_AMPERAGEL2,
	P1TYPE_AMPERAGEL3,
	P1TYPE_POWERUSEL1,
	P1TYPE_POWERUSEL2,
	P1TYPE_POWERUSEL3,
	P1TYPE_POWERDELL1,
	P1TYPE_POWERDELL2,
	P1TYPE_POWERDELL3,
	P1TYPE_MBUSDEVICETYPE,
	P1TYPE_GASUSAGEDSMR4,
	P1TYPE_GASTIMESTAMP,
	P1TYPE_GASUSAGE
};

typedef struct {
	_eP1MatchType matchtype;
	_eP1Type type;
	const char* key;
	const char* topic;
	int start;
	int width;
} P1Match;

P1Match p1_matchlist[] = {
	{_eP1MatchType::ID,			P1TYPE_SMID,			P1SMID,		"",					0,  0},
	{_eP1MatchType::EXCLMARK,	P1TYPE_END,				P1EOT,		"",					0,  0},
	{_eP1MatchType::STD,		P1TYPE_VERSION,			P1VER,		"version",			10,  2},
	{_eP1MatchType::STD,		P1TYPE_VERSION,			P1VERBE,	"versionBE",		11,  5},
	{_eP1MatchType::STD,		P1TYPE_POWERUSAGE,		P1PUSG,		"powerusage",		10,  9},
	{_eP1MatchType::STD,		P1TYPE_POWERDELIV,		P1PDLV,		"powerdeliv",		10,  9},
	{_eP1MatchType::STD,		P1TYPE_USAGECURRENT,	P1PUC,		"powerusagec",		10,  7},
	{_eP1MatchType::STD,		P1TYPE_DELIVCURRENT,	P1PDC,		"powerdelivc",		10,  7},
	{_eP1MatchType::STD,		P1TYPE_VOLTAGEL1,		P1VOLTL1,	"voltagel1",		11,  5},
	{_eP1MatchType::STD,		P1TYPE_VOLTAGEL2,		P1VOLTL2,	"voltagel2",		11,  5},
	{_eP1MatchType::STD,		P1TYPE_VOLTAGEL3,		P1VOLTL3,	"voltagel3",		11,  5},
	{_eP1MatchType::STD,		P1TYPE_AMPERAGEL1,		P1AMPEREL1,	"amperagel1",		11,  3},
	{_eP1MatchType::STD,		P1TYPE_AMPERAGEL2,		P1AMPEREL2,	"amperagel2",		11,  3},
	{_eP1MatchType::STD,		P1TYPE_AMPERAGEL3,		P1AMPEREL3,	"amperagel3",		11,  3},
	{_eP1MatchType::STD,		P1TYPE_POWERUSEL1,		P1POWUSL1,	"powerusel1",		11,  6},
	{_eP1MatchType::STD,		P1TYPE_POWERUSEL2,		P1POWUSL2,	"powerusel2",		11,  6},
	{_eP1MatchType::STD,		P1TYPE_POWERUSEL3,		P1POWUSL3,	"powerusel3",		11,  6},
	{_eP1MatchType::STD,		P1TYPE_POWERDELL1,		P1POWDLL1,	"powerdell1",		11,  6},
	{_eP1MatchType::STD,		P1TYPE_POWERDELL2,		P1POWDLL2,	"powerdell2",		11,  6},
	{_eP1MatchType::STD,		P1TYPE_POWERDELL3,		P1POWDLL3,	"powerdell3",		11,  6},
	{_eP1MatchType::DEVTYPE,	P1TYPE_MBUSDEVICETYPE,	P1MBTYPE,	"mbusdevicetype",	11,  3},
	{_eP1MatchType::GAS,		P1TYPE_GASUSAGEDSMR4,	P1GUDSMR4,	"gasusage",	 		26,  8},
	{_eP1MatchType::LINE17,		P1TYPE_GASTIMESTAMP,	P1GTS,		"gastimestamp",		11, 12},
	{_eP1MatchType::LINE18,		P1TYPE_GASUSAGE,		P1GUDSMR2,	"gasusage",			1,  9}
}; // must keep DEVTYPE, GAS, LINE17 and LINE18 in this order at end of p1_matchlist

P1MeterBase::P1MeterBase(void)
{
	m_bDisableCRC = true;
	m_ratelimit = 0;
	Init();
}


P1MeterBase::~P1MeterBase(void)
{
	if (m_pDecryptBuffer)
		delete[] m_pDecryptBuffer;
}

void P1MeterBase::Init()
{
	m_p1version = 0;
	m_linecount = 0;
	m_exclmarkfound = 0;
	m_CRfound = 0;
	m_bufferpos = 0;
	m_lastgasusage = 0;
	m_lastSharedSendGas = 0;
	m_lastUpdateTime = 0;

	l_exclmarkfound = 0;
	l_bufferpos = 0;

	m_voltagel1 = -1;
	m_voltagel2 = -1;
	m_voltagel3 = -1;

	m_bReceivedAmperage = false;
	m_amperagel1 = 0;
	m_amperagel2 = 0;
	m_amperagel3 = 0;

	m_powerusel1 = -1;
	m_powerusel2 = -1;
	m_powerusel3 = -1;

	m_powerdell1 = -1;
	m_powerdell2 = -1;
	m_powerdell3 = -1;

	memset(&m_buffer, 0, sizeof(m_buffer));
	memset(&l_buffer, 0, sizeof(l_buffer));

	memset(&m_power, 0, sizeof(m_power));
	memset(&m_gas, 0, sizeof(m_gas));

	m_power.len = sizeof(P1Power) - 1;
	m_power.type = pTypeP1Power;
	m_power.subtype = sTypeP1Power;
	m_power.ID = 1;

	m_gas.len = sizeof(P1Gas) - 1;
	m_gas.type = pTypeP1Gas;
	m_gas.subtype = sTypeP1Gas;
	m_gas.ID = 1;
	m_gas.gasusage = 0;

	m_gasmbuschannel = 0;
	m_gasprefix = "0-n";
	m_gastimestamp = "";
	m_gasclockskew = 0;
	m_gasoktime = 0;

	std::vector<std::vector<std::string> > result;
	result = m_sql.safe_query("SELECT Value FROM UserVariables WHERE (Name='P1GasMeterChannel')");
	if (!result.empty())
	{
		std::string s_gasmbuschannel = result[0][0];
		if ((s_gasmbuschannel.length() == 1) && (s_gasmbuschannel[0] > 0x30) && (s_gasmbuschannel[0] < 0x35)) // value must be a single digit number between 1 and 4
		{
			m_gasmbuschannel = (char)s_gasmbuschannel[0];
			m_gasprefix[2] = m_gasmbuschannel;
			_log.Log(LOG_STATUS, "P1 Smart Meter: Gas meter M-Bus channel %c enforced by 'P1GasMeterChannel' user variable", m_gasmbuschannel);
		}
	}
	InitP1EncryptionState();
}

void P1MeterBase::InitP1EncryptionState()
{
	m_p1_encryption_state = P1EcryptionState::waitingForStartByte;
	m_currentBytePosition = 0;
	m_changeToNextStateAt = 0;
	m_dataLength = 0;
	m_systemTitle.clear();
	m_frameCounter = 0;
	m_dataPayload.clear();
	m_gcmTag.clear();
	m_gcmTag.reserve(GCMTagLength);
}


bool P1MeterBase::MatchLine()
{
	if ((strlen((const char*)&l_buffer) < 1) || (l_buffer[0] == 0x0a))
		return true; //null value (startup)
	uint8_t i;
	uint8_t found = 0;
	P1Match* t;
	char value[20] = "";
	std::string vString;

	for (i = 0; (i < sizeof(p1_matchlist) / sizeof(P1Match)) & (!found); i++)
	{
		t = &p1_matchlist[i];
		switch (t->matchtype)
		{
		case _eP1MatchType::ID:
			// start of data
			if (strncmp(t->key, (const char*)&l_buffer, strlen(t->key)) == 0)
			{
				m_linecount = 1;
				found = 1;
			}
			continue; // we do not process anything else on this line
			break;
		case _eP1MatchType::EXCLMARK:
			// end of data
			if (strncmp(t->key, (const char*)&l_buffer, strlen(t->key)) == 0)
			{
				l_exclmarkfound = 1;
				found = 1;
			}
			break;
		case _eP1MatchType::STD:
			if (strncmp(t->key, (const char*)&l_buffer, strlen(t->key)) == 0)
				found = 1;
			break;
		case _eP1MatchType::DEVTYPE:
			if (m_gasmbuschannel == 0)
			{
				vString = (const char*)t->key + 3;
				if (strncmp(vString.c_str(), (const char*)&l_buffer + 3, strlen(t->key) - 3) == 0)
					found = 1;
				else
					i += 100; // skip matches with any other gas lines - we need to find the M0-Bus channel first
			}
			break;
		case _eP1MatchType::GAS:
			if (strncmp((m_gasprefix + (t->key + 3)).c_str(), (const char*)&l_buffer, strlen(t->key)) == 0)
			{
				// verify that 'tariff' indicator is either 1 (Nld) or 3 (Bel)
				if ((l_buffer[9] & 0xFD) == 0x31)
					found = 1;
			}
			if (m_p1version >= 4)
				i += 100; // skip matches with any DSMR v2 gas lines
			break;
		case _eP1MatchType::LINE17:
			if (strncmp((m_gasprefix + (t->key + 3)).c_str(), (const char*)&l_buffer, strlen(t->key)) == 0)
			{
				m_linecount = 17;
				found = 1;
			}
			break;
		case _eP1MatchType::LINE18:
			if ((m_linecount == 18) && (strncmp(t->key, (const char*)&l_buffer, strlen(t->key)) == 0))
				found = 1;
			break;
		} //switch

		if (!found)
			continue;

		if (l_exclmarkfound)
		{
			if (m_p1version == 0)
			{
				_log.Log(LOG_STATUS, "P1 Smart Meter: Meter is pre DSMR 4.0 - using DSMR 2.2 compatibility");
				m_p1version = 2;
			}
			time_t atime = mytime(NULL);
			if (difftime(atime, m_lastUpdateTime) >= m_ratelimit)
			{
				m_lastUpdateTime = atime;
				sDecodeRXMessage(this, (const unsigned char*)&m_power, "Power", 255);
				if (m_voltagel1 != -1) {
					SendVoltageSensor(0, 1, 255, m_voltagel1, "Voltage L1");
				}
				if (m_voltagel2 != -1) {
					SendVoltageSensor(0, 2, 255, m_voltagel2, "Voltage L2");
				}
				if (m_voltagel3 != -1) {
					SendVoltageSensor(0, 3, 255, m_voltagel3, "Voltage L3");
				}
				/* The ampere is rounded to whole numbers and therefor not accurate enough
				//we could calculate this ourselfs I=P/U I1=(m_power.powerusage1/m_voltagel1)
				if (m_bReceivedAmperage) {
					SendCurrentSensor(1, 255, m_amperagel1, m_amperagel2, m_amperagel3, "Amperage" );
				}
				*/
				if (m_powerusel1 != -1) {
					SendWattMeter(0, 1, 255, m_powerusel1, "Usage L1");
				}
				if (m_powerusel2 != -1) {
					SendWattMeter(0, 2, 255, m_powerusel2, "Usage L2");
				}
				if (m_powerusel3 != -1) {
					SendWattMeter(0, 3, 255, m_powerusel3, "Usage L3");
				}

				if (m_powerdell1 != -1) {
					SendWattMeter(0, 4, 255, m_powerdell1, "Delivery L1");
				}
				if (m_powerdell2 != -1) {
					SendWattMeter(0, 5, 255, m_powerdell2, "Delivery L2");
				}
				if (m_powerdell3 != -1) {
					SendWattMeter(0, 6, 255, m_powerdell3, "Delivery L3");
				}

				if ((m_gas.gasusage > 0) && ((m_gas.gasusage != m_lastgasusage) || (difftime(atime, m_lastSharedSendGas) >= 300)))
				{
					//only update gas when there is a new value, or 5 minutes are passed
					if (m_gasclockskew >= 300)
					{
						// just accept it - we cannot sync to our clock
						m_lastSharedSendGas = atime;
						m_lastgasusage = m_gas.gasusage;
						sDecodeRXMessage(this, (const unsigned char*)&m_gas, "Gas", 255);
					}
					else if (atime >= m_gasoktime)
					{
						struct tm ltime;
						localtime_r(&atime, &ltime);
						char myts[80];
						sprintf(myts, "%02d%02d%02d%02d%02d%02dW", ltime.tm_year % 100, ltime.tm_mon + 1, ltime.tm_mday, ltime.tm_hour, ltime.tm_min, ltime.tm_sec);
						if (ltime.tm_isdst)
							myts[12] = 'S';
						if ((m_gastimestamp.length() > 13) || (strncmp((const char*)&myts, m_gastimestamp.c_str(), m_gastimestamp.length()) >= 0))
						{
							m_lastSharedSendGas = atime;
							m_lastgasusage = m_gas.gasusage;
							m_gasoktime += 300;
							sDecodeRXMessage(this, (const unsigned char*)&m_gas, "Gas", 255);
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
							if (m_gastimestamp.length() == 12)
								gastm.tm_isdst = -1;
							else if (m_gastimestamp[12] == 'W')
								gastm.tm_isdst = 0;
							else
								gastm.tm_isdst = 1;

							time_t gtime = mktime(&gastm);
							m_gasclockskew = difftime(gtime, atime);
							if (m_gasclockskew >= 300)
							{
								_log.Log(LOG_ERROR, "P1 Smart Meter: Unable to synchronize to the gas meter clock because it is more than 5 minutes ahead of my time");
							}
							else {
								m_gasoktime = gtime;
								_log.Log(LOG_STATUS, "P1 Smart Meter: Gas meter clock is %i seconds ahead - wait for my clock to catch up", (int)m_gasclockskew);
							}
						}
					}
				}
			}
			m_linecount = 0;
			l_exclmarkfound = 0;
		}
		else
		{
			vString = (const char*)&l_buffer + t->start;
			size_t ePos = t->width;
			ePos = vString.find_first_of("*)");

			if (ePos == std::string::npos)
			{
				// invalid message: value not delimited
				_log.Log(LOG_NORM, "P1 Smart Meter: Dismiss incoming - value is not delimited in line \"%s\"", l_buffer);
				return false;
			}

			if (ePos > 19)
			{
				// invalid message: line too long
				_log.Log(LOG_NORM, "P1 Smart Meter: Dismiss incoming - value in line \"%s\" is oversized", l_buffer);
				return false;
			}

			if (ePos > 0)
			{
				strcpy(value, vString.substr(0, ePos).c_str());
#ifdef _DEBUG
				_log.Log(LOG_NORM, "P1 Smart Meter: Key: %s, Value: %s", t->topic, value);
#endif
			}

			unsigned long temp_usage = 0;
			float temp_volt = 0;
			float temp_ampere = 0;
			float temp_power = 0;
			char* validate = value + ePos;

			switch (t->type)
			{
			case P1TYPE_VERSION:
				if (m_p1version == 0)
				{
					m_p1version = value[0] - 0x30;
					char szVersion[12];
					if (t->width == 5)
					{
						// Belgian meter
						sprintf(szVersion, "ESMR %c.%c.%c", value[0], value[1], value[2]);
					}
					else // if (t->width == 2)
					{
						// Dutch meter
						sprintf(szVersion, "ESMR %c.%c", value[0], value[1]);
						if (m_p1version < 5)
							szVersion[0] = 'D';
					}
					_log.Log(LOG_STATUS, "P1 Smart Meter: Meter reports as %s", szVersion);
				}
				break;
			case P1TYPE_MBUSDEVICETYPE:
				temp_usage = (unsigned long)(strtod(value, &validate));
				if (temp_usage == 3)
				{
					m_gasmbuschannel = (char)l_buffer[2];
					m_gasprefix[2] = m_gasmbuschannel;
					_log.Log(LOG_STATUS, "P1 Smart Meter: Found gas meter on M-Bus channel %c", m_gasmbuschannel);
				}
				break;
			case P1TYPE_POWERUSAGE:
				temp_usage = (unsigned long)(strtod(value, &validate) * 1000.0f);
				if ((l_buffer[8] & 0xFE) == 0x30)
				{
					// map tariff IDs 0 (Lux) and 1 (Bel, Nld) both to powerusage1
					if (!m_power.powerusage1 || m_p1version >= 4)
						m_power.powerusage1 = temp_usage;
					else if (temp_usage - m_power.powerusage1 < 10000)
						m_power.powerusage1 = temp_usage;
				}
				else if (l_buffer[8] == 0x32)
				{
					if (!m_power.powerusage2 || m_p1version >= 4)
						m_power.powerusage2 = temp_usage;
					else if (temp_usage - m_power.powerusage2 < 10000)
						m_power.powerusage2 = temp_usage;
				}
				break;
			case P1TYPE_POWERDELIV:
				temp_usage = (unsigned long)(strtod(value, &validate) * 1000.0f);
				if ((l_buffer[8] & 0xFE) == 0x30)
				{
					// map tariff IDs 0 (Lux) and 1 (Bel, Nld) both to powerdeliv1
					if (!m_power.powerdeliv1 || m_p1version >= 4)
						m_power.powerdeliv1 = temp_usage;
					else if (temp_usage - m_power.powerdeliv1 < 10000)
						m_power.powerdeliv1 = temp_usage;
				}
				else if (l_buffer[8] == 0x32)
				{
					if (!m_power.powerdeliv2 || m_p1version >= 4)
						m_power.powerdeliv2 = temp_usage;
					else if (temp_usage - m_power.powerdeliv2 < 10000)
						m_power.powerdeliv2 = temp_usage;
				}
				break;
			case P1TYPE_USAGECURRENT:
				temp_usage = (unsigned long)(strtod(value, &validate) * 1000.0f);	//Watt
				if (temp_usage < 17250)
					m_power.usagecurrent = temp_usage;
				break;
			case P1TYPE_DELIVCURRENT:
				temp_usage = (unsigned long)(strtod(value, &validate) * 1000.0f);	//Watt;
				if (temp_usage < 17250)
					m_power.delivcurrent = temp_usage;
				break;
			case P1TYPE_VOLTAGEL1:
				temp_volt = strtof(value, &validate);
				if (temp_volt < 300)
					m_voltagel1 = temp_volt; //Voltage L1;
				break;
			case P1TYPE_VOLTAGEL2:
				temp_volt = strtof(value, &validate);
				if (temp_volt < 300)
					m_voltagel2 = temp_volt; //Voltage L2;
				break;
			case P1TYPE_VOLTAGEL3:
				temp_volt = strtof(value, &validate);
				if (temp_volt < 300)
					m_voltagel3 = temp_volt; //Voltage L3;
				break;
			case P1TYPE_AMPERAGEL1:
				temp_ampere = strtof(value, &validate);
				if (temp_ampere < 100)
				{
					m_amperagel1 = temp_ampere; //Amperage L1;
					m_bReceivedAmperage = true;
				}
				break;
			case P1TYPE_AMPERAGEL2:
				temp_ampere = strtof(value, &validate);
				if (temp_ampere < 100)
				{
					m_amperagel2 = temp_ampere; //Amperage L2;
					m_bReceivedAmperage = true;
				}
				break;
			case P1TYPE_AMPERAGEL3:
				temp_ampere = strtof(value, &validate);
				if (temp_ampere < 100)
				{
					m_amperagel3 = temp_ampere; //Amperage L3;
					m_bReceivedAmperage = true;
				}
				break;
			case P1TYPE_POWERUSEL1:
				temp_power = static_cast<float>(strtod(value, &validate) * 1000.0f);
				if (temp_power < 10000)
					m_powerusel1 = temp_power; //Power Used L1;
				break;
			case P1TYPE_POWERUSEL2:
				temp_power = static_cast<float>(strtod(value, &validate) * 1000.0f);
				if (temp_power < 10000)
					m_powerusel2 = temp_power; //Power Used L2;
				break;
			case P1TYPE_POWERUSEL3:
				temp_power = static_cast<float>(strtod(value, &validate) * 1000.0f);
				if (temp_power < 10000)
					m_powerusel3 = temp_power; //Power Used L3;
				break;
			case P1TYPE_POWERDELL1:
				temp_power = static_cast<float>(strtod(value, &validate) * 1000.0f);
				if (temp_power < 10000)
					m_powerdell1 = temp_power; //Power Used L1;
				break;
			case P1TYPE_POWERDELL2:
				temp_power = static_cast<float>(strtod(value, &validate) * 1000.0f);
				if (temp_power < 10000)
					m_powerdell2 = temp_power; //Power Used L2;
				break;
			case P1TYPE_POWERDELL3:
				temp_power = static_cast<float>(strtod(value, &validate) * 1000.0f);
				if (temp_power < 10000)
					m_powerdell3 = temp_power; //Power Used L3;
				break;
			case P1TYPE_GASTIMESTAMP:
				m_gastimestamp = std::string(value);
				break;
			case P1TYPE_GASUSAGE:
			case P1TYPE_GASUSAGEDSMR4:
				temp_usage = (unsigned long)(strtod(value, &validate) * 1000.0f);
				if (!m_gas.gasusage || m_p1version >= 4)
					m_gas.gasusage = temp_usage;
				else if (temp_usage - m_gas.gasusage < 20000)
					m_gas.gasusage = temp_usage;
				break;
			}

			if (ePos > 0 && ((validate - value) != ePos))
			{
				// invalid message: value is not a number
				_log.Log(LOG_NORM, "P1 Smart Meter: Dismiss incoming - value in line \"%s\" is not a number", l_buffer);
				return false;
			}

			if (t->type == P1TYPE_GASUSAGEDSMR4)
			{
				// need to get timestamp from this line as well
				vString = (const char*)&l_buffer + 11;
				m_gastimestamp = vString.substr(0, 13);
#ifdef _DEBUG
				_log.Log(LOG_NORM, "P1 Smart Meter: Key: gastimestamp, Value: %s", m_gastimestamp.c_str());
#endif
			}
		}
	}
	return true;
}


/*
/ 	DSMR 4.0 defines a CRC checksum at the end of the message, calculated from
/	and including the message starting character '/' upto and including the message
/	end character '!'. According to the specs the CRC is a 16bit checksum using the
/	polynomial x^16 + x^15 + x^2 + 1, however input/output are reflected.
*/

bool P1MeterBase::CheckCRC()
{
	// sanity checks
	if (l_buffer[1] == 0)
	{
		if (m_p1version == 0)
		{
			_log.Log(LOG_STATUS, "P1 Smart Meter: Meter is pre DSMR 4.0 and does not send a CRC checksum - using DSMR 2.2 compatibility");
			m_p1version = 2;
		}
		// always return true with pre DSMRv4 format message
		return true;
	}

	if (l_buffer[5] != 0)
	{
		// trailing characters after CRC
		_log.Log(LOG_NORM, "P1 Smart Meter: Dismiss incoming - CRC value in message has trailing characters");
		return false;
	}

	if (!m_CRfound)
	{
		_log.Log(LOG_NORM, "P1 Smart Meter: You appear to have middleware that changes the message content - skipping CRC validation");
		return true;
	}

	// retrieve CRC from the current line
	char crc_str[5];
	strncpy(crc_str, (const char*)&l_buffer + 1, 4);
	crc_str[4] = 0;
	uint16_t m_crc16 = (uint16_t)strtoul(crc_str, NULL, 16);

	// calculate CRC
	const unsigned char* c_buffer = m_buffer;
	uint8_t i;
	uint16_t crc = 0;
	int m_size = m_bufferpos;
	while (m_size--)
	{
		crc = crc ^ *c_buffer++;
		for (i = 0; i < 8; i++)
		{
			if ((crc & 0x0001))
			{
				crc = (crc >> 1) ^ CRC16_ARC_REFL;
			}
			else {
				crc = crc >> 1;
			}
		}
	}
	if (crc != m_crc16)
	{
		_log.Log(LOG_NORM, "P1 Smart Meter: Dismiss incoming - CRC failed");
	}
	return (crc == m_crc16);
}


bool P1MeterBase::ParseP1EncryptedData(const uint8_t p1_byte)
{
	switch (m_p1_encryption_state)
	{
	case P1EcryptionState::waitingForStartByte:
		if (p1_byte == 0xDB)
		{
			InitP1EncryptionState();
			m_p1_encryption_state = P1EcryptionState::readSystemTitleLength;
		}
		break;
	case P1EcryptionState::readSystemTitleLength:
		m_p1_encryption_state = P1EcryptionState::readSystemTitle;
		// 2 start bytes (position 0 and 1) + system title length
		m_changeToNextStateAt = 1 + int(p1_byte);
		m_systemTitle.clear();
		break;
	case P1EcryptionState::readSystemTitle:
		m_systemTitle.insert(m_systemTitle.end(), 1, p1_byte);
		if (m_currentBytePosition >= m_changeToNextStateAt)
		{
			m_p1_encryption_state = P1EcryptionState::readSeparator82;
			m_changeToNextStateAt++;
		}
		break;
	case P1EcryptionState::readSeparator82:
		if (p1_byte == 0x82)
		{
			m_p1_encryption_state = P1EcryptionState::readPayloadLength; // Ignore separator byte
			m_changeToNextStateAt += 2;
		}
		else {
			//Missing separator (0x82). Dropping telegram
			m_p1_encryption_state = P1EcryptionState::waitingForStartByte;
		}
		break;
	case P1EcryptionState::readPayloadLength:
		m_dataLength <<= 8;
		m_dataLength |= int(p1_byte);
		if (m_dataLength > 2000)
		{
			//something is not right here
			m_p1_encryption_state = P1EcryptionState::readSystemTitleLength;
		}
		else if (m_currentBytePosition >= m_changeToNextStateAt)
		{
			m_p1_encryption_state = P1EcryptionState::readSeparator30;
			m_changeToNextStateAt++;
		}
		break;
	case P1EcryptionState::readSeparator30:
		if (p1_byte == 0x30)
		{
			m_p1_encryption_state = P1EcryptionState::readFrameCounter;
			// 4 bytes for frame counter
			m_changeToNextStateAt += 4;
		}
		else {
			//Missing separator (0x30). Dropping telegram
			m_p1_encryption_state = P1EcryptionState::waitingForStartByte;
		}
		break;
	case P1EcryptionState::readFrameCounter:
		m_frameCounter <<= 8;
		m_frameCounter |= p1_byte;
		if (m_currentBytePosition >= m_changeToNextStateAt)
		{
			m_p1_encryption_state = P1EcryptionState::readPayload;
			m_changeToNextStateAt += m_dataLength - 17;
			m_dataPayload.reserve(m_dataLength - 17);
		}
		break;
	case P1EcryptionState::readPayload:
		m_dataPayload.insert(m_dataPayload.end(), 1, p1_byte);
		if (m_currentBytePosition >= m_changeToNextStateAt)
		{
			m_p1_encryption_state = P1EcryptionState::readGcmTag;
			m_changeToNextStateAt += GCMTagLength;
		}
		break;
	case P1EcryptionState::readGcmTag:
		// All input has been read.
		m_gcmTag.insert(m_gcmTag.end(), 1, p1_byte);
		if (m_currentBytePosition >= m_changeToNextStateAt)
		{
			m_p1_encryption_state = P1EcryptionState::doneReadingTelegram;
		}
		break;
	}
	m_currentBytePosition++;
	if (m_p1_encryption_state == P1EcryptionState::doneReadingTelegram)
	{
		m_p1_encryption_state = P1EcryptionState::waitingForStartByte;
		return true;
	}
	return false;
}


/*
/ 	ParseP1Data() can be called with either a complete message (P1MeterTCP) or individual
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

void P1MeterBase::ParseP1Data(const uint8_t* pDataIn, const int LenIn, const bool disable_crc, int ratelimit)
{
	const uint8_t* pData = pDataIn;
	int Len = LenIn;

	if (m_bIsEncrypted)
	{
		int tLen = 0;
		while (tLen < LenIn)
		{
			if (ParseP1EncryptedData(pDataIn[tLen]))
			{
				try
				{
					//We have a complete Telegram
					std::string iv, cipherText;
					iv.reserve(m_systemTitle.size() + 4);
					cipherText.reserve(m_dataPayload.size() + GCMTagLength);

					iv.append(m_systemTitle.begin(), m_systemTitle.end());
					iv.append(1, (m_frameCounter & 0xFF000000) >> 24);
					iv.append(1, (m_frameCounter & 0x00FF0000) >> 16);
					iv.append(1, (m_frameCounter & 0x0000FF00) >> 8);
					iv.append(1, m_frameCounter & 0x000000FF);

					cipherText.append(m_dataPayload.begin(), m_dataPayload.end());
					cipherText.append(m_gcmTag.begin(), m_gcmTag.end());

					size_t neededDecryptBufferSize = std::min(1000, static_cast<int>(cipherText.size() + 16));
					if (neededDecryptBufferSize > m_DecryptBufferSize)
					{
						if (m_pDecryptBuffer)
							delete[] m_pDecryptBuffer;

						m_DecryptBufferSize = neededDecryptBufferSize;
						m_pDecryptBuffer = new uint8_t[m_DecryptBufferSize];
						if (m_pDecryptBuffer == nullptr)
							return;
					}
					memset(m_pDecryptBuffer, 0, m_DecryptBufferSize);

					EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
					if (ctx == nullptr)
						return;
					EVP_DecryptInit_ex(ctx, EVP_aes_128_gcm(), NULL, NULL, NULL);
					EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_AEAD_SET_IVLEN, iv.size(), NULL);

					EVP_DecryptInit_ex(ctx, NULL, NULL, (const unsigned char*)m_szHexKey.data(), (const unsigned char*)iv.c_str());

					int outlen = 0;
					//std::vector<char> m_szDecodeAdd = HexToBytes(_szDecodeAdd);
					//EVP_DecryptUpdate(ctx, NULL, &outlen, (const uint8_t*)m_szDecodeAdd.data(), m_szDecodeAdd.size());
					EVP_DecryptUpdate(ctx, (uint8_t*)m_pDecryptBuffer, &outlen, (uint8_t*)cipherText.c_str(), cipherText.size());
					EVP_CIPHER_CTX_free(ctx);
					if (outlen <= 0)
						return;
/*
					CryptoPP::GCM< CryptoPP::AES >::Decryption decryptor;
					decryptor.SetKeyWithIV((uint8_t*)m_szHexKey.data(), 16, (uint8_t*)iv.c_str(), 12);
					decryptor.ProcessData(m_pDecryptBuffer, (uint8_t*)cipherText.c_str(), cipherText.size());
*/
					pData = m_pDecryptBuffer;
					Len = static_cast<int>(strlen((const char*)m_pDecryptBuffer));
				}
				catch (const std::exception& e)
				{
					_log.Log(LOG_ERROR, "P1Meter: Error decrypting payload (%s)", e.what());
				}
				break;
			}
			tLen++;
		}
		if (tLen >= LenIn)
			return; //not ready with payload
	}

	int ii = 0;
	m_ratelimit = ratelimit;
	// a new message should not start with an empty line, but just in case it does (crude check is sufficient here)
	while ((m_linecount == 0) && (pData[ii] < 0x10))
	{
		ii++;
	}

	// re enable reading pData when a new message starts, empty buffers
	if (pData[ii] == 0x2f)
	{
		if ((l_buffer[0] == 0x21) && !l_exclmarkfound && (m_linecount > 0))
		{
			_log.Log(LOG_STATUS, "P1 Smart Meter: WARNING: got new message but buffer still contains unprocessed data from previous message.");
			l_buffer[l_bufferpos] = 0;
			if (disable_crc || CheckCRC())
			{
				MatchLine();
			}
		}
		m_linecount = 1;
		l_bufferpos = 0;
		m_bufferpos = 0;
		m_exclmarkfound = 0;
	}

	// assemble complete message in message buffer
	while ((ii < Len) && (m_linecount > 0) && (!m_exclmarkfound) && (m_bufferpos < sizeof(m_buffer)))
	{
		const unsigned char c = pData[ii];
		m_buffer[m_bufferpos] = c;
		m_bufferpos++;
		if (c == 0x21)
		{
			// stop reading at exclamation mark (do not include CRC)
			ii = Len;
			m_exclmarkfound = 1;
		}
		else {
			ii++;
		}
	}

	if (m_bufferpos == sizeof(m_buffer))
	{
		// discard oversized message
		if ((Len > 400) || (pData[0] == 0x21))
		{
			// 400 is an arbitrary chosen number to differentiate between full messages and single line commits
			_log.Log(LOG_NORM, "P1 Smart Meter: Dismiss incoming - message oversized");
		}
		m_linecount = 0;
		return;
	}

	// read pData, ignore/stop if there is a message validation failure
	ii = 0;
	while ((ii < Len) && (m_linecount > 0))
	{
		const unsigned char c = pData[ii];
		ii++;
		if (c == 0x0d)
		{
			m_CRfound = 1;
			continue;
		}

		if (c == 0x0a)
		{
			// close string, parse line and clear it.
			m_linecount++;
			if ((l_bufferpos > 0) && (l_bufferpos < sizeof(l_buffer)))
			{
				// don't try to match empty or oversized lines
				l_buffer[l_bufferpos] = 0;
				if (l_buffer[0] == 0x21 && !disable_crc)
				{
					if (!CheckCRC())
					{
						m_linecount = 0;
						return;
					}
				}
				if (!MatchLine())
				{
					// discard message
					m_linecount = 0;
				}
			}
			l_bufferpos = 0;
		}
		else if (l_bufferpos < sizeof(l_buffer))
		{
			l_buffer[l_bufferpos] = c;
			l_bufferpos++;
		}
	}
}
