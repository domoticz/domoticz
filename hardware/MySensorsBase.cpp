#include "stdafx.h"
#include "MySensorsBase.h"
#include "../main/Logger.h"
#include "../main/Helper.h"
#include "../main/RFXtrx.h"
#include "../main/SQLHelper.h"
#include "../main/localtime_r.h"
#include "hardwaretypes.h"
#include <string>
#include <algorithm>
#include <iostream>
#include <boost/bind.hpp>

#include <ctime>

#define round(a) ( int ) ( a + .5 )

enum _eSetType
{
	V_TEMP=0,			//	Temperature
	V_HUM=1,			//	Humidity
	V_LIGHT=2,			//	Light status. 0 = off 1 = on
	V_DIMMER=3,			//	Dimmer value. 0 - 100 %
	V_PRESSURE=4,		//	Atmospheric Pressure
	V_FORECAST=5,		//	Whether forecast.One of "stable", "sunny", "cloudy", "unstable", "thunderstorm" or "unknown"
	V_RAIN=6,			//	Amount of rain
	V_RAINRATE=7,		//	Rate of rain
	V_WIND=8,			//	Windspeed
	V_GUST=9,			//	Gust
	V_DIRECTION=10,		//	Wind direction
	V_UV=11,			//	UV light level
	V_WEIGHT=12,		//	Weight(for scales etc)
	V_DISTANCE=13,		//	Distance
	V_IMPEDANCE=14,		//	Impedance value
	V_ARMED=15,			//	Armed status of a security sensor. 1 = Armed, 0 = Bypassed
	V_TRIPPED=16,		//	Tripped status of a security sensor. 1 = Tripped, 0 = Untripped
	V_WATT=17,			//	Watt value for power meters
	V_KWH=18,			//	Accumulated number of KWH for a power meter
	V_SCENE_ON=19,		//	Turn on a scene
	V_SCENE_OFF=20,		//	Turn of a scene
	V_HEATER=21,		//	Mode of header.One of "Off", "HeatOn", "CoolOn", or "AutoChangeOver"
	V_HEATER_SW=22,		//	Heater switch power. 1 = On, 0 = Off
	V_LIGHT_LEVEL=23,	//	Light level. 0 - 100 %
	V_VAR1=24,			//	Custom value
	V_VAR2=25,			//	Custom value
	V_VAR3=26,			//	Custom value
	V_VAR4=27,			//	Custom value
	V_VAR5=28,			//	Custom value
	V_UP=29,			//	Window covering.Up.
	V_DOWN=30,			//	Window covering.Down.
	V_STOP=31,			//	Window covering.Stop.
	V_IR_SEND=32,		//	Send out an IR - command
	V_IR_RECEIVE=33,	//	This message contains a received IR - command
	V_FLOW=34,			//	Flow of water(in meter)
	V_VOLUME=35,		//	Water volume
	V_LOCK_STATUS=36,	//	Set or get lock status. 1 = Locked, 0 = Unlocked
	V_DUST_LEVEL=37,	//	Dust level
	V_VOLTAGE=38,		//	Voltage level
	V_CURRENT=39,		//	Current level
};

MySensorsBase::MySensorsBase(void)
{
	m_bufferpos = 0;
}


MySensorsBase::~MySensorsBase(void)
{
}

void MySensorsBase::ParseData(const unsigned char *pData, int Len)
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

void MySensorsBase::UpdateSwitch(const unsigned char Idx, const int SubUnit, const bool bOn, const double Level, const std::string &defaultname)
{
	bool bDeviceExits = true;
	char szIdx[10];
	sprintf(szIdx, "%X%02X%02X%02X", 0, 0, 0, Idx);
	std::stringstream szQuery;
	std::vector<std::vector<std::string> > result;
	szQuery << "SELECT Name,nValue,sValue FROM DeviceStatus WHERE (HardwareID==" << m_HwdID << ") AND (DeviceID=='" << szIdx << "')";
	result = m_sql.query(szQuery.str()); //-V519
	if (result.size() < 1)
	{
		bDeviceExits = false;
	}
	else
	{
		//check if we have a change, if not do not update it
		int nvalue = atoi(result[0][1].c_str());
		if ((!bOn) && (nvalue == 0))
			return;
		if ((bOn && (nvalue != 0)))
			return;
	}

	//Send as Lighting 2
	tRBUF lcmd;
	memset(&lcmd, 0, sizeof(RBUF));
	lcmd.LIGHTING2.packetlength = sizeof(lcmd.LIGHTING2) - 1;
	lcmd.LIGHTING2.packettype = pTypeLighting2;
	lcmd.LIGHTING2.subtype = sTypeAC;
	lcmd.LIGHTING2.id1 = 0;
	lcmd.LIGHTING2.id2 = 0;
	lcmd.LIGHTING2.id3 = 0;
	lcmd.LIGHTING2.id4 = Idx;
	lcmd.LIGHTING2.unitcode = 1;
	double rlevel = (15.0 / 100)*Level;
	int level = int(rlevel);
	if (!bOn)
	{
		level = 0;
		lcmd.LIGHTING2.cmnd = light2_sOff;
	}
	else
	{
		lcmd.LIGHTING2.cmnd = light2_sOn;
	}
	lcmd.LIGHTING2.level = level;
	lcmd.LIGHTING2.filler = 0;
	lcmd.LIGHTING2.rssi = 12;
	sDecodeRXMessage(this, (const unsigned char *)&lcmd.LIGHTING2);//decode message

	if (!bDeviceExits)
	{
		//Assign default name for device
		szQuery.clear();
		szQuery.str("");
		szQuery << "UPDATE DeviceStatus SET Name='" << defaultname << "' WHERE (HardwareID==" << m_HwdID << ") AND (DeviceID=='" << szIdx << "')";
		m_sql.query(szQuery.str());
	}

}

