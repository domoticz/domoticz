#include "stdafx.h"
#include "OTGWBase.h"
#include "../main/Logger.h"
#include "../main/Helper.h"
#include "../main/RFXtrx.h"
#include "../main/SQLHelper.h"
#include "../main/localtime_r.h"
#include "../main/mainworker.h"
#include "../main/WebServerHelper.h"
#include "../webserver/cWebem.h"
#include "hardwaretypes.h"
#include <string>
#include <algorithm>
#include <iostream>
#include <boost/bind.hpp>
#include "../json/json.h"
#include <bitset>
#include <ctime>

#define round(a) ( int ) ( a + .5 )

extern http::server::CWebServerHelper m_webservers;

OTGWBase::OTGWBase(void) :
	m_Version("--")
{
	m_OutsideTemperatureIdx=0;//use build in
	m_bufferpos = 0;
	m_OverrideTemperature = 0.0f;
	m_bRequestVersion = true;
}

OTGWBase::~OTGWBase(void)
{
}

void OTGWBase::SetModes(const int Mode1, const int Mode2, const int Mode3, const int Mode4, const int Mode5, const int Mode6)
{
	m_OutsideTemperatureIdx=Mode1;
}

void OTGWBase::ParseData(const unsigned char *pData, int Len)
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

		if(c == 0x0a || m_bufferpos == sizeof(m_buffer) - 1)
		{
			// discard newline, close string, parse line and clear it.
			if(m_bufferpos > 0) m_buffer[m_bufferpos] = 0;
			ParseLine();
			m_bufferpos = 0;
		}
		else
		{
			m_buffer[m_bufferpos] = c;
			m_bufferpos++;
		}
		ii++;
	}
}

void OTGWBase::UpdateSwitch(const unsigned char Idx, const bool bOn, const std::string &defaultname)
{
	char szIdx[10];
	sprintf(szIdx,"%X%02X%02X%02X",0,0,0,Idx);
	std::vector<std::vector<std::string> > result;
	result = m_sql.safe_query("SELECT Name,nValue,sValue FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='%q')", m_HwdID, szIdx);
	if (!result.empty())
	{
		//check if we have a change, if not do not update it
		int nvalue=atoi(result[0][1].c_str());
		if ((!bOn)&&(nvalue==0))
			return;
		if ((bOn&&(nvalue!=0)))
			return;
	}

	//Send as Lighting 2
	tRBUF lcmd;
	memset(&lcmd,0,sizeof(RBUF));
	lcmd.LIGHTING2.packetlength=sizeof(lcmd.LIGHTING2)-1;
	lcmd.LIGHTING2.packettype=pTypeLighting2;
	lcmd.LIGHTING2.subtype=sTypeAC;
	lcmd.LIGHTING2.id1=0;
	lcmd.LIGHTING2.id2=0;
	lcmd.LIGHTING2.id3=0;
	lcmd.LIGHTING2.id4=Idx;
	lcmd.LIGHTING2.unitcode=1;
	int level=15;
	if (!bOn)
	{
		level=0;
		lcmd.LIGHTING2.cmnd=light2_sOff;
	}
	else
	{
		level=15;
		lcmd.LIGHTING2.cmnd=light2_sOn;
	}
	lcmd.LIGHTING2.level=level;
	lcmd.LIGHTING2.filler=0;
	lcmd.LIGHTING2.rssi=12;
	sDecodeRXMessage(this, (const unsigned char *)&lcmd.LIGHTING2, defaultname.c_str(), 255);
}

void OTGWBase::UpdateSetPointSensor(const unsigned char Idx, const float Temp, const std::string &defaultname)
{
	_tThermostat thermos;
	thermos.subtype=sTypeThermSetpoint;
	thermos.id1=0;
	thermos.id2=0;
	thermos.id3=0;
	thermos.id4=Idx;
	thermos.dunit=0;

	thermos.temp=Temp;

	sDecodeRXMessage(this, (const unsigned char *)&thermos, defaultname.c_str(), 255);
}

