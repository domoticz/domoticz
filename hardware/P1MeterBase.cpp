#include "stdafx.h"
#include "P1MeterBase.h"
#include "hardwaretypes.h"
#include "../main/SQLHelper.h"
#include "../main/Logger.h"

#include <openssl/bio.h>
#include <openssl/evp.h>

#define DEFAULT_RATE_LIMIT_P1 5

#define CRC16_ARC	0x8005
#define CRC16_ARC_REFL	0xA001

#define GCMTagLength 12
const std::string _szDecodeAdd = "3000112233445566778899AABBCCDDEEFF";

enum class _eP1MatchType {
	ID = 0,
	EXCLMARK,
	STD,
	DEVTYPE,
	MBUS,
	LINE17,
	LINE18
};

#define P1MAXTOTALPOWER 55200		// Define Max total Power possible (80A * 3fase * 230V)
#define P1MAXPHASEPOWER 18400		// Define Max phase Power possible (80A * 3fase * 230V)

#define P1SMID		"/"				// Smart Meter ID. Used to detect start of telegram.
#define P1VER		"1-3:0.2.8"		// P1 version
#define P1VERBE		"0-0:96.1.4"	// P1 version + e-MUCS version (Belgium)
#define P1TS		"0-0:1.0.0"		// Timestamp
#define P1PUSG		"1-0:1.8."		// total power usage (excluding tariff indicator)
#define P1PDLV		"1-0:2.8."		// total delivered power (excluding tariff indicator)
#define P1TIP		"0-0:96.14.0"	// tariff indicator power
#define P1PUC		"1-0:1.7.0"		// current power usage
#define P1PDC		"1-0:2.7.0"		// current power delivery
#define P1NOPF		"0-0:96.7.21"	// Number of power failures in any phases
#define P1NOLPF		"0-0:96.7.9"	// Number of power failures in any phases
#define P1NOVSGL1	"1-0:32.32.0"	// Number of voltage sags in phase L1
#define P1NOVSGL2	"1-0:52.32.0"	// Number of voltage sags in phase L2
#define P1NOVSGL3	"1-0:72.32.0"	// Number of voltage sags in phase L3
#define P1NOVSWL1	"1-0:32.36.0"	// Number of voltage swells in phase L1
#define P1NOVSWL2	"1-0:52.36.0"	// Number of voltage swells in phase L2
#define P1NOVSWL3	"1-0:72.36.0"	// Number of voltage swells in phase L3
#define P1VOLTL1	"1-0:32.7.0"	// voltage L1 (DSMRv5)
#define P1VOLTL2	"1-0:52.7.0"	// voltage L2 (DSMRv5)
#define P1VOLTL3	"1-0:72.7.0"	// voltage L3 (DSMRv5)
#define P1AMPEREL1	"1-0:31.7.0"	// amperage L1 (DSMRv5)
#define P1AMPEREL2	"1-0:51.7.0"	// amperage L2 (DSMRv5)
#define P1AMPEREL3	"1-0:71.7.0"	// amperage L3 (DSMRv5)
#define P1POWUSL1	"1-0:21.7.0"	// Power used L1 (DSMRv5)
#define P1POWUSL2	"1-0:41.7.0"	// Power used L2 (DSMRv5)
#define P1POWUSL3	"1-0:61.7.0"	// Power used L3 (DSMRv5)
#define P1POWDLL1	"1-0:22.7.0"	// Power delivered L1 (DSMRv5)
#define P1POWDLL2	"1-0:42.7.0"	// Power delivered L2 (DSMRv5)
#define P1POWDLL3	"1-0:62.7.0"	// Power delivered L3 (DSMRv5)
#define P1GTS		"0-n:24.3.0"	// DSMR2 timestamp gas usage sample
#define P1GUDSMR2	"("				// DSMR2 gas usage sample
#define P1MUDSMR4	"0-n:24.2."		// DSMR4 mbus value (excluding 'tariff' indicator)
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
	P1TYPE_NUMPWRFAIL,
	P1TYPE_NUMLONGPWRFAIL,
	P1TYPE_NUMVOLTSAGSL1,
	P1TYPE_NUMVOLTSAGSL2,
	P1TYPE_NUMVOLTSAGSL3,
	P1TYPE_NUMVOLTSWELLSL1,
	P1TYPE_NUMVOLTSWELLSL2,
	P1TYPE_NUMVOLTSWELLSL3,
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
	P1TYPE_MBUSUSAGEDSMR4,
	P1TYPE_GASTIMESTAMP,
	P1TYPE_GASUSAGE
};

using P1Match = struct
{
	_eP1MatchType matchtype;
	_eP1Type type;
	const char* key;
	const char* topic;
	uint8_t start;
	uint8_t width;
};

