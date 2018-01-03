
#include "stdafx.h"
#include "RFLinkBase.h"
#include "../main/Logger.h"
#include "../main/Helper.h"
#include "../main/RFXtrx.h"
#include "hardwaretypes.h"
#include "../main/localtime_r.h"
#include "../main/SQLHelper.h"

#include "../main/mainworker.h"
#include "../main/WebServer.h"
#include "../webserver/cWebem.h"
#include "../json/json.h"

#ifdef _DEBUG
	#define ENABLE_LOGGING
#endif

struct _tRFLinkStringIntHelper
{
	std::string szType;
	int gType;
};

const _tRFLinkStringIntHelper rfswitches[] =
{
	{ "X10", sSwitchTypeX10 },               // p9
	{ "Kaku", sSwitchTypeARC },              // p3
	{ "AB400D", sSwitchTypeAB400D },         // p3
	{ "Waveman", sSwitchTypeWaveman },       // p-
	{ "EMW200", sSwitchTypeEMW200 },         // p-
	{ "Impuls", sSwitchTypeIMPULS },         // p3
	{ "RisingSun", sSwitchTypeRisingSun },   // p-
	{ "Philips", sSwitchTypePhilips },       // p-
	{ "Energenie", sSwitchTypeEnergenie },   // p-
	{ "Energenie5", sSwitchTypeEnergenie5 }, // p-
	{ "GDR2", sSwitchTypeGDR2 },             // p-
	{ "NewKaku", sSwitchTypeAC },            // p4
	{ "HomeEasy", sSwitchTypeHEU },          // p15
	{ "Anslut", sSwitchTypeANSLUT },         // p17
	{ "Kambrook", sSwitchTypeKambrook },     // p8
	{ "Ikea Koppla", sSwitchTypeKoppla },    // p14
	{ "PT2262", sSwitchTypePT2262 },         // p3
	{ "Lightwave", sSwitchTypeLightwaveRF }, // p18
	{ "EMW100", sSwitchTypeEMW100 },         // p13
	{ "BSB", sSwitchTypeBBSB },              // p-
	{ "MDRemote", sSwitchTypeMDREMOTE },     // p14
	{ "Conrad", sSwitchTypeRSL },            // p7
	{ "Livolo", sSwitchTypeLivolo },         // p-
	{ "TRC02RGB", sSwitchTypeTRC02 },        // p10
	{ "Aoke", sSwitchTypeAoke },             // p-
	{ "TRC022RGB", sSwitchTypeTRC02_2 },     // p-
	{ "Eurodomest", sSwitchTypeEurodomest }, // p5
	{ "Livolo App", sSwitchTypeLivoloAppliance }, // p-
	{ "Blyss", sSwitchTypeBlyss },           // p6
	{ "Byron", sSwitchTypeByronSX },         // p72
	{ "Byron MP", sSwitchTypeByronMP001 },   // p74
	{ "SelectPlus", sSwitchTypeSelectPlus }, // p70
	{ "Doorbell", sSwitchTypeSelectPlus3 },  // p73
	{ "FA20RF", sSwitchTypeFA20 },           // p80
	{ "Chuango", sSwitchTypeChuango },       // p62
	{ "Plieger", sSwitchTypePlieger },       // p71
	{ "SilverCrest", sSwitchTypeSilvercrest }, // p75
	{ "Mertik", sSwitchTypeMertik },         // p82
	{ "HomeConfort", sSwitchTypeHomeConfort }, // p11
	{ "Powerfix", sSwitchTypePowerfix },     // p13
	{ "TriState", sSwitchTypeTriState },     // p16
	{ "Deltronic", sSwitchTypeDeltronic },   // p73
	{ "FA500", sSwitchTypeFA500 },           // p12
	{ "HT12E", sSwitchTypeHT12E },           // p60
	{ "EV1527", sSwitchTypeEV1527 },         // p61
	{ "Elmes", sSwitchTypeElmes },           // p65
	{ "Aster", sSwitchTypeAster },           // p17
	{ "Sartano", sSwitchTypeSartano },       // p3
	{ "Europe", sSwitchTypeEurope },         // p18
	{ "Avidsen", sSwitchTypeAvidsen },       // p..
	{ "BofuMotor", sSwitchTypeBofu },        // p..
	{ "BrelMotor", sSwitchTypeBrel },        // p..
	{ "RTS", sSwitchTypeRTS },               // p..
	{ "ElroDB", sSwitchTypeElroDB },         // p..
	{ "Dooya", sSwitchTypeDooya },           // p..
	{ "Unitec", sSwitchTypeUnitec },         // p..
	{ "Maclean", sSwitchTypeMaclean },		 // p..
	{ "R546", sSwitchTypeR546 },	         // p..
	{ "Diya", sSwitchTypeDiya },	         // p..
	{ "X10Secure", sSwitchTypeX10secu },	 // p..
	{ "Atlantic", sSwitchTypeAtlantic },	 // p..
	{ "SilvercrestDB", sSwitchTypeSilvercrestDB }, // p..
	{ "MedionDB", sSwitchTypeMedionDB },	 // p..
	{ "VMC", sSwitchTypeVMC },				 // p..
	{ "Keeloq", sSwitchTypeKeeloq },		 // p..
	{ "CustomSwitch", sSwitchCustomSwitch }, // NA
	{ "GeneralSwitch", sSwitchGeneralSwitch }, // NA
	{ "Koch", sSwitchTypeKoch },			 // NA
	{ "Kingpin", sSwitchTypeKingpin },		 // NA
	{ "Funkbus", sSwitchTypeFunkbus },		 // NA
	{ "Nice", sSwitchTypeNice },			 // NA
	{ "Forest", sSwitchTypeForest },		 // NA
	{ "MC145026", sSwitchMC145026 },		 // NA
	{ "Lobeco", sSwitchLobeco },			 // NA
	{ "Friedland", sSwitchFriedland },		 // NA
	{ "BFT", sSwitchBFT },					 // NA
	{ "Novatys", sSwitchNovatys},			 // NA
	{ "Halemeier", sSwitchHalemeier},
	{ "Gaposa", sSwitchGaposa },
	{ "MiLightv1", sSwitchMiLightv1 },
	{ "MiLightv2", sSwitchMiLightv2 },
	{ "HT6P20", sSwitchHT6P20 },
	{ "Doitrand", sSwitchTypeDoitrand },
	{ "Warema", sSwitchTypeWarema },
	{ "Ansluta", sSwitchTypeAnsluta },
	{ "Livcol", sSwitchTypeLivcol },
	{ "Bosch", sSwitchTypeBosch },
	{ "Ningbo", sSwitchTypeNingbo },
	{ "Ditec", sSwitchTypeDitec },
	{ "Steffen", sSwitchTypeSteffen },
	{ "AlectoSA", sSwitchTypeAlectoSA },
	{ "GPIOset", sSwitchTypeGPIOset },
	{ "KonigSec", sSwitchTypeKonigSec },
	{ "RM174RF", sSwitchTypeRM174RF },
	{ "Liwin", sSwitchTypeLiwin },
	{ "YW_Secu", sSwitchTypeYW_Secu },
	{ "Mertik_GV60", sSwitchTypeMertik_GV60 },
	{ "Ningbo64", sSwitchTypeNingbo64 },
	{ "X2D", sSwitchTypeX2D },
	{ "HRCMotor", sSwitchTypeHRCMotor },
	{ "Velleman", sSwitchTypeVelleman },
	{ "RFCustom", sSwitchTypeRFCustom },
	{ "YW_Sensor", sSwitchTypeYW_Sensor },
	{ "LEGRANDCAD", sSwitchTypeLegrandcad },
	{ "SysfsGpio", sSwitchTypeSysfsGpio },
	{ "Hager", sSwitchTypeHager },
	{ "Faber", sSwitchTypeFaber },
	{ "Drayton", sSwitchTypeDrayton },
	{ "V2Phoenix", sSwitchTypeV2Phoenix },
	{ "", -1 }
};

