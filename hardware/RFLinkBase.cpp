#include "stdafx.h"
#include "RFLinkBase.h"
#include "../main/Logger.h"
#include "../main/Helper.h"
#include "../main/RFXtrx.h"
#include "hardwaretypes.h"
#include "../main/localtime_r.h"

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
	{ "NewKaku", sSwitchTypeAC },            // p4
	{ "Kaku", sSwitchTypeARC },              // p3
	{ "HomeEasy", sSwitchTypeHEU },          // p15
	{ "PT2262", sSwitchTypePT2262 },         // p3
	{ "Eurodomest", sSwitchTypeEurodomest }, // p5
	{ "Blyss", sSwitchTypeBlyss },           // p6
	{ "Conrad", sSwitchTypeRSL },            // p7
	{ "Kambrook", sSwitchTypeKambrook },     // p8
	{ "SelectPlus", sSwitchTypeSelectPlus }, // p70
	{ "Byron", sSwitchTypeByronSX },         // p72
	{ "Byron MP", sSwitchTypeByronMP001 },   // p74
	{ "Doorbell", sSwitchTypeSelectPlus3 },  // p73  
	{ "X10", sSwitchTypeX10 },               // p9
	{ "EMW100", sSwitchTypeEMW100 },         // p13
	{ "EMW200", sSwitchTypeEMW200 },         // p-
	{ "BSB", sSwitchTypeBBSB },              // p-
	{ "MDRemote", sSwitchTypeMDREMOTE },     // p14
	{ "Waveman", sSwitchTypeWaveman },       // p-
	{ "AB400D", sSwitchTypeAB400D },         // p3
	{ "Impuls", sSwitchTypeIMPULS },         // p3
	{ "Anslut", sSwitchTypeANSLUT },         // p17
	{ "Lightwave", sSwitchTypeLightwaveRF }, // p18
	{ "FA20RF", sSwitchTypeFA20 },           // p80
	{ "GDR2", sSwitchTypeGDR2 },             // p-
	{ "RisingSun", sSwitchTypeRisingSun },   // p-
	{ "Philips", sSwitchTypePhilips },       // p-
	{ "Energenie", sSwitchTypeEnergenie },   // p-
	{ "Energenie5", sSwitchTypeEnergenie5 }, // p-
	{ "Ikea Koppla", sSwitchTypeKoppla },    // p14
	{ "TRC02RGB", sSwitchTypeTRC02 },        // p10
	{ "TRC022RGB", sSwitchTypeTRC02_2 },     // p-
	{ "Livolo", sSwitchTypeLivolo },         // p-
	{ "Livolo App", sSwitchTypeLivoloAppliance }, // p-
	{ "Aoke", sSwitchTypeAoke },             // p-
	{ "Powerfix", sSwitchTypePowerfix },     // p13
	{ "TriState", sSwitchTypeTriState },     // p16
    { "Deltronic", sSwitchTypeDeltronic },   // p73
    { "FA500", sSwitchTypeFA500 },           // p12
    { "Chuango", sSwitchTypeChuango },       // p62
    { "Plieger", sSwitchTypePlieger },       // p71
    { "SilverCrest", sSwitchTypeSilvercrest }, // p75
    { "Mertik", sSwitchTypeMertik },         // p82
    { "HomeConfort", sSwitchTypeHomeConfort }, // p11
	{ "HT12E", sSwitchTypeHT12E },           // p60
	{ "EV1527", sSwitchTypeEV1527 },         // p61
	{ "Elmes", sSwitchTypeElmes },           // p65
	{ "Aster", sSwitchTypeAster },           // p17
	{ "Sartano", sSwitchTypeSartano },       // p3
	{ "Europe", sSwitchTypeEurope },         // p18
	{ "Avidsen", sSwitchTypeAvidsen },       // p..
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