constexpr std::array<P1Match, 32> p1_matchlist{
	{
		{ _eP1MatchType::ID, P1TYPE_SMID, P1SMID, "", 0, 0 },
		{ _eP1MatchType::EXCLMARK, P1TYPE_END, P1EOT, "", 0, 0 },
		{ _eP1MatchType::STD, P1TYPE_VERSION, P1VER, "version", 10, 2 },
		{ _eP1MatchType::STD, P1TYPE_VERSION, P1VERBE, "versionBE", 11, 5 },
		{ _eP1MatchType::STD, P1TYPE_POWERUSAGE, P1PUSG, "powerusage", 10, 9 },
		{ _eP1MatchType::STD, P1TYPE_POWERDELIV, P1PDLV, "powerdeliv", 10, 9 },
		{ _eP1MatchType::STD, P1TYPE_USAGECURRENT, P1PUC, "powerusagec", 10, 7 },
		{ _eP1MatchType::STD, P1TYPE_DELIVCURRENT, P1PDC, "powerdelivc", 10, 7 },
		{ _eP1MatchType::STD, P1TYPE_NUMPWRFAIL, P1NOPF, "numpwrfail", 12, 5 },
		{ _eP1MatchType::STD, P1TYPE_NUMLONGPWRFAIL, P1NOLPF, "numlongpwrfail", 11, 5 },
		{ _eP1MatchType::STD, P1TYPE_NUMVOLTSAGSL1, P1NOVSGL1, "numvoltsagsl1", 12, 5 },
		{ _eP1MatchType::STD, P1TYPE_NUMVOLTSAGSL2, P1NOVSGL2, "numvoltsagsl1", 12, 5 },
		{ _eP1MatchType::STD, P1TYPE_NUMVOLTSAGSL3, P1NOVSGL3, "numvoltsagsl1", 12, 5 },
		{ _eP1MatchType::STD, P1TYPE_NUMVOLTSWELLSL1, P1NOVSWL1, "numvoltswellsl1", 12, 5 },
		{ _eP1MatchType::STD, P1TYPE_NUMVOLTSWELLSL2, P1NOVSWL2, "numvoltswellsl2", 12, 5 },
		{ _eP1MatchType::STD, P1TYPE_NUMVOLTSWELLSL3, P1NOVSWL3, "numvoltswellsl3", 12, 5 },
		{ _eP1MatchType::STD, P1TYPE_VOLTAGEL1, P1VOLTL1, "voltagel1", 11, 5 },
		{ _eP1MatchType::STD, P1TYPE_VOLTAGEL2, P1VOLTL2, "voltagel2", 11, 5 },
		{ _eP1MatchType::STD, P1TYPE_VOLTAGEL3, P1VOLTL3, "voltagel3", 11, 5 },
		{ _eP1MatchType::STD, P1TYPE_AMPERAGEL1, P1AMPEREL1, "amperagel1", 11, 3 },
		{ _eP1MatchType::STD, P1TYPE_AMPERAGEL2, P1AMPEREL2, "amperagel2", 11, 3 },
		{ _eP1MatchType::STD, P1TYPE_AMPERAGEL3, P1AMPEREL3, "amperagel3", 11, 3 },
		{ _eP1MatchType::STD, P1TYPE_POWERUSEL1, P1POWUSL1, "powerusel1", 11, 6 },
		{ _eP1MatchType::STD, P1TYPE_POWERUSEL2, P1POWUSL2, "powerusel2", 11, 6 },
		{ _eP1MatchType::STD, P1TYPE_POWERUSEL3, P1POWUSL3, "powerusel3", 11, 6 },
		{ _eP1MatchType::STD, P1TYPE_POWERDELL1, P1POWDLL1, "powerdell1", 11, 6 },
		{ _eP1MatchType::STD, P1TYPE_POWERDELL2, P1POWDLL2, "powerdell2", 11, 6 },
		{ _eP1MatchType::STD, P1TYPE_POWERDELL3, P1POWDLL3, "powerdell3", 11, 6 },
		{ _eP1MatchType::DEVTYPE, P1TYPE_MBUSDEVICETYPE, P1MBTYPE, "mbusdevicetype", 11, 3 },
		{ _eP1MatchType::MBUS, P1TYPE_MBUSUSAGEDSMR4, P1MUDSMR4, "mbus_meter", 26, 8 },
		// must keep DEVTYPE, GAS, LINE17 and LINE18 in this order at end of p1_matchlist
		{ _eP1MatchType::LINE17, P1TYPE_GASTIMESTAMP, P1GTS, "gastimestamp", 11, 12 },
		{ _eP1MatchType::LINE18, P1TYPE_GASUSAGE, P1GUDSMR2, "gasusage", 1, 9 },
	}
};

struct P1MBusType
{
	P1MeterBase::P1MBusType type = P1MeterBase::P1MBusType::deviceType_Unknown;
	const char* name;
};

constexpr std::array<P1MBusType, 8> p1_supported_mbus_list{
	{
		{ P1MeterBase::P1MBusType::deviceType_Electricity , "Electricity" },
		{ P1MeterBase::P1MBusType::deviceType_Gas , "Gas" },
		//{ P1MeterBase::P1MBusType::deviceType_Heat, "Heat" },
		{ P1MeterBase::P1MBusType::deviceType_WarmWater, "Warm Water" },
		{ P1MeterBase::P1MBusType::deviceType_Water, "Water" },
		//{ P1MeterBase::P1MBusType::deviceType_Heat_Cost_Allocator, "Heat Cost Allocator" },
		//{ P1MeterBase::P1MBusType::deviceType_Cooling_RT, "Cooling_RT" },
		//{ P1MeterBase::P1MBusType::deviceType_Cooling_FT, "Cooling_FT" },
		//{ P1MeterBase::P1MBusType::deviceType_Heat_FT, "Heat_FT" },
		//{ P1MeterBase::P1MBusType::deviceType_CombinedHeat_Cooling, "CombinedHeat_Cooling" },
		{ P1MeterBase::P1MBusType::deviceType_HotWater, "Hot Water" },
		{ P1MeterBase::P1MBusType::deviceType_ColdWater, "Cold Water" },
		//{ P1MeterBase::P1MBusType::deviceType_Breaker_electricity, "Breaker_electricity" },
		{ P1MeterBase::P1MBusType::deviceType_Valve_Gas_or_water, "Valve_Gas_or_water" },
		{ P1MeterBase::P1MBusType::deviceType_WasteWater, "Waste Water" },
	}
};