bool OTGWBase::GetOutsideTemperatureFromDomoticz(float &tvalue)
{
	if (m_OutsideTemperatureIdx == 0)
		return false;
	Json::Value tempjson;
	std::stringstream sstr;
	sstr << m_OutsideTemperatureIdx;
	m_webservers.GetJSonDevices(tempjson, "", "temp", "ID", sstr.str(), "", "", true, false, false, 0, "");

	size_t tsize = tempjson.size();
	if (tsize < 1)
		return false;

	Json::Value::const_iterator itt;
	Json::ArrayIndex rsize = tempjson["result"].size();
	if (rsize < 1)
		return false;

	bool bHaveTimeout = tempjson["result"][0]["HaveTimeout"].asBool();
	if (bHaveTimeout)
		return false;
	tvalue = tempjson["result"][0]["Temp"].asFloat();
	return true;
}

bool OTGWBase::SwitchLight(const int idx, const std::string &LCmd, const int svalue)
{
	char szCmd[100];
	char szOTGWCommand[3] = "-";
	if (idx == 102)
	{
		sprintf(szOTGWCommand, "HW");
	}
	else if (idx == 101)
	{
		sprintf(szOTGWCommand, "CH");
	}
	else if (idx == 96)
	{
		sprintf(szOTGWCommand, "GA");
	}
	else if (idx == 97)
	{
		sprintf(szOTGWCommand, "GB");
	}
	if (szOTGWCommand[0] != '-')
	{
		if (LCmd == "On")
		{
			sprintf(szCmd, "%s=1\r\n", szOTGWCommand);
		}
		else if (LCmd == "Off")
		{
			sprintf(szCmd, "%s=0\r\n", szOTGWCommand);
		}
		else
		{
			_log.Log(LOG_ERROR, "(OTGW) Invalid switch command received!");
			return false;
		}
		WriteInt((const unsigned char*)&szCmd, (const unsigned char)strlen(szCmd));
	}
	return true;
}

bool OTGWBase::WriteToHardware(const char *pdata, const unsigned char length)
{
	const tRBUF *pSen = reinterpret_cast<const tRBUF*>(pdata);

	unsigned char packettype = pSen->ICMND.packettype;
	unsigned char subtype = pSen->ICMND.subtype;

	if (packettype == pTypeLighting2)
	{
		std::string LCmd = "";
		int nodeID = 0;
		int svalue = 0;
		//light command
		nodeID = pSen->LIGHTING2.id4;
		if ((pSen->LIGHTING2.cmnd == light2_sOff) || (pSen->LIGHTING2.cmnd == light2_sGroupOff))
		{
			LCmd = "Off";
			svalue = 0;
		}
		else if ((pSen->LIGHTING2.cmnd == light2_sOn) || (pSen->LIGHTING2.cmnd == light2_sGroupOn))
		{
			LCmd = "On";
			svalue = 255;
		}
		SwitchLight(nodeID, LCmd, svalue);
  }
  else if ((packettype == pTypeThermostat) && (subtype == sTypeThermSetpoint))
  {
      const _tThermostat *pMeter=reinterpret_cast<const _tThermostat*>(pdata);
      float temp = pMeter->temp;
      unsigned char idx = pMeter->id4;
      SetSetpoint(idx, temp);
   }
  else
  {
     _log.Log(LOG_STATUS, "(OTGW) Skipping writing to Hardware for type: %02X, subType: %02X", packettype, subtype);
	}
	return true;
}

void OTGWBase::GetVersion()
{
	char szCmd[30];
	strcpy(szCmd, "PR=A\r\n");
	WriteInt((const unsigned char*)&szCmd, (const unsigned char)strlen(szCmd));
}