const _tRFLinkStringIntHelper rfswitchcommands[] =
{
	{ "ON", gswitch_sOn },
	{ "OFF", gswitch_sOff },
	{ "ALLON", gswitch_sGroupOn },
	{ "ALLOFF", gswitch_sGroupOff },
	{ "DIM", gswitch_sDim },
	{ "BRIGHT", gswitch_sBright },
	{ "UP", blinds_sOpen },
	{ "DOWN", blinds_sClose },
	{ "STOP", gswitch_sStop },
	{ "COLOR", gswitch_sColor },
	{ "DISCO+", gswitch_sDiscop },
	{ "DISCO-", gswitch_sDiscom },
	{ "", -1 }
};

const _tRFLinkStringIntHelper rfblindcommands[] =
{
	{ "UP", blinds_sOpen },
	{ "DOWN", blinds_sClose },
	{ "STOP", blinds_sStop },
	{ "CONFIRM", blinds_sConfirm },
	{ "LIMIT", blinds_sLimit },
	{ "", -1 }
};



int GetGeneralRFLinkFromString(const _tRFLinkStringIntHelper *pTable, const std::string &szType)
{
	int ii = 0;
	while (pTable[ii].gType!=-1)
	{
		if (pTable[ii].szType == szType)
			return pTable[ii].gType;
		ii++;
	}
	return -1;
}

std::string GetGeneralRFLinkFromInt(const _tRFLinkStringIntHelper *pTable, const int gType)
{
	int ii = 0;
	while (pTable[ii].gType!=-1)
	{
		if (pTable[ii].gType == gType)
			return pTable[ii].szType;
		ii++;
	}
	return "";
}

CRFLinkBase::CRFLinkBase()
{
	m_rfbufferpos=0;
	memset(&m_rfbuffer,0,sizeof(m_rfbuffer));
	/*
	ParseLine("20;08;NewKaku;ID=31c42a;SWITCH=2;CMD=OFF;");
	ParseLine("20;3A;NewKaku;ID=c142;SWITCH=1;CMD=ALLOFF;");
	ParseLine("20;14;Oregon BTHR;ID=5a00;TEMP=00d1;HUM=29;BARO=0407;BAT=LOW;");
	ParseLine("20;27;Cresta;ID=8001;RAIN=69e7;");
	ParseLine("20;2D;UPM/Esic;ID=0001;TEMP=00cf;HUM=16;BAT=OK;");
	ParseLine("20;31;Mebus;ID=c201;TEMP=00cf;");
	ParseLine("20;32;Auriol;ID=008f;TEMP=00d3;BAT=OK;");
	ParseLine("20;AF;SelectPlus;ID=1bb4;CHIME=01;");
	ParseLine("20;04;Eurodomest;ID=03696b;SWITCH=07;CMD=ALLOFF;");
	*/
}

CRFLinkBase::~CRFLinkBase()
{
}


/*
void CRFLinkBase::Add2SendQueue(const char* pData, const size_t length)
{
	std::string sBytes;
	sBytes.insert(0,pData,length);
	boost::lock_guard<boost::mutex> l(m_sendMutex);
	m_sendqueue.push_back(sBytes);
}
*/

void CRFLinkBase::ParseData(const char *data, size_t len)
{
	size_t ii=0;
	while (ii<len)
	{
		const unsigned char c = data[ii];
		if (c == 0x0d)
		{
			ii++;
			continue;
		}

		if (c == 0x0a || m_rfbufferpos == sizeof(m_rfbuffer) - 1)
		{
			// discard newline, close string, parse line and clear it.
			if (m_rfbufferpos > 0) m_rfbuffer[m_rfbufferpos] = 0;
			std::string sLine((char*)&m_rfbuffer);
			ParseLine(sLine);
			m_rfbufferpos = 0;
		}
		else
		{
			m_rfbuffer[m_rfbufferpos] = c;
			m_rfbufferpos++;
		}
		ii++;
	}

}

#define round(a) ( int ) ( a + .5 )

