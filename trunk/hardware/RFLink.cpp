#include "stdafx.h"
#include "RFLink.h"
#include "../main/Logger.h"
#include "../main/Helper.h"
#include "../main/RFXtrx.h"
#include "../main/SQLHelper.h"

#include <string>
#include <algorithm>
#include <iostream>
#include <stdlib.h>
#include <boost/bind.hpp>
#include "hardwaretypes.h"
#include "../main/localtime_r.h"

#define ENABLE_LOGGING

#define RFLINK_RETRY_DELAY 30

struct _tRFLinkStringIntHelper
{
	std::string szType;
	int gType;
};

const _tRFLinkStringIntHelper rfswitches[] =
{
	{ "NewKaku", sSwitchTypeAC },            // p4
	{ "Kaku", sSwitchTypeARC },              // p2
	{ "HomeEasy", sSwitchTypeHEU },          // p15
	{ "FA500", sSwitchTypePT2262 },          // p3
	{ "Eurodomest", sSwitchTypeEurodomest }, // p5
	{ "Blyss", sSwitchTypeBlyss },           // p6
	{ "Conrad", sSwitchTypeRSL },            // p7
	{ "Kambrook", sSwitchTypeKambrook },     // p8
	{ "SelectPlus", sSwitchTypeSelectPlus }, // p70
	{ "Byron", sSwitchTypeByronSX },         // p72
	{ "Byron MP", sSwitchTypeByronMP001 },   // p74
	{ "Deltronic", sSwitchTypeSelectPlus3 }, // p73
	{ "X10", sSwitchTypeX10 },               // p9
	{ "EMW100", sSwitchTypeEMW100 },         // p13
	{ "EMW200", sSwitchTypeEMW200 },         // p-
	{ "BSB", sSwitchTypeBBSB },              // p-
	{ "MDRemote", sSwitchTypeMDREMOTE },     // p14
	{ "Waveman", sSwitchTypeWaveman },       // p-
	{ "AB400D", sSwitchTypeAB400D },         // p-
	{ "Impuls", sSwitchTypeIMPULS },         // p16
	{ "Anslut", sSwitchTypeANSLUT },         // p17
	{ "Lightwave", sSwitchTypeLightwaveRF }, // p18
	{ "", -1 }
};

const _tRFLinkStringIntHelper rfswitchcommands[] =
{
	{ "ON", gswitch_sOn },
	{ "OFF", gswitch_sOff },
	{ "ALLON", gswitch_sGroupOn },
	{ "ALLOFF", gswitch_sGroupOff },
	{ "", -1 }
};

int GetGeneralRFLinkFromString(const _tRFLinkStringIntHelper *pTable, const std::string &szType)
{
	int ii = 0;
	while (!pTable[ii].szType.empty())
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
	while (!pTable[ii].szType.empty())
	{
		if (pTable[ii].gType == gType)
			return pTable[ii].szType;
		ii++;
	}
	return "";
}

CRFLink::CRFLink(const int ID, const std::string& devname)
{
	m_HwdID=ID;
	m_szSerialPort=devname;
	m_rfbufferpos=0;
	memset(&m_rfbuffer,0,sizeof(m_rfbuffer));
	m_stoprequested=false;

	/*
	ParseLine("20;08;NewKaku;ID=31c42a;SWITCH=2;CMD=OFF;");
	ParseLine("20;3A;NewKaku;ID=c142;SWITCH=1;CMD=ALLOFF;");
	ParseLine("20;14;Oregon BTHR;ID=5a00;TEMP=00d1;HUM=29;BARO=0407;BAT=LOW;");
	ParseLine("20;27;Cresta;ID=8001;RAIN=69e7;");
	ParseLine("20;2D;UPM/Esic;ID=0001;TEMP=00cf;HUM=16;BAT=OK;");
	ParseLine("20;31;Mebus;ID=c201;TEMP=00cf;");
	ParseLine("20;32;Auriol;ID=008f;TEMP=00d3;BAT=OK;");
	ParseLine("20;AF;SelectPlus;ID=1bb4;CHIME=01;");
	ParseLine("20;12;Pir;ID=aa66;PIR=ON;");
	ParseLine("20;63;SmokeAlert;ID=1234;SMOKEALERT=ON;");
	ParseLine("20;06;Kaku;ID=41;SWITCH=A1;CMD=ON;");
	ParseLine("20;0C;Kaku;ID=41;SWITCH=A2;CMD=OFF;");
	ParseLine("20;0D;Kaku;ID=41;SWITCH=A2;CMD=ON;");
	ParseLine("20;46;Kaku;ID=44;SWITCH=D4;CMD=OFF;");
	ParseLine("20;3A;NewKaku;ID=c142;SWITCH=1;CMD=ALLOFF;");
	ParseLine("20;3B;NewKaku;ID=c142;SWITCH=3;CMD=OFF;");
	ParseLine("20;0C;HomeEasy;ID=7900b200;SWITCH=0b;CMD=ALLON;");
	ParseLine("20;AD;FA500;ID=0d00b900;SWITCH=0001;CMD=UNKOWN;");
	ParseLine("20;AE;FA500;ID=0a01;SWITCH=0a01;CMD=OFF;");
	ParseLine("20;03;Eurodomest;ID=03696b;SWITCH=00;CMD=OFF;");
	ParseLine("20;04;Eurodomest;ID=03696b;SWITCH=07;CMD=ALLOFF;");
	ParseLine("20;03;Cresta;ID=9701;WINDIR=009d;WINSP=0001;WINGS=0000;WINCHL=003d;");
	*/
}