void OTGWBase::GetGatewayDetails()
{
	
	char szCmd[30];
	strcpy(szCmd, "PR=G\r\n");
	WriteInt((const unsigned char*)&szCmd, (const unsigned char)strlen(szCmd));
	strcpy(szCmd, "PR=I\r\n");
	WriteInt((const unsigned char*)&szCmd, (const unsigned char)strlen(szCmd));
	strcpy(szCmd, "PR=O\r\n");
	WriteInt((const unsigned char*)&szCmd, (const unsigned char)strlen(szCmd));
	// Statusreport PS=1 is no longer needed since it now parses every message from OT. Previous it requested all values (and thus needed to send a command). No it just listens (no TX over serial device needed thus more error prone)
	//strcpy(szCmd, "PS=1\r\n");
	//WriteInt((const unsigned char*)&szCmd, (const unsigned char)strlen(szCmd));
	//strcpy(szCmd, "PS=0\r\n");
	//WriteInt((const unsigned char*)&szCmd, (const unsigned char)strlen(szCmd));
}

void OTGWBase::SendTime()
{
	time_t atime = mytime(NULL);
	struct tm ltime;
	localtime_r(&atime, &ltime);

	int lday = 0;
	if (ltime.tm_wday == 0)
		lday = 7;
	else
		lday = ltime.tm_wday;

	char szCmd[20];
	sprintf(szCmd, "SC=%d:%02d/%d\r\n", ltime.tm_hour, ltime.tm_min, lday);
	WriteInt((const unsigned char*)&szCmd, (const unsigned char)strlen(szCmd));
}

void OTGWBase::SendOutsideTemperature()
{
	float temp;
	if (!GetOutsideTemperatureFromDomoticz(temp))
		return;
	char szCmd[30];
	sprintf(szCmd, "OT=%.1f\r\n", temp);
	WriteInt((const unsigned char*)&szCmd, (const unsigned char)strlen(szCmd));
}

void OTGWBase::SetSetpoint(const int idx, const float temp)
{
	char szCmd[30];
	if (idx == 1)
	{
		//Control Set Point (MsgID=1)
		_log.Log(LOG_STATUS, "(OTGW) Setting Control SetPoint to: %.1f", temp);
		sprintf(szCmd, "CS=%.1f\r\n", temp);
		WriteInt((const unsigned char*)&szCmd, (const unsigned char)strlen(szCmd));
	}
	else if (idx == 16)
	{
		//Room Set Point
		//Make this a temporarily Set Point, this will be overridden when the thermostat changes/applying it's program
		_log.Log(LOG_STATUS, "(OTGW) Setting Room SetPoint to: %.1f", temp);
		sprintf(szCmd, "TT=%.1f\r\n", temp);
		WriteInt((const unsigned char*)&szCmd, (const unsigned char)strlen(szCmd));
		UpdateSetPointSensor(idx, temp, "Room Setpoint");
	}
	else if (idx == 56)
	{
		//DHW setpoint (MsgID=56)
		_log.Log(LOG_STATUS, "(OTGW) Setting DHW SetPoint to: %.1f", temp);
		sprintf(szCmd, "SW=%.1f\r\n", temp);
		WriteInt((const unsigned char*)&szCmd, (const unsigned char)strlen(szCmd));
		UpdateSetPointSensor(idx, temp, "DHW Setpoint");
	}
	else if (idx == 57)
	{
		//Max CH water setpoint (MsgID=57)
		_log.Log(LOG_STATUS, "(OTGW) Setting Max CH water SetPoint to: %.1f", temp);
		sprintf(szCmd, "SH=%.1f\r\n", temp);
		WriteInt((const unsigned char*)&szCmd, (const unsigned char)strlen(szCmd));
		UpdateSetPointSensor(idx, temp, "Max_CH Water Setpoint");
	}
	GetGatewayDetails();
}