void GetSwitchType(const char* ID, const unsigned char unit, const unsigned char devType, const unsigned char subType, int &switchType)
{
	switchType = 0;
	std::vector<std::vector<std::string> > result;
	result = m_sql.safe_query("SELECT SwitchType FROM DeviceStatus WHERE (DeviceID='%q' AND Unit=%d AND Type=%d AND SubType=%d)", ID, unit, devType, subType);
	if (result.size() != 0)
	{
		switchType = atoi(result[0][0].c_str());
	}
}

bool CRFLinkBase::WriteToHardware(const char *pdata, const unsigned char length)
{
	const _tGeneralSwitch *pSwitch = reinterpret_cast<const _tGeneralSwitch*>(pdata);

	if ((pSwitch->type != pTypeGeneralSwitch) && (pSwitch->type != pTypeLimitlessLights))
		return false; //only allowed to control regular switches and MiLight

	//_log.Log(LOG_ERROR, "RFLink: switch type: %d", pSwitch->subtype);
	std::string switchtype = GetGeneralRFLinkFromInt(rfswitches, pSwitch->subtype);
	if (switchtype.empty())
	{
		_log.Log(LOG_ERROR, "RFLink: trying to send unknown switch type: %d", pSwitch->subtype);
		return false;
	}
	//_log.Log(LOG_ERROR, "RFLink: id: %d", pSwitch->id);
	//_log.Log(LOG_ERROR, "RFLink: subtype: %d", pSwitch->subtype);

	// get SwitchType from SQL table
	int m_SwitchType = 0;
	char szDeviceID[20];
	sprintf(szDeviceID, "%08X", pSwitch->id);
	std::string ID = szDeviceID;
	GetSwitchType(ID.c_str(), pSwitch->unitcode, pSwitch->type, pSwitch->subtype, m_SwitchType);
	//_log.Log(LOG_ERROR, "RFLink: switch type: %d", m_SwitchType);
	//_log.Log(LOG_ERROR, "RFLink: switch cmd: %d", pSwitch->cmnd);

	if (pSwitch->type == pTypeGeneralSwitch) {
		std::string switchcmnd = GetGeneralRFLinkFromInt(rfswitchcommands, pSwitch->cmnd);
		if (pSwitch->cmnd != gswitch_sStop) {
			if ((m_SwitchType == STYPE_VenetianBlindsEU) || (m_SwitchType == STYPE_Blinds) || (m_SwitchType == STYPE_BlindsInverted)) {
				switchcmnd = GetGeneralRFLinkFromInt(rfblindcommands, pSwitch->cmnd);
			}
			else {
				if (m_SwitchType == STYPE_VenetianBlindsUS) {
				//if ((m_SwitchType == STYPE_VenetianBlindsUS) || (m_SwitchType == STYPE_BlindsInverted)) {
					switchcmnd = GetGeneralRFLinkFromInt(rfblindcommands, pSwitch->cmnd);
					if (pSwitch->cmnd == blinds_sOpen) switchcmnd = GetGeneralRFLinkFromInt(rfblindcommands, blinds_sClose);
					else if (pSwitch->cmnd == blinds_sClose) switchcmnd = GetGeneralRFLinkFromInt(rfblindcommands, blinds_sOpen);
				}
			}
		}
		//_log.Log(LOG_ERROR, "RFLink: switch command: %d", pSwitch->cmnd);

        // check setlevel command
        if (pSwitch->cmnd == gswitch_sSetLevel) {
           // Get device level to set
			float fvalue = (15.0f / 100.0f)*float(pSwitch->level);
			if (fvalue > 15.0f)
				fvalue = 15.0f; //99 is fully on
			int svalue = round(fvalue);
			//_log.Log(LOG_ERROR, "RFLink: level: %d", svalue);
	        char buffer[50] = {0};
			sprintf(buffer, "%d", svalue);
			switchcmnd = buffer;
	    }

		if (switchcmnd.empty()) {
			_log.Log(LOG_ERROR, "RFLink: trying to send unknown switch command: %d", pSwitch->cmnd);
			return false;
		}

		//Build send string
		std::stringstream sstr;
		//10;NewKaku;00c142;1;ON;     => protocol;address;button number;action (ON/OFF/ALLON/ALLOFF/15 -11-15 for dim level)

		sstr << "10;" << switchtype << ";" << std::hex << std::nouppercase << std::setw(6) << std::setfill('0') << pSwitch->id << ";" << std::hex << std::nouppercase << pSwitch->unitcode << ";" << switchcmnd;
//#ifdef _DEBUG
		_log.Log(LOG_STATUS, "RFLink Sending: %s", sstr.str().c_str());
//#endif
		sstr << "\n";
		m_bTXokay = false; // clear OK flag
		WriteInt(sstr.str());
		time_t atime = mytime(NULL);
		time_t btime = mytime(NULL);

		// Wait for an OK response from RFLink to make sure the command was executed
		while (m_bTXokay == false) {
			if (difftime(btime,atime) > 4) {
				_log.Log(LOG_ERROR, "RFLink: TX time out...");
				return false;
			}
			btime = mytime(NULL);
		}
		return true;
	}
	else {		// RFLink Milight extension
		_tLimitlessLights *pLed = (_tLimitlessLights*)pdata;
      /*
		_log.Log(LOG_ERROR, "RFLink: ledtype: %d", pLed->type);			// type limitlessled
		_log.Log(LOG_ERROR, "RFLink: subtype: %d", pLed->subtype);		// rgbw/rgb/white?
		_log.Log(LOG_ERROR, "RFLink: id: %d", pLed->id);				// id
		_log.Log(LOG_ERROR, "RFLink: unit: %d", pLed->dunit);			// unit 0=All, 1=Group1,2=Group2,3=Group3,4=Group4
		_log.Log(LOG_ERROR, "RFLink: command: %d", pLed->command);		// command
		_log.Log(LOG_ERROR, "RFLink: value: %d", pLed->value);			// brightness/color value
        */
		bool bSendOn = false;

		const int m_LEDType = pLed->type;
		std::string switchtype = GetGeneralRFLinkFromInt(rfswitches, 0x57);
		std::string switchcmnd = GetGeneralRFLinkFromInt(rfswitchcommands, pLed->command);
		unsigned int m_colorbright = 0;

		switch (pLed->command){
		case Limitless_LedOn:
			switchcmnd = "ON";
			break;
		case Limitless_LedOff:
			switchcmnd = "OFF";
			break;
		case Limitless_SetRGBColour:
			{
            int iHue = ((pLed->value)+0x20) & 0xFF;  //Milight color offset correction
			m_colorbright = m_colorbright & 0xff;
			m_colorbright = (((unsigned char) iHue) << 8) + m_colorbright;
			switchcmnd = "COLOR";
			bSendOn = true;
		    }
			break;
		case Limitless_DiscoSpeedSlower:
			switchcmnd = "DISCO-";
			bSendOn = true;
			break;
		case Limitless_DiscoSpeedFaster:
			switchcmnd = "DISCO+";
			bSendOn = true;
			break;
		case Limitless_DiscoMode:
			switchcmnd = "MODE";
			break;
		case Limitless_SetColorToWhite:
			switchcmnd = "ALLON";
			bSendOn = true;
			break;
		case Limitless_SetBrightnessLevel:
			{
			//brightness (0-100) converted to 0x00-0xff
			int m_brightness = (unsigned char)pLed->value;
			m_brightness = (m_brightness * 255) / 100;
			m_brightness = m_brightness & 0xff;
			m_colorbright = m_colorbright & 0xff00;
			m_colorbright = m_colorbright + (unsigned char)m_brightness;
			switchcmnd = "BRIGHT";
			bSendOn = true;
		    }
			break;
		case Limitless_NightMode:
			switchcmnd = "ALLOFF";
			bSendOn = true;
			break;
		// need work:
		case Limitless_SetBrightUp:
			switchcmnd = "BRIGHT";
			bSendOn = true;
			break;
		case Limitless_SetBrightDown:
			switchcmnd = "BRIGHT";
			bSendOn = true;
			break;
		case Limitless_DiscoSpeedFasterLong:
			switchcmnd = "DISCO+";
			bSendOn = true;
			break;
		case Limitless_RGBDiscoNext:
			switchcmnd = "DISCO+";
			bSendOn = true;
			break;
		case Limitless_RGBDiscoPrevious:
			switchcmnd = "DISCO-";
			bSendOn = true;
			break;
		default:
			_log.Log(LOG_ERROR, "RFLink: trying to send unknown led switch command: %d", pLed->command);
			return false;
		}

		// --- Sending first an "ON command" needed
		if (bSendOn == true) {
			std::string tswitchcmnd = "ON";
			//Build send string
			std::stringstream sstr;
			sstr << "10;" << switchtype << ";" << std::hex << std::nouppercase << std::setw(4) << std::setfill('0') << pLed->id << ";" << std::setw(2) << std::setfill('0') << int(pLed->dunit) << ";" << std::hex << std::nouppercase << std::setw(4) << m_colorbright << ";" << tswitchcmnd;
			_log.Log(LOG_STATUS, "RFLink Sending: %s", sstr.str().c_str());
			sstr << "\n";
			m_bTXokay = false; // clear OK flag
			WriteInt(sstr.str());
			time_t atime = mytime(NULL);
			time_t btime = mytime(NULL);

			// Wait for an OK response from RFLink to make sure the command was executed
			while (m_bTXokay == false) {
				if (difftime(btime,atime) > 4) {
					_log.Log(LOG_ERROR, "RFLink: TX time out...");
					return false;
				}
				btime = mytime(NULL);
			}
		}
		// ---

		//Build send string
		std::stringstream sstr;
		//10;MiLightv1;1234;01;5566;ON;     => protocol;address;unit number;color&brightness;action (ON/OFF/ALLON/ALLOFF etc)

		sstr << "10;" << switchtype << ";" << std::hex << std::nouppercase << std::setw(4) << std::setfill('0') << pLed->id << ";" << std::setw(2) << std::setfill('0') << int(pLed->dunit) << ";" << std::hex << std::nouppercase << std::setw(4) << m_colorbright << ";" << switchcmnd;
		//#ifdef _DEBUG
		_log.Log(LOG_STATUS, "RFLink Sending: %s", sstr.str().c_str());
		//#endif
		sstr << "\n";
		m_bTXokay = false; // clear OK flag
		WriteInt(sstr.str());
		time_t atime = mytime(NULL);
		time_t btime = mytime(NULL);

		// Wait for an OK response from RFLink to make sure the command was executed
		while (m_bTXokay == false) {
			if (difftime(btime,atime) > 4) {
				_log.Log(LOG_ERROR, "RFLink: TX time out...");
				return false;
			}
			btime = mytime(NULL);
		}
		return true;
	}
}

