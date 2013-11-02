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

#include <ctime>

#define round(a) ( int ) ( a + .5 )

OTGWBase::OTGWBase(void)
{
}

OTGWBase::~OTGWBase(void)
{
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
	tsen.TEMP.rssi=6;
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
		_tOTGWStatus _status;
		int idx=0;

		_status.MsgID=results[idx++];
		_status.Control_setpoint=(float)atof(results[idx++].c_str());						UpdateTempSensor(idx-1,_status.Control_setpoint,"Control Setpoint");
		_status.Remote_parameter_flags=results[idx++];
		_status.Maximum_relative_modulation_level=(float)atof(results[idx++].c_str());
		_status.Boiler_capacity_and_modulation_limits=results[idx++];
		_status.Room_Setpoint=(float)atof(results[idx++].c_str());							UpdateTempSensor(idx-1,_status.Room_Setpoint,"Room Setpoint");
		_status.Relative_modulation_level=(float)atof(results[idx++].c_str());
		_status.CH_water_pressure=(float)atof(results[idx++].c_str());
		_status.Room_temperature=(float)atof(results[idx++].c_str());						UpdateTempSensor(idx-1,_status.Room_temperature,"Room Temperature");
		_status.Boiler_water_temperature=(float)atof(results[idx++].c_str());				UpdateTempSensor(idx-1,_status.Boiler_water_temperature,"Boiler Water Temperature");
		_status.DHW_temperature=(float)atof(results[idx++].c_str());						UpdateTempSensor(idx-1,_status.DHW_temperature,"DHW Temperature");
		_status.Outside_temperature=(float)atof(results[idx++].c_str());					UpdateTempSensor(idx-1,_status.Outside_temperature,"Outside Temperature");
		_status.Return_water_temperature=(float)atof(results[idx++].c_str());				UpdateTempSensor(idx-1,_status.Return_water_temperature,"Return Water Temperature");
		_status.DHW_setpoint_boundaries=results[idx++];
		_status.Max_CH_setpoint_boundaries=results[idx++];
		_status.DHW_setpoint=(float)atof(results[idx++].c_str());							UpdateTempSensor(idx-1,_status.DHW_setpoint,"DHW Setpoint");
		_status.Max_CH_water_setpoint=(float)atof(results[idx++].c_str());					UpdateTempSensor(idx-1,_status.Max_CH_water_setpoint,"Max_CH Water Setpoint");
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
	}

}