float OTGWBase::msgDataToFloat(std::string msg)
{
	int val1 = std::stoi(std::string(msg, 5, 2), nullptr, 16);
	int val2 = std::stoi(std::string(msg, 7, 2), nullptr, 16);
	std::string sp = std::to_string(val1) + "." + std::to_string(val2);

	return static_cast<float>(atof(sp.c_str()));
}

long OTGWBase::msgDataToUInt(std::string msg)
{
	long val = std::stoi(std::string(msg, 5, 4), nullptr, 16);;
	return val;
}
/*
int OTGWBase::msgDataToHInt(std::string msg)
{
	int val = std::stoi(std::string(msg, 5, 2), nullptr, 16);
	std::string sp = std::to_string(val);
	return static_cast<int>(atoi(sp.c_str()));
}

int OTGWBase::msgDataToLInt(std::string msg)
{
	int val = std::stoi(std::string(msg, 7, 2), nullptr, 16);
	std::string sp = std::to_string(val);
	return static_cast<int>(atoi(sp.c_str()));
}
*/
void OTGWBase::ParseLine()
{
	if (m_bufferpos < 2)
		return;
	std::string sLine((char*)&m_buffer);
	/*
	<msg - type> (id = <data - id>, <data - value>)						READ-DATA (id=1, ControlSetpoint-value)
	<msg - type> (id = <data - id>, <data - byte1>, <data - byte2>)		READ-DATA (id=11, TSP-index, 00)
	*/
	std::vector<std::string> results;
	StringSplit(sLine, ",", results);
	//Single line of respone
	if (results.size() == 1)
	{
		_tOTGWStatus _status;
		int msgID = -1;
		std::string msgData = "";
		std::string msgType;

		_status.MsgID = results[0];
		// Listen only to messages from (B)oiler and remote (A) and not to (T)hermostat messages
		// Replies on remote commands (like TT: or PR:A) will be handeled later
		if (_status.MsgID.size() == 9 && (_status.MsgID[0] == 'B' || _status.MsgID[0] == 'A') && _status.MsgID[2] != ':')
		{
			msgID = std::stoi(std::string(_status.MsgID, 3, 2), nullptr, 16);

			// 16 bits data
			unsigned int msgDataInt = std::stoi(std::string(_status.MsgID, 5, 4), nullptr, 16); // Hexvalue --> integer
			msgData = std::bitset< 16>(msgDataInt).to_string(); // INT --> BIT string


			// y0xx  = Master to Slave 
			// y1xx  = Slave to master
			// xx=00 = read  xx=01 = write  xx=10 = invalid  xx=11 = reserved   y=parity
			int msgTypeInt = std::stoi(std::string(_status.MsgID, 1, 1), nullptr, 16);// Hexvalue --> integer

			msgType = std::bitset< 4>(msgTypeInt).to_string(); // INT --> BIT string
			//_log.Log(LOG_STATUS, "OTGW: %s", msgType.c_str());
			bool ignore =
				msgType[2] == '1' && msgType[3] == '1' ||						// Ignore invalid message x10
				msgType[2] == '1' && msgType[3] == '1';							// Ignore reserved or unknown message types 011
			if (ignore)
			{
				return;
			}

			//0    Status (MsgID=0) - Printed as two 8-bit bitfields
			if (msgID == 0)
			{
				if (msgType[1] == '0')
				{
					// HB Master sending to slave"0000 0011	 0000 0000"
					bool bCH_enabled = (msgData[7] == '1');					UpdateSwitch(101, bCH_enabled, "CH_enabled");
					bool bDHW_enabled = (msgData[6] == '1');				UpdateSwitch(102, bDHW_enabled, "DHW_enabled");
					bool bCooling_enable = (msgData[5] == '1');				UpdateSwitch(103, bCooling_enable, "Cooling_enable");
					bool bOTC_active = (msgData[4] == '1');					UpdateSwitch(104, bOTC_active, "OTC_active");
					bool bCH2_enabled = (msgData[3] == '1');				UpdateSwitch(105, bCH2_enabled, "CH2_enabled");
				}
				else
				{
					//LB Slave sending to master "0000 ?0011 0000 1100?
					bool bFault_indication = (msgData[8 + 7] == '1');			UpdateSwitch(110, bFault_indication, "Fault_indication");
					bool bCH_active = (msgData[8 + 6] == '1');				UpdateSwitch(111, bCH_active, "CH_active");
					bool bDHW_active = (msgData[8 + 5] == '1');				UpdateSwitch(112, bDHW_active, "DHW_active");
					bool bFlameOn = (msgData[8 + 4] == '1');					UpdateSwitch(113, bFlameOn, "FlameOn");
					bool bCooling_Mode_Active = (msgData[8 + 3] == '1');		UpdateSwitch(114, bCooling_Mode_Active, "Cooling_Mode_Active");
					bool bCH2_Active = (msgData[8 + 2] == '1');				UpdateSwitch(115, bCH2_Active, "CH2_Active");
					bool bDiagnosticEvent = (msgData[8 + 1] == '1');			UpdateSwitch(116, bDiagnosticEvent, "DiagnosticEvent");
				}
			}
			//1    Control setpoint (MsgID=1) - Printed as a floating point value
			else if (msgID == 1)
			{
				_status.Control_setpoint = msgDataToFloat(_status.MsgID);
				SendTempSensor(msgID, 255, _status.Control_setpoint, "Control Setpoint");
			}
			//2    Remote parameter flags (MsgID= 6) - Printed as two 8-bit bitfields
			else if (msgID == 6)
			{
				//_log.Log(LOG_STATUS, "OTGW: MsgID=6 %s", msgData);
				_status.Remote_parameter_flags = msgData;
			}
			//3    Maximum relative modulation level (MsgID=14) - Printed as a floating point value
			else if (msgID == 14)
			{
				_status.Maximum_relative_modulation_level = msgDataToFloat(_status.MsgID);
				bool bExists = CheckPercentageSensorExists(msgID, 1);
				if ((_status.Maximum_relative_modulation_level != 0) || (bExists))
				{
					SendPercentageSensor(msgID, 1, 255, _status.Maximum_relative_modulation_level, "Maximum Relative Modulation Level");
				}
			}
			//4    Boiler capacity and modulation limits (MsgID=15) - Printed as two bytes
			else if (msgID == 15)
			{
				_status.Boiler_capacity_and_modulation_limits = msgData;
			}
			//5    Room Setpoint (MsgID=16) - Printed as a floating point value
			else if (msgID == 16)
			{
				_status.Room_Setpoint = msgDataToFloat(_status.MsgID);
				UpdateSetPointSensor(msgID, ((m_OverrideTemperature != 0.0f) ? m_OverrideTemperature : _status.Room_Setpoint), "Room Setpoint");
			}
			//6    Relative modulation level (MsgID=17) - Printed as a floating point value
			else if (msgID == 17)
			{
				_status.Relative_modulation_level = msgDataToFloat(_status.MsgID);

				bool bExists = CheckPercentageSensorExists(msgID, 1);
				if ((_status.Relative_modulation_level != 0) || (bExists))
				{
					SendPercentageSensor(msgID, 1, 255, _status.Relative_modulation_level, "Relative modulation level");
				}
			}
			//7    CH water pressure (MsgID=18) - Printed as a floating point value
			else if (msgID == 18)
			{
				_status.CH_water_pressure = msgDataToFloat(_status.MsgID);
				if (_status.CH_water_pressure != 0)
				{
					SendPressureSensor(0, msgID, 255, _status.CH_water_pressure, "CH Water Pressure");
				}
			}
			//8    Room temperature (MsgID=24) - Printed as a floating point value
			else if (msgID == 24)
			{
				_status.Room_temperature = msgDataToFloat(_status.MsgID);
				SendTempSensor(msgID, 255, _status.Room_temperature, "Room Temperature");
			}
			//9    Boiler water temperature (MsgID=25) - Printed as a floating point value
			else if (msgID == 25)
			{
				_status.Boiler_water_temperature = msgDataToFloat(_status.MsgID);
				SendTempSensor(msgID, 255, _status.Boiler_water_temperature, "Boiler Water Temperature");
			}
			//10    DHW temperature (MsgID=26) - Printed as a floating point value
			else if (msgID == 26)
			{
				_status.DHW_temperature = msgDataToFloat(_status.MsgID);
				SendTempSensor(msgID, 255, _status.DHW_temperature, "DHW Temperature");
			}
			//11    Outside temperature (MsgID=27) - Printed as a floating point value
			else if (msgID == 27)
			{
				_status.Outside_temperature = msgDataToFloat(_status.MsgID);
				SendTempSensor(msgID, 255, _status.Outside_temperature, "Outside Temperature");
			}
			//12    Return water temperature (MsgID=28) - Printed as a floating point value
			else if (msgID == 28)
			{
				_status.Return_water_temperature = msgDataToFloat(_status.MsgID);
				SendTempSensor(msgID, 255, _status.Return_water_temperature, "Return Water Temperature");
			}
			//13    DHW setpoint boundaries (MsgID=48) - Printed as two bytes
			else if (msgID == 48)
			{
				_status.DHW_setpoint_boundaries = msgData;
			}
			//14    Max CH setpoint boundaries (MsgID=49) - Printed as two bytes
			else if (msgID == 49)
			{
				_status.Max_CH_setpoint_boundaries = msgData;
			}
			//15    DHW setpoint (MsgID=56) - Printed as a floating point value
			else if (msgID == 56)
			{
				_status.DHW_setpoint = msgDataToFloat(_status.MsgID);
				if (_status.DHW_setpoint != 0.0f)
				{
					UpdateSetPointSensor(msgID, _status.DHW_setpoint, "DHW Setpoint");
				}
			}
			//16    Max CH water setpoint (MsgID=57) - Printed as a floating point value
			else if (msgID == 57)
			{
				_status.Max_CH_water_setpoint = msgDataToFloat(_status.MsgID);
				if (_status.Max_CH_water_setpoint != 0.0f)
				{
					UpdateSetPointSensor(msgID, _status.Max_CH_water_setpoint, "Max_CH Water Setpoint");
				}
			}
			//17    Burner starts (MsgID=116) - Printed as a decimal value
			else if (msgID == 116)
			{
				_status.Burner_starts = msgDataToUInt(_status.MsgID);
			}
			//18    CH pump starts (MsgID=117) - Printed as a decimal value
			else if (msgID == 117)
			{
				_status.CH_pump_starts = msgDataToUInt(_status.MsgID);
			}
			//19    DHW pump/valve starts (MsgID=118) - Printed as a decimal value
			else if (msgID == 118)
			{
				_status.DHW_pump_valve_starts = msgDataToUInt(_status.MsgID);
			}
			//20    DHW burner starts (MsgID=119) - Printed as a decimal value
			else if (msgID == 119)
			{
				_status.DHW_burner_starts = msgDataToUInt(_status.MsgID);
			}
			//21    Burner operation hours (MsgID=120) - Printed as a decimal value
			else if (msgID == 120)
			{
				_status.Burner_operation_hours = msgDataToUInt(_status.MsgID);
			}
			//22    CH pump operation hours (MsgID=121) - Printed as a decimal value
			else if (msgID == 121)
			{
				_status.CH_pump_operation_hours = msgDataToUInt(_status.MsgID);
			}
			//23    DHW pump/valve operation hours (MsgID=122) - Printed as a decimal value
			else if (msgID == 122)
			{
				_status.DHW_pump_valve_operation_hours = msgDataToUInt(_status.MsgID);
			}
			//24    DHW burner operation hours (MsgID=123) - Printed as a decimal value
			else if (msgID == 123)
			{
				_status.DHW_burner_operation_hours = msgDataToUInt(_status.MsgID);
			}
			// Unhandeled IDs
			else
			{
				//std::string msg = sLine.c_str() + std::string(" |") + std::to_string(msgID) + std::string("|") + msgType +std::string("|") + msgData;
				//_log.Log(LOG_STATUS, "(OTGW) %s", msg.c_str());
			}
		}
		/*
		NG - No Good
		The command code is unknown.
		SE - Syntax Error
		The command contained an unexpected character or was incomplete.
		BV - Bad Value
		The command contained a data value that is not allowed.
		OR - Out of Range
		A number was specified outside of the allowed range.
		NS - No Space
		The alternative Data-ID could not be added because the table is full.
		NF - Not Found
		The specified alternative Data-ID could not be removed because it does not exist in the table.
		OE - Overrun Error.
		The processor was busy and failed to process all received characters.

		*/
		else if (sLine == "SE")
		{
			_log.Log(LOG_ERROR, "(OTGW) The command contained an unexpected character or was incomplete.");
		}
		else if (sLine.find("PR: G") != std::string::npos)
		{
			_tOTGWGPIO _GPIO;
			_GPIO.A = static_cast<int>(sLine[6] - '0');
			//			if (_GPIO.A==0 || _GPIO.A==1)
			//			{
			UpdateSwitch(96, (_GPIO.A == 1), "GPIOAPulledToGround");
			//			}
			//			else
			//			{
			// Remove device (how?)
			//			}
			_GPIO.B = static_cast<int>(sLine[7] - '0');
			//			if (_GPIO.B==0 || _GPIO.B==1)
			//			{
			UpdateSwitch(97, (_GPIO.B == 1), "GPIOBPulledToGround");
			//			}
			//			else
			//			{
			// Remove device (how?)
			//			}
		}
		/*
		PR=item Print Report - Request the gateway to report some information item. The following items are currently defined:
			A.	About opentherm gateway (prints the welcome message)
			B.	Build date and time
			C.	The clock speed the code was compiled for (4 MHz)
			G.	Configured functions for the two GPIO pins. The response will be 2 digits that represent the functions of GPIO A and GPIO B respectively.
			I.	Current state of the two GPIO pins. The response will be 2 digits that represent the level (0 or 1) of GPIO A and GPIO B respectively.
			L.	Configured functions for all 6 LEDS. The response consists of 6 letters representing the functions of LED A through LED F.
			M.	Gateway mode. G=Gateway, M=Monitor.
			O.	Report the setpoint override value
			P.	Current Smart-Power mode (low/medium/high).
			R.	The state of the automatic Remeha thermostat detection.
			S.	The configured setback temperature.
			T.	Tweaks. Reports the state of the ignore transitions and override in high byte settings.
			V.	Report the reference voltage setting
			W.	Report the domestic hot water setting
		*/
		else if (sLine.find("PR: I") != std::string::npos)
		{
			_tOTGWGPIO _GPIO;
			_GPIO.A = static_cast<int>(sLine[6] - '0');
			UpdateSwitch(98, (_GPIO.A == 1), "GPIOAStatusIsHigh");
			_GPIO.B = static_cast<int>(sLine[7] - '0');
			UpdateSwitch(99, (_GPIO.B == 1), "GPIOBStatusIsHigh");
		}
		else if (sLine.find("PR: O") != std::string::npos)
		{
			// Check if setpoint is overriden
			// PR: O=T14.50
			m_OverrideTemperature = 0.0f;
			char status = sLine[6];
			if (status == 'C' || status == 'T')
			{	
				// Get override setpoint value
				m_OverrideTemperature = static_cast<float>(atof(sLine.substr(7).c_str()));
 			//UpdateSetPointSensor(16, ((m_OverrideTemperature != 0.0f) ? m_OverrideTemperature : _status.Room_Setpoint), "Room Setpoint");
				_log.Log(LOG_STATUS, "(OTGW) Remote overide room setpoint %0.1f", m_OverrideTemperature);
			}
		}
		else if (sLine.find("PR: A") != std::string::npos)
		{
			//Gateway Version
			std::string tmpstr = sLine.substr(6);
			size_t tpos = tmpstr.find(' ');
			if (tpos != std::string::npos)
			{
				m_Version = tmpstr.substr(tpos + 9);
			}	
			_log.Log(LOG_STATUS, "(OTGW)  %s", tmpstr.c_str());
		}
		/*
		else if (
			(sLine.find("OT") == std::string::npos)&&
			(sLine.find("PS") == std::string::npos)&&
			(sLine.find("SC") == std::string::npos)
			)
		{
			//Dont report OT/PS/SC feedback
			//_log.Log(LOG_STATUS,"(OTGW) %s",sLine.c_str());
		} */
	
	}
	// PS=1
	else
	{
		_log.Log(LOG_STATUS, "(OTGW) %s", sLine.c_str());	
	}	
	//_log.Log(LOG_STATUS, "(OTGW) %s", sLine.c_str());
}