constexpr std::array<uint16_t, 256> p1_crc_16{
	0x0000, 0xC0C1, 0xC181, 0x0140, 0xC301, 0x03C0, 0x0280, 0xC241,
	0xC601, 0x06C0, 0x0780, 0xC741, 0x0500, 0xC5C1, 0xC481, 0x0440,
	0xCC01, 0x0CC0, 0x0D80, 0xCD41, 0x0F00, 0xCFC1, 0xCE81, 0x0E40,
	0x0A00, 0xCAC1, 0xCB81, 0x0B40, 0xC901, 0x09C0, 0x0880, 0xC841,
	0xD801, 0x18C0, 0x1980, 0xD941, 0x1B00, 0xDBC1, 0xDA81, 0x1A40,
	0x1E00, 0xDEC1, 0xDF81, 0x1F40, 0xDD01, 0x1DC0, 0x1C80, 0xDC41,
	0x1400, 0xD4C1, 0xD581, 0x1540, 0xD701, 0x17C0, 0x1680, 0xD641,
	0xD201, 0x12C0, 0x1380, 0xD341, 0x1100, 0xD1C1, 0xD081, 0x1040,
	0xF001, 0x30C0, 0x3180, 0xF141, 0x3300, 0xF3C1, 0xF281, 0x3240,
	0x3600, 0xF6C1, 0xF781, 0x3740, 0xF501, 0x35C0, 0x3480, 0xF441,
	0x3C00, 0xFCC1, 0xFD81, 0x3D40, 0xFF01, 0x3FC0, 0x3E80, 0xFE41,
	0xFA01, 0x3AC0, 0x3B80, 0xFB41, 0x3900, 0xF9C1, 0xF881, 0x3840,
	0x2800, 0xE8C1, 0xE981, 0x2940, 0xEB01, 0x2BC0, 0x2A80, 0xEA41,
	0xEE01, 0x2EC0, 0x2F80, 0xEF41, 0x2D00, 0xEDC1, 0xEC81, 0x2C40,
	0xE401, 0x24C0, 0x2580, 0xE541, 0x2700, 0xE7C1, 0xE681, 0x2640,
	0x2200, 0xE2C1, 0xE381, 0x2340, 0xE101, 0x21C0, 0x2080, 0xE041,
	0xA001, 0x60C0, 0x6180, 0xA141, 0x6300, 0xA3C1, 0xA281, 0x6240,
	0x6600, 0xA6C1, 0xA781, 0x6740, 0xA501, 0x65C0, 0x6480, 0xA441,
	0x6C00, 0xACC1, 0xAD81, 0x6D40, 0xAF01, 0x6FC0, 0x6E80, 0xAE41,
	0xAA01, 0x6AC0, 0x6B80, 0xAB41, 0x6900, 0xA9C1, 0xA881, 0x6840,
	0x7800, 0xB8C1, 0xB981, 0x7940, 0xBB01, 0x7BC0, 0x7A80, 0xBA41,
	0xBE01, 0x7EC0, 0x7F80, 0xBF41, 0x7D00, 0xBDC1, 0xBC81, 0x7C40,
	0xB401, 0x74C0, 0x7580, 0xB541, 0x7700, 0xB7C1, 0xB681, 0x7640,
	0x7200, 0xB2C1, 0xB381, 0x7340, 0xB101, 0x71C0, 0x7080, 0xB041,
	0x5000, 0x90C1, 0x9181, 0x5140, 0x9301, 0x53C0, 0x5280, 0x9241,
	0x9601, 0x56C0, 0x5780, 0x9741, 0x5500, 0x95C1, 0x9481, 0x5440,
	0x9C01, 0x5CC0, 0x5D80, 0x9D41, 0x5F00, 0x9FC1, 0x9E81, 0x5E40,
	0x5A00, 0x9AC1, 0x9B81, 0x5B40, 0x9901, 0x59C0, 0x5880, 0x9841,
	0x8801, 0x48C0, 0x4980, 0x8941, 0x4B00, 0x8BC1, 0x8A81, 0x4A40,
	0x4E00, 0x8EC1, 0x8F81, 0x4F40, 0x8D01, 0x4DC0, 0x4C80, 0x8C41,
	0x4400, 0x84C1, 0x8581, 0x4540, 0x8701, 0x47C0, 0x4680, 0x8641,
	0x8201, 0x42C0, 0x4380, 0x8341, 0x4100, 0x81C1, 0x8081, 0x4040
};

P1MeterBase::P1MeterBase()
{
	m_bDisableCRC = true;
	m_ratelimit = DEFAULT_RATE_LIMIT_P1;
	Init();
}

P1MeterBase::~P1MeterBase()
{
	delete[] m_pDecryptBuffer;
}

