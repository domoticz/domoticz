#include "stdafx.h"
#include "OTGWBase.h"
#include "../main/Logger.h"
#include "../main/Helper.h"
#include "../main/RFXtrx.h"
#include "../main/mainworker.h"
#include "P1MeterBase.h"
#include "hardwaretypes.h"
#include <string>
#include <algorithm>
#include <iostream>
#include <boost/bind.hpp>
#include "../json/json.h"

#include <ctime>

#define round(a) ( int ) ( a + .5 )

OTGWBase::OTGWBase(void)
{
	m_OutsideTemperatureIdx=0;//use build in
}

OTGWBase::~OTGWBase(void)
{
}

void OTGWBase::SetModes( const int Mode1, const int Mode2, const int Mode3, const int Mode4, const int Mode5)
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
	if (m_pMainWorker==NULL)
		return;
	bool bDeviceExits=true;
	char szIdx[10];
	sprintf(szIdx,"%X%02X%02X%02X",0,0,0,Idx);
	std::stringstream szQuery;
	std::vector<std::vector<std::string> > result;
	szQuery << "SELECT Name,nValue,sValue FROM DeviceStatus WHERE (HardwareID==" << m_HwdID << ") AND (DeviceID=='" << szIdx << "')";
	result=m_pMainWorker->m_sql.query(szQuery.str()); //-V519
	if (result.size()<1)
	{
		bDeviceExits=false;
	}
	else
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
	sDecodeRXMessage(this, (const unsigned char *)&lcmd.LIGHTING2);//decode message

	if (!bDeviceExits)
	{
		//Assign default name for device
		szQuery.clear();
		szQuery.str("");
		szQuery << "UPDATE DeviceStatus SET Name='" << defaultname << "' WHERE (HardwareID==" << m_HwdID << ") AND (DeviceID=='" << szIdx << "')";
		result=m_pMainWorker->m_sql.query(szQuery.str());
	}

}

void OTGWBase::UpdateTempSensor(const unsigned char Idx, const float Temp, const std::string &defaultname)
{
	if (m_pMainWorker==NULL)
		return;
	bool bDeviceExits=true;
	std::stringstream szQuery;
	std::vector<std::vector<std::string> > result;
	szQuery << "SELECT Name FROM DeviceStatus WHERE (HardwareID==" << m_HwdID << ") AND (DeviceID==" << int(Idx) << ")";
	result=m_pMainWorker->m_sql.query(szQuery.str()); //-V519
	if (result.size()<1)
	{
		bDeviceExits=false;
	}

	RBUF tsen;
	memset(&tsen,0,sizeof(RBUF));

	tsen.TEMP.packetlength=sizeof(tsen.TEMP)-1;
	tsen.TEMP.packettype=pTypeTEMP;
	tsen.TEMP.subtype=sTypeTEMP10;
	tsen.TEMP.battery_level=9;
	tsen.TEMP.rssi=12;
	tsen.TEMP.id1=0;
	tsen.TEMP.id2=Idx;

	tsen.TEMP.tempsign=(Temp>=0)?0:1;
	int at10=round(abs(Temp*10.0f));
	tsen.TEMP.temperatureh=(BYTE)(at10/256);
	at10-=(tsen.TEMP.temperatureh*256);
	tsen.TEMP.temperaturel=(BYTE)(at10);

	sDecodeRXMessage(this, (const unsigned char *)&tsen.TEMP);//decode message

	if (!bDeviceExits)
	{
		//Assign default name for device
		szQuery.clear();
		szQuery.str("");
		szQuery << "UPDATE DeviceStatus SET Name='" << defaultname << "' WHERE (HardwareID==" << m_HwdID << ") AND (DeviceID==" << int(Idx) << ")";
		result=m_pMainWorker->m_sql.query(szQuery.str());
	}
}