bool CRFLinkBase::SendSwitchInt(const int ID, const int switchunit, const int BatteryLevel, const std::string &switchType, const std::string &switchcmd, const int level)
{
	int intswitchtype = GetGeneralRFLinkFromString(rfswitches, switchType);
	if (intswitchtype == -1)
	{
		_log.Log(LOG_ERROR, "RFLink: Unhandled switch type: %s", switchType.c_str());
		return false;
	}

	int cmnd = GetGeneralRFLinkFromString(rfswitchcommands, switchcmd);

	int svalue=level;
	if (cmnd==-1) {
		if (switchcmd.compare(0, 10, "SET_LEVEL=") == 0 ){
			cmnd=gswitch_sSetLevel;
			std::string str2 = switchcmd.substr(10);
			svalue=atoi(str2.c_str());
	  		_log.Log(LOG_STATUS, "RFLink: %d level: %d", cmnd, svalue);
		}
	}

	if (cmnd==-1)
	{
		_log.Log(LOG_ERROR, "RFLink: Unhandled switch command: %s", switchcmd.c_str());
		return false;
	}

	_tGeneralSwitch gswitch;
	gswitch.subtype = intswitchtype;
	gswitch.id = ID;
	gswitch.unitcode = switchunit;
	gswitch.cmnd = cmnd;
    gswitch.level = svalue; //level;
	gswitch.battery_level = BatteryLevel;
	gswitch.rssi = 12;
	gswitch.seqnbr = 0;
	sDecodeRXMessage(this, (const unsigned char *)&gswitch, NULL, BatteryLevel);
	return true;
}