bool CRFLinkBase::WriteToHardware(const char *pdata, const unsigned char length)
{
	_tGeneralSwitch *pSwitch = (_tGeneralSwitch*)pdata;
	if (pSwitch->type != pTypeGeneralSwitch)
		return false; //only allowed to control switches

	std::string switchtype = GetGeneralRFLinkFromInt(rfswitches, pSwitch->subtype);
	if (switchtype.empty())
	{
		_log.Log(LOG_ERROR, "RFLink: trying to send unknown switch type: %d", pSwitch->subtype);
		return false;
	}
	std::string switchcmnd = GetGeneralRFLinkFromInt(rfswitchcommands, pSwitch->cmnd);
    
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
    
	if (switchcmnd.empty())
	{
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
        if (btime-atime > 4) {
			_log.Log(LOG_ERROR, "RFLink: TX time out...");
			return false;
        }
        btime = mytime(NULL);
	}
	return true;
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
		if (switchcmd.compare(0, 9, "SETLEVEL=") ){
			cmnd=gswitch_sSetLevel;
			std::string str2 = switchcmd.substr(10);
			svalue=atoi(str2.c_str()); 
	  		//_log.Log(LOG_STATUS, "RFLink: %d level: %d", cmnd, svalue);
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
	sDecodeRXMessage(this, (const unsigned char *)&gswitch);
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

	//std::string Sensor_ID = results[1];
	if (results.size() >2)
	{
		//Status reply
		std::string Name_ID = results[2];
		if ((Name_ID.find("Nodo RadioFrequencyLink") != std::string::npos) || (Name_ID.find("RFLink Gateway") != std::string::npos))
		{
			_log.Log(LOG_STATUS, "RFLink: Controller Initialized!...");
			//Enable DEBUG
			//write("10;RFDEBUG=ON;\n");

			//Enable Undecoded DEBUG
			//write("10;RFUDEBUG=ON;\n");
			return true;
		}
		if (Name_ID.find("PONG") != std::string::npos) {
			//_log.Log(LOG_STATUS, "RFLink: PONG received!...");
            mytime(&m_LastHeartbeatReceive);  // keep heartbeat happy
            m_bTXokay = true; // variable to indicate an OK was received
			return true;
		}
		if (Name_ID.find("OK") != std::string::npos) {
			//_log.Log(LOG_STATUS, "RFLink: OK received!...");
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
	bool bHaveRain = false; int raincounter = 0;
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
			raincounter = RFLinkGetHexStringValue(results[ii]);
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
			uv = float(iTemp);
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
			iTemp = RFLinkGetHexStringValue(results[ii]);
			windspeed = float(iTemp) * 0.0277778f; //convert to m/s
		}
		else if (results[ii].find("WINGS") != std::string::npos)
		{
			bHaveWindGust = true;
			iTemp = RFLinkGetHexStringValue(results[ii]);
			windgust = float(iTemp) * 0.0277778f; //convert to m/s
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
			kwatt = float(iTemp) / 10.0f;
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
		else if (results[ii].find("IMPEDANCE") != std::string::npos)
		{
			iTemp = RFLinkGetHexStringValue(results[ii]);
			bHaveCurrent = true;
			current = float(iTemp) / 10.0f;
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
	}

	if (bHaveTemp&&bHaveHum&&bHaveBaro)
	{
		SendTempHumBaroSensor(ID, BatteryLevel, temp, humidity, baro, baroforecast);
	}
	else if (bHaveTemp&&bHaveHum)
	{
		SendTempHumSensor(ID, BatteryLevel, temp, humidity, "TempHum");
	}
	else if (bHaveTemp)
	{
		SendTempSensor(ID, BatteryLevel, temp,"Temp");
	}
	else if (bHaveHum)
	{
		SendHumiditySensor(ID, BatteryLevel, humidity);
	}
	else if (bHaveBaro)
	{
		SendBaroSensor(Node_ID, Child_ID, BatteryLevel, baro, baroforecast);
	}

	if (bHaveLux)
	{
		SendLuxSensor(Node_ID, Child_ID, BatteryLevel, lux, "Lux");
	}

	if (bHaveUV)
	{
  		SendUVSensor(Node_ID, Child_ID, BatteryLevel, uv, "UV");
	}
    
	if (bHaveRain)
	{
		SendRainSensor(ID, BatteryLevel, float(raincounter), "Rain");
	}

	if (bHaveWindDir && bHaveWindSpeed && bHaveWindGust && bHaveWindChill)
	{
		SendWind(ID, BatteryLevel, float(windir), windspeed, windgust, windtemp, windchill, bHaveWindTemp, "Wind");
	}
	else if (bHaveWindDir && bHaveWindGust)
	{
		SendWind(ID, BatteryLevel, float(windir), windspeed, windgust, windtemp, windchill, bHaveWindTemp, "Wind");
	}
	else if (bHaveWindSpeed)
	{
		SendWind(ID, BatteryLevel, float(windir), windspeed, windgust, windtemp, windchill, bHaveWindTemp, "Wind");
	}
    
	if (bHaveCO2)
	{
		SendAirQualitySensor((ID & 0xFF00) >> 8, ID & 0xFF, BatteryLevel, co2, "CO2");
	}
	if (bHaveSound)
	{
		SendSoundSensor(ID, BatteryLevel, sound, "Sound");
	}

	if (bHaveRGB)
	{
		//RRGGBB
		SendRGBWSwitch(Node_ID, Child_ID, BatteryLevel, rgb, false, "RGB Light");
	}
	if (bHaveRGBW)
	{
		//RRGGBBWW
		SendRGBWSwitch(Node_ID, Child_ID, BatteryLevel, rgbw, true, "RGBW Light");
	}
	if (bHaveBlind)
	{
		SendBlindSensor(Node_ID, Child_ID, BatteryLevel, blind, "Blinds/Window");
	}

	if (bHaveKWatt)
	{
		SendKwhMeterOldWay(Node_ID, Child_ID, BatteryLevel, kwatt / 1000.0f, kwatt, "Meter");
	}
	if (bHaveWatt)
	{
		SendKwhMeterOldWay(Node_ID, Child_ID, BatteryLevel, 0, watt, "Meter");
	}
	if (bHaveDistance)
	{
		SendDistanceSensor(Node_ID, Child_ID, BatteryLevel, distance, "Distance");
	}
	if (bHaveMeter)
	{
		SendMeterSensor(Node_ID, Child_ID, BatteryLevel, meter, "Meter");
	}
	if (bHaveVoltage)
	{
		SendVoltageSensor(Node_ID, Child_ID, BatteryLevel, voltage, "Voltage");
	}
	if (bHaveCurrent)
	{
		SendCurrentSensor(ID, BatteryLevel, current, 0, 0, "Current");
	}
	if (bHaveImpedance)
	{
		SendPercentageSensor(Node_ID, Child_ID, BatteryLevel, impedance, "Impedance");
	}
	if (bHaveSwitch && bHaveSwitchCmd)
	{
		std::string switchType = results[2];
		SendSwitchInt(ID, switchunit, BatteryLevel, switchType, switchcmd, switchlevel);
	}

    return true;
}