void OTGWBase::UpdateSetPointSensor(const unsigned char Idx, const float Temp, const std::string &defaultname)
{
	if (m_pMainWorker==NULL)
		return;
	bool bDeviceExits=true;
	std::stringstream szQuery;

	char szID[10];
	sprintf(szID,"%X%02X%02X%02X", 0, 0, 0, Idx);

	std::vector<std::vector<std::string> > result;
	szQuery << "SELECT Name FROM DeviceStatus WHERE (HardwareID==" << m_HwdID << ") AND (DeviceID=='" << szID << "')";
	result=m_pMainWorker->m_sql.query(szQuery.str());
	if (result.size()<1)
	{
		bDeviceExits=false;
	}

	_tThermostat thermos;
	thermos.subtype=sTypeThermSetpoint;
	thermos.id1=0;
	thermos.id2=0;
	thermos.id3=0;
	thermos.id4=Idx;
	thermos.dunit=0;

	thermos.temp=Temp;

	sDecodeRXMessage(this, (const unsigned char *)&thermos);

	if (!bDeviceExits)
	{
		//Assign default name for device
		szQuery.clear();
		szQuery.str("");
		szQuery << "UPDATE DeviceStatus SET Name='" << defaultname << "' WHERE (HardwareID==" << m_HwdID << ") AND (DeviceID=='" << szID << "')";
		result=m_pMainWorker->m_sql.query(szQuery.str());
	}
}