static std::string RFLinkGetStringValue(const std::string &svalue)
{
	std::string ret = "";
	size_t pos = svalue.find("=");
	if (pos == std::string::npos)
		return ret;
	return svalue.substr(pos+1);
}
static unsigned int RFLinkGetHexStringValue(const std::string &svalue)
{
	unsigned int ret = -1;
	size_t pos = svalue.find("=");
	if (pos == std::string::npos)
		return ret;
	std::stringstream ss;
	ss << std::hex << svalue.substr(pos + 1);
	ss >> ret;
	return ret;
}
static unsigned int RFLinkGetIntStringValue(const std::string &svalue)
{
	unsigned int ret = -1;
	size_t pos = svalue.find("=");
	if (pos == std::string::npos)
		return ret;
	std::stringstream ss;
	ss << std::dec << svalue.substr(pos + 1);
	ss >> ret;
	return ret;
}

static unsigned int RFLinkGetIntDecStringValue(const std::string &svalue)
{
	unsigned int ret = -1;
	size_t pos = svalue.find(".");
	if (pos == std::string::npos)
		return ret;
	std::stringstream ss;
	ss << std::dec << svalue.substr(pos + 1);
	ss >> ret;
	return ret;
}

bool CRFLinkBase::ParseLine(const std::string &sLine)
{
	m_LastReceivedTime = mytime(NULL);

	std::vector<std::string> results;
	StringSplit(sLine, ";", results);
	if (results.size() < 2)
		return false; //not needed

	bool bHideDebugLog = (
		(sLine.find("PONG") != std::string::npos)||
		(sLine.find("PING") != std::string::npos)
		);

	int RFLink_ID = atoi(results[0].c_str());
	if (RFLink_ID != 20)
	{
		return false; //only accept RFLink->Master messages
	}

#ifdef ENABLE_LOGGING
	if (!bHideDebugLog)
		_log.Log(LOG_NORM, "RFLink: %s", sLine.c_str());
#endif
   if (m_bRFDebug == true) _log.Log(LOG_NORM, "RFLink: %s", sLine.c_str());

	//std::string Sensor_ID = results[1];
	if (results.size() >2)
	{
		//Status reply
		std::string Name_ID = results[2];
		if ((Name_ID.find("Nodo RadioFrequencyLink") != std::string::npos) || (Name_ID.find("RFLink Gateway") != std::string::npos))
		{
			_log.Log(LOG_STATUS, "RFLink: Controller Initialized!...");
			WriteInt("10;VERSION;\n");  // 20;3C;VER=1.1;REV=37;BUILD=01;

			//Enable DEBUG
			//write("10;RFDEBUG=ON;\n");

			//Enable Undecoded DEBUG
			//write("10;RFUDEBUG=ON;\n");
			return true;
		}
		if (Name_ID.find("VER") != std::string::npos) {
			//_log.Log(LOG_STATUS, "RFLink: %s", sLine.c_str());
			int versionlo = 0;
			int versionhi = 0;
			int revision = 0;
			int build = 0;
			if (results[2].find("VER") != std::string::npos) {
				versionhi = RFLinkGetIntStringValue(results[2]);
				versionlo = RFLinkGetIntDecStringValue(results[2]);
			}
			if (results[3].find("REV") != std::string::npos){
				revision = RFLinkGetIntStringValue(results[3]);
			}
			if (results[4].find("BUILD") != std::string::npos) {
				build = RFLinkGetIntStringValue(results[4]);
			}
			_log.Log(LOG_STATUS, "RFLink Detected, Version: %d.%d Revision: %d Build: %d", versionhi, versionlo, revision, build);

			std::stringstream sstr;
			sstr << revision << "." << build;
			m_Version = sstr.str();

			mytime(&m_LastHeartbeatReceive);  // keep heartbeat happy
			mytime(&m_LastHeartbeat);  // keep heartbeat happy
			m_LastReceivedTime = m_LastHeartbeat;

			m_bTXokay = true; // variable to indicate an OK was received
			return true;
		}
		if (Name_ID.find("PONG") != std::string::npos) {
			//_log.Log(LOG_STATUS, "RFLink: PONG received!...");
			mytime(&m_LastHeartbeatReceive);  // keep heartbeat happy
			mytime(&m_LastHeartbeat);  // keep heartbeat happy
			m_LastReceivedTime = m_LastHeartbeat;

			m_bTXokay = true; // variable to indicate an OK was received
			return true;
		}
		if (Name_ID.find("OK") != std::string::npos) {
			//_log.Log(LOG_STATUS, "RFLink: OK received!...");
			mytime(&m_LastHeartbeatReceive);  // keep heartbeat happy
			mytime(&m_LastHeartbeat);  // keep heartbeat happy
			m_LastReceivedTime = m_LastHeartbeat;

			m_bTXokay = true; // variable to indicate an OK was received
			return true;
		}
		else if (Name_ID.find("CMD UNKNOWN") != std::string::npos) {
			_log.Log(LOG_ERROR, "RFLink: Error/Unknown command received!...");
			m_bTXokay = true; // variable to indicate an ERROR was received
			return true;
		}
	}
	if (results.size() < 4)
		return true;

	if (results[3].find("ID=") == std::string::npos)
		return false; //??

	mytime(&m_LastHeartbeatReceive);  // keep heartbeat happy
	mytime(&m_LastHeartbeat);  // keep heartbeat happy
	//_log.Log(LOG_STATUS, "RFLink: t1=%d t2=%d", m_LastHeartbeat, m_LastHeartbeatReceive);
	m_LastReceivedTime = m_LastHeartbeat;

	std::stringstream ss;
	unsigned int ID;
	ss << std::hex << results[3].substr(3);
	ss >> ID;

	int Node_ID = (ID & 0xFF00) >> 8;
	int Child_ID = ID & 0xFF;

	bool bHaveTemp = false; float temp = 0;
	bool bHaveHum = false; int humidity = 0;
	bool bHaveHumStatus = false; int humstatus = 0;
	bool bHaveBaro = false; float baro = 0;
	int baroforecast = 0;
	bool bHaveRain = false; float raincounter = 0;
	bool bHaveLux = false; float lux = 0;
	bool bHaveUV = false; float uv = 0;

	bool bHaveWindDir = false; int windir = 0;
	bool bHaveWindSpeed = false; float windspeed = 0;
	bool bHaveWindGust = false; float windgust = 0;
	bool bHaveWindTemp = false; float windtemp = 0;
	bool bHaveWindChill = false; float windchill = 0;

	bool bHaveRGB = false; int rgb = 0;
	bool bHaveRGBW = false; int rgbw = 0;
	bool bHaveSound = false; int sound = 0;
	bool bHaveCO2 = false; int co2 = 0;
	bool bHaveBlind = false; int blind = 0;

	bool bHaveKWatt = false; float kwatt = 0;
	bool bHaveWatt = false; float watt = 0;
	bool bHaveDistance = false; float distance = 0;
	bool bHaveMeter = false; float meter = 0;
	bool bHaveVoltage = false; float voltage = 0;
	bool bHaveCurrent = false; float current = 0;
	bool bHaveCurrent2 = false; float current2 = 0;
	bool bHaveCurrent3 = false; float current3 = 0;
	bool bHaveWeight = false; float weight = 0;
	bool bHaveImpedance = false; float impedance = 0;
	bool bHaveSwitch = false; int switchunit = 0;
	bool bHaveSwitchCmd = false; std::string switchcmd = ""; int switchlevel = 0;

	int BatteryLevel = 255;
	std::string tmpstr;
	int iTemp;
	for (size_t ii = 4; ii < results.size(); ii++)
	{
		if (results[ii].find("TEMP") != std::string::npos)
		{
			iTemp = RFLinkGetHexStringValue(results[ii]);
			bHaveTemp = true;
			if ((iTemp & 0x8000) == 0x8000) {
				//negative temp
				iTemp = -(iTemp & 0xFFF);
			}
			temp = float(iTemp) / 10.0f;
		}
		else if (results[ii].find("HUM") != std::string::npos)
		{
			bHaveHum = true;
			humidity = RFLinkGetIntStringValue(results[ii]);
		}
		else if (results[ii].find("HSTATUS") != std::string::npos)
		{
			bHaveHumStatus = true;
			humstatus = RFLinkGetIntStringValue(results[ii]);
		}
		else if (results[ii].find("BARO") != std::string::npos)
		{
			iTemp = RFLinkGetHexStringValue(results[ii]);
			bHaveBaro = true;
			baro = float(iTemp);
		}
		else if (results[ii].find("BFORECAST") != std::string::npos)
		{
			baroforecast = RFLinkGetIntStringValue(results[ii]);
		}
		else if (results[ii].find("RAIN") != std::string::npos)
		{
			bHaveRain = true;
			iTemp = RFLinkGetHexStringValue(results[ii]);
			raincounter = float(iTemp) / 10.0f;
		}
		else if (results[ii].find("LUX") != std::string::npos)
		{
			iTemp = RFLinkGetHexStringValue(results[ii]);
			bHaveLux = true;
			lux = float(iTemp);
		}
		else if (results[ii].find("UV") != std::string::npos)
		{
			iTemp = RFLinkGetHexStringValue(results[ii]);
			bHaveUV = true;
			uv = float(iTemp) /10.0f;
		}
		else if (results[ii].find("BAT") != std::string::npos)
		{
			tmpstr = RFLinkGetStringValue(results[ii]);
			BatteryLevel = (tmpstr == "OK") ? 100 : 0;
		}
		else if (results[ii].find("WINDIR") != std::string::npos)
		{
			bHaveWindDir = true;
			windir = RFLinkGetIntStringValue(results[ii]);
		}
		else if (results[ii].find("WINSP") != std::string::npos)
		{
			bHaveWindSpeed = true;
			iTemp = RFLinkGetHexStringValue(results[ii]); // received value is km/u
			windspeed = (float(iTemp) * 0.0277778f);   //convert to m/s
		}
		else if (results[ii].find("WINGS") != std::string::npos)
		{
			bHaveWindGust = true;
			iTemp = RFLinkGetHexStringValue(results[ii]); // received value is km/u
			windgust = (float(iTemp) * 0.0277778f);    //convert to m/s
		}
		else if (results[ii].find("WINTMP") != std::string::npos)
		{
			iTemp = RFLinkGetHexStringValue(results[ii]);
			bHaveWindTemp = true;
			if ((iTemp & 0x8000) == 0x8000) {
				//negative temp
				iTemp = -(iTemp & 0xFFF);
			}
			windtemp = float(iTemp) / 10.0f;
		}
		else if (results[ii].find("WINCHL") != std::string::npos)
		{
			iTemp = RFLinkGetHexStringValue(results[ii]);
			bHaveWindChill = true;
			if ((iTemp & 0x8000) == 0x8000) {
				//negative temp
				iTemp = -(iTemp & 0xFFF);
			}
			windchill = float(iTemp) / 10.0f;
		}
		else if (results[ii].find("SOUND") != std::string::npos)
		{
			bHaveSound = true;
			sound = RFLinkGetIntStringValue(results[ii]);
		}
		else if (results[ii].find("CO2") != std::string::npos)
		{
			bHaveCO2 = true;
			co2 = RFLinkGetIntStringValue(results[ii]);
		}
		else if (results[ii].find("RGBW") != std::string::npos)
		{
			bHaveRGBW = true;
			rgbw = RFLinkGetIntStringValue(results[ii]);
		}
		else if (results[ii].find("RGB") != std::string::npos)
		{
			bHaveRGB = true;
			rgb = RFLinkGetIntStringValue(results[ii]);
		}
		else if (results[ii].find("BLIND") != std::string::npos)
		{
			bHaveBlind = true;
			blind = RFLinkGetIntStringValue(results[ii]);
		}
		else if (results[ii].find("KWATT") != std::string::npos)
		{
			iTemp = RFLinkGetHexStringValue(results[ii]);
			bHaveKWatt = true;
			kwatt = float(iTemp) / 1000.0f;
		}
		else if (results[ii].find("WATT") != std::string::npos)
		{
			iTemp = RFLinkGetHexStringValue(results[ii]);
			bHaveWatt = true;
			watt = float(iTemp) / 10.0f;
		}
		else if (results[ii].find("DIST") != std::string::npos)
		{
			iTemp = RFLinkGetHexStringValue(results[ii]);
			bHaveDistance = true;
			distance = float(iTemp) / 10.0f;
		}
		else if (results[ii].find("METER") != std::string::npos)
		{
			iTemp = RFLinkGetHexStringValue(results[ii]);
			bHaveMeter = true;
			meter = float(iTemp) / 10.0f;
		}
		else if (results[ii].find("VOLT") != std::string::npos)
		{
			iTemp = RFLinkGetHexStringValue(results[ii]);
			bHaveVoltage = true;
			voltage = float(iTemp) / 10.0f;
		}
		else if (results[ii].find("CURRENT") != std::string::npos)
		{
			iTemp = RFLinkGetHexStringValue(results[ii]);
			bHaveCurrent = true;
			current = float(iTemp) / 10.0f;
		}
		else if (results[ii].find("CURRENT2") != std::string::npos)
		{
			iTemp = RFLinkGetHexStringValue(results[ii]);
			bHaveCurrent2 = true;
			current2 = float(iTemp) / 10.0f;
		}
		else if (results[ii].find("CURRENT3") != std::string::npos)
		{
			iTemp = RFLinkGetHexStringValue(results[ii]);
			bHaveCurrent3 = true;
			current3 = float(iTemp) / 10.0f;
		}
		else if (results[ii].find("WEIGHT") != std::string::npos)
		{
			iTemp = RFLinkGetHexStringValue(results[ii]);
			bHaveWeight = true;
			weight = float(iTemp) *100;			// weight in grams
		}
		else if (results[ii].find("IMPEDANCE") != std::string::npos)
		{
			iTemp = RFLinkGetHexStringValue(results[ii]);
			bHaveImpedance = true;
			impedance = float(iTemp) / 10.0f;
		}
		else if (results[ii].find("SWITCH") != std::string::npos)
		{
			bHaveSwitch = true;
			switchunit = RFLinkGetHexStringValue(results[ii]);
		}
		else if (results[ii].find("CMD") != std::string::npos)
		{
			bHaveSwitchCmd = true;
			switchcmd = RFLinkGetStringValue(results[ii]);
		}
		else if (results[ii].find("SMOKEALERT") != std::string::npos)
		{
			bHaveSwitch = true;
			switchunit = 1;
			bHaveSwitchCmd = true;
			switchcmd = RFLinkGetStringValue(results[ii]);
		}
		else if (results[ii].find("CHIME") != std::string::npos)
		{
			bHaveSwitch = true;
			switchunit = 2;
			bHaveSwitchCmd = true;
			switchcmd = "ON";
		}
	}

	std::string tmp_Name = results[2];
	if (bHaveTemp&&bHaveHum&&bHaveBaro)
	{
		SendTempHumBaroSensor(ID, BatteryLevel, temp, humidity, baro, baroforecast, tmp_Name);
	}
	else if (bHaveTemp&&bHaveHum)
	{
		SendTempHumSensor(ID, BatteryLevel, temp, humidity, tmp_Name);
	}
	else if (bHaveTemp)
	{
		SendTempSensor(ID, BatteryLevel, temp, tmp_Name);
	}
	else if (bHaveHum)
	{
		SendHumiditySensor(ID, BatteryLevel, humidity, tmp_Name);
	}
	else if (bHaveBaro)
	{
		SendBaroSensor(Node_ID, Child_ID, BatteryLevel, baro, baroforecast, tmp_Name);
	}

	if (bHaveLux)
	{
		SendLuxSensor(Node_ID, Child_ID, BatteryLevel, lux, tmp_Name);
	}

	if (bHaveUV)
	{
  		SendUVSensor(Node_ID, Child_ID, BatteryLevel, uv, tmp_Name);
	}

	if (bHaveRain)
	{
		SendRainSensor(ID, BatteryLevel, float(raincounter), tmp_Name);
	}

	if (bHaveWindDir || bHaveWindSpeed || bHaveWindGust || bHaveWindChill)
	{
		bool bExists = false;
		int twindir = 0;float twindspeed = 0;float twindgust = 0;float twindtemp = 0;float twindchill = 0;
		GetWindSensorValue(ID, twindir, twindspeed, twindgust, twindtemp, twindchill, bHaveWindTemp, bExists);
#ifdef _DEBUG
		if (bExists) {
			_log.Log(LOG_STATUS, "RFLink: id:%d wd %d ws %f wg: %f wt: %f wc: %f status:%d", ID, twindir, twindspeed, twindgust, twindtemp, twindchill, bExists);
		}
#endif
		if (bHaveWindDir) twindir = round(float(windir)*22.5f);
		if (!bHaveWindSpeed) windspeed = twindspeed;
		if (!bHaveWindGust) windgust = twindgust;
		if (!bHaveWindTemp) windtemp = twindtemp;
		if (!bHaveWindChill) windchill = twindchill;

		SendWind(ID, BatteryLevel, twindir, windspeed, windgust, windtemp, windchill, bHaveWindTemp, tmp_Name);
	}

	if (bHaveCO2)
	{
		SendAirQualitySensor((ID & 0xFF00) >> 8, ID & 0xFF, BatteryLevel, co2, tmp_Name);
	}
	if (bHaveSound)
	{
		SendSoundSensor(ID, BatteryLevel, sound, tmp_Name);
	}

	if (bHaveBlind)
	{
		SendBlindSensor(Node_ID, Child_ID, BatteryLevel, blind, tmp_Name);
	}

	if (bHaveKWatt&bHaveWatt)
	{
		SendKwhMeterOldWay(Node_ID, Child_ID, BatteryLevel, watt / 100.0f, kwatt, tmp_Name);
	}
	else if (bHaveKWatt)
	{
		SendKwhMeterOldWay(Node_ID, Child_ID, BatteryLevel, watt / 100.0f, kwatt, tmp_Name);
	}
	else if (bHaveWatt)
	{
		SendKwhMeterOldWay(Node_ID, Child_ID, BatteryLevel, watt / 100.0f, kwatt, tmp_Name);
	}
	if (bHaveDistance)
	{
		SendDistanceSensor(Node_ID, Child_ID, BatteryLevel, distance, tmp_Name);
	}
	if (bHaveMeter)
	{
		SendMeterSensor(Node_ID, Child_ID, BatteryLevel, meter, tmp_Name);
	}
	if (bHaveVoltage)
	{
		SendVoltageSensor(Node_ID, Child_ID, BatteryLevel, voltage, tmp_Name);
	}
	if (bHaveCurrent && bHaveCurrent2 && bHaveCurrent3)
	{
		SendCurrentSensor(ID, BatteryLevel, current, current2, current3, tmp_Name);
	}
	else if (bHaveCurrent)
	{
		SendCurrentSensor(ID, BatteryLevel, current, 0, 0, tmp_Name);
	}
	if (bHaveWeight)
	{
		SendCustomSensor(Node_ID, Child_ID, BatteryLevel, weight, "Weight", "g");
	}

	if (bHaveImpedance)
	{
		SendPercentageSensor(Node_ID, Child_ID, BatteryLevel, impedance, tmp_Name);
	}

	if (bHaveRGB)
	{
		//RRGGBB
		if (switchcmd == "ON") rgb = 0xffff;
		SendRGBWSwitch(ID, switchunit, BatteryLevel, rgb, false, tmp_Name);
	} else
	if (bHaveRGBW)
	{
		//RRGGBBWW
		//_log.Log(LOG_STATUS, "RFLink ID,unit,level,cmd: %x , %x, %x, %x", ID, switchunit, rgbw, switchcmd);
		if (switchcmd == "OFF") rgbw = 0;
		SendRGBWSwitch(ID, switchunit, BatteryLevel, rgbw, true, tmp_Name);
	} else
	if (bHaveSwitch && bHaveSwitchCmd)
	{
		std::string switchType = results[2];
		SendSwitchInt(ID, switchunit, BatteryLevel, switchType, switchcmd, switchlevel);
	}

    return true;
}