//Webserver helpers
namespace http {
	namespace server {
		void CWebServer::SetOpenThermSettings(WebEmSession & session, const request& req, std::string & redirect_uri)
		{
			redirect_uri = "/index.html";
			if (session.rights != 2)
			{
				session.reply_status = reply::forbidden;
				return; //Only admin user allowed
			}

			std::string idx = request::findValue(&req, "idx");
			if (idx == "") {
				return;
			}
			std::vector<std::vector<std::string> > result;

			result = m_sql.safe_query("SELECT Mode1, Mode2, Mode3, Mode4, Mode5, Mode6 FROM Hardware WHERE (ID='%q')", idx.c_str());
			if (result.size() < 1)
				return;


			int currentMode1 = atoi(result[0][0].c_str());

			std::string sOutsideTempSensor = request::findValue(&req, "combooutsidesensor");
			int newMode1 = atoi(sOutsideTempSensor.c_str());

			if (currentMode1 != newMode1)
			{
				m_sql.UpdateRFXCOMHardwareDetails(atoi(idx.c_str()), newMode1, 0, 0, 0, 0, 0);
				m_mainworker.RestartHardware(idx);
			}
		}
		void CWebServer::Cmd_SendOpenThermCommand(WebEmSession & session, const request& req, Json::Value &root)
		{
			if (session.rights != 2)
			{
				session.reply_status = reply::forbidden;
				return; //Only admin user allowed
			}

			std::string idx = request::findValue(&req, "idx");
			std::string cmnd = request::findValue(&req, "cmnd");
			if (idx.empty() || cmnd.empty())
			{
				return;
			}
			size_t tpos = cmnd.find('=');
			if (tpos != 2)
			{
				_log.Log(LOG_STATUS, "(OTGW) Invalid user command!: %s", cmnd.c_str());
				return;
			}
			std::string rcmnd = cmnd.substr(0, 2);
			std::string rdata = cmnd.substr(2);
			stdupper(rcmnd);
			cmnd = rcmnd + rdata;

			OTGWBase *pOTGW = reinterpret_cast<OTGWBase*>(m_mainworker.GetHardware(atoi(idx.c_str())));
			if (pOTGW == NULL)
				return;

			_log.Log(LOG_STATUS, "User: %s initiated a manual command: %s", session.username.c_str(), cmnd.c_str());

			cmnd += "\r\n";

			pOTGW->WriteInt((const unsigned char*)cmnd.c_str(), (const unsigned char)cmnd.size());
			root["status"] = "OK";
			root["title"] = "SendOpenThermCommand";
		}
	}
}