void MySensorsBase::ParseLine()
{
	if (m_bufferpos<2)
		return;
	std::string sLine((char*)&m_buffer);

	std::vector<std::string> results;
	StringSplit(sLine, ";", results);
	if (results.size()!=6)
		return; //invalid data

	int node_id = atoi(results[0].c_str());
	int child_sensor_id = atoi(results[1].c_str());
	int message_type = atoi(results[2].c_str());
	int ack = atoi(results[3].c_str());
	int sub_type = atoi(results[4].c_str());
	std::string payload = results[5];

	if (message_type != 1)
		return; //dont want you

	switch (sub_type)
	{
	case V_TEMP:
	{
		float temperature = (float)atof(payload.c_str());
		RBUF tsen;
		memset(&tsen, 0, sizeof(RBUF));
		tsen.TEMP.packetlength = sizeof(tsen.TEMP) - 1;
		tsen.TEMP.packettype = pTypeTEMP;
		tsen.TEMP.subtype = sTypeTEMP5;
		tsen.TEMP.battery_level = 9;
		tsen.TEMP.rssi = 12;
		tsen.TEMP.id1 = (BYTE)0;
		tsen.TEMP.id2 = (BYTE)node_id;

		tsen.TEMP.tempsign = (temperature >= 0) ? 0 : 1;
		int at10 = round(abs(temperature*10.0f));
		tsen.TEMP.temperatureh = (BYTE)(at10 / 256);
		at10 -= (tsen.TEMP.temperatureh * 256);
		tsen.TEMP.temperaturel = (BYTE)(at10);

		sDecodeRXMessage(this, (const unsigned char *)&tsen.TEMP);//decode message
	}
	break;
	case V_HUM:
	{
		float humidity = (float)atof(payload.c_str());
		RBUF tsen;
		memset(&tsen, 0, sizeof(RBUF));
		tsen.HUM.packetlength = sizeof(tsen.HUM) - 1;
		tsen.HUM.packettype = pTypeHUM;
		tsen.HUM.subtype = sTypeHUM1;
		tsen.HUM.battery_level = 9;
		tsen.HUM.rssi = 12;
		tsen.TEMP.id1 = (BYTE)0;
		tsen.TEMP.id2 = (BYTE)node_id;

		tsen.HUM.humidity = (BYTE)round(humidity);
		tsen.HUM.humidity_status = Get_Humidity_Level(tsen.HUM.humidity);

		sDecodeRXMessage(this, (const unsigned char *)&tsen.HUM);//decode message
	}
	break;
	case V_PRESSURE:
	{
		float pressure = (float)atof(payload.c_str());
		_tGeneralDevice gdevice;
		gdevice.subtype = sTypePressure;
		gdevice.floatval1 = pressure;
		sDecodeRXMessage(this, (const unsigned char *)&gdevice);
	}
	break;
	case V_VAR1: //Custom value
	case V_VAR2:
	case V_VAR3:
	case V_VAR4:
	case V_VAR5:
		_log.Log(LOG_STATUS, "MySensors: Custom vars ignored! Node: %d, Child: %d, Payload: %s",node_id, child_sensor_id, payload.c_str());
		break;
	case V_TRIPPED:
		//	Tripped status of a security sensor. 1 = Tripped, 0 = Untripped
		UpdateSwitch(node_id, child_sensor_id, (payload == "1"), 100,  "Security Sensor");
		break;
	case V_LIGHT:
		//	Light status. 0 = off 1 = on
		UpdateSwitch(node_id, child_sensor_id, (atoi(payload.c_str())!=0), 100, "Light");
		break;
	case V_DIMMER:
		//	Dimmer value. 0 - 100 %
		{
			double level = atof(payload.c_str());
			UpdateSwitch(node_id, child_sensor_id, (atoi(payload.c_str()) != 0), level, "Light");
		}
		break;
	default:
		_log.Log(LOG_ERROR, "MySensors: Unhandled sensor, please report with log!");
		break;
	}
}