//Webserver helpers
namespace http {
	namespace server {
		void CWebServer::RType_CreateRFLinkDevice(WebEmSession & session, const request& req, Json::Value &root)
		{
			if (session.rights != 2)
			{
				session.reply_status = reply::forbidden;
				return; //Only admin user allowed
			}

			std::string idx = request::findValue(&req, "idx");
			std::string scommand = request::findValue(&req, "command");
			if (idx.empty() || scommand.empty())
			{
				return;
			}

			#ifdef _DEBUG
			_log.Log(LOG_STATUS, "RFLink Custom Command: %s", scommand.c_str());
			_log.Log(LOG_STATUS, "RFLink Custom Command idx: %s", idx.c_str());
			#endif

			bool bCreated = false;						// flag to know if the command was a success
			CRFLinkBase *pRFLINK = reinterpret_cast<CRFLinkBase*>(m_mainworker.GetHardware(atoi(idx.c_str())));
			if (pRFLINK == NULL)
				return;

			if (scommand.substr(0, 14).compare("10;rfdebug=on;") == 0) {
				pRFLINK->m_bRFDebug = true; // enable debug
				_log.Log(LOG_STATUS, "User: %s initiated RFLink Enable Debug mode with command: %s", session.username.c_str(), scommand.c_str());
				pRFLINK->WriteInt("10;RFDEBUG=ON;\n");
				root["status"] = "OK";
				root["title"] = "DebugON";
				return;
			}
			if (scommand.substr(0, 15).compare("10;rfdebug=off;") == 0) {
				pRFLINK->m_bRFDebug = false; // disable debug
				_log.Log(LOG_STATUS, "User: %s initiated RFLink Disable Debug mode with command: %s", session.username.c_str(), scommand.c_str());
				pRFLINK->WriteInt("10;RFDEBUG=OFF;\n");
				root["status"] = "OK";
				root["title"] = "DebugOFF";
				return;
			}

			_log.Log(LOG_STATUS, "User: %s initiated a RFLink Device Create command: %s", session.username.c_str(), scommand.c_str());
			scommand = "11;" + scommand;
			#ifdef _DEBUG
			_log.Log(LOG_STATUS, "User: %s initiated a RFLink Device Create command: %s", session.username.c_str(), scommand.c_str());
			#endif
			scommand += "\r\n";

			bCreated = true;
			pRFLINK->m_bTXokay = false; // clear OK flag
			pRFLINK->WriteInt(scommand.c_str());
			time_t atime = mytime(NULL);
			time_t btime = mytime(NULL);

			// Wait for an OK response from RFLink to make sure the command was executed
			while (pRFLINK->m_bTXokay == false) {
				if (difftime(btime,atime) > 4) {
					_log.Log(LOG_ERROR, "RFLink: TX time out...");
					bCreated = false;
					break;
				}
				btime = mytime(NULL);
			}

			#ifdef _DEBUG
			_log.Log(LOG_STATUS, "RFLink custom command done");
			#endif

			if (bCreated)
			{
				root["status"] = "OK";
				root["title"] = "CreateRFLinkDevice";
			}
		}
	}
}