CRFLink::~CRFLink()
{
	clearReadCallback();
}

bool CRFLink::StartHardware()
{
	m_retrycntr=RFLINK_RETRY_DELAY*5; //will force reconnect first thing

	//Start worker thread
	m_thread = boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&CRFLink::Do_Work, this)));

	return (m_thread!=NULL);
}

bool CRFLink::StopHardware()
{
	m_stoprequested=true;
	if (m_thread)
	{
		m_thread->join();
		// Wait a while. The read thread might be reading. Adding this prevents a pointer error in the async serial class.
		sleep_milliseconds(10);
	}
	if (isOpen())
	{
		try {
			clearReadCallback();
			close();
			doClose();
			setErrorStatus(true);
		} catch(...)
		{
			//Don't throw from a Stop command
		}
	}
	m_bIsStarted=false;
	return true;
}


void CRFLink::Do_Work()
{
	int msec_counter = 0;
	int sec_counter = 0;
	while (!m_stoprequested)
	{
		sleep_milliseconds(200);
		if (m_stoprequested)
			break;

		msec_counter++;
		if (msec_counter == 5)
		{
			msec_counter = 0;
			sec_counter++;

			if (sec_counter % 12 == 0) {
				m_LastHeartbeat = mytime(NULL);
			}
			if (isOpen())
			{
				if (sec_counter % 10 == 0)
				{
					//Send ping (keep alive)
					time_t atime = mytime(NULL);
					if (atime - m_LastReceivedTime > 15)
					{
						//Timeout
						_log.Log(LOG_ERROR, "RFLink: Not received anything for more then 15 seconds, restarting...");
						m_retrycntr = (RFLINK_RETRY_DELAY-3) * 5;
						m_LastReceivedTime = atime;
						try {
							clearReadCallback();
							close();
							doClose();
							setErrorStatus(true);
						}
						catch (...)
						{
							//Don't throw from a Stop command
						}
					}
					else
						write("10;PING;\n");
				}
			}
		}

		if (!isOpen())
		{
			if (m_retrycntr==0)
			{
				_log.Log(LOG_STATUS,"RFLink: serial retrying in %d seconds...", RFLINK_RETRY_DELAY);
			}
			m_retrycntr++;
			if (m_retrycntr/5>=RFLINK_RETRY_DELAY)
			{
				m_retrycntr=0;
				m_rfbufferpos=0;
				OpenSerialDevice();
			}
		}
		/*
		if (m_sendqueue.size()>0)
		{
			boost::lock_guard<boost::mutex> l(m_sendMutex);

			std::vector<std::string>::iterator itt=m_sendqueue.begin();
			if (itt!=m_sendqueue.end())
			{
				std::string sBytes=*itt;
				write(sBytes.c_str(),sBytes.size());
				m_sendqueue.erase(itt);
			}
		}
		*/
	}
	_log.Log(LOG_STATUS,"RFLink: Serial Worker stopped...");
}