void P1MeterBase::Init()
{
	if (m_ratelimit == 0)
		m_ratelimit = DEFAULT_RATE_LIMIT_P1;
	m_p1version = 0;
	m_linecount = 0;
	m_exclmarkfound = 0;
	m_CRfound = 0;
	m_bufferpos = 0;
	m_lastgasusage = 0;
	m_lastSharedSendGas = 0;
	m_lastSendMBusDevice = 0;
	m_lastUpdateTime = mytime(nullptr);
	m_lastSendCalculated = mytime(nullptr);

	l_exclmarkfound = 0;
	l_bufferpos = 0;

	m_nbr_pwr_failures = -1;
	m_nbr_long_pwr_failures = -1;
	m_nbr_volt_sags_l1 = -1;
	m_nbr_volt_sags_l2 = -1;
	m_nbr_volt_sags_l3 = -1;
	m_nbr_volt_swells_l1 = -1;
	m_nbr_volt_swells_l2 = -1;
	m_nbr_volt_swells_l3 = -1;

	bool bExists = false;
	std::string tmpval;
	tmpval = GetTextSensorText(0, 7, bExists);
	if (bExists)
		m_last_nbr_pwr_failures = std::stoi(tmpval);

	tmpval = GetTextSensorText(0, 8, bExists);
	if (bExists)
		m_last_nbr_long_pwr_failures = std::stoi(tmpval);

	tmpval = GetTextSensorText(0, 9, bExists);
	if (bExists)
		m_last_nbr_volt_sags_l1 = std::stoi(tmpval);
	tmpval = GetTextSensorText(0, 10, bExists);
	if (bExists)
		m_last_nbr_volt_sags_l2 = std::stoi(tmpval);
	tmpval = GetTextSensorText(0, 11, bExists);
	if (bExists)
		m_last_nbr_volt_sags_l3 = std::stoi(tmpval);

	tmpval = GetTextSensorText(0, 12, bExists);
	if (bExists)
		m_last_nbr_volt_swells_l1 = std::stoi(tmpval);
	tmpval = GetTextSensorText(0, 13, bExists);
	if (bExists)
		m_last_nbr_volt_swells_l2 = std::stoi(tmpval);
	tmpval = GetTextSensorText(0, 14, bExists);
	if (bExists)
		m_last_nbr_volt_swells_l3 = std::stoi(tmpval);

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

	for (int ii = 0; ii < 3; ii++)
	{
		m_avr_rate_limit[ii].Init();
		m_avr_calculated[ii].Init();

		m_avr_calculated[ii].usage_cntr = GetKwhMeter(0, 1 + ii, bExists);
		m_avr_calculated[ii].delivery_cntr = GetKwhMeter(0, 4 + ii, bExists);
	}

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
		/*
		Gas meter not reporting any data
		When a gas meter is paired with the Smart Meter it gets assigned one of the four available channels.
		Sometimes technicians leave the old gas meter registered in the Smart Meter and Domoticz ends up with multiple gas meters
		and the wrong one is used
		To fix this you can specify a fixed channel
		Go to the Domoticz 'Set-up'->'More Options'->'User Variables' menu and create a new 'integer' value named 'P1GasMeterChannel'.
		Allowed values are 0 which is the default and enables auto select, or 1 to 4 for either of the four possible channels.
		*/
		std::string s_gasmbuschannel = result[0][0];
		if ((s_gasmbuschannel.length() == 1) && (s_gasmbuschannel[0] > 0x30) && (s_gasmbuschannel[0] < 0x35)) // value must be a single digit number between 1 and 4
		{
			m_gasmbuschannel = (char)s_gasmbuschannel[0];
			m_gasprefix[2] = m_gasmbuschannel;
			Log(LOG_STATUS, "Gas meter M-Bus channel %c enforced by 'P1GasMeterChannel' user variable", m_gasmbuschannel);

			_tMBusDevice mdevice;
			mdevice.channel = m_gasmbuschannel - 0x30;
			mdevice.name = "Gas";
			mdevice.prefix[2] = m_gasmbuschannel;
			m_mbus_devices[P1MBusType::deviceType_Gas] = mdevice;
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
	try {
		if ((strlen(l_buffer) < 1) || (l_buffer[0] == 0x0a))
			return true; //null value (startup)

		bool bFound = false;

		for (size_t i = 0; i < p1_matchlist.size(); ++i)
		{
			if (bFound)
				break;

			std::string sValue;

			const P1Match* t = &p1_matchlist[i];
			switch (t->matchtype)
			{
			case _eP1MatchType::ID:
				// start of data
				if (strncmp(t->key, l_buffer, strlen(t->key)) == 0)
				{
					m_linecount = 1;
					bFound = true;
				}
				continue; // we do not process anything else on this line
				break;
			case _eP1MatchType::EXCLMARK:
				// end of data
				if (strncmp(t->key, l_buffer, strlen(t->key)) == 0)
				{
					l_exclmarkfound = 1;
					bFound = true;
				}
				break;
			case _eP1MatchType::STD:
				if (strncmp(t->key, l_buffer, strlen(t->key)) == 0)
					bFound = true;
				break;
			case _eP1MatchType::DEVTYPE:
				if (m_p1_mbus_type == P1MBusType::deviceType_Unknown)
				{
					const char* pValue = t->key + 3;
					if (strncmp(pValue, l_buffer + 3, strlen(t->key) - 3) == 0)
						bFound = true;
					else
						i += 100; // skip matches with any other m-bus lines - we need to find the M0-Bus channel first
				}
				break;
			case _eP1MatchType::MBUS:
				if (strncmp((m_gasprefix + (t->key + 3)).c_str(), l_buffer, strlen(t->key)) == 0)
				{
					// verify that 'tariff' indicator is either 1 (Nld) or 3 (Bel)
					if ((l_buffer[9] & 0xFD) == 0x31)
						bFound = true;
				}
				else
				{
					for (const auto& itt : m_mbus_devices)
					{
						if (strncmp((itt.second.prefix + (t->key + 3)).c_str(), l_buffer, strlen(t->key)) == 0)
						{
							// verify that 'tariff' indicator is either 1 (Nld) or 3 (Bel)
							if ((l_buffer[9] & 0xFD) == 0x31)
								bFound = true;
						}
					}
				}
				if (m_p1version >= 4)
					i += 100; // skip matches with any DSMR v2 gas lines
				break;
			case _eP1MatchType::LINE17:
				if (strncmp((m_gasprefix + (t->key + 3)).c_str(), l_buffer, strlen(t->key)) == 0)
				{
					m_linecount = 17;
					bFound = true;
				}
				break;
			case _eP1MatchType::LINE18:
				if ((m_linecount == 18) && (strncmp(t->key, l_buffer, strlen(t->key)) == 0))
					bFound = true;
				break;
			} //switch

			if (!bFound)
				continue;

			if (l_exclmarkfound)
			{
				m_avr_rate_limit[0].Add_Usage(m_powerusel1);
				m_avr_rate_limit[1].Add_Usage(m_powerusel2);
				m_avr_rate_limit[2].Add_Usage(m_powerusel3);
				m_avr_rate_limit[0].Add_Delivery(m_powerdell1);
				m_avr_rate_limit[1].Add_Delivery(m_powerdell2);
				m_avr_rate_limit[2].Add_Delivery(m_powerdell3);

				m_avr_calculated[0].Add_Usage(m_powerusel1);
				m_avr_calculated[1].Add_Usage(m_powerusel2);
				m_avr_calculated[2].Add_Usage(m_powerusel3);
				m_avr_calculated[0].Add_Delivery(m_powerdell1);
				m_avr_calculated[1].Add_Delivery(m_powerdell2);
				m_avr_calculated[2].Add_Delivery(m_powerdell3);

				if (m_p1version == 0)
				{
					Log(LOG_STATUS, "Meter is pre DSMR 4.0 - using DSMR 2.2 compatibility");
					m_p1version = 2;
				}
				time_t atime = mytime(nullptr);
				if (difftime(atime, m_lastUpdateTime) >= m_ratelimit)
				{
					m_lastUpdateTime = atime;
					sDecodeRXMessage(this, (const unsigned char*)&m_power, "Power", 255, nullptr);
					if (m_voltagel1 != -1) {
						SendVoltageSensor(0, 1, 255, m_voltagel1, "Voltage L1");
					}
					if (m_voltagel2 != -1) {
						SendVoltageSensor(0, 2, 255, m_voltagel2, "Voltage L2");
					}
					if (m_voltagel3 != -1) {
						SendVoltageSensor(0, 3, 255, m_voltagel3, "Voltage L3");
					}

					float avr_usage[3];
					float avr_deliv[3];
					float calculated_usage = 0;
					float calculated_deliv = 0;
					bool bHaveDelivery = false;

					for (int iif = 0; iif < 3; iif++)
					{
						avr_usage[iif] = m_avr_rate_limit[iif].Get_Usage_Avr();
						avr_deliv[iif] = m_avr_rate_limit[iif].Get_Delivery_Avr();

						m_avr_rate_limit[iif].ResetTotals();

						if (avr_usage[iif] != -1) {
							calculated_usage += avr_usage[iif];
							SendWattMeter(0, 1 + iif, 255, round(avr_usage[iif]), std_format("Usage L%d", 1 + iif));
						}
						if (avr_deliv[iif] != -1) {
							bHaveDelivery = true;
							calculated_deliv += avr_deliv[iif];
							SendWattMeter(0, 4 + iif, 255, round(avr_deliv[iif]), std_format("Delivery L%d", 1 + iif));
						}
					}

					// create calculated usage/delivery Watt sensors
					SendWattMeter(0, 7, 255, round(calculated_usage), "Actual Usage (L1 + L2 + L3)");
					if (bHaveDelivery)
						SendWattMeter(0, 8, 255, round(calculated_deliv), "Actual Delivery (L1 + L2 + L3)");

					if (m_nbr_pwr_failures != -1) {
						SendTextSensorWhenDifferent(7, m_nbr_pwr_failures, m_last_nbr_pwr_failures, "# Power failures");
					}
					if (m_nbr_long_pwr_failures != -1) {
						SendTextSensorWhenDifferent(8, m_nbr_long_pwr_failures, m_last_nbr_long_pwr_failures, "# Long power failures");
					}
					if (m_nbr_volt_sags_l1 != -1) {
						SendTextSensorWhenDifferent(9, m_nbr_volt_sags_l1, m_last_nbr_volt_sags_l1, "# Voltage sags L1");
					}
					if (m_nbr_volt_sags_l2 != -1) {
						SendTextSensorWhenDifferent(10, m_nbr_volt_sags_l2, m_last_nbr_volt_sags_l2, "# Voltage sags L2");
					}
					if (m_nbr_volt_sags_l3 != -1) {
						SendTextSensorWhenDifferent(11, m_nbr_volt_sags_l3, m_last_nbr_volt_sags_l3, "# Voltage sags L3");
					}
					if (m_nbr_volt_swells_l1 != -1) {
						SendTextSensorWhenDifferent(12, m_nbr_volt_swells_l1, m_last_nbr_volt_swells_l1, "# Voltage swells L1");
					}
					if (m_nbr_volt_swells_l2 != -1) {
						SendTextSensorWhenDifferent(13, m_nbr_volt_swells_l2, m_last_nbr_volt_swells_l2, "# Voltage swells L2");
					}
					if (m_nbr_volt_swells_l3 != -1) {
						SendTextSensorWhenDifferent(14, m_nbr_volt_swells_l3, m_last_nbr_volt_swells_l3, "# Voltage swells L3");
					}

					if (
						(m_voltagel1 != -1)
						&& (m_voltagel2 != -1)
						&& (m_voltagel3 != -1)
						)
					{
						// The ampere is rounded to whole numbers and therefor not accurate enough
						// Therefor we calculate this ourselfs I=P/U, I1=(m_power.m_powerusel1/m_voltagel1)
						float I1 = avr_usage[0] / m_voltagel1;
						float I2 = avr_usage[1] / m_voltagel2;
						float I3 = avr_usage[2] / m_voltagel3;
						SendCurrentSensor(0, 255, I1, I2, I3, "Current L1/L2/L3");

						//Do the same for delivered
						if (m_powerdell1 || m_powerdell2 || m_powerdell3)
						{
							I1 = avr_deliv[0] / m_voltagel1;
							I2 = avr_deliv[1] / m_voltagel2;
							I3 = avr_deliv[2] / m_voltagel3;
							SendCurrentSensor(1, 255, I1, I2, I3, "Delivery Current L1/L2/L3");
						}
					}

					if ((m_gas.gasusage > 0) && ((m_gas.gasusage != m_lastgasusage) || (difftime(atime, m_lastSharedSendGas) >= 300)))
					{
						//only update gas when there is a new value, or 5 minutes are passed
						if (m_gasclockskew >= 300)
						{
							// just accept it - we cannot sync to our clock
							m_lastSharedSendGas = atime;
							m_lastgasusage = m_gas.gasusage;
							sDecodeRXMessage(this, (const unsigned char*)&m_gas, "Gas", 255, nullptr);
						}
						else if (atime >= m_gasoktime)
						{
							struct tm ltime;
							localtime_r(&atime, &ltime);
							char myts[80];
							sprintf(myts, "%02d%02d%02d%02d%02d%02dW", ltime.tm_year % 100, ltime.tm_mon + 1, ltime.tm_mday, ltime.tm_hour, ltime.tm_min, ltime.tm_sec);
							if (ltime.tm_isdst)
								myts[12] = 'S';
							if ((m_gastimestamp.length() > 13) || (strncmp(myts, m_gastimestamp.c_str(), m_gastimestamp.length()) >= 0))
							{
								m_lastSharedSendGas = atime;
								m_lastgasusage = m_gas.gasusage;
								m_gasoktime += 300;
								sDecodeRXMessage(this, (const unsigned char*)&m_gas, "Gas", 255, nullptr);
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
									Log(LOG_ERROR, "Unable to synchronize to the gas meter clock because it is more than 5 minutes ahead of my time");
								}
								else {
									m_gasoktime = gtime;
									Log(LOG_STATUS, "Gas meter clock is %i seconds ahead - wait for my clock to catch up", (int)m_gasclockskew);
								}
							}
						}
					} //gas
					for (auto& itt : m_mbus_devices)
					{
						if ((itt.second.usage > 0) && ((itt.second.usage != itt.second.last_usage) || (difftime(atime, m_lastSendMBusDevice) >= 300)))
						{
							SendMeterSensor((uint8_t)itt.first, 1, 255, itt.second.usage, itt.second.name);
							itt.second.last_usage = itt.second.usage;
							m_lastSendMBusDevice = atime;
						} //water
					}
				} //if (difftime(atime, m_lastUpdateTime) >= m_ratelimit)

#define kWh_Update_Interval 10
				if (difftime(atime, m_lastSendCalculated) >= kWh_Update_Interval)
				{
					m_lastSendCalculated = atime;

					for (int iif = 0; iif < 3; iif++)
					{
						float avr_usage = m_avr_calculated[iif].Get_Usage_Avr();
						float avr_deliv = m_avr_calculated[iif].Get_Delivery_Avr();
						m_avr_calculated[iif].ResetTotals();

						if (avr_usage != -1)
						{
							m_avr_calculated[iif].usage_cntr += (avr_usage * kWh_Update_Interval / 3600.0);
							if (m_avr_calculated[iif].usage_cntr > 0)
								SendKwhMeter(0, 1 + iif, 255, round(avr_usage), m_avr_calculated[iif].usage_cntr * 0.001, std_format("kWh Usage L%d (Calculated)", 1 + iif));
						}
						if (avr_deliv != -1)
						{
							m_avr_calculated[iif].delivery_cntr += (avr_deliv * kWh_Update_Interval / 3600.0);
							if (m_avr_calculated[iif].delivery_cntr > 0)
								SendKwhMeter(0, 4 + iif, 255, round(avr_deliv), m_avr_calculated[iif].delivery_cntr * 0.001, std_format("kWh Delivery L%d (Calculated)", 1 + iif));
						}
					}
				} //if (difftime(atime, m_lastSendCalculated) >= kWh_Update_Interval)

				m_linecount = 0;
				l_exclmarkfound = 0;
			}
			else
			{
				std::string vString = l_buffer + t->start;

				size_t ePos = vString.find_first_of("*)");
				if (ePos == std::string::npos)
				{
					// invalid message: value not delimited
					Log(LOG_NORM, "Dismiss incoming - value is not delimited in line \"%s\"", l_buffer);
					return false;
				}

				if (ePos > 0)
				{
					sValue = vString.substr(0, ePos);
#ifdef _DEBUG
					Log(LOG_NORM, "Key: %s, Value: %s", t->topic, sValue.c_str());
#endif
				}

				unsigned long temp_usage = 0;
				float temp_volt = 0;
				float temp_ampere = 0;
				float temp_power = 0;
				float temp_float = 0;
				P1MBusType mbus_type = P1MBusType::deviceType_Unknown;
				uint8_t mbus_channel = 0;

				switch (t->type)
				{
				case P1TYPE_VERSION:
					if (m_p1version == 0)
					{
						m_p1version = sValue.at(0) - 0x30;
						char szVersion[12];
						if (t->width == 5)
						{
							// Belgian meter
							sprintf(szVersion, "ESMR %c.%c.%c", sValue.at(0), sValue.at(1), sValue.at(2));
						}
						else // if (t->width == 2)
						{
							// Dutch meter
							sprintf(szVersion, "ESMR %c.%c", sValue.at(0), sValue.at(1));
							if (m_p1version < 5)
								szVersion[0] = 'D';
						}
						Log(LOG_STATUS, "Meter reports as %s", szVersion);
					}
					break;
				case P1TYPE_MBUSDEVICETYPE:
					mbus_type = (P1MBusType)std::stoul(sValue);
					mbus_channel = l_buffer[2];
					//Open Metering System Specification 4.3.3 table 2 (Device Types of OMS-Meter)
					/*
					* Electricity meter 02h
					* Gas meter 03h
					* Heat meter 04h
					* Warm water meter (30°C ... 90°C) 06h
					* Water meter 07h
					* Heat Cost Allocator 08h
					* Cooling meter (Volume measured at return temperature: outlet) 0Ah
					* Cooling meter (Volume measured at flow temperature: inlet) 0Bh
					* Heat meter (Volume measured at flow temperature: inlet) 0Ch
					* Combined Heat / Cooling meter 0Dh
					* Hot water meter (= 90°C) 15h
					* Cold water meter a 16h
					* Breaker (electricity) 20h
					* Valve (gas or water) 21h
					* Waste water meter 28h
					*/
					if (mbus_type == P1MBusType::deviceType_Gas)
					{
						if (m_gasmbuschannel == 0)
						{
							m_gasmbuschannel = (char)l_buffer[2];
							if (m_gasprefix[2] == 'n')
								Log(LOG_STATUS, "Found gas meter on M-Bus channel %c", m_gasmbuschannel);
							m_gasprefix[2] = m_gasmbuschannel;
						}
					}
					else
					{
						for (const auto& itt : p1_supported_mbus_list)
						{
							if (mbus_type == itt.type)
							{
								if (m_mbus_devices.find(mbus_type) == m_mbus_devices.end())
								{
									//new
									_tMBusDevice mdevice;
									mdevice.channel = l_buffer[2] - 0x30;
									mdevice.name = itt.name;
									mdevice.prefix[2] = (char)l_buffer[2];
									m_mbus_devices[mbus_type] = mdevice;
									Log(LOG_STATUS, "Found '%s' meter on M-Bus channel %c", itt.name, mdevice.channel);
								}
							}
						}
					}
					m_p1_mbus_type = mbus_type;
					m_p1_mbus_channel = l_buffer[2];
					break;
				case P1TYPE_POWERUSAGE:
					temp_usage = (unsigned long)(std::stof(sValue) * 1000.0F);
					if ((l_buffer[8] & 0xFE) == 0x30)
					{
						// map tariff IDs 0 (Lux) and 1 (Bel, Nld) both to powerusage1
						if (!m_power.powerusage1 || m_p1version >= 4)
							m_power.powerusage1 = temp_usage;
						else if (temp_usage - m_power.powerusage1 < P1MAXPHASEPOWER)
							m_power.powerusage1 = temp_usage;
					}
					else if (l_buffer[8] == 0x32)
					{
						if (!m_power.powerusage2 || m_p1version >= 4)
							m_power.powerusage2 = temp_usage;
						else if (temp_usage - m_power.powerusage2 < P1MAXPHASEPOWER)
							m_power.powerusage2 = temp_usage;
					}
					break;
				case P1TYPE_POWERDELIV:
					temp_usage = (unsigned long)(std::stof(sValue) * 1000.0F);
					if ((l_buffer[8] & 0xFE) == 0x30)
					{
						// map tariff IDs 0 (Lux) and 1 (Bel, Nld) both to powerdeliv1
						if (!m_power.powerdeliv1 || m_p1version >= 4)
							m_power.powerdeliv1 = temp_usage;
						else if (temp_usage - m_power.powerdeliv1 < P1MAXPHASEPOWER)
							m_power.powerdeliv1 = temp_usage;
					}
					else if (l_buffer[8] == 0x32)
					{
						if (!m_power.powerdeliv2 || m_p1version >= 4)
							m_power.powerdeliv2 = temp_usage;
						else if (temp_usage - m_power.powerdeliv2 < P1MAXPHASEPOWER)
							m_power.powerdeliv2 = temp_usage;
					}
					break;
				case P1TYPE_USAGECURRENT:
					temp_usage = (unsigned long)(std::stof(sValue) * 1000.0F); // Watt
					if (temp_usage < P1MAXTOTALPOWER)
						m_power.usagecurrent = temp_usage;
					break;
				case P1TYPE_DELIVCURRENT:
					temp_usage = (unsigned long)(std::stof(sValue) * 1000.0F); // Watt;
					if (temp_usage < P1MAXTOTALPOWER)
						m_power.delivcurrent = temp_usage;
					break;
				case P1TYPE_NUMPWRFAIL:
					m_nbr_pwr_failures = std::stoi(sValue);
					break;
				case P1TYPE_NUMLONGPWRFAIL:
					m_nbr_long_pwr_failures = std::stoi(sValue);
					break;
				case P1TYPE_NUMVOLTSAGSL1:
					m_nbr_volt_sags_l1 = std::stoi(sValue);
					break;
				case P1TYPE_NUMVOLTSAGSL2:
					m_nbr_volt_sags_l2 = std::stoi(sValue);
					break;
				case P1TYPE_NUMVOLTSAGSL3:
					m_nbr_volt_sags_l3 = std::stoi(sValue);
					break;
				case P1TYPE_NUMVOLTSWELLSL1:
					m_nbr_volt_swells_l1 = std::stoi(sValue);
					break;
				case P1TYPE_NUMVOLTSWELLSL2:
					m_nbr_volt_swells_l2 = std::stoi(sValue);
					break;
				case P1TYPE_NUMVOLTSWELLSL3:
					m_nbr_volt_swells_l3 = std::stoi(sValue);
					break;
				case P1TYPE_VOLTAGEL1:
					temp_volt = std::stof(sValue);
					if (temp_volt < 300)
						m_voltagel1 = temp_volt; //Voltage L1;
					break;
				case P1TYPE_VOLTAGEL2:
					temp_volt = std::stof(sValue);
					if (temp_volt < 300)
						m_voltagel2 = temp_volt; //Voltage L2;
					break;
				case P1TYPE_VOLTAGEL3:
					temp_volt = std::stof(sValue);
					if (temp_volt < 300)
						m_voltagel3 = temp_volt; //Voltage L3;
					break;
				case P1TYPE_AMPERAGEL1:
					temp_ampere = std::stof(sValue);
					if (temp_ampere < 100)
					{
						m_amperagel1 = temp_ampere; //Amperage L1;
						m_bReceivedAmperage = true;
					}
					break;
				case P1TYPE_AMPERAGEL2:
					temp_ampere = std::stof(sValue);
					if (temp_ampere < 100)
					{
						m_amperagel2 = temp_ampere; //Amperage L2;
						m_bReceivedAmperage = true;
					}
					break;
				case P1TYPE_AMPERAGEL3:
					temp_ampere = std::stof(sValue);
					if (temp_ampere < 100)
					{
						m_amperagel3 = temp_ampere; //Amperage L3;
						m_bReceivedAmperage = true;
					}
					break;
				case P1TYPE_POWERUSEL1:
					temp_power = std::stof(sValue) * 1000.0F;
					if (temp_power < P1MAXPHASEPOWER)
						m_powerusel1 = temp_power; //Power Used L1;
					break;
				case P1TYPE_POWERUSEL2:
					temp_power = std::stof(sValue) * 1000.0F;
					if (temp_power < P1MAXPHASEPOWER)
						m_powerusel2 = temp_power; //Power Used L2;
					break;
				case P1TYPE_POWERUSEL3:
					temp_power = std::stof(sValue) * 1000.0F;
					if (temp_power < P1MAXPHASEPOWER)
						m_powerusel3 = temp_power; //Power Used L3;
					break;
				case P1TYPE_POWERDELL1:
					temp_power = std::stof(sValue) * 1000.0F;
					if (temp_power < P1MAXPHASEPOWER)
						m_powerdell1 = temp_power; //Power Used L1;
					break;
				case P1TYPE_POWERDELL2:
					temp_power = std::stof(sValue) * 1000.0F;
					if (temp_power < P1MAXPHASEPOWER)
						m_powerdell2 = temp_power; //Power Used L2;
					break;
				case P1TYPE_POWERDELL3:
					temp_power = std::stof(sValue) * 1000.0F;
					if (temp_power < P1MAXPHASEPOWER)
						m_powerdell3 = temp_power; //Power Used L3;
					break;
				case P1TYPE_GASTIMESTAMP:
					m_gastimestamp = sValue;
					break;
				case P1TYPE_GASUSAGE:
				case P1TYPE_MBUSUSAGEDSMR4:
					temp_float = std::stof(sValue);
					temp_usage = (unsigned long)(temp_float * 1000.0F);

					if (
						(t->type == P1TYPE_GASUSAGE)
						|| (m_p1_mbus_type == P1MBusType::deviceType_Gas)
						)
					{
						if (!m_gas.gasusage || m_p1version >= 4)
							m_gas.gasusage = temp_usage;
						else if (temp_usage - m_gas.gasusage < 20000)
							m_gas.gasusage = temp_usage;
					}
					else
					{
						for (auto& itt : m_mbus_devices)
						{
							if (itt.first == m_p1_mbus_type)
							{
								itt.second.usage = temp_float;
							}
						}
					}
					break;
				}
				if (ePos > 0 && (sValue.size() != ePos))
				{
					// invalid message: value is not a number
					Log(LOG_NORM, "Dismiss incoming - value in line \"%s\" is not a number", l_buffer);
					return false;
				}

				if (t->type == P1TYPE_MBUSUSAGEDSMR4)
				{
					// need to get timestamp from this line as well
					vString = l_buffer + 11;
					if (m_p1_mbus_type == P1MBusType::deviceType_Gas)
					{
						m_gastimestamp = vString.substr(0, 13);
#ifdef _DEBUG
						Log(LOG_NORM, "Key: gastimestamp, Value: %s", m_gastimestamp.c_str());
#endif
					}
					else
					{
						for (auto& itt : m_mbus_devices)
						{
							if (itt.first == m_p1_mbus_type)
							{
								itt.second.timestamp = vString.substr(0, 13);
#ifdef _DEBUG
								Log(LOG_NORM, "Key: %s timestamp value: %s", itt.second.name.c_str(), itt.second.timestamp.c_str());
#endif
							}
						}
					}
					m_p1_mbus_type = P1MBusType::deviceType_Unknown;
				}
			}
		}
		return true;
	}
	catch (const std::exception& e)
	{
		Log(LOG_NORM, "P1Meter: Line parsing error (%s)", e.what());
	}
	return false;
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
			Log(LOG_STATUS, "Meter is pre DSMR 4.0 and does not send a CRC checksum - using DSMR 2.2 compatibility");
			m_p1version = 2;
		}
		// always return true with pre DSMRv4 format message
		return true;
	}

	if (l_buffer[5] != 0)
	{
		// trailing characters after CRC
		Log(LOG_NORM, "Dismiss incoming - CRC value in message has trailing characters");
		return false;
	}

	if (!m_CRfound)
	{
		Log(LOG_NORM, "You appear to have middleware that changes the message content - skipping CRC validation");
		return true;
	}

	// retrieve CRC from the current line
	char crc_str[5];
	strncpy(crc_str, l_buffer + 1, 4);
	crc_str[4] = 0;
	uint16_t m_crc16 = (uint16_t)strtoul(crc_str, nullptr, 16);

	// calculate CRC
	uint16_t crc = 0;
	for (int ii = 0; ii < m_bufferpos; ii++)
	{
		crc = (crc >> 8) ^ p1_crc_16[(crc ^ m_buffer[ii]) & 0xFF];
	}
	if (crc != m_crc16)
	{
		Log(LOG_NORM, "Dismiss incoming - CRC failed");
	}
	return (crc == m_crc16);
}