void OTGWBase::ParseLine()
{
	if (m_bufferpos<2)
		return;
	std::string sLine((char*)&m_buffer);

	std::vector<std::string> results;
	StringSplit(sLine,",",results);
	if (results.size()==25)
	{
		//status report
		//0    Status (MsgID=0) - Printed as two 8-bit bitfields
		//1    Control setpoint (MsgID=1) - Printed as a floating point value
		//2    Remote parameter flags (MsgID= 6) - Printed as two 8-bit bitfields
		//3    Maximum relative modulation level (MsgID=14) - Printed as a floating point value
		//4    Boiler capacity and modulation limits (MsgID=15) - Printed as two bytes
		//5    Room Setpoint (MsgID=16) - Printed as a floating point value
		//6    Relative modulation level (MsgID=17) - Printed as a floating point value
		//7    CH water pressure (MsgID=18) - Printed as a floating point value
		//8    Room temperature (MsgID=24) - Printed as a floating point value
		//9    Boiler water temperature (MsgID=25) - Printed as a floating point value
		//10    DHW temperature (MsgID=26) - Printed as a floating point value
		//11    Outside temperature (MsgID=27) - Printed as a floating point value
		//12    Return water temperature (MsgID=28) - Printed as a floating point value
		//13    DHW setpoint boundaries (MsgID=48) - Printed as two bytes
		//14    Max CH setpoint boundaries (MsgID=49) - Printed as two bytes
		//15    DHW setpoint (MsgID=56) - Printed as a floating point value
		//16    Max CH water setpoint (MsgID=57) - Printed as a floating point value
		//17    Burner starts (MsgID=116) - Printed as a decimal value
		//18    CH pump starts (MsgID=117) - Printed as a decimal value
		//19    DHW pump/valve starts (MsgID=118) - Printed as a decimal value
		//20    DHW burner starts (MsgID=119) - Printed as a decimal value
		//21    Burner operation hours (MsgID=120) - Printed as a decimal value
		//22    CH pump operation hours (MsgID=121) - Printed as a decimal value
		//23    DHW pump/valve operation hours (MsgID=122) - Printed as a decimal value
		//24    DHW burner operation hours (MsgID=123) - Printed as a decimal value 
		_tOTGWStatus _status;
		int idx=0;

		_status.MsgID=results[idx++];
		if (_status.MsgID.size()==17)
		{
			bool bCH_enabled=(_status.MsgID[7]=='1');										UpdateSwitch(101,bCH_enabled,"CH_enabled");
			bool bDHW_enabled=(_status.MsgID[6]=='1');										UpdateSwitch(102,bDHW_enabled,"DHW_enabled");
			bool bCooling_enable=(_status.MsgID[5]=='1');									UpdateSwitch(103,bCooling_enable,"Cooling_enable");
			bool bOTC_active=(_status.MsgID[4]=='1');										UpdateSwitch(104,bOTC_active,"OTC_active");
			bool bCH2_enabled=(_status.MsgID[3]=='1');										UpdateSwitch(105,bCH2_enabled,"CH2_enabled");

			bool bFault_indication=(_status.MsgID[9+7]=='1');								UpdateSwitch(110,bFault_indication,"Fault_indication");
			bool bCH_active=(_status.MsgID[9+6]=='1');										UpdateSwitch(111,bCH_active,"CH_active");
			bool bDHW_active=(_status.MsgID[9+5]=='1');										UpdateSwitch(112,bDHW_active,"DHW_active");
			bool bFlameOn=(_status.MsgID[9+4]=='1');										UpdateSwitch(113,bFlameOn,"FlameOn");
			bool bCooling_Mode_Active=(_status.MsgID[9+3]=='1');							UpdateSwitch(114,bCooling_Mode_Active,"Cooling_Mode_Active");
			bool bCH2_Active=(_status.MsgID[9+2]=='1');										UpdateSwitch(115,bCH2_Active,"CH2_Active");
			bool bDiagnosticEvent=(_status.MsgID[9+1]=='1');								UpdateSwitch(116,bDiagnosticEvent,"DiagnosticEvent");
		}
		
		_status.Control_setpoint=(float)atof(results[idx++].c_str());						UpdateTempSensor(idx-1,_status.Control_setpoint,"Control Setpoint");
		_status.Remote_parameter_flags=results[idx++];
		_status.Maximum_relative_modulation_level=(float)atof(results[idx++].c_str());
		_status.Boiler_capacity_and_modulation_limits=results[idx++];
		_status.Room_Setpoint=(float)atof(results[idx++].c_str());							UpdateSetPointSensor(idx-1,_status.Room_Setpoint,"Room Setpoint");
		_status.Relative_modulation_level=(float)atof(results[idx++].c_str());
		_status.CH_water_pressure=(float)atof(results[idx++].c_str());
		_status.Room_temperature=(float)atof(results[idx++].c_str());						UpdateTempSensor(idx-1,_status.Room_temperature,"Room Temperature");
		_status.Boiler_water_temperature=(float)atof(results[idx++].c_str());				UpdateTempSensor(idx-1,_status.Boiler_water_temperature,"Boiler Water Temperature");
		_status.DHW_temperature=(float)atof(results[idx++].c_str());						UpdateTempSensor(idx-1,_status.DHW_temperature,"DHW Temperature");
		_status.Outside_temperature=(float)atof(results[idx++].c_str());					UpdateTempSensor(idx-1,_status.Outside_temperature,"Outside Temperature");
		_status.Return_water_temperature=(float)atof(results[idx++].c_str());				UpdateTempSensor(idx-1,_status.Return_water_temperature,"Return Water Temperature");
		_status.DHW_setpoint_boundaries=results[idx++];
		_status.Max_CH_setpoint_boundaries=results[idx++];
		_status.DHW_setpoint=(float)atof(results[idx++].c_str());							UpdateSetPointSensor(idx-1,_status.DHW_setpoint,"DHW Setpoint");
		_status.Max_CH_water_setpoint=(float)atof(results[idx++].c_str());					UpdateSetPointSensor(idx-1,_status.Max_CH_water_setpoint,"Max_CH Water Setpoint");
		_status.Burner_starts=atol(results[idx++].c_str());
		_status.CH_pump_starts=atol(results[idx++].c_str());
		_status.DHW_pump_valve_starts=atol(results[idx++].c_str());
		_status.DHW_burner_starts=atol(results[idx++].c_str());
		_status.Burner_operation_hours=atol(results[idx++].c_str());
		_status.CH_pump_operation_hours=atol(results[idx++].c_str());
		_status.DHW_pump_valve_operation_hours=atol(results[idx++].c_str());
		_status.DHW_burner_operation_hours=atol(results[idx++].c_str());
		return;
	}
	else
	{
		if (sLine=="SE")
		{
			_log.Log(LOG_ERROR,"OTGW: Error received!");
		}
		else
		{
			if (
				(sLine.find("OT")==std::string::npos)&&
				(sLine.find("PS")==std::string::npos)
				)
			{
				//Dont report OT/PS feedback
				_log.Log(LOG_NORM,"OTGW: %s",sLine.c_str());
			}
		}
	}

}

bool OTGWBase::GetOutsideTemperatureFromDomoticz(float &tvalue)
{
	if (m_OutsideTemperatureIdx==0)
		return false;
	Json::Value tempjson;
	std::stringstream sstr;
	sstr << m_OutsideTemperatureIdx;
	m_pMainWorker->m_webserver.GetJSonDevices(tempjson, "", "temp","ID",sstr.str(),"");

	size_t tsize=tempjson.size();
	if (tsize<1)
		return false;

	Json::Value::const_iterator itt;
	int ii=0;
	Json::ArrayIndex rsize=tempjson["result"].size();
	if (rsize<1)
		return false;

	bool bHaveTimeout=tempjson["result"][0]["HaveTimeout"].asBool();
	if (bHaveTimeout)
		return false;
	tvalue=tempjson["result"][0]["Temp"].asFloat();
	return true;
}