/*
void CRFLink::Add2SendQueue(const char* pData, const size_t length)
{
	std::string sBytes;
	sBytes.insert(0,pData,length);
	boost::lock_guard<boost::mutex> l(m_sendMutex);
	m_sendqueue.push_back(sBytes);
}
*/

bool CRFLink::OpenSerialDevice()
{
	//Try to open the Serial Port
	try
	{
		open(m_szSerialPort, 57600);
		_log.Log(LOG_STATUS,"RFLink: Using serial port: %s", m_szSerialPort.c_str());
	}
	catch (boost::exception & e)
	{
		_log.Log(LOG_ERROR,"RFLink: Error opening serial port!");
#ifdef _DEBUG
		_log.Log(LOG_ERROR,"-----------------\n%s\n----------------", boost::diagnostic_information(e).c_str());
#endif
		return false;
	}
	catch ( ... )
	{
		_log.Log(LOG_ERROR,"RFLink: Error opening serial port!!!");
		return false;
	}
	m_bIsStarted=true;
	m_LastReceivedTime = mytime(NULL);

	setReadCallback(boost::bind(&CRFLink::readCallback, this, _1, _2));
	sOnConnected(this);

	return true;
}

void CRFLink::readCallback(const char *data, size_t len)
{
	boost::lock_guard<boost::mutex> l(readQueueMutex);
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

bool CRFLink::WriteToHardware(const char *pdata, const unsigned char length)
{
	if (!isOpen())
		return false;

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

	sstr << "10;" << switchtype << ";" << std::hex << std::nouppercase << std::setw(6) << std::setfill('0') << pSwitch->id << ";" << std::hex << std::nouppercase << pSwitch->unitcode << ";" << switchcmnd << ";\n";

#ifdef _DEBUG
	_log.Log(LOG_STATUS, "RFLink Sending: %s", sstr.str().c_str());
#endif
    m_bTXokay=false; // clear OK flag
	write(sstr.str());
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

bool CRFLink::SendSwitchInt(const int ID, const int switchunit, const int BatteryLevel, const std::string &switchType, const std::string &switchcmd, const int level)
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

bool CRFLink::ParseLine(const std::string &sLine)
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

	std::string Sensor_ID = results[1];
	if (results.size() >2)
	{
		//Status reply
		std::string Name_ID = results[2];
		if (Name_ID.find("Nodo RadioFrequencyLink") != std::string::npos)
		{
			_log.Log(LOG_STATUS, "RFLink: Controller Initialized!...");
			//Enable DEBUG
			//write("10;RFDEBUG=ON;\n");

			//Enable Undecoded DEBUG
			//write("10;RFUDEBUG=ON;\n");
			return true;
		}
		if (Name_ID.find("OK") != std::string::npos) {
			//_log.Log(LOG_STATUS, "RFLink: OK received!...");
            m_bTXokay = true; // variable to indicate an OK was received
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

	bool bHaveWindDir = false; int windir = 0;
	bool bHaveWindSpeed = false; float windspeed = 0;
	bool bHaveWindGust = false; float windgust = 0;
	bool bHaveWindTemp = false; float windtemp = 0;
	bool bHaveWindChill = false; float windchill = 0;

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
		else if (results[ii].find("UV") != std::string::npos)
		{
			iTemp = RFLinkGetHexStringValue(results[ii]);
			bHaveLux = true;
			lux = float(iTemp);
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
		SendLuxSensor(Node_ID, Child_ID, BatteryLevel, lux);
	}

	if (bHaveRain)
	{
		SendRainSensor(ID, BatteryLevel, raincounter);
	}

	if (bHaveWindDir && bHaveWindSpeed && bHaveWindGust && bHaveWindChill)
	{
		SendWind(ID, BatteryLevel, windir, windspeed, windgust, windtemp, windchill, bHaveWindTemp);
	}
	else if (bHaveWindDir && bHaveWindGust)
	{
		SendWind(ID, BatteryLevel, windir, windspeed, windgust, windtemp, windchill, bHaveWindTemp);
	}
	else if (bHaveWindSpeed)
	{
		SendWind(ID, BatteryLevel, windir, windspeed, windgust, windtemp, windchill, bHaveWindTemp);
	}

	if (bHaveSwitch && bHaveSwitchCmd)
	{
		std::string switchType = results[2];
		SendSwitchInt(ID, switchunit, BatteryLevel, switchType, switchcmd, switchlevel);
	}

    return true;
}