void P1MeterBase::SendTextSensorWhenDifferent(const int ID, const int value, int& cmp_value, const std::string& Name)
{
	if (value == cmp_value)
		return; //no difference
	cmp_value = value;
	SendTextSensor(0, ID, 255, std::to_string(value), Name);
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

					size_t neededDecryptBufferSize = std::min(2048, static_cast<int>(cipherText.size() + 16));
					if (neededDecryptBufferSize > m_DecryptBufferSize)
					{
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
					EVP_DecryptInit_ex(ctx, EVP_aes_128_gcm(), nullptr, nullptr, nullptr);
					EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_AEAD_SET_IVLEN, static_cast<int>(iv.size()), nullptr);

					EVP_DecryptInit_ex(ctx, nullptr, nullptr, (const unsigned char*)m_szHexKey.data(),
						(const unsigned char*)iv.c_str());

					int outlen = 0;
					// std::vector<char> m_szDecodeAdd = HexToBytes(_szDecodeAdd);
					// EVP_DecryptUpdate(ctx, nullptr, &outlen, (const uint8_t*)m_szDecodeAdd.data(),
					// m_szDecodeAdd.size());
					EVP_DecryptUpdate(ctx, (uint8_t*)m_pDecryptBuffer, &outlen, (uint8_t*)cipherText.c_str(), static_cast<int>(cipherText.size()));
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
					Log(LOG_ERROR, "P1Meter: Error decrypting payload (%s)", e.what());
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
			Log(LOG_STATUS, "WARNING: got new message but buffer still contains unprocessed data from previous message.");
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
		m_p1_mbus_type = P1MBusType::deviceType_Unknown;
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
			Log(LOG_NORM, "Dismiss incoming - message oversized");
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
